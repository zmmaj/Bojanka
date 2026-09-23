#include <ui/ui.h>
#include <ui/menu.h>
#include <ui/menubar.h>
#include <ui/menudd.h>
#include <ui/menuentry.h>
#include <ui/msgdialog.h>
#include <ui/filedialog.h>
#include <ui/promptdialog.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include "../include/draw.h"

// Callback functions for menu entries
static void menu_action_new(ui_menu_entry_t *entry, void *arg);
static void menu_action_open(ui_menu_entry_t *entry, void *arg);
static void menu_action_save(ui_menu_entry_t *entry, void *arg);
static void menu_action_save_as(ui_menu_entry_t *entry, void *arg);

static void menu_action_exit(ui_menu_entry_t *entry, void *arg);
static void menu_action_flood_fill(ui_menu_entry_t *entry, void *arg);
static void menu_action_undo(ui_menu_entry_t *entry, void *arg);
static void menu_action_redo(ui_menu_entry_t *entry, void *arg);
static void menu_action_zoom(ui_menu_entry_t *entry, void *arg);
static void menu_action_size_1(ui_menu_entry_t *entry, void *arg);
static void menu_action_size_2(ui_menu_entry_t *entry, void *arg);
static void menu_action_size_3(ui_menu_entry_t *entry, void *arg);
static void menu_action_size_4(ui_menu_entry_t *entry, void *arg);
static void menu_action_size_5(ui_menu_entry_t *entry, void *arg);
static void menu_action_color_red(ui_menu_entry_t *entry, void *arg);
static void menu_action_color_green(ui_menu_entry_t *entry, void *arg);
static void menu_action_color_blue(ui_menu_entry_t *entry, void *arg);
static void menu_action_color_white(ui_menu_entry_t *entry, void *arg);
static void menu_action_color_black(ui_menu_entry_t *entry, void *arg);
static void menu_action_color_yellow(ui_menu_entry_t *entry, void *arg);
static void menu_action_color_orange(ui_menu_entry_t *entry, void *arg);
static void menu_action_color_brown(ui_menu_entry_t *entry, void *arg);
static void menu_action_shape_handwrite(ui_menu_entry_t *entry, void *arg);
static void menu_action_shape_line(ui_menu_entry_t *entry, void *arg);
static void menu_action_shape_circle(ui_menu_entry_t *entry, void *arg);
static void menu_action_shape_rectangle(ui_menu_entry_t *entry, void *arg);
static void menu_action_shape_circle_filled(ui_menu_entry_t *entry, void *arg);
static void menu_action_shape_rectangle_filled(ui_menu_entry_t *entry, void *arg);
static void menu_action_help(ui_menu_entry_t *entry, void *arg);



    
/* Callback za file dialog */
static void file_dialog_bok(ui_file_dialog_t *dialog, void *arg, const char *fname);
static void file_dialog_bcancel(ui_file_dialog_t *dialog, void *arg);
static void file_dialog_close(ui_file_dialog_t *dialog, void *arg);

static ui_file_dialog_cb_t file_dialog_cb = {
    .bok = file_dialog_bok,
    .bcancel = file_dialog_bcancel,
    .close = file_dialog_close
};

//kreacija fajl dijaloga
static void file_dialog_bok(ui_file_dialog_t *dialog, void *arg, const char *fname)
{
    paint_t *paint = (paint_t *)arg;

    printf("CALLBACK: pozvan\n");
    printf("CALLBACK: fname=%p\n", (void*)fname);
    if (fname) {
        printf("CALLBACK: fname='%s', len=%zu\n", fname, str_length(fname));
    }

    ui_file_dialog_destroy(dialog);

        /* 1) Obriši undo/redo stack */
        undo_stack_clear();

    /* Obriši postojeću sliku */
    clear_canvas();

    /* Učitaj BOJ */
    gfx_bitmap_alloc_t alloc;
    gfx_bitmap_get_alloc(paint->bitmap, &alloc);

    if (load_boj(fname, paint->bitmap, alloc.pitch)) {
      //  gfx_bitmap_render(paint->bitmap, &paint->bparams.rect, NULL);
        gfx_update(paint->gc);
        push_undo();
        printf("Ucitano: %s\n", fname);
        update_status_bar1(paint, fname);
    } else {
        printf("Greska pri ucitavanju: %s\n", fname);
        update_status_bar1(paint, "Greska pri ucitavanju");
    }
}

