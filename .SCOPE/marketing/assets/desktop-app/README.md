# Desktop App Assets
> **Graphics and assets for HEKTOR desktop application**

## Asset Inventory

### App Icons
**Required Sizes** (macOS, Windows, Linux):
- 16x16, 32x32, 64x64, 128x128, 256x256, 512x512, 1024x1024

**Files**:
- `icon.icns` (macOS)
- `icon.ico` (Windows)
- `icon.png` (Linux, various sizes)

**Design**: Use `../svgs/hektor-icon.svg` as base, export at required sizes

### Launch Screen
**Dimensions**: 800x600 (minimum), scales up
**Content**: 
- HEKTOR logo (centered)
- "Spectral Vector Database" tagline
- Loading indicator
- Version number

**File**: `launch-screen.png` (2x for retina)

### Tray/Menu Bar Icons
**Sizes**: 16x16, 32x32 (for different display densities)
**Variants**:
- Default state
- Active/connected state
- Warning/error state
- Notification state

**Design**: Simplified "H" monochrome icon

### Empty States
**Query Results Empty**:
- Illustration: Magnifying glass + empty box
- Text: "No results found. Try adjusting your query."

**No Database Connected**:
- Illustration: Disconnected plug icon
- Text: "Connect to a HEKTOR instance to get started."

**No Vectors Indexed**:
- Illustration: Empty database icon
- Text: "Your database is empty. Start indexing vectors."

### Onboarding Graphics
**Welcome Screen**:
- Hero: HEKTOR logo + gradient background
- Feature highlights (3 panels):
  1. Performance (speed icon + "Sub-3ms queries")
  2. Accuracy (target icon + "98.1% recall")
  3. Scale (growth icon + "Billion-vector scale")

**Setup Wizard**:
- Step 1: Connect (server icon)
- Step 2: Configure (settings icon)
- Step 3: Index (upload icon)
- Step 4: Query (search icon)

### In-App Graphics
**Dashboard**:
- Performance chart backgrounds
- Metric cards (latency, throughput, recall)
- Health status indicators

**Query Builder**:
- Vector visualization icons
- Filter/sort icons
- Result type icons

**Settings**:
- Section icons (general, performance, security, advanced)
- Toggle/checkbox states
- Success/error indicators

### Illustrations
**404/Error Pages**:
- Broken link illustration
- Server error illustration
- Network error illustration

**Success States**:
- Query completed (checkmark + sparkles)
- Indexing completed (progress complete)
- Export completed (download icon)

### Brand Elements
**Color Palette** (from main brand):
- Primary: #6366f1 (indigo)
- Secondary: #8b5cf6 (purple)
- Success: #10b981 (green)
- Warning: #f59e0b (amber)
- Error: #ef4444 (red)
- Neutral: #64748b (slate)

**Typography**:
- Headlines: Inter Bold
- Body: Inter Regular
- Code/Monospace: JetBrains Mono

**Spacing**:
- Base unit: 4px
- Common: 8px, 16px, 24px, 32px

### Screenshot Guidelines
**For Marketing**:
- 1920x1080 (standard desktop)
- Clean data (no personal info)
- Populated but not cluttered
- Highlight key features
- Professional appearance

**Key Views to Screenshot**:
1. Dashboard (performance metrics)
2. Query builder (mid-construction)
3. Results view (with data)
4. Settings panel (open)
5. About/info screen

### Animation Assets
**Loading Animations**:
- Spinner (HEKTOR logo rotating?)
- Progress bar (gradient fill)
- Skeleton screens (content placeholders)

**Transitions**:
- Fade in/out
- Slide left/right
- Scale up/down

**Micro-interactions**:
- Button press (scale + shadow)
- Checkbox toggle (slide + color)
- Input focus (border glow)

---

## Design Specifications

### App Window
**Minimum Size**: 1024x768  
**Default Size**: 1280x800  
**Maximum Size**: Fullscreen

**Title Bar**:
- Left: App menu, navigation
- Center: Current view title
- Right: User profile, settings, help

### Layout Grid
**Columns**: 12-column grid  
**Gutters**: 24px  
**Margins**: 32px (desktop), 16px (compact)

### Components
All components follow **Material Design** or **Fluent Design** principles depending on platform.

**Buttons**:
- Primary: Filled, indigo
- Secondary: Outlined, indigo
- Tertiary: Text only

**Cards**:
- Elevation: 1 (default), 2 (hover), 3 (active)
- Border-radius: 8px
- Padding: 16px or 24px

**Inputs**:
- Border: 1px, slate-300
- Focus: 2px, indigo-500
- Error: 2px, red-500
- Height: 40px (default)

---

## Asset Workflow

### Creating New Assets
1. Design in Figma (design file link TBD)
2. Export at 1x and 2x (retina)
3. Optimize (ImageOptim, SVGO)
4. Add to `/assets/desktop-app/`
5. Update this README

### Naming Convention
```
[component]-[variant]-[size].ext
```

Examples:
- `icon-active-32.png`
- `launch-screen-2x.png`
- `empty-state-no-results.svg`
- `onboarding-step1.png`

### File Organization
```
assets/desktop-app/
├── README.md (this file)
├── icons/
│   ├── app-icon/ (various sizes)
│   ├── tray-icon/ (menu bar)
│   └── in-app/ (UI icons)
├── illustrations/
│   ├── empty-states/
│   ├── errors/
│   └── onboarding/
├── screenshots/ (for marketing)
└── animations/ (Lottie JSON or GIF)
```

---

## Brand Consistency

**Do**:
✅ Use official color palette  
✅ Maintain HEKTOR logo integrity  
✅ Follow spacing guidelines  
✅ Use approved typography  
✅ Keep UI clean and minimal

**Don't**:
❌ Distort logo  
❌ Use off-brand colors  
❌ Clutter interface  
❌ Mix typography families  
❌ Ignore accessibility (contrast, sizing)

---

## Accessibility

**Color Contrast**: WCAG AAA (7:1 for text)  
**Icon Sizes**: Minimum 16x16 for clarity  
**Touch Targets**: Minimum 44x44 for buttons  
**Alt Text**: All images need descriptive alt text  
**Animations**: Respect `prefers-reduced-motion`

---

## Platform-Specific Notes

### macOS
- Use SF Pro for system consistency (fallback: Inter)
- Follow macOS HIG (Human Interface Guidelines)
- Native title bar style
- Use `.icns` format for app icon

### Windows
- Follow Fluent Design principles
- Use `.ico` format for app icon
- Acrylic material effects (optional)
- Native title bar with custom accent

### Linux
- Follow GNOME or KDE guidelines
- Use `.png` icons (various sizes)
- Respect system theme colors
- GTK or Qt native widgets

---

## Tools & Resources

**Design**: Figma (primary), Sketch (legacy)  
**Export**: Figma, Illustrator  
**Optimization**: ImageOptim, SVGO, TinyPNG  
**Icons**: Heroicons, Lucide, custom  
**Illustrations**: Undraw (customized), custom

**Figma Design File**: [Link TBD - create and share]

---

## Future Additions

As desktop app evolves, add:
- [ ] Dark mode variants (all assets)
- [ ] Localized screenshots (i18n)
- [ ] Platform-specific variations
- [ ] Seasonal/holiday themes (optional)
- [ ] Marketing video stills
- [ ] Tutorial GIFs/videos

---

## Questions?

Contact design team or see main marketing README.md for team contacts.

---

**Last Updated**: 2026-01-23  
**Version**: 1.0  
**Maintainer**: Design Team
