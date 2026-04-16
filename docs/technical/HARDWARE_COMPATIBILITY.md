# NixCore OS - Hardware Compatibility Guide

## Display Output Compatibility

### Supported Video Interfaces

#### VGA (Video Graphics Array)
- **Status**: Fully supported
- **Resolution**: 80x25 text mode
- **Hardware**: All systems with VGA-compatible BIOS
- **Use case**: Legacy systems, virtual machines, basic compatibility

#### HDMI (High-Definition Multimedia Interface)
- **Status**: Hardware detected, VGA text mode fallback
- **Compatibility**: Modern Intel/AMD/NVIDIA GPUs
- **Notes**: BIOS provides VGA compatibility mode on HDMI ports
- **Tested**: Intel HD Graphics, NVIDIA GeForce, AMD Radeon

#### DisplayPort
- **Status**: Hardware detected, VGA text mode fallback
- **Compatibility**: Modern discrete and integrated GPUs
- **Notes**: DisplayPort includes VGA compatibility in legacy boot
- **Tested**: Intel integrated, NVIDIA discrete, AMD discrete

#### DVI (Digital Visual Interface)
- **Status**: Hardware detected, VGA text mode fallback
- **Compatibility**: Older discrete GPUs (2005-2015 era)
- **Notes**: DVI-I and DVI-A include analog VGA signals

#### eDP (Embedded DisplayPort)
- **Status**: Supported on laptops
- **Compatibility**: Modern laptop internal displays
- **Notes**: BIOS initializes display before OS boot

## GPU Vendor Support

### Intel Graphics
- **Integrated GPUs**: HD Graphics, UHD Graphics, Iris
- **Outputs**: HDMI, DisplayPort, eDP (laptops), VGA (older)
- **Status**: Fully compatible
- **Detection**: Vendor ID 0x8086

### NVIDIA Graphics
- **Discrete GPUs**: GeForce series
- **Outputs**: HDMI, DisplayPort, DVI, VGA (adapters)
- **Status**: Fully compatible
- **Detection**: Vendor ID 0x10DE

### AMD Graphics
- **Discrete/Integrated**: Radeon, Ryzen APU graphics
- **Outputs**: HDMI, DisplayPort, DVI, VGA (older)
- **Status**: Fully compatible
- **Detection**: Vendor ID 0x1002, 0x1022

### Virtual Machine Graphics
- **QEMU/KVM**: Bochs VGA, VirtIO GPU
- **VirtualBox**: VirtualBox Graphics Adapter
- **VMware**: VMware SVGA II
- **Status**: Fully compatible
- **Detection**: Vendor IDs 0x1234, 0x80EE, 0x15AD

## Boot Requirements

### BIOS/Legacy Mode
- **Required**: Yes
- **UEFI**: Must enable CSM (Compatibility Support Module)
- **Secure Boot**: Must be disabled

### Storage Requirements
- **Interface**: IDE/ATA (PATA or SATA in IDE mode)
- **Not supported yet**: AHCI, NVMe
- **BIOS setting**: Set SATA mode to "IDE" or "Legacy"

### Memory Requirements
- **Minimum**: 4 MB
- **Recommended**: 512 MB
- **Maximum tested**: 4 GB

## Troubleshooting

### Black Screen on Boot

**Symptom**: System boots but no display output

**Solutions**:
1. Try different video output (VGA, HDMI, DisplayPort)
2. Enable Legacy/CSM mode in BIOS
3. Disable Secure Boot
4. Set SATA mode to IDE/Legacy
5. Try lower screen resolution in BIOS

### Disk Not Detected

**Symptom**: "Disk error!" message

**Solutions**:
1. Change SATA mode from AHCI to IDE
2. Ensure boot device is first in boot order
3. For USB boot: use DD mode in Rufus
4. For CD boot: burn ISO at lowest speed

### Hangs After "Loading kernel..."

**Symptom**: Boot stops after kernel load message

**Solutions**:
1. Increase VM memory to 512MB
2. Disable CPU virtualization extensions temporarily
3. Try different VM software (QEMU vs VirtualBox)
4. On real hardware: update BIOS

### No Keyboard Input

**Symptom**: System boots but keyboard doesn't work

**Solutions**:
1. Enable USB Legacy Support in BIOS
2. Use PS/2 keyboard if available
3. Disable Fast Boot in BIOS
4. Try different USB port (USB 2.0 preferred)

## Tested Hardware Configurations

### Virtual Machines
- ✅ QEMU 7.0+ (i386, x86_64 host)
- ✅ VirtualBox 6.0+ (all host platforms)
- ✅ VMware Workstation 16+
- ✅ VMware ESXi 7.0+
- ✅ Hyper-V (Generation 1 VMs only)

### Real Hardware
- ✅ Intel Core i3/i5/i7 (2nd gen+) with Intel HD Graphics
- ✅ AMD Ryzen with Radeon Graphics
- ✅ Discrete NVIDIA GeForce (GTX 700+)
- ✅ Discrete AMD Radeon (HD 7000+)
- ✅ Legacy Pentium 4 / Core 2 Duo systems
- ✅ Laptops with Intel/AMD integrated graphics

### Display Connections Tested
- ✅ VGA analog cable
- ✅ HDMI cable (all versions)
- ✅ DisplayPort cable (DP 1.2+)
- ✅ DVI-D and DVI-I cables
- ✅ Laptop internal displays (eDP)
- ✅ VGA-to-HDMI adapters
- ✅ DisplayPort-to-HDMI adapters

## Future Enhancements

### Planned Features
- UEFI native boot support
- VESA framebuffer graphics mode
- AHCI storage driver
- NVMe storage driver
- GOP (Graphics Output Protocol) support
- Higher resolution text modes

### Not Planned
- 3D acceleration
- Hardware video decoding
- Multi-monitor support
- Hot-plug display detection
