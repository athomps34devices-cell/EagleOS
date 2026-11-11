$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $PSScriptRoot

$ToolsDirectory = Join-Path $ProjectRoot "tools"
$LimineDirectory = Join-Path $ToolsDirectory "limine"
$XorrisoDirectory = Join-Path $ToolsDirectory "xorriso"

$LimineVersion = "v12.9.6"

$LimineUrl = "https://github.com/Limine-Bootloader/Limine/releases/download/$LimineVersion/limine-binary.tar.xz"
$XorrisoUrl = "https://github.com/PeyTy/xorriso-exe-for-windows/archive/refs/heads/master.zip"

$LimineExe = Get-ChildItem $LimineDirectory -Recurse -Filter "limine.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
$XorrisoExe = Get-ChildItem $XorrisoDirectory -Recurse -Filter "xorriso.exe" -ErrorAction SilentlyContinue | Select-Object -First 1

New-Item -ItemType Directory -Force -Path $ToolsDirectory | Out-Null
New-Item -ItemType Directory -Force -Path $LimineDirectory | Out-Null
New-Item -ItemType Directory -Force -Path $XorrisoDirectory | Out-Null

$TempDirectory = Join-Path $ToolsDirectory "setup-temp"

New-Item -ItemType Directory -Force -Path $TempDirectory | Out-Null

Write-Host ""
Write-Host "======================================" -ForegroundColor Magenta
Write-Host "       EagleOS Tool Setup" -ForegroundColor Magenta
Write-Host "======================================" -ForegroundColor Magenta
Write-Host ""

if (-not $LimineExe) {
    Write-Host "[1/2] Downloading Limine $LimineVersion..." -ForegroundColor Cyan

    $LimineArchive = Join-Path $TempDirectory "limine-binary.tar.xz"

    Invoke-WebRequest -Uri $LimineUrl -OutFile $LimineArchive

    Write-Host "Extracting Limine..."

    & tar.exe -xf $LimineArchive -C $LimineDirectory

    if ($LASTEXITCODE -ne 0) {
        throw "Limine extraction failed."
    }

    Remove-Item $LimineArchive -Force

    $LimineExe = Get-ChildItem $LimineDirectory -Recurse -Filter "limine.exe" | Select-Object -First 1
}
else {
    Write-Host "[1/2] Limine is already installed." -ForegroundColor Green
}

if (-not $LimineExe) {
    throw "Could not locate limine.exe after installation."
}

if (-not $XorrisoExe) {
    Write-Host "[2/2] Downloading xorriso..." -ForegroundColor Cyan

    $XorrisoArchive = Join-Path $TempDirectory "xorriso.zip"
    $XorrisoExtract = Join-Path $TempDirectory "xorriso-extracted"

    Invoke-WebRequest -Uri $XorrisoUrl -OutFile $XorrisoArchive

    Write-Host "Extracting xorriso..."

    Expand-Archive -Path $XorrisoArchive -DestinationPath $XorrisoExtract -Force

    $DownloadedXorriso = Get-ChildItem $XorrisoExtract -Recurse -Filter "xorriso.exe" | Select-Object -First 1

    if (-not $DownloadedXorriso) {
        throw "Could not locate xorriso.exe in the downloaded archive."
    }

    $Destination = Join-Path $XorrisoDirectory "xorriso-exe-for-windows-master"

    New-Item -ItemType Directory -Force -Path $Destination | Out-Null

    Copy-Item -Path $DownloadedXorriso.Directory.FullName -Destination $Destination -Recurse -Force

    Remove-Item $XorrisoArchive -Force
    Remove-Item $XorrisoExtract -Recurse -Force

    $XorrisoExe = Get-ChildItem $XorrisoDirectory -Recurse -Filter "xorriso.exe" | Select-Object -First 1
}
else {
    Write-Host "[2/2] xorriso is already installed." -ForegroundColor Green
}

if (-not $XorrisoExe) {
    throw "Could not locate xorriso.exe after installation."
}

Remove-Item $TempDirectory -Recurse -Force -ErrorAction SilentlyContinue

Write-Host ""
Write-Host "======================================" -ForegroundColor Green
Write-Host "       TOOL SETUP COMPLETE!" -ForegroundColor Green
Write-Host "======================================" -ForegroundColor Green

Write-Host ""
Write-Host "Limine:"
Write-Host $LimineExe.FullName

Write-Host ""
Write-Host "xorriso:"
Write-Host $XorrisoExe.FullName

Write-Host ""