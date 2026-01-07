import { Component, inject, signal } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { AuthService, LoginCredentials } from '../services/auth.service';

@Component({
    selector: 'app-login',
    standalone: true,
    imports: [CommonModule, FormsModule],
    template: `
    <div class="min-h-screen flex items-center justify-center bg-gradient-to-br from-zinc-950 via-zinc-900 to-zinc-950 p-4">
      <!-- Ambient Glow Effect -->
      <div class="absolute inset-0 overflow-hidden pointer-events-none">
        <div class="absolute top-1/4 left-1/4 w-96 h-96 bg-primary/10 rounded-full blur-3xl animate-pulse"></div>
        <div class="absolute bottom-1/4 right-1/4 w-96 h-96 bg-purple-500/10 rounded-full blur-3xl animate-pulse" style="animation-delay: 1s"></div>
      </div>

      <!-- Login Card -->
      <div class="relative w-full max-w-md">
        <!-- Card -->
        <div class="bg-zinc-900/80 backdrop-blur-xl border border-zinc-800 rounded-2xl shadow-2xl overflow-hidden">
          
          <!-- Header -->
          <div class="px-8 pt-8 pb-4 text-center">
            <!-- Logo -->
            <div class="w-16 h-16 mx-auto mb-4 bg-gradient-to-br from-primary to-purple-500 rounded-xl flex items-center justify-center shadow-lg shadow-primary/25">
              <svg xmlns="http://www.w3.org/2000/svg" class="h-8 w-8 text-white" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M20 7l-8-4-8 4m16 0l-8 4m8-4v10l-8 4m0-10L4 7m8 4v10M4 7v10l8 4" />
              </svg>
            </div>
            
            <h1 class="text-2xl font-bold text-white mb-1">HEKTOR</h1>
            <p class="text-sm text-zinc-400">Vector Operations Console</p>
          </div>

          <!-- Form -->
          <form (ngSubmit)="onSubmit()" class="px-8 pb-8 space-y-5">
            
            <!-- Error Alert -->
            @if (auth.error()) {
              <div class="p-3 bg-red-500/10 border border-red-500/30 rounded-lg flex items-center gap-2 text-red-400 text-sm animate-shake">
                <svg xmlns="http://www.w3.org/2000/svg" class="h-4 w-4 flex-shrink-0" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                  <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 8v4m0 4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z" />
                </svg>
                <span>{{ auth.error() }}</span>
              </div>
            }

            <!-- Email Input -->
            <div class="space-y-1.5">
              <label class="block text-xs font-medium text-zinc-400 uppercase tracking-wide">Email</label>
              <div class="relative">
                <input 
                  type="email" 
                  [(ngModel)]="email" 
                  name="email"
                  required
                  placeholder="you@company.com"
                  class="w-full px-4 py-3 bg-zinc-800/50 border border-zinc-700 rounded-lg text-white placeholder-zinc-500 focus:border-primary focus:ring-1 focus:ring-primary focus:outline-none transition-all"
                  [class.border-red-500]="emailError()"
                />
                <div class="absolute right-3 top-1/2 -translate-y-1/2 text-zinc-500">
                  <svg xmlns="http://www.w3.org/2000/svg" class="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                    <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M16 12a4 4 0 10-8 0 4 4 0 008 0zm0 0v1.5a2.5 2.5 0 005 0V12a9 9 0 10-9 9m4.5-1.206a8.959 8.959 0 01-4.5 1.207" />
                  </svg>
                </div>
              </div>
            </div>

            <!-- Password Input -->
            <div class="space-y-1.5">
              <label class="block text-xs font-medium text-zinc-400 uppercase tracking-wide">Password</label>
              <div class="relative">
                <input 
                  [type]="showPassword() ? 'text' : 'password'" 
                  [(ngModel)]="password" 
                  name="password"
                  required
                  placeholder="••••••••"
                  class="w-full px-4 py-3 bg-zinc-800/50 border border-zinc-700 rounded-lg text-white placeholder-zinc-500 focus:border-primary focus:ring-1 focus:ring-primary focus:outline-none transition-all pr-12"
                  [class.border-red-500]="passwordError()"
                />
                <button 
                  type="button"
                  (click)="showPassword.set(!showPassword())"
                  class="absolute right-3 top-1/2 -translate-y-1/2 text-zinc-500 hover:text-zinc-300 transition-colors"
                >
                  @if (showPassword()) {
                    <svg xmlns="http://www.w3.org/2000/svg" class="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                      <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M13.875 18.825A10.05 10.05 0 0112 19c-4.478 0-8.268-2.943-9.543-7a9.97 9.97 0 011.563-3.029m5.858.908a3 3 0 114.243 4.243M9.878 9.878l4.242 4.242M9.88 9.88l-3.29-3.29m7.532 7.532l3.29 3.29M3 3l3.59 3.59m0 0A9.953 9.953 0 0112 5c4.478 0 8.268 2.943 9.543 7a10.025 10.025 0 01-4.132 5.411m0 0L21 21" />
                    </svg>
                  } @else {
                    <svg xmlns="http://www.w3.org/2000/svg" class="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                      <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M15 12a3 3 0 11-6 0 3 3 0 016 0z" />
                      <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M2.458 12C3.732 7.943 7.523 5 12 5c4.478 0 8.268 2.943 9.542 7-1.274 4.057-5.064 7-9.542 7-4.477 0-8.268-2.943-9.542-7z" />
                    </svg>
                  }
                </button>
              </div>
            </div>

            <!-- Remember Me & Forgot Password -->
            <div class="flex items-center justify-between text-sm">
              <label class="flex items-center gap-2 cursor-pointer group">
                <input 
                  type="checkbox" 
                  [(ngModel)]="rememberMe" 
                  name="rememberMe"
                  class="w-4 h-4 rounded border-zinc-600 bg-zinc-800 text-primary focus:ring-primary focus:ring-offset-0"
                />
                <span class="text-zinc-400 group-hover:text-zinc-300 transition-colors">Remember me</span>
              </label>
              <a href="#" class="text-primary hover:text-primary/80 transition-colors">Forgot password?</a>
            </div>

            <!-- Submit Button -->
            <button 
              type="submit"
              [disabled]="auth.isLoading()"
              class="w-full py-3 px-4 bg-gradient-to-r from-primary to-purple-500 hover:from-primary/90 hover:to-purple-500/90 text-white font-medium rounded-lg shadow-lg shadow-primary/25 hover:shadow-primary/40 transition-all duration-200 disabled:opacity-50 disabled:cursor-not-allowed flex items-center justify-center gap-2"
            >
              @if (auth.isLoading()) {
                <div class="w-5 h-5 border-2 border-white/30 border-t-white rounded-full animate-spin"></div>
                <span>Signing in...</span>
              } @else {
                <span>Sign In</span>
                <svg xmlns="http://www.w3.org/2000/svg" class="h-4 w-4" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                  <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M14 5l7 7m0 0l-7 7m7-7H3" />
                </svg>
              }
            </button>
          </form>

          <!-- Footer -->
          <div class="px-8 py-4 bg-zinc-800/30 border-t border-zinc-800 text-center">
            <p class="text-xs text-zinc-500">
              Protected by enterprise-grade security.
              <br />
              <span class="text-zinc-600">JWT Authentication • TLS Encrypted</span>
            </p>
          </div>
        </div>

        <!-- Dev Mode Badge -->
        <div class="mt-4 text-center">
          <span class="inline-flex items-center gap-1 px-2 py-1 bg-amber-500/10 border border-amber-500/30 text-amber-400 text-xs rounded-full">
            <svg xmlns="http://www.w3.org/2000/svg" class="h-3 w-3" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-3L13.732 4c-.77-1.333-2.694-1.333-3.464 0L3.34 16c-.77 1.333.192 3 1.732 3z" />
            </svg>
            Dev Mode: Any email/password works (min 4 chars)
          </span>
        </div>
      </div>
    </div>
  `,
    styles: [`
    @keyframes shake {
      0%, 100% { transform: translateX(0); }
      25% { transform: translateX(-5px); }
      75% { transform: translateX(5px); }
    }
    .animate-shake {
      animation: shake 0.3s ease-in-out;
    }
  `]
})
export class LoginComponent {
    auth = inject(AuthService);

    // Form state
    email = '';
    password = '';
    rememberMe = false;
    showPassword = signal(false);

    // Validation
    emailError = signal(false);
    passwordError = signal(false);

    async onSubmit() {
        // Reset errors
        this.emailError.set(false);
        this.passwordError.set(false);

        // Validate
        if (!this.email || !this.email.includes('@')) {
            this.emailError.set(true);
            return;
        }

        if (!this.password || this.password.length < 4) {
            this.passwordError.set(true);
            return;
        }

        const credentials: LoginCredentials = {
            email: this.email,
            password: this.password,
            rememberMe: this.rememberMe
        };

        const success = await this.auth.login(credentials);

        if (success) {
            // Navigation will be handled by AppComponent watching isAuthenticated
            console.log('Login successful!');
        }
    }
}
