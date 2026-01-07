import { Injectable, signal, computed, effect } from '@angular/core';
import { environment } from '../environments/environment';
import {
    decodeJwt,
    isTokenExpired,
    getTimeUntilExpiry,
    getUserFromToken,
    generateMockJwt,
    JwtPayload
} from '../utils/jwt.utils';

export interface User {
    id: string;
    email: string;
    name: string;
    role: string;
}

export interface LoginCredentials {
    email: string;
    password: string;
    rememberMe?: boolean;
}

export interface AuthState {
    user: User | null;
    isAuthenticated: boolean;
    isLoading: boolean;
    error: string | null;
}

@Injectable({
    providedIn: 'root'
})
export class AuthService {
    // Reactive State
    private _token = signal<string | null>(null);
    private _refreshToken = signal<string | null>(null);
    private _user = signal<User | null>(null);
    private _isLoading = signal(false);
    private _error = signal<string | null>(null);

    // Public computed signals
    isAuthenticated = computed(() => {
        const token = this._token();
        return token !== null && !isTokenExpired(token);
    });

    user = computed(() => this._user());
    isLoading = computed(() => this._isLoading());
    error = computed(() => this._error());

    // Auto-refresh timer
    private refreshTimer: ReturnType<typeof setTimeout> | null = null;

    constructor() {
        // Initialize from storage on startup
        this.initFromStorage();

        // Set up auto-refresh when token changes
        effect(() => {
            const token = this._token();
            if (token && !isTokenExpired(token)) {
                this.scheduleTokenRefresh(token);
            }
        });
    }

    /**
     * Initialize auth state from localStorage
     */
    private initFromStorage(): void {
        try {
            const token = localStorage.getItem(environment.tokenKey);
            const refreshToken = localStorage.getItem(environment.refreshTokenKey);

            if (token && !isTokenExpired(token)) {
                this._token.set(token);
                this._refreshToken.set(refreshToken);

                const userInfo = getUserFromToken(token);
                if (userInfo) {
                    this._user.set({
                        id: userInfo.id,
                        email: userInfo.email || '',
                        name: userInfo.name || '',
                        role: userInfo.role || 'user'
                    });
                }
            } else if (token) {
                // Token expired, clear storage
                this.clearTokens();
            }
        } catch (e) {
            console.warn('Failed to restore auth state:', e);
            this.clearTokens();
        }
    }

    /**
     * Login with email and password
     */
    async login(credentials: LoginCredentials): Promise<boolean> {
        this._isLoading.set(true);
        this._error.set(null);

        try {
            if (environment.enableMockAuth) {
                // Development: Use mock authentication
                return await this.mockLogin(credentials);
            }

            // Production: Call backend API
            const response = await fetch(`${environment.apiUrl}/auth/login`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(credentials)
            });

            if (!response.ok) {
                const error = await response.json();
                throw new Error(error.message || 'Login failed');
            }

            const data = await response.json();
            this.setTokens(data.token, data.refreshToken, credentials.rememberMe);

            return true;
        } catch (e: any) {
            this._error.set(e.message || 'Login failed');
            return false;
        } finally {
            this._isLoading.set(false);
        }
    }

    /**
     * Mock login for development
     */
    private async mockLogin(credentials: LoginCredentials): Promise<boolean> {
        // Simulate network delay
        await new Promise(resolve => setTimeout(resolve, 500));

        // Simple validation (dev only)
        if (!credentials.email.includes('@')) {
            throw new Error('Invalid email format');
        }

        if (credentials.password.length < 4) {
            throw new Error('Password must be at least 4 characters');
        }

        // Generate mock JWT
        const mockUser = {
            id: `user_${Date.now()}`,
            email: credentials.email,
            name: credentials.email.split('@')[0],
            role: credentials.email.includes('admin') ? 'admin' : 'user'
        };

        const token = generateMockJwt(mockUser);
        this.setTokens(token, null, credentials.rememberMe);

        console.log('Mock login successful:', mockUser);
        return true;
    }

    /**
     * Logout and clear all auth state
     */
    logout(): void {
        this.clearTokens();
        this.cancelRefreshTimer();

        // Optionally call backend to invalidate token
        if (!environment.enableMockAuth) {
            fetch(`${environment.apiUrl}/auth/logout`, {
                method: 'POST',
                headers: this.getAuthHeaders()
            }).catch(() => { }); // Ignore errors
        }
    }

    /**
     * Refresh the access token
     */
    async refreshAccessToken(): Promise<boolean> {
        const refreshToken = this._refreshToken();

        if (!refreshToken) {
            this.logout();
            return false;
        }

        try {
            if (environment.enableMockAuth) {
                // Mock refresh: Just extend the current token
                const user = this._user();
                if (user) {
                    const newToken = generateMockJwt(user);
                    this.setTokens(newToken, refreshToken, true);
                    return true;
                }
                return false;
            }

            const response = await fetch(`${environment.apiUrl}/auth/refresh`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ refreshToken })
            });

            if (!response.ok) {
                throw new Error('Token refresh failed');
            }

            const data = await response.json();
            this.setTokens(data.token, data.refreshToken || refreshToken, true);

            return true;
        } catch (e) {
            console.warn('Token refresh failed:', e);
            this.logout();
            return false;
        }
    }

    /**
     * Get current access token
     */
    getToken(): string | null {
        return this._token();
    }

    /**
     * Get authorization headers for HTTP requests
     */
    getAuthHeaders(): Record<string, string> {
        const token = this._token();
        if (!token) {
            return {};
        }
        return {
            'Authorization': `Bearer ${token}`
        };
    }

    /**
     * Check if user has a specific role
     */
    hasRole(role: string): boolean {
        const user = this._user();
        return user?.role === role;
    }

    /**
     * Check if user has any of the specified permissions
     */
    hasPermission(permission: string): boolean {
        const token = this._token();
        if (!token) return false;

        const payload = decodeJwt(token);
        return payload?.permissions?.includes(permission) || false;
    }

    // --- Private Helpers ---

    private setTokens(token: string, refreshToken: string | null, persist: boolean = true): void {
        this._token.set(token);
        this._refreshToken.set(refreshToken);

        // Extract user from token
        const userInfo = getUserFromToken(token);
        if (userInfo) {
            this._user.set({
                id: userInfo.id,
                email: userInfo.email || '',
                name: userInfo.name || '',
                role: userInfo.role || 'user'
            });
        }

        // Persist to storage
        if (persist) {
            localStorage.setItem(environment.tokenKey, token);
            if (refreshToken) {
                localStorage.setItem(environment.refreshTokenKey, refreshToken);
            }
        }
    }

    private clearTokens(): void {
        this._token.set(null);
        this._refreshToken.set(null);
        this._user.set(null);
        localStorage.removeItem(environment.tokenKey);
        localStorage.removeItem(environment.refreshTokenKey);
    }

    private scheduleTokenRefresh(token: string): void {
        this.cancelRefreshTimer();

        const timeUntilExpiry = getTimeUntilExpiry(token);
        const refreshTime = timeUntilExpiry - environment.tokenExpiryBuffer;

        if (refreshTime > 0) {
            this.refreshTimer = setTimeout(() => {
                this.refreshAccessToken();
            }, refreshTime);
        }
    }

    private cancelRefreshTimer(): void {
        if (this.refreshTimer) {
            clearTimeout(this.refreshTimer);
            this.refreshTimer = null;
        }
    }
}
