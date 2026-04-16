#include <efi.h>
#include <efilib.h>

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    EFI_STATUS Status;
    EFI_LOADED_IMAGE *LoadedImage;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
    EFI_FILE_PROTOCOL *Root;
    EFI_FILE_PROTOCOL *KernelFile;
    VOID *KernelBuffer;
    UINTN KernelSize;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *GOP;

    InitializeLib(ImageHandle, SystemTable);

    Print(L"NixCore UEFI Bootloader\r\n");
    Print(L"Loading kernel...\r\n");

    // Get loaded image protocol
    Status = uefi_call_wrapper(BS->HandleProtocol, 3, ImageHandle,
                               &LoadedImageProtocol, (VOID**)&LoadedImage);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to get LoadedImage protocol\r\n");
        return Status;
    }

    // Get file system protocol
    Status = uefi_call_wrapper(BS->HandleProtocol, 3, LoadedImage->DeviceHandle,
                               &FileSystemProtocol, (VOID**)&FileSystem);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to get FileSystem protocol\r\n");
        return Status;
    }

    // Open root directory
    Status = uefi_call_wrapper(FileSystem->OpenVolume, 2, FileSystem, &Root);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to open root directory\r\n");
        return Status;
    }

    // Open kernel file
    Status = uefi_call_wrapper(Root->Open, 5, Root, &KernelFile,
                               L"\\EFI\\NIXCORE\\KERNEL.ELF",
                               EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to open kernel file\r\n");
        return Status;
    }

    // Get kernel size
    EFI_FILE_INFO *FileInfo;
    UINTN FileInfoSize = sizeof(EFI_FILE_INFO) + 512;
    FileInfo = AllocatePool(FileInfoSize);
    Status = uefi_call_wrapper(KernelFile->GetInfo, 4, KernelFile,
                               &gEfiFileInfoGuid, &FileInfoSize, FileInfo);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to get kernel file info\r\n");
        return Status;
    }

    KernelSize = FileInfo->FileSize;
    FreePool(FileInfo);

    // Allocate memory for kernel
    Status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData,
                               KernelSize, &KernelBuffer);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to allocate memory for kernel\r\n");
        return Status;
    }

    // Read kernel
    Status = uefi_call_wrapper(KernelFile->Read, 3, KernelFile,
                               &KernelSize, KernelBuffer);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to read kernel\r\n");
        return Status;
    }

    uefi_call_wrapper(KernelFile->Close, 1, KernelFile);
    uefi_call_wrapper(Root->Close, 1, Root);

    Print(L"Kernel loaded: %d bytes\r\n", KernelSize);

    // Set up graphics mode 1280x720
    Status = uefi_call_wrapper(BS->LocateProtocol, 3, &GraphicsOutputProtocol,
                               NULL, (VOID**)&GOP);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to locate GOP\r\n");
        return Status;
    }

    // Find 1280x720 mode
    UINT32 Mode1280x720 = 0;
    BOOLEAN Found = FALSE;
    for (UINT32 i = 0; i < GOP->Mode->MaxMode; i++) {
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
        UINTN SizeOfInfo;
        Status = uefi_call_wrapper(GOP->QueryMode, 4, GOP, i, &SizeOfInfo, &Info);
        if (!EFI_ERROR(Status)) {
            if (Info->HorizontalResolution == 1280 && Info->VerticalResolution == 720) {
                Mode1280x720 = i;
                Found = TRUE;
                break;
            }
        }
    }

    if (Found) {
        Status = uefi_call_wrapper(GOP->SetMode, 2, GOP, Mode1280x720);
        if (EFI_ERROR(Status)) {
            Print(L"Failed to set 1280x720 mode\r\n");
        } else {
            Print(L"Graphics mode set to 1280x720\r\n");
        }
    }

    // Get memory map
    UINTN MemoryMapSize = 0;
    EFI_MEMORY_DESCRIPTOR *MemoryMap = NULL;
    UINTN MapKey;
    UINTN DescriptorSize;
    UINT32 DescriptorVersion;

    Status = uefi_call_wrapper(BS->GetMemoryMap, 5, &MemoryMapSize, MemoryMap,
                               &MapKey, &DescriptorSize, &DescriptorVersion);
    MemoryMapSize += 2 * DescriptorSize;

    Status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData,
                               MemoryMapSize, (VOID**)&MemoryMap);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to allocate memory map\r\n");
        return Status;
    }

    Status = uefi_call_wrapper(BS->GetMemoryMap, 5, &MemoryMapSize, MemoryMap,
                               &MapKey, &DescriptorSize, &DescriptorVersion);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to get memory map\r\n");
        return Status;
    }

    // Exit boot services
    Status = uefi_call_wrapper(BS->ExitBootServices, 2, ImageHandle, MapKey);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to exit boot services\r\n");
        return Status;
    }

    // Jump to kernel
    typedef void (*KernelEntry)(EFI_GRAPHICS_OUTPUT_PROTOCOL *GOP,
                                EFI_MEMORY_DESCRIPTOR *MemoryMap,
                                UINTN MemoryMapSize,
                                UINTN DescriptorSize);

    KernelEntry entry = (KernelEntry)KernelBuffer;
    entry(GOP, MemoryMap, MemoryMapSize, DescriptorSize);

    // Should never reach here
    return EFI_SUCCESS;
}
