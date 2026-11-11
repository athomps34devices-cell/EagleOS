$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $PSScriptRoot

Set-Location $ProjectRoot

$BuildDirectory = Join-Path $ProjectRoot "build"
$IsoRoot = Join-Path $BuildDirectory "iso"
$IsoOutput = Join-Path $BuildDirectory "EagleOS.iso"

$LimineDirectory = Join-Path $ProjectRoot "tools\limine"
$XorrisoDirectory = Join-Path $ProjectRoot "tools\xorriso"

function Find-ToolFile {
    param(
        [string]$Directory,
        [string]$FileName
    )

    $File = Get-ChildItem $Directory -Recurse -File -Filter $FileName |
        Select-Object -First 1

    if (-not $File) {
        throw "Could not find $FileName in $Directory"
    }

    return $File.FullName
}

function ConvertTo-CygwinPath {
    param([string]$WindowsPath)

    $FullPath = [System.IO.Path]::GetFullPath($WindowsPath)
    $Drive = $FullPath.Substring(0, 1).ToLower()
    $RemainingPath = $FullPath.Substring(2).Replace("\", "/")

    return "/cygdrive/$Drive$RemainingPath"
}

Write-Host ""
Write-Host "======================================" -ForegroundColor Magenta
Write-Host "        EagleOS ISO Builder" -ForegroundColor Magenta
Write-Host "======================================" -ForegroundColor Magenta
Write-Host ""

Write-Host "[1/6] Locating development tools..." -ForegroundColor Cyan

$LimineExe = Find-ToolFile $LimineDirectory "limine.exe"
$XorrisoExe = Find-ToolFile $XorrisoDirectory "xorriso.exe"

$LimineBios = Find-ToolFile $LimineDirectory "limine-bios.sys"
$LimineBiosCd = Find-ToolFile $LimineDirectory "limine-bios-cd.bin"
$LimineUefiCd = Find-ToolFile $LimineDirectory "limine-uefi-cd.bin"
$LimineEfi = Find-ToolFile $LimineDirectory "BOOTX64.EFI"

Write-Host "[SUCCESS] Limine and xorriso found." -ForegroundColor Green

Write-Host ""
Write-Host "[2/6] Building EagleOS kernel..." -ForegroundColor Cyan

$BuildScript = Join-Path $ProjectRoot "tools\build.ps1"

if (-not (Test-Path $BuildScript)) {
    throw "Could not find tools/build.ps1"
}

& $BuildScript

$Kernel = Join-Path $ProjectRoot "kernel\kernel.elf"

if (-not (Test-Path $Kernel)) {
    throw "Kernel ELF was not generated."
}

Write-Host "[SUCCESS] Kernel build complete." -ForegroundColor Green

Write-Host ""
Write-Host "[3/6] Preparing ISO directory..." -ForegroundColor Cyan

if (Test-Path $IsoRoot) {
    Remove-Item $IsoRoot -Recurse -Force
}

New-Item -ItemType Directory -Force -Path $IsoRoot | Out-Null

$BootDirectory = Join-Path $IsoRoot "boot"
$LimineBootDirectory = Join-Path $BootDirectory "limine"
$EfiDirectory = Join-Path $IsoRoot "EFI\BOOT"

New-Item -ItemType Directory -Force -Path $LimineBootDirectory | Out-Null
New-Item -ItemType Directory -Force -Path $EfiDirectory | Out-Null

Write-Host "[SUCCESS] ISO directory prepared." -ForegroundColor Green

Write-Host ""
Write-Host "[4/6] Copying boot files..." -ForegroundColor Cyan

Copy-Item $Kernel (Join-Path $BootDirectory "kernel.elf")

$LimineConfig = Join-Path $ProjectRoot "boot\limine.conf"

if (-not (Test-Path $LimineConfig)) {
    throw "Could not find boot/limine.conf"
}

Copy-Item $LimineConfig (Join-Path $LimineBootDirectory "limine.conf")

Copy-Item $LimineBios $LimineBootDirectory
Copy-Item $LimineBiosCd $LimineBootDirectory
Copy-Item $LimineUefiCd $LimineBootDirectory

Copy-Item $LimineEfi (Join-Path $EfiDirectory "BOOTX64.EFI")

foreach ($RequiredFile in @(
    (Join-Path $BootDirectory "kernel.elf"),
    (Join-Path $LimineBootDirectory "limine.conf"),
    (Join-Path $LimineBootDirectory "limine-bios.sys"),
    (Join-Path $LimineBootDirectory "limine-bios-cd.bin"),
    (Join-Path $LimineBootDirectory "limine-uefi-cd.bin"),
    (Join-Path $EfiDirectory "BOOTX64.EFI")
)) {
    if (-not (Test-Path $RequiredFile)) {
        throw "Required ISO boot file missing: $RequiredFile"
    }
}

Write-Host "[SUCCESS] All boot files copied." -ForegroundColor Green

Write-Host ""
Write-Host "[5/6] Creating hybrid BIOS/UEFI ISO..." -ForegroundColor Cyan

if (-not (Test-Path $BuildDirectory)) {
    New-Item -ItemType Directory -Force -Path $BuildDirectory | Out-Null
}

if (Test-Path $IsoOutput) {
    Remove-Item $IsoOutput -Force
}

$XorrisoOutputPath = ConvertTo-CygwinPath $IsoOutput

Push-Location $IsoRoot

try {
    & $XorrisoExe `
        -as mkisofs `
        -R `
        -r `
        -J `
        -b "boot/limine/limine-bios-cd.bin" `
        -no-emul-boot `
        -boot-load-size 4 `
        -boot-info-table `
        -hfsplus `
        -apm-block-size 2048 `
        --efi-boot "boot/limine/limine-uefi-cd.bin" `
        -efi-boot-part `
        --efi-boot-image `
        --protective-msdos-label `
        -o $XorrisoOutputPath `
        .

    if ($LASTEXITCODE -ne 0) {
        throw "xorriso failed to create the ISO. Exit code: $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}

if (-not (Test-Path $IsoOutput)) {
    throw "xorriso did not create the ISO: $IsoOutput"
}

Write-Host "[SUCCESS] ISO image created." -ForegroundColor Green

Write-Host ""
Write-Host "[6/6] Installing Limine BIOS boot code..." -ForegroundColor Cyan

& $LimineExe bios-install $IsoOutput

if ($LASTEXITCODE -ne 0) {
    throw "Limine BIOS installation failed. Exit code: $LASTEXITCODE"
}

$IsoSize = [math]::Round((Get-Item $IsoOutput).Length / 1MB, 2)

Write-Host ""
Write-Host "======================================" -ForegroundColor Green
Write-Host "       EAGLEOS ISO CREATED!" -ForegroundColor Green
Write-Host "======================================" -ForegroundColor Green

Write-Host ""
Write-Host "ISO: $IsoOutput"
Write-Host "Size: $IsoSize MB"
Write-Host ""