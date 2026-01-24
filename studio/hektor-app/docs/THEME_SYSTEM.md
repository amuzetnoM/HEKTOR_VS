# HEKTOR Quantization Studio - Theme System

## Overview

The HEKTOR theme system is designed to be extensible, allowing developers and users to create custom themes without modifying core UI code. This follows a plugin-based architecture similar to VS Code extensions.

## Architecture

### Theme Structure

Each theme is a JSON file with the following structure:

```json
{
  "id": "unique-theme-id",
  "name": "Display Name",
  "type": "light" | "dark",
  "colors": {
    "bgPrimary": "#hex",
    "bgSecondary": "#hex",
    "bgTertiary": "#hex",
    "textPrimary": "#hex",
    "textSecondary": "#hex",
    "textTertiary": "#hex",
    "accentColor": "#hex",
    "accentSecondary": "#hex",
    "borderColor": "#hex",
    "successColor": "#hex",
    "warningColor": "#hex",
    "errorColor": "#hex"
  }
}
```

### Built-in Themes

HEKTOR includes 6 starter themes:

1. **Quantum Slate (Dark)** - Default slate theme with cyan accents
2. **Quantum Slate (Light)** - Light variant of Quantum Slate
3. **Midnight Purple (Dark)** - Deep purple with pink accents
4. **Ocean Breeze (Dark)** - Blue tones inspired by deep ocean
5. **Forest Green (Dark)** - Green nature-inspired theme
6. **Sunset Amber (Light)** - Warm amber and orange tones

## Creating Custom Themes

### Method 1: Built-in Themes (Dev-Only)

1. Create a JSON file in `/themes/` directory
2. Follow the theme structure above
3. Restart the application
4. Theme will appear in the theme selector

### Method 2: External Themes (Production)

External themes can be loaded from:
- User theme directory: `~/.hektor/themes/`
- System theme directory: `/usr/share/hektor/themes/` (Linux)
- Application data: `AppData/Roaming/HEKTOR/themes/` (Windows)

### Theme Installation

Users can install themes by:
1. Placing `.json` theme files in the user theme directory
2. Using the theme marketplace (future feature)
3. Importing theme files via UI (future feature)

## CSS Variables

Themes are applied using CSS custom properties:

```css
:root {
  --bg-primary: /* bgPrimary value */
  --bg-secondary: /* bgSecondary value */
  --bg-tertiary: /* bgTertiary value */
  --text-primary: /* textPrimary value */
  --text-secondary: /* textSecondary value */
  --text-tertiary: /* textTertiary value */
  --accent-color: /* accentColor value */
  --accent-secondary: /* accentSecondary value */
  --border-color: /* borderColor value */
  --success-color: /* successColor value */
  --warning-color: /* warningColor value */
  --error-color: /* errorColor value */
}
```

## Using Themes in Components

### React Components

```tsx
import { useTheme } from '@/lib/theme-system';

function MyComponent() {
  const { theme } = useTheme();
  
  return (
    <div style={{ backgroundColor: theme.colors.bgPrimary }}>
      Content
    </div>
  );
}
```

### CSS

```css
.my-component {
  background-color: var(--bg-primary);
  color: var(--text-primary);
  border: 1px solid var(--border-color);
}
```

## Theme Guidelines

### Color Contrast

- Ensure text colors have sufficient contrast with backgrounds (WCAG AA minimum)
- Test themes with accessibility tools

### Naming Conventions

- Use kebab-case for theme IDs: `my-awesome-theme`
- Use Title Case for theme names: `My Awesome Theme`

### Color Palette

Each theme should provide:
- 3 background colors (primary, secondary, tertiary)
- 3 text colors (primary, secondary, tertiary)
- 2 accent colors (primary, secondary)
- 1 border color
- 3 semantic colors (success, warning, error)

## API Reference

### ThemeProvider

React context provider for theme system.

```tsx
<ThemeProvider>
  <App />
</ThemeProvider>
```

### useTheme Hook

Hook to access theme context.

```tsx
const { theme, setTheme, availableThemes } = useTheme();
```

**Returns:**
- `theme`: Current active theme
- `setTheme`: Function to change theme
- `availableThemes`: Array of all available themes

### Window API

Global API exposed to main process.

```ts
window.themeAPI.loadTheme(themePath: string): Promise<Theme>
window.themeAPI.getAvailableThemes(): Promise<Theme[]>
window.themeAPI.setActiveTheme(themeId: string): Promise<void>
```

## Future Enhancements

- [ ] Theme marketplace
- [ ] Theme editor UI
- [ ] Theme validation tool
- [ ] Theme hot-reload during development
- [ ] Advanced theme features (gradients, shadows, animations)
- [ ] Theme inheritance (extend existing themes)
- [ ] Per-module theme overrides

## Contributing Themes

To contribute a theme to HEKTOR:

1. Create a theme following the guidelines
2. Test thoroughly on different displays
3. Submit PR with theme file to `/themes/` directory
4. Include screenshots in PR description

## License

All built-in themes are MIT licensed.
