# Building NixCore OS

## Prerequisites

### Windows (MinGW)

1. **Install MinGW-w64**
   - Download from https://mingw-w64.org/
   - Add to PATH: `C:\MinGW\bin`

2. **Install NASM**
   - Download from https://www.nasm.us/
   - Add to PATH

3. **Install QEMU**
   - Download from https://www.qemu.org/download/#windows
   - Add to PATH: `C:\Program Files\qemu`

4. **Install Make**
   - Included with MinGW or install separately

### Linux (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install build-essential nasm qemu-system-x86 git
```

### macOS

```bash
brew install nasm qemu gcc
```

## Building from Source

### Clone Repository

```bash
git clone https://github.com/yourusername/nixcore.git
cd nixcore
```

### Build Commands

```bash
# Clean previous builds
make clean

# Build kernel and drivers
make all

# Create bootable image
make image

# Run in QEMU
make run

# Run with debugging
make debug
```

### Build Targets

- `make all` - Compile everything
- `make kernel` - Build kernel only
- `make drivers` - Build drivers only
- `make clean` - Remove build artifacts
- `make image` - Create nixcore.img
- `make run` - Run in QEMU
- `make debug` - Run with GDB support

## Build Output

After successful build, you'll have:

```
build/
├── boot.bin          # Bootloader (512 bytes)
├── kernel.bin        # Kernel binary
├── kernel.pe         # Kernel PE executable
├── drivers/          # Compiled drivers
├── lib/              # Compiled libraries
└── apps/             # Compiled applications

nixcore.img           # Bootable disk image (10MB)
```

## Troubleshooting

### "gcc: command not found"

**Windows**: Add MinGW to PATH
```powershell
$env:Path += ";C:\MinGW\bin"
```

**Linux**: Install gcc
```bash
sudo apt install gcc
```

### "nasm: command not found"

Download and install NASM from https://www.nasm.us/

### "ld: cannot find -lgcc"

Install gcc multilib:
```bash
sudo apt install gcc-multilib
```

### Build fails with "Permission denied"

**Windows**: Run as Administrator
**Linux**: Check file permissions
```bash
chmod +x build.sh
```

### QEMU doesn't start

Check QEMU installation:
```bash
qemu-system-x86_64 --version
```

## Advanced Build Options

### Custom Compiler Flags

Edit `Makefile` and modify `CFLAGS`:

```makefile
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin \
         -fno-stack-protector -Wall -Wextra -O2
```

### Debug Build

```bash
make CFLAGS="-g -O0" all
```

### Cross-Compilation

For cross-compiling from Linux to x86:

```bash
sudo apt install gcc-multilib g++-multilib
make CC=gcc LD=ld all
```

## Creating Bootable USB

### Linux

```bash
# Find USB device
lsblk

# Write image (replace sdX with your device)
sudo dd if=nixcore.img of=/dev/sdX bs=4M status=progress
sudo sync
```

### Windows

Use **Rufus** or **Win32DiskImager**:

1. Download Rufus from https://rufus.ie/
2. Select nixcore.img
3. Select USB drive
4. Click "Start"

### macOS

```bash
# Find USB device
diskutil list

# Unmount
diskutil unmountDisk /dev/diskX

# Write image
sudo dd if=nixcore.img of=/dev/rdiskX bs=4m
```

## Testing in Virtual Machines

### QEMU

```bash
# Basic boot
qemu-system-x86_64 -drive file=nixcore.img,format=raw -m 512M

# With network
qemu-system-x86_64 -drive file=nixcore.img,format=raw -m 512M \
    -netdev user,id=net0 -device e1000,netdev=net0

# With USB
qemu-system-x86_64 -drive file=nixcore.img,format=raw -m 512M \
    -usb -device usb-mouse -device usb-kbd

# With logging
qemu-system-x86_64 -drive file=nixcore.img,format=raw -m 512M \
    -serial file:qemu.log -d int,cpu_reset
```

### VirtualBox

1. Create new VM (Type: Other, Version: Other/Unknown 64-bit)
2. Allocate 512MB RAM
3. Use existing hard disk: nixcore.img
4. Enable EFI (if using UEFI boot)
5. Start VM

### VMware

1. Create new VM (Other 64-bit)
2. Allocate 512MB RAM
3. Use existing disk: nixcore.img
4. Enable UEFI firmware
5. Start VM

## Continuous Integration

### GitHub Actions

Create `.github/workflows/build.yml`:

```yaml
name: Build NixCore

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v2
    
    - name: Install dependencies
      run: |
        sudo apt update
        sudo apt install -y build-essential nasm qemu-system-x86
    
    - name: Build
      run: make all
    
    - name: Create image
      run: make image
    
    - name: Upload artifact
      uses: actions/upload-artifact@v2
      with:
        name: nixcore-image
        path: nixcore.img
```

## Performance Optimization

### Compiler Optimizations

```makefile
# Speed optimization
CFLAGS += -O3 -march=native

# Size optimization
CFLAGS += -Os -ffunction-sections -fdata-sections
LDFLAGS += -Wl,--gc-sections

# Link-time optimization
CFLAGS += -flto
LDFLAGS += -flto
```

### Parallel Build

```bash
# Use all CPU cores
make -j$(nproc) all
```

## Debugging

### GDB Debugging

Terminal 1:
```bash
qemu-system-x86_64 -drive file=nixcore.img,format=raw -m 512M -s -S
```

Terminal 2:
```bash
gdb build/kernel.pe
(gdb) target remote localhost:1234
(gdb) break kernel_main
(gdb) continue
```

### Serial Output

```bash
qemu-system-x86_64 -drive file=nixcore.img,format=raw -m 512M \
    -serial stdio
```

### Memory Dump

```bash
qemu-system-x86_64 -drive file=nixcore.img,format=raw -m 512M \
    -monitor stdio
(qemu) info registers
(qemu) x/10i $rip
(qemu) info mem
```

## Common Issues

### Kernel doesn't boot

- Check bootloader is exactly 512 bytes
- Verify kernel is loaded at correct address
- Enable QEMU logging: `-d int,cpu_reset`

### Disk errors

- Verify image format: `file nixcore.img`
- Check image size: `ls -lh nixcore.img`
- Recreate image: `make clean && make image`

### Network not working

- Enable E1000 device in QEMU
- Check driver initialization in logs
- Verify DHCP server is running

### USB devices not detected

- Enable USB controller in VM
- Check USB driver initialization
- Try different USB controller (XHCI/EHCI/UHCI)
