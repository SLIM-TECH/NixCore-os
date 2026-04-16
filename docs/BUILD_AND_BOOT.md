# Build And Boot

## Build Output

- `build/boot.bin`: 512-byte BIOS boot sector
- `build/kernel.pe`: linked intermediate kernel image
- `build/kernel.bin`: raw flat kernel binary
- `nixcore.img`: bootable raw disk image

## Why The Raw Kernel Exists

The BIOS boot sector cannot jump into a PE executable directly.

The build therefore uses:

1. link to `build/kernel.pe`
2. convert with `objcopy -O binary`
3. write `build/kernel.bin` into the disk image

## QEMU

Known working command:

```powershell
qemu-system-i386 -drive format=raw,file=nixcore.img -m 512M -device AC97
```

## Real Hardware Notes

The current boot path targets:

- BIOS boot
- legacy ATA-style disks
- 32-bit x86

It does not yet fully support:

- UEFI
- AHCI-only systems
- NVMe-only systems
- modern framebuffer handoff

## ISO

A proper bootable ISO is not produced in this environment right now because no ISO authoring tool is installed.

To add ISO output later, one of these tools is needed:

- `grub-mkrescue`
- `xorriso`
- `mkisofs`
