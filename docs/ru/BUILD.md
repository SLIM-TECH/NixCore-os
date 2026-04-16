# Сборка NixCore OS

## Требования

### Windows (MinGW)

1. **Установить MinGW-w64**
   - Скачать с https://mingw-w64.org/
   - Добавить в PATH: `C:\MinGW\bin`

2. **Установить NASM**
   - Скачать с https://www.nasm.us/
   - Добавить в PATH

3. **Установить QEMU**
   - Скачать с https://www.qemu.org/download/#windows
   - Добавить в PATH: `C:\Program Files\qemu`

4. **Установить Make**
   - Включен в MinGW или установить отдельно

### Linux (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install build-essential nasm qemu-system-x86 git
```

### macOS

```bash
brew install nasm qemu gcc
```

## Сборка из исходников

### Клонировать репозиторий

```bash
git clone https://github.com/yourusername/nixcore.git
cd nixcore
```

### Команды сборки

```bash
# Очистить предыдущие сборки
make clean

# Собрать ядро и драйверы
make all

# Создать загрузочный образ
make image

# Запустить в QEMU
make run

# Запустить с отладкой
make debug
```

### Цели сборки

- `make all` - Скомпилировать всё
- `make kernel` - Собрать только ядро
- `make drivers` - Собрать только драйверы
- `make clean` - Удалить артефакты сборки
- `make image` - Создать nixcore.img
- `make run` - Запустить в QEMU
- `make debug` - Запустить с поддержкой GDB

## Результат сборки

После успешной сборки у вас будет:

```
build/
├── boot.bin          # Загрузчик (512 байт)
├── kernel.bin        # Бинарник ядра
├── kernel.pe         # PE-исполняемый файл ядра
├── drivers/          # Скомпилированные драйверы
├── lib/              # Скомпилированные библиотеки
└── apps/             # Скомпилированные приложения

nixcore.img           # Загрузочный образ диска (10МБ)
```

## Решение проблем

### "gcc: command not found"

**Windows**: Добавить MinGW в PATH
```powershell
$env:Path += ";C:\MinGW\bin"
```

**Linux**: Установить gcc
```bash
sudo apt install gcc
```

### "nasm: command not found"

Скачать и установить NASM с https://www.nasm.us/

### "ld: cannot find -lgcc"

Установить gcc multilib:
```bash
sudo apt install gcc-multilib
```

### Сборка не удается с "Permission denied"

**Windows**: Запустить от имени администратора
**Linux**: Проверить права доступа к файлам
```bash
chmod +x build.sh
```

### QEMU не запускается

Проверить установку QEMU:
```bash
qemu-system-x86_64 --version
```

## Расширенные опции сборки

### Пользовательские флаги компилятора

Отредактировать `Makefile` и изменить `CFLAGS`:

```makefile
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin \
         -fno-stack-protector -Wall -Wextra -O2
```

### Отладочная сборка

```bash
make CFLAGS="-g -O0" all
```

### Кросс-компиляция

Для кросс-компиляции с Linux на x86:

```bash
sudo apt install gcc-multilib g++-multilib
make CC=gcc LD=ld all
```

## Создание загрузочной флешки

### Linux

```bash
# Найти USB устройство
lsblk

# Записать образ (замените sdX на ваше устройство)
sudo dd if=nixcore.img of=/dev/sdX bs=4M status=progress
sudo sync
```

### Windows

Использовать **Rufus** или **Win32DiskImager**:

1. Скачать Rufus с https://rufus.ie/
2. Выбрать nixcore.img
3. Выбрать USB диск
4. Нажать "Start"

### macOS

```bash
# Найти USB устройство
diskutil list

# Размонтировать
diskutil unmountDisk /dev/diskX

# Записать образ
sudo dd if=nixcore.img of=/dev/rdiskX bs=4m
```

## Тестирование в виртуальных машинах

### QEMU

```bash
# Базовая загрузка
qemu-system-x86_64 -drive file=nixcore.img,format=raw -m 512M

# С сетью
qemu-system-x86_64 -drive file=nixcore.img,format=raw -m 512M \
    -netdev user,id=net0 -device e1000,netdev=net0

# С USB
qemu-system-x86_64 -drive file=nixcore.img,format=raw -m 512M \
    -usb -device usb-mouse -device usb-kbd

# С логированием
qemu-system-x86_64 -drive file=nixcore.img,format=raw -m 512M \
    -serial file:qemu.log -d int,cpu_reset
```

### VirtualBox

1. Создать новую ВМ (Тип: Other, Версия: Other/Unknown 64-bit)
2. Выделить 512МБ RAM
3. Использовать существующий жесткий диск: nixcore.img
4. Включить EFI (если используется UEFI загрузка)
5. Запустить ВМ

### VMware

1. Создать новую ВМ (Other 64-bit)
2. Выделить 512МБ RAM
3. Использовать существующий диск: nixcore.img
4. Включить UEFI firmware
5. Запустить ВМ

## Непрерывная интеграция

### GitHub Actions

Создать `.github/workflows/build.yml`:

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

## Оптимизация производительности

### Оптимизации компилятора

```makefile
# Оптимизация скорости
CFLAGS += -O3 -march=native

# Оптимизация размера
CFLAGS += -Os -ffunction-sections -fdata-sections
LDFLAGS += -Wl,--gc-sections

# Оптимизация времени компоновки
CFLAGS += -flto
LDFLAGS += -flto
```

### Параллельная сборка

```bash
# Использовать все ядра CPU
make -j$(nproc) all
```

## Отладка

### Отладка GDB

Терминал 1:
```bash
qemu-system-x86_64 -drive file=nixcore.img,format=raw -m 512M -s -S
```

Терминал 2:
```bash
gdb build/kernel.pe
(gdb) target remote localhost:1234
(gdb) break kernel_main
(gdb) continue
```

### Вывод через последовательный порт

```bash
qemu-system-x86_64 -drive file=nixcore.img,format=raw -m 512M \
    -serial stdio
```

### Дамп памяти

```bash
qemu-system-x86_64 -drive file=nixcore.img,format=raw -m 512M \
    -monitor stdio
(qemu) info registers
(qemu) x/10i $rip
(qemu) info mem
```

## Частые проблемы

### Ядро не загружается

- Проверить, что загрузчик ровно 512 байт
- Проверить, что ядро загружено по правильному адресу
- Включить логирование QEMU: `-d int,cpu_reset`

### Ошибки диска

- Проверить формат образа: `file nixcore.img`
- Проверить размер образа: `ls -lh nixcore.img`
- Пересоздать образ: `make clean && make image`

### Сеть не работает

- Включить устройство E1000 в QEMU
- Проверить инициализацию драйвера в логах
- Проверить, что DHCP сервер запущен

### USB устройства не обнаружены

- Включить USB контроллер в ВМ
- Проверить инициализацию USB драйвера
- Попробовать другой USB контроллер (XHCI/EHCI/UHCI)
