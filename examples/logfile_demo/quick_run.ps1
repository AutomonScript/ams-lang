#!/usr/bin/env pwsh
##
## Quick Run Script - Compile and Execute Demo
##

Write-Host "=== Production Log Monitoring Demo - Quick Run ===" -ForegroundColor Cyan
Write-Host ""

## Get script directory and project root
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = Split-Path -Parent (Split-Path -Parent $scriptDir)

## Change to project root
Push-Location $projectRoot

Write-Host "[INFO] Working directory: $(Get-Location)" -ForegroundColor Gray
Write-Host ""

## Compile
Write-Host "[1/2] Compiling..." -ForegroundColor Yellow
& "./build/ams.exe" build "examples/logfile_demo.ams"

if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Compilation failed!" -ForegroundColor Red
    Pop-Location
    exit 1
}

Write-Host "[SUCCESS] Compiled!" -ForegroundColor Green
Write-Host ""

## Run
Write-Host "[2/2] Running demo (will run for ~15 seconds)..." -ForegroundColor Yellow
Write-Host "Press Ctrl+C to stop early" -ForegroundColor Gray
Write-Host ""

& "./logfile_demo.exe"

Write-Host ""
Write-Host "[DONE] Demo complete!" -ForegroundColor Green

## Return to original directory
Pop-Location
