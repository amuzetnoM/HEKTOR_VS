#!/usr/bin/env python3
"""
Vector Studio - Automated Model Download Script
Downloads and caches ONNX models for text and image embeddings.

Models:
- Text: sentence-transformers/all-MiniLM-L6-v2 (384-dim → projected to 512)
- Image: laion/CLIP-ViT-B-32-laion2B-s34B-b79K (512-dim native, open-source CLIP alternative)
"""

import argparse
import hashlib
import os
import sys
from pathlib import Path
from typing import Optional

try:
    from huggingface_hub import hf_hub_download, snapshot_download
    from transformers import AutoTokenizer
    HAS_HF = True
except ImportError:
    HAS_HF = False

# Model configurations
MODELS = {
    "text": {
        "repo_id": "sentence-transformers/all-MiniLM-L6-v2",
        "onnx_file": "model.onnx",
        "vocab_file": "vocab.txt",
        "dimension": 384,
        "description": "Semantic text embeddings (MiniLM-L6-v2)"
    },
    "image": {
        "repo_id": "laion/CLIP-ViT-B-32-laion2B-s34B-b79K",
        "onnx_file": "visual_model.onnx",
        "dimension": 512,
        "description": "CLIP ViT-B/32 visual encoder (LAION open-source)"
    }
}

# Alternative ONNX model sources (if HuggingFace doesn't have ONNX)
ONNX_SOURCES = {
    "text": {
        "url": "https://huggingface.co/optimum/all-MiniLM-L6-v2-onnx/resolve/main/model.onnx",
        "vocab_url": "https://huggingface.co/sentence-transformers/all-MiniLM-L6-v2/resolve/main/vocab.txt",
    },
    "image": {
        "url": "https://huggingface.co/laion/CLIP-ViT-B-32-laion2B-s34B-b79K/resolve/main/visual.onnx",
    }
}


def get_cache_dir() -> Path:
    """Get the default cache directory for models."""
    if os.name == 'nt':  # Windows
        cache_base = Path(os.environ.get('LOCALAPPDATA', Path.home() / 'AppData' / 'Local'))
    else:
        cache_base = Path(os.environ.get('XDG_CACHE_HOME', Path.home() / '.cache'))
    
    return cache_base / 'vector_studio' / 'models'


def download_file(url: str, dest: Path, chunk_size: int = 8192) -> bool:
    """Download a file from URL with progress indication."""
    import urllib.request
    
    print(f"  Downloading: {url}")
    print(f"  Destination: {dest}")
    
    dest.parent.mkdir(parents=True, exist_ok=True)
    
    try:
        with urllib.request.urlopen(url) as response:
            total_size = int(response.headers.get('content-length', 0))
            downloaded = 0
            
            with open(dest, 'wb') as f:
                while True:
                    chunk = response.read(chunk_size)
                    if not chunk:
                        break
                    f.write(chunk)
                    downloaded += len(chunk)
                    
                    if total_size > 0:
                        progress = (downloaded / total_size) * 100
                        print(f"\r  Progress: {progress:.1f}% ({downloaded}/{total_size})", end='', flush=True)
            
            print()  # Newline after progress
            return True
            
    except Exception as e:
        print(f"\n  Error downloading: {e}")
        return False


def download_with_hf(repo_id: str, filename: str, dest_dir: Path) -> Optional[Path]:
    """Download a file using HuggingFace Hub."""
    if not HAS_HF:
        return None
    
    try:
        path = hf_hub_download(
            repo_id=repo_id,
            filename=filename,
            local_dir=dest_dir,
            local_dir_use_symlinks=False
        )
        return Path(path)
    except Exception as e:
        print(f"  HuggingFace download failed: {e}")
        return None


def download_text_model(dest_dir: Path, force: bool = False) -> bool:
    """Download the text embedding model."""
    print("\n" + "=" * 60)
    print("Downloading Text Encoder (all-MiniLM-L6-v2)")
    print("=" * 60)
    
    model_dir = dest_dir / "text"
    model_path = model_dir / "model.onnx"
    vocab_path = model_dir / "vocab.txt"
    
    if model_path.exists() and vocab_path.exists() and not force:
        print(f"  ✓ Model already exists at {model_dir}")
        return True
    
    model_dir.mkdir(parents=True, exist_ok=True)
    
    # Try HuggingFace first
    success = True
    
    if HAS_HF:
        print("  Trying HuggingFace Hub...")
        # Download ONNX model
        hf_path = download_with_hf(
            "optimum/all-MiniLM-L6-v2-onnx", 
            "model.onnx",
            model_dir
        )
        
        if hf_path:
            if hf_path != model_path:
                import shutil
                shutil.move(str(hf_path), str(model_path))
            print(f"  ✓ ONNX model downloaded")
        else:
            success = download_file(ONNX_SOURCES["text"]["url"], model_path)
        
        # Download vocab
        try:
            tokenizer = AutoTokenizer.from_pretrained(
                "sentence-transformers/all-MiniLM-L6-v2"
            )
            tokenizer.save_pretrained(str(model_dir))
            print(f"  ✓ Tokenizer downloaded")
        except Exception as e:
            print(f"  Warning: Tokenizer download failed: {e}")
            success = download_file(ONNX_SOURCES["text"]["vocab_url"], vocab_path)
    else:
        # Fallback to direct download
        print("  Using direct download (huggingface_hub not installed)")
        success = download_file(ONNX_SOURCES["text"]["url"], model_path)
        success = success and download_file(ONNX_SOURCES["text"]["vocab_url"], vocab_path)
    
    if success:
        print(f"\n  ✓ Text model ready at: {model_dir}")
    else:
        print(f"\n  ✗ Text model download failed")
    
    return success


