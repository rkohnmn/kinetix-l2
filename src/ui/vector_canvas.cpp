#include "kinetix/ui/vector_canvas.hpp"
#include "kinetix/domain/memory_pool.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unordered_map>

namespace kinetix::ui {

namespace {
constexpr double kPi = 3.14159265358979323846;

std::string build_pango_font_spec(const char* family, bool bold, double px_size) {
    const std::string fam = (family && family[0] != '\0') ? family : "Inter";
    const int pt_size = std::max(6, static_cast<int>(std::round(px_size * 0.76)));
    std::ostringstream oss;
    oss << fam << (bold ? " Semi-Bold " : " Regular ") << pt_size;
    return oss.str();
}

struct RenderAssetCache {
    std::unordered_map<std::string, PangoFontDescription*> font_cache;
    std::unordered_map<std::string, RsvgHandle*> svg_cache;
    std::unordered_map<std::string, cairo_surface_t*> sprite_cache;
    std::unordered_map<std::string, cairo_surface_t*> text_sprite_cache;
    std::unordered_map<std::string, std::string> file_cache;
    std::unordered_map<std::string, double> width_cache;

    ~RenderAssetCache() {
        for (auto& kv : text_sprite_cache) {
            if (kv.second) cairo_surface_destroy(kv.second);
        }
        for (auto& kv : sprite_cache) {
            if (kv.second) cairo_surface_destroy(kv.second);
        }
        for (auto& kv : svg_cache) {
            if (kv.second) g_object_unref(kv.second);
        }
        for (auto& kv : font_cache) {
            if (kv.second) pango_font_description_free(kv.second);
        }
    }
};

RenderAssetCache& asset_cache() {
    static RenderAssetCache s_cache;
    return s_cache;
}

const PangoFontDescription* get_cached_font_desc(const std::string& spec) {
    auto& fc = asset_cache().font_cache;
    auto it = fc.find(spec);
    if (it != fc.end()) {
        return it->second;
    }
    PangoFontDescription* desc = pango_font_description_from_string(spec.c_str());
    fc.emplace(spec, desc);
    return desc;
}

RsvgHandle* get_cached_rsvg_handle(const std::string& svg_xml) {
    auto& sc = asset_cache().svg_cache;
    auto it = sc.find(svg_xml);
    if (it != sc.end()) {
        return it->second;
    }
    RsvgHandle* h = rsvg_handle_new_from_data(
        reinterpret_cast<const uint8_t*>(svg_xml.data()),
        svg_xml.size(),
        nullptr
    );
    if (h) {
        sc.emplace(svg_xml, h);
    }
    return h;
}

std::string color_to_svg_hex(const ColorRGBA& c) {
    const int r = std::clamp(static_cast<int>(std::round(c.r * 255.0)), 0, 255);
    const int g = std::clamp(static_cast<int>(std::round(c.g * 255.0)), 0, 255);
    const int b = std::clamp(static_cast<int>(std::round(c.b * 255.0)), 0, 255);
    std::ostringstream oss;
    oss << "#" << std::hex << std::uppercase << std::setfill('0')
        << std::setw(2) << r << std::setw(2) << g << std::setw(2) << b;
    return oss.str();
}

// Embedded canonical Kinetix L2 SVG Brand Assets (matching assets/branding/*.svg 1:1)
const char* kEmbeddedMarkDarkSvg = R"SVG(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 256 256">
  <path d="M 130.5 61.5 H 61.5 V 117.5 H 120.5" fill="none" stroke="#F4EFE6" stroke-width="21" stroke-linecap="square" stroke-linejoin="miter"/>
  <path d="M 134.5 144.0 H 191.5 V 199.0 H 120.5" fill="none" stroke="#F4EFE6" stroke-width="21" stroke-linecap="square" stroke-linejoin="miter"/>
  <path d="M 124.5 81.5 C 166.0 89.0, 174.0 113.5, 145.5 132.0 C 118.0 150.0, 114.5 168.5, 129.0 188.0" fill="none" stroke="#91D1B5" stroke-width="23" stroke-linecap="butt"/>
  <rect x="182" y="51" width="23" height="23" fill="#EE9A7C"/>
</svg>
)SVG";

const char* kEmbeddedMarkLightSvg = R"SVG(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 256 256">
  <path d="M 130.5 61.5 H 61.5 V 117.5 H 120.5" fill="none" stroke="#16272B" stroke-width="21" stroke-linecap="square" stroke-linejoin="miter"/>
  <path d="M 134.5 144.0 H 191.5 V 199.0 H 120.5" fill="none" stroke="#16272B" stroke-width="21" stroke-linecap="square" stroke-linejoin="miter"/>
  <path d="M 124.5 81.5 C 166.0 89.0, 174.0 113.5, 145.5 132.0 C 118.0 150.0, 114.5 168.5, 129.0 188.0" fill="none" stroke="#1C7B5F" stroke-width="23" stroke-linecap="butt"/>
  <rect x="182" y="51" width="23" height="23" fill="#B6573D"/>
</svg>
)SVG";

} // namespace

VectorCanvas::VectorCanvas(int width, int height)
    : width_(width), height_(height) {
    surface_ = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width_, height_);
    cr_ = cairo_create(surface_);
    layout_ = pango_cairo_create_layout(cr_);
}

VectorCanvas::~VectorCanvas() {
    if (backdrop_cache_) {
        cairo_surface_destroy(backdrop_cache_);
        backdrop_cache_ = nullptr;
    }
    if (layout_) {
        g_object_unref(layout_);
        layout_ = nullptr;
    }
    if (cr_) {
        cairo_destroy(cr_);
        cr_ = nullptr;
    }
    if (surface_) {
        cairo_surface_destroy(surface_);
        surface_ = nullptr;
    }
}

void VectorCanvas::resize(int width, int height) {
    if (width == width_ && height == height_ && surface_ && cr_) {
        return;
    }
    if (backdrop_cache_) {
        cairo_surface_destroy(backdrop_cache_);
        backdrop_cache_ = nullptr;
        backdrop_cache_dark_ = -1;
    }
    if (layout_) {
        g_object_unref(layout_);
        layout_ = nullptr;
    }
    if (cr_) cairo_destroy(cr_);
    if (surface_) cairo_surface_destroy(surface_);
    width_ = std::max(1, width);
    height_ = std::max(1, height);
    surface_ = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width_, height_);
    cr_ = cairo_create(surface_);
    layout_ = pango_cairo_create_layout(cr_);
}

int VectorCanvas::stride() const noexcept {
    return surface_ ? cairo_image_surface_get_stride(surface_) : 0;
}

const unsigned char* VectorCanvas::pixels() const noexcept {
    if (!surface_) return nullptr;
    cairo_surface_flush(surface_);
    return cairo_image_surface_get_data(surface_);
}

bool VectorCanvas::export_png(const std::string& filepath) const {
    if (!surface_) return false;
    cairo_surface_flush(surface_);
    return cairo_surface_write_to_png(surface_, filepath.c_str()) == CAIRO_STATUS_SUCCESS;
}

void VectorCanvas::trace_rounded_rect_path(const Rect& rect, double radius) const {
    const double r = std::min({std::max(0.0, radius), rect.w * 0.5, rect.h * 0.5});
    cairo_new_path(cr_);
    cairo_new_sub_path(cr_);
    cairo_arc(cr_, rect.x + rect.w - r, rect.y + r, r, -kPi * 0.5, 0.0);
    cairo_arc(cr_, rect.x + rect.w - r, rect.y + rect.h - r, r, 0.0, kPi * 0.5);
    cairo_arc(cr_, rect.x + r, rect.y + rect.h - r, r, kPi * 0.5, kPi);
    cairo_arc(cr_, rect.x + r, rect.y + r, r, kPi, 1.5 * kPi);
    cairo_close_path(cr_);
}

void VectorCanvas::clear(const ColorRGBA& color) {
    if (!cr_) return;
    cairo_save(cr_);
    cairo_new_path(cr_);
    cairo_set_source_rgba(cr_, color.r, color.g, color.b, color.a);
    cairo_paint(cr_);
    cairo_restore(cr_);
}

void VectorCanvas::clear_gradient(const ColorRGBA& top_color, const ColorRGBA& bottom_color) {
    if (!cr_) return;
    cairo_save(cr_);
    cairo_new_path(cr_);
    memory::ScopedCairoPattern pat(cairo_pattern_create_linear(0.0, 0.0, 0.0, static_cast<double>(height_)));
    cairo_pattern_add_color_stop_rgba(pat.get(), 0.0, top_color.r, top_color.g, top_color.b, top_color.a);
    cairo_pattern_add_color_stop_rgba(pat.get(), 1.0, bottom_color.r, bottom_color.g, bottom_color.b, bottom_color.a);
    cairo_set_source(cr_, pat.get());
    cairo_paint(cr_);
    cairo_restore(cr_);
}

