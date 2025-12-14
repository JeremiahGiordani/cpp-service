#!/usr/bin/env bash
set -euo pipefail

# build_linux.sh
# Build script for Linux (RHEL9-compatible) that does not require sudo.
# Features:
# - Checks for required build tools (cmake, pkg-config, g++, make)
# - Attempts to detect required libs via pkg-config (jsoncpp, yaml-cpp)
# - Optionally builds/install missing deps into a local prefix (default: $HOME/.local)
# - Configures and builds the project using CMake and Make

PREFIX="${PREFIX:-$HOME/.local}"
JOBS=${JOBS:-$(nproc 2>/dev/null || echo 1)}
ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"

usage() {
    cat <<EOF
Usage: $0 [--deps-local] [--prefix PATH] [--jobs N]

Options:
  --deps-local    Build and install missing dependencies into local prefix (no sudo)
  --prefix PATH   Install prefix for local dependencies (default: $PREFIX)
  --jobs N        Parallel jobs for build (default: $JOBS)
  -h, --help      Show this help
EOF
}

DEPS_LOCAL=0

while [[ ${#} -gt 0 ]]; do
    case "$1" in
        --deps-local) DEPS_LOCAL=1; shift;;
        --prefix) PREFIX="$2"; shift 2;;
        --jobs) JOBS="$2"; shift 2;;
        -h|--help) usage; exit 0;;
        --) shift; break;;
        *) echo "Unknown arg: $1"; usage; exit 1;;
    esac
done

echo "Linux build (no-sudo)"
# Prevent accidental runs on macOS (Darwin). This script targets RHEL/CentOS-like Linux.
if [[ "$(uname -s)" == "Darwin" && "${FORCE_LINUX:-0}" != "1" ]]; then
        cat <<MSG
This script is intended for Linux (RHEL9). You're running on macOS (Darwin).

To build on macOS, use the repository's `build.sh` or adjust this script.
If you really want to force this script on macOS (not recommended), set:
    FORCE_LINUX=1 $0 [--deps-local]

Exiting to avoid confusion.
MSG
        exit 1
fi
echo "Project root: $ROOT_DIR"
echo "Install prefix: $PREFIX"
echo "Jobs: $JOBS"

require_cmd() {
    if ! command -v "$1" &>/dev/null; then
        echo "Error: required command '$1' not found. Please install it (or enable --deps-local where supported)."
        exit 1
    fi
}

require_cmd cmake
require_cmd pkg-config
require_cmd g++
require_cmd make

# Prefer wget, fall back to curl
DOWNLOAD=()
if command -v wget &>/dev/null; then
    DOWNLOAD=(wget -q -O-)
elif command -v curl &>/dev/null; then
    DOWNLOAD=(curl -sL)
else
    echo "Error: need 'wget' or 'curl' to download dependency sources."; exit 1
fi

check_pkg() {
    pkg-config --exists "$1" 2>/dev/null
}

MISSING_LIBS=()
if ! check_pkg jsoncpp; then
    MISSING_LIBS+=(jsoncpp)
fi
if ! check_pkg yaml-cpp; then
    MISSING_LIBS+=(yaml-cpp)
fi

echo "Checking for required libraries..."
if [ ${#MISSING_LIBS[@]} -ne 0 ]; then
    echo "Missing libraries detected: ${MISSING_LIBS[*]}"
    if [ "$DEPS_LOCAL" -eq 1 ]; then
        echo "Will attempt to build and install missing libraries into $PREFIX"
    else
        cat <<EOF
Error: Missing required libraries: ${MISSING_LIBS[*]}

Since this environment does not allow sudo, you have two options:
  1) Install the libraries system-wide (requires sudo / admin): e.g. on RHEL/DNF
       sudo dnf install -y cmake pkgconfig jsoncpp-devel yaml-cpp-devel boost-devel openssl-devel
  2) Re-run this script with --deps-local to build jsoncpp and yaml-cpp into ${PREFIX} (no sudo).

If you choose option 2, rerun:
  $0 --deps-local --prefix ${PREFIX} --jobs ${JOBS}

EOF
        exit 1
    fi
else
    echo "✓ Required libraries found via pkg-config"
fi

ensure_prefix_env() {
    # Ensure pkg-config and CMake can find local installs
    export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
    export CMAKE_PREFIX_PATH="$PREFIX:${CMAKE_PREFIX_PATH:-}"
    export LD_LIBRARY_PATH="$PREFIX/lib:${LD_LIBRARY_PATH:-}"
    export PATH="$PREFIX/bin:${PATH:-}"
}

build_jsoncpp() {
    echo "Building jsoncpp into $PREFIX..."
    tmpdir=$(mktemp -d)
    pushd "$tmpdir" >/dev/null
    git clone --depth 1 --branch 1.9.5 https://github.com/open-source-parsers/jsoncpp.git jsoncpp-src
    mkdir -p jsoncpp-src/build
    pushd jsoncpp-src/build >/dev/null
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX" ..
    make -j"$JOBS"
    make install
    popd >/dev/null
    popd >/dev/null
    rm -rf "$tmpdir"
}

