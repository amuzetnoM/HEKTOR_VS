#!/usr/bin/env python3
"""
Vector Studio - Setup and Build Automation Script
Handles full project setup including dependencies, build, and testing.
Automatically installs missing dependencies on Windows via winget.
"""

import argparse
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path
from typing import List, Optional


class Colors:
    """ANSI color codes for terminal output."""
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    BOLD = '\033[1m'
    END = '\033[0m'
    
    @staticmethod
    def disable():
        Colors.RED = Colors.GREEN = Colors.YELLOW = ''
        Colors.BLUE = Colors.BOLD = Colors.END = ''


def print_header(text: str):
    """Print a formatted header."""
    print(f"\n{Colors.BOLD}{Colors.BLUE}{'=' * 60}{Colors.END}")
    print(f"{Colors.BOLD}{Colors.BLUE}{text}{Colors.END}")
    print(f"{Colors.BOLD}{Colors.BLUE}{'=' * 60}{Colors.END}")


def print_success(text: str):
    print(f"{Colors.GREEN}✓ {text}{Colors.END}")


def print_error(text: str):
    print(f"{Colors.RED}✗ {text}{Colors.END}")


def print_warning(text: str):
    print(f"{Colors.YELLOW}⚠ {text}{Colors.END}")


def print_info(text: str):
    print(f"{Colors.BLUE}→ {text}{Colors.END}")


def install_dependency_windows(package_id: str, package_name: str) -> bool:
    """Install a dependency via winget on Windows."""
    print_info(f"Installing {package_name} via winget...")
    try:
        result = subprocess.run(
            ['winget', 'install', package_id, 
             '--accept-source-agreements', '--accept-package-agreements'],
            capture_output=True,
            text=True
        )
        if result.returncode == 0:
            # Refresh PATH
            refresh_path_windows()
            print_success(f"{package_name} installed successfully")
            return True
        else:
            print_error(f"Failed to install {package_name}")
            return False
    except FileNotFoundError:
        print_error("winget not found. Please install Windows Package Manager.")
        return False


def refresh_path_windows():
    """Refresh PATH environment variable on Windows."""
    if platform.system() != 'Windows':
        return
    try:
        import winreg
        # Get system PATH
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, 
                           r'SYSTEM\CurrentControlSet\Control\Session Manager\Environment') as key:
            system_path = winreg.QueryValueEx(key, 'Path')[0]
        # Get user PATH
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER, r'Environment') as key:
            user_path = winreg.QueryValueEx(key, 'Path')[0]
        # Update current process PATH
        os.environ['PATH'] = f"{system_path};{user_path}"
    except Exception:
        pass  # Fallback - user may need to restart terminal


def run_command(cmd: List[str], cwd: Optional[Path] = None, 
                capture: bool = False) -> subprocess.CompletedProcess:
    """Run a command with error handling."""
    print_info(f"Running: {' '.join(cmd)}")
    
    try:
        result = subprocess.run(
            cmd,
            cwd=cwd,
            capture_output=capture,
            text=True,
            check=True
        )
        return result
    except subprocess.CalledProcessError as e:
        print_error(f"Command failed with exit code {e.returncode}")
        if capture and e.stderr:
            print(e.stderr)
        raise


def check_requirements() -> dict:
    """Check system requirements and return status."""
    requirements = {
        'python': {'required': True, 'found': False, 'version': None},
        'cmake': {'required': True, 'found': False, 'version': None},
        'compiler': {'required': True, 'found': False, 'name': None},
        'git': {'required': False, 'found': False, 'version': None},
        'ninja': {'required': False, 'found': False, 'version': None},
    }
    
    # Python
    requirements['python']['found'] = True
    requirements['python']['version'] = platform.python_version()
    
    # CMake
    try:
        result = subprocess.run(['cmake', '--version'], capture_output=True, text=True)
        if result.returncode == 0:
            requirements['cmake']['found'] = True
            requirements['cmake']['version'] = result.stdout.split('\n')[0]
    except FileNotFoundError:
        pass
    
    # Compiler - Check for Visual Studio using vswhere
    if platform.system() == 'Windows':
        vswhere = r"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
        if os.path.exists(vswhere):
            try:
                result = subprocess.run([vswhere, '-latest', '-property', 'displayName'], 
                                       capture_output=True, text=True)
                if result.returncode == 0 and result.stdout.strip():
                    requirements['compiler']['found'] = True
                    requirements['compiler']['name'] = result.stdout.strip()
            except Exception:
                pass
        
        # Also check Build Tools path
        if not requirements['compiler']['found']:
            bt_path = r"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools"
            if os.path.exists(bt_path):
                requirements['compiler']['found'] = True
                requirements['compiler']['name'] = 'VS 2022 Build Tools'
    else:
        # Check for GCC/Clang on Unix
        for compiler in ['g++', 'clang++']:
            try:
                result = subprocess.run([compiler, '--version'], capture_output=True, text=True)
                if result.returncode == 0:
                    requirements['compiler']['found'] = True
                    requirements['compiler']['name'] = compiler
                    break
            except FileNotFoundError:
                continue
    
    # Git
    try:
        result = subprocess.run(['git', '--version'], capture_output=True, text=True)
        if result.returncode == 0:
            requirements['git']['found'] = True
            requirements['git']['version'] = result.stdout.strip()
    except FileNotFoundError:
        pass
    
    # Ninja
    try:
        result = subprocess.run(['ninja', '--version'], capture_output=True, text=True)
        if result.returncode == 0:
            requirements['ninja']['found'] = True
            requirements['ninja']['version'] = result.stdout.strip()
    except FileNotFoundError:
        pass
    
    return requirements


