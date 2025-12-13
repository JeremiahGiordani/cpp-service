#!/bin/bash
# Local build and run script for SAR ATR Service

set -e

echo "========================================="
echo "SAR ATR Service - Local Build"
echo "========================================="
echo ""

# Check for required tools
echo "Checking prerequisites..."

if ! command -v cmake &> /dev/null; then
    echo "Error: CMake is not installed"
    echo "Install with: brew install cmake"
    exit 1
fi

if ! command -v pkg-config &> /dev/null; then
    echo "Error: pkg-config is not installed"
    echo "Install with: brew install pkg-config"
    exit 1
fi

echo "✓ Build tools found"
echo ""

# Check for required libraries
echo "Checking for required libraries..."
MISSING_LIBS=()

if ! pkg-config --exists jsoncpp; then
    MISSING_LIBS+=("jsoncpp")
fi

if ! pkg-config --exists yaml-cpp; then
    MISSING_LIBS+=("yaml-cpp")
fi

if [ ${#MISSING_LIBS[@]} -ne 0 ]; then
    echo ""
    echo "Error: Missing required libraries: ${MISSING_LIBS[*]}"
    echo ""
    echo "Install with Homebrew:"
    echo "  brew install jsoncpp yaml-cpp boost openssl websocketpp"
    echo ""
    exit 1
fi

echo "✓ Required libraries found"
echo ""

# Create build directory
echo "Creating build directory..."
mkdir -p build
cd build

echo ""
echo "Running CMake..."
cmake ..

echo ""
echo "Building project..."
make -j$(sysctl -n hw.ncpu)

echo ""
echo "========================================="
echo "Build Complete!"
echo "========================================="
echo ""
echo "The service binary is at: build/sar_atr_service"
echo ""
echo "To run the service:"
echo "  ./build/sar_atr_service config/service_config_local.yaml"
echo ""
echo "Note: Make sure ActiveMQ is running first!"
echo "You can use Docker to run just ActiveMQ:"
echo "  docker run -p 61616:61616 -p 8161:8161 -p 61614:61614 -p 61613:61613 rmohr/activemq:5.15.9-alpine"
echo ""
