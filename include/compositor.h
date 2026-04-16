#ifndef COMPOSITOR_H
#define COMPOSITOR_H

#include "types.h"
#include "gui.h"

#define MAX_LAYERS 16
#define SHADOW_OFFSET 4
#define SHADOW_BLUR 8

typedef enum {
    LAYER_DESKTOP,
    LAYER_WINDOW,
    LAYER_OVERLAY,
    LAYER_CURSOR
} layer_type_t;

typedef struct compositor_layer {
    layer_type_t type;
    gui_rect_t bounds;
    uint32_t* buffer;
    uint8_t alpha;
    bool visible;
    bool dirty;
    struct compositor_layer* next;
} compositor_layer_t;

typedef struct {
    compositor_layer_t* layers[MAX_LAYERS];
    uint32_t* back_buffer;
    uint32_t* front_buffer;
    int layer_count;
    bool vsync_enabled;
    bool effects_enabled;
} compositor_t;

void compositor_init(void);
void compositor_shutdown(void);
void compositor_render(void);
void compositor_present(void);
compositor_layer_t* compositor_create_layer(layer_type_t type, gui_rect_t bounds);
void compositor_destroy_layer(compositor_layer_t* layer);
void compositor_add_layer(compositor_layer_t* layer);
void compositor_remove_layer(compositor_layer_t* layer);
void compositor_move_layer(compositor_layer_t* layer, int z_order);
void compositor_mark_dirty(compositor_layer_t* layer);
void compositor_draw_shadow(gui_rect_t bounds, uint32_t color);
void compositor_apply_blur(uint32_t* buffer, gui_rect_t bounds, int radius);
void compositor_blend_layer(compositor_layer_t* layer);

#endif
