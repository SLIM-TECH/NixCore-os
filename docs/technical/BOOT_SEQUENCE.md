# NixCore OS - Boot Sequence Technical Documentation

## Boot Process Overview

### Stage 1: BIOS Boot Sector (boot/boot.asm)

#### 1. Initial Setup (Real Mode - 16-bit)
```
Address: 0x7C00
Mode: Real Mode (16-bit)
Loaded by: BIOS
```

**Operations:**
1. Save boot drive number (DL register from BIOS)
2. Initialize segment registers (DS, ES, SS = 0x0000)
3. Set stack pointer (SP = 0x7C00)
4. Clear screen (INT 0x10, AH=0x00, AL=0x03)

#### 2. A20 Line Activation
**Purpose**: Enable access to memory above 1MB

**Method**: Keyboard controller (8042)
- Disable keyboard
- Read output port
- Set bit 1 (A20 gate)
- Re-enable keyboard

**Why needed**: Without A20, address line 20 wraps around, limiting memory to 1MB

#### 3. Kernel Loading
**Source**: Disk sectors 2-51 (50 sectors = 25KB)
**Destination**: 0x2000:0x0000 (physical 0x20000)
**Method**: BIOS INT 0x13 (AH=0x02 - Read Sectors)

**Parameters:**
- CH = 0 (cylinder 0)
- CL = 2 (sector 2, first sector after boot)
- DH = 0 (head 0)
- DL = boot drive
- AL = 50 (sector count)

#### 4. Protected Mode Transition

**Step 1**: Disable interrupts (CLI)
**Step 2**: Load GDT (Global Descriptor Table)
```
GDT Structure:
- Null descriptor (0x00)
- Code segment (0x08): Base=0, Limit=4GB, Type=Code, DPL=0
- Data segment (0x10): Base=0, Limit=4GB, Type=Data, DPL=0
```

**Step 3**: Enable protected mode
```asm
mov eax, cr0
or eax, 1        ; Set PE bit
mov cr0, eax
```

**Step 4**: Far jump to flush pipeline
```asm
jmp 0x08:init_pm  ; CS=0x08 (code segment)
```

**Step 5**: Initialize segment registers (32-bit)
```asm
mov ax, 0x10     ; Data segment selector
mov ds, ax
mov ss, ax
mov es, ax
mov fs, ax
mov gs, ax
```

**Step 6**: Set up stack
```asm
mov esp, 0x90000  ; Stack at 576KB
mov ebp, 0x90000
```

**Step 7**: Jump to kernel
```asm
jmp 0x08:0x2000   ; Jump to kernel entry point
```

### Stage 2: Kernel Entry (kernel/kernel_entry.asm)

#### Protected Mode Setup (32-bit)
```
Address: 0x2000
Mode: Protected Mode (32-bit)
```

**Operations:**
1. Disable interrupts (CLI)
2. Set up kernel stack (ESP = stack_top, 16KB stack)
3. Clear EFLAGS (push 0, popf)
4. Call C kernel main (_kernel_main)
5. Halt on return (should never happen)

### Stage 3: Kernel Initialization (kernel/kernel.c)

#### Initialization Sequence

**1. VGA Initialization**
- Map VGA buffer (0xB8000)
- Disable hardware cursor (port 0x3D4/0x3D5)
- Clear screen
- Initialize terminal buffer

**2. Interrupt System**
- Set up IDT (Interrupt Descriptor Table)
- Remap PIC (Programmable Interrupt Controller)
  - Master PIC: IRQ 0-7 → INT 0x20-0x27
  - Slave PIC: IRQ 8-15 → INT 0x28-0x2F
- Install ISR handlers (32 CPU exceptions)
- Install IRQ handlers (16 hardware interrupts)
- Load IDT (LIDT instruction)
- Enable interrupts (STI)

**3. Keyboard Driver**
- Register IRQ1 handler (keyboard interrupt)
- Initialize keyboard buffer
- Set keyboard controller mode

**4. Memory Management**
- Detect available memory (CMOS)
- Initialize heap allocator
- Set up memory regions