void VectorCanvas::draw_ambient_mesh_backdrop(const ThemePalette& theme) {
    if (!cr_) return;
    const int dark_flag = theme.is_dark ? 1 : 0;
    if (backdrop_cache_ && backdrop_cache_w_ == width_ && backdrop_cache_h_ == height_ && backdrop_cache_dark_ == dark_flag) {
        cairo_save(cr_);
        cairo_set_source_surface(cr_, backdrop_cache_, 0.0, 0.0);
        cairo_paint(cr_);
        cairo_restore(cr_);
        return;
    }
    clear_gradient(theme.bg_canvas_top, theme.bg_canvas_bottom);

    cairo_save(cr_);
    // 1. Top-Left Kinetic Mint (#91D1B5 / #1C7B5F) Radial Bloom
    {
        const double cx = width_ * 0.16;
        const double cy = height_ * 0.18;
        const double r = std::max(width_, height_) * 0.48;
        memory::ScopedCairoPattern glow1(cairo_pattern_create_radial(cx, cy, 10.0, cx, cy, r));
        const double a = theme.is_dark ? 0.10 : 0.06;
        cairo_pattern_add_color_stop_rgba(glow1.get(), 0.0, theme.primary_accent.r, theme.primary_accent.g, theme.primary_accent.b, a);
        cairo_pattern_add_color_stop_rgba(glow1.get(), 0.55, theme.primary_accent.r, theme.primary_accent.g, theme.primary_accent.b, a * 0.35);
        cairo_pattern_add_color_stop_rgba(glow1.get(), 1.0, theme.primary_accent.r, theme.primary_accent.g, theme.primary_accent.b, 0.0);
        cairo_set_source(cr_, glow1.get());
        cairo_paint(cr_);
    }

    // 2. Bottom-Right Terracotta Coral (#EE9A7C / #B6573D) Radial Bloom
    {
        const double cx = width_ * 0.85;
        const double cy = height_ * 0.82;
        const double r = std::max(width_, height_) * 0.45;
        memory::ScopedCairoPattern glow2(cairo_pattern_create_radial(cx, cy, 10.0, cx, cy, r));
        const double a = theme.is_dark ? 0.085 : 0.055;
        cairo_pattern_add_color_stop_rgba(glow2.get(), 0.0, theme.secondary_accent.r, theme.secondary_accent.g, theme.secondary_accent.b, a);
        cairo_pattern_add_color_stop_rgba(glow2.get(), 0.55, theme.secondary_accent.r, theme.secondary_accent.g, theme.secondary_accent.b, a * 0.30);
        cairo_pattern_add_color_stop_rgba(glow2.get(), 1.0, theme.secondary_accent.r, theme.secondary_accent.g, theme.secondary_accent.b, 0.0);
        cairo_set_source(cr_, glow2.get());
        cairo_paint(cr_);
    }

    // 3. Subtle Editorial Dot-Matrix Architectural Grid
    const ColorRGBA dot_col = theme.is_dark
        ? ColorRGBA::from_hex(0xF4EFE6, 0.032)
        : ColorRGBA::from_hex(0x16272B, 0.038);
    cairo_set_source_rgba(cr_, dot_col.r, dot_col.g, dot_col.b, dot_col.a);
    for (int y = 28; y < height_; y += 32) {
        for (int x = 28; x < width_; x += 32) {
            cairo_rectangle(cr_, static_cast<double>(x), static_cast<double>(y), 1.2, 1.2);
        }
    }
    cairo_fill(cr_);
    cairo_restore(cr_);

    if (backdrop_cache_) {
        cairo_surface_destroy(backdrop_cache_);
    }
    backdrop_cache_ = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width_, height_);
    backdrop_cache_w_ = width_;
    backdrop_cache_h_ = height_;
    backdrop_cache_dark_ = dark_flag;
    cairo_t* bcr = cairo_create(backdrop_cache_);
    cairo_set_source_surface(bcr, surface_, 0.0, 0.0);
    cairo_paint(bcr);
    cairo_destroy(bcr);
}

void VectorCanvas::apply_gaussian_blur_region(const Rect& region, int radius) {
    if (!surface_ || radius <= 0) return;
    cairo_surface_flush(surface_);

    unsigned char* raw = cairo_image_surface_get_data(surface_);
    const int row_stride = cairo_image_surface_get_stride(surface_);
    if (!raw || row_stride <= 0) return;

    const int x0 = std::clamp(static_cast<int>(std::floor(region.x)), 0, width_ - 1);
    const int y0 = std::clamp(static_cast<int>(std::floor(region.y)), 0, height_ - 1);
    const int x1 = std::clamp(static_cast<int>(std::ceil(region.x + region.w)), 0, width_);
    const int y1 = std::clamp(static_cast<int>(std::ceil(region.y + region.h)), 0, height_);
    const int rw = x1 - x0;
    const int rh = y1 - y0;
    if (rw <= 2 || rh <= 2) return;

    const int r = std::clamp(radius, 1, 24);
    const int diam = 2 * r + 1;
    std::vector<uint32_t> temp(static_cast<size_t>(rw * rh), 0);

    // O(1) sliding-window separable horizontal + vertical box blur
    for (int y = 0; y < rh; ++y) {
        const uint32_t* src_row = reinterpret_cast<const uint32_t*>(raw + (y0 + y) * row_stride) + x0;
        uint32_t* dst_row = temp.data() + static_cast<size_t>(y * rw);
        uint32_t sum_b = 0, sum_g = 0, sum_r = 0, sum_a = 0;
        for (int k = -r; k <= r; ++k) {
            const uint32_t px = src_row[std::clamp(k, 0, rw - 1)];
            sum_b += (px & 0xFFU);
            sum_g += ((px >> 8) & 0xFFU);
            sum_r += ((px >> 16) & 0xFFU);
            sum_a += ((px >> 24) & 0xFFU);
        }
        for (int x = 0; x < rw; ++x) {
            dst_row[x] = ((sum_a / diam) << 24) | ((sum_r / diam) << 16) | ((sum_g / diam) << 8) | (sum_b / diam);
            const uint32_t out_px = src_row[std::clamp(x - r, 0, rw - 1)];
            const uint32_t in_px  = src_row[std::clamp(x + r + 1, 0, rw - 1)];
            sum_b = sum_b + (in_px & 0xFFU) - (out_px & 0xFFU);
            sum_g = sum_g + ((in_px >> 8) & 0xFFU) - ((out_px >> 8) & 0xFFU);
            sum_r = sum_r + ((in_px >> 16) & 0xFFU) - ((out_px >> 16) & 0xFFU);
            sum_a = sum_a + ((in_px >> 24) & 0xFFU) - ((out_px >> 24) & 0xFFU);
        }
    }
    for (int x = 0; x < rw; ++x) {
        uint32_t sum_b = 0, sum_g = 0, sum_r = 0, sum_a = 0;
        for (int k = -r; k <= r; ++k) {
            const uint32_t px = temp[static_cast<size_t>(std::clamp(k, 0, rh - 1) * rw + x)];
            sum_b += (px & 0xFFU);
            sum_g += ((px >> 8) & 0xFFU);
            sum_r += ((px >> 16) & 0xFFU);
            sum_a += ((px >> 24) & 0xFFU);
        }
        for (int y = 0; y < rh; ++y) {
            uint32_t* dst_row = reinterpret_cast<uint32_t*>(raw + (y0 + y) * row_stride) + x0;
            dst_row[x] = ((sum_a / diam) << 24) | ((sum_r / diam) << 16) | ((sum_g / diam) << 8) | (sum_b / diam);
            const uint32_t out_px = temp[static_cast<size_t>(std::clamp(y - r, 0, rh - 1) * rw + x)];
            const uint32_t in_px  = temp[static_cast<size_t>(std::clamp(y + r + 1, 0, rh - 1) * rw + x)];
            sum_b = sum_b + (in_px & 0xFFU) - (out_px & 0xFFU);
            sum_g = sum_g + ((in_px >> 8) & 0xFFU) - ((out_px >> 8) & 0xFFU);
            sum_r = sum_r + ((in_px >> 16) & 0xFFU) - ((out_px >> 16) & 0xFFU);
            sum_a = sum_a + ((in_px >> 24) & 0xFFU) - ((out_px >> 24) & 0xFFU);
        }
    }

    cairo_surface_mark_dirty(surface_);
}

