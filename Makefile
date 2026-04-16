CC = gcc
AS = nasm
LD = ld
OBJCOPY = objcopy

CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
         -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -c \
         -fno-pie -ffreestanding -Iinclude -fno-asynchronous-unwind-tables \
         -mno-stack-arg-probe

ASFLAGS = -f coff
LDFLAGS = -m i386pe -T linker.ld
QEMUIMG = qemu-img
XORRISO = ./xorriso.exe

CFLAGS += -ID:/msys64/mingw32/include

BUILDDIR = build
FIX_SLASH = $(subst /,\,$1)

C_SOURCES = $(wildcard kernel/*.c) $(wildcard drivers/*.c) $(wildcard fs/*.c) \
            $(wildcard shell/*.c) $(wildcard games/*.c) $(wildcard lib/*.c) \
            $(wildcard lang/*.c) $(wildcard gui/*.c) $(wildcard apps/*.c)
ASM_SOURCES = $(wildcard kernel/kernel_entry.asm) $(wildcard kernel/isr.asm) $(wildcard drivers/*.asm)
BOOT_ASM = boot/boot.asm

C_OBJECTS = $(patsubst %.c,$(BUILDDIR)/%.o,$(C_SOURCES))
ASM_OBJECTS = $(patsubst %.asm,$(BUILDDIR)/%.o,$(ASM_SOURCES))
OBJECTS = $(BUILDDIR)/kernel/kernel_entry.o $(BUILDDIR)/kernel/gdt_simple.asm.o $(BUILDDIR)/kernel/isr.o $(C_OBJECTS)

KERNEL_PE = $(BUILDDIR)/kernel.pe
KERNEL = $(BUILDDIR)/kernel.bin
BOOTLOADER = $(BUILDDIR)/boot.bin
IMG = nixcore.img
VDISK = nixcore.vmdk
ISO = nixcore.iso
INSTALL_IMG = install.img

all: $(IMG)

# Real Mode fallback - simple and reliable
realmode: boot/boot_realmode.asm
	$(AS) -f bin boot/boot_realmode.asm -o nixcore_realmode.img
	@echo Real Mode image created: nixcore_realmode.img
	@echo Run with: qemu-system-i386 -drive format=raw,file=nixcore_realmode.img

run-realmode: realmode
	qemu-system-i386 -drive format=raw,file=nixcore_realmode.img -m 512M

$(BUILDDIR):
	@powershell -Command "New-Item -ItemType Directory -Force -Path 'build' | Out-Null"
	@powershell -Command "New-Item -ItemType Directory -Force -Path 'build/kernel' | Out-Null"
	@powershell -Command "New-Item -ItemType Directory -Force -Path 'build/drivers' | Out-Null"
	@powershell -Command "New-Item -ItemType Directory -Force -Path 'build/boot' | Out-Null"
	@powershell -Command "New-Item -ItemType Directory -Force -Path 'build/fs' | Out-Null"
	@powershell -Command "New-Item -ItemType Directory -Force -Path 'build/gui' | Out-Null"
	@powershell -Command "New-Item -ItemType Directory -Force -Path 'build/apps' | Out-Null"
	@powershell -Command "New-Item -ItemType Directory -Force -Path 'build/games' | Out-Null"
	@powershell -Command "New-Item -ItemType Directory -Force -Path 'build/lib' | Out-Null"
	@powershell -Command "New-Item -ItemType Directory -Force -Path 'build/lang' | Out-Null"
	@powershell -Command "New-Item -ItemType Directory -Force -Path 'build/shell' | Out-Null"

$(BUILDDIR)/%.o: %.c | $(BUILDDIR)
	@powershell -Command "if (!(Test-Path '$(dir $@)')) { New-Item -ItemType Directory -Force -Path '$(dir $@)' | Out-Null }"
	$(CC) $(CFLAGS) $< -o $@

$(BUILDDIR)/%.o: %.asm | $(BUILDDIR)
	@powershell -Command "if (!(Test-Path '$(dir $@)')) { New-Item -ItemType Directory -Force -Path '$(dir $@)' | Out-Null }"
	$(AS) $(ASFLAGS) $< -o $@

$(BOOTLOADER): $(BOOT_ASM) | $(BUILDDIR)
	$(AS) -f bin $(BOOT_ASM) -o $@

$(BUILDDIR)/kernel/gdt_simple.asm.o: kernel/gdt_simple.asm | $(BUILDDIR)
	$(AS) $(ASFLAGS) kernel/gdt_simple.asm -o $@

$(KERNEL_PE): $(OBJECTS) | $(BUILDDIR)
	$(CC) -m32 -nostdlib -T linker.ld $(BUILDDIR)/kernel/kernel_entry.o $(BUILDDIR)/kernel/gdt_simple.asm.o -Wl,--start-group $(C_OBJECTS) $(BUILDDIR)/kernel/isr.o -Wl,--end-group -o $@
	@echo Kernel linked.

$(KERNEL): $(KERNEL_PE)
	$(OBJCOPY) -O binary $(KERNEL_PE) $@
	@echo Raw kernel image created.

$(IMG): $(BOOTLOADER) $(KERNEL)
	@echo Creating disk image...
	@powershell -NoProfile -ExecutionPolicy Bypass -File makeimg.ps1

$(VDISK): $(IMG)
	@echo Creating VMDK virtual disk...
	powershell -NoProfile -Command "if (Test-Path '$(VDISK)') { Remove-Item '$(VDISK)' -Force }; & 'C:\Program Files\qemu\qemu-img.exe' convert -f raw -O vmdk '$(IMG)' '$(VDISK)'"
	@echo VMDK created: $(VDISK)

vdisk: $(VDISK)
	@echo Running QEMU with virtual disk...
	powershell -NoProfile -Command "Start-Process -FilePath 'qemu-system-i386' -ArgumentList '-drive', 'file=nixcore.vmdk,format=vmdk,if=ide', '-m', '512M', '-soundhci', 'AC97', '-net', 'nic', '-net', 'user'"

vdisk-install: $(VDISK)
	@echo Running QEMU with empty disk for installation...
	powershell -NoProfile -Command "if (-not (Test-Path '$(ISO)')) { Write-Host 'ISO not found. Run make iso first.' }; Start-Process -FilePath 'qemu-system-i386' -ArgumentList '-drive', 'file=nixcore.vmdk,format=vmdk,if=ide', '-m', '512M', '-cdrom', '$(ISO)', '-boot', 'd'"

$(INSTALL_IMG):
	@echo Creating blank install image...
	fsutil file createnew $(INSTALL_IMG) 2147483648

vdisk-new: $(INSTALL_IMG)
	@echo Running QEMU with blank disk for fresh install...
	qemu-system-i386 -drive file=$(INSTALL_IMG),format=raw,if=ide -m 512M -cdrom $(ISO) -boot d

run: $(IMG)
	qemu-system-i386 -drive format=raw,file=$(IMG) -m 512M -device AC97

iso: $(IMG)
	@echo Creating bootable ISO...
	powershell -NoProfile -Command " \
	    New-Item -ItemType Directory -Force -Path 'iso_build\boot\grub' | Out-Null; \
	    Copy-Item 'nixcore.img' 'iso_build\boot\nixcore.img'; \
	    'set timeout=0', 'menuentry \"Nix Core OS\" {', '    linux16 /boot/nixcore.img', '}' | Out-File -FilePath 'iso_build\boot\grub\grub.cfg' -Encoding ascii"
	$(XORRISO) -as mkisofs -b boot/nixcore.img -no-emul-boot -boot-load-size 4 -o $(ISO) iso_build
	@echo ISO created: $(ISO)

iso-full: iso
	@echo.
	@echo Or use with QEMU: qemu-system-i386 -cdrom nixcore.iso -m 512M

run-vbox: $(VDISK)
	@echo Run in VirtualBox with $(VDISK)
	@echo To use in VirtualBox:
	@echo 1. Open VirtualBox
	@echo 2. Create new VM - Type: Other, Version: DOS
	@echo 3. Use existing hard disk: $(VDISK)
	@echo 4. Or mount ISO: $(ISO)

clean:
	@powershell -Command "if (Test-Path 'build') { Remove-Item -Recurse -Force 'build' }"
	@powershell -Command "if (Test-Path 'nixcore.img') { Remove-Item -Force 'nixcore.img' }"
	@powershell -Command "if (Test-Path 'nixcore.vmdk') { Remove-Item -Force 'nixcore.vmdk' }"
	@powershell -Command "if (Test-Path 'nixcore.iso') { Remove-Item -Force 'nixcore.iso' }"
	@powershell -Command "if (Test-Path 'install.img') { Remove-Item -Force 'install.img' }"
	@powershell -Command "if (Test-Path 'iso_root') { Remove-Item -Recurse -Force 'iso_root' }"
	@powershell -Command "if (Test-Path 'iso_dir') { Remove-Item -Recurse -Force 'iso_dir' }"
	@powershell -Command "if (Test-Path 'iso_build') { Remove-Item -Recurse -Force 'iso_build' }"

.PHONY: all run vdisk vdisk-install vdisk-new iso iso-full run-vbox clean