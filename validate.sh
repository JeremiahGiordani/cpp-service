#!/bin/bash
# Test script to validate SAR ATR Service setup

set -e

echo "========================================="
echo "SAR ATR Service - Setup Validation"
echo "========================================="
echo ""

SUCCESS=0
WARNINGS=0
ERRORS=0

# Function to check file exists
check_file() {
    if [ -f "$1" ]; then
        echo "✓ Found: $1"
        SUCCESS=$((SUCCESS + 1))
    else
        echo "✗ Missing: $1"
        ERRORS=$((ERRORS + 1))
    fi
}

# Function to check directory exists
check_dir() {
    if [ -d "$1" ]; then
        echo "✓ Found: $1/"
        SUCCESS=$((SUCCESS + 1))
    else
        echo "✗ Missing: $1/"
        ERRORS=$((ERRORS + 1))
    fi
}

# Check project structure
echo "Checking project structure..."
echo ""

echo "Core Files:"
check_file "CMakeLists.txt"
check_file "Dockerfile"
check_file "docker-compose.yml"
check_file "README.md"
check_file ".gitignore"
check_file ".dockerignore"
echo ""

echo "Build Scripts:"
check_file "build.sh"
check_file "quickstart.sh"
echo ""

echo "Documentation:"
check_file "INTEGRATION_GUIDE.md"
check_file "PROJECT_SUMMARY.md"
check_file "EXAMPLE_MESSAGES.md"
echo ""

echo "Directories:"
check_dir "include"
check_dir "src"
check_dir "config"
check_dir "test_client"
echo ""

echo "Header Files:"
check_file "include/inference_engine.h"
check_file "include/mock_inference_engine.h"
check_file "include/uci_messages.h"
check_file "include/config_manager.h"
check_file "include/amq_client.h"
check_file "include/sar_atr_service.h"
check_file "include/logger.h"
echo ""

echo "Source Files:"
check_file "src/main.cpp"
check_file "src/mock_inference_engine.cpp"
check_file "src/uci_messages.cpp"
check_file "src/config_manager.cpp"
check_file "src/amq_client.cpp"
check_file "src/sar_atr_service.cpp"
echo ""

echo "Configuration:"
check_file "config/service_config.yaml"
echo ""

echo "Test Client:"
check_file "test_client/mock_test_client.py"
check_file "test_client/test_client_config.yaml"
check_file "test_client/requirements.txt"
echo ""

# Check script permissions
echo "Checking script permissions..."
if [ -x "build.sh" ]; then
    echo "✓ build.sh is executable"
    SUCCESS=$((SUCCESS + 1))
else
    echo "⚠ build.sh is not executable (run: chmod +x build.sh)"
    WARNINGS=$((WARNINGS + 1))
fi

if [ -x "quickstart.sh" ]; then
    echo "✓ quickstart.sh is executable"
    SUCCESS=$((SUCCESS + 1))
else
    echo "⚠ quickstart.sh is not executable (run: chmod +x quickstart.sh)"
    WARNINGS=$((WARNINGS + 1))
fi
echo ""

# Check for Docker
echo "Checking Docker availability..."
if command -v docker &> /dev/null; then
    echo "✓ Docker is installed"
    SUCCESS=$((SUCCESS + 1))
    
    if docker info &> /dev/null; then
        echo "✓ Docker daemon is running"
        SUCCESS=$((SUCCESS + 1))
    else
        echo "⚠ Docker daemon is not running"
        WARNINGS=$((WARNINGS + 1))
    fi
else
    echo "⚠ Docker is not installed"
    WARNINGS=$((WARNINGS + 1))
fi

if command -v docker-compose &> /dev/null; then
    echo "✓ Docker Compose is installed"
    SUCCESS=$((SUCCESS + 1))
else
    echo "⚠ Docker Compose is not installed"
    WARNINGS=$((WARNINGS + 1))
fi
echo ""

# Check for Python (for test client)
echo "Checking Python availability..."
if command -v python3 &> /dev/null; then
    PYTHON_VERSION=$(python3 --version)
    echo "✓ Python3 is installed: $PYTHON_VERSION"
    SUCCESS=$((SUCCESS + 1))
else
    echo "⚠ Python3 is not installed (needed for test client)"
    WARNINGS=$((WARNINGS + 1))
fi
echo ""

# Summary
echo "========================================="
echo "Validation Summary"
echo "========================================="
echo "✓ Successful checks: $SUCCESS"
echo "⚠ Warnings: $WARNINGS"
echo "✗ Errors: $ERRORS"
echo ""

if [ $ERRORS -eq 0 ] && [ $WARNINGS -eq 0 ]; then
    echo "🎉 All checks passed! Project is ready."
    echo ""
    echo "Next steps:"
    echo "1. Review README.md for detailed instructions"
    echo "2. Run ./quickstart.sh to start the service"
    echo "3. Run the test client to send test messages"
    exit 0
elif [ $ERRORS -eq 0 ]; then
    echo "✓ Project structure is complete!"
    echo "⚠ Some optional components missing (see warnings above)"
    echo ""
    echo "You can still:"
    echo "- Build and run the service"
    echo "- Review the documentation"
    exit 0
else
    echo "✗ Some required files are missing"
    echo "Please check the errors above"
    exit 1
fi
