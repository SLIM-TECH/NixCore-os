#!/bin/bash

echo "========================================"
echo "NixCore OS - Build and Run Script"
echo "========================================"
echo ""

# Clean previous build
echo "[1/4] Cleaning previous build..."
make clean

# Build kernel
echo "[2/4] Building kernel and drivers..."
make all
if [ $? -ne 0 ]; then
    echo "[ERROR] Build failed!"
    exit 1
fi

# Create image
echo "[3/4] Creating bootable image..."
make image
if [ $? -ne 0 ]; then
    echo "[ERROR] Image creation failed!"
    exit 1
fi

# Run in QEMU
echo "[4/4] Starting QEMU..."
echo "Log file: qemu_run.log"
echo ""

qemu-system-x86_64 \
    -drive file=nixcore.img,format=raw \
    -m 512M \
    -serial file:qemu_run.log \
    -d int,cpu_reset \
    -no-reboot -no-shutdown \
    -netdev user,id=net0 -device e1000,netdev=net0 \
    -usb -device usb-mouse -device usb-kbd

echo ""
echo "QEMU exited. Check qemu_run.log for details."
