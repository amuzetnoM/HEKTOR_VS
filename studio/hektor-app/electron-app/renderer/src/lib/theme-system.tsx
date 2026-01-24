/**
 * HEKTOR Theme System
 * 
 * Provides theme management with hot-swapping support
 * 6 built-in themes (3 dark, 3 light)
 */

import React, { createContext, useContext, useState, useEffect, ReactNode } from 'react';

// ============================================================================
// Theme Types
// ============================================================================

export interface ThemeColors {
  // Background colors
  bgPrimary: string;
  bgSecondary: string;
  bgTertiary: string;
  
  // Text colors
  textPrimary: string;
  textSecondary: string;
  textMuted: string;
  
  // Accent colors
  accentPrimary: string;
  accentSecondary: string;
  
  // UI colors
  border: string;
  success: string;
  warning: string;
  error: string;
  info: string;
}

export interface Theme {
  name: string;
  displayName: string;
  isDark: boolean;
  colors: ThemeColors;
}

export interface ThemeContextValue {
  currentTheme: Theme;
  setTheme: (themeName: string) => void;
  availableThemes: Theme[];
}

// ============================================================================
// Built-in Themes
// ============================================================================

const themes: Theme[] = [
  // Dark Themes
  {
    name: 'midnight-purple-dark',
    displayName: 'Midnight Purple',
    isDark: true,
    colors: {
      bgPrimary: '#0f0f1a',
      bgSecondary: '#1a1a2e',
      bgTertiary: '#16213e',
      textPrimary: '#ffffff',
      textSecondary: '#a0a0b0',
      textMuted: '#6c6c7c',
      accentPrimary: '#9d4edd',
      accentSecondary: '#7b2cbf',
      border: '#2d2d44',
      success: '#22c55e',
      warning: '#f59e0b',
      error: '#ef4444',
      info: '#3b82f6',
    },
  },
  {
    name: 'quantum-slate-dark',
    displayName: 'Quantum Slate',
    isDark: true,
    colors: {
      bgPrimary: '#0a0a0f',
      bgSecondary: '#141419',
      bgTertiary: '#1e1e24',
      textPrimary: '#e4e4e7',
      textSecondary: '#a1a1aa',
      textMuted: '#71717a',
      accentPrimary: '#06b6d4',
      accentSecondary: '#0891b2',
      border: '#27272a',
      success: '#10b981',
      warning: '#f59e0b',
      error: '#f43f5e',
      info: '#6366f1',
    },
  },
  {
    name: 'ocean-breeze-dark',
    displayName: 'Ocean Breeze',
    isDark: true,
    colors: {
      bgPrimary: '#0c1929',
      bgSecondary: '#132238',
      bgTertiary: '#1a2d47',
      textPrimary: '#e0f2fe',
      textSecondary: '#93c5fd',
      textMuted: '#60a5fa',
      accentPrimary: '#38bdf8',
      accentSecondary: '#0ea5e9',
      border: '#1e3a5f',
      success: '#34d399',
      warning: '#fbbf24',
      error: '#fb7185',
      info: '#818cf8',
    },
  },
  {
    name: 'forest-green-dark',
    displayName: 'Forest Green',
    isDark: true,
    colors: {
      bgPrimary: '#0d1712',
      bgSecondary: '#142018',
      bgTertiary: '#1a291f',
      textPrimary: '#ecfdf5',
      textSecondary: '#a7f3d0',
      textMuted: '#6ee7b7',
      accentPrimary: '#10b981',
      accentSecondary: '#059669',
      border: '#1f3a2c',
      success: '#22c55e',
      warning: '#fbbf24',
      error: '#f87171',
      info: '#60a5fa',
    },
  },
  // Light Themes
  {
    name: 'quantum-slate-light',
    displayName: 'Quantum Light',
    isDark: false,
    colors: {
      bgPrimary: '#fafafa',
      bgSecondary: '#f4f4f5',
      bgTertiary: '#e4e4e7',
      textPrimary: '#18181b',
      textSecondary: '#3f3f46',
      textMuted: '#71717a',
      accentPrimary: '#0891b2',
      accentSecondary: '#06b6d4',
      border: '#d4d4d8',
      success: '#059669',
      warning: '#d97706',
      error: '#dc2626',
      info: '#4f46e5',
    },
  },
  {
    name: 'sunset-amber-light',
    displayName: 'Sunset Amber',
    isDark: false,
    colors: {
      bgPrimary: '#fffbeb',
      bgSecondary: '#fef3c7',
      bgTertiary: '#fde68a',
      textPrimary: '#78350f',
      textSecondary: '#92400e',
      textMuted: '#b45309',
      accentPrimary: '#f59e0b',
      accentSecondary: '#d97706',
      border: '#fcd34d',
      success: '#16a34a',
      warning: '#ea580c',
      error: '#dc2626',
      info: '#2563eb',
    },
  },
];

// ============================================================================
// Theme Context
// ============================================================================

const ThemeContext = createContext<ThemeContextValue | undefined>(undefined);

export const useTheme = (): ThemeContextValue => {
  const context = useContext(ThemeContext);
  if (!context) {
    throw new Error('useTheme must be used within a ThemeProvider');
  }
  return context;
};

// ============================================================================
// Theme Provider Component
// ============================================================================

interface ThemeProviderProps {
  children: ReactNode;
  defaultTheme?: string;
}

export const ThemeProvider: React.FC<ThemeProviderProps> = ({ 
  children, 
  defaultTheme = 'midnight-purple-dark' 
}) => {
  const [currentThemeName, setCurrentThemeName] = useState(defaultTheme);
  
  const currentTheme = themes.find(t => t.name === currentThemeName) || themes[0];
  
  // Apply theme CSS variables
  useEffect(() => {
    const root = document.documentElement;
    const { colors } = currentTheme;
    
    root.style.setProperty('--bg-primary', colors.bgPrimary);
    root.style.setProperty('--bg-secondary', colors.bgSecondary);
    root.style.setProperty('--bg-tertiary', colors.bgTertiary);
    root.style.setProperty('--text-primary', colors.textPrimary);
    root.style.setProperty('--text-secondary', colors.textSecondary);
    root.style.setProperty('--text-muted', colors.textMuted);
    root.style.setProperty('--accent-primary', colors.accentPrimary);
    root.style.setProperty('--accent-secondary', colors.accentSecondary);
    root.style.setProperty('--border', colors.border);
    root.style.setProperty('--success', colors.success);
    root.style.setProperty('--warning', colors.warning);
    root.style.setProperty('--error', colors.error);
    root.style.setProperty('--info', colors.info);
    
    // Legacy compatibility
    root.style.setProperty('--primary', colors.accentPrimary);
    root.style.setProperty('--accent', colors.accentSecondary);
    
    // Set body background
    document.body.style.backgroundColor = colors.bgPrimary;
    document.body.style.color = colors.textPrimary;
  }, [currentTheme]);
  
  const setTheme = (themeName: string) => {
    const theme = themes.find(t => t.name === themeName);
    if (theme) {
      setCurrentThemeName(themeName);
      localStorage.setItem('hektor-theme', themeName);
    }
  };
  
  // Load saved theme
  useEffect(() => {
    const savedTheme = localStorage.getItem('hektor-theme');
    if (savedTheme && themes.find(t => t.name === savedTheme)) {
      setCurrentThemeName(savedTheme);
    }
  }, []);
  
  const value: ThemeContextValue = {
    currentTheme,
    setTheme,
    availableThemes: themes,
  };
  
  return (
    <ThemeContext.Provider value={value}>
      {children}
    </ThemeContext.Provider>
  );
};

export default ThemeProvider;
