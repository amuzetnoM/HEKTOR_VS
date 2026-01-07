/**
 * JWT Utilities
 * 
 * Client-side JWT decoding and validation utilities.
 * NOTE: These are for UX only - all security verification must happen server-side.
 */

export interface JwtPayload {
    sub: string;           // Subject (user ID)
    email?: string;
    name?: string;
    role?: string;
    permissions?: string[];
    iat: number;           // Issued at (Unix timestamp)
    exp: number;           // Expiration (Unix timestamp)
    iss?: string;          // Issuer
    aud?: string;          // Audience
}

/**
 * Decode a JWT token without verification
 * @param token JWT string
 * @returns Decoded payload or null if invalid
 */
export function decodeJwt(token: string): JwtPayload | null {
    try {
        const parts = token.split('.');
        if (parts.length !== 3) {
            return null;
        }

        // Base64Url decode the payload (middle part)
        const payload = parts[1];
        const decoded = atob(payload.replace(/-/g, '+').replace(/_/g, '/'));
        return JSON.parse(decoded);
    } catch (e) {
        console.warn('Failed to decode JWT:', e);
        return null;
    }
}

/**
 * Check if a JWT token is expired
 * @param token JWT string
 * @param bufferMs Buffer time in milliseconds (default 0)
 * @returns true if expired or invalid
 */
export function isTokenExpired(token: string, bufferMs: number = 0): boolean {
    const payload = decodeJwt(token);
    if (!payload || !payload.exp) {
        return true;
    }

    const expirationTime = payload.exp * 1000; // Convert to milliseconds
    return Date.now() >= (expirationTime - bufferMs);
}

/**
 * Get the expiration date of a JWT token
 * @param token JWT string
 * @returns Date object or null if invalid
 */
export function getTokenExpirationDate(token: string): Date | null {
    const payload = decodeJwt(token);
    if (!payload || !payload.exp) {
        return null;
    }
    return new Date(payload.exp * 1000);
}

/**
 * Get time until token expires
 * @param token JWT string
 * @returns Milliseconds until expiry, or 0 if expired/invalid
 */
export function getTimeUntilExpiry(token: string): number {
    const payload = decodeJwt(token);
    if (!payload || !payload.exp) {
        return 0;
    }

    const expirationTime = payload.exp * 1000;
    const remaining = expirationTime - Date.now();
    return Math.max(0, remaining);
}

/**
 * Extract user info from JWT payload
 * @param token JWT string
 * @returns User info object or null
 */
export function getUserFromToken(token: string): { id: string; email?: string; name?: string; role?: string } | null {
    const payload = decodeJwt(token);
    if (!payload) {
        return null;
    }

    return {
        id: payload.sub,
        email: payload.email,
        name: payload.name,
        role: payload.role
    };
}

/**
 * Generate a mock JWT for development/testing
 * NOTE: This is NOT secure and should only be used in development
 */
export function generateMockJwt(user: { id: string; email: string; name: string; role?: string }): string {
    const header = { alg: 'HS256', typ: 'JWT' };
    const payload: JwtPayload = {
        sub: user.id,
        email: user.email,
        name: user.name,
        role: user.role || 'user',
        iat: Math.floor(Date.now() / 1000),
        exp: Math.floor(Date.now() / 1000) + (24 * 60 * 60), // 24 hours
        iss: 'hektor-dev',
        aud: 'hektor-ui'
    };

    // Base64Url encode (not cryptographically signed - dev only!)
    const base64Header = btoa(JSON.stringify(header)).replace(/=/g, '').replace(/\+/g, '-').replace(/\//g, '_');
    const base64Payload = btoa(JSON.stringify(payload)).replace(/=/g, '').replace(/\+/g, '-').replace(/\//g, '_');
    const mockSignature = 'dev_signature_not_valid';

    return `${base64Header}.${base64Payload}.${mockSignature}`;
}
