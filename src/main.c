/** @addtogroup paint
 * @{
 */
/** @file Paint app
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <ui/ui.h>
#include <ui/window.h>
#include <ui/wdecor.h>
#include "../../../lib/ui/private/wdecor.h"
#include <ui/image.h>
#include <ui/label.h>
#include <ui/fixed.h>
#include <ui/paint.h>
#include <ui/pbutton.h>
#include <gfx/context.h> // Include graphics context headers
#include <gfx/render.h>
#include <gfx/color.h>
#include <ui/menu.h>
#include <ui/menubar.h>
#include <ui/menudd.h>
#include <ui/menuentry.h>
#include <str.h>
#include <ui/resource.h>
#include <io/pixelmap.h>
#include <io/pixel.h>
#include "../include/draw.h"
#include <gfx/cursor.h>  // Include the necessary header for cursor functions
#include <gfx/coord.h>   // For gfx_coord2_t
#include <mem.h>
#include <async.h>       // Include async for timer

#define NAME  "paint"
 paint_t paint;

errno_t rc;


typedef unsigned short u16;

static char buffer[64];
static char buffer1[64];

//Status BAR
void update_status_bar(paint_t *paint, const char *text) {
    ui_label_destroy(paint->status);
    rc = ui_label_create(paint->ui_res, "", &paint->status);
    if (rc != EOK) {
        printf("Greska pri kreiranju STATUS-a.\n");
        return;
    }
    ui_label_set_rect(paint->status, &paint->arect);
     ui_label_set_text(paint->status, text);
     ui_label_set_halign(paint->status, gfx_halign_left);
     ui_label_paint(paint->status);
   
}
void update_status_bar1(paint_t *paint, const char *text) {
    ui_label_destroy(paint->label_boja);

    rc = ui_label_create(paint->ui_res, "", &paint->label_boja);
    if (rc != EOK) {
        printf("Greska pri kreiranju STATUS1-a.\n");
        return;
    }

    ui_label_set_rect(paint->label_boja, &paint->arect1);
    ui_label_set_halign(paint->label_boja, gfx_halign_right);
     ui_label_set_text(paint->label_boja, text);
     gfx_update(paint->gc);
     ui_label_paint(paint->label_boja);
     
}
// set drawing color
 int set_color(paint_t *paint, u16 red, u16 green, u16 blue) {
    int rc;
   paint->last_red = red;
    paint->last_green = green;
    paint->last_blue = blue;


    printf("Setting color: R=%04X, G=%04X, B=%04X\n", red, green, blue);
    if (paint->color) {
        gfx_color_delete(paint->color);
    }

    rc = gfx_color_new_rgb_i16(red, green, blue, &paint->color);
    if (rc != EOK) {
        printf("Greska pri kreiranju color.\n");
        return rc;
    }

    
    // Apply color immediately to ensure persistence
    rc = gfx_set_color(paint->gc, paint->color);
    if (rc != EOK) {
        printf("Error setting color.\n");
        return rc;
    }
     gfx_update(paint->gc);
    // ✅ Update status bar
    snprintf(buffer1, sizeof(buffer1), "Selected Color: R=%04X G=%04X B=%04X", red, green, blue);
    update_status_bar1(paint, buffer1);

    return EOK;
}


/** paint window keyboard event.
 *
 * @param window UI window
 * @param arg Argument (paint_t *)
 * @param event Dogadjaj Tastature
 */
void wnd_kbd_event(ui_window_t *window, void *arg, kbd_event_t *event) {
    
    if (event->type != KEY_PRESS)
        return;
    ui_window_def_kbd(window, event);
}

 void paint_cursor_moved(ui_window_t *window, void *arg, int x, int y) {
    paint_t *paint = (paint_t *)arg;
    gfx_rect_t rect;
    
    // Get the application window boundaries
    ui_window_get_app_rect(window, &rect);

    // Check if cursor is inside the window area
    if (x < rect.p0.x || y < rect.p0.y || x > rect.p1.x || y > rect.p1.y) {
        paint->has_focus = 0;  // Cursor is outside
    } else {
        paint->has_focus = 1;  // Cursor is inside
        // set_color(paint, paint->last_red, paint->last_green, paint->last_blue);
    }
  }


