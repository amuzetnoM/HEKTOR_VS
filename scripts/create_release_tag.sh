#!/bin/bash
#
# Create v3.0.1 Release Tag
#
# This script creates and pushes the v3.0.1 release tag for Hektor
#

set -e

echo "======================================================================"
echo "  Hektor v3.0.1 Release Tag Creation"
echo "======================================================================"
echo ""

# Check if we're in the hektor repository
if [ ! -f "CMakeLists.txt" ] || ! grep -q "project(VectorDB" CMakeLists.txt; then
    echo "❌ Error: This script must be run from the hektor repository root"
    exit 1
fi

# Check current branch
CURRENT_BRANCH=$(git branch --show-current)
echo "📌 Current branch: $CURRENT_BRANCH"

# Check if on the CLI implementation branch
if [ "$CURRENT_BRANCH" != "copilot/build-exhaustive-cli" ]; then
    echo "⚠️  Warning: Not on copilot/build-exhaustive-cli branch"
    echo "   Are you sure you want to create the tag from this branch?"
    read -p "   Continue? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Check for uncommitted changes
if ! git diff-index --quiet HEAD --; then
    echo "❌ Error: You have uncommitted changes. Please commit or stash them first."
    exit 1
fi

# Verify version in CMakeLists.txt
VERSION=$(grep "project(VectorDB VERSION" CMakeLists.txt | sed 's/.*VERSION \([0-9.]*\).*/\1/')
echo "📦 Version in CMakeLists.txt: $VERSION"

if [ "$VERSION" != "3.0.1" ]; then
    echo "❌ Error: Version in CMakeLists.txt is not 3.0.1"
    echo "   Found: $VERSION"
    exit 1
fi

echo ""
echo "Creating annotated tag v3.0.1..."
echo ""

# Create annotated tag with release notes
git tag -a v3.0.1 -m "Release v3.0.1 - Security Patch Release

Patch Release - Hektor Vector Database

## Overview

Hektor v3.0.1 is a patch release that includes important dependency updates to address security vulnerabilities. This release maintains full backward compatibility with v3.0.0.

## Changes

### Security Updates
- Dependency Update: Upgraded tar package from 7.5.2 to 7.5.3 in UI dependencies
  - Fixes vulnerability related to absolute linkpath sanitization
  - Addresses security advisory for tar package

## Technical Details
- Changed Files: 1 (ui/package-lock.json)
- Lines Changed: 8 (4 additions, 4 deletions)
- Scope: UI/Frontend dependencies only

## Migration Guide
- No breaking changes - This is a drop-in replacement for v3.0.0
- No action required - simply update to v3.0.1
- All v3.0.0 features and APIs remain unchanged

## Dependencies
- Updated: tar 7.5.2 → 7.5.3 (UI dependency)

## Installation

git clone https://github.com/amuzetnoM/hektor.git
cd hektor
git checkout v3.0.1
cmake -B build -G Ninja
cmake --build build

Full release notes: docs/changelog/v3.0.1.md
"

if [ $? -eq 0 ]; then
    echo "✅ Tag v3.0.1 created successfully"
else
    echo "❌ Failed to create tag"
    exit 1
fi

echo ""
echo "Pushing tag to origin..."
echo ""

# Push the tag to remote
git push origin v3.0.1

if [ $? -eq 0 ]; then
    echo "✅ Tag v3.0.1 pushed successfully"
else
    echo "❌ Failed to push tag"
    echo "   You can manually push with: git push origin v3.0.1"
    exit 1
fi

echo ""
echo "======================================================================"
echo "  ✅ Release v3.0.1 Tagged Successfully!"
echo "======================================================================"
echo ""
echo "Next steps:"
echo "  1. Create GitHub Release from tag v3.0.1"
echo "  2. Attach release notes from docs/releases/v3.0.1.md"
echo "  3. Announce release to community"
echo ""
echo "Tag information:"
git show v3.0.1 --no-patch
echo ""
