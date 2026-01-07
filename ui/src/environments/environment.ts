/**
 * Environment Configuration
 * 
 * Production: Set these via environment variables or build-time replacement
 */
export const environment = {
  production: false,
  
  // API Configuration
  apiUrl: process.env['API_URL'] || 'http://localhost:3000/api',
  wsUrl: process.env['WS_URL'] || 'ws://localhost:3000/ws',
  
  // Auth Configuration
  tokenKey: 'hektor_auth_token',
  refreshTokenKey: 'hektor_refresh_token',
  tokenExpiryBuffer: 60 * 1000, // Refresh token 1 minute before expiry
  
  // Feature Flags
  useBackend: false, // Set to true when BFF is ready
  enableMockAuth: true, // Allow mock login for development
};
