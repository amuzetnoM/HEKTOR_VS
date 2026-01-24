import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';
import path from 'path';

export default defineConfig({
  plugins: [react()],
  base: './',
  root: 'electron-app/renderer',
  build: {
    outDir: '../../dist/renderer',
    emptyOutDir: true,
    rollupOptions: {
      input: {
        main: path.resolve(__dirname, 'electron-app/renderer/index.html')
      }
    }
  },
  resolve: {
    alias: {
      '@': path.resolve(__dirname, './electron-app/renderer/src'),
      '@components': path.resolve(__dirname, './electron-app/renderer/src/components'),
      '@lib': path.resolve(__dirname, './electron-app/renderer/src/lib'),
      '@themes': path.resolve(__dirname, './themes')
    }
  },
  server: {
    port: 5173
  }
});
