#ifndef DRAW_H
#define DRAW_H

#include <ui/window.h>
#include <ui/wdecor.h>
#include <ui/image.h>
#include <ui/label.h>
#include <ui/fixed.h>
#include <ui/paint.h>
#include <gfx/context.h> // Include graphics context headers
#include <gfx/render.h>
#include <gfx/color.h>
#include <ui/menu.h>
#include <ui/menubar.h>
#include <ui/menudd.h>
#include <ui/menuentry.h>
#include <str.h>
#include <io/pixelmap.h>
#include <io/pixel.h>
#include "../include/draw.h"
#include <gfx/cursor.h>  // Include the necessary header for cursor functions
#include <gfx/coord.h>  
#include <io/pixelmap.h>
#include <memgfx/memgc.h>


#define UI_EVCLAIMED 1
#define STBTT_min(a,b)  ((a) < (b) ? (a) : (b))
#define STBTT_max(a,b)  ((a) < (b) ? (b) : (a))
#define DRAW_MODE_FREEHAND 1
#define DRAW_MODE_CIRCLE    2
#define DRAW_MODE_RECTANGLE 3
#define DRAW_MODE_STRAIGHT_LINE 4
#define DRAW_MODE_FILLED_CIRCLE    5
#define DRAW_MODE_FILLED_RECTANGLE 6
#define DRAW_MODE_FLOOD_FILL 7
typedef unsigned short u16;
#define ALIGN_TO(x, a)  (((x) + ((a)-1)) & ~((a)-1))
 #define STACK_CHUNK_SIZE 10000  // Veličina jednog "komada" stoga
#define TOLERANCE 10  // Tolerancija za poređenje boja


typedef struct {
    ui_t *ui;
    ui_window_t *window;
    ui_wdecor_t *wdecor;
    gfx_rect_t rect;
    gfx_rect_t wrect;
    gfx_rect_t arect;
    gfx_rect_t arect1;
    gfx_rect_t app_rect;
    ui_label_t *status;
    ui_label_t *label_boja;
    ui_wnd_params_t params;
    pixelmap_t pixelmap;
    int x, y, width, height;
    gfx_coord2_t center;
    gfx_coord2_t mouse_pos;
    gfx_coord2_t prev_pos;
    gfx_coord2_t off;
    gfx_rect_t srect;
    gfx_coord2_t offs;
    gfx_color_t *color;
    ui_resource_t *ui_res;
    ui_resource_t *res;
    ui_fixed_t *fixed;
    ui_fixed_t *lfixed;
    gfx_context_t *gc;
    gfx_context_t *gca;
    bool cursor_get_pos;
    bool cursor_move;
    ui_control_t *control;
   ui_menu_bar_t *menubar;
   ui_menu_bar_cb_t *meni;
   ui_menu_t *menu;
   ui_menu_t *edit;
   ui_menu_t *view;
   ui_menu_t *help;
    int draw_mode;
     u16 last_red, last_green, last_blue;
      int has_focus;
      int brush_size;
    gfx_bitmap_t *bitmap;       // Prva bitmapa
    gfx_bitmap_t *bitmap1;       // Prva bitmapa
    gfx_bitmap_params_t bparams;
    gfx_bitmap_params_t bparams1;
     gfx_bitmap_alloc_t pixels;
     gfx_bitmap_params_t previous_params;  // Prethodni parametri bitmape
    bool is_direct_output;   
} paint_t;

extern paint_t paint;


typedef enum {
    SHAPE_RECTANGLE,
    SHAPE_CIRCLE
} shape_type_t;

typedef struct {
    shape_type_t type;
    int x, y;
    int size;      // Used for radius (circles) or brush size (freehand)
    int width;     // Used for rectangles/lines
    int height;    // Used for rectangles
    bool filled;
} draw_action_t;



extern void push_undo(void);
extern void undo(void);
extern void redo(void);