static void file_dialog_bcancel(ui_file_dialog_t *dialog, void *arg)
{
    (void)arg;
    ui_file_dialog_destroy(dialog);
    printf("Otvori: otkazano\n");
}

static void file_dialog_close(ui_file_dialog_t *dialog, void *arg)
{
    (void)arg;
    ui_file_dialog_destroy(dialog);
    printf("Otvori: zatvoreno\n");
}


/* Forward deklaracije */
static void save_prompt_bok(ui_prompt_dialog_t *dialog, void *arg, const char *text);
static void save_prompt_bcancel(ui_prompt_dialog_t *dialog, void *arg);
static void save_prompt_close(ui_prompt_dialog_t *dialog, void *arg);

static ui_prompt_dialog_cb_t save_prompt_cb = {
    .bok = save_prompt_bok,
    .bcancel = save_prompt_bcancel,
    .close = save_prompt_close
};

void menu_action_save_as(ui_menu_entry_t *entry, void *arg)
{
    paint_t *paint = (paint_t *)arg;
    ui_prompt_dialog_params_t pdparams;
    ui_prompt_dialog_t *dialog;
    errno_t rc;

    ui_prompt_dialog_params_init(&pdparams);
    pdparams.caption = "Sacuvaj kao";
    pdparams.prompt = "Unesi ime fajla:";

    rc = ui_prompt_dialog_create(paint->ui, &pdparams, &dialog);
    if (rc != EOK) {
        printf("Greska pri kreiranju prompt dijaloga\n");
        return;
    }

    ui_prompt_dialog_set_cb(dialog, &save_prompt_cb, paint);
}

static void save_prompt_bok(ui_prompt_dialog_t *dialog, void *arg, const char *text)
{
    paint_t *paint = (paint_t *)arg;

    /* Kopiraj ime PRE destroy-a */
    char *fname = str_dup(text);
    if (!fname) return;

    ui_prompt_dialog_destroy(dialog);

    /* Dodaj .boj ako nema */
    size_t len = str_length(fname);
    if (len < 4 || str_cmp(fname + len - 4, ".boj") != 0) {
        char buf[256];
        snprintf(buf, sizeof(buf), "%s.boj", fname);
        free(fname);
        fname = str_dup(buf);
    }

    printf("Sacuvaj kao: '%s'\n", fname);

    gfx_bitmap_alloc_t alloc;
    gfx_bitmap_get_alloc(paint->bitmap, &alloc);

    if (save_boj(fname, paint->bitmap, paint->width, paint->height, alloc.pitch)) {
        printf("Sacuvano: %s\n", fname);
        update_status_bar1(paint, fname);
    } else {
        printf("Greska pri cuvanju: %s\n", fname);
        update_status_bar1(paint, "Greska pri cuvanju");
    }

    free(fname);
}

static void save_prompt_bcancel(ui_prompt_dialog_t *dialog, void *arg)
{
    (void)arg;
    ui_prompt_dialog_destroy(dialog);
}

static void save_prompt_close(ui_prompt_dialog_t *dialog, void *arg)
{
    (void)arg;
    ui_prompt_dialog_destroy(dialog);
}