def print_requirements(requirements: dict) -> bool:
    """Print requirements status and return True if all required are met."""
    print_header("System Requirements")
    
    all_met = True
    for name, info in requirements.items():
        status = '✓' if info['found'] else '✗'
        color = Colors.GREEN if info['found'] else (Colors.RED if info['required'] else Colors.YELLOW)
        required = '(required)' if info['required'] else '(optional)'
        
        details = info.get('version') or info.get('name') or 'Not found'
        print(f"  {color}{status}{Colors.END} {name}: {details} {required}")
        
        if info['required'] and not info['found']:
            all_met = False
    
    return all_met


def setup_python_env(project_dir: Path, venv_name: str = 'venv'):
    """Set up Python virtual environment and install dependencies."""
    print_header("Python Environment Setup")
    
    venv_dir = project_dir / venv_name
    
    # Create virtual environment
    if not venv_dir.exists():
        print_info(f"Creating virtual environment: {venv_dir}")
        run_command([sys.executable, '-m', 'venv', str(venv_dir)])
        print_success("Virtual environment created")
    else:
        print_info(f"Virtual environment already exists: {venv_dir}")
    
    # Determine pip path
    if platform.system() == 'Windows':
        pip_path = venv_dir / 'Scripts' / 'pip.exe'
        python_path = venv_dir / 'Scripts' / 'python.exe'
    else:
        pip_path = venv_dir / 'bin' / 'pip'
        python_path = venv_dir / 'bin' / 'python'
    
    # Upgrade pip
    print_info("Upgrading pip...")
    run_command([str(python_path), '-m', 'pip', 'install', '--upgrade', 'pip'])
    
    # Install requirements
    requirements_file = project_dir / 'requirements.txt'
    if requirements_file.exists():
        print_info("Installing requirements...")
        run_command([str(pip_path), 'install', '-r', str(requirements_file)])
        print_success("Requirements installed")
    
    return venv_dir


def configure_cmake(project_dir: Path, build_dir: Path, 
                   build_type: str = 'Release',
                   use_ninja: bool = True) -> bool:
    """Configure the CMake project."""
    print_header("CMake Configuration")
    
    build_dir.mkdir(parents=True, exist_ok=True)
    
    cmake_args = [
        'cmake',
        '-S', str(project_dir),
        '-B', str(build_dir),
        f'-DCMAKE_BUILD_TYPE={build_type}',
        '-DVDB_BUILD_TESTS=ON',
        '-DVDB_BUILD_CLI=ON',
        '-DVDB_BUILD_PYTHON=ON',
    ]
    
    # Use Ninja if available
    if use_ninja and shutil.which('ninja'):
        cmake_args.extend(['-G', 'Ninja'])
    elif platform.system() == 'Windows':
        cmake_args.extend(['-G', 'Visual Studio 17 2022', '-A', 'x64'])
    
    try:
        run_command(cmake_args)
        print_success("CMake configuration complete")
        return True
    except subprocess.CalledProcessError:
        print_error("CMake configuration failed")
        return False


def build_project(build_dir: Path, parallel: int = None) -> bool:
    """Build the project using CMake."""
    print_header("Building Project")
    
    cmake_args = ['cmake', '--build', str(build_dir), '--config', 'Release']
    
    if parallel:
        cmake_args.extend(['--parallel', str(parallel)])
    else:
        cmake_args.extend(['--parallel'])
    
    try:
        run_command(cmake_args)
        print_success("Build complete")
        return True
    except subprocess.CalledProcessError:
        print_error("Build failed")
        return False


def run_tests(build_dir: Path) -> bool:
    """Run the test suite."""
    print_header("Running Tests")
    
    try:
        run_command(['ctest', '--output-on-failure', '-C', 'Release'], cwd=build_dir)
        print_success("All tests passed")
        return True
    except subprocess.CalledProcessError:
        print_error("Some tests failed")
        return False


