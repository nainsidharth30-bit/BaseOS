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

# 6. Launch QEMU BaseOS Environment:
#    - Pass custom DTB natively (replaces internal DTB at 0x40000000)
#    - Load Kernel Image at 0x41400000 and start CPU 0 execution
qemu-system-aarch64 `
  -M virt `
  -cpu cortex-a53 `
  -m 512M `
  -nographic `
  -dtb build/test_board.dtb `
  -device loader,file=build/Image,addr=0x41400000,cpu-num=0 `
  -device loader,addr=0x41400000,cpu-num=0