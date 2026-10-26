#pragma once
// Kinetix L2 v3.0 — PangoCairo + librsvg + Photon Frosted-Glass Shader Vector Canvas Engine
// Renders:
// 1. Official Kinetix L2 SVG Brand Assets & multi-layer duotone SVG icons via `librsvg-2.so.2`
// 2. C++20 Separable Gaussian Blur, Frosted-Glass (Acrylic/Mica) card backdrops, Ambient Radial Mesh Glows,
//    and 1.4% perceptual surface noise grain directly on the ARGB32 surface buffer
// 3. HarfBuzz-shaped Inter, Inter Display, Noto Sans Mono, and Noto Sans CJK JP typography
// 4. Photon circular `dial_gauge` rings and ImPlot-style scientific curves

#include "kinetix/ui/design_tokens.hpp"
#include "kinetix/ui/photon_elements.hpp"
#include "kinetix/domain/types.hpp"
#include "kinetix/domain/fsrs_engine.hpp"
#include "kinetix/domain/knowledge_graph.hpp"
#include "kinetix/domain/pedagogical_loop.hpp"
#include "third_party/cairo_abi.h"
#include "third_party/pango_abi.h"
#include "third_party/rsvg_abi.h"
#include <string>
#include <utility>
#include <vector>

namespace kinetix::ui {

using Rect = photon::box_rect;

enum class VectorIcon : int {
    Sparkles = 0,
    BookOpen = 1,
    MicWaveform = 2,
    GitBranchNodes = 3,
    MessageChat = 4,
    SearchMagnifier = 5,
    ShieldCheck = 6,
    AlertTriangle = 7,
    ClockLatency = 8,
    CpuBolt = 9
};

class VectorCanvas {
public:
    VectorCanvas(int width, int height);
    ~VectorCanvas();

    VectorCanvas(const VectorCanvas&) = delete;
    VectorCanvas& operator=(const VectorCanvas&) = delete;

    void resize(int width, int height);
    int width() const noexcept { return width_; }
    int height() const noexcept { return height_; }
    int stride() const noexcept;
    const unsigned char* pixels() const noexcept;

    bool export_png(const std::string& filepath) const;

    // 1. Canvas, Ambient Mesh Shader & Pixel-Buffer Post-Processing Primitives
    void clear_gradient(const ColorRGBA& top_color, const ColorRGBA& bottom_color);
    void clear(const ColorRGBA& color);
    void draw_ambient_mesh_backdrop(const ThemePalette& theme);
    void apply_gaussian_blur_region(const Rect& region, int radius);
    void apply_perceptual_noise_grain(const Rect& region, double intensity = 0.012);

    // 2. Native SVG Brand & Duotone Vector Asset Pipeline (`librsvg-2.so.2`)
    bool draw_svg_file(const std::string& svg_path, const Rect& viewport);
    bool draw_svg_string(const std::string& svg_xml, const Rect& viewport);
    void draw_brand_logo(double x, double y, double height, bool is_dark_theme, bool compact_mark_only = false);

    // 3. Photon Frosted-Glass & Geometric Surface Primitives
    void draw_rounded_rect(
        const Rect& rect,
        double radius,
        const ColorRGBA& fill_color,
        const ColorRGBA& border_color = ColorRGBA{0, 0, 0, 0},
        double border_width = 1.0
    );
    // Photon Frosted-Glass Card: Backdrop Gaussian Blur + Multi-pass Soft Shadow + Linear Surface Gradient + Specular Rim + Noise Grain
    void draw_photon_card(
        const Rect& rect,
        double radius,
        const ThemePalette& theme,
        bool elevated_shadow = true,
        const ColorRGBA& custom_border = ColorRGBA{0, 0, 0, 0},
        double border_width = 1.0,
        const ColorRGBA& top_accent_stripe = ColorRGBA{0, 0, 0, 0}
    );
    void draw_linear_gradient_rect(
        const Rect& rect,
        double radius,
        const ColorRGBA& c0,
        const ColorRGBA& c1,
        bool vertical = true,
        const ColorRGBA& border = ColorRGBA{0, 0, 0, 0},
        double border_w = 1.0
    );
    void draw_line(double x1, double y1, double x2, double y2, const ColorRGBA& color, double width = 1.5);
    void draw_circle(double cx, double cy, double r, const ColorRGBA& fill, const ColorRGBA& stroke = ColorRGBA{0,0,0,0}, double stroke_w = 1.0);
    void draw_vector_icon(VectorIcon icon, double cx, double cy, double size, const ColorRGBA& color);