void VectorCanvas::apply_perceptual_noise_grain(const Rect& region, double intensity) {
    if (!surface_ || intensity <= 0.0) return;
    cairo_surface_flush(surface_);

    unsigned char* raw = cairo_image_surface_get_data(surface_);
    const int row_stride = cairo_image_surface_get_stride(surface_);
    if (!raw || row_stride <= 0) return;

    const int x0 = std::clamp(static_cast<int>(std::floor(region.x)), 0, width_ - 1);
    const int y0 = std::clamp(static_cast<int>(std::floor(region.y)), 0, height_ - 1);
    const int x1 = std::clamp(static_cast<int>(std::ceil(region.x + region.w)), 0, width_);
    const int y1 = std::clamp(static_cast<int>(std::ceil(region.y + region.h)), 0, height_);
    const int amp = std::max(1, static_cast<int>(std::round(intensity * 255.0)));

    for (int y = y0; y < y1; ++y) {
        uint32_t* row = reinterpret_cast<uint32_t*>(raw + y * row_stride);
        for (int x = x0; x < x1; ++x) {
            uint32_t h = static_cast<uint32_t>(x) * 374761393U + static_cast<uint32_t>(y) * 668265263U;
            h = (h ^ (h >> 13)) * 1274126177U;
            const int delta = static_cast<int>(h & 0xFFU) * (2 * amp) / 255 - amp;

            const uint32_t px = row[x];
            const int b = std::clamp(static_cast<int>(px & 0xFFU) + delta, 0, 255);
            const int g = std::clamp(static_cast<int>((px >> 8) & 0xFFU) + delta, 0, 255);
            const int r = std::clamp(static_cast<int>((px >> 16) & 0xFFU) + delta, 0, 255);
            const uint32_t a = (px >> 24) & 0xFFU;
            row[x] = (a << 24) | (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
        }
    }

    cairo_surface_mark_dirty(surface_);
}

bool VectorCanvas::draw_svg_string(const std::string& svg_xml, const Rect& viewport) {
    if (!cr_ || svg_xml.empty() || viewport.w <= 0.0 || viewport.h <= 0.0) return false;
    const int sw = std::max(1, static_cast<int>(std::round(viewport.w)));
    const int sh = std::max(1, static_cast<int>(std::round(viewport.h)));
    const std::string cache_key = std::to_string(sw) + "x" + std::to_string(sh) + "|" + svg_xml;

    auto& s_sprite_cache = asset_cache().sprite_cache;
    auto it = s_sprite_cache.find(cache_key);
    if (it == s_sprite_cache.end()) {
        RsvgHandle* handle = get_cached_rsvg_handle(svg_xml);
        if (!handle) return false;
        cairo_surface_t* sprite = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, sw, sh);
        cairo_t* scr = cairo_create(sprite);
        const RsvgRectangle vp{0.0, 0.0, static_cast<double>(sw), static_cast<double>(sh)};
        const int ok = rsvg_handle_render_document(handle, scr, &vp, nullptr);
        cairo_destroy(scr);
        if (!ok) {
            cairo_surface_destroy(sprite);
            return false;
        }
        it = s_sprite_cache.emplace(cache_key, sprite).first;
    }

    cairo_save(cr_);
    cairo_set_source_surface(cr_, it->second, std::round(viewport.x), std::round(viewport.y));
    cairo_paint(cr_);
    cairo_restore(cr_);
    return true;
}

bool VectorCanvas::draw_svg_file(const std::string& svg_path, const Rect& viewport) {
    if (!cr_ || svg_path.empty() || viewport.w <= 0.0 || viewport.h <= 0.0) return false;
    auto& s_file_cache = asset_cache().file_cache;
    auto it = s_file_cache.find(svg_path);
    if (it == s_file_cache.end()) {
        std::ifstream ifs(svg_path, std::ios::in | std::ios::binary);
        if (!ifs.is_open()) {
            std::error_code ec;
            std::filesystem::path p(svg_path);
            if (std::filesystem::exists(std::filesystem::path("..") / p, ec)) {
                ifs.open(std::filesystem::path("..") / p, std::ios::in | std::ios::binary);
            }
        }
        if (!ifs.is_open()) return false;
        std::ostringstream ss;
        ss << ifs.rdbuf();
        it = s_file_cache.emplace(svg_path, ss.str()).first;
    }
    return draw_svg_string(it->second, viewport);
}

void VectorCanvas::draw_brand_logo(double x, double y, double height, bool is_dark_theme, bool compact_mark_only) {
    if (!cr_ || height <= 4.0) return;

    const std::string mark_file = is_dark_theme
        ? "assets/branding/kinetix-l2-mark-dark.svg"
        : "assets/branding/kinetix-l2-mark-light.svg";
    const char* fallback_mark = is_dark_theme ? kEmbeddedMarkDarkSvg : kEmbeddedMarkLightSvg;

    const Rect mark_rect{x, y, height, height};
    if (!draw_svg_file(mark_file, mark_rect)) {
        draw_svg_string(fallback_mark, mark_rect);
    }

    if (compact_mark_only) return;

    // Render crisp HarfBuzz-shaped "kinetix L2" lockup matching kinetix-l2-logo-dark.svg / light.svg
    const ColorRGBA word_col = is_dark_theme
        ? ColorRGBA::from_hex(0xF4EFE6)
        : ColorRGBA::from_hex(0x16272B);
    const ColorRGBA l2_col = is_dark_theme
        ? ColorRGBA::from_hex(0xEE9A7C)
        : ColorRGBA::from_hex(0xB6573D);

    const double word_px = height * 0.60;
    const double l2_px = height * 0.34;
    const double text_x = x + height + 8.0;
    const double baseline_y = y + height * 0.76;

    draw_text(text_x, baseline_y, "kinetix", word_px, word_col, true, "Inter Display");
    const double word_w = measure_text_width("kinetix", word_px, true, "Inter Display");
    draw_text(text_x + word_w + 5.0, baseline_y, "L2", l2_px, l2_col, true, "Inter");
}

void VectorCanvas::draw_rounded_rect(
    const Rect& rect,
    double radius,
    const ColorRGBA& fill_color,
    const ColorRGBA& border_color,
    double border_width
) {
    if (!cr_ || rect.w <= 0 || rect.h <= 0) return;
    cairo_save(cr_);
    trace_rounded_rect_path(rect, radius);

    if (fill_color.a > 0.0) {
        cairo_set_source_rgba(cr_, fill_color.r, fill_color.g, fill_color.b, fill_color.a);
        if (border_color.a > 0.0 && border_width > 0.0) {
            cairo_fill_preserve(cr_);
        } else {
            cairo_fill(cr_);
        }
    }
    if (border_color.a > 0.0 && border_width > 0.0) {
        cairo_set_source_rgba(cr_, border_color.r, border_color.g, border_color.b, border_color.a);
        cairo_set_line_width(cr_, border_width);
        cairo_stroke(cr_);
    }
    cairo_new_path(cr_);
    cairo_restore(cr_);
}

void VectorCanvas::draw_linear_gradient_rect(
    const Rect& rect,
    double radius,
    const ColorRGBA& c0,
    const ColorRGBA& c1,
    bool vertical,
    const ColorRGBA& border,
    double border_w
) {
    if (!cr_ || rect.w <= 0 || rect.h <= 0) return;
    cairo_save(cr_);
    trace_rounded_rect_path(rect, radius);

    memory::ScopedCairoPattern pat(
        vertical
            ? cairo_pattern_create_linear(rect.x, rect.y, rect.x, rect.y + rect.h)
            : cairo_pattern_create_linear(rect.x, rect.y, rect.x + rect.w, rect.y)
    );
    cairo_pattern_add_color_stop_rgba(pat.get(), 0.0, c0.r, c0.g, c0.b, c0.a);
    cairo_pattern_add_color_stop_rgba(pat.get(), 1.0, c1.r, c1.g, c1.b, c1.a);
    cairo_set_source(cr_, pat.get());

    if (border.a > 0.0 && border_w > 0.0) {
        cairo_fill_preserve(cr_);
        cairo_set_source_rgba(cr_, border.r, border.g, border.b, border.a);
        cairo_set_line_width(cr_, border_w);
        cairo_stroke(cr_);
    } else {
        cairo_fill(cr_);
    }
    cairo_new_path(cr_);
    cairo_restore(cr_);
}

