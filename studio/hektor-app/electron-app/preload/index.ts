import { contextBridge, ipcRenderer } from 'electron';

// Native addon bridge
let hektorNative: any = null;

try {
  // Load native addon
  hektorNative = require('../../../build/Release/hektor_native.node');
} catch (error) {
  console.warn('Native addon not available:', error);
}

// Expose protected methods that allow the renderer process to use
// the ipcRenderer without exposing the entire object
contextBridge.exposeInMainWorld('electronAPI', {
  getAppVersion: () => ipcRenderer.invoke('get-app-version'),
  getTheme: (themeName: string) => ipcRenderer.invoke('get-theme', themeName),
  onThemeChanged: (callback: (theme: any) => void) => {
    ipcRenderer.on('theme-changed', (_, theme) => callback(theme));
  }
});

// Theme API for external theme system
contextBridge.exposeInMainWorld('themeAPI', {
  loadTheme: (themePath: string) => ipcRenderer.invoke('load-theme', themePath),
  getAvailableThemes: () => ipcRenderer.invoke('get-available-themes'),
  setActiveTheme: (themeId: string) => ipcRenderer.invoke('set-active-theme', themeId)
});

// HEKTOR Native API
contextBridge.exposeInMainWorld('hektorAPI', {
  openDatabase: (path: string) => {
    if (!hektorNative) throw new Error('Native addon not loaded');
    return hektorNative.openDatabase(path);
  },
  closeDatabase: () => {
    if (!hektorNative) throw new Error('Native addon not loaded');
    return hektorNative.closeDatabase();
  },
  addVector: (vector: number[], metadata?: any) => {
    if (!hektorNative) throw new Error('Native addon not loaded');
    return hektorNative.addVector(vector, metadata);
  },
  queryVectors: (vector: number[], topK: number = 10) => {
    if (!hektorNative) throw new Error('Native addon not loaded');
    return hektorNative.queryVectors(vector, topK);
  },
  queryVectorsAsync: (vector: number[], topK: number, callback: Function) => {
    if (!hektorNative) throw new Error('Native addon not loaded');
    return hektorNative.queryVectorsAsync(vector, topK, callback);
  },
  getVersion: () => {
    if (!hektorNative) return '4.0.0 (native addon not loaded)';
    return hektorNative.getVersion();
  },
  isNativeAvailable: () => hektorNative !== null
});