build_yaml_cpp() {
    echo "Building yaml-cpp into $PREFIX..."
    tmpdir=$(mktemp -d)
    pushd "$tmpdir" >/dev/null
    git clone --depth 1 --branch yaml-cpp-0.7.0 https://github.com/jbeder/yaml-cpp.git yaml-cpp-src
    mkdir -p yaml-cpp-src/build
    pushd yaml-cpp-src/build >/dev/null
    cmake -DYAML_BUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX" ..
    make -j"$JOBS"
    make install
    popd >/dev/null
    popd >/dev/null
    rm -rf "$tmpdir"
}

has_websocketpp() {
    # Check common install locations and local prefix
    if [ -f "$PREFIX/include/websocketpp/config/asio_no_tls_client.hpp" ]; then
        return 0
    fi
    if [ -f "/usr/include/websocketpp/config/asio_no_tls_client.hpp" ]; then
        return 0
    fi
    if [ -f "/usr/local/include/websocketpp/config/asio_no_tls_client.hpp" ]; then
        return 0
    fi
    return 1
}

build_websocketpp() {
    echo "Installing websocketpp headers into $PREFIX..."
    tmpdir=$(mktemp -d)
    pushd "$tmpdir" >/dev/null
    git clone --depth 1 https://github.com/zaphoyd/websocketpp.git
    mkdir -p "$PREFIX/include"
    # The websocketpp repo places headers in a top-level 'websocketpp' directory
    cp -r websocketpp "$PREFIX/include/"
    popd >/dev/null
    rm -rf "$tmpdir"
}

build_boost() {
    echo "Building Boost (libboost_system) into $PREFIX. This can be large and may take several minutes."
    tmpdir=$(mktemp -d)
    pushd "$tmpdir" >/dev/null
    BOOST_VER="1_80_0"
    BOOST_TARBALL="boost_${BOOST_VER}.tar.bz2"
    BOOST_URL="https://boostorg.jfrog.io/artifactory/main/release/1.80.0/source/${BOOST_TARBALL}"
    echo "Downloading Boost ${BOOST_VER}..."
    if command -v wget &>/dev/null; then
        wget -q "$BOOST_URL"
    else
        curl -sLO "$BOOST_URL"
    fi
    tar -xf "$BOOST_TARBALL"
    cd "boost_1_80_0"
    ./bootstrap.sh --prefix="$PREFIX" --with-libraries=system
    ./b2 install -j"$JOBS"
    popd >/dev/null
    rm -rf "$tmpdir"
}

if [ "$DEPS_LOCAL" -eq 1 ]; then
    ensure_prefix_env
    # Re-check after setting prefix
    REMAINING=()
    if ! pkg-config --exists jsoncpp; then
        build_jsoncpp
    fi
    if ! pkg-config --exists yaml-cpp; then
        build_yaml_cpp
    fi
    # websocketpp is header-only and may not be available via pkg-config; install locally if missing
    if ! has_websocketpp; then
        echo "websocketpp headers not found; installing locally into $PREFIX"
        build_websocketpp
    fi
    # Boost is required for Boost::system; try CMake configure and if it fails, offer to build boost
    echo "Attempting CMake configure to verify Boost availability..."
    mkdir -p "$ROOT_DIR/build"
    pushd "$ROOT_DIR/build" >/dev/null
    if ! cmake .. -DCMAKE_PREFIX_PATH="$PREFIX" >/dev/null 2>&1; then
        echo "CMake configure failed (Boost or other libs missing)."
        read -r -p "Do you want to attempt building Boost locally into $PREFIX? [y/N]: " yn || true
        case "$yn" in
            [Yy]*) build_boost ;;
            *) echo "Skipping Boost build. CMake configure will likely fail. Exiting."; exit 1 ;;
        esac
    else
        echo "CMake configure succeeded.";
    fi
    popd >/dev/null
fi

# Final build
ensure_prefix_env
mkdir -p "$ROOT_DIR/build"
cd "$ROOT_DIR/build"

echo "Running CMake..."
cmake .. -DCMAKE_PREFIX_PATH="$PREFIX"

echo "Building project (make -j$JOBS)..."
make -j"$JOBS"

echo ""
echo "========================================="
echo "Build Complete"
echo "Binary: $ROOT_DIR/build/sar_atr_service"
echo "To run (ensure runtime libs are found):"
echo "  export LD_LIBRARY_PATH=\"$PREFIX/lib:\$LD_LIBRARY_PATH\""
echo "  $ROOT_DIR/build/sar_atr_service config/service_config_local.yaml"
echo ""
echo "If you installed deps into $PREFIX, you may want to add the following to your shell rc:"
echo "  export PATH=\"$PREFIX/bin:\$PATH\""
echo "  export PKG_CONFIG_PATH=\"$PREFIX/lib/pkgconfig:\$PKG_CONFIG_PATH\""
echo "  export LD_LIBRARY_PATH=\"$PREFIX/lib:\$LD_LIBRARY_PATH\""
