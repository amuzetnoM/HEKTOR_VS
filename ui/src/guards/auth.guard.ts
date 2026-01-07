import { inject } from '@angular/core';
import { CanActivateFn, Router, ActivatedRouteSnapshot, RouterStateSnapshot } from '@angular/router';
import { AuthService } from '../services/auth.service';

/**
 * Auth Guard - Protects routes from unauthenticated access
 * 
 * Usage in routes:
 * { path: 'dashboard', component: DashboardComponent, canActivate: [authGuard] }
 */
export const authGuard: CanActivateFn = (
    route: ActivatedRouteSnapshot,
    state: RouterStateSnapshot
) => {
    const authService = inject(AuthService);
    const router = inject(Router);

    if (authService.isAuthenticated()) {
        return true;
    }

    // Store the attempted URL for redirecting after login
    const returnUrl = state.url;

    // Redirect to login with return URL
    router.navigate(['/login'], {
        queryParams: { returnUrl }
    });

    return false;
};

/**
 * Role Guard - Protects routes based on user role
 * 
 * Usage in routes:
 * { path: 'admin', component: AdminComponent, canActivate: [roleGuard], data: { roles: ['admin'] } }
 */
export const roleGuard: CanActivateFn = (
    route: ActivatedRouteSnapshot,
    state: RouterStateSnapshot
) => {
    const authService = inject(AuthService);
    const router = inject(Router);

    // First check authentication
    if (!authService.isAuthenticated()) {
        router.navigate(['/login'], {
            queryParams: { returnUrl: state.url }
        });
        return false;
    }

    // Check required roles
    const requiredRoles = route.data['roles'] as string[] | undefined;

    if (!requiredRoles || requiredRoles.length === 0) {
        return true;
    }

    const user = authService.user();
    const hasRole = user && requiredRoles.includes(user.role);

    if (!hasRole) {
        console.warn('Access denied: insufficient role');
        router.navigate(['/unauthorized']);
        return false;
    }

    return true;
};

/**
 * Guest Guard - Only allows unauthenticated users (e.g., for login page)
 * 
 * Usage in routes:
 * { path: 'login', component: LoginComponent, canActivate: [guestGuard] }
 */
export const guestGuard: CanActivateFn = (
    route: ActivatedRouteSnapshot,
    state: RouterStateSnapshot
) => {
    const authService = inject(AuthService);
    const router = inject(Router);

    if (!authService.isAuthenticated()) {
        return true;
    }

    // Already authenticated, redirect to home
    router.navigate(['/']);
    return false;
};