// Function to create menu bar
errno_t create_menu_bar(paint_t *paint) {
    errno_t rc;
    ui_menu_t *file_menu;
    ui_menu_t *edit_menu;
    ui_menu_t *size_menu;
    ui_menu_t *color_menu;   // For Color menu
    ui_menu_t *shape_menu;   // For Shape menu
    ui_menu_t *help_menu;    // For Help menu

    ui_menu_entry_t *file_new_entry = NULL;
    ui_menu_entry_t *file_open_entry = NULL;
    ui_menu_entry_t *file_save_entry = NULL;
    ui_menu_entry_t *file_save_as_entry = NULL;
    ui_menu_entry_t *file_exit_entry = NULL;

    ui_menu_entry_t *action_flood_fill_entry = NULL;
    ui_menu_entry_t *action_undo_entry = NULL;
    ui_menu_entry_t *action_redo_entry = NULL;
    ui_menu_entry_t *action_zoom_entry = NULL;

    ui_menu_entry_t *size_1_entry = NULL;
    ui_menu_entry_t *size_2_entry = NULL;
    ui_menu_entry_t *size_3_entry = NULL;
    ui_menu_entry_t *size_4_entry = NULL;
    ui_menu_entry_t *size_5_entry = NULL;

    ui_menu_entry_t *color_red_entry = NULL;    // Red color entry
    ui_menu_entry_t *color_green_entry = NULL;    // Green color entry
    ui_menu_entry_t *color_blue_entry = NULL;     // Blue color entry
    ui_menu_entry_t *color_white_entry = NULL;    // White color entry
    ui_menu_entry_t *color_black_entry = NULL;    // Black color entry
    ui_menu_entry_t *color_yellow_entry = NULL;   // Yellow color entry
    ui_menu_entry_t *color_orange_entry = NULL;   // Orange color entry
    ui_menu_entry_t *color_brown_entry = NULL;    // Brown color entry

    ui_menu_entry_t *shape_handwrite_entry = NULL;
    ui_menu_entry_t *shape_line_entry = NULL;
    ui_menu_entry_t *shape_circle_entry = NULL;
    ui_menu_entry_t *shape_circle_filled_entry = NULL;
    ui_menu_entry_t *shape_rectangle_entry = NULL;
    ui_menu_entry_t *shape_rectangle_filled_entry = NULL;

    ui_menu_entry_t *help_entry = NULL;  // Help stavke menija

    gfx_rect_t rectm;
    rectm.p0.x = 5;
    rectm.p0.y = 25;
    rectm.p1.x = paint->rect.p1.x - 5;
    rectm.p1.y = 42;



    /* Create the menu bar */
    rc = ui_menu_bar_create(paint->ui, paint->window, &paint->menubar);
    if (rc != EOK) {
        printf("Greska pri kreiranju menu bar.\n");
        return rc;
    }
    ui_menu_bar_set_rect(paint->menubar, &rectm);

    /* Create Document menu */
    rc = ui_menu_dd_create(paint->menubar, "~D~okument", NULL, &file_menu);
    if (rc != EOK) {
        printf("Greska pri kreiranju Document menu.\n");
        return rc;
    }

    rc = ui_menu_entry_create(file_menu, "Novo", "Ctrl+N", &file_new_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju NEW stavke menija.\n");
        return rc;
    }
    ui_menu_entry_set_cb(file_new_entry, menu_action_new, paint);

    rc = ui_menu_entry_create(file_menu, "Otvori", "Ctrl+O", &file_open_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Open stavke menija.\n");
        return rc;
    }
    ui_menu_entry_set_cb(file_open_entry, menu_action_open, paint);

    rc = ui_menu_entry_create(file_menu, "Sacuvaj", "Ctrl+S", &file_save_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Save stavke menija.\n");
        return rc;
    }
    ui_menu_entry_set_cb(file_save_entry, menu_action_save, paint);


    rc = ui_menu_entry_create(file_menu, "Sacuvaj kao", "Ctrl+Shift+S",
        &file_save_as_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Save As entry\n");
        return rc;
    }
    ui_menu_entry_set_cb(file_save_as_entry, menu_action_save_as, paint);


    rc = ui_menu_entry_create(file_menu, "Izlaz", "Ctrl+Q", &file_exit_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Exit stavke menija.\n");
        return rc;
    }
    ui_menu_entry_set_cb(file_exit_entry, menu_action_exit, NULL);

    /* Create Actions menu */
    rc = ui_menu_dd_create(paint->menubar, "~A~kcije", NULL, &edit_menu);
    if (rc != EOK) {
        printf("Greska pri kreiranju Actions menu.\n");
        return rc;
    }

    rc = ui_menu_entry_create(edit_menu, "Popuni", "Ctrl+F", &action_flood_fill_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Flood fill stavke menija.\n");
        return rc;
    }
    ui_menu_entry_set_cb(action_flood_fill_entry, menu_action_flood_fill, paint);


    rc = ui_menu_entry_create(edit_menu, "Undo", "Ctrl+Z", &action_undo_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Undo stavke menija.\n");
        return rc;
    }
    ui_menu_entry_set_cb(action_undo_entry, menu_action_undo, paint);

    rc = ui_menu_entry_create(edit_menu, "Redo", "Ctrl+Y", &action_redo_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Redo stavke menija.\n");
        return rc;
    }
    ui_menu_entry_set_cb(action_redo_entry, menu_action_redo, paint);

    rc = ui_menu_entry_create(edit_menu, "Zoom", "Ctrl+Z", &action_zoom_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Zoom stavke menija.\n");
        return rc;
    }
    ui_menu_entry_set_cb(action_zoom_entry, menu_action_zoom, paint);

    /* Create Size menu */
    rc = ui_menu_dd_create(paint->menubar, "~V~elicina", NULL, &size_menu);
    if (rc != EOK) {
        printf("Greska pri kreiranju Size menu.\n");
        return rc;
    }

    rc = ui_menu_entry_create(size_menu, "1", "Ctrl+1", &size_1_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Size 1 stavke menija.\n");
        return rc;
    }
    ui_menu_entry_set_cb(size_1_entry, menu_action_size_1, paint);

    rc = ui_menu_entry_create(size_menu, "2", "Ctrl+2", &size_2_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Size 2 stavke menija.\n");
        return rc;
    }
    ui_menu_entry_set_cb(size_2_entry, menu_action_size_2, paint);

    rc = ui_menu_entry_create(size_menu, "3", "Ctrl+3", &size_3_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Size 3 stavke menija.\n");
        return rc;
    }
    ui_menu_entry_set_cb(size_3_entry, menu_action_size_3, paint);

    rc = ui_menu_entry_create(size_menu, "4", "Ctrl+4", &size_4_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Size 4 stavke menija.\n");
        return rc;
    }
    ui_menu_entry_set_cb(size_4_entry, menu_action_size_4, paint);

    rc = ui_menu_entry_create(size_menu, "5", "Ctrl+5", &size_5_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Size 5 stavke menija.\n");
        return rc;
    }
    ui_menu_entry_set_cb(size_5_entry, menu_action_size_5, paint);

    /* Create Color menu */
    rc = ui_menu_dd_create(paint->menubar, "~B~oja", NULL, &color_menu);
    if (rc != EOK) {
        printf("Greska pri kreiranju menija Boja.\n");
        return rc;
    }

    rc = ui_menu_entry_create(color_menu, "Crvena", "Ctrl+R", &color_red_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Crvena izbora menija.\n");
        return rc;
    }
    ui_menu_entry_set_cb(color_red_entry, menu_action_color_red, paint);

    rc = ui_menu_entry_create(color_menu, "Zelena", "Ctrl+G", &color_green_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Zelena izbora menija.\n");
        return rc;
    }
    ui_menu_entry_set_cb(color_green_entry, menu_action_color_green, paint);

    rc = ui_menu_entry_create(color_menu, "Plava", "Ctrl+B", &color_blue_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju PLava izbora menija.\n");
        return rc;
    }
    ui_menu_entry_set_cb(color_blue_entry, menu_action_color_blue, paint);

    rc = ui_menu_entry_create(color_menu, "Bela", "Ctrl+W", &color_white_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Bela izbora menija.\n");
        return rc;
    }
    ui_menu_entry_set_cb(color_white_entry, menu_action_color_white, paint);

    rc = ui_menu_entry_create(color_menu, "Crna", "Ctrl+K", &color_black_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Crna izbora menija.\n");
        return rc;
    }
    ui_menu_entry_set_cb(color_black_entry, menu_action_color_black, paint);

    rc = ui_menu_entry_create(color_menu, "Zuta", "Ctrl+Shift+Y", &color_yellow_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Zuta izbora menija.\n");
        return rc;
    }
    ui_menu_entry_set_cb(color_yellow_entry, menu_action_color_yellow, paint);

    rc = ui_menu_entry_create(color_menu, "Narandzasta", "Ctrl+Shift+O", &color_orange_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Narandzasta izbora menija.\n");
        return rc;
    }
    ui_menu_entry_set_cb(color_orange_entry, menu_action_color_orange, paint);

    rc = ui_menu_entry_create(color_menu, "Braon", "Ctrl+N", &color_brown_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Braon izbora menija.\n");
        return rc;
    }
    ui_menu_entry_set_cb(color_brown_entry, menu_action_color_brown, paint);

    /* Create Shape menu */
    rc = ui_menu_dd_create(paint->menubar, "~O~blik", NULL, &shape_menu);
    if (rc != EOK) {
        printf("Greska pri kreiranju Shape menu.\n");
        return rc;
    }

    rc = ui_menu_entry_create(shape_menu, "Slobodno", "Ctrl+H", &shape_handwrite_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Handwrite shape stavke menija.\n");
        return rc;
    }
    ui_menu_entry_set_cb(shape_handwrite_entry, menu_action_shape_handwrite, paint);

    rc = ui_menu_entry_create(shape_menu, "Linija", "Ctrl+L", &shape_line_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Line shape stavke menija.\n");
        return rc;
    }
    ui_menu_entry_set_cb(shape_line_entry, menu_action_shape_line, paint);

    rc = ui_menu_entry_create(shape_menu, "Krug", "Ctrl+C", &shape_circle_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Circle shape stavke menija.\n");
        return rc;
    }
    ui_menu_entry_set_cb(shape_circle_entry, menu_action_shape_circle, paint);

    rc = ui_menu_entry_create(shape_menu, "Pun Krug", "Ctrl+D", &shape_circle_filled_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Filled Circle shape stavke menija.\n");
        return rc;
    }
    ui_menu_entry_set_cb(shape_circle_filled_entry, menu_action_shape_circle_filled, paint);


    rc = ui_menu_entry_create(shape_menu, "Pun Cetvorougao", "Ctrl+F", &shape_rectangle_filled_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju FILLED Rectangle shape stavke menija.\n");
        return rc;
    }
    ui_menu_entry_set_cb(shape_rectangle_filled_entry, menu_action_shape_rectangle_filled, paint);

    rc = ui_menu_entry_create(shape_menu, "Cetvorougao", "Ctrl+R", &shape_rectangle_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Rectangle shape stavke menija.\n");
        return rc;
    }
    ui_menu_entry_set_cb(shape_rectangle_entry, menu_action_shape_rectangle, paint);


    /* Create Help menu */
    rc = ui_menu_dd_create(paint->menubar, "~P~omoc", NULL, &help_menu);
    if (rc != EOK) {
        printf("Greska pri kreiranju Help menu.\n");
        return rc;
    }
printf("Pre Help stavke menija.\n");
    rc = ui_menu_entry_create(help_menu, "Pomoc", "F1", &help_entry);
    if (rc != EOK) {
       printf("Greska pri kreiranju Help stavke menija.\n");
        return rc;
    }

    ui_menu_entry_set_cb(help_entry, menu_action_help, paint);


    /* Paint the menu bar */
    ui_menu_bar_paint(paint->menubar);
    return EOK;
}

