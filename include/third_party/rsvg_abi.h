#ifndef KINETIX_THIRD_PARTY_RSVG_ABI_H
#define KINETIX_THIRD_PARTY_RSVG_ABI_H

/**
 * @file rsvg_abi.h
 * @brief Minimal C ABI declarations for librsvg-2.so.2 (2.54.x) to render
 *        scalable SVG vector brand assets, multi-stop duotone icons, and
 *        illustrations directly onto native Cairo surfaces.
 */

#include "third_party/cairo_abi.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _RsvgHandle RsvgHandle;
typedef struct _GError GError;

typedef struct _RsvgRectangle {
    double x;
    double y;
    double width;
    double height;
} RsvgRectangle;

RsvgHandle* rsvg_handle_new_from_data(const uint8_t* data, size_t data_len, GError** error);
RsvgHandle* rsvg_handle_new_from_file(const char* file_name, GError** error);
void rsvg_handle_set_dpi(RsvgHandle* handle, double dpi);
int rsvg_handle_render_cairo(RsvgHandle* handle, cairo_t* cr);
int rsvg_handle_render_document(RsvgHandle* handle, cairo_t* cr, const RsvgRectangle* viewport, GError** error);

#ifdef __cplusplus
}
#endif

#endif // KINETIX_THIRD_PARTY_RSVG_ABI_H
