"""
Hektor Vector Database - High-performance vector database with SIMD-optimized similarity search.

Author: Ali A. Shakil
Organization: ARTIFACT VIRTUAL
License: MIT
"""

__version__ = "4.1.2"
__author__ = "Ali A. Shakil"
__email__ = "ali.shakil@artifactvirtual.com"

_NATIVE_AVAILABLE = False
_IMPORT_ERROR = None

# Import the native extension
try:
    # Try direct import first (for installed wheel where module is in same directory)
    from . import pyvdb as _native
    from .pyvdb import *
    _NATIVE_AVAILABLE = True
except ImportError:
    try:
        # Fallback: import the compiled extension directly by name pattern
        import importlib.util
        import os
        import glob
        
        # Find the .pyd/.so file in the package directory
        pkg_dir = os.path.dirname(__file__)
        pyd_files = glob.glob(os.path.join(pkg_dir, "pyvdb*.pyd")) + glob.glob(os.path.join(pkg_dir, "pyvdb*.so"))
        
        if pyd_files:
            spec = importlib.util.spec_from_file_location("pyvdb", pyd_files[0])
            _native = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(_native)
            
            # Import all public symbols
            for name in dir(_native):
                if not name.startswith('_'):
                    globals()[name] = getattr(_native, name)
            _NATIVE_AVAILABLE = True
        else:
            raise ImportError("No native extension found")
    except ImportError as e:
        _IMPORT_ERROR = str(e)

# Only warn if native extension is not available
if not _NATIVE_AVAILABLE:
    import warnings
    warnings.warn(
        f"Native pyvdb extension not available: {_IMPORT_ERROR}. "
        "Some features may be limited. Try reinstalling with: pip install --force-reinstall hektor-vdb"
    )

def is_native_available() -> bool:
    """Check if the native C++ extension is available."""
    return _NATIVE_AVAILABLE

# Re-export version
VERSION = __version__