// Action callbacks for menu entries
void menu_action_new(ui_menu_entry_t *entry, void *arg) {
    undo_stack_clear();
    clear_canvas();
    push_undo();
}

// Action callbacks for menu entries
void menu_action_open(ui_menu_entry_t *entry, void *arg)
{
    paint_t *paint = (paint_t *)arg;
    ui_file_dialog_params_t fdparams;
    errno_t rc;

    ui_file_dialog_params_init(&fdparams);
    fdparams.caption = "Otvori BOJ sliku";

    rc = ui_file_dialog_create(paint->ui, &fdparams, &paint->dialog);
    if (rc != EOK) {
        printf("Greska pri kreiranju file dijaloga.\n");
        return;
    }

    ui_file_dialog_set_cb(paint->dialog, &file_dialog_cb, paint);
}

void menu_action_save(ui_menu_entry_t *entry, void *arg)
{
    gfx_bitmap_alloc_t alloc;
    gfx_bitmap_get_alloc(paint.bitmap, &alloc);

    if (save_boj("output.boj", paint.bitmap,
                 paint.width, paint.height, alloc.pitch)) {
        printf("Sacuvano: output.boj\n");
        update_status_bar1(&paint, "Sacuvano: output.boj");
    } else {
        printf("Greska pri cuvanju\n");
    }
}


