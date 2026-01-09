# Deployment Guide for HEKTOR Vector Studio on Vercel

## Prerequisites
- Vercel account
- Git repository access

## Quick Deploy

### Option 1: Vercel CLI
```bash
cd ui
npm install -g vercel
vercel
```

### Option 2: Vercel Dashboard
1. Go to [vercel.com](https://vercel.com)
2. Click "New Project"
3. Import the repository
4. Set the following:
   - **Framework Preset**: Other
   - **Root Directory**: `ui`
   - **Build Command**: `npm run build`
   - **Output Directory**: `dist`
   - **Install Command**: `npm install`

## Build Configuration

The `vercel.json` in the ui directory contains:
- Build command configuration
- Output directory specification  
- SPA routing configuration (all routes redirect to index.html)

## Environment Variables (Optional)

If you need to set API keys or backend URLs:
- `API_KEY`: Google Gemini API key for embeddings
- `API_URL`: Backend API endpoint

## Troubleshooting

### Build Fails
- Ensure Node.js version is 20+
- Check that all dependencies install correctly
- Verify TypeScript version is 5.9.x

### Fonts Not Loading
Google Fonts are commented out in index.html for build compatibility. You can:
- Uncomment them in the deployed version if needed
- Use system fonts as fallback (already configured in Tailwind)

## Post-Deployment

After successful deployment:
1. The app will be available at your Vercel URL
2. Login works in dev mode (any email with @ and password 4+ chars)
3. For production, configure proper authentication backend

## Support

See main [README.md](README.md) for full documentation.