def download_image_model(dest_dir: Path, force: bool = False) -> bool:
    """Download the image embedding model (CLIP)."""
    print("\n" + "=" * 60)
    print("Downloading Image Encoder (CLIP ViT-B/32)")
    print("=" * 60)
    
    model_dir = dest_dir / "image"
    model_path = model_dir / "visual.onnx"
    
    if model_path.exists() and not force:
        print(f"  ✓ Model already exists at {model_dir}")
        return True
    
    model_dir.mkdir(parents=True, exist_ok=True)
    
    # Try direct download (CLIP ONNX models are less common on HF)
    success = download_file(ONNX_SOURCES["image"]["url"], model_path)
    
    if success:
        print(f"\n  ✓ Image model ready at: {model_dir}")
    else:
        print(f"\n  ✗ Image model download failed")
        print("  Note: You can manually export CLIP using:")
        print("    pip install transformers onnx")
        print("    python -c \"from transformers import CLIPModel; ...\"")
    
    return success


def verify_models(model_dir: Path) -> dict:
    """Verify downloaded models and return status."""
    status = {
        "text": {
            "model": (model_dir / "text" / "model.onnx").exists(),
            "vocab": (model_dir / "text" / "vocab.txt").exists(),
        },
        "image": {
            "model": (model_dir / "image" / "visual.onnx").exists(),
        }
    }
    return status


def print_status(model_dir: Path):
    """Print model status summary."""
    status = verify_models(model_dir)
    
    print("\n" + "=" * 60)
    print("Model Status Summary")
    print("=" * 60)
    print(f"Model directory: {model_dir}\n")
    
    # Text model
    text_ok = all(status["text"].values())
    print(f"Text Encoder (all-MiniLM-L6-v2):")
    print(f"  ONNX Model: {'✓' if status['text']['model'] else '✗'}")
    print(f"  Vocabulary: {'✓' if status['text']['vocab'] else '✗'}")
    print(f"  Status: {'Ready' if text_ok else 'Incomplete'}")
    
    # Image model
    image_ok = all(status["image"].values())
    print(f"\nImage Encoder (CLIP ViT-B/32):")
    print(f"  ONNX Model: {'✓' if status['image']['model'] else '✗'}")
    print(f"  Status: {'Ready' if image_ok else 'Incomplete'}")
    
    print("\n" + "-" * 60)
    if text_ok and image_ok:
        print("All models ready! You can now use Vector Studio.")
    else:
        print("Some models are missing. Run with --download to fetch them.")


def main():
    parser = argparse.ArgumentParser(
        description="Download and manage Vector Studio embedding models",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s --download           Download all models
  %(prog)s --download --text    Download text model only
  %(prog)s --status             Check model status
  %(prog)s --dir /path/to/models --download
        """
    )
    
    parser.add_argument(
        '--dir', '-d',
        type=Path,
        default=None,
        help=f'Model directory (default: {get_cache_dir()})'
    )
    parser.add_argument(
        '--download',
        action='store_true',
        help='Download models'
    )
    parser.add_argument(
        '--text',
        action='store_true',
        help='Text model only'
    )
    parser.add_argument(
        '--image',
        action='store_true',
        help='Image model only'
    )
    parser.add_argument(
        '--force', '-f',
        action='store_true',
        help='Force re-download even if exists'
    )
    parser.add_argument(
        '--status', '-s',
        action='store_true',
        help='Show model status'
    )
    
    args = parser.parse_args()
    
    # Determine model directory
    model_dir = args.dir if args.dir else get_cache_dir()
    model_dir = model_dir.resolve()
    
    print("=" * 60)
    print("Vector Studio Model Manager")
    print("=" * 60)
    print(f"Model directory: {model_dir}")
    
    if not HAS_HF:
        print("\nNote: huggingface_hub not installed. Using direct downloads.")
        print("      Install with: pip install huggingface-hub transformers")
    
    if args.download:
        download_both = not args.text and not args.image
        
        success = True
        if args.text or download_both:
            success = download_text_model(model_dir, args.force) and success
        
        if args.image or download_both:
            success = download_image_model(model_dir, args.force) and success
        
        print_status(model_dir)
        sys.exit(0 if success else 1)
    
    elif args.status:
        print_status(model_dir)
    
    else:
        parser.print_help()
        print("\n")
        print_status(model_dir)


if __name__ == "__main__":
    main()