void menu_action_exit(ui_menu_entry_t *entry, void *arg) {
    printf("Exit action triggered\n");
    exit(0); // Exit the application
}

void menu_action_flood_fill(ui_menu_entry_t *entry, void *arg) {
    printf("flood_fill action triggered\n");
     update_status_bar1(&paint, "Akcija: Popuni Bojom");
     paint.draw_mode = DRAW_MODE_FLOOD_FILL;
}

void menu_action_undo(ui_menu_entry_t *entry, void *arg) {
    printf("Undo action triggered\n");
    update_status_bar1(&paint, "Akcija: UNDO");
   undo();
    printf("menu action UNDO finished.\n");
}

void menu_action_redo(ui_menu_entry_t *entry, void *arg) {
     printf("Redo action triggered\n");
   update_status_bar1(&paint, "Akcija: REDO");
     redo();
    printf("menu action REDO finished.\n");
}

void menu_action_zoom(ui_menu_entry_t *entry, void *arg) {
    update_status_bar1(&paint, "Akcija: UVECAJ");
    printf("Zoom action triggered\n");
}

// Size callbacks for menu entries
void menu_action_size_1(ui_menu_entry_t *entry, void *arg) {
    printf("Size 1 action triggered\n");
    update_status_bar1(&paint, "Akcija: Cetka 1");
    paint.brush_size =3;
}

