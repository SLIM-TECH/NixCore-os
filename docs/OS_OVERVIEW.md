# OS Overview

## Boot Flow

1. BIOS loads `boot/boot.asm` from the first sector.
2. The boot sector reads the kernel from disk using BIOS LBA read.
3. The bootloader switches to protected mode.
4. Control jumps to `_start` in `kernel/kernel_entry.asm`.
5. `_kernel_main` initializes interrupts, keyboard, memory, hardware probing and filesystem.

## Runtime Layers

- `kernel/`: boot, interrupts, hardware probing, top-level startup
- `drivers/`: VGA text, mouse, keyboard, sound
- `fs/`: simple persistent ATA-backed filesystem
- `shell/`: CLI command interpreter
- `gui/`: desktop rendering, windows, services
- `apps/`: desktop apps
- `lang/`: NixLang interpreter and `.bin` loader

## Storage

The current filesystem is a fixed in-memory node table with ATA sector persistence.

- metadata sectors: `100..109`
- file content sectors: `110 + file_index`

## Graphics

There are two rendering paths:

- VGA text mode for CLI
- a simple framebuffer-oriented GUI path

The GUI is currently a software-drawn desktop profile for `1280x720`.

## Hardware Detection

Current probing:

- CPU vendor and brand via `CPUID`
- PCI scan for display/audio/USB/storage controllers
- ATA disk detect on primary/secondary channels
- basic RAM estimate from CMOS

## Installer

The first-boot installer currently:

- detects ATA disks
- lets the user choose a target disk
- writes installation marker/config
- creates user folders

It is still text-driven, not a full framebuffer installer yet.
