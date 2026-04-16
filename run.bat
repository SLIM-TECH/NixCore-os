@echo off
echo ========================================
echo NixCore OS - QEMU Runner
echo ========================================
echo.

if not exist nixcore.img (
    echo [ERROR] nixcore.img not found!
    echo Run 'make image' first
    exit /b 1
)

set QEMU=qemu-system-x86_64
set IMG=nixcore.img
set LOG=qemu_run.log
set MEM=512M

echo Starting NixCore in QEMU...
echo Log file: %LOG%
echo.

%QEMU% -drive file=%IMG%,format=raw -m %MEM% ^
    -serial file:%LOG% ^
    -d int,cpu_reset ^
    -no-reboot -no-shutdown ^
    -netdev user,id=net0 -device e1000,netdev=net0 ^
    -usb -device usb-mouse -device usb-kbd

echo.
echo QEMU exited. Check %LOG% for details.
pause