void menu_action_size_2(ui_menu_entry_t *entry, void *arg) {
    printf("Size 2 action triggered\n");
    update_status_bar1(&paint, "Akcija: Cetka 2");
    paint.brush_size =4;
}

void menu_action_size_3(ui_menu_entry_t *entry, void *arg) {
    printf("Size 3 action triggered\n");
    update_status_bar1(&paint, "Akcija: Cetka 3");
    paint.brush_size =6;
}

void menu_action_size_4(ui_menu_entry_t *entry, void *arg) {
    printf("Size 4 action triggered\n");
    update_status_bar1(&paint, "Akcija: Cetka 4");
    paint.brush_size =8;
}

void menu_action_size_5(ui_menu_entry_t *entry, void *arg) {
    printf("Size 5 action triggered\n");
    update_status_bar1(&paint, "Akcija: Cetka 5");
    paint.brush_size =10;
}

// Color callbacks for menu entries
void menu_action_color_red(ui_menu_entry_t *entry, void *arg) {
    printf("Red color action triggered\n");
     paint_t *paint = (paint_t *)arg;
    
    // Call set_color with full red and no green/blue
    set_color(paint, 0xFFFF, 0x0000, 0x0000);
}

void menu_action_color_green(ui_menu_entry_t *entry, void *arg) {
    printf("Green color action triggered\n");
    paint_t *paint = (paint_t *)arg;
     set_color(paint, 0x0000, 0xFFFF, 0x0000);
}

void menu_action_color_blue(ui_menu_entry_t *entry, void *arg) {
    printf("Blue color action triggered\n");
    paint_t *paint = (paint_t *)arg;
    set_color(paint, 0x0000, 0x0000, 0xFFFF);
}

void menu_action_color_white(ui_menu_entry_t *entry, void *arg) {
    printf("White color action triggered\n");
    paint_t *paint = (paint_t *)arg;
     set_color(paint, 0xFFFF, 0xFFFF, 0xFFFF);
}