**5. Hardware Detection**
- CPU identification (CPUID instruction)
- PCI bus scan for devices
- GPU detection (VGA, HDMI, DisplayPort support)
- Storage device enumeration (ATA/IDE)

**6. Filesystem**
- Mount root filesystem
- Check installation status
- Initialize file operations

**7. Boot Menu / Installer**
- If not installed: Run first-time installer
- If installed: Show boot menu (CLI/GUI choice)

**8. Start Selected Mode**
- CLI Mode: Initialize shell, start command loop
- GUI Mode: Initialize desktop, window manager, services

## Memory Map

```
0x00000000 - 0x000003FF : Real Mode IVT (Interrupt Vector Table)
0x00000400 - 0x000004FF : BIOS Data Area
0x00000500 - 0x00007BFF : Free conventional memory
0x00007C00 - 0x00007DFF : Boot sector (512 bytes)
0x00007E00 - 0x0007FFFF : Free conventional memory
0x00080000 - 0x0009FFFF : Extended BIOS Data Area (EBDA)
0x000A0000 - 0x000BFFFF : Video memory
0x000C0000 - 0x000FFFFF : BIOS ROM

0x00002000 - 0x0000FFFF : Kernel code and data (loaded here)
0x00010000 - 0x0008FFFF : Kernel heap and data structures
0x00090000 - 0x0009FFFF : Kernel stack (grows down from 0x90000)

0x000B8000 - 0x000B8FA0 : VGA text mode buffer (80x25x2 = 4000 bytes)
```

## Critical Boot Requirements

### BIOS Settings
1. **Boot Mode**: Legacy/CSM (not UEFI native)
2. **Secure Boot**: Disabled
3. **SATA Mode**: IDE/Legacy (not AHCI)
4. **USB Legacy Support**: Enabled (for USB keyboards)
5. **Fast Boot**: Disabled

### Hardware Requirements
1. **CPU**: x86 32-bit compatible (i386+)
2. **Memory**: Minimum 4MB, recommended 512MB
3. **Storage**: IDE/ATA compatible (PATA or SATA in IDE mode)
4. **Display**: VGA-compatible (text mode 80x25)

### Boot Media
1. **Floppy/USB**: Raw image (nixcore.img) written with DD mode
2. **CD/DVD**: ISO image (nixcore.iso) burned at low speed
3. **Hard Disk**: Install to MBR, mark partition active

## Troubleshooting Boot Issues

### Symptom: "Disk error!" message
**Causes:**
- SATA mode set to AHCI instead of IDE
- Boot device not first in boot order
- Corrupted boot sector

**Solutions:**
- Change SATA mode to IDE/Legacy in BIOS
- Reorder boot devices
- Rewrite boot media

### Symptom: Black screen after "Loading kernel..."
**Causes:**
- A20 line not enabled
- Protected mode transition failed
- Kernel not loaded correctly

**Solutions:**
- Ensure full 50 sectors loaded
- Check kernel size < 25KB
- Verify linker script (entry at 0x2000)

### Symptom: System hangs after boot
**Causes:**
- Interrupts not properly initialized
- PIC not remapped
- IDT not loaded

**Solutions:**
- Verify PIC remapping code
- Check IDT installation
- Ensure ISR handlers linked

### Symptom: No keyboard input
**Causes:**
- USB Legacy Support disabled
- IRQ1 not registered
- Keyboard controller not initialized

**Solutions:**
- Enable USB Legacy Support in BIOS
- Use PS/2 keyboard if available
- Check IRQ handler registration

## Boot Performance

**Typical boot times:**
- QEMU/KVM: 2-3 seconds
- VirtualBox: 3-5 seconds
- Real hardware: 5-10 seconds

**Boot stages:**
1. BIOS POST: 1-3 seconds
2. Boot sector load: < 1 second
3. Kernel load: < 1 second
4. Kernel init: 1-2 seconds
5. Hardware detection: 1-2 seconds
6. Boot splash: 2-3 seconds (artificial delay)
