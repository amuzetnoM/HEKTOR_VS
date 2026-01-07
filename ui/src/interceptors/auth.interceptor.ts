import { HttpInterceptorFn, HttpRequest, HttpHandlerFn, HttpErrorResponse } from '@angular/common/http';
import { inject } from '@angular/core';
import { AuthService } from '../services/auth.service';
import { catchError, throwError } from 'rxjs';

/**
 * HTTP Interceptor for adding JWT Authorization headers
 * 
 * This interceptor:
 * 1. Adds Bearer token to all API requests
 * 2. Handles 401 responses by redirecting to login
 * 3. Optionally triggers token refresh on 401
 */
export const authInterceptor: HttpInterceptorFn = (req: HttpRequest<unknown>, next: HttpHandlerFn) => {
    const authService = inject(AuthService);

    // Skip auth for public endpoints
    const publicUrls = ['/auth/login', '/auth/register', '/auth/refresh', '/health'];
    const isPublic = publicUrls.some(url => req.url.includes(url));

    if (isPublic) {
        return next(req);
    }

    // Get token and add header
    const token = authService.getToken();

    if (token) {
        req = req.clone({
            setHeaders: {
                Authorization: `Bearer ${token}`
            }
        });
    }

    return next(req).pipe(
        catchError((error: HttpErrorResponse) => {
            if (error.status === 401) {
                // Token expired or invalid
                console.warn('Received 401 Unauthorized - logging out');
                authService.logout();

                // Optionally redirect to login
                // In Angular, you might want to use Router here
                // For now, we just clear the auth state
            }

            if (error.status === 403) {
                console.warn('Received 403 Forbidden - insufficient permissions');
            }

            return throwError(() => error);
        })
    );
};

/**
 * Alternative class-based interceptor (for older Angular versions)
 */
// @Injectable()
// export class AuthInterceptor implements HttpInterceptor {
//   constructor(private authService: AuthService) {}
//
//   intercept(req: HttpRequest<any>, next: HttpHandler): Observable<HttpEvent<any>> {
//     const token = this.authService.getToken();
//     if (token) {
//       req = req.clone({
//         setHeaders: { Authorization: `Bearer ${token}` }
//       });
//     }
//     return next.handle(req);
//   }
// }
