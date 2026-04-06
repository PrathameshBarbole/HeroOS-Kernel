#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <kernel/types.h>

/* Multiboot2 framebuffer info (populated by bootloader) */
typedef struct {
    uintptr_t addr;         /* Physical address of framebuffer */
    uint32_t  pitch;        /* Bytes per row */
    uint32_t  width;        /* Pixels per row */
    uint32_t  height;       /* Rows */
    uint8_t   bpp;          /* Bits per pixel */
    uint8_t   type;         /* 1 = RGB */
} fb_info_t;

/* Colour (ARGB) */
typedef uint32_t color_t;
#define RGB(r,g,b)  ((color_t)(((r)<<16)|((g)<<8)|(b)))
#define RGBA(r,g,b,a) ((color_t)(((a)<<24)|((r)<<16)|((g)<<8)|(b)))

/* HeroOS colour palette */
#define COLOR_BLACK       RGB(0x10, 0x10, 0x18)
#define COLOR_WHITE       RGB(0xFF, 0xFF, 0xFF)
#define COLOR_ACCENT      RGB(0x00, 0xD4, 0xFF)   /* HeroOS cyan */
#define COLOR_ACCENT2     RGB(0x7C, 0x3A, 0xFF)   /* Purple accent */
#define COLOR_BG          RGB(0x12, 0x12, 0x1F)   /* Dark background */
#define COLOR_PANEL       RGB(0x1E, 0x1E, 0x2E)
#define COLOR_TEXT        RGB(0xCB, 0xD5, 0xE1)
#define COLOR_SUCCESS     RGB(0x00, 0xFF, 0x88)
#define COLOR_WARNING     RGB(0xFF, 0xC2, 0x00)
#define COLOR_ERROR       RGB(0xFF, 0x45, 0x5A)

/* Framebuffer API */
void fb_init(fb_info_t *info);
void fb_set_pixel(uint32_t x, uint32_t y, color_t color);
void fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, color_t color);
void fb_draw_hline(uint32_t x, uint32_t y, uint32_t len, color_t color);
void fb_draw_vline(uint32_t x, uint32_t y, uint32_t len, color_t color);
void fb_draw_rect_outline(uint32_t x, uint32_t y, uint32_t w, uint32_t h, color_t color);
void fb_draw_char(uint32_t x, uint32_t y, char c, color_t fg, color_t bg);
void fb_draw_string(uint32_t x, uint32_t y, const char *s, color_t fg, color_t bg);
void fb_scroll_up(uint32_t lines);
void fb_clear(color_t color);
void fb_get_info(fb_info_t *out);

/* Terminal (framebuffer-based) */
void fb_term_init(void);
void fb_term_putc(char c);
void fb_term_puts(const char *s);
void fb_term_set_color(color_t fg, color_t bg);

/* Driver registration */
void framebuffer_driver_register(void);

#endif /* FRAMEBUFFER_H */
