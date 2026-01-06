#!/bin/bash
# Script to push v2.2.0 tag and create GitHub release

echo "Pushing v2.2.0 tag to origin..."
git push origin v2.2.0

echo ""
echo "Creating GitHub release draft..."
gh release create v2.2.0 \
  --draft \
  --title "Vector Studio v2.2.0 - Hybrid Search, RAG Engine, Framework Integration" \
  --notes-file RELEASE_NOTES_v2.2.0.md

echo ""
echo "✓ Tag pushed and release draft created!"
echo "✓ Visit https://github.com/amuzetnoM/vector_studio/releases to publish the release"
