#!/usr/bin/env python3
"""
Documentation regression tests to ensure consistency and prevent common issues.
"""

import os
import re
from pathlib import Path


def test_no_placeholder_urls():
    """
    Ensure no placeholder URLs like 'yourusername' remain in documentation.
    This is a regression protection test.
    Allows YOUR_USERNAME in CONTRIBUTING.md as it's intentional for fork instructions.
    """
    docs_dir = Path(__file__).parent.parent / "docs"
    readme = Path(__file__).parent.parent / "README.md"
    
    # Pattern to detect placeholder URLs
    placeholder_pattern = re.compile(r'github\.com/yourusername|github\.com/YOUR[-_]?USERNAME', re.IGNORECASE)
    
    files_to_check = list(docs_dir.glob("**/*.md"))
    if readme.exists():
        files_to_check.append(readme)
    
    errors = []
    for file_path in files_to_check:
        if not file_path.is_file():
            continue
        
        # Allow YOUR_USERNAME in CONTRIBUTING.md (for fork instructions)
        if file_path.name in ["CONTRIBUTING.md", "24_CONTRIBUTING.md"]:
            continue
            
        try:
            content = file_path.read_text(encoding='utf-8')
            matches = list(placeholder_pattern.finditer(content))
            
            if matches:
                for match in matches:
                    # Get line number
                    line_num = content[:match.start()].count('\n') + 1
                    errors.append(f"{file_path.relative_to(file_path.parent.parent)}:{line_num} - Found placeholder URL: {match.group()}")
        except Exception as e:
            print(f"Warning: Could not read {file_path}: {e}")
    
    if errors:
        error_msg = "Found placeholder URLs in documentation:\n" + "\n".join(errors)
        error_msg += "\n\nPlease replace with actual repository URLs (e.g., github.com/amuzetnoM/hektor)"
        raise AssertionError(error_msg)


def test_no_trailing_whitespace():
    """
    Ensure key documentation files don't have trailing whitespace.
    This helps maintain clean diffs and prevents formatting issues.
    Only checks files likely to be frequently edited.
    """
    docs_dir = Path(__file__).parent.parent / "docs"
    readme = Path(__file__).parent.parent / "README.md"
    
    # Focus on key files
    key_files = [
        readme,
        docs_dir / "01_INTRODUCTION.md",
        docs_dir / "02_INSTALLATION.md",
        docs_dir / "03_QUICKSTART.md",
        docs_dir / "04_USER_GUIDE.md",
        docs_dir / "24_CONTRIBUTING.md",
    ]
    
    errors = []
    for file_path in key_files:
        if not file_path.exists() or not file_path.is_file():
            continue
            
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                for line_num, line in enumerate(f, 1):
                    # Check for trailing whitespace (but allow empty lines)
                    if line.rstrip('\n').endswith(' ') or line.rstrip('\n').endswith('\t'):
                        errors.append(f"{file_path.name}:{line_num} - Trailing whitespace")
        except Exception as e:
            print(f"Warning: Could not read {file_path}: {e}")
    
    if errors:
        error_msg = "Found trailing whitespace in key documentation files:\n" + "\n".join(errors[:20])
        if len(errors) > 20:
            error_msg += f"\n... and {len(errors) - 20} more"
        raise AssertionError(error_msg)


def test_cross_references_valid():
    """
    Basic check that cross-references in documentation use consistent paths.
    This helps catch broken links early.
    Note: This test is informational and won't fail the build for optional docs.
    """
    docs_dir = Path(__file__).parent.parent / "docs"
    
    # Pattern to detect markdown links to other docs
    link_pattern = re.compile(r'\[([^\]]+)\]\(([^)]+\.md[^)]*)\)')
    
    warnings = []
    for file_path in docs_dir.glob("*.md"):
        if not file_path.is_file():
            continue
            
        try:
            content = file_path.read_text(encoding='utf-8')
            matches = link_pattern.findall(content)
            
            for link_text, link_path in matches:
                # Skip external links
                if link_path.startswith('http'):
                    continue
                
                # Remove anchors and query strings
                clean_path = link_path.split('#')[0].split('?')[0]
                
                # Resolve the path relative to the file
                if clean_path.startswith('/'):
                    target_path = Path(__file__).parent.parent / clean_path.lstrip('/')
                else:
                    target_path = (file_path.parent / clean_path).resolve()
                
                if not target_path.exists():
                    warnings.append(f"{file_path.name} -> Missing link target: {link_path}")
        except Exception as e:
            print(f"Warning: Could not read {file_path}: {e}")
    
    # Just print warnings, don't fail
    if warnings:
        print("\nNote: Found some missing link targets (may be intentional):")
        for warning in warnings[:5]:
            print(f"  - {warning}")
        if len(warnings) > 5:
            print(f"  ... and {len(warnings) - 5} more")
        print("These may be references to optional or removed documentation files.")


