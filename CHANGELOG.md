# NixCore OS - Changelog

## Version 1.0.1 - Critical Bug Fixes (2026-04-15)

### Boot System Fixes

#### 1. A20 Line Initialization
- **Fixed**: A20 gate not enabled, causing memory access failures above 1MB
- **Impact**: System now properly accesses extended memory
- **File**: `boot/boot.asm`

#### 2. Protected Mode Transition
- **Fixed**: BIOS interrupts incorrectly called after entering 32-bit mode
- **Impact**: Clean transition to protected mode without crashes
- **File**: `boot/boot.asm`

#### 3. GDT Segment Calculation
- **Fixed**: Hardcoded segment selectors instead of calculated offsets
- **Impact**: Proper segment descriptor addressing
- **File**: `boot/boot.asm`

### Interrupt System Fixes

#### 4. PIC Initialization
- **Fixed**: Programmable Interrupt Controller not properly remapped
- **Impact**: Hardware interrupts now work correctly (keyboard, timer, etc.)
- **File**: `kernel/interrupts.c`

#### 5. IDT Installation
- **Fixed**: Interrupt Descriptor Table created but never loaded
- **Impact**: CPU exceptions and hardware interrupts properly handled
- **File**: `kernel/interrupts.c`

#### 6. ISR/IRQ Handler Linking
- **Fixed**: Interrupt service routines not linked due to COFF symbol naming
- **Impact**: All 32 CPU exceptions and 16 hardware IRQs now functional
- **Files**: `kernel/isr.asm`, `kernel/interrupts.c`, `Makefile`

### Hardware Detection Fixes

#### 7. Video Output Detection
- **Fixed**: Missing HDMI and DisplayPort in GPU detection
- **Impact**: Proper detection of modern video outputs
- **Supported**: VGA, HDMI, DisplayPort, DVI, eDP
- **File**: `kernel/hardware.c`

#### 8. VGA Cursor Disable
- **Fixed**: Hardware cursor visible and blinking
- **Impact**: Clean text display without cursor artifacts
- **File**: `drivers/vga.c`

### Build System Fixes

#### 9. Makefile Shell Compatibility
- **Fixed**: Windows shell command syntax errors
- **Impact**: Clean builds on Windows with MinGW
- **File**: `Makefile`

#### 10. Link Order and Groups
- **Fixed**: ISR object file not properly linked
- **Impact**: All interrupt handlers correctly included in kernel
- **File**: `Makefile`

## Hardware Compatibility Improvements

### Display Outputs Now Supported
- ✅ VGA (all systems)
- ✅ HDMI (Intel, NVIDIA, AMD)
- ✅ DisplayPort (Intel, NVIDIA, AMD)
- ✅ DVI (older discrete GPUs)
- ✅ eDP (laptop internal displays)

### Tested Platforms
- ✅ QEMU/KVM virtual machines
- ✅ VirtualBox VMs
- ✅ VMware VMs
- ✅ Real hardware with modern GPUs
- ✅ Legacy BIOS systems

## Technical Documentation Added

### New Documentation Files
- `docs/technical/BUGFIXES.md` - Detailed bug fix documentation
- `docs/technical/HARDWARE_COMPATIBILITY.md` - Hardware compatibility guide
- `docs/technical/BOOT_SEQUENCE.md` - Complete boot process documentation

## Known Issues

### Not Yet Fixed
1. UEFI native boot not implemented (requires CSM/Legacy mode)
2. AHCI/NVMe storage not supported (IDE/ATA only)
3. Graphics limited to VGA text mode (no framebuffer)

## Build Instructions

```powershell
# Clean build
mingw32-make clean

# Build disk image
mingw32-make

# Run in QEMU
mingw32-make run
```

## Testing

### Virtual Machine Testing
```powershell
# QEMU
qemu-system-i386 -drive format=raw,file=nixcore.img -m 512M

# VirtualBox
# Use nixcore.vmdk or nixcore.iso
```

### Real Hardware Testing
1. Write `nixcore.img` to USB with Rufus (DD mode)
2. Boot in Legacy/CSM mode (disable UEFI)
3. Disable Secure Boot
4. Set SATA mode to IDE/Legacy

## Contributors
- CIBERSSH - Original author
- Bug fixes and improvements - 2026-04-15
