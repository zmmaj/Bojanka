/** @addtogroup paint
 * @{
 */
/** @file  draw
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <byteorder.h>
#include <align.h>
#include <io/pixelmap.h>
#include <io/pixel.h>
#include <gfx/context.h> 
#include <gfximage/tga.h>
#include <str.h>
#include "gfx/color.h"
#include "../../lib/gfx/private/color.h"
#include <pixconv.h>
#include <gfx/render.h>
#include "../include/draw.h"
#include <gfx/bitmap.h>
#include <types/gfx/bitmap.h>
#include <memgfx/memgc.h>
#include <ui/menubar.h>
#include <ui/menudd.h>

#include "../private/menubar.h"
// Include the full definition of struct mem_gc
#include "../private/memgc.h"

//NOVO
#include <gfximage/tga.h>   
#include <vfs/vfs.h>

#define MAX_STACK_SIZE 10000  

typedef struct {
    int x, y;
} Point;


// Function to draw a line between two points using Bresenham's line algorithm
void draw_line(gfx_context_t *ctx, gfx_coord2_t pos1, gfx_coord2_t pos2) {
    int dx = abs(pos2.x - pos1.x);
    int dy = abs(pos2.y - pos1.y);
    int sx = (pos1.x < pos2.x) ? 1 : -1;
    int sy = (pos1.y < pos2.y) ? 1 : -1;
    int err = dx - dy;
    gfx_set_color(ctx, paint.color);
    while (pos1.x != pos2.x || pos1.y != pos2.y) {
        gfx_coord2_t pixel = { pos1.x, pos1.y };
        gfx_fill_rect(ctx, &(gfx_rect_t){ .p0 = pixel, .p1 = { pixel.x + paint.brush_size, pixel.y + paint.brush_size } });

        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; pos1.x += sx; }
        if (e2 < dx) { err += dx; pos1.y += sy; }
    }
}


void save_bmp(const char *filename, gfx_bitmap_t *bitmap) {
    printf("Saving bitmap to %s\n", filename);

    gfx_bitmap_alloc_t alloc_info;
    errno_t err = gfx_bitmap_get_alloc(paint.bitmap, &alloc_info);
    if (err != EOK) {
        printf("Ne mogu da dobijem info alokacije\n");
        return;
    }

    int width = paint.bparams.rect.p1.x;
    int height = paint.bparams.rect.p1.y;

    uint8_t *pixel_data = (uint8_t *)alloc_info.pixels + alloc_info.off0;


    FILE *f = fopen(filename, "wb");
    if (!f) {
        printf("Error opening file %s\n", filename);
        return;
    }

    printf("Opened file.\n");

    // BMP header (14 bytes) + DIB header (40 bytes) = 54 bytes total
    uint8_t header[54] = {
        0x42, 0x4D,             // "BM"
        0, 0, 0, 0,             // File size (set later)
        0, 0, 0, 0,             // Reserved
        54, 0, 0, 0,            // Offset to pixel data
        40, 0, 0, 0,            // DIB header size
        0, 0, 0, 0,             // Width (set later)
        0, 0, 0, 0,             // Height (set later)
        1, 0,                   // Color planes (always 1)
        24, 0,                  // Bits per pixel (24-bit)
        0, 0, 0, 0,             // Compression (none)
        0, 0, 0, 0,             // Image size (set later)
        0x13, 0x0B, 0, 0,       // Horizontal resolution (2835 pixels per meter)
        0x13, 0x0B, 0, 0,       // Vertical resolution (2835 pixels per meter)
        0, 0, 0, 0,             // Number of colors (0 = all)
        0, 0, 0, 0              // Important colors (0 = all)
    };
// Pomeramo pokazivač na podatke da preskočimo prvih 45 redova
//uint8_t *adjusted_pixel_data = pixel_data + (45 * alloc_info.pitch);
    printf("Width: %d, Height: %d\n", width, height);
   int new_height = height - 45;
    // Set width and height in header
    header[18] = (uint8_t)(width & 0xFF);
    header[19] = (uint8_t)((width >> 8) & 0xFF);
    header[22] = (uint8_t)(new_height & 0xFF);
    header[23] = (uint8_t)((new_height >> 8) & 0xFF);

    // BMP rows must be aligned to 4-byte boundaries
    int row_padding = (4 - (width * 3) % 4) % 4;
    uint32_t file_size = 54 + (width * 3 + row_padding) * height;
    
    // Set file size
    header[2] = (uint8_t)(file_size & 0xFF);
    header[3] = (uint8_t)((file_size >> 8) & 0xFF);
    header[4] = (uint8_t)((file_size >> 16) & 0xFF);
    header[5] = (uint8_t)((file_size >> 24) & 0xFF);

    fwrite(header, sizeof(uint8_t), 54, f);
    printf("Wrote header. Writing pixels...\n");

    // BMP stores pixels from bottom to top
    for (int y = height - 1; y >= 0; --y) {
        uint8_t *row = pixel_data + y * alloc_info.pitch;
        for (int x = 0; x < width; ++x) {
            uint32_t pixel = *(uint32_t *)(row + x * 4); // Use x * 4 to skip alpha

            uint8_t b = (pixel & 0xFF);
            uint8_t g = ((pixel >> 8) & 0xFF);
            uint8_t r = ((pixel >> 16) & 0xFF);

            uint8_t color[] = {b, g, r};
            fwrite(color, sizeof(uint8_t), 3, f);
        }

        // Write row padding
        uint8_t padding[3] = {0, 0, 0};
        fwrite(padding, sizeof(uint8_t), row_padding, f);
    }

    fclose(f);
    printf("BMP image saved as %s\n", filename);
}


void save_tga(const char *filename, gfx_bitmap_t *bitmap)
{
    printf("Saving TGA bitmap...\n");

    gfx_bitmap_alloc_t alloc_info;
    errno_t err = gfx_bitmap_get_alloc(bitmap, &alloc_info);
    if (err != EOK) {
        printf("Error getting bitmap allocation info\n");
        return;
    }

    int width = paint.bparams.rect.p1.x - paint.bparams.rect.p0.x;
    int height = paint.bparams.rect.p1.y - paint.bparams.rect.p0.y;

    printf("save_tga: width=%d height=%d p0=(%d,%d) p1=(%d,%d)\n",
        width, height,
        (int)paint.bparams.rect.p0.x, (int)paint.bparams.rect.p0.y,
        (int)paint.bparams.rect.p1.x, (int)paint.bparams.rect.p1.y);

    uint8_t *pixel_data = (uint8_t *)alloc_info.pixels + alloc_info.off0;

    FILE *f = fopen(filename, "wb");
    if (!f) {
        printf("Error opening file %s, errno: %d\n", filename, errno);
        return;
    }

    /* TGA header */
    uint8_t header[18] = {0};
    header[2] = 2;   /* uncompressed truecolor */
    header[12] = (width & 0xFF);
    header[13] = (width >> 8) & 0xFF;
    header[14] = (height & 0xFF);
    header[15] = (height >> 8) & 0xFF;
    header[16] = 24; /* 24 bpp */

    fwrite(header, sizeof(header), 1, f);

    /* Bottom-up, BGR, sa p0 offset-om */
    for (int y = height - 1; y >= 0; y--) {
        uint8_t *row_start = pixel_data
            + (paint.bparams.rect.p0.y + y) * alloc_info.pitch
            + paint.bparams.rect.p0.x * 4;

        for (int x = 0; x < width; x++) {
            pixel_t pix = *((pixel_t *)(row_start + x * 4));
            uint8_t bgr[3];
            pixel2bgr_888(bgr, pix);
            fwrite(bgr, 1, 3, f);
        }
    }

    fclose(f);
    printf("TGA saved to %s\n", filename);
}

