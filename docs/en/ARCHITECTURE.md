# NixCore OS - Architecture Overview

## System Architecture

NixCore is a modern 64-bit operating system with a monolithic kernel architecture. The system is designed with modularity in mind while maintaining high performance through direct hardware access.

### Core Components

```
┌─────────────────────────────────────────────────────────────┐
│                     User Applications                        │
│  Firefox | Tor | GCC | Python | Git | Shell | File Manager │
└─────────────────────────────────────────────────────────────┘
                            ▲
                            │ System Calls
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                      Standard Library                        │
│         libc (malloc, printf, string, math, etc.)           │
└─────────────────────────────────────────────────────────────┘
                            ▲
                            │ Syscall Interface
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                      Kernel Space                            │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │   Process    │  │   Memory     │  │     VFS      │     │
│  │  Scheduler   │  │  Management  │  │  (FAT32/ext2)│     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │   Network    │  │     GUI      │  │     IPC      │     │
│  │   Stack      │  │  Compositor  │  │   (Pipes)    │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
└─────────────────────────────────────────────────────────────┘
                            ▲
                            │ Hardware Interface
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                      Device Drivers                          │
│   AHCI | NVMe | E1000 | USB (XHCI/EHCI/UHCI) | PCI         │
└─────────────────────────────────────────────────────────────┘
                            ▲
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                         Hardware                             │
│      CPU | RAM | Disk | Network | USB | Graphics           │
└─────────────────────────────────────────────────────────────┘
```

## Memory Management

### Virtual Memory

NixCore uses 4-level paging for virtual memory management:

- **PML4** (Page Map Level 4) - Top level
- **PDP** (Page Directory Pointer) - Second level
- **PD** (Page Directory) - Third level
- **PT** (Page Table) - Bottom level

Each page is 4KB in size. The system supports:
- Demand paging
- Copy-on-Write (COW) for fork()
- Swapping to disk
- Memory-mapped files

### Physical Memory

Physical memory is managed using a bitmap allocator:
- Each bit represents one 4KB page
- Fast allocation/deallocation
- Tracks free and used pages
- Supports memory zones

### Heap Management

The kernel and userspace use separate heap allocators:
- **Kernel heap**: Simple bump allocator with coalescing
- **User heap**: malloc/free with block splitting and merging

## Process Management

### Scheduler

5-priority preemptive scheduler:
- **Priority 0**: Real-time (highest)
- **Priority 1**: High
- **Priority 2**: Normal
- **Priority 3**: Low
- **Priority 4**: Idle (lowest)

Round-robin scheduling within each priority level with 10ms time slices.

### Context Switching

Full CPU state save/restore:
- General purpose registers (RAX, RBX, RCX, RDX, RSI, RDI, RBP, RSP)
- Segment registers (CS, DS, ES, FS, GS, SS)
- Control registers (CR0, CR2, CR3, CR4)
- FPU state (FXSAVE/FXRSTOR)
- Instruction pointer (RIP)
- Flags register (RFLAGS)

### Process Creation

```c
process_t *process_create(const char *name) {
    // Allocate process structure
    // Create address space (CR3)
    // Set up kernel/user stacks
    // Initialize file descriptors
    // Add to process list
}
```

## Filesystem Layer

### Virtual File System (VFS)

Unified interface for all filesystems:

```c
struct filesystem_operations {
    int (*mount)(filesystem_t *fs, const char *device);
    inode_t *(*get_root)(filesystem_t *fs);
};

struct inode_operations {
    inode_t *(*lookup)(inode_t *dir, const char *name);
};

struct file_operations {
    ssize_t (*read)(inode_t *inode, void *buf, size_t count, uint64_t offset);
    ssize_t (*write)(inode_t *inode, const void *buf, size_t count, uint64_t offset);
};
```

### Supported Filesystems

**FAT32**:
- Boot sector parsing
- FAT table caching
- Cluster chain following
- Directory entry lookup
- Long filename support (planned)