void wnd_pos_event(ui_window_t *window, void *arg, pos_event_t *event) {
    ui_window_def_pos(window, event);

    // Ignore events in the menu bar area
    if (event->vpos < 45) {
        paint.cursor_move = 0;
        return;
    }

    if (!paint.cursor_get_pos) {
        paint.cursor_move = 0;
        paint.cursor_get_pos = 1;
    }

    paint_cursor_moved(window, &paint, event->hpos, event->vpos);
     snprintf(buffer, sizeof(buffer), "X/Y: %ld/%ld", event->hpos, event->vpos);
    update_status_bar(&paint, buffer);
    if (event->type == POS_PRESS) {
        paint.cursor_move = 1;
        paint.mouse_pos.x = event->hpos;
        paint.mouse_pos.y = event->vpos;

        if (paint.draw_mode == DRAW_MODE_FREEHAND) {
            gfx_set_color(paint.gc, paint.color);
            ui_paint_filled_circle(paint.gc, &paint.mouse_pos, paint.brush_size / 3, ui_fcircle_entire);
        }
         
        gfx_update(paint.gc);

    }

    if (event->type == POS_UPDATE) {
        if (paint.cursor_move == 1) {
            gfx_coord2_t new_pos = { event->hpos, event->vpos };

            if (new_pos.x == paint.mouse_pos.x && new_pos.y == paint.mouse_pos.y) {
                return;
            }

            if (paint.draw_mode == DRAW_MODE_FREEHAND) {
                draw_line(paint.gc, paint.mouse_pos, new_pos);
                paint.mouse_pos = new_pos;
            }
           
            gfx_update(paint.gc);
        }
    }

    if (event->type == POS_RELEASE) {
        paint.cursor_move = 0;
        gfx_coord2_t end_pos = { event->hpos, event->vpos };


        if (paint.draw_mode == DRAW_MODE_STRAIGHT_LINE) {
            draw_line(paint.gc, paint.mouse_pos, end_pos);  // Changed function call
        } 
        else if (paint.draw_mode == DRAW_MODE_FLOOD_FILL) {
            flood_fill(paint.mouse_pos.x, paint.mouse_pos.y);

        } 
        else if (paint.draw_mode == DRAW_MODE_RECTANGLE) {
            draw_empty_rect(paint.gc, paint.mouse_pos, end_pos, paint.brush_size, paint.color);
        } 
        else if (paint.draw_mode == DRAW_MODE_FILLED_RECTANGLE) {
            draw_filled_rect(paint.gc, paint.mouse_pos, end_pos, paint.brush_size, paint.color);
        } 
        else if (paint.draw_mode == DRAW_MODE_CIRCLE) {
            int radius = _koren((end_pos.x - paint.mouse_pos.x) * (end_pos.x - paint.mouse_pos.x) +
                (end_pos.y - paint.mouse_pos.y) * (end_pos.y - paint.mouse_pos.y));
            draw_empty_circle(paint.gc, paint.mouse_pos, radius, paint.brush_size, paint.color);
        }
        else if (paint.draw_mode == DRAW_MODE_FILLED_CIRCLE) {
            int radius = _koren((end_pos.x - paint.mouse_pos.x) * (end_pos.x - paint.mouse_pos.x) +
                (end_pos.y - paint.mouse_pos.y) * (end_pos.y - paint.mouse_pos.y));
            draw_filled_circle(paint.gc, paint.mouse_pos, radius, paint.brush_size, paint.color);
        }
            
        gfx_update(paint.gc);
        push_undo();
    }
}

static ui_window_cb_t window_cb = {
   
    .close = wnd_close,
    .kbd = wnd_kbd_event,
    .pos = wnd_pos_event
};

/** Prozor close button was clicked.
 *
 * @param window Prozor
 * @param arg Argument (ttf)
 */
void wnd_close(ui_window_t *window, void *arg) {
    paint_t *paint = (paint_t *) arg;
    ui_quit(paint->ui);
}



// Callback for handling window decoration events
static void wdecor_close(ui_wdecor_t *, void *);
static void wdecor_move(ui_wdecor_t *, void *, gfx_coord2_t *, sysarg_t);

static void wdecor_close(ui_wdecor_t *, void *) {
    ui_quit(paint.ui);
}

static void wdecor_move(ui_wdecor_t *, void *, gfx_coord2_t *, sysarg_t) {
    //paint_t *paint = (paint_t *)arg;
    //void paint(void);
    //printf("Prozor moved to: (%d, %d)\n", pos->x, pos->y);
    // Handle window move (e.g., update window position in your application)
}

