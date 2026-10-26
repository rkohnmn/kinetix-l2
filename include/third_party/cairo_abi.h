#pragma once
// Exact C ABI declarations for Cairo 2D Vector Graphics Library (libcairo.so.2)
// Used for native vector UI rendering, gradients, drop shadows, Bezier curves, and PNG export.

#include <cstdint>

extern "C" {

typedef struct _cairo cairo_t;
typedef struct _cairo_surface cairo_surface_t;
typedef struct _cairo_pattern cairo_pattern_t;

typedef enum _cairo_status {
    CAIRO_STATUS_SUCCESS = 0,
    CAIRO_STATUS_NO_MEMORY,
    CAIRO_STATUS_INVALID_RESTORE,
    CAIRO_STATUS_INVALID_POP_GROUP,
    CAIRO_STATUS_NO_CURRENT_POINT,
    CAIRO_STATUS_INVALID_MATRIX,
    CAIRO_STATUS_INVALID_STATUS,
    CAIRO_STATUS_NULL_POINTER,
    CAIRO_STATUS_INVALID_STRING,
    CAIRO_STATUS_INVALID_PATH_DATA,
    CAIRO_STATUS_READ_ERROR,
    CAIRO_STATUS_WRITE_ERROR
} cairo_status_t;

typedef enum _cairo_format {
    CAIRO_FORMAT_INVALID   = -1,
    CAIRO_FORMAT_ARGB32    = 0,
    CAIRO_FORMAT_RGB24     = 1,
    CAIRO_FORMAT_A8        = 2,
    CAIRO_FORMAT_A1        = 3,
    CAIRO_FORMAT_RGB16_565 = 4,
    CAIRO_FORMAT_RGB30     = 5
} cairo_format_t;

typedef enum _cairo_font_slant {
    CAIRO_FONT_SLANT_NORMAL  = 0,
    CAIRO_FONT_SLANT_ITALIC  = 1,
    CAIRO_FONT_SLANT_OBLIQUE = 2
} cairo_font_slant_t;

typedef enum _cairo_font_weight {
    CAIRO_FONT_WEIGHT_NORMAL = 0,
    CAIRO_FONT_WEIGHT_BOLD   = 1
} cairo_font_weight_t;

typedef enum _cairo_line_cap {
    CAIRO_LINE_CAP_BUTT   = 0,
    CAIRO_LINE_CAP_ROUND  = 1,
    CAIRO_LINE_CAP_SQUARE = 2
} cairo_line_cap_t;

typedef enum _cairo_line_join {
    CAIRO_LINE_JOIN_MITER = 0,
    CAIRO_LINE_JOIN_ROUND = 1,
    CAIRO_LINE_JOIN_BEVEL = 2
} cairo_line_join_t;

typedef struct {
    double x_bearing;
    double y_bearing;
    double width;
    double height;
    double x_advance;
    double y_advance;
} cairo_text_extents_t;

const char* cairo_version_string(void);

cairo_surface_t* cairo_image_surface_create(cairo_format_t format, int width, int height);
cairo_surface_t* cairo_image_surface_create_for_data(
    unsigned char* data,
    cairo_format_t format,
    int width,
    int height,
    int stride
);
unsigned char* cairo_image_surface_get_data(cairo_surface_t* surface);
int cairo_image_surface_get_width(cairo_surface_t* surface);
int cairo_image_surface_get_height(cairo_surface_t* surface);
int cairo_image_surface_get_stride(cairo_surface_t* surface);
void cairo_surface_flush(cairo_surface_t* surface);
void cairo_surface_mark_dirty(cairo_surface_t* surface);
void cairo_surface_destroy(cairo_surface_t* surface);
cairo_status_t cairo_surface_write_to_png(cairo_surface_t* surface, const char* filename);

cairo_t* cairo_create(cairo_surface_t* target);
void cairo_destroy(cairo_t* cr);
void cairo_save(cairo_t* cr);
void cairo_restore(cairo_t* cr);

void cairo_set_source_rgb(cairo_t* cr, double red, double green, double blue);
void cairo_set_source_rgba(cairo_t* cr, double red, double green, double blue, double alpha);
void cairo_set_source(cairo_t* cr, cairo_pattern_t* source);
void cairo_set_source_surface(cairo_t* cr, cairo_surface_t* surface, double x, double y);

cairo_pattern_t* cairo_pattern_create_linear(double x0, double y0, double x1, double y1);
cairo_pattern_t* cairo_pattern_create_radial(double cx0, double cy0, double radius0, double cx1, double cy1, double radius1);
void cairo_pattern_add_color_stop_rgb(cairo_pattern_t* pattern, double offset, double red, double green, double blue);
void cairo_pattern_add_color_stop_rgba(cairo_pattern_t* pattern, double offset, double red, double green, double blue, double alpha);
void cairo_pattern_destroy(cairo_pattern_t* pattern);

void cairo_set_line_width(cairo_t* cr, double width);
void cairo_set_line_cap(cairo_t* cr, cairo_line_cap_t line_cap);
void cairo_set_line_join(cairo_t* cr, cairo_line_join_t line_join);
void cairo_set_dash(cairo_t* cr, const double* dashes, int num_dashes, double offset);

void cairo_new_path(cairo_t* cr);
void cairo_new_sub_path(cairo_t* cr);
void cairo_close_path(cairo_t* cr);
void cairo_move_to(cairo_t* cr, double x, double y);
void cairo_line_to(cairo_t* cr, double x, double y);
void cairo_curve_to(cairo_t* cr, double x1, double y1, double x2, double y2, double x3, double y3);
void cairo_arc(cairo_t* cr, double xc, double yc, double radius, double angle1, double angle2);
void cairo_arc_negative(cairo_t* cr, double xc, double yc, double radius, double angle1, double angle2);
void cairo_rectangle(cairo_t* cr, double x, double y, double width, double height);

void cairo_stroke(cairo_t* cr);
void cairo_stroke_preserve(cairo_t* cr);
void cairo_fill(cairo_t* cr);
void cairo_fill_preserve(cairo_t* cr);
void cairo_clip(cairo_t* cr);
void cairo_reset_clip(cairo_t* cr);
void cairo_paint(cairo_t* cr);

void cairo_select_font_face(
    cairo_t* cr,
    const char* family,
    cairo_font_slant_t slant,
    cairo_font_weight_t weight
);
void cairo_set_font_size(cairo_t* cr, double size);
void cairo_text_extents(cairo_t* cr, const char* utf8, cairo_text_extents_t* extents);
void cairo_show_text(cairo_t* cr, const char* utf8);

} // extern "C"
