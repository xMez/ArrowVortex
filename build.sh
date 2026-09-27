#!/bin/zsh

# ArrowVortex Build Script for macOS
# This script checks for vcpkg, sets up the environment based on architecture

set -e

echo "ArrowVortex vcpkg Build Script - macOS"
echo "=================================="

if [[ -z "$VCPKG_ROOT" ]]; then
    echo "Warning: VCPKG_ROOT environment variable is not set."
    echo "Please set VCPKG_ROOT to point to your vcpkg installation directory."
    echo ""
    echo "For example, if vcpkg is at /Users/yourusername/vcpkg:"
    echo "  export VCPKG_ROOT=/Users/yourusername/vcpkg"
    echo ""
    echo "Add this to your ~/.zshrc or ~/.zshenv to make it permanent:"
    echo "  echo 'export VCPKG_ROOT=/path/to/your/vcpkg' >> ~/.zshrc"
    echo ""
    echo "If you don't have vcpkg installed:"
    echo "1. Clone it: git clone https://github.com/Microsoft/vcpkg/git"
    echo "2. Bootstrap it: ./vcpkg/bootstrap-vcpkg.sh"
    echo "3. Set VCPKG_ROOT: echo VCPKG_ROOT=\$(pwd)/vcpkg"
    exit 1
fi

if [ ! -f "$VCPKG_ROOT/vcpkg" ]; then
    echo "Error: vcpkg executable not found at $VCPKG_ROOT/vcpkg"
    echo "Please make sure vcpkg is properly bootstrapped."
    echo "Run: $VCPKG_ROOT/bootstrap-vcpkg.sh"
    exit 1
fi

echo "vcpkg found at: $VCPKG_ROOT"

# Detect macOS architecture and set appropriate triplet
# Fallback to Homebrew paths for pkg-config if needed.
# aubio is likely easier to install via Homebrew on macOS
ARCH=$(uname -m)
if [ "$ARCH" = "arm64" ]; then
    TRIPLET="arm64-osx"
    export VCPKG_DEFAULT_TRIPLET="arm64-osx"
    export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig:$PKG_CONFIG_PATH"
    echo "Detected Apple Silicon (ARM64) - using arm64-osx triplet"
elif [ "$ARCH" = "x86_64" ]; then
    TRIPLET="x64-osx"
    export VCPKG_DEFAULT_TRIPLET="x64-osx"
    export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH"
    echo "Detected Intel (x86_64) - using x64-osx triplet"
else
    echo "Warning: Unknown architecture ($ARCH), defaulting to x64-osx"
    TRIPLET="x64-osx"
    export VCPKG_DEFAULT_TRIPLET="x64-osx"
fi
echo "PKG_CONFIG_PATH set to include Homebrew packages"

echo "Installing dependencies with vcpkg..."
"$VCPKG_ROOT/vcpkg" install --triplet=$TRIPLET

echo "Configure with CMake..."
cmake --preset vcpkg

echo "Building ArrowVortex..."
cmake --build build --parallel

echo "Build complete!"
echo "The application bundle is at: build/bin/ArrowVortex.app"
echo "Install it with: cmake --install build --prefix \"\$HOME/Applications\""