def download_models(project_dir: Path, venv_dir: Path) -> bool:
    """Download the embedding models."""
    print_header("Downloading Models")
    
    if platform.system() == 'Windows':
        python_path = venv_dir / 'Scripts' / 'python.exe'
    else:
        python_path = venv_dir / 'bin' / 'python'
    
    script_path = project_dir / 'scripts' / 'download_models.py'
    
    if not script_path.exists():
        print_error(f"Model download script not found: {script_path}")
        return False
    
    try:
        run_command([str(python_path), str(script_path), '--download'])
        print_success("Models downloaded")
        return True
    except subprocess.CalledProcessError:
        print_warning("Model download had issues (check output above)")
        return False


def main():
    parser = argparse.ArgumentParser(
        description="Vector Studio Setup and Build Script",
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    
    parser.add_argument(
        '--check',
        action='store_true',
        help='Check system requirements only'
    )
    parser.add_argument(
        '--skip-python',
        action='store_true',
        help='Skip Python environment setup'
    )
    parser.add_argument(
        '--skip-build',
        action='store_true',
        help='Skip C++ build'
    )
    parser.add_argument(
        '--skip-tests',
        action='store_true',
        help='Skip running tests'
    )
    parser.add_argument(
        '--skip-models',
        action='store_true',
        help='Skip model download'
    )
    parser.add_argument(
        '--build-type',
        choices=['Debug', 'Release', 'RelWithDebInfo'],
        default='Release',
        help='CMake build type (default: Release)'
    )
    parser.add_argument(
        '--jobs', '-j',
        type=int,
        default=None,
        help='Number of parallel build jobs'
    )
    parser.add_argument(
        '--no-color',
        action='store_true',
        help='Disable colored output'
    )
    parser.add_argument(
        '--auto-install',
        action='store_true',
        help='Automatically install missing dependencies (Windows only, uses winget)'
    )
    
    args = parser.parse_args()
    
    if args.no_color or not sys.stdout.isatty():
        Colors.disable()
    
    # Determine project directory (parent of scripts/)
    project_dir = Path(__file__).parent.parent.resolve()
    build_dir = project_dir / 'build'
    
    print_header("Vector Studio Setup")
    print(f"Project directory: {project_dir}")
    print(f"Build directory: {build_dir}")
    
    # Check requirements
    requirements = check_requirements()
    all_met = print_requirements(requirements)
    
    if args.check:
        sys.exit(0 if all_met else 1)
    
    # Auto-install missing dependencies on Windows
    if not all_met and args.auto_install and platform.system() == 'Windows':
        print_header("Installing Missing Dependencies")
        
        if not requirements['cmake']['found']:
            if install_dependency_windows('Kitware.CMake', 'CMake'):
                requirements['cmake']['found'] = True
        
        if not requirements['compiler']['found']:
            if install_dependency_windows('Microsoft.VisualStudio.2022.BuildTools', 
                                         'Visual Studio Build Tools 2022'):
                requirements['compiler']['found'] = True
                print_warning("You may need to add C++ workload via Visual Studio Installer")
        
        if not requirements['ninja']['found']:
            install_dependency_windows('Ninja-build.Ninja', 'Ninja')
        
        # Re-check requirements
        requirements = check_requirements()
        all_met = print_requirements(requirements)
    
    if not all_met:
        print_error("\nRequired dependencies missing.")
        if platform.system() == 'Windows':
            print_info("Run with --auto-install to automatically install them.")
        else:
            print_info("Please install CMake and a C++ compiler manually.")
        sys.exit(1)
    
    # Setup Python environment
    venv_dir = None
    if not args.skip_python:
        venv_dir = setup_python_env(project_dir)
    
    # Configure and build
    if not args.skip_build:
        if not configure_cmake(project_dir, build_dir, args.build_type):
            sys.exit(1)
        
        if not build_project(build_dir, args.jobs):
            sys.exit(1)
    
    # Run tests
    if not args.skip_tests and not args.skip_build:
        run_tests(build_dir)
    
    # Download models
    if not args.skip_models and venv_dir:
        download_models(project_dir, venv_dir)
    
    print_header("Setup Complete!")
    print(f"""
Next steps:
  1. Activate the virtual environment:
     {'venv\\Scripts\\activate' if platform.system() == 'Windows' else 'source venv/bin/activate'}
  
  2. Run the CLI:
     {build_dir / 'Release' / 'vdb_cli' if platform.system() == 'Windows' else build_dir / 'vdb_cli'} --help
  
  3. Use in Python:
     python -c "import pyvdb; db = pyvdb.VectorDatabase(...)"
""")


if __name__ == "__main__":
    main()
