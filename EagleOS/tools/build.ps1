$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $PSScriptRoot

$KernelDirectory = Join-Path $ProjectRoot "kernel"
$SourceDirectory = Join-Path $KernelDirectory "src"
$HeaderDirectory = Join-Path $KernelDirectory "include"
$LinkerScript = Join-Path $KernelDirectory "linker.ld"
$KernelOutput = Join-Path $KernelDirectory "kernel.elf"

$DriverDirectory = Join-Path $ProjectRoot "drivers"
$DriverInputDirectory = Join-Path $DriverDirectory "input"
$DriverBusDirectory = Join-Path $DriverDirectory "bus"
$DriverStorageDirectory = Join-Path $DriverDirectory "storage"
$DriverHeaderDirectory = Join-Path $DriverDirectory "include"

$SystemDirectory = Join-Path $ProjectRoot "system"
$SystemFilesystemDirectory = Join-Path $SystemDirectory "fs"
$SystemHeaderDirectory = Join-Path $SystemDirectory "include"

$LLVM = Join-Path $env:USERPROFILE "Tools\LLVM\bin"
$Clang = Join-Path $LLVM "clang.exe"
$Linker = Join-Path $LLVM "ld.lld.exe"

if (-not (Test-Path $Clang)) {
    throw "Clang not found: $Clang"
}

if (-not (Test-Path $Linker)) {
    throw "LLD not found: $Linker"
}

$ClangResourceDirectory = (& $Clang -print-resource-dir).Trim()
$ClangBuiltinHeaders = Join-Path $ClangResourceDirectory "include"

$StdintHeader = Join-Path $ClangBuiltinHeaders "stdint.h"
$StddefHeader = Join-Path $ClangBuiltinHeaders "stddef.h"

foreach ($RequiredPath in @(
    $ClangBuiltinHeaders,
    $StdintHeader,
    $StddefHeader,
    $HeaderDirectory,
    $SourceDirectory,
    $DriverInputDirectory,
    $DriverBusDirectory,
    $DriverStorageDirectory,
    $DriverHeaderDirectory,
    $SystemFilesystemDirectory,
    $SystemHeaderDirectory,
    $LinkerScript
)) {
    if (-not (Test-Path $RequiredPath)) {
        throw "Required build path not found: $RequiredPath"
    }
}

$SourceFiles = @(
    "main.c"
    "pmm.c"
    "vmm.c"
    "idt.c"
    "pic.c"
    "irq.c"
    "pit.c"
    "clock.c"
    "rtc.c"
)

$InputDriverFiles = @(
    "keyboard.c"
)

$BusDriverFiles = @(
    "pci.c"
)

$StorageDriverFiles = @(
    "ahci.c"
    "blockdev.c"
)

$FilesystemFiles = @(
    "iso9660.c"
    "filesystem.c"
)

$AssemblyFiles = @(
    "irq.S"
)

$ObjectFiles = [System.Collections.Generic.List[string]]::new()

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "          EAGLEOS KERNEL BUILD" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$CompilerFlags = @(
    "--target=x86_64-unknown-none-elf"
    "-std=c11"
    "-O2"
    "-Wall"
    "-Wextra"
    "-ffreestanding"
    "-fno-stack-protector"
    "-fno-pic"
    "-fno-pie"
    "-fno-builtin"
    "-fno-unwind-tables"
    "-fno-asynchronous-unwind-tables"
    "-mno-red-zone"
    "-mcmodel=kernel"
    "-mno-mmx"
    "-mno-sse"
    "-mno-sse2"
    "-mgeneral-regs-only"
    "-nostdinc"
    "-isystem"
    $ClangBuiltinHeaders
    "-I"
    $HeaderDirectory
    "-I"
    $DriverHeaderDirectory
    "-I"
    $SystemHeaderDirectory
)

Write-Host "[INFO] LLVM: $LLVM" -ForegroundColor Gray
Write-Host "[INFO] Kernel headers: $HeaderDirectory" -ForegroundColor Gray
Write-Host "[INFO] Driver headers: $DriverHeaderDirectory" -ForegroundColor Gray
Write-Host "[INFO] System headers: $SystemHeaderDirectory" -ForegroundColor Gray
Write-Host "[INFO] LLVM headers: $ClangBuiltinHeaders" -ForegroundColor Gray
Write-Host ""