bool save_boj(const char *filename, gfx_bitmap_t *bitmap,
    int width, int height, size_t pitch)
{
    gfx_bitmap_alloc_t alloc;
    errno_t rc = gfx_bitmap_get_alloc(bitmap, &alloc);
    if (rc != EOK) return false;

    uint32_t *pixels = (uint32_t *)((uint8_t *)alloc.pixels + alloc.off0);

    /* Encode u RLE */
    rle_image_t rle;
    rc = rle_encode(pixels, width, height, pitch, &rle);
    if (rc != EOK) return false;

    FILE *f = fopen(filename, "wb");
    if (!f) {
        rle_free(&rle);
        return false;
    }

    /* Header */
    fwrite(&width, sizeof(uint32_t), 1, f);
    fwrite(&height, sizeof(uint32_t), 1, f);
    fwrite(&rle.count, sizeof(size_t), 1, f);

    /* RLE parovi */
    fwrite(rle.data, sizeof(uint32_t), rle.count * 2, f);

    fclose(f);
    rle_free(&rle);
    return true;
}


bool load_boj(const char *filename, gfx_bitmap_t *bitmap, size_t pitch)
{

  //  printf("load_boj: filename='%s'\n", filename);
    FILE *f = fopen(filename, "rb");
    if (!f) return false;

    uint32_t width, height;
    size_t count;

    if (fread(&width, sizeof(uint32_t), 1, f) != 1) goto err;
    if (fread(&height, sizeof(uint32_t), 1, f) != 1) goto err;
    if (fread(&count, sizeof(size_t), 1, f) != 1) goto err;

    /* Alociraj RLE */
    rle_image_t rle;
    rle.data = malloc(count * 2 * sizeof(uint32_t));
    if (!rle.data) goto err;

    rle.count = count;
    rle.width = width;
    rle.height = height;

    if (fread(rle.data, sizeof(uint32_t), count * 2, f) != count * 2) {
        free(rle.data);
        goto err;
    }

    fclose(f);

    /* Decode u bitmapu */
    gfx_bitmap_alloc_t alloc;
    errno_t rc = gfx_bitmap_get_alloc(bitmap, &alloc);
    if (rc != EOK) {
        free(rle.data);
        return false;
    }

    uint32_t *pixels = (uint32_t *)((uint8_t *)alloc.pixels + alloc.off0);

    rc = rle_decode(&rle, pixels, pitch);
    free(rle.data);

    return rc == EOK;

err:
    fclose(f);
    return false;
}

