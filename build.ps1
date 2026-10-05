# BaseOS build script (Windows PowerShell)
# Run from the BaseOS folder:
#     .\build.ps1
# Build only, don't start QEMU:
#     .\build.ps1 -NoRun

param([switch]$NoRun)

$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot
[Environment]::CurrentDirectory = $PSScriptRoot   # needed for the ssd.img file calls

# ---------------------------------------------------------------
# Kill any running QEMU first, otherwise it locks build/Image
# and the clean step silently fails -> old code keeps running.
# ---------------------------------------------------------------
Get-Process qemu-system-aarch64 -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 300

# ---------------------------------------------------------------
# Freshness check: warn if a source file looks newer than what
# you probably expect. Catches the "I forgot to save" case.
# ---------------------------------------------------------------
$newest = Get-ChildItem src -Recurse -Include *.c,*.s -ErrorAction SilentlyContinue |
          Sort-Object LastWriteTime -Descending |
          Select-Object -First 1
if ($newest) {
    $ageSeconds = [int]((Get-Date) - $newest.LastWriteTime).TotalSeconds
    Write-Host "Newest source: $($newest.Name)  (saved $ageSeconds s ago)" -ForegroundColor Yellow
}

# ---------------------------------------------------------------
# Tool locations (change these only if you install somewhere else)
# ---------------------------------------------------------------
$gcc     = "C:\arm-gnu-toolchain\bin\aarch64-none-elf-gcc.exe"
$ld      = "C:\arm-gnu-toolchain\bin\aarch64-none-elf-ld.exe"
$objcopy = "C:\arm-gnu-toolchain\bin\aarch64-none-elf-objcopy.exe"
$qemu    = "C:\Program Files\qemu\qemu-system-aarch64.exe"
$dtc     = "C:\tools\dtc\dtc.exe"

foreach ($tool in $gcc, $ld, $objcopy, $qemu, $dtc) {
    if (-not (Test-Path $tool)) { throw "Missing tool: $tool" }
}

# Run a command and stop the build if it fails
function Run {
    param([string]$Exe, [string[]]$Arguments)
    & $Exe @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "FAILED: $(Split-Path $Exe -Leaf) $($Arguments -join ' ')"
    }
}

# ---------------------------------------------------------------
# 1. Device tree & Clean Directory Setup
# ---------------------------------------------------------------
if (Test-Path build) {
    Remove-Item -Recurse -Force build -ErrorAction Stop
}
if (Test-Path build) {
    throw "Failed to remove build directory - is QEMU still running?"
}
New-Item -ItemType Directory -Force -Path build | Out-Null

Write-Host "Compiling device tree..." -ForegroundColor Cyan
Run $dtc @("-I", "dts", "-O", "dtb", "-o", "build/test_board.dtb", "testboard.dts")

# ---------------------------------------------------------------
# 2. Compile sources
#    To add a new C file, add it to this list. Order = link order.
# ---------------------------------------------------------------
$includes = @("-Iinclude/driverHeaders", "-Iinclude/mkMAU", "-Iinclude/lib")

$sources = @(
    "src/string_utility.c",
    "src/ccode/main.c",
    "src/ccode/trampoline.c",
    "src/driverCode/uart.c",
    "src/driverCode/stack_track.c",
    "src/driverCode/find_ssd_device.c",
    "src/mkMAU/mkmau.c",
    "src/mkMAU/mkmau_utils.c",
    "src/mkMAU/mobilemkMAU.c",
    "src/lib/dbt.c",
    "src/lib/alignbyte.c",
    "src/lib/quicksort.c",
     "src/driverCode/virtIO.c"

)

Write-Host "Compiling..." -ForegroundColor Cyan

# boot.s must come first in the link
Run $gcc @("-c", "src/assemblycode/boot.s", "-o", "build/boot.o")
$objects = @("build/boot.o")

foreach ($src in $sources) {
    # Replace slashes to create distinct object file names
    $safeName = ($src -replace '[/\\]', '_') -replace '\.c$', '.o'
    $obj      = "build/$safeName"
    Write-Host "  $src -> $obj"
    Run $gcc (@("-c", $src) + $includes + @("-o", $obj))
    $objects += $obj
}

# ---------------------------------------------------------------
# 3. Link and make flat binary
# ---------------------------------------------------------------
Write-Host "Linking..." -ForegroundColor Cyan
Run $ld (@("--no-warn-rwx-segments", "-T", "linker/linker.ld") + $objects + @("-o", "build/boot.elf"))
Run $objcopy @("-O", "binary", "build/boot.elf", "build/Image")

# Print a timestamp + size so you can SEE that a fresh Image was produced
$img = Get-Item "build/Image"
Write-Host "Image built at $($img.LastWriteTime)  ($($img.Length) bytes)" -ForegroundColor Yellow

# ---------------------------------------------------------------
# 4. Virtual SSD with extensions (512-byte sectors)
# ---------------------------------------------------------------
Write-Host "Creating virtual SSD..." -ForegroundColor Cyan

$ssdPath = "build/ssd.img"
$fs = [System.IO.File]::Open($ssdPath, [System.IO.FileMode]::Create)
$fs.SetLength(10MB)
$fs.Close()

function Write-ExtensionToSector {
    param([string]$BinPath, [string]$ImgPath, [int]$SectorSize, [int]$SectorIndex)
    if (Test-Path $BinPath) {
        $offset   = [int64]$SectorIndex * $SectorSize
        $binBytes = [System.IO.File]::ReadAllBytes($BinPath)
        $stream   = [System.IO.File]::Open($ImgPath, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Write)
        $stream.Seek($offset, [System.IO.SeekOrigin]::Begin) | Out-Null
        $stream.Write($binBytes, 0, $binBytes.Length)
        $stream.Close()
        Write-Host "  Flashed $BinPath to sector $SectorIndex (offset $offset)" -ForegroundColor Green
    } else {
        Write-Warning "  $BinPath not found, skipping."
    }
}

Write-ExtensionToSector "first_extension.bin"  $ssdPath 512 1024
Write-ExtensionToSector "second_extension.bin" $ssdPath 512 2048

Write-Host "`nBuild OK." -ForegroundColor Green
if ($NoRun) { return }

# ---------------------------------------------------------------
# 5. Run in QEMU  (quit with Ctrl+A, then X)
# ---------------------------------------------------------------
Write-Host "Starting QEMU (Ctrl+A then X to quit)...`n" -ForegroundColor Cyan

$qemuArgs = @(
    "-M", "virt",
    "-cpu", "cortex-a53",
    "-m", "512M",
    "-nographic",
    "-dtb", "build/test_board.dtb",
    "-drive", "if=none,file=build/ssd.img,format=raw,id=hd0",
    "-device", "virtio-blk-device,drive=hd0",
    "-device", "loader,file=build/Image,addr=0x41400000,cpu-num=0"
)
& $qemu @qemuArgs