void VectorCanvas::draw_photon_card(
    const Rect& rect,
    double radius,
    const ThemePalette& theme,
    bool elevated_shadow,
    const ColorRGBA& custom_border,
    double border_width,
    const ColorRGBA& top_accent_stripe
) {
    if (!cr_ || rect.w <= 0 || rect.h <= 0) return;

    if (elevated_shadow) {
        // Multi-layer soft ambient elevation shadow
        const double sh_base = theme.is_dark ? 0.14 : 0.035;
        draw_rounded_rect(Rect{rect.x - 2.0, rect.y + 6.0, rect.w + 4.0, rect.h + 2.0}, radius + 3.0, ColorRGBA{0.0, 0.0, 0.0, sh_base * 0.55});
        draw_rounded_rect(Rect{rect.x - 1.0, rect.y + 4.0, rect.w + 2.0, rect.h + 1.0}, radius + 1.5, ColorRGBA{0.0, 0.0, 0.0, sh_base * 0.85});
        draw_rounded_rect(Rect{rect.x, rect.y + 2.0, rect.w, rect.h}, radius, ColorRGBA{0.0, 0.0, 0.0, sh_base * 1.15});

    }

    const ColorRGBA bcol = (custom_border.a > 0.0) ? custom_border : theme.border_subtle;
    const ColorRGBA top_glass = theme.bg_card_top.with_alpha(elevated_shadow ? 0.90 : 0.96);
    const ColorRGBA bot_glass = theme.bg_card_bottom.with_alpha(elevated_shadow ? 0.93 : 0.98);
    draw_linear_gradient_rect(rect, radius, top_glass, bot_glass, true, bcol, border_width);

    // Inner frosted-glass rim glow (zero cairo_surface_flush stall, preserving >100 FPS Pango glyph cache)
    if (elevated_shadow && rect.w >= 48.0 && rect.h >= 36.0) {
        draw_rounded_rect(
            rect.inset(1.0, 1.0),
             std::max(1.0, radius - 1.0),
            ColorRGBA{0, 0, 0, 0},
            theme.border_specular.with_alpha(theme.is_dark ? 0.09 : 0.45),
            1.0
        );
    }

    // 1px Top Specular Light Catch
    if (rect.w > 24.0) {
        draw_line(
            rect.x + radius,
            rect.y + 1.0,
            rect.x + rect.w - radius,
            rect.y + 1.0,
            theme.border_specular,
            1.0
        );
    }

    // Semantic Top Accent Bar
    if (top_accent_stripe.a > 0.0 && rect.w > 24.0) {
        draw_rounded_rect(
            Rect{rect.x + 14.0, rect.y + 0.5, rect.w - 28.0, 2.6},
            1.3,
            top_accent_stripe
        );
    }
}

void VectorCanvas::draw_line(double x1, double y1, double x2, double y2, const ColorRGBA& color, double width) {
    if (!cr_) return;
    cairo_save(cr_);
    cairo_new_path(cr_);
    cairo_set_source_rgba(cr_, color.r, color.g, color.b, color.a);
    cairo_set_line_width(cr_, width);
    cairo_set_line_cap(cr_, CAIRO_LINE_CAP_ROUND);
    cairo_move_to(cr_, x1, y1);
    cairo_line_to(cr_, x2, y2);
    cairo_stroke(cr_);
    cairo_new_path(cr_);
    cairo_restore(cr_);
}

void VectorCanvas::draw_circle(double cx, double cy, double r, const ColorRGBA& fill, const ColorRGBA& stroke, double stroke_w) {
    if (!cr_ || r <= 0.0) return;
    cairo_save(cr_);
    cairo_new_path(cr_);
    cairo_new_sub_path(cr_);
    cairo_arc(cr_, cx, cy, r, 0.0, 2.0 * kPi);
    cairo_close_path(cr_);
    if (fill.a > 0.0) {
        cairo_set_source_rgba(cr_, fill.r, fill.g, fill.b, fill.a);
        if (stroke.a > 0.0 && stroke_w > 0.0) {
            cairo_fill_preserve(cr_);
        } else {
            cairo_fill(cr_);
        }
    }
    if (stroke.a > 0.0 && stroke_w > 0.0) {
        cairo_set_source_rgba(cr_, stroke.r, stroke.g, stroke.b, stroke.a);
        cairo_set_line_width(cr_, stroke_w);
        cairo_stroke(cr_);
    }
    cairo_new_path(cr_);
    cairo_restore(cr_);
}

void VectorCanvas::draw_vector_icon(VectorIcon icon, double cx, double cy, double size, const ColorRGBA& color) {
    if (!cr_ || size <= 2.0) return;
    const std::string hex = color_to_svg_hex(color);
    std::ostringstream svg;
    svg << "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'>";

    switch (icon) {
        case VectorIcon::Sparkles:
            svg << "<circle cx='12' cy='12' r='9' fill='" << hex << "' fill-opacity='0.18'/>"
                << "<path d='M12 2.5 L14.4 9.6 L21.5 12 L14.4 14.4 L12 21.5 L9.6 14.4 L2.5 12 L9.6 9.6 Z' "
                << "fill='" << hex << "' fill-opacity='0.28' stroke='" << hex << "' stroke-width='1.8' stroke-linejoin='round'/>";
            break;
        case VectorIcon::BookOpen:
            svg << "<path d='M2 5.5 C5 4.2, 9 4.2, 12 6.2 C15 4.2, 19 4.2, 22 5.5 V19.2 C19 18, 15 18, 12 20 C9 18, 5 18, 2 19.2 Z' "
                << "fill='" << hex << "' fill-opacity='0.20' stroke='" << hex << "' stroke-width='1.8' stroke-linejoin='round'/>"
                << "<line x1='12' y1='6.2' x2='12' y2='20' stroke='" << hex << "' stroke-width='1.8'/>";
            break;
        case VectorIcon::MicWaveform:
            svg << "<rect x='2' y='4' width='20' height='16' rx='5' fill='" << hex << "' fill-opacity='0.16'/>"
                << "<path d='M5 10 V14 M8.5 7 V17 M12 4.5 V19.5 M15.5 7.5 V16.5 M19 10 V14' "
                << "stroke='" << hex << "' stroke-width='2.1' stroke-linecap='round'/>";
            break;
        case VectorIcon::GitBranchNodes:
            svg << "<path d='M6 6 C6 12, 18 10, 18 17 M6 6 V18' fill='none' stroke='" << hex << "' stroke-width='1.9' stroke-linecap='round'/>"
                << "<circle cx='6' cy='6' r='3.2' fill='" << hex << "'/>"
                << "<circle cx='6' cy='18' r='3.0' fill='" << hex << "' fill-opacity='0.45' stroke='" << hex << "' stroke-width='1.6'/>"
                << "<circle cx='18' cy='17' r='3.2' fill='" << hex << "'/>";
            break;
        case VectorIcon::MessageChat:
            svg << "<path d='M3 6.5 C3 4.6, 4.6 3, 6.5 3 H17.5 C19.4 3, 21 4.6, 21 6.5 V14.5 C21 16.4, 19.4 18, 17.5 18 H9 L4.5 21 V18 C3.6 17.5, 3 16.2, 3 14.5 Z' "
                << "fill='" << hex << "' fill-opacity='0.20' stroke='" << hex << "' stroke-width='1.8' stroke-linejoin='round'/>"
                << "<line x1='7.5' y1='9' x2='16.5' y2='9' stroke='" << hex << "' stroke-width='1.7' stroke-linecap='round'/>"
                << "<line x1='7.5' y1='13' x2='13.5' y2='13' stroke='" << hex << "' stroke-width='1.7' stroke-linecap='round'/>";
            break;
        case VectorIcon::SearchMagnifier:
            svg << "<circle cx='10.5' cy='10.5' r='6.8' fill='" << hex << "' fill-opacity='0.18' stroke='" << hex << "' stroke-width='2.0'/>"
                << "<line x1='15.6' y1='15.6' x2='20.8' y2='20.8' stroke='" << hex << "' stroke-width='2.2' stroke-linecap='round'/>";
            break;
        case VectorIcon::ShieldCheck:
            svg << "<path d='M12 2.5 L19.5 5.8 V11.8 C19.5 16.6, 16.2 20.1, 12 21.6 C7.8 20.1, 4.5 16.6, 4.5 11.8 V5.8 Z' "
                << "fill='" << hex << "' fill-opacity='0.20' stroke='" << hex << "' stroke-width='1.8' stroke-linejoin='round'/>"
                << "<path d='M8.5 12.2 L11.0 14.7 L15.8 9.6' fill='none' stroke='" << hex << "' stroke-width='2.1' stroke-linecap='round' stroke-linejoin='round'/>";
            break;
        case VectorIcon::AlertTriangle:
            svg << "<path d='M12 3.2 L21.2 19.5 H2.8 Z' fill='" << hex << "' fill-opacity='0.22' stroke='" << hex << "' stroke-width='1.9' stroke-linejoin='round'/>"
                << "<line x1='12' y1='9.2' x2='12' y2='14.0' stroke='" << hex << "' stroke-width='2.1' stroke-linecap='round'/>"
                << "<circle cx='12' cy='16.9' r='1.2' fill='" << hex << "'/>";
            break;
        case VectorIcon::ClockLatency:
            svg << "<circle cx='12' cy='12' r='8.8' fill='" << hex << "' fill-opacity='0.18' stroke='" << hex << "' stroke-width='1.9'/>"
                << "<path d='M12 7.2 V12.2 L15.6 14.4' fill='none' stroke='" << hex << "' stroke-width='2.0' stroke-linecap='round' stroke-linejoin='round'/>";
            break;
        case VectorIcon::CpuBolt:
            svg << "<polygon points='13.2,2.5 4.8,13.2 11.4,13.2 10.6,21.5 19.2,10.8 12.6,10.8' "
                << "fill='" << hex << "' fill-opacity='0.26' stroke='" << hex << "' stroke-width='1.8' stroke-linejoin='round'/>";
            break;
    }
    svg << "</svg>";

    const Rect vp{cx - size * 0.5, cy - size * 0.5, size, size};
    draw_svg_string(svg.str(), vp);
}