// UTILS
float roundf(float value) {
    return (float)(value < 0.0f ? (int)(value - 0.5f) : (int)(value + 0.5f));
}
void int_to_string(int value, char *buffer) {
    snprintf(buffer, 50, "%d", value);
}


void draw_empty_rect(gfx_context_t *mgc, gfx_coord2_t start, gfx_coord2_t end, int brush_size, gfx_color_t *color) {

    for (int i = 0; i < brush_size; i++) {
        // Top edge
        draw_line(paint.gc, (gfx_coord2_t){start.x, start.y + i}, (gfx_coord2_t){end.x, start.y + i});
        // Bottom edge
        draw_line(paint.gc, (gfx_coord2_t){start.x, end.y - i}, (gfx_coord2_t){end.x, end.y - i});
        // Left edge
        draw_line(paint.gc, (gfx_coord2_t){start.x + i, start.y}, (gfx_coord2_t){start.x + i, end.y});
        // Right edge
        draw_line(paint.gc, (gfx_coord2_t){end.x - i, start.y}, (gfx_coord2_t){end.x - i, end.y});
    }
}

void draw_filled_rect(gfx_context_t *gc, gfx_coord2_t start, gfx_coord2_t end, int brush_size, gfx_color_t *color) {
   // printf("Drawing filled rectangle from (%d, %d) to (%d, %d)\n", start.x, start.y, end.x, end.y);
   
	gfx_rect_t rect;
    rect.p0.x = start.x;
	rect.p0.y = start.y;
	rect.p1.x = end.x;
	rect.p1.y = end.y;
    gfx_set_color(gc,color);
    printf("Rectangle values:\n");
printf("  p0.x: %d\n", rect.p0.x);
printf("  p0.y: %d\n", rect.p0.y);
printf("  p1.x: %d\n", rect.p1.x);
printf("  p1.y: %d\n", rect.p1.y);
    errno_t rc= gfx_fill_rect(gc, &rect);
    if (rc != EOK) {
        printf("Nisam postavio boju.\n");
        return ;
    }
}

void draw_empty_circle(gfx_context_t *ctx, gfx_coord2_t center, int radius, int brush_size, gfx_color_t *color) {
    if (brush_size < 1) brush_size = 1; // Ensure brush size is valid

    for (int r = radius; r < radius + brush_size; r++) {  // Create thickness
        int x = r;
        int y = 0;
        int err = 1 - r;

        while (x >= y) {
            // Instead of a single pixel, draw a small 2-pixel line
            draw_line(ctx, (gfx_coord2_t){center.x + x, center.y + y}, (gfx_coord2_t){center.x + x + 1, center.y + y});
            draw_line(ctx, (gfx_coord2_t){center.x - x, center.y + y}, (gfx_coord2_t){center.x - x - 1, center.y + y});
            draw_line(ctx, (gfx_coord2_t){center.x + x, center.y - y}, (gfx_coord2_t){center.x + x + 1, center.y - y});
            draw_line(ctx, (gfx_coord2_t){center.x - x, center.y - y}, (gfx_coord2_t){center.x - x - 1, center.y - y});

            draw_line(ctx, (gfx_coord2_t){center.x + y, center.y + x}, (gfx_coord2_t){center.x + y + 1, center.y + x});
            draw_line(ctx, (gfx_coord2_t){center.x - y, center.y + x}, (gfx_coord2_t){center.x - y - 1, center.y + x});
            draw_line(ctx, (gfx_coord2_t){center.x + y, center.y - x}, (gfx_coord2_t){center.x + y + 1, center.y - x});
            draw_line(ctx, (gfx_coord2_t){center.x - y, center.y - x}, (gfx_coord2_t){center.x - y - 1, center.y - x});

            y++;
            if (err < 0) {
                err += 2 * y + 1;
            } else {
                x--;
                err += 2 * (y - x) + 1;
            }
        }
    }
}



void draw_filled_circle(gfx_context_t *ctx, gfx_coord2_t center, int radius, int brush_size, gfx_color_t *color) {

   for (int r = radius; r < radius + brush_size; r++) {  // Draw multiple circles for thickness
        int x = r;
        int y = 0;
        int err = 1 - r;

        while (x >= y) {
            // Draw short lines instead of single pixels
            draw_line(ctx, (gfx_coord2_t){center.x + x, center.y + y}, (gfx_coord2_t){center.x - x, center.y + y});
            draw_line(ctx, (gfx_coord2_t){center.x + x, center.y - y}, (gfx_coord2_t){center.x - x, center.y - y});
            draw_line(ctx, (gfx_coord2_t){center.x + y, center.y + x}, (gfx_coord2_t){center.x - y, center.y + x});
            draw_line(ctx, (gfx_coord2_t){center.x + y, center.y - x}, (gfx_coord2_t){center.x - y, center.y - x});

            y++;
            if (err < 0) {
                err += 2 * y + 1;
            } else {
                x--;
                err += 2 * (y - x) + 1;
            }
        }
    }
}


   void fill_closed_shape(gfx_context_t *gc, sysarg_t startx, sysarg_t starty, gfx_color_t *fill_color, gfx_color_t *border_color) {
    return;
}


