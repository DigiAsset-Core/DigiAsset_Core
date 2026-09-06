#!/bin/bash
set -e

# DigiAsset Core - Linux Build & Test Script
# Prerequisites: DigiByte Core must be running and synced on this machine

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

echo "=========================================="
echo " DigiAsset Core - Linux Build & Test"
echo "=========================================="

# ---- Install dependencies ----
echo ""
echo "[1/5] Installing build dependencies..."
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    libcurl4-openssl-dev \
    libsqlite3-dev \
    libssl-dev \
    libjsoncpp-dev \
    libjsonrpccpp-dev \
    libjsonrpccpp-tools \
    pkg-config

# ---- Initialize googletest submodule ----
echo ""
echo "[2/5] Setting up Google Test..."
if [ ! -d "tests/lib/googletest/CMakeLists.txt" ]; then
    git submodule update --init --recursive
fi

# ---- Configure with CMake ----
echo ""
echo "[3/5] Configuring with CMake..."
rm -rf build
mkdir -p build
cd build
cmake .. -DBUILD_TEST=ON -DBUILD_CLI=ON -DBUILD_WEB=ON

# ---- Build ----
echo ""
echo "[4/5] Building (this may take a few minutes)..."
NPROC=$(nproc 2>/dev/null || echo 4)
make -j"$NPROC"

echo ""
echo "Build complete!"
echo "  Binary: $(pwd)/src/digiasset_core"
echo "  Tests:  $(pwd)/tests/Google_Tests_run"

# ---- Run tests ----
echo ""
echo "[5/5] Running tests..."
echo ""

# Check if DigiByte Core is running
if digibyte-cli getblockchaininfo >/dev/null 2>&1; then
    echo "DigiByte Core is running - good."
else
    echo "WARNING: DigiByte Core does not appear to be running."
    echo "Some tests that require RPC will fail."
    echo "Start digibyted and wait for sync before running tests."
    echo ""
    read -p "Continue anyway? [y/N] " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "Exiting. Run tests manually later with:"
        echo "  cd $SCRIPT_DIR/build/tests && ./Google_Tests_run"
        exit 0
    fi
fi

cd tests
./Google_Tests_run
echo ""
echo "=========================================="
echo " Done!"
echo "=========================================="