void VectorCanvas::draw_text(
    double x,
    double y,
    const std::string& utf8_text,
    double font_size_px,
    const ColorRGBA& color,
    bool bold,
    const char* font_family
) {
    if (!cr_ || !layout_ || utf8_text.empty()) return;

    const std::string spec = build_pango_font_spec(font_family, bold, font_size_px);
    const int a_int = std::clamp(static_cast<int>(std::round(color.a * 255.0)), 0, 255);
    const std::string cache_key = spec + "|" + color_to_svg_hex(color) + ":" + std::to_string(a_int) + "|" + utf8_text;

    auto& s_text_sprite_cache = asset_cache().text_sprite_cache;
    auto it = s_text_sprite_cache.find(cache_key);
    if (it == s_text_sprite_cache.end()) {
        const PangoFontDescription* desc = get_cached_font_desc(spec);
        pango_layout_set_font_description(layout_, desc);
        pango_layout_set_text(layout_, utf8_text.c_str(), -1);
        int tw = 0, th = 0;
        pango_layout_get_pixel_size(layout_, &tw, &th);
        tw = std::max(1, tw + 2);
        th = std::max(1, th + 2);

        cairo_surface_t* sprite = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, tw, th);
        cairo_t* tcr = cairo_create(sprite);
        PangoLayout* tlay = pango_cairo_create_layout(tcr);
        pango_layout_set_font_description(tlay, desc);
        pango_layout_set_text(tlay, utf8_text.c_str(), -1);
        cairo_set_source_rgba(tcr, color.r, color.g, color.b, color.a);
        cairo_move_to(tcr, 0.0, 0.0);
        pango_cairo_show_layout(tcr, tlay);
        g_object_unref(tlay);
        cairo_destroy(tcr);

        if (s_text_sprite_cache.size() >= 4096) {
            for (auto& kv : s_text_sprite_cache) {
                cairo_surface_destroy(kv.second);
            }
            s_text_sprite_cache.clear();
        }
        it = s_text_sprite_cache.emplace(cache_key, sprite).first;
    }

    cairo_save(cr_);
    cairo_set_source_surface(cr_, it->second, std::round(x), std::round(y - font_size_px * 0.88));
    cairo_paint(cr_);
    cairo_restore(cr_);
}

double VectorCanvas::draw_wrapped_text(
    double x,
    double y,
    double max_width,
    double line_height,
    const std::string& utf8_text,
    double font_size_px,
    const ColorRGBA& color,
    bool bold,
    int max_lines,
    const char* font_family
) {
    if (utf8_text.empty() || max_width <= 20.0) return y;
    std::istringstream iss(utf8_text);
    std::string word;
    std::string current_line;
    int line_idx = 0;
    double cur_y = y;

    while (iss >> word) {
        const std::string candidate = current_line.empty() ? word : (current_line + " " + word);
        if (measure_text_width(candidate, font_size_px, bold, font_family) <= max_width || current_line.empty()) {
            current_line = candidate;
        } else {
            draw_text(x, cur_y, current_line, font_size_px, color, bold, font_family);
            ++line_idx;
            cur_y += line_height;
            if (line_idx >= max_lines) {
                return cur_y;
            }
            current_line = word;
        }
    }
    if (!current_line.empty() && line_idx < max_lines) {
        draw_text(x, cur_y, current_line, font_size_px, color, bold, font_family);
        cur_y += line_height;
    }
    return cur_y;
}

double VectorCanvas::measure_text_width(
    const std::string& utf8_text,
    double font_size_px,
    bool bold,
    const char* font_family
) const {
    if (!cr_ || !layout_ || utf8_text.empty()) return 0.0;
    const std::string spec = build_pango_font_spec(font_family, bold, font_size_px);
    const std::string key = spec + "|" + utf8_text;

    auto& s_width_cache = asset_cache().width_cache;
    auto it = s_width_cache.find(key);
    if (it != s_width_cache.end()) {
        return it->second;
    }

    const PangoFontDescription* desc = get_cached_font_desc(spec);
    pango_layout_set_font_description(layout_, desc);
    pango_layout_set_text(layout_, utf8_text.c_str(), -1);
    int w = 0, h = 0;
    pango_layout_get_pixel_size(layout_, &w, &h);
    const double res = static_cast<double>(w);
    if (s_width_cache.size() < 8192) {
        s_width_cache.emplace(key, res);
    }
    return res;
}

void VectorCanvas::draw_badge(
    double x,
    double y,
    const std::string& label,
    const ColorRGBA& bg_color,
    const ColorRGBA& text_color,
    double font_size_px
) {
    const double tw = measure_text_width(label, font_size_px, true, "Inter");
    const double pad_x = 8.5;
    const double h = font_size_px + 9.0;
    draw_rounded_rect(
        Rect{x, y, tw + pad_x * 2.0, h},
        h * 0.48,
        bg_color,
        text_color.with_alpha(0.36),
        1.0
    );
    draw_text(x + pad_x, y + font_size_px + 1.5, label, font_size_px, text_color, true, "Inter");
}

void VectorCanvas::draw_button(
    const Rect& rect,
    const std::string& label,
    const ThemePalette& theme,
    bool is_primary,
    bool is_active,
    double font_size_px,
    int leading_icon
) {
    if (!cr_ || rect.w <= 0 || rect.h <= 0) return;
    const double rad = std::min(10.0, rect.h * 0.28);

    if (is_primary) {
        // Subtle drop shadow under primary button
        draw_rounded_rect(Rect{rect.x, rect.y + 2.0, rect.w, rect.h}, rad, ColorRGBA{0, 0, 0, theme.is_dark ? 0.28 : 0.10});
        draw_linear_gradient_rect(rect, rad, theme.primary_accent, theme.primary_hover, false, theme.primary_hover, 1.2);
        // 1px top specular highlight
        draw_line(rect.x + rad, rect.y + 1.0, rect.x + rect.w - rad, rect.y + 1.0, ColorRGBA{1.0, 1.0, 1.0, 0.32}, 1.0);
    } else if (is_active) {
        draw_linear_gradient_rect(
            rect,
            rad,
            theme.primary_accent.with_alpha(0.22),
            theme.primary_accent.with_alpha(0.12),
            true,
            theme.primary_accent,
            1.4
        );
    } else {
        draw_linear_gradient_rect(
            rect,
            rad,
            theme.bg_surface_2,
            theme.bg_card_bottom,
            true,
            theme.border_subtle,
            1.0
        );
        draw_line(rect.x + rad, rect.y + 1.0, rect.x + rect.w - rad, rect.y + 1.0, theme.border_specular, 0.8);
    }

    const ColorRGBA txt_col = is_primary
        ? theme.text_on_primary()
        : (is_active ? theme.primary_accent : theme.text_primary);

    const double tw = measure_text_width(label, font_size_px, true, "Inter");
    const double icon_space = (leading_icon >= 0) ? 18.0 : 0.0;
    const double total_w = tw + icon_space;
    const double start_x = rect.x + std::max(8.0, (rect.w - total_w) * 0.5);
    const double baseline_y = rect.y + rect.h * 0.5 + font_size_px * 0.36;

    if (leading_icon >= 0) {
        draw_vector_icon(static_cast<VectorIcon>(leading_icon), start_x + 6.0, rect.y + rect.h * 0.5, 13.0, txt_col);
    }
    draw_text(start_x + icon_space, baseline_y, label, font_size_px, txt_col, true, "Inter");
}

