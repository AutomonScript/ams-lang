#!/bin/bash

ROOT_PATH=$(pwd)
EXTERNAL_DIR="$ROOT_PATH/external"
ANTLR_JAR="$EXTERNAL_DIR/antlr-4.13.1-complete.jar"
RUNTIME_FOLDER="$EXTERNAL_DIR/antlr4-runtime"

CYAN='\033[0;36m'
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${CYAN}Environmental Setup for ASM LANG PROJECT${NC}"

check_dep() {
    if ! command -v "$1" &> /dev/null; then
        echo -e "${RED}[!] ERROR: $1 is not installed. $2${NC}"
        exit 1
    fi
}

check_dep "g++" "Install with: sudo apt install build-essential"
check_dep "cmake" "Install with: sudo apt install cmake"
check_dep "java" "Install with: sudo apt install default-jre"
check_dep "unzip" "Install with: sudo apt install unzip"
check_dep "curl" "Install with: sudo apt install curl"

echo -e "${GREEN}[v] All system build tools found.${NC}"

mkdir -p "$EXTERNAL_DIR"

# ANTLR Java Tool
if [ -f "$ANTLR_JAR" ]; then
    echo -e "${GREEN}[v] ANTLR 4.13.1 JAR already installed.${NC}"
else
    echo -e "${YELLOW}[!] ANTLR JAR missing. Downloading...${NC}"
    curl -L "https://www.antlr.org/download/antlr-4.13.1-complete.jar" -o "$ANTLR_JAR"
fi

# ANTLR C++ Runtime
if [ -d "$RUNTIME_FOLDER" ]; then
    echo -e "${GREEN}[v] ANTLR C++ Runtime folder exists.${NC}"
else
    echo -e "${YELLOW}[!] ANTLR C++ Runtime missing. Downloading...${NC}"
    ZIP_PATH="$EXTERNAL_DIR/temp_runtime.zip"
    TEMP_EXTRACT_PATH="$EXTERNAL_DIR/temp_extract"

    curl -L "https://github.com/antlr/antlr4/archive/refs/tags/4.13.1.zip" -o "$ZIP_PATH"
    mkdir -p "$TEMP_EXTRACT_PATH"
    unzip -q "$ZIP_PATH" -d "$TEMP_EXTRACT_PATH"
    
    mkdir -p "$RUNTIME_FOLDER"
    cp -r "$TEMP_EXTRACT_PATH/antlr4-4.13.1/runtime/Cpp/"* "$RUNTIME_FOLDER/"
    
    rm -rf "$TEMP_EXTRACT_PATH"
    rm "$ZIP_PATH"
    echo -e "${GREEN}[+] Runtime Installed to $RUNTIME_FOLDER${NC}"
fi

echo -e "\n${CYAN}Environment Setup Complete: Ready to build.${NC}"