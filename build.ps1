# Ensure build directory exists
New-Item -ItemType Directory -Force -Path build | Out-Null

# 1. Locate dtc.exe dynamically
$dtcExe = (Get-ChildItem -Path "$env:LOCALAPPDATA\Microsoft\WinGet\Packages" -Filter "dtc.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -ExpandProperty FullName -First 1)

if (-not $dtcExe) {
    $dtcExe = "dtc"
}

# 2. Compile custom testboard.dts -> build/test_board.dtb
& $dtcExe -I dts -O dtb -o build/test_board.dtb testboard.dts

# 3. Compile Assembly & C source files
aarch64-none-elf-gcc -c src/assemblycode/boot.s -o build/boot.o
aarch64-none-elf-gcc -c src/ccode/main.c -Iinclude/driverHeaders -Iinclude/mkMAU -Iinclude/lib -o build/main.o
aarch64-none-elf-gcc -c src/ccode/trampoline.c -Iinclude/driverHeaders -Iinclude/mkMAU -Iinclude/lib -o build/trampoline.o
aarch64-none-elf-gcc -c src/driverCode/uart.c -Iinclude/driverHeaders -Iinclude/mkMAU -Iinclude/lib -o build/uart.o
aarch64-none-elf-gcc -c src/mkMAU/mkmau.c -Iinclude/driverHeaders -Iinclude/mkMAU -Iinclude/lib -o build/mkmau.o
aarch64-none-elf-gcc -c src/mkMAU/mkmau_utils.c -Iinclude/driverHeaders -Iinclude/mkMAU -Iinclude/lib -o build/mkmau_utils.o
aarch64-none-elf-gcc -c src/mkMAU/mobilemkMAU.c -Iinclude/driverHeaders -Iinclude/mkMAU -Iinclude/lib -o build/mobilemkMAU.o
aarch64-none-elf-gcc -c src/lib/dbt.c -Iinclude/driverHeaders -Iinclude/mkMAU -Iinclude/lib -o build/dbt.o
aarch64-none-elf-gcc -c src/lib/alignbyte.c -Iinclude/driverHeaders -Iinclude/mkMAU -Iinclude/lib -o build/alignbyte.o
aarch64-none-elf-gcc -c src/lib/quicksort.c -Iinclude/driverHeaders -Iinclude/mkMAU -Iinclude/lib -o build/quicksort.o

# 4. Link object files into ELF
aarch64-none-elf-ld --no-warn-rwx-segments -T linker/linker.ld build/boot.o build/main.o build/trampoline.o build/uart.o build/mkmau.o build/mkmau_utils.o build/mobilemkMAU.o build/dbt.o build/alignbyte.o build/quicksort.o -o build/boot.elf

# 5. Extract flat raw binary from ELF
aarch64-none-elf-objcopy -O binary build/boot.elf build/Image

# 6. Create and Populate Virtual SSD (ssd.img) with Extensions
$ssdPath = "build/ssd.img"
$fs = [System.IO.File]::Open($ssdPath, [System.IO.FileMode]::Create)
$fs.SetLength(10MB) # Create a 10MB virtual disk
$fs.Close()

function Write-ExtensionToSector {
    param($binPath, $imgPath, $sectorSize, $sectorIndex)
    if (Test-Path $binPath) {
        $offset = $sectorIndex * $sectorSize
        $binBytes = [System.IO.File]::ReadAllBytes($binPath)
        $imgStream = [System.IO.File]::Open($imgPath, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Write)
        $imgStream.Seek($offset, [System.IO.SeekOrigin]::Begin) | Out-Null
        $imgStream.Write($binBytes, 0, $binBytes.Length)
        $imgStream.Close()
        Write-Host "Successfully flashed $binPath to sector $sectorIndex (Offset $offset)" -ForegroundColor Green
    } else {
        Write-Warning "Binary not found: $binPath. Skipping flash."
    }
}

# Flash extensions assuming 512-byte sectors
Write-ExtensionToSector "first_extension.bin" $ssdPath 512 1024
Write-ExtensionToSector "second_extension.bin" $ssdPath 512 2048

# 7. Launch QEMU BaseOS Environment:
#    - Pass custom DTB natively (replaces internal DTB at 0x40000000)
#    - Load Kernel Image at 0x41400000 and start CPU 0 execution
#    - Attach emulated SSD via virtio-blk
qemu-system-aarch64 `
  -M virt `
  -cpu cortex-a53 `
  -m 512M `
  -nographic `
  -dtb build/test_board.dtb `
  -drive if=none,file=build/ssd.img,format=raw,id=hd0 `
  -device virtio-blk-device,drive=hd0 `
  -device loader,file=build/Image,addr=0x41400000,cpu-num=0 `
  -device loader,addr=0x41400000,cpu-num=0