void menu_action_color_black(ui_menu_entry_t *entry, void *arg) {
    printf("Black color action triggered\n");
    paint_t *paint = (paint_t *)arg;
     set_color(paint, 0x0000, 0x0000, 0x0000);
}

void menu_action_color_yellow(ui_menu_entry_t *entry, void *arg) {
    printf("Yellow color action triggered\n");
    paint_t *paint = (paint_t *)arg;
    set_color(paint, 0xffff, 0xffff, 0x0000);
}

void menu_action_color_orange(ui_menu_entry_t *entry, void *arg) {
    printf("Orange color action triggered\n");
    paint_t *paint = (paint_t *)arg;
     set_color(paint, 0xFFFF, 0xA500, 0x0000); // ~RGB(255, 165, 0)
}

void menu_action_color_brown(ui_menu_entry_t *entry, void *arg) {
    printf("Brown color action triggered\n");
    paint_t *paint = (paint_t *)arg;
    set_color(paint, 0x7C00, 0x3F00, 0x1F00); // ~RGB(139, 69, 19)
   // update_status_bar1(paint, "Brown color") ;
}

// Shape callbacks for menu entries
void menu_action_shape_handwrite(ui_menu_entry_t *entry, void *arg) {
    printf("Handwrite shape action triggered\n");
    update_status_bar1(&paint, "Akcija: Slobodno pisanje");
    paint.draw_mode = DRAW_MODE_FREEHAND;
}

void menu_action_shape_line(ui_menu_entry_t *entry, void *arg) {
    printf("Line shape action triggered\n");
    update_status_bar1(&paint, "Akcija: Linija");
    paint.draw_mode = DRAW_MODE_STRAIGHT_LINE;
}

void menu_action_shape_circle(ui_menu_entry_t *entry, void *arg) {
    printf("Circle shape action triggered\n");
    update_status_bar1(&paint, "Akcija: Krug");
    paint.draw_mode = DRAW_MODE_CIRCLE;
}

void menu_action_shape_circle_filled(ui_menu_entry_t *entry, void *arg) {
    printf("Filled Circle shape action triggered\n");
    update_status_bar1(&paint, "Akcija: Pun Krug");
    paint.draw_mode = DRAW_MODE_FILLED_CIRCLE;
}

void menu_action_shape_rectangle(ui_menu_entry_t *entry, void *arg) {
    printf("Rectangle shape action triggered\n");
    update_status_bar1(&paint, "Akcija: Cetvorougao");
    paint.draw_mode = DRAW_MODE_RECTANGLE;
}
void menu_action_shape_rectangle_filled(ui_menu_entry_t *entry, void *arg) {
    printf("Filled Rectangle shape action triggered\n");
    update_status_bar1(&paint, "Akcija: Pun Cetvorougao");
    paint.draw_mode = DRAW_MODE_FILLED_RECTANGLE;
}

/* Callback declarations for the message dialog */
static void msg_dialog_button(ui_msg_dialog_t *dialog, void *arg, unsigned btn);

/* Message dialog callback structure -- make sure this is declared before menu_action_help */
static ui_msg_dialog_cb_t msg_dialog_cb = {
    .button = msg_dialog_button
};
void menu_action_help(ui_menu_entry_t *entry, void *arg)
{    update_status_bar1(&paint, "Akcija: Pomoc");
    paint_t *paint = (paint_t *)arg;
    ui_msg_dialog_params_t mdparams;
    ui_msg_dialog_t *dialog;
    errno_t rc;

    ui_msg_dialog_params_init(&mdparams);
    mdparams.caption = "Pomoc";
    mdparams.text = "version: 0.0.1. *Autor-ZmajSoft *2025";
    rc = ui_msg_dialog_create(paint->ui, &mdparams, &dialog);
    if (rc != EOK) {
        printf("Greska pri kreiranju dijaloga poruke.\n");
        return;
    }
   ui_msg_dialog_set_cb(dialog, &msg_dialog_cb, paint);
}

static void msg_dialog_button(ui_msg_dialog_t *dialog, void *arg, unsigned btn)
{
    ui_msg_dialog_destroy(dialog);
}