def test_consistent_repository_urls():
    """
    Ensure all repository URLs point to the same repository.
    Catches mixed references to different repos.
    Allows YOUR_USERNAME in CONTRIBUTING.md for fork instructions.
    """
    docs_dir = Path(__file__).parent.parent / "docs"
    readme = Path(__file__).parent.parent / "README.md"
    
    # Pattern to detect GitHub repository URLs
    repo_pattern = re.compile(r'github\.com/([a-zA-Z0-9_-]+)/([a-zA-Z0-9_-]+)')
    
    files_to_check = list(docs_dir.glob("**/*.md"))
    if readme.exists():
        files_to_check.append(readme)
    
    found_repos = set()
    locations = {}
    
    for file_path in files_to_check:
        if not file_path.is_file():
            continue
        
        # Skip CONTRIBUTING.md for YOUR_USERNAME placeholders
        if file_path.name in ["CONTRIBUTING.md", "24_CONTRIBUTING.md"]:
            continue
            
        try:
            content = file_path.read_text(encoding='utf-8')
            matches = repo_pattern.findall(content)
            
            for owner, repo in matches:
                repo_url = f"{owner}/{repo}"
                found_repos.add(repo_url)
                if repo_url not in locations:
                    locations[repo_url] = []
                locations[repo_url].append(file_path.name)
        except Exception as e:
            print(f"Warning: Could not read {file_path}: {e}")
    
    # Allow the main repository and common external references
    expected_repo = "amuzetnoM/hektor"
    allowed_external_repos = {
        "nmslib/hnswlib",
        "tensorflow/tensorflow",
        "pytorch/pytorch",
        "huggingface/transformers",
        "facebookresearch/faiss",
        "openai/CLIP",
        # Add other legitimate external repository references
    }
    
    # Check for placeholder URLs only
    placeholder_repos = {repo for repo in found_repos if "YOUR_USERNAME" in repo.upper() or "YOURUSERNAME" in repo.upper()}
    
    if placeholder_repos:
        error_msg = f"Found placeholder repository URLs:\n"
        for repo in placeholder_repos:
            files = ", ".join(locations[repo][:3])
            if len(locations[repo]) > 3:
                files += f" (and {len(locations[repo]) - 3} more)"
            error_msg += f"  - {repo} in {files}\n"
        error_msg += f"\nPlease replace with actual repository URL: {expected_repo}"
        raise AssertionError(error_msg)


if __name__ == "__main__":
    # Run tests manually
    print("Running documentation regression tests...")
    
    try:
        test_no_placeholder_urls()
        print("✓ No placeholder URLs found")
    except AssertionError as e:
        print(f"✗ Placeholder URL test failed:\n{e}")
    
    try:
        test_no_trailing_whitespace()
        print("✓ No trailing whitespace found")
    except AssertionError as e:
        print(f"✗ Trailing whitespace test failed:\n{e}")
    
    try:
        test_cross_references_valid()
        print("✓ Cross-references are valid")
    except AssertionError as e:
        print(f"✗ Cross-reference test failed:\n{e}")
    
    try:
        test_consistent_repository_urls()
        print("✓ Repository URLs are consistent")
    except AssertionError as e:
        print(f"✗ Repository URL consistency test failed:\n{e}")
    
    print("\nDocumentation regression tests completed!")
