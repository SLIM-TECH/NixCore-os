# Kernel And Hardware API

## Core Headers

- `include/kernel.h`
- `include/hardware.h`
- `include/memory.h`
- `include/filesystem.h`
- `include/gui.h`
- `include/apps.h`
- `include/services.h`

## Memory

Header: `include/memory.h`

Functions:

- `memory_init()`
- `kmalloc(size_t size)`
- `kfree(void* ptr)`
- `memory_get_total()`
- `memory_get_used()`
- `memory_get_free()`
- `memory_get_peak()`

Notes:

- heap is a simple free-list allocator
- heap base is configured by `HEAP_START`

## Filesystem

Header: `include/filesystem.h`

Functions:

- `filesystem_init()`
- `create_file(name, parent_dir)`
- `create_directory(name, parent_dir)`
- `write_file(index, data, size)`
- `read_file(index, buffer, size)`
- `delete_file(index)`
- `change_directory(path)`
- `list_directory(dir, buffer, size, long_format)`
- `get_current_path()`
- `filesystem_find_by_path(path)`

Stats:

- `filesystem_get_total_space()`
- `filesystem_get_used_space()`
- `filesystem_get_free_space()`
- `filesystem_get_file_count()`

## Hardware

Header: `include/hardware.h`

Functions:

- `hardware_init()`
- `hardware_get_info()`
- `hardware_get_storage_devices()`
- `hardware_get_storage_count()`
- `hardware_get_selected_disk()`
- `hardware_select_disk(index)`
- `hardware_read_selected_sector(sector, buffer)`
- `hardware_write_selected_sector(sector, buffer)`

Useful fields in `hardware_info_t`:

- `cpu_vendor`
- `cpu_brand`
- `gpu_name`
- `gpu_vendor`
- `video_outputs`
- `detected_memory_kb`
- `usb_controller_count`
- `audio_controller_count`
- `storage_controller_count`
- `integrated_graphics`

## GUI

Header: `include/gui.h`

Key functions:

- `gui_init()`
- `gui_update()`
- `gui_render()`
- `gui_create_window(...)`
- `gui_create_button(...)`
- `gui_create_label(...)`
- `gui_create_textbox(...)`
- `gui_add_desktop_icon(...)`

## Services

Header: `include/services.h`

Runtime helpers:

- `services_init()`
- `services_update()`
- `services_shutdown()`
- `services_get_running_count()`
- `services_describe_running(buffer, size)`

## Applications

Header: `include/apps.h`

Helpers:

- `apps_init()`
- `apps_update()`
- `apps_launch(type)`
- `apps_close(app)`
- `apps_find(name)`
- `apps_get_running_count()`
- `apps_describe_running(buffer, size)`