extern bool img_load(gfx_context_t *gc, const char *fname, gfx_bitmap_t **rbitmap, gfx_rect_t *rect);
//kraj novo
extern bool file_exists(const char *path);
extern void delete_file(const char *path);
extern gfx_bitmap_t *copy_bitmap(gfx_bitmap_t *src_bitmap);

extern gfx_bitmap_t *get_bitmap_by_index(int index);
extern gfx_bitmap_flags_t get_params_by_index(int index);
extern void redraw_canvas(void);
extern void clear_canvas(void);
extern errno_t create_menu_bar(paint_t *paint);
extern void update_status_bar(paint_t *paint, const char *text);
extern void update_status_bar1(paint_t *paint, const char *text);
extern int set_color(paint_t *paint, u16 red, u16 green, u16 blue);
extern void paint_cursor_moved(ui_window_t *window, void *arg, int x, int y);

extern errno_t gfx_cursor_get_pos(gfx_context_t *, gfx_coord2_t *);
extern errno_t gfx_cursor_set_pos(gfx_context_t *, gfx_coord2_t *);
extern errno_t gfx_cursor_set_visible(gfx_context_t *, bool);


extern void wnd_close(ui_window_t *, void *);
extern void wnd_kbd_event(ui_window_t *, void *, kbd_event_t *);
extern void wnd_pos_event(ui_window_t *window, void *arg, pos_event_t *event);
extern void cursor_setvis(bool visible);

// Function prototypes
void draw_pixel(gfx_context_t *gc, int x, int y, int color);
void draw_line(gfx_context_t *gc, gfx_coord2_t pos1, gfx_coord2_t pos2);

void draw_rect(gfx_context_t *gc, sysarg_t x, sysarg_t y, sysarg_t width, sysarg_t height, gfx_color_t color, bool filled);
extern errno_t draw_init(pixelmap_t *pixelmap, sysarg_t width, sysarg_t height);
void pixelmap_render(gfx_context_t *gc, pixelmap_t *pixelmap);
extern void save_bmp(const char *filename, gfx_bitmap_t *bitmap);
extern void save_tga(const char *filename, gfx_bitmap_t *bitmap);
extern pixelmap_t *copy_gfx_context_to_pixelmap(gfx_context_t *gc);
extern pixelmap_t *copy_pixelmap(pixelmap_t *src);

extern void save_pixelmap_to_ppm(pixelmap_t *pixelmap, const char *filename);

extern void int_to_string(int value, char *buffer);
extern float roundf(float value);

void draw_empty_rect(gfx_context_t *gc, gfx_coord2_t start, gfx_coord2_t end, int brush_size, gfx_color_t *color);
void draw_filled_rect(gfx_context_t *gc, gfx_coord2_t start, gfx_coord2_t end, int brush_size, gfx_color_t *color);
void draw_empty_circle(gfx_context_t *gc, gfx_coord2_t center, int radius, int brush_size, gfx_color_t *color);
void draw_filled_circle(gfx_context_t *ctx, gfx_coord2_t center, int radius, int brush_size, gfx_color_t *color);
void fill_closed_shape(gfx_context_t *gc, sysarg_t startx, sysarg_t starty, gfx_color_t *fill_color, gfx_color_t *border_color);
void flood_fill(sysarg_t mouse_x, sysarg_t mouse_y);
//helper function for flood fill
pixel_t get_canvas_pixel(gfx_context_t *gc, sysarg_t x, sysarg_t y);
void bitmap_to_pixelmap_copy(void);
void pixelmap_to_bitmap_copy(void);
void init_pixelmap(void);
void flood_fill_recursive(sysarg_t x, sysarg_t y, gfx_color_t old_color, gfx_color_t new_color);
void flood_fill_iterative(sysarg_t x, sysarg_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t new_r, uint8_t new_g, uint8_t new_b);
void flood_fill_dfs(sysarg_t x, sysarg_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t new_r, uint8_t new_g, uint8_t new_b);
void close_log_file(void);
void pixelmap_to_file(const char *file_name,pixelmap_t pixelmap);
#endif // DRAW_H
