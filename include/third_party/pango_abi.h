#pragma once
// Exact C ABI declarations for Pango, PangoCairo, and GObject
// (libpango-1.0.so.0, libpangocairo-1.0.so.0, libgobject-2.0.so.0)
// Enables HarfBuzz-shaped typography with Inter, Inter Display, Noto Sans Mono,
// IPA phonetics, and Noto Sans CJK JP (Japanese Kanji/Kana) inside Cairo surfaces.

#include "third_party/cairo_abi.h"
#include <cstdint>

extern "C" {

typedef struct _PangoLayout PangoLayout;
typedef struct _PangoFontDescription PangoFontDescription;

typedef enum {
    PANGO_ALIGN_LEFT = 0,
    PANGO_ALIGN_CENTER = 1,
    PANGO_ALIGN_RIGHT = 2
} PangoAlignment;

typedef enum {
    PANGO_ELLIPSIZE_NONE = 0,
    PANGO_ELLIPSIZE_START = 1,
    PANGO_ELLIPSIZE_MIDDLE = 2,
    PANGO_ELLIPSIZE_END = 3
} PangoEllipsizeMode;

#define PANGO_SCALE 1024

PangoLayout* pango_cairo_create_layout(cairo_t* cr);
void pango_cairo_update_layout(cairo_t* cr, PangoLayout* layout);
void pango_cairo_show_layout(cairo_t* cr, PangoLayout* layout);

void pango_layout_set_text(PangoLayout* layout, const char* text, int length);
void pango_layout_set_markup(PangoLayout* layout, const char* markup, int length);
void pango_layout_set_width(PangoLayout* layout, int width);
void pango_layout_set_spacing(PangoLayout* layout, int spacing);
void pango_layout_set_alignment(PangoLayout* layout, PangoAlignment alignment);
void pango_layout_set_ellipsize(PangoLayout* layout, PangoEllipsizeMode ellipsize);
void pango_layout_get_pixel_size(PangoLayout* layout, int* width, int* height);

PangoFontDescription* pango_font_description_from_string(const char* str);
void pango_layout_set_font_description(PangoLayout* layout, const PangoFontDescription* desc);
void pango_font_description_free(PangoFontDescription* desc);

void g_object_unref(void* object);

} // extern "C"
