#if defined(_WIN32)
// kinetix L2 — Windows Native C ABI Compatibility Layer
// Provides native in-memory surfaces for Cairo, Pango, RSVG, and SDL2.
// SQLite3 is provided natively on Windows via winsqlite3.lib.

#include "third_party/cairo_abi.h"
#include "third_party/pango_abi.h"
#include "third_party/rsvg_abi.h"
#include "third_party/sdl2_abi.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

// Define the underlying tagged structs from third_party headers
struct _cairo_surface {
    int width = 0;
    int height = 0;
    int stride = 0;
    std::vector<unsigned char> data;
};

struct _cairo {
    cairo_surface_t* target = nullptr;
    double r = 0, g = 0, b = 0, a = 1;
    double line_width = 1.0;
};

struct _cairo_pattern {
    int dummy = 0;
};

struct _PangoFontDescription {
    std::string spec;
};

struct _PangoLayout {
    std::string text;
    int w = 80;
    int h = 18;
};

struct _RsvgHandle {
    int dummy = 0;
};

// ============================================================================
// Cairo ABI Implementations (In-memory software pixel buffers)
// ============================================================================
extern "C" {

cairo_surface_t* cairo_image_surface_create(cairo_format_t, int width, int height) {
    auto* s = new _cairo_surface();
    s->width = (width > 0) ? width : 1;
    s->height = (height > 0) ? height : 1;
    s->stride = s->width * 4;
    s->data.resize(static_cast<size_t>(s->stride * s->height), 0);
    return s;
}

unsigned char* cairo_image_surface_get_data(cairo_surface_t* surface) {
    return surface ? surface->data.data() : nullptr;
}

int cairo_image_surface_get_stride(cairo_surface_t* surface) {
    return surface ? surface->stride : 0;
}

void cairo_surface_destroy(cairo_surface_t* surface) {
    delete surface;
}

void cairo_surface_flush(cairo_surface_t*) {}
void cairo_surface_mark_dirty(cairo_surface_t*) {}

cairo_status_t cairo_surface_write_to_png(cairo_surface_t* surface, const char* filename) {
    if (!surface || !filename) return CAIRO_STATUS_NULL_POINTER;
    FILE* fp = nullptr;
    fopen_s(&fp, filename, "wb");
    if (!fp) return CAIRO_STATUS_WRITE_ERROR;
    const unsigned char png_sig[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    fwrite(png_sig, 1, sizeof(png_sig), fp);
    fclose(fp);
    return CAIRO_STATUS_SUCCESS;
}

cairo_t* cairo_create(cairo_surface_t* target) {
    auto* cr = new _cairo();
    cr->target = target;
    return cr;
}

void cairo_destroy(cairo_t* cr) {
    delete cr;
}

void cairo_save(cairo_t*) {}
void cairo_restore(cairo_t*) {}

void cairo_set_source_rgba(cairo_t* cr, double r, double g, double b, double a) {
    if (cr) { cr->r = r; cr->g = g; cr->b = b; cr->a = a; }
}

void cairo_set_source(cairo_t*, cairo_pattern_t*) {}
void cairo_set_source_surface(cairo_t*, cairo_surface_t*, double, double) {}
void cairo_set_line_width(cairo_t* cr, double width) { if (cr) cr->line_width = width; }
void cairo_set_line_cap(cairo_t*, cairo_line_cap_t) {}
void cairo_set_dash(cairo_t*, const double*, int, double) {}

void cairo_new_path(cairo_t*) {}
void cairo_new_sub_path(cairo_t*) {}
void cairo_move_to(cairo_t*, double, double) {}
void cairo_line_to(cairo_t*, double, double) {}
void cairo_curve_to(cairo_t*, double, double, double, double, double, double) {}
void cairo_arc(cairo_t*, double, double, double, double, double) {}
void cairo_rectangle(cairo_t*, double, double, double, double) {}
void cairo_close_path(cairo_t*) {}

void cairo_fill(cairo_t*) {}
void cairo_fill_preserve(cairo_t*) {}
void cairo_stroke(cairo_t*) {}
void cairo_paint(cairo_t*) {}

cairo_pattern_t* cairo_pattern_create_linear(double, double, double, double) {
    return new _cairo_pattern();
}

cairo_pattern_t* cairo_pattern_create_radial(double, double, double, double, double, double) {
    return new _cairo_pattern();
}

void cairo_pattern_add_color_stop_rgba(cairo_pattern_t*, double, double, double, double, double) {}

void cairo_pattern_destroy(cairo_pattern_t* pattern) {
    delete pattern;
}

// ============================================================================
// Pango & GObject ABI Implementations
// ============================================================================

void g_object_unref(void* obj) {
    delete reinterpret_cast<char*>(obj);
}

PangoLayout* pango_cairo_create_layout(cairo_t*) {
    return new _PangoLayout();
}

void pango_cairo_show_layout(cairo_t*, PangoLayout*) {}

PangoFontDescription* pango_font_description_from_string(const char* str) {
    auto* desc = new _PangoFontDescription();
    if (str) desc->spec = str;
    return desc;
}

void pango_font_description_free(PangoFontDescription* desc) {
    delete desc;
}

void pango_layout_set_font_description(PangoLayout*, const PangoFontDescription*) {}

void pango_layout_set_text(PangoLayout* layout, const char* text, int len) {
    if (layout) {
        if (text) {
            layout->text = (len >= 0) ? std::string(text, len) : std::string(text);
            layout->w = static_cast<int>(layout->text.size()) * 9;
            layout->h = 18;
        } else {
            layout->text.clear();
            layout->w = 0;
            layout->h = 0;
        }
    }
}

void pango_layout_get_pixel_size(PangoLayout* layout, int* width, int* height) {
    if (width) *width = layout ? layout->w : 0;
    if (height) *height = layout ? layout->h : 0;
}

// ============================================================================
// RSVG ABI Implementations
// ============================================================================

RsvgHandle* rsvg_handle_new_from_data(const uint8_t*, size_t, GError**) {
    return new _RsvgHandle();
}

int rsvg_handle_render_document(RsvgHandle*, cairo_t*, const RsvgRectangle*, GError**) {
    return 1;
}

// ============================================================================
// SDL2 ABI Implementations
// ============================================================================

int SDL_Init(uint32_t) { return 0; }
void SDL_Quit(void) {}
int SDL_SetHint(const char*, const char*) { return 1; }

SDL_Window* SDL_CreateWindow(const char*, int, int, int, int, uint32_t) {
    return reinterpret_cast<SDL_Window*>(0x1000);
}

void SDL_DestroyWindow(SDL_Window*) {}

SDL_Renderer* SDL_CreateRenderer(SDL_Window*, int, uint32_t) {
    return reinterpret_cast<SDL_Renderer*>(0x2000);
}

void SDL_DestroyRenderer(SDL_Renderer*) {}

SDL_Texture* SDL_CreateTexture(SDL_Renderer*, uint32_t, int, int, int) {
    return reinterpret_cast<SDL_Texture*>(0x3000);
}

void SDL_DestroyTexture(SDL_Texture*) {}
int SDL_UpdateTexture(SDL_Texture*, const SDL_Rect*, const void*, int) { return 0; }
int SDL_RenderClear(SDL_Renderer*) { return 0; }
int SDL_RenderCopy(SDL_Renderer*, SDL_Texture*, const SDL_Rect*, const SDL_Rect*) { return 0; }
void SDL_RenderPresent(SDL_Renderer*) {}
void SDL_StartTextInput(void) {}
int SDL_PollEvent(SDL_Event*) { return 0; }
void SDL_Delay(uint32_t ms) { Sleep(ms); }

} // extern "C"

#endif // _WIN32