void VectorCanvas::draw_progress_bar(
    const Rect& rect,
    double ratio,
    const ColorRGBA& track_color,
    const ColorRGBA& fill_color,
    const std::string& right_label,
    const ColorRGBA& label_color
) {
    const double clamped = std::clamp(ratio, 0.0, 1.0);
    draw_rounded_rect(rect, rect.h * 0.5, track_color, fill_color.with_alpha(0.22), 0.8);
    if (clamped > 0.01) {
        const Rect fill_r{rect.x, rect.y, std::max(rect.h, rect.w * clamped), rect.h};
        draw_linear_gradient_rect(
            fill_r,
            rect.h * 0.5,
            fill_color,
            ColorRGBA{
                std::min(1.0, fill_color.r + 0.14),
                std::min(1.0, fill_color.g + 0.14),
                std::min(1.0, fill_color.b + 0.14),
                fill_color.a
            },
            false
        );
    }
    if (!right_label.empty()) {
        const double tw = measure_text_width(right_label, 10.5, true, "Noto Sans Mono");
        draw_text(rect.x + std::max(0.0, rect.w - tw), rect.y - 7.0, right_label, 10.5, label_color, true, "Noto Sans Mono");
    }
}

void VectorCanvas::draw_photon_dial_gauge(
    double cx,
    double cy,
    double radius,
    double normalized_value,
    const std::string& center_value_text,
    const std::string& bottom_caption,
    const ColorRGBA& arc_color,
    const ThemePalette& theme
) {
    if (!cr_ || radius <= 8.0) return;
    const double clamped = std::clamp(normalized_value, 0.0, 1.0);
    const double start_angle = 0.75 * kPi;
    const double sweep_angle = 1.50 * kPi;

    cairo_save(cr_);
    // Subtle inner radial glow
    memory::ScopedCairoPattern glow(cairo_pattern_create_radial(cx, cy, 1.0, cx, cy, radius));
    cairo_pattern_add_color_stop_rgba(glow.get(), 0.0, arc_color.r, arc_color.g, arc_color.b, 0.14);
    cairo_pattern_add_color_stop_rgba(glow.get(), 1.0, arc_color.r, arc_color.g, arc_color.b, 0.0);
    cairo_set_source(cr_, glow.get());
    cairo_arc(cr_, cx, cy, radius, 0.0, 2.0 * kPi);
    cairo_fill(cr_);

    cairo_new_path(cr_);
    cairo_set_line_width(cr_, 5.0);
    cairo_set_line_cap(cr_, CAIRO_LINE_CAP_ROUND);
    cairo_set_source_rgba(cr_, theme.border_subtle.r, theme.border_subtle.g, theme.border_subtle.b, 0.65);
    cairo_arc(cr_, cx, cy, radius, start_angle, start_angle + sweep_angle);
    cairo_stroke(cr_);

    if (clamped > 0.01) {
        cairo_new_path(cr_);
        cairo_set_line_width(cr_, 5.5);
        cairo_set_source_rgba(cr_, arc_color.r, arc_color.g, arc_color.b, 0.98);
        cairo_arc(cr_, cx, cy, radius, start_angle, start_angle + sweep_angle * clamped);
        cairo_stroke(cr_);
    }
    cairo_new_path(cr_);
    cairo_restore(cr_);

    const double vw = measure_text_width(center_value_text, 11.5, true, "Noto Sans Mono");
    draw_text(cx - vw * 0.5, cy + 4.0, center_value_text, 11.5, theme.text_primary, true, "Noto Sans Mono");

    const double cw = measure_text_width(bottom_caption, 9.5, true, "Inter");
    draw_text(cx - cw * 0.5, cy + radius + 12.0, bottom_caption, 9.5, arc_color, true, "Inter");
}

void VectorCanvas::draw_fsrs_forgetting_curve_chart(
    const Rect& bounds,
    const domain::FsrsEngine& engine,
    const domain::LinguisticNode& node,
    const ThemePalette& theme
) {
    draw_photon_card(bounds, SpacingTokens::radius_md, theme, true, theme.border_subtle, 1.0, theme.primary_accent);

    const bool compact = (bounds.w < 480.0);
    draw_vector_icon(VectorIcon::Sparkles, bounds.x + 20.0, bounds.y + 20.0, 15.0, theme.primary_hover);
    draw_text(
        bounds.x + 34.0,
        bounds.y + 24.0,
        compact
            ? "FSRS-4.5 Forgetting Curves: R(t, S) = (1 + (19/81)·(t/S))^(-0.5)"
            : "FSRS-4.5 Power-Law Memory Decay Curves — R(t, S) = (1 + (19/81)·(t/S))^(-0.5)",
        compact ? 10.5 : 12.0,
        theme.text_primary,
        true,
        "Inter Display"
    );
    draw_text(
        bounds.x + 34.0,
        bounds.y + 40.0,
        compact
            ? ("Node: " + node.target_form + "  |  Dashed = 90% Retention Floor")
            : ("Active Node: " + node.target_form + " " + node.phonetic_ipa + "  |  Dashed = 90% Retention Floor"),
        10.0,
        theme.text_secondary,
        false,
        "Inter"
    );

    const double plot_x = bounds.x + 48.0;
    const double plot_y = bounds.y + 52.0;
    const double plot_w = bounds.w - 66.0;
    const double plot_h = bounds.h - (compact ? 96.0 : 82.0);
    if (plot_w <= 40 || plot_h <= 36) return;

    draw_rounded_rect(Rect{plot_x, plot_y, plot_w, plot_h}, 6.0, theme.bg_dock_chrome.with_alpha(0.65), theme.border_subtle.with_alpha(0.5), 0.8);

    for (int pct = 50; pct <= 100; pct += 10) {
        const double norm = static_cast<double>(pct - 50) / 50.0;
        const double y = plot_y + plot_h * (1.0 - norm);
        const ColorRGBA grid_col = (pct == 90)
            ? theme.secondary_accent.with_alpha(0.88)
            : theme.border_subtle.with_alpha(0.38);

        if (pct == 90) {
            cairo_save(cr_);
            const double dashes[2] = {6.0, 4.0};
            cairo_set_dash(cr_, dashes, 2, 0.0);
            draw_line(plot_x, y, plot_x + plot_w, y, grid_col, 1.5);
            cairo_restore(cr_);
        } else {
            draw_line(plot_x, y, plot_x + plot_w, y, grid_col, 0.8);
        }
        draw_text(bounds.x + 12.0, y + 4.0, std::to_string(pct) + "%", 9.5, (pct == 90) ? theme.secondary_accent : theme.text_muted, (pct == 90), "Noto Sans Mono");
    }

    const double max_days = 30.0;
    for (int d = 0; d <= 30; d += 5) {
        const double x = plot_x + plot_w * (static_cast<double>(d) / max_days);
        draw_line(x, plot_y, x, plot_y + plot_h, theme.border_subtle.with_alpha(0.30), 0.8);
        draw_text(x - 8.0, plot_y + plot_h + 14.0, std::to_string(d) + "d", 9.5, theme.text_muted, false, "Noto Sans Mono");
    }

    const std::array<ColorRGBA, 4> mod_colors = {
        theme.mod_rec_text,
        theme.mod_rec_audio,
        theme.mod_prod_text,
        theme.mod_prod_audio
    };
    const std::array<const char*, 4> mod_short_names = {
        "Rec-Text", "Rec-Audio", "Prod-Text", "Prod-Audio"
    };

    for (size_t m = 0; m < 4; ++m) {
        const double s = std::max(0.4, node.modalities[m].stability);
        const auto pts = engine.sample_forgetting_curve(s, max_days, 54);

        cairo_save(cr_);
        cairo_new_path(cr_);
        for (size_t i = 0; i < pts.size(); ++i) {
            const double px = plot_x + plot_w * (pts[i].first / max_days);
            const double clamped_r = std::clamp(pts[i].second, 0.50, 1.0);
            const double py = plot_y + plot_h * (1.0 - (clamped_r - 0.50) / 0.50);
            if (i == 0) cairo_move_to(cr_, px, py);
            else cairo_line_to(cr_, px, py);
        }
        cairo_line_to(cr_, plot_x + plot_w, plot_y + plot_h);
        cairo_line_to(cr_, plot_x, plot_y + plot_h);
        cairo_close_path(cr_);

        memory::ScopedCairoPattern area_pat(cairo_pattern_create_linear(plot_x, plot_y, plot_x, plot_y + plot_h));
        cairo_pattern_add_color_stop_rgba(area_pat.get(), 0.0, mod_colors[m].r, mod_colors[m].g, mod_colors[m].b, 0.22);
        cairo_pattern_add_color_stop_rgba(area_pat.get(), 1.0, mod_colors[m].r, mod_colors[m].g, mod_colors[m].b, 0.01);
        cairo_set_source(cr_, area_pat.get());
        cairo_fill(cr_);
        cairo_restore(cr_);

        cairo_save(cr_);
        cairo_new_path(cr_);
        cairo_set_source_rgba(cr_, mod_colors[m].r, mod_colors[m].g, mod_colors[m].b, 0.96);
        cairo_set_line_width(cr_, 2.3);
        for (size_t i = 0; i < pts.size(); ++i) {
            const double px = plot_x + plot_w * (pts[i].first / max_days);
            const double clamped_r = std::clamp(pts[i].second, 0.50, 1.0);
            const double py = plot_y + plot_h * (1.0 - (clamped_r - 0.50) / 0.50);
            if (i == 0) cairo_move_to(cr_, px, py);
            else cairo_line_to(cr_, px, py);
        }
        cairo_stroke(cr_);
        cairo_restore(cr_);

        if (s <= max_days) {
            const double ix = plot_x + plot_w * (s / max_days);
            const double iy = plot_y + plot_h * (1.0 - (0.90 - 0.50) / 0.50);
            draw_circle(ix, iy, 4.4, mod_colors[m], theme.bg_card_top, 1.5);
        }

        std::ostringstream lss;
        lss << mod_short_names[m] << " (" << std::fixed << std::setprecision(1) << node.modalities[m].stability << "d)";

        if (compact) {
            const double leg_x = plot_x + static_cast<double>(m % 2) * (plot_w * 0.52);
            const double leg_y = plot_y + plot_h + 27.0 + static_cast<double>(m / 2) * 15.0;
            draw_circle(leg_x + 4.0, leg_y - 4.0, 3.8, mod_colors[m]);
            draw_text(leg_x + 11.0, leg_y, lss.str(), 9.5, theme.text_secondary, true, "Inter");
        } else {
            const double leg_x = plot_x + static_cast<double>(m) * (plot_w * 0.25);
            const double leg_y = plot_y + plot_h + 28.0;
            draw_circle(leg_x + 4.0, leg_y - 4.0, 4.0, mod_colors[m]);
            draw_text(leg_x + 11.0, leg_y, lss.str(), 9.5, theme.text_secondary, true, "Inter");
        }
    }
}