**ext2**:
- Superblock and group descriptors
- Inode table management
- Direct/indirect/double/triple indirect blocks
- Directory entry parsing
- Symbolic links (planned)

**devfs**: Device files (/dev/null, /dev/zero, /dev/random, /dev/sda*)
**procfs**: Process information (/proc/cpuinfo, /proc/meminfo, etc.)
**sysfs**: System information and control

## Network Stack

### Layer Architecture

```
Application Layer    │ HTTP, DNS, NTP
Transport Layer      │ TCP, UDP
Network Layer        │ IP, ICMP
Link Layer          │ Ethernet, ARP
Physical Layer      │ E1000 Driver
```

### Socket API

POSIX-compatible socket interface:

```c
int socket(int domain, int type, int protocol);
int bind(int sockfd, const sockaddr_t *addr, uint32_t addrlen);
int listen(int sockfd, int backlog);
int connect(int sockfd, const sockaddr_t *addr, uint32_t addrlen);
ssize_t send(int sockfd, const void *buf, size_t len, int flags);
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
```

### TCP State Machine

```
CLOSED → LISTEN → SYN_RECEIVED → ESTABLISHED → FIN_WAIT_1 → 
FIN_WAIT_2 → TIME_WAIT → CLOSED

CLOSED → SYN_SENT → ESTABLISHED → FIN_WAIT_1 → FIN_WAIT_2 → 
TIME_WAIT → CLOSED
```

## Device Drivers

### AHCI (SATA)

- PCI bus scanning
- Port initialization
- Command list and FIS setup
- DMA transfers
- READ/WRITE DMA EXT commands

### NVMe (PCIe SSD)

- Admin queue setup
- I/O queue pairs
- Submission/Completion queues
- READ/WRITE commands
- Interrupt handling

### E1000 (Intel Gigabit Ethernet)

- MMIO register access
- RX/TX descriptor rings
- Packet transmission
- Packet reception
- MAC address reading from EEPROM

### USB Stack

**XHCI (USB 3.0)**:
- Command ring
- Event ring
- Transfer ring
- Slot management

**EHCI (USB 2.0)**:
- Async schedule
- Periodic schedule
- Queue heads
- Transfer descriptors

**UHCI (USB 1.1)**:
- Frame list
- Queue heads
- Transfer descriptors

## GUI System

### Compositor

Window compositing with:
- Alpha blending
- Drop shadows
- Z-order management
- Dirty rectangle optimization

### Window Manager

Features:
- Window creation/destruction
- Focus management
- Event routing
- Drag and drop (planned)

### Rendering Pipeline

```
Application draws to framebuffer
         ↓
Compositor blends windows
         ↓
Apply transparency/shadows
         ↓
Output to screen buffer
         ↓
GOP/VESA display
```

## Boot Process

1. **UEFI Firmware** - Loads bootloader
2. **Bootloader** - Sets up GOP, loads kernel
3. **Kernel Entry** - Initializes GDT, IDT, paging
4. **Memory Init** - PMM and VMM setup
5. **Driver Init** - AHCI, NVMe, Network, USB
6. **VFS Init** - Mount root filesystem
7. **Scheduler Start** - Create init process
8. **Init Process** - Mount other filesystems, start services
9. **Desktop** - Launch GUI and shell

## Performance Considerations

- **Zero-copy networking**: Direct DMA to user buffers
- **Page cache**: Cache frequently accessed disk blocks
- **Lazy allocation**: Allocate memory on first access
- **COW fork**: Share pages until write
- **Interrupt coalescing**: Batch interrupt handling

## Security Features

- **ASLR**: Address Space Layout Randomization
- **NX bit**: No-execute pages
- **User/Kernel separation**: Ring 0/Ring 3
- **Capability-based security** (planned)
- **Sandboxing** (planned)

## Future Enhancements

- SMP (multi-core) support
- ACPI power management
- PCI Express hotplug
- Bluetooth support
- Audio subsystem
- GPU acceleration
- Journaling filesystem
- Encrypted filesystems
