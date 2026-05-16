#!/usr/bin/env pwsh
##
## Simple Demo Runner - Direct Execution with Real-Time Output
##

Write-Host "============================================================================" -ForegroundColor Cyan
Write-Host "PRODUCTION LOG MONITORING SYSTEM - SIMPLE DEMO" -ForegroundColor Cyan
Write-Host "============================================================================" -ForegroundColor Cyan
Write-Host ""

## Get script directory and project root
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = Split-Path -Parent (Split-Path -Parent $scriptDir)

## Change to project root
Push-Location $projectRoot

## Check if AMS compiler exists
if (-not (Test-Path "build/ams.exe")) {
    Write-Host "[ERROR] AMS compiler not found at build/ams.exe" -ForegroundColor Red
    Write-Host "[ERROR] Please build the compiler first: cmake --build build --config Release" -ForegroundColor Red
    Pop-Location
    exit 1
}

Write-Host "[STEP 1] Compiling AMS demo program..." -ForegroundColor Yellow
Write-Host ""

## Compile the AMS program
& "./build/ams.exe" build "examples/logfile_demo.ams"

if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Compilation failed!" -ForegroundColor Red
    Pop-Location
    exit 1
}

Write-Host "[SUCCESS] Compilation completed!" -ForegroundColor Green
Write-Host ""

Write-Host "[STEP 2] Running log monitoring system..." -ForegroundColor Yellow
Write-Host ""
Write-Host "The demo will monitor 3 log files and trigger events." -ForegroundColor Cyan
Write-Host "You will see:" -ForegroundColor Cyan
Write-Host "  - SOURCE blocks checking log files every few seconds" -ForegroundColor Cyan
Write-Host "  - EVENT blocks triggered when conditions are met" -ForegroundColor Cyan
Write-Host "  - OBSERVER blocks responding to events" -ForegroundColor Cyan
Write-Host ""
Write-Host "Press Ctrl+C to stop the demo at any time." -ForegroundColor Yellow
Write-Host ""
Write-Host "============================================================================" -ForegroundColor Cyan
Write-Host ""

Start-Sleep -Seconds 2

## Run the demo directly (will run until Ctrl+C)
& "./logfile_demo.exe"

Write-Host ""
Write-Host "============================================================================" -ForegroundColor Cyan
Write-Host "DEMO STOPPED" -ForegroundColor Cyan
Write-Host "============================================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "[INFO] Demo execution completed" -ForegroundColor Green
Write-Host ""
Write-Host "Log files location: examples/logfile_demo/" -ForegroundColor Cyan
Write-Host "Demo program: logfile_demo.exe" -ForegroundColor Cyan
Write-Host ""

## Return to original directory
Pop-Location