void VectorCanvas::draw_four_modality_radar(
    const Rect& bounds,
    const domain::LinguisticNode& node,
    const ThemePalette& theme
) {
    draw_photon_card(bounds, SpacingTokens::radius_md, theme, true, theme.border_subtle, 1.0, theme.secondary_accent);
    draw_text(bounds.x + 16.0, bounds.y + 23.0, "4-Modality Procedural Radar", 12.0, theme.text_primary, true, "Inter Display");
    draw_text(bounds.x + 16.0, bounds.y + 39.0, "Outer Ring = 30d Automatization", 10.0, theme.text_secondary, false, "Inter");

    const double cx = bounds.x + bounds.w * 0.5;
    const double cy = bounds.y + 46.0 + (bounds.h - 54.0) * 0.5;
    const double max_r = std::min(bounds.w, bounds.h - 66.0) * 0.33;
    if (max_r <= 15.0) return;

    cairo_save(cr_);
    memory::ScopedCairoPattern rad(cairo_pattern_create_radial(cx, cy, 2.0, cx, cy, max_r));
    cairo_pattern_add_color_stop_rgba(rad.get(), 0.0, theme.primary_accent.r, theme.primary_accent.g, theme.primary_accent.b, 0.18);
    cairo_pattern_add_color_stop_rgba(rad.get(), 1.0, theme.primary_accent.r, theme.primary_accent.g, theme.primary_accent.b, 0.0);
    cairo_set_source(cr_, rad.get());
    cairo_arc(cr_, cx, cy, max_r, 0.0, 2.0 * kPi);
    cairo_fill(cr_);
    cairo_restore(cr_);

    for (int ring = 1; ring <= 4; ++ring) {
        const double r = max_r * (static_cast<double>(ring) / 4.0);
        draw_circle(cx, cy, r, ColorRGBA{0,0,0,0}, theme.border_subtle.with_alpha(0.55), 0.9);
    }

    const std::array<double, 4> angles = {-kPi * 0.5, 0.0, kPi * 0.5, kPi};
    const std::array<const char*, 4> labels = {"Rec-Text", "Rec-Audio", "Prod-Text", "Prod-Audio"};
    const std::array<ColorRGBA, 4> colors = {
        theme.mod_rec_text, theme.mod_rec_audio, theme.mod_prod_text, theme.mod_prod_audio
    };

    std::array<std::pair<double, double>, 4> poly_pts{};
    for (size_t i = 0; i < 4; ++i) {
        const double ax = cx + std::cos(angles[i]) * max_r;
        const double ay = cy + std::sin(angles[i]) * max_r;
        draw_line(cx, cy, ax, ay, theme.border_subtle.with_alpha(0.7), 1.0);

        const double s = node.modalities[i].stability;
        const double norm = std::clamp(std::log1p(s) / std::log1p(30.0), 0.10, 1.0);
        poly_pts[i] = {cx + std::cos(angles[i]) * (max_r * norm), cy + std::sin(angles[i]) * (max_r * norm)};

        const double lx = cx + std::cos(angles[i]) * (max_r + 28.0) - 26.0;
        const double ly = cy + std::sin(angles[i]) * (max_r + 15.0) + 4.0;
        draw_text(lx, ly, labels[i], 9.5, colors[i], true, "Inter");
    }

    cairo_save(cr_);
    cairo_new_path(cr_);
    for (size_t i = 0; i < 4; ++i) {
        if (i == 0) cairo_move_to(cr_, poly_pts[i].first, poly_pts[i].second);
        else cairo_line_to(cr_, poly_pts[i].first, poly_pts[i].second);
    }
    cairo_close_path(cr_);
    cairo_set_source_rgba(cr_, theme.primary_accent.r, theme.primary_accent.g, theme.primary_accent.b, 0.24);
    cairo_fill_preserve(cr_);
    cairo_set_source_rgba(cr_, theme.primary_accent.r, theme.primary_accent.g, theme.primary_accent.b, 0.96);
    cairo_set_line_width(cr_, 2.2);
    cairo_stroke(cr_);
    cairo_restore(cr_);

    for (size_t i = 0; i < 4; ++i) {
        draw_circle(poly_pts[i].first, poly_pts[i].second, 4.5, colors[i], theme.bg_card_top, 1.5);
    }
}