    // 4. PangoCairo + HarfBuzz Shaped Typography Primitives (Inter, Inter Display, Noto Sans Mono, Noto Sans CJK JP)
    void draw_text(
        double x,
        double y,
        const std::string& utf8_text,
        double font_size_px,
        const ColorRGBA& color,
        bool bold = false,
        const char* font_family = "Inter"
    );
    double draw_wrapped_text(
        double x,
        double y,
        double max_width,
        double line_height,
        const std::string& utf8_text,
        double font_size_px,
        const ColorRGBA& color,
        bool bold = false,
        int max_lines = 3,
        const char* font_family = "Inter"
    );
    double measure_text_width(
        const std::string& utf8_text,
        double font_size_px,
        bool bold = false,
        const char* font_family = "Inter"
    ) const;
    void draw_badge(
        double x,
        double y,
        const std::string& label,
        const ColorRGBA& bg_color,
        const ColorRGBA& text_color,
        double font_size_px = 10.5
    );
    void draw_button(
        const Rect& rect,
        const std::string& label,
        const ThemePalette& theme,
        bool is_primary = false,
        bool is_active = false,
        double font_size_px = 11.0,
        int leading_icon = -1
    );
    void draw_progress_bar(
        const Rect& rect,
        double ratio,
        const ColorRGBA& track_color,
        const ColorRGBA& fill_color,
        const std::string& right_label = "",
        const ColorRGBA& label_color = ColorRGBA{1,1,1,1}
    );
    // Photon Micro-GUI Circular Dial Gauge (`dial_gauge`)
    void draw_photon_dial_gauge(
        double cx,
        double cy,
        double radius,
        double normalized_value,
        const std::string& center_value_text,
        const std::string& bottom_caption,
        const ColorRGBA& arc_color,
        const ThemePalette& theme
    );

    // 5. ImPlot / Photon Scientific Visualizations
    void draw_fsrs_forgetting_curve_chart(
        const Rect& bounds,
        const domain::FsrsEngine& engine,
        const domain::LinguisticNode& node,
        const ThemePalette& theme
    );
    void draw_four_modality_radar(
        const Rect& bounds,
        const domain::LinguisticNode& node,
        const ThemePalette& theme
    );
    void draw_prosodic_pitch_contour(
        const Rect& bounds,
        const std::vector<domain::BackChainSegment>& segments,
        int active_step_index,
        const ThemePalette& theme
    );
    void draw_dependency_graph_panel(
        const Rect& bounds,
        const domain::KnowledgeGraph& graph,
        const std::string& language_code,
        domain::NodeId selected_node_id,
        const ThemePalette& theme,
        std::vector<std::pair<Rect, domain::NodeId>>& out_clickable_nodes
    );

private:
    void trace_rounded_rect_path(const Rect& rect, double radius) const;
    int width_ = 0;
    int height_ = 0;
    cairo_surface_t* surface_ = nullptr;
    cairo_t* cr_ = nullptr;
    PangoLayout* layout_ = nullptr;
    cairo_surface_t* backdrop_cache_ = nullptr;
    int backdrop_cache_w_ = 0;
    int backdrop_cache_h_ = 0;
    int backdrop_cache_dark_ = -1;
};

} // namespace kinetix::ui