void flood_fill(sysarg_t x, sysarg_t y) {
    bitmap_to_pixelmap_copy();
    pixel_t pixel = pixelmap_get_pixel(&paint.pixelmap, x, y);

    uint8_t bgr[3];
    pixel2bgr_888(bgr, pixel);

    uint8_t r = bgr[2];
    uint8_t g = bgr[1];
    uint8_t b = bgr[0];

    uint16_t chosen_r, chosen_g, chosen_b;
    gfx_color_get_rgb_i16(paint.color, &chosen_r, &chosen_g, &chosen_b);

    // Convert chosen color to 8-bit RGB
    uint8_t chosen_r8 = (uint8_t)(chosen_r / 257);
    uint8_t chosen_g8 = (uint8_t)(chosen_g / 257);
    uint8_t chosen_b8 = (uint8_t)(chosen_b / 257);

    if (r == chosen_r8 && g == chosen_g8 && b == chosen_b8) {
        printf("POdudarne bopje! Ne treba popunjavanje.\n");
    } else {
        printf("Boje se ne podudaraju. Flood fill treba.\n");
        flood_fill_iterative(x, y, r, g, b, chosen_r8, chosen_g8, chosen_b8);
    }
    gfx_update(paint.gc);
}

void bitmap_to_pixelmap_copy() {
    // Get allocation info for bitmap
    gfx_bitmap_alloc_t alloc;
    gfx_bitmap_get_alloc(paint.bitmap, &alloc);

    // Get dimensions and data for bitmap
    sysarg_t width = paint.width;
    sysarg_t height = paint.height;
    uint8_t *bitmap_data = (uint8_t *)alloc.pixels;

    // Iterate over bitmap
    for (sysarg_t y = 0; y < height; y++) {
        for (sysarg_t x = 0; x < width; x++) {
            // Calculate global coordinates in bitmap
            sysarg_t global_x = x;
            sysarg_t global_y = y;

            // Check if coordinates are within bitmap bounds
            if (global_x < (sysarg_t)paint.width &&
                global_y < (sysarg_t)paint.height) {
                size_t byte_index = (global_y * alloc.pitch) + (global_x * 4); // 4 bytes per pixel (BGR)

                // Get pixel from bitmap
                uint32_t pixel = *(uint32_t *)(bitmap_data + byte_index);

                // Assign pixel value to pixelmap
                paint.pixelmap.data[y * paint.width + x] = pixel;
            }
        }
    }
    printf("Info: Bitmap copied to pixelmap successfully!\n");
}