void VectorCanvas::draw_prosodic_pitch_contour(
    const Rect& bounds,
    const std::vector<domain::BackChainSegment>& segments,
    int active_step_index,
    const ThemePalette& theme
) {
    draw_photon_card(bounds, SpacingTokens::radius_md, theme, false, theme.border_subtle, 1.0, theme.secondary_accent);
    if (segments.empty()) return;

    const size_t idx = static_cast<size_t>(std::clamp(active_step_index, 0, static_cast<int>(segments.size()) - 1));
    const auto& seg = segments[idx];

    draw_vector_icon(VectorIcon::MicWaveform, bounds.x + 20.0, bounds.y + 19.0, 15.0, theme.secondary_accent);
    draw_text(
        bounds.x + 34.0,
        bounds.y + 23.0,
        "Prosodic Back-Chain Step " + std::to_string(seg.step_index) + "/" +
        std::to_string(segments.size()) + ": \"" + seg.syllable_slice + "\" " + seg.ipa_slice,
        12.0,
        theme.text_primary,
        true,
        "Inter Display"
    );
    draw_text(
        bounds.x + 34.0,
        bounds.y + 39.0,
        "Acoustic F0 Pitch Contour (100-260 Hz) + Waveform Envelope | Target Window: " + std::to_string(seg.target_duration_ms) + " ms",
        10.0,
        theme.text_secondary,
        false,
        "Inter"
    );

    const double px = bounds.x + 48.0;
    const double py = bounds.y + 46.0;
    const double pw = bounds.w - 64.0;
    const double ph = bounds.h - 58.0;
    if (pw <= 30 || ph <= 20 || seg.pitch_contour_hz.size() < 2) return;

    draw_rounded_rect(Rect{px, py, pw, ph}, 6.0, theme.bg_dock_chrome.with_alpha(0.75), theme.border_subtle.with_alpha(0.5), 0.8);

    const int num_bars = 36;
    const double bar_step = pw / static_cast<double>(num_bars);
    for (int b = 0; b < num_bars; ++b) {
        const double phase = static_cast<double>(b + active_step_index * 5) * 0.45;
        const double amp = 0.25 + 0.55 * std::abs(std::sin(phase)) * std::sin(kPi * (b + 0.5) / num_bars);
        const double bh = ph * amp * 0.65;
        const double bx = px + b * bar_step + 2.0;
        const double by = py + ph * 0.5 - bh * 0.5;
        draw_rounded_rect(
            Rect{bx, by, std::max(2.0, bar_step - 3.5), bh},
            1.5,
            theme.primary_accent.with_alpha(0.24)
        );
    }

    for (int hz : {120, 180, 240}) {
        const double norm = (static_cast<double>(hz) - 100.0) / 160.0;
        const double y = py + ph * (1.0 - norm);
        draw_line(px, y, px + pw, y, theme.border_subtle.with_alpha(0.42), 0.8);
        draw_text(bounds.x + 8.0, y + 4.0, std::to_string(hz) + "Hz", 9.5, theme.text_muted, false, "Noto Sans Mono");
    }

    cairo_save(cr_);
    cairo_new_path(cr_);
    for (size_t i = 0; i < seg.pitch_contour_hz.size(); ++i) {
        const double x = px + pw * (static_cast<double>(i) / static_cast<double>(seg.pitch_contour_hz.size() - 1));
        const double norm = std::clamp((seg.pitch_contour_hz[i] - 100.0) / 160.0, 0.0, 1.0);
        const double y = py + ph * (1.0 - norm);
        if (i == 0) cairo_move_to(cr_, x, y);
        else cairo_line_to(cr_, x, y);
    }
    cairo_line_to(cr_, px + pw, py + ph);
    cairo_line_to(cr_, px, py + ph);
    cairo_close_path(cr_);
    memory::ScopedCairoPattern f0_pat(cairo_pattern_create_linear(px, py, px, py + ph));
    cairo_pattern_add_color_stop_rgba(f0_pat.get(), 0.0, theme.secondary_accent.r, theme.secondary_accent.g, theme.secondary_accent.b, 0.30);
    cairo_pattern_add_color_stop_rgba(f0_pat.get(), 1.0, theme.secondary_accent.r, theme.secondary_accent.g, theme.secondary_accent.b, 0.02);
    cairo_set_source(cr_, f0_pat.get());
    cairo_fill(cr_);
    cairo_restore(cr_);

    cairo_save(cr_);
    cairo_new_path(cr_);
    cairo_set_source_rgba(cr_, theme.secondary_accent.r, theme.secondary_accent.g, theme.secondary_accent.b, 0.98);
    cairo_set_line_width(cr_, 2.8);
    for (size_t i = 0; i < seg.pitch_contour_hz.size(); ++i) {
        const double x = px + pw * (static_cast<double>(i) / static_cast<double>(seg.pitch_contour_hz.size() - 1));
        const double norm = std::clamp((seg.pitch_contour_hz[i] - 100.0) / 160.0, 0.0, 1.0);
        const double y = py + ph * (1.0 - norm);
        if (i == 0) cairo_move_to(cr_, x, y);
        else cairo_line_to(cr_, x, y);
    }
    cairo_stroke(cr_);
    cairo_restore(cr_);

    for (size_t i = 0; i < seg.pitch_contour_hz.size(); ++i) {
        const double x = px + pw * (static_cast<double>(i) / static_cast<double>(seg.pitch_contour_hz.size() - 1));
        const double norm = std::clamp((seg.pitch_contour_hz[i] - 100.0) / 160.0, 0.0, 1.0);
        const double y = py + ph * (1.0 - norm);
        draw_circle(x, y, 4.2, theme.secondary_accent, theme.bg_card_top, 1.5);
    }
}

void VectorCanvas::draw_dependency_graph_panel(
    const Rect& bounds,
    const domain::KnowledgeGraph& graph,
    const std::string& language_code,
    domain::NodeId selected_node_id,
    const ThemePalette& theme,
    std::vector<std::pair<Rect, domain::NodeId>>& out_clickable_nodes
) {
    out_clickable_nodes.clear();
    draw_photon_card(bounds, SpacingTokens::radius_md, theme, true, theme.border_subtle, 1.0, theme.primary_accent);

    draw_vector_icon(VectorIcon::GitBranchNodes, bounds.x + 22.0, bounds.y + 20.0, 15.0, theme.primary_hover);
    draw_text(
        bounds.x + 36.0,
        bounds.y + 24.0,
        "WaniKani Prerequisite Dependency DAG — Productive Unlock Gate (S_rec >= 3.0 Days)",
        12.0,
        theme.text_primary,
        true,
        "Inter Display"
    );
    draw_text(
        bounds.x + 36.0,
        bounds.y + 41.0,
        "Select any node to inspect its 4-Modality Photon Dials & FSRS-4.5 Decay Curves.",
        10.0,
        theme.text_secondary,
        false,
        "Inter"
    );

    const auto nodes = graph.nodes_for_language(language_code);
    if (nodes.empty()) return;

    struct PositionedNode {
        domain::LinguisticNode node;
        Rect box;
    };
    std::vector<PositionedNode> positioned;

    const double start_x = bounds.x + 18.0;
    const double start_y = bounds.y + 56.0;
    const double card_w = (bounds.w - 60.0) / 3.0;
    const double card_h = std::min(92.0, (bounds.h - 76.0) / 2.2);

    for (size_t i = 0; i < nodes.size(); ++i) {
        const size_t col = i % 3;
        const size_t row = i / 3;
        const double nx = start_x + static_cast<double>(col) * (card_w + 12.0);
        const double ny = start_y + static_cast<double>(row) * (card_h + 18.0);
        positioned.push_back(PositionedNode{nodes[i], Rect{nx, ny, card_w, card_h}});
    }

    for (const auto& target_pn : positioned) {
        for (domain::NodeId pre_id : target_pn.node.prerequisites) {
            for (const auto& src_pn : positioned) {
                if (src_pn.node.id == pre_id) {
                    const double x1 = src_pn.box.x + src_pn.box.w * 0.5;
                    const double y1 = src_pn.box.y + src_pn.box.h * 0.5;
                    const double x2 = target_pn.box.x + target_pn.box.w * 0.5;
                    const double y2 = target_pn.box.y + target_pn.box.h * 0.5;
                    const bool unlocked = graph.is_productive_unlocked(target_pn.node.id);
                    const ColorRGBA edge_col = unlocked ? theme.status_success.with_alpha(0.75) : theme.status_warning.with_alpha(0.75);

                    cairo_save(cr_);
                    cairo_new_path(cr_);
                    cairo_set_source_rgba(cr_, edge_col.r, edge_col.g, edge_col.b, edge_col.a);
                    cairo_set_line_width(cr_, 2.2);
                    cairo_move_to(cr_, x1, y1);
                    cairo_curve_to(cr_, (x1 + x2) * 0.5, y1, (x1 + x2) * 0.5, y2, x2, y2);
                    cairo_stroke(cr_);
                    cairo_restore(cr_);
                }
            }
        }
    }

    for (const auto& pn : positioned) {
        const bool is_sel = (pn.node.id == selected_node_id);
        const bool prod_ok = graph.is_productive_unlocked(pn.node.id);
        const auto leech = graph.evaluate_leech_intervention(pn.node.id, domain::Modality::ProductiveText);

        ColorRGBA border = is_sel ? theme.border_focus : theme.border_subtle;
        if (leech.is_leech) border = theme.status_danger;

        const ColorRGBA stripe = leech.is_leech
            ? theme.status_danger
            : (prod_ok ? theme.status_success : theme.status_warning);

        draw_photon_card(
            pn.box,
            SpacingTokens::radius_sm,
            theme,
            is_sel,
            border,
            is_sel ? 2.0 : 1.0,
            stripe
        );

        draw_text(
            pn.box.x + 12.0,
            pn.box.y + 22.0,
            "#" + std::to_string(pn.node.id) + " " + pn.node.target_form,
            11.5,
            theme.text_primary,
            true,
            "Inter"
        );

        const std::string gate_badge = leech.is_leech
            ? "! LEECH ACTIVE"
            : (prod_ok ? "✓ PROD UNLOCKED" : "⏳ REC GATE <3d");

        draw_badge(
            pn.box.x + 12.0,
            pn.box.y + 29.0,
            gate_badge,
            stripe.with_alpha(0.18),
            stripe,
            9.0
        );

        std::ostringstream s_line;
        s_line << "S_rec:" << std::fixed << std::setprecision(1)
               << pn.node.modalities[0].stability << "d  S_prod:"
               << pn.node.modalities[2].stability << "d";
        draw_text(pn.box.x + 12.0, pn.box.y + pn.box.h - 10.0, s_line.str(), 9.5, theme.text_secondary, false, "Noto Sans Mono");

        out_clickable_nodes.emplace_back(pn.box, pn.node.id);
    }
}

} // namespace kinetix::ui
