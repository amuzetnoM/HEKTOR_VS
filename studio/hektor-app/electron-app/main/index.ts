// Electron main process entry point
// The 'electron' module should resolve to Electron's internal bindings when running in Electron
const electron = require('electron');

// Handle the case where electron module returns just the binary path (shouldn't happen in Electron process)
if (typeof electron === 'string') {
  console.error('ERROR: Electron module returned a path string instead of the API.');
  console.error('This usually means the app is not running in the Electron process.');
  console.error('Make sure to run this with: npm start');
  process.exit(1);
}

const { app, BrowserWindow, ipcMain } = electron;
import * as path from 'path';

let mainWindow: typeof BrowserWindow.prototype | null = null;

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1920,
    height: 1080,
    minWidth: 1280,
    minHeight: 720,
    frame: true,
    titleBarStyle: 'hiddenInset',
    backgroundColor: '#0f172a',
    webPreferences: {
      preload: path.join(__dirname, '../preload/index.js'),
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: false
    }
  });

  // Load the app
  if (process.env.NODE_ENV === 'development') {
    mainWindow.loadURL('http://localhost:5173');
    mainWindow.webContents.openDevTools();
  } else {
    mainWindow.loadFile(path.join(__dirname, '../renderer/index.html'));
  }

  mainWindow.on('closed', () => {
    mainWindow = null;
  });
}

app.whenReady().then(() => {
  createWindow();

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow();
    }
  });
});

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

// IPC Handlers
ipcMain.handle('get-app-version', () => {
  return app.getVersion();
});

ipcMain.handle('get-theme', async (_event: any, themeName: string) => {
  // Theme loading logic will be implemented
  return { name: themeName, loaded: true };
});