void flood_fill_iterative(sysarg_t x, sysarg_t y, 
                          uint8_t r, uint8_t g, uint8_t b, 
                          uint8_t new_r, uint8_t new_g, uint8_t new_b) {
    pixelmap_t pixelmap;
    gfx_bitmap_alloc_t alloc;
    errno_t rc;

    // Uzmi alokaciju za bitmapu
    rc = gfx_bitmap_get_alloc(paint.bitmap, &alloc);
    if (rc != EOK) {
        printf("Error: Nisam uspeo da dobijem bitmap allocation.\n");
        return;
    }

    // Inicijalizuj pixelmap
    pixelmap.width = paint.width;
    pixelmap.height = paint.height;
    pixelmap.data = alloc.pixels;

    // Efektivna širina reda u pikselima
    sysarg_t effective_width = alloc.pitch / 4;

    // Alociraj memoriju za red (queue)
    Point *queue = malloc(paint.width * paint.height * sizeof(Point));
    if (!queue) {
        printf("Error: Ne mogu da alociram memory for queue.\n");
        return;
    }

    int queue_start = 0; // Početak reda
    int queue_end = 0;   // Kraj reda

    // Dodaj početni piksel u red
    queue[queue_end].x = x;
    queue[queue_end].y = y;
    queue_end++;

    while (queue_start < queue_end) {
     
        Point p = queue[queue_start++]; // Uzmi piksel iz reda

        // Provera granica koristeći effective_width za horizontalne koordinate
        if (p.x < 0 || p.x >= (int)effective_width || p.y < 0 || p.y >= (int)paint.height)
            continue;

        size_t index = p.y * (alloc.pitch / 4) + p.x;
        uint32_t pixel_value = ((uint32_t *)pixelmap.data)[index];

        uint8_t pixel_r = (pixel_value >> 16) & 0xFF;
        uint8_t pixel_g = (pixel_value >> 8) & 0xFF;
        uint8_t pixel_b = pixel_value & 0xFF;

        // Ako piksel nema originalnu boju, preskoči ga
        if (pixel_r != r || pixel_g != g || pixel_b != b)
            continue;

        // Skeniraj levo
        int left = p.x;
        while (left >= 0 && ((uint32_t *)pixelmap.data)[p.y * (alloc.pitch / 4) + left] == pixel_value) {
            left--;
        }
        left++;

        // Skeniraj desno
        int right = p.x;
        while (right < (int)effective_width && ((uint32_t *)pixelmap.data)[p.y * (alloc.pitch / 4) + right] == pixel_value) {
            right++;
        }

        // Popuni liniju u pixelmap i bitmap
        gfx_set_color(paint.gc, paint.color);
for (int i = left; i < right; i++) {
    size_t idx = p.y * (alloc.pitch / 4) + i;

    // Ažuriraj pixelmap (za praćenje)
    ((uint32_t *)pixelmap.data)[idx] = PIXEL(255, new_r, new_g, new_b);

    // Iscrtaj preko gc
    gfx_rect_t r = { .p0 = { i, p.y }, .p1 = { i + 1, p.y + 1 } };
    gfx_fill_rect(paint.gc, &r);
}

        // Dodaj susedne linije u red
        for (int i = left; i < right; i++) {
            if (p.y > 0) {
                size_t idx = (p.y - 1) * (alloc.pitch / 4) + i;
                if (((uint32_t *)pixelmap.data)[idx] == pixel_value) {
                    queue[queue_end].x = i;
                    queue[queue_end].y = p.y - 1;
                    queue_end++;
                }
            }
            if (p.y < paint.height - 1) {
                size_t idx = (p.y + 1) * (alloc.pitch / 4) + i;
                if (((uint32_t *)pixelmap.data)[idx] == pixel_value) {
                    queue[queue_end].x = i;
                    queue[queue_end].y = p.y + 1;
                    queue_end++;
                }
            }
        }
    }
    printf("FFI: kraj, queue_end=%d\n", queue_end);
    free(queue);

    // Forsiraj ažuriranje cele površine
    gfx_update(paint.gc);
    printf("FFI: zavrseno\n");
}

void pixelmap_to_bitmap_copy() {
    // Get allocation info for bitmap
    gfx_bitmap_alloc_t alloc;
   
    gfx_bitmap_get_alloc(paint.bitmap, &alloc);

    // Get dimensions and data for bitmap
    sysarg_t width = paint.width;
    sysarg_t height = paint.height;
    uint8_t *bitmap_data = (uint8_t *)alloc.pixels;

    // Iterate over pixelmap
    for (sysarg_t y = 0; y < height; y++) {
        for (sysarg_t x = 0; x < width; x++) {
            // Get pixel from pixelmap
            pixel_t *pixel = &paint.pixelmap.data[y * paint.width + x];

            // Calculate global coordinates in bitmap
            sysarg_t global_x = x;
            sysarg_t global_y = y;

            // Check if coordinates are within bitmap bounds
            if (global_x < (sysarg_t)paint.width &&
                global_y < (sysarg_t)paint.height) {
                size_t byte_index = (global_y * alloc.pitch) + (global_x * 4); // 4 bytes per pixel (BGR)

                // Determine the color
                if (*pixel == 255) {
                    bitmap_data[byte_index] = 255; // Blue
                    bitmap_data[byte_index + 1] = 255; // Green
                    bitmap_data[byte_index + 2] = 255; // Red
                } else if (*pixel == 0) {
                    bitmap_data[byte_index] = 0; // Blue
                    bitmap_data[byte_index + 1] = 0; // Green
                    bitmap_data[byte_index + 2] = 0; // Red
                } else {
                    bitmap_data[byte_index] = 255; // Blue
                    bitmap_data[byte_index + 1] = 0; // Green
                    bitmap_data[byte_index + 2] = 0; // Red
                }
                bitmap_data[byte_index + 3] = 255; // Alpha channel
            }
        }
    }

    printf("Info: Pixelmap copied to bitmap1 successfully!\n");
}

void pixelmap_to_file(const char *file_name, pixelmap_t pixelmap) {
    printf("upis u fajl 1.\n");
    FILE *fp = fopen(file_name, "wb");
    if (fp == NULL) {
        printf("Error: Ne mogu da otvorim fajl for writing!\n");
        return;
    }

    for (sysarg_t y = 0; y < pixelmap.height; y++) {
        for (sysarg_t x = 0; x < pixelmap.width; x++) {
            pixel_t pixel = pixelmap.data[y * pixelmap.width + x];

            char color_char;
            if ((pixel & 0xFF) == 0xFF && ((pixel >> 8) & 0xFF) == 0xFF && ((pixel >> 16) & 0xFF) == 0xFF) {
                color_char = 'w'; // bela
            } else if ((pixel & 0xFF) == 0 && ((pixel >> 8) & 0xFF) == 0 && ((pixel >> 16) & 0xFF) == 0xFF) {
                color_char = 'b'; // blue
            } else if ((pixel & 0xFF) == 0 && ((pixel >> 8) & 0xFF) == 0xFF && ((pixel >> 16) & 0xFF) == 0) {
                color_char = 'g'; // green
            } else if ((pixel & 0xFF) == 0xFF && ((pixel >> 8) & 0xFF) == 0 && ((pixel >> 16) & 0xFF) == 0) {
                color_char = 'r'; // red
            } else if ((pixel & 0xFF) == 0 && ((pixel >> 8) & 0xFF) == 0 && ((pixel >> 16) & 0xFF) == 0) {
                color_char = 'c'; // black
            } else {
                color_char = '?'; // all other colors
            }

            fprintf(fp, "%c", color_char);
        }
        fprintf(fp, "\n"); // new line for each row
    }

    fclose(fp);
    printf("Info: Pixelmap copied to file successfully!\n");
}

