# App Development

## Desktop App Model

Desktop apps are registered and launched through `apps/apps.c`.

Each app is an `application_t`:

- `name`
- `type`
- `window`
- `running`
- `init`
- `update`
- `cleanup`
- `data`

## Minimal App Pattern

1. Add an enum entry to `app_type_t` in `include/apps.h`.
2. Add a launch case in `apps_launch(...)`.
3. Implement:
   - `your_app_init(application_t* app)`
   - `your_app_update(application_t* app)`
   - `your_app_cleanup(application_t* app)`
   - `your_app_paint(gui_window_t* window)`
4. Store per-app state in `app->data`.

## Example Skeleton

```c
void demo_init(application_t* app) {
    app->window = gui_create_window("Demo", 120, 120, 400, 260);
    app->window->on_paint = demo_paint;
    app->window->user_data = app;
    app->data = kmalloc(sizeof(demo_data_t));
    gui_show_window(app->window);
}
```

## Recommended Current Targets

- text editor improvements
- file explorer actions
- system monitor views
- local browser-style viewer for docs/files

## Important Constraint

The GUI stack is still custom and lightweight. Apps should avoid:

- large allocations
- deep recursion
- assuming real GPU acceleration

## Adding Desktop Icons

Use `gui_add_desktop_icon(name, x, y, handler)` in `gui/gui.c`.
