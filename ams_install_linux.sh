#!/bin/bash

set -e

CYAN='\033[0;36m'
GREEN='\033[0;32m'
NC='\033[0m'

if [ ! -d "build" ]; then
    mkdir build
fi
cd build

echo -e "${CYAN}Configuring AutomonScript:${NC}"
cmake ..

echo -e "${CYAN}Compiling AMS-Lang Engine:${NC}"
make -j$(nproc)

USER_INSTALL_DIR="$HOME/.local/bin"
SHARE_DIR="$HOME/.local/share/ams-lang"
BINARY_NAME="ams"

if [ ! -d "$USER_INSTALL_DIR" ]; then
    mkdir -p "$USER_INSTALL_DIR"
fi
if [ ! -d "$SHARE_DIR" ]; then
    mkdir -p "$SHARE_DIR"
fi

echo -e "${CYAN}Installing AMS-lang to $USER_INSTALL_DIR${NC}"
cp "./$BINARY_NAME" "$USER_INSTALL_DIR/$BINARY_NAME"
cp -r "../include" "$SHARE_DIR/include"

if [[ ":$PATH:" != *":$USER_INSTALL_DIR:"* ]]; then
    echo -e "${CYAN}Adding $USER_INSTALL_DIR to .bashrc...${NC}"
    echo "export PATH=\"\$HOME/.local/bin:\$PATH\"" >> ~/.bashrc
    echo -e "${GREEN}[!] Added to PATH. Please run 'source ~/.bashrc' or restart your terminal.${NC}"
fi

cd ..

echo -e "\n${GREEN}[SUCCESS] AutomonScript installed for current user!${NC}"
echo -e "${CYAN}Use 'ams' command on any terminal to begin.${NC}"