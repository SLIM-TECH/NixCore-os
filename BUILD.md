# Nix Core - Build Instructions

## Quick Start

```powershell
# Build disk image
mingw32-make nixcore.img

# Create VMDK for VirtualBox
mingw32-make vdisk

# Build bootable ISO
mingw32-make iso

# Run in QEMU
mingw32-make run
```

## Available Targets

| Target | Description |
|--------|------------|
| `nixcore.img` | Raw disk image (10MB) |
| `vdisk` | VMDK for VirtualBox |
| `iso` | Bootable ISO (11MB) |
| `run` | Run in QEMU |
| `vdisk-install` | Run with blank disk for installation |
| `run-vbox` | Show VirtualBox instructions |
| `clean` | Clean build artifacts |

## Files Generated

- `nixcore.img` - Raw образ (для QEMU, Rufus, dd)
- `nixcore.vmdk` - VirtualBox HDD
- `nixcore.iso` - Загрузочный ISO (VirtualBox, реальный ПК)

## Usage

### QEMU
```powershell
mingw32-make run
```

### VirtualBox
1. Create new VM (Type: Other, Version: DOS)
2. Use existing hard disk: `nixcore.vmdk`
3. Or mount ISO: `nixcore.iso`

### Real PC
- Записать `nixcore.img` через Rufus на USB
- Или записать `nixcore.iso` на CD/DVD

## Requirements
- MinGW (gcc, make)
- QEMU
- xorriso.exe (included in project)