foreach ($SourceFile in $SourceFiles) {
    $SourcePath = Join-Path $SourceDirectory $SourceFile
    $ObjectName = [System.IO.Path]::GetFileNameWithoutExtension($SourceFile) + ".o"
    $ObjectPath = Join-Path $KernelDirectory $ObjectName

    if (-not (Test-Path $SourcePath)) {
        throw "Missing kernel source file: $SourcePath"
    }

    Write-Host "[BUILD] Compiling kernel/$SourceFile..." -ForegroundColor Yellow

    & $Clang @CompilerFlags "-c" $SourcePath "-o" $ObjectPath

    if ($LASTEXITCODE -ne 0) {
        throw "Compilation failed for kernel/$SourceFile (exit code $LASTEXITCODE)."
    }

    $ObjectFiles.Add($ObjectPath)

    Write-Host "[SUCCESS] kernel/$SourceFile compiled." -ForegroundColor Green
}

$DriverGroups = @(
    @{
        Name = "input"
        Directory = $DriverInputDirectory
        Files = $InputDriverFiles
    },
    @{
        Name = "bus"
        Directory = $DriverBusDirectory
        Files = $BusDriverFiles
    },
    @{
        Name = "storage"
        Directory = $DriverStorageDirectory
        Files = $StorageDriverFiles
    }
)

foreach ($Group in $DriverGroups) {
    foreach ($DriverSourceFile in $Group.Files) {
        $SourcePath = Join-Path $Group.Directory $DriverSourceFile
        $ObjectName = [System.IO.Path]::GetFileNameWithoutExtension($DriverSourceFile) + ".o"
        $ObjectPath = Join-Path $KernelDirectory $ObjectName

        if (-not (Test-Path $SourcePath)) {
            throw "Missing driver source file: $SourcePath"
        }

        Write-Host "[BUILD] Compiling drivers/$($Group.Name)/$DriverSourceFile..." -ForegroundColor Yellow

        & $Clang @CompilerFlags "-c" $SourcePath "-o" $ObjectPath

        if ($LASTEXITCODE -ne 0) {
            throw "Compilation failed for drivers/$($Group.Name)/$DriverSourceFile (exit code $LASTEXITCODE)."
        }

        $ObjectFiles.Add($ObjectPath)

        Write-Host "[SUCCESS] drivers/$($Group.Name)/$DriverSourceFile compiled." -ForegroundColor Green
    }
}

foreach ($FilesystemFile in $FilesystemFiles) {
    $SourcePath = Join-Path $SystemFilesystemDirectory $FilesystemFile
    $ObjectName = [System.IO.Path]::GetFileNameWithoutExtension($FilesystemFile) + ".o"
    $ObjectPath = Join-Path $KernelDirectory $ObjectName

    if (-not (Test-Path $SourcePath)) {
        throw "Missing filesystem source file: $SourcePath"
    }

    Write-Host "[BUILD] Compiling system/fs/$FilesystemFile..." -ForegroundColor Yellow

    & $Clang @CompilerFlags "-c" $SourcePath "-o" $ObjectPath

    if ($LASTEXITCODE -ne 0) {
        throw "Compilation failed for system/fs/$FilesystemFile (exit code $LASTEXITCODE)."
    }

    $ObjectFiles.Add($ObjectPath)

    Write-Host "[SUCCESS] system/fs/$FilesystemFile compiled." -ForegroundColor Green
}

foreach ($AssemblyFile in $AssemblyFiles) {
    $AssemblyPath = Join-Path $SourceDirectory $AssemblyFile
    $ObjectName = [System.IO.Path]::GetFileNameWithoutExtension($AssemblyFile) + "_asm.o"
    $ObjectPath = Join-Path $KernelDirectory $ObjectName

    if (-not (Test-Path $AssemblyPath)) {
        throw "Missing kernel assembly file: $AssemblyPath"
    }

    Write-Host "[BUILD] Assembling $AssemblyFile..." -ForegroundColor Yellow

    & $Clang `
        "--target=x86_64-unknown-none-elf" `
        "-c" `
        "-x" "assembler" `
        $AssemblyPath `
        "-o" $ObjectPath

    if ($LASTEXITCODE -ne 0) {
        throw "Assembly failed for $AssemblyFile (exit code $LASTEXITCODE)."
    }

    $ObjectFiles.Add($ObjectPath)

    Write-Host "[SUCCESS] $AssemblyFile assembled." -ForegroundColor Green
}

Write-Host ""
Write-Host "[LINK] Linking kernel objects..." -ForegroundColor Yellow

$LinkerArguments = @(
    "-T"
    $LinkerScript
    "-nostdlib"
    "-static"
    "-z"
    "max-page-size=0x1000"
    "-o"
    $KernelOutput
) + $ObjectFiles.ToArray()

& $Linker @LinkerArguments

if ($LASTEXITCODE -ne 0) {
    throw "Kernel linking failed (exit code $LASTEXITCODE)."
}

if (-not (Test-Path $KernelOutput)) {
    throw "Kernel output was not created: $KernelOutput"
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "       EAGLEOS BUILD SUCCESSFUL" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "[OUTPUT] $KernelOutput" -ForegroundColor Green
Write-Host ""