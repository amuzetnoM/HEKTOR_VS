#!/usr/bin/env node
// Simple Electron starter that avoids module resolution issues
const { spawn } = require('child_process');
const path = require('path');

const electronPath = path.join(__dirname, 'node_modules', 'electron', 'dist', 'electron.exe');
const appPath = __dirname;

console.log('Starting HEKTOR Quantization Studio...');
console.log('Electron:', electronPath);
console.log('App:', appPath);

const child = spawn(electronPath, [appPath], {
  stdio: 'inherit',
  env: {
    ...process.env,
    // Force Electron to use its internal electron module
    ELECTRON_RUN_AS_NODE: undefined,
    // Remove node_modules from resolution to force internal electron
    NODE_PATH: ''
  }
});

child.on('exit', (code) => {
  process.exit(code);
});
