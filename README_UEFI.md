# NixCore UEFI - Modern OS

## Возможности

- ✅ **UEFI Boot** - современная загрузка вместо Legacy BIOS
- ✅ **1280x720** - нативное разрешение везде (GUI, boot, console)
- ✅ **AHCI Driver** - поддержка SATA дисков
- ✅ **NVMe Driver** - поддержка современных SSD
- ✅ **TCP/IP Stack** - полноценная сеть (Ethernet, IP, TCP, UDP, ARP)
- ✅ **GOP Graphics** - UEFI Graphics Output Protocol

## Требования

### Linux (рекомендуется)
```bash
sudo apt install gcc binutils gnu-efi mtools dosfstools qemu-system-x86
```

### Windows (MSYS2)
```bash
pacman -S mingw-w64-x86_64-gcc gnu-efi mtools dosfstools qemu
```

## Сборка

```bash
make -f Makefile.uefi
```

Это создаст:
- `boot/BOOTX64.EFI` - UEFI bootloader
- `kernel/KERNEL.ELF` - Kernel в формате ELF64
- `nixcore.img` - FAT32 образ диска с UEFI структурой

## Запуск

### QEMU (с OVMF UEFI firmware)
```bash
make -f Makefile.uefi run
```

Или вручную:
```bash
qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd \
                   -drive format=raw,file=nixcore.img \
                   -m 512M \
                   -net nic -net user
```

### VirtualBox
1. Создай новую VM (Type: Other, Version: Other/Unknown 64-bit)
2. Settings → System → Enable EFI
3. Settings → Display → Video Memory: 128MB
4. Подключи `nixcore.img` как жесткий диск
5. Запусти

### Реальное железо
1. Запиши образ на USB:
   ```bash
   sudo dd if=nixcore.img of=/dev/sdX bs=4M status=progress
   ```
2. Загрузись с USB в UEFI режиме

## Структура проекта

```
nix-core-alpha101/
├── boot/
│   ├── uefi_boot.c          # UEFI bootloader
│   └── BOOTX64.EFI          # Скомпилированный bootloader
├── kernel/
│   ├── kernel_uefi.c        # Главный kernel
│   └── KERNEL.ELF           # Скомпилированный kernel
├── drivers/
│   ├── ahci.c/h             # AHCI (SATA) драйвер
│   ├── nvme.c/h             # NVMe драйвер
│   └── network.c/h          # TCP/IP стек
├── Makefile.uefi            # UEFI Makefile
├── kernel_uefi.ld           # Linker script
└── nixcore.img              # Готовый образ диска
```

## Архитектура

### Boot Process
1. **UEFI Firmware** загружает `BOOTX64.EFI`
2. **Bootloader** устанавливает GOP 1280x720
3. **Bootloader** загружает `KERNEL.ELF` в память
4. **Bootloader** передает управление kernel с:
   - GOP framebuffer
   - Memory map
   - UEFI runtime services
5. **Kernel** инициализирует драйверы

### Драйверы

#### AHCI (SATA)
- Поддержка HBA (Host Bus Adapter)
- Command List / FIS структуры
- DMA transfers
- Чтение/запись секторов

#### NVMe
- Admin Queue (Create SQ/CQ, Identify)
- I/O Queues
- PRP (Physical Region Pages)
- Submission/Completion Queues

#### Network Stack
- **Ethernet**: Frame send/receive
- **ARP**: IP to MAC resolution
- **IP**: Routing, fragmentation
- **TCP**: Connection management, flow control
- **UDP**: Datagram send/receive

### Графика

Все работает в **1280x720**:
- Boot screen
- Kernel console
- GUI (если добавишь)

Framebuffer: 32-bit BGRA, linear

## Отладка

### QEMU с serial output
```bash
qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd \
                   -drive format=raw,file=nixcore.img \
                   -serial stdio \
                   -m 512M
```

### QEMU monitor
```bash
qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd \
                   -drive format=raw,file=nixcore.img \
                   -monitor stdio \
                   -m 512M
```

Команды:
- `info registers` - регистры CPU
- `info mem` - memory map
- `info pci` - PCI устройства

## Следующие шаги

1. **PCI Enumeration** - автоматическое обнаружение устройств
2. **Filesystem** - FAT32/ext2 для чтения/записи файлов
3. **GUI Framework** - окна, кнопки, меню
4. **Multitasking** - процессы и потоки
5. **Userspace** - ring 3 приложения

## Проблемы?

### Bootloader не загружается
- Проверь что UEFI включен в BIOS/VM
- Проверь структуру FAT32: `/EFI/BOOT/BOOTX64.EFI`

### Черный экран
- Проверь что GOP поддерживает 1280x720
- Добавь fallback на другие разрешения

### Kernel не запускается
- Проверь что KERNEL.ELF валидный ELF64
- Проверь linker script

### Драйверы не работают
- Проверь PCI адреса устройств
- Добавь debug вывод

## Лицензия

MIT

## Автор

CIBERSSH - 2026

---

**Это полноценная UEFI OS с современными драйверами!** 🚀