void init_pixelmap() {
    // Get bitmap dimensions from bparams
    sysarg_t bitmap_width = paint.width;
    sysarg_t bitmap_height = paint.height;

    // Set pixelmap dimensions
    paint.pixelmap.width = paint.width;
    paint.pixelmap.height = paint.height;

    // Allocate memory for pixelmap data
    paint.pixelmap.data = (pixel_t *)malloc(bitmap_width * bitmap_height * sizeof(pixel_t));
    if (paint.pixelmap.data == NULL) {
        printf("Ne mogu da alociram memory for pixelmap!\n");
        // Handle error
    }
}


#define MAX_UNDO 2
#define MAX_REDO 1

// File names for undo/redo backups
const char *undo_files[MAX_UNDO] = {"undo_0.tga", "undo_1.tga"};
const char *redo_files[MAX_REDO] = {"redo_0.tga"};

// Save the current canvas state for undo
void push_undo(void)
{
    undo_stack_t *stack = &paint.undo_stack;

    /* Ako je stack pun, izbaci najstariji */
    if (stack->count >= MAX_UNDO_STEPS) {
        rle_free(&stack->steps[0]);
        for (int i = 1; i < stack->count; i++)
            stack->steps[i - 1] = stack->steps[i];
        stack->count--;
        if (stack->head > 0)
            stack->head--;
    }

    /* Uzmi piksele iz paint.bitmap */
    gfx_bitmap_alloc_t alloc;
    errno_t rc = gfx_bitmap_get_alloc(paint.bitmap, &alloc);
    if (rc != EOK) {
        printf("push_undo: greska pri get_alloc (%d)\n", rc);
        return;
    }

    uint32_t *pixels = (uint32_t *)((uint8_t *)alloc.pixels + alloc.off0);

    /* Encode u RLE */
    rle_image_t rle;
    rc = rle_encode(pixels, paint.width, paint.height, alloc.pitch, &rle);
    if (rc != EOK) {
        printf("push_undo: greska pri encode (%d)\n", rc);
        return;
    }

    /* Sačuvaj u stack */
    stack->steps[stack->count] = rle;
    stack->count++;
    stack->head = stack->count;

    printf("push_undo: step %d, %zu parova, %zu bajtova\n",
        stack->count, rle.count, rle_size_bytes(&rle));
}

// Undo: Restore the canvas from the most recent undo file
void undo(void)
{
    undo_stack_t *stack = &paint.undo_stack;

    if (stack->head <= 1) {
        printf("undo: nema koraka\n");
        return;
    }

    stack->head--;
    rle_image_t *rle = &stack->steps[stack->head - 1];

    /* 1) Napravi temp bitmapu (bez bmpf_direct_output) */
    gfx_bitmap_params_t params;
    gfx_bitmap_params_init(&params);
    params.rect.p0.x = 0;
    params.rect.p0.y = 0;
    params.rect.p1.x = paint.width;
    params.rect.p1.y = paint.height;

    gfx_bitmap_t *temp_bmp = NULL;
    errno_t rc = gfx_bitmap_create(paint.gc, &params, NULL, &temp_bmp);
    if (rc != EOK) {
        printf("undo: greska pri kreiranju temp bitmap\n");
        return;
    }

    /* 2) Uzmi alloc za temp bitmapu */
    gfx_bitmap_alloc_t temp_alloc;
    gfx_bitmap_get_alloc(temp_bmp, &temp_alloc);
    uint32_t *temp_pixels = (uint32_t *)((uint8_t *)temp_alloc.pixels + temp_alloc.off0);

    /* 3) Decode RLE u temp bitmapu */
    rc = rle_decode(rle, temp_pixels, temp_alloc.pitch);
    if (rc != EOK) {
        printf("undo: greska pri decode\n");
        gfx_bitmap_destroy(temp_bmp);
        return;
    }

    /* 4) Kopiraj u paint.bitmap */
    gfx_bitmap_alloc_t paint_alloc;
    gfx_bitmap_get_alloc(paint.bitmap, &paint_alloc);
    uint32_t *paint_pixels = (uint32_t *)((uint8_t *)paint_alloc.pixels + paint_alloc.off0);

    /* Kopiraj red po red (poštuj pitch) */
    size_t row_bytes = paint.width * 4;
    for (uint32_t y = 0; y <(unsigned int)paint.height; y++) {
        memcpy((uint8_t *)paint_pixels + y * paint_alloc.pitch,
               (uint8_t *)temp_pixels + y * temp_alloc.pitch,
               row_bytes);
    }

    /* 5) Renderuj temp bitmapu na gc */
    rc = gfx_bitmap_render(temp_bmp, &params.rect, NULL);
    if (rc != EOK) {
        printf("undo: greska pri render\n");
        gfx_bitmap_destroy(temp_bmp);
        return;
    }

    gfx_update(paint.gc);
    ui_menu_bar_paint(paint.menubar);
    ui_wdecor_paint(paint.wdecor);

    gfx_bitmap_destroy(temp_bmp);

    printf("undo: vracen korak %d\n", stack->head);
}