static ui_wdecor_cb_t wdecor_cb = {

	.close = wdecor_close,
	.move = wdecor_move,

};


int main(int argc, char* argv[]) {
    printf("\n\t\tSimple paint app\n\n");
    const char *display_spec = UI_ANY_DEFAULT;
   // Initialize the undo/redo system

    /** Paint */
    memset(&paint, 0, sizeof(paint));
      paint.brush_size = 3; 
      paint.draw_mode = DRAW_MODE_FREEHAND; 
    paint.mouse_pos.x = 1;
    paint.mouse_pos.y = 2;
    paint.prev_pos.x = -1;  // Invalid initial position
    paint.prev_pos.y = -1;
    paint.cursor_move = 0;
 /*
    rc = gfx_color_new_rgb_i16(0xffff, 0xffff, 0xffff, &paint.color);
    if (rc != EOK) {
        printf("Nisam postavio boju.\n");
        return 1;
    }
*/
    paint.center.x = 0;
    paint.center.y = 0;

    rc = ui_create(display_spec, &paint.ui);
    if (rc != EOK) {
        printf("Greska pri kreiranju UI na displeju %s.\n", display_spec);
        return 1;
    }
    ui_wnd_params_init(&paint.params);
    paint.params.caption = "BOJANKA";

    if (ui_is_textmode(paint.ui)) {
    paint.params.rect.p0.x = 0;
    paint.params.rect.p0.y = 0;
    paint.params.rect.p1.x = 80;
    paint.params.rect.p1.y = 40;
    } else {
    paint.params.rect.p0.x = 0;
    paint.params.rect.p0.y = 0;
    paint.params.rect.p1.x = 500;
    paint.params.rect.p1.y = 400;
    }
    paint.center.x = 250;
    paint.center.y = 200;


   // paint.params.flags=ui_wndf_system;

       rc = ui_window_create(paint.ui, &paint.params, &paint.window);
    if (rc != EOK) {
        printf("Greska pri kreiranju prozora.\n");
        return 1;
    }
   
    /*
     * Compute window rectangle such that application area corresponds
     * to rect
     */

   ui_wdecor_rect_from_app(paint.ui, paint.params.style, &paint.rect, &paint.wrect);
    paint.off = paint.wrect.p0;
    gfx_rect_rtranslate(&paint.off, &paint.wrect, &paint.params.rect);
    


    ui_window_get_app_rect(paint.window, &paint.app_rect);
    ui_window_set_cb(paint.window, &window_cb, (void *) &paint);
    paint.ui_res = ui_window_get_res(paint.window);

     //ui_wdecor_style_t style = ui_wds_decorated;

    rc = ui_fixed_create(&paint.fixed);
    if (rc != EOK) {
        printf("Greska pri kreiranju fiksnog izgleda.\n");
        return rc;
    }

    // Create the menu bar
    rc = create_menu_bar(&paint);
    if (rc != EOK) {
        return rc;
    }

      rc = ui_fixed_add(paint.fixed, ui_menu_bar_ctl(paint.menubar));
	if (rc != EOK) {
		printf("Greska pri dodavanja kontrola izgledu.\n");
		return rc;
	}
    gfx_rect_t rect;
    
    rc = ui_label_create(paint.ui_res, "", &paint.status);
    if (rc != EOK) {
        printf("Greska pri kreiranju meni bar-a.\n");
        return rc;
    }

    rect.p0.x = paint.app_rect.p0.x;
    rect.p0.y = 376;
    rect.p1.x = paint.app_rect.p0.x+120;
    rect.p1.y = 396;
    paint.arect = rect;

    ui_label_set_rect(paint.status, &rect);
     //ui_label_set_text(paint.status, "X/Y:");
  //  ui_fixed_add(paint.lfixed, ui_label_ctl(paint.status));
    ui_label_set_halign(paint.status, gfx_halign_left);


    rc = ui_label_create(paint.ui_res, "", &paint.label_boja);
    if (rc != EOK) {
        printf("Greska pri kreiranju meni bar-a.\n");
        return rc;
    }
 
    rect.p0.x = paint.app_rect.p0.x+125;
    rect.p0.y = 376;
    rect.p1.x = paint.app_rect.p1.x;
    rect.p1.y = 396;
    paint.arect1 = rect;


    ui_label_set_rect(paint.label_boja, &rect);
   // ui_label_set_text(paint.label_boja, "Dobro Dosli");
    ui_label_set_halign(paint.label_boja, gfx_halign_right);
   // ui_fixed_add(paint.lfixed, ui_label_ctl(paint.label_boja));
   ui_label_paint(paint.label_boja);
 
     ui_label_paint(paint.status);


     ui_window_add(paint.window, ui_fixed_ctl(paint.fixed));
  

    paint.gc = ui_window_get_gc(paint.window); // Enable graphics context
    if (paint.gc == NULL) {
        printf("Error obtaining graphics context.\n");
        return 1;
    }

 rc = ui_wdecor_create(paint.ui_res, "PAINT", ui_wds_decorated, &paint.wdecor);
if (rc != EOK) {
    printf("Greska pri kreiranju window decorations: %d\n", rc);
    return rc;
}

ui_wdecor_set_cb(paint.wdecor, &wdecor_cb, (void *) &paint);

    rc = gfx_cursor_get_pos(paint.gc, &paint.mouse_pos);
    printf("Mouse position: X = %d, Y = %d\n", paint.mouse_pos.x, paint.mouse_pos.y);

    rc = ui_window_paint(paint.window);
    if (rc != EOK) {
        printf("Greska pri bojenju prozora.\n");
        return 1;
    }


    gfx_bitmap_params_init(&paint.bparams);
 paint.bparams.rect.p0.x = paint.app_rect.p0.x;
 paint.bparams.rect.p0.y = paint.app_rect.p0.y;
 paint.bparams.rect.p1.x =  paint.app_rect.p1.x;
 paint.bparams.rect.p1.y = paint.app_rect.p1.y;
 paint.bparams.flags = bmpf_direct_output;
 //paint.bparams.key_color = 0xFF0000;
paint.width= paint.app_rect.p1.x - paint.app_rect.p0.x;
 paint.height= paint.app_rect.p1.y - paint.app_rect.p0.y;

    gfx_bitmap_params_init(&paint.bparams1);
 paint.bparams1.rect.p0.x = paint.app_rect.p0.x;
 paint.bparams1.rect.p0.y = paint.app_rect.p0.y;
 paint.bparams1.rect.p1.x =  paint.app_rect.p1.x;
 paint.bparams1.rect.p1.y = paint.app_rect.p1.y;


    
 // 1. Prvo kreiramo bitmape jer su one osnova za platno
 rc = gfx_bitmap_create(paint.gc, &paint.bparams, NULL, &paint.bitmap);
 if (rc != EOK) {
     printf("Greska pri kreiranju bitmap.\n");
     return rc;  
 }
 
 rc = gfx_bitmap_create(paint.gc, &paint.bparams1, NULL, &paint.bitmap1);
 if (rc != EOK) {
     printf("Greska pri kreiranju bitmap.\n");
     return rc;  
 }
 

 init_pixelmap();

 // 2. Postavljamo defaultne vrednosti aplikacije pre prvog iscrtavanja
 paint.draw_mode = DRAW_MODE_FREEHAND;  // Slobodno crtanje
 paint.brush_size = 1;                  // Debljina 1px

 // 3. Eksplicitno inicijalizujemo crnu boju (0x0000, 0x0000, 0x0000)
 // Ovo će ispravno ažurirati sistem, ubaciti boju u grafički kontekst i osvežiti statusnu traku
 set_color(&paint, 0x0000, 0x0000, 0x0000);

 // 4. Sada bezbedno čistimo platno (clear_canvas interno koristi belu boju za punjenje)
 clear_canvas();

 // 5. Vraćamo aktivnu crnu boju u grafički kontekst jer ju je clear_canvas promenio u belu
 gfx_set_color(paint.gc, paint.color);


 ui_menu_bar_paint(paint.menubar);

 rc = ui_wdecor_paint(paint.wdecor);
 if (rc != EOK) {
     printf("Error repainting window decorations: %d\n", rc);
 }
 
 gfx_update(paint.gc);
 push_undo();
 ui_run(paint.ui);
 
 // Clean up allocated memory
 ui_label_destroy(paint.status);
 ui_label_destroy(paint.label_boja);
 gfx_bitmap_destroy(paint.bitmap);
 ui_window_destroy(paint.window);
 ui_destroy(paint.ui);
 return 0;
}


/** @}
 */
