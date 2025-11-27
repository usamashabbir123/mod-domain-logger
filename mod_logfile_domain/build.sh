#!/bin/bash
# Production build script for mod_logfile_domain
# This script automates the complete build and installation process

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
FREESWITCH_PREFIX="${FREESWITCH_PREFIX:-/usr}"
INSTALL_PREFIX="${INSTALL_PREFIX:-$FREESWITCH_PREFIX}"

echo -e "${BLUE}=== mod_logfile_domain Production Build ===${NC}"
echo "FreeSWITCH Prefix: $FREESWITCH_PREFIX"
echo "Install Prefix: $INSTALL_PREFIX"
echo ""

# Check prerequisites
echo -e "${BLUE}Checking prerequisites...${NC}"

if ! command -v autoreconf &> /dev/null; then
    echo -e "${RED}Error: autoreconf not found. Install autoconf and automake${NC}"
    exit 1
fi

if ! command -v libtoolize &> /dev/null && ! command -v glibtoolize &> /dev/null; then
    echo -e "${RED}Error: libtoolize not found. Install libtool${NC}"
    exit 1
fi

if ! command -v gcc &> /dev/null; then
    echo -e "${RED}Error: gcc not found. Install build-essential${NC}"
    exit 1
fi

if [ ! -f "${FREESWITCH_PREFIX}/include/freeswitch.h" ] && [ ! -d "${FREESWITCH_PREFIX}/include/freeswitch" ]; then
    echo -e "${RED}Error: FreeSWITCH headers not found in ${FREESWITCH_PREFIX}${NC}"
    exit 1
fi

echo -e "${GREEN}✓ All prerequisites satisfied${NC}"
echo ""

# Clean previous build
echo -e "${BLUE}Cleaning previous build artifacts...${NC}"
rm -rf configure Makefile.in Makefile config.* autom4te.cache m4 compile install-sh missing depcomp
echo -e "${GREEN}✓ Cleaned${NC}"
echo ""

# Generate build system
echo -e "${BLUE}Generating build system...${NC}"
autoreconf -i --force
echo -e "${GREEN}✓ Build system generated${NC}"
echo ""

# Configure
echo -e "${BLUE}Configuring...${NC}"
./configure --with-freeswitch="${FREESWITCH_PREFIX}" --prefix="${INSTALL_PREFIX}"
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Configuration successful${NC}"
else
    echo -e "${RED}Configuration failed${NC}"
    exit 1
fi
echo ""

# Build
echo -e "${BLUE}Building module...${NC}"
make
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Build successful${NC}"
else
    echo -e "${RED}Build failed${NC}"
    exit 1
fi
echo ""

# Show built artifacts
echo -e "${BLUE}Build artifacts:${NC}"
file mod_logfile_domain/.libs/mod_logfile_domain.so 2>/dev/null || find . -name "*.so" -o -name "*.la"
echo ""

# Installation instructions
echo -e "${GREEN}=== Build Complete ===${NC}"
echo ""
echo -e "${BLUE}To install the module:${NC}"
echo "  sudo make install"
echo ""
echo -e "${BLUE}To verify installation:${NC}"
echo "  ls ${INSTALL_PREFIX}/lib/freeswitch/mod/mod_logfile_domain.so"
echo "  ls ${INSTALL_PREFIX}/etc/freeswitch/autoload_configs/logfile_domain.conf.xml"
echo ""
echo -e "${BLUE}To test the module:${NC}"
echo "  fs_cli -x 'load mod_logfile_domain'"
echo "  fs_cli -x 'show modules'"
echo ""