// Redo: Restore the canvas from the redo file
void redo(void)
{
    undo_stack_t *stack = &paint.undo_stack;

    if (stack->head >= stack->count) {
        printf("redo: nema koraka (head=%d count=%d)\n",
            stack->head, stack->count);
        return;
    }

    stack->head++;
    rle_image_t *rle = &stack->steps[stack->head - 1];

    /* 1) Napravi temp bitmapu */
    gfx_bitmap_params_t params;
    gfx_bitmap_params_init(&params);
    params.rect.p0.x = 0;
    params.rect.p0.y = 0;
    params.rect.p1.x = paint.width;
    params.rect.p1.y = paint.height;

    gfx_bitmap_t *temp_bmp = NULL;
    errno_t rc = gfx_bitmap_create(paint.gc, &params, NULL, &temp_bmp);
    if (rc != EOK) {
        printf("redo: greska pri kreiranju temp bitmap\n");
        return;
    }

    /* 2) Uzmi alloc za temp bitmapu */
    gfx_bitmap_alloc_t temp_alloc;
    gfx_bitmap_get_alloc(temp_bmp, &temp_alloc);
    uint32_t *temp_pixels = (uint32_t *)((uint8_t *)temp_alloc.pixels + temp_alloc.off0);

    /* 3) Decode RLE */
    rc = rle_decode(rle, temp_pixels, temp_alloc.pitch);
    if (rc != EOK) {
        printf("redo: greska pri decode\n");
        gfx_bitmap_destroy(temp_bmp);
        return;
    }

    /* 4) Kopiraj u paint.bitmap */
    gfx_bitmap_alloc_t paint_alloc;
    gfx_bitmap_get_alloc(paint.bitmap, &paint_alloc);
    uint32_t *paint_pixels = (uint32_t *)((uint8_t *)paint_alloc.pixels + paint_alloc.off0);

    size_t row_bytes = paint.width * 4;
    for (uint32_t y = 0; y < (unsigned int)paint.height; y++) {
        memcpy((uint8_t *)paint_pixels + y * paint_alloc.pitch,
               (uint8_t *)temp_pixels + y * temp_alloc.pitch,
               row_bytes);
    }

    /* 5) Renderuj temp bitmapu */
    rc = gfx_bitmap_render(temp_bmp, &params.rect, NULL);
    if (rc != EOK) {
        printf("redo: greska pri render\n");
        gfx_bitmap_destroy(temp_bmp);
        return;
    }

    gfx_update(paint.gc);
    ui_menu_bar_paint(paint.menubar);
    ui_wdecor_paint(paint.wdecor);

    gfx_bitmap_destroy(temp_bmp);

    printf("redo: vracen korak %d\n", stack->head);
}

bool img_load(gfx_context_t *gc, const char *fname, gfx_bitmap_t **rbitmap, gfx_rect_t *rect) {
    // Ensure filename is duplicated to prevent scope issues
    char *filename_dup = str_dup(fname);  // This allocates memory for the filename
    if (!filename_dup) {
        printf("Failed to duplicate filename.\n");
        return false;
    }

    int fd;
    printf("Attempting to open file: %s\n", filename_dup);
    errno_t rc = vfs_lookup_open(filename_dup, WALK_REGULAR, MODE_READ, &fd);
    if (rc != EOK) {
        printf("Failed to open file: %s, error: %d\n", filename_dup, rc);
        free(filename_dup);  // Free before returning
        return false;
    }

    vfs_stat_t stat;
    rc = vfs_stat(fd, &stat);
    if (rc != EOK) {
        printf("Failed to stat file: %s, error: %d\n", filename_dup, rc);
        vfs_put(fd);
        free(filename_dup);  // Free before returning
        return false;
    }

    void *tga = malloc(stat.size);
    if (tga == NULL) {
        printf("Ne mogu da alociram memory for image\n");
        vfs_put(fd);
        free(filename_dup);  // Free before returning
        return false;
    }

    size_t nread;
    rc = vfs_read(fd, (aoff64_t []) { 0 }, tga, stat.size, &nread);
    vfs_put(fd);
    if (rc != EOK || nread != stat.size) {
        printf("Failed to read file: %s, error: %d\n", filename_dup, rc);
        free(tga);
        free(filename_dup);  // Free before returning
        return false;
    }

    gfx_bitmap_t *bitmap = NULL;
    gfx_rect_t bitmap_rect;
    
    rc = decode_tga(gc, tga, stat.size, &bitmap, &bitmap_rect);
    free(tga);

    if (rc != EOK) {
        printf("Failed to decode image: %s\n", filename_dup);
        free(filename_dup);  // Free after usage
        return false;
    }

    // Render the bitmap
    rc = gfx_bitmap_render(bitmap, &bitmap_rect, NULL);
    if (rc != EOK) {
        printf("Failed to render bitmap.\n");
        gfx_bitmap_destroy(bitmap);
        free(filename_dup);
        return false;
    }
 ui_menu_bar_paint(paint.menubar);
    // Update the graphics context
    gfx_update(paint.gc);
ui_wdecor_paint(paint.wdecor);
   
    // Store the loaded bitmap for future use (undo/redo)
    *rbitmap = bitmap;
    *rect = bitmap_rect;

    printf("Successfully loaded and rendered image: %s\n", filename_dup);

    // Free the filename after use
    free(filename_dup);

    return true;
}


