#!/bin/bash
# Script to create and push v2.3.0 PR branch

echo "Creating v2.3.0 feature branch..."
git checkout feature/v2.3.0-distributed-and-ml-framework

echo ""
echo "Pushing v2.3.0 branch to origin..."
git push -u origin feature/v2.3.0-distributed-and-ml-framework

echo ""
echo "Creating pull request..."
gh pr create \
  --title "v2.3.0 - Complete Distributed Features and ML Framework Integration" \
  --body-file V2.3.0_PR_DESCRIPTION.md \
  --base main \
  --head feature/v2.3.0-distributed-and-ml-framework

echo ""
echo "✓ v2.3.0 branch pushed and PR created!"
echo "✓ Visit https://github.com/amuzetnoM/vector_studio/pulls to view the PR"
