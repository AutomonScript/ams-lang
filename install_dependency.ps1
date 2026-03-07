# AutomonScript Environment & Dependency Setup
$RootPath = $PSScriptRoot
$ExternalDir = Join-Path $RootPath "external"
$AntlrJar = Join-Path $ExternalDir "antlr-4.13.1-complete.jar"
$RuntimeFolder = Join-Path $ExternalDir "antlr4-runtime"

# Ensure the external directory exists before starting
if (-not (Test-Path $ExternalDir)) {
    New-Item -Path $ExternalDir -ItemType Directory | Out-Null
}

Write-Host "Enviromental Setup for ASM LANG PROJECT & Installing Dependencies" -ForegroundColor Cyan

# Check for C++ Compiler (MinGW/GCC)
if (Get-Command g++ -ErrorAction SilentlyContinue) {
    $cppVer = (g++ --version | Select-Object -First 1)
    Write-Host "[v] C++ Compiler Found: $cppVer" -ForegroundColor Green
} else {
    Write-Host "[!] ERROR: g++ (MinGW) not found in PATH. Install It Before Working." -ForegroundColor Red
}

# Check for CMake
if (Get-Command cmake -ErrorAction SilentlyContinue) {
    $cmakeVer = (cmake --version | Select-Object -First 1)
    Write-Host "[v] CMake Found: $cmakeVer" -ForegroundColor Green
} else {
    Write-Host "[!] ERROR: CMake not found in PATH. Install It Before Working." -ForegroundColor Red
}

# Check for Java (Required to run the ANTLR JAR)
if (Get-Command java -ErrorAction SilentlyContinue) {
    Write-Host "[v] Java Runtime Found." -ForegroundColor Green
} else {
    Write-Host "[!] ERROR: Java not found. ANTLR cannot generate Parser code without Java. Install It Before Working" -ForegroundColor Red
}

# ANTLR Java Tool
if (Test-Path $AntlrJar) {
    Write-Host "[v] ANTLR 4.13.1 JAR already installed." -ForegroundColor Green
} else {
    Write-Host "[!] ANTLR JAR missing. Downloading..." -ForegroundColor Yellow
    Invoke-WebRequest -Uri "https://www.antlr.org/download/antlr-4.13.1-complete.jar" -OutFile $AntlrJar
    Write-Host "[+] Downloaded: $AntlrJar" -ForegroundColor Green
}

# ANTLR C++ Runtime
if (Test-Path $RuntimeFolder) {
    Write-Host "[v] ANTLR C++ Runtime folder exists." -ForegroundColor Green
} else {
    Write-Host "[!] ANTLR C++ Runtime missing. Downloading..." -ForegroundColor Yellow
    $ZipPath = Join-Path $ExternalDir "temp_runtime.zip"
    $TempExtractPath = Join-Path $ExternalDir "temp_extract"
    
    Invoke-WebRequest -Uri "https://github.com/antlr/antlr4/archive/refs/tags/4.13.1.zip" -OutFile $ZipPath
    
    # Create temp extract folder if not exists
    if (-not (Test-Path $TempExtractPath)) { New-Item -Path $TempExtractPath -ItemType Directory | Out-Null }
    
    Expand-Archive -Path $ZipPath -DestinationPath $TempExtractPath -Force
    
    # Ensure the target runtime folder exists
    if (-not (Test-Path $RuntimeFolder)) { New-Item -Path $RuntimeFolder -ItemType Directory | Out-Null }
    
    # Move specific files and remove unwanted ones
    Move-Item -Path "$TempExtractPath\antlr4-4.13.1\runtime\Cpp\*" -Destination $RuntimeFolder -Force
    
    # Clean up unwanted zip and temporary extraction folders immediately
    Remove-Item -Path $TempExtractPath -Recurse -Force
    Remove-Item -Path $ZipPath -Force
    
    Write-Host "[+] Runtime Installed to $RuntimeFolder" -ForegroundColor Green
}

Write-Host "`n Environment Setup Complete : Installed all dependencies (If any Error Fix)" -ForegroundColor Cyan