// Function to save the pixelmap to a PPM file
void save_pixelmap_to_ppm(pixelmap_t *pixelmap, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        printf("Error opening file for writing pixelmap log!\n");
        return;
    }

    // PPM header (plain text format)
    fprintf(file, "P3\n");
    fprintf(file, "# Pixelmap log\n");
    fprintf(file, "%lu %lu\n", pixelmap->width, pixelmap->height);
    fprintf(file, "255\n"); // Max color value

    for (size_t y = 0; y < pixelmap->height; y++) {
        for (size_t x = 0; x < pixelmap->width; x++) {
            pixel_t pixel = pixelmap_get_pixel(pixelmap, x, y);

            uint8_t rgb[3];  // Array to hold converted color

            // Ako koristiš 16-bitni format (RGB 565), koristi konverziju
            pixel2rgb_565_le(rgb, pixel);  
            
            // Ispisujemo R, G, B vrednosti (ignorišemo alpha kanal)
            fprintf(file, "%d %d %d\n", rgb[0], rgb[1], rgb[2]);
        }
    }

    fclose(file);
    printf("Pixelmap data logged successfully.\n");
}

void redraw_canvas() {
    //clear_canvas(&paint);  // Očisti ceo ekran
 
  // After drawing everything, update the screen
      gfx_bitmap_render(paint.bitmap, &paint.bparams.rect, NULL);


    // After drawing everything, update the screen
    errno_t rc = gfx_update(paint.gc);
    if (rc != EOK) {
        // Handle any potential errors, like updating the screen
        printf("Error updating the screen: %d\n", rc);
    }
}



void clear_canvas(void) {
   
    gfx_rect_t rect;
    ui_window_get_app_rect(paint.window, &rect);
    size_t width = rect.p1.x;   // Width from rect
    size_t height = rect.p1.y;  // Height from rect
    gfx_color_t old_color;
    // Create the white color using gfx_color_new_rgb_i16 (65535 is the max value for each channel)
   // gfx_color_t white_color;
    old_color =*paint.color;
      if (paint.color) {
        gfx_color_delete(paint.color);
    }
    int rc = gfx_color_new_rgb_i16(0xFFFF, 0xFFFF, 0xFFFF, &paint.color);  // White color
    if (rc != EOK) {
        printf("Greska pri kreiranju white color\n");
        return;  // Exit if there's an error creating the color
    }
    gfx_set_color(paint.gc, paint.color);    
    // Set the color to white (or any other color you want)
  

    gfx_rect_t fill_rect = {
        .p0 = { 2, 43 },
        .p1 = { width + 6, height - 8 }
    };
    gfx_fill_rect(paint.gc, &fill_rect);

    // Update the screen with the new content
    gfx_update(paint.gc);  // This should render the filled canvas to the screen
  gfx_set_color(paint.gc, &old_color);   
    printf("Canvas cleared and filled with white.\n");
    return;
}

void delete_file(const char *path) {
    errno_t rc = vfs_unlink_path(path);
    if (rc == EOK) {
        printf("File '%s' deleted successfully.\n", path);
    } else {
        printf("Failed to delete file '%s': %d\n", path, rc);
    }
}

// Check if a file exists using vfs_lookup()
bool file_exists(const char *path) {
    int fd;
    errno_t rc = vfs_lookup(path, WALK_REGULAR, &fd);
    if (rc == EOK) {
        vfs_put(fd); // Close the file descriptor
        return true;
    }
    return false;
}


void undo_stack_clear(void)
{
    undo_stack_t *stack = &paint.undo_stack;

    for (int i = 0; i < stack->count; i++) {
        rle_free(&stack->steps[i]);
    }

    stack->count = 0;
    stack->head = 0;

    printf("undo_stack_clear: stack ociscen\n");
}
/** @}
 */
