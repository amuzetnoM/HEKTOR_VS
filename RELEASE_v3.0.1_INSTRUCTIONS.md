# Instructions for Pushing v3.0.1 Tag

The v3.0.1 tag has been created locally and is ready to be pushed to origin.

## Option 1: Push Tag Directly (Recommended)

Once this PR is merged to main, run:

```bash
git checkout main
git pull origin main
git push origin v3.0.1
```

## Option 2: Create GitHub Release from UI

1. Merge this PR to main
2. Go to: https://github.com/amuzetnoM/hektor/releases/new
3. Choose tag: `v3.0.1` (or create new tag if not available)
4. Target: `main` branch
5. Release title: `v3.0.1 - Security Patch Release`
6. Description: Copy from `docs/changelog/v3.0.1.md`
7. Click "Publish release"

## Verify Tag Locally

To verify the tag was created correctly:

```bash
git tag -l v3.0.1
git show v3.0.1
```

## Release Notes

The complete release notes are available in:
- `docs/changelog/v3.0.1.md`
- Tag annotation (accessible via `git show v3.0.1`)

## What Changed Since v3.0.0

- Security fix: Updated tar dependency from 7.5.2 to 7.5.3
- Fixed vulnerability in absolute linkpath sanitization
- No breaking changes or functional modifications
- Fully backward compatible with v3.0.0

## Next Steps After Publishing

1. Announce the release in project channels
2. Update documentation if needed
3. Monitor for any issues
4. Consider creating Docker images with updated tag
