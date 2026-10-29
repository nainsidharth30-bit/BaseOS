# BaseOS build script (Windows PowerShell)
# Run from the BaseOS folder:
#     .\build.ps1
# Options:
#     .\build.ps1 -NoRun      build only, don't start QEMU
#     .\build.ps1 -Modern     ask QEMU for modern virtio-mmio (version 2) instead of legacy (version 1)
#     .\build.ps1 -QemuLog    save QEMU's complaints about bad device accesses to build\qemu.log

param([switch]$NoRun, [switch]$Modern, [switch]$QemuLog)

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
# Freshness check: shows when the newest source file was saved.
# Catches the "I forgot to save" case.
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
$includes = @(
    "-Iinclude/driverHeaders", 
    "-Iinclude/mkMAU", 
    "-Iinclude/lib",
    "-Iinclude/interrupts"
)

$sources = @(
    "src/string_utility.c",
    "src/ccode/main.c",
    "src/ccode/trampoline.c",
    "src/driverCode/uart.c",
    "src/driverCode/stack_track.c",
    "src/mkMAU/mkmau.c",
    "src/mkMAU/mkmau_utils.c",
    "src/mkMAU/mobilemkMAU.c",
    "src/lib/dbt.c",
    "src/lib/memcpy.c",
    "src/lib/alignbyte.c",
    "src/lib/setzero.c",
    "src/lib/quicksort.c",
    "src/driverCode/virtIO.c",
    "src/lib/compare_start_address.c",
    "src/interrupts/arm_cortex_a15_gic.c",
    "src/interrupts/arm_pl190_vic.c",
    "src/driverCode/ahci.c",
    "src/interrupts/enable_irq.c",
    "src/interrupts/devices_setup.c"
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
#
#    Layout rules (must match the C-side meta_ext parser exactly):
#
#    Sector 1 (base sector, from DTB: baseos,sector-number-extensions = <1>)
#      offset 0   : next_sector_number (uint64, LE) = 0xFFFFFFFFFFFFFFFF -> no more sectors
#      offset 8   : 24 bytes reserved, zero-filled
#      offset 32  : meta_ext entry[0]  (first_extension)
#      offset 121 : meta_ext entry[1]  (second_extension)
#      offset 210 : sentinel meta_ext entry, uniq_id = 0xFFFFFFFF -> "no more entries"
#      rest       : zero-filled up to 512
#
#    meta_ext on-disk layout (PACKED, 89 bytes total -- the C struct
#    MUST be declared __attribute__((packed)) or offsets will not match):
#      [0 .. 63]   extension_name   (64 bytes, ASCII, zero-padded)
#      [64.. 67]   extension_type   (uint32, LE)
#      [68]        load_at_boot_extension (uint8)
#      [69..76]    sector_number    (uint64, LE)  -- where the extension's raw bytes start
#      [77..80]    byte_offset      (uint32, LE)  -- always 0 for now
#      [81..84]    size_of_extension(uint32, LE)
#      [85..88]    uniq_id          (uint32, LE)  -- which SSD controller this belongs to
# ---------------------------------------------------------------

Write-Host "Creating virtual SSD..." -ForegroundColor Cyan

$ssdPath = "build/ssd.img"
$fs = [System.IO.File]::Open($ssdPath, [System.IO.FileMode]::Create)
$fs.SetLength(10MB)
$fs.Close()

$SECTOR_SIZE = 512

# 1024 bytes (= 2 sectors) of repeating text
function New-PlaceholderExtension {
    param([string]$Path, [string]$Label)
    $pattern = [System.Text.Encoding]::ASCII.GetBytes("BASEOS-PLACEHOLDER-$Label ")
    $bytes   = New-Object byte[] 1024
    for ($i = 0; $i -lt 1024; $i++) { $bytes[$i] = $pattern[$i % $pattern.Length] }
    [System.IO.File]::WriteAllBytes($Path, $bytes)
}

function Write-ExtensionToSector {
    param([string]$BinPath, [string]$Label, [string]$ImgPath, [int]$SectorSize, [int]$SectorIndex)

    $source = $BinPath
    if (-not (Test-Path $BinPath)) {
        $source = "build/$Label.placeholder.bin"
        New-PlaceholderExtension $source $Label.ToUpper()
        Write-Warning "  $BinPath not found. Using a PLACEHOLDER file instead."
    }

    $offset   = [int64]$SectorIndex * $SectorSize
    $binBytes = [System.IO.File]::ReadAllBytes($source)
    $stream   = [System.IO.File]::Open($ImgPath, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Write)
    $stream.Seek($offset, [System.IO.SeekOrigin]::Begin) | Out-Null
    $stream.Write($binBytes, 0, $binBytes.Length)
    $stream.Close()
    Write-Host "  Flashed $source ($($binBytes.Length) bytes) to sector $SectorIndex (offset $offset)" -ForegroundColor Green
    return $binBytes.Length
}

# ---------------------------------------------------------------
# 4a. Flash the two actual extensions to their data sectors
# ---------------------------------------------------------------
$firstSector  = 1024
$secondSector = 2048

$firstSize  = Write-ExtensionToSector "first_extension.bin"  "first"  $ssdPath $SECTOR_SIZE $firstSector
$secondSize = Write-ExtensionToSector "second_extension.bin" "second" $ssdPath $SECTOR_SIZE $secondSector

# ---------------------------------------------------------------
# 4b. Build one packed meta_ext entry (89 bytes) as raw bytes
# ---------------------------------------------------------------
function New-MetaExtEntry {
    param(
        [string]$Name,
        [uint32]$Type,
        [byte]$LoadAtBoot,
        [uint64]$SectorNumber,
        [uint32]$ByteOffset,
        [uint32]$SizeOfExtension,
        [uint32]$UniqId
    )

    $buf = New-Object byte[] 89

    # extension_name[64], ASCII, zero-padded, truncated if too long
    $nameBytes = [System.Text.Encoding]::ASCII.GetBytes($Name)
    $copyLen   = [Math]::Min($nameBytes.Length, 63)   # leave room for null terminator
    [Array]::Copy($nameBytes, 0, $buf, 0, $copyLen)
    # remaining bytes in [0..63] are already 0 from New-Object byte[]

    [Array]::Copy([BitConverter]::GetBytes($Type),            0, $buf, 64, 4)
    $buf[68] = $LoadAtBoot
    [Array]::Copy([BitConverter]::GetBytes($SectorNumber),    0, $buf, 69, 8)
    [Array]::Copy([BitConverter]::GetBytes($ByteOffset),      0, $buf, 77, 4)
    [Array]::Copy([BitConverter]::GetBytes($SizeOfExtension), 0, $buf, 81, 4)
    [Array]::Copy([BitConverter]::GetBytes($UniqId),          0, $buf, 85, 4)

    return $buf
}

# extension_type: 1 = Driver (per your own convention). Adjust per-extension if needed.
# load_at_boot:   1 = yes, autoload at boot.
# uniq_id:        1  (matches DTB: baseos,uniq-id = <1> on the virtio_block node)

$entry0 = New-MetaExtEntry -Name "first_extension"  -Type 1 -LoadAtBoot 1 `
            -SectorNumber $firstSector  -ByteOffset 0 -SizeOfExtension $firstSize  -UniqId 1

$entry1 = New-MetaExtEntry -Name "second_extension" -Type 1 -LoadAtBoot 0 `
            -SectorNumber $secondSector -ByteOffset 0 -SizeOfExtension $secondSize -UniqId 1

# Sentinel entry: uniq_id = 0xFFFFFFFF marks "no more entries in this sector"
# NOTE: on Windows PowerShell 5.1, the hex literal 0xFFFFFFFF is parsed as a
# signed Int32 and wraps to -1, which then fails to cast into [uint32]. Using
# [uint32]::MaxValue avoids that entirely and is unambiguous in any PS version.
$sentinel = New-MetaExtEntry -Name "" -Type 0 -LoadAtBoot 0 `
            -SectorNumber 0 -ByteOffset 0 -SizeOfExtension 0 -UniqId ([uint32]::MaxValue)

# ---------------------------------------------------------------
# 4c. Assemble the base sector (sector 1) and write it
# ---------------------------------------------------------------
$baseSectorIndex = 1   # must match DTB: baseos,sector-number-extensions = <1>

$sector = New-Object byte[] $SECTOR_SIZE   # zero-filled by default

# offset 0: next_sector_number = -1 (no more chained sectors, for now)
$nextSectorNumber = [uint64]::MaxValue
[Array]::Copy([BitConverter]::GetBytes($nextSectorNumber), 0, $sector, 0, 8)
# offset 8..31: reserved, left as zero

# offset 32 onward: the meta_ext entries
$writeOffset = 32
[Array]::Copy($entry0,   0, $sector, $writeOffset, $entry0.Length);   $writeOffset += $entry0.Length
[Array]::Copy($entry1,   0, $sector, $writeOffset, $entry1.Length);   $writeOffset += $entry1.Length
[Array]::Copy($sentinel, 0, $sector, $writeOffset, $sentinel.Length)

$baseOffsetBytes = [int64]$baseSectorIndex * $SECTOR_SIZE
$stream = [System.IO.File]::Open($ssdPath, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Write)
$stream.Seek($baseOffsetBytes, [System.IO.SeekOrigin]::Begin) | Out-Null
$stream.Write($sector, 0, $sector.Length)
$stream.Close()

Write-Host "  Wrote base sector (meta_ext table) to sector $baseSectorIndex (offset $baseOffsetBytes)" -ForegroundColor Green

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
if ($Modern)  { $qemuArgs += @("-global", "virtio-mmio.force-legacy=false") }
if ($QemuLog) { $qemuArgs += @("-d", "guest_errors", "-D", "build/qemu.log") }

& $qemu @qemuArgs

if ($QemuLog) { Write-Host "`nQEMU log: build\qemu.log" -ForegroundColor Yellow }