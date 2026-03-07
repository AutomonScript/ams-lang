# Prepare Build Directory
if (-not (Test-Path "build")) { New-Item -Path "build" -ItemType Directory | Out-Null }
Set-Location build

# Build AutomonScript Engine
Write-Host "Configuring AutomonScript:" -ForegroundColor Cyan
cmake -G "MinGW Makefiles" .. 

Write-Host "Compiling AMS-Lang Engine:" -ForegroundColor Cyan
mingw32-make -j8

# Setup User Installation Directory
# Installs to C:\Users\<Name>\AppData\Local\AutomonScript
$UserInstallDir = "$env:LOCALAPPDATA\AutomonScript"
$BinaryName = "ams.exe"
$FinalPath = Join-Path $UserInstallDir $BinaryName

if (-not (Test-Path $UserInstallDir)) {
    New-Item -Path $UserInstallDir -ItemType Directory | Out-Null
}

# Moving ams.exe to C:\Users\<Name>\AppData\Local\AutomonScript
Write-Host "Installing AMS-lang at User AppData (for current user)." -ForegroundColor Cyan
Copy-Item -Path ".\$BinaryName" -Destination $FinalPath -Force

# Setup Global Access (User Alias) :  Run from anywhere using ams command
# Example : asm run <filename> OR asm build <filename>
$ProfileFile = $PROFILE
if (!(Test-Path $ProfileFile)) { New-Item -Type File -Path $ProfileFile -Force | Out-Null }

$AliasFunction = "`nfunction ams { & '$FinalPath' `$args }"
$ProfileContent = Get-Content $ProfileFile

if ($ProfileContent -notcontains "function ams") {
    Add-Content $ProfileFile $AliasFunction
} else {
    $NewContent = $ProfileContent | Where-Object { $_ -notmatch "function.*ams" }
    $NewContent += $AliasFunction
    $NewContent | Set-Content $ProfileFile
}

# Finalize : BACK TO Previous Directory
Set-Location ..

# Verbose : Sucessful Installation of AutomonScript
Write-Host "`n[SUCCESS] AutomonScript installed for current user!" -ForegroundColor Green
Write-Host "Use 'ams' command on any terminal to begin." -ForegroundColor Cyan