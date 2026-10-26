#pragma once
// Kinetix L2 v3.0 — Cycfi Photon / Elements Declarative Micro-GUI Composition Layer
// Inspired by Joel de Guzman's Photon Micro-GUI / Elements C++ library
// (https://www.cycfi.com/2019/07/photon-micro-gui/ & https://github.com/cycfi/elements):
// - Lightweight value-type declarative layout combinators (`vtile`, `htile`, `margin`, `inset`)
// - `view_limits` constraint solver (`min`, `max`, `flex`)
// - Supports both Consumer Focus Studio (`show_bottom_console == false`) and
//   Pro 6-Region Telemetry Dockspace (`show_bottom_console == true`)

#include <algorithm>
#include <array>
#include <cstddef>
#include <vector>

namespace kinetix::photon {

constexpr double kFullExtent = 1e6;

struct view_limits {
    double min_w = 0.0;
    double min_h = 0.0;
    double max_w = kFullExtent;
    double max_h = kFullExtent;
};

struct box_rect {
    double x = 0.0;
    double y = 0.0;
    double w = 0.0;
    double h = 0.0;

    constexpr bool contains(double px, double py) const noexcept {
        return px >= x && px <= (x + w) && py >= y && py <= (y + h);
    }

    constexpr box_rect inset(double left, double top, double right, double bottom) const noexcept {
        return box_rect{
            x + left,
            y + top,
            std::max(0.0, w - (left + right)),
            std::max(0.0, h - (top + bottom))
        };
    }

    constexpr box_rect inset(double h_pad, double v_pad) const noexcept {
        return inset(h_pad, v_pad, h_pad, v_pad);
    }

    constexpr box_rect margin(double uniform_pad) const noexcept {
        return inset(uniform_pad, uniform_pad, uniform_pad, uniform_pad);
    }
};

// Declarative Horizontal Tile Combinator (`htile`) with proportional or fixed weights
template <size_t N>
constexpr std::array<box_rect, N> htile(
    const box_rect& bounds,
    const std::array<double, N>& weights,
    double gap = 14.0
) noexcept {
    std::array<box_rect, N> out{};
    double total_weight = 0.0;
    for (double w : weights) total_weight += std::max(0.001, w);

    const double usable_w = std::max(0.0, bounds.w - gap * static_cast<double>(N > 0 ? N - 1 : 0));
    double cursor_x = bounds.x;
    for (size_t i = 0; i < N; ++i) {
        const double slice_w = usable_w * (std::max(0.001, weights[i]) / total_weight);
        out[i] = box_rect{cursor_x, bounds.y, slice_w, bounds.h};
        cursor_x += slice_w + gap;
    }
    return out;
}

// Declarative Vertical Tile Combinator (`vtile`) with proportional weights
template <size_t N>
constexpr std::array<box_rect, N> vtile(
    const box_rect& bounds,
    const std::array<double, N>& weights,
    double gap = 14.0
) noexcept {
    std::array<box_rect, N> out{};
    double total_weight = 0.0;
    for (double w : weights) total_weight += std::max(0.001, w);

    const double usable_h = std::max(0.0, bounds.h - gap * static_cast<double>(N > 0 ? N - 1 : 0));
    double cursor_y = bounds.y;
    for (size_t i = 0; i < N; ++i) {
        const double slice_h = usable_h * (std::max(0.001, weights[i]) / total_weight);
        out[i] = box_rect{bounds.x, cursor_y, bounds.w, slice_h};
        cursor_y += slice_h + gap;
    }
    return out;
}

struct WorkspaceDockLayout {
    box_rect command_bar;       // Region 1: Top studio header with official SVG brand lockup & controls
    box_rect left_nav_pane;     // Region 2: Left studio navigation & corpus rail
    box_rect center_work_area;  // Region 3: Primary interactive studio canvas
    box_rect right_inspector;   // Region 4: Selected node mastery & FSRS-4.5 inspector card
    box_rect bottom_console;    // Region 5: On-demand Pro Telemetry Console (Revlog + SoA Profiler)
    box_rect status_bar;        // Region 6: Editorial 4-modality brand track bar & status footer
};

inline WorkspaceDockLayout compute_desktop_dockspace(double width, double height, bool show_bottom_console = false) noexcept {
    WorkspaceDockLayout d{};
    const double cmd_h = 56.0;
    const double status_h = 30.0;
    d.command_bar = box_rect{0.0, 0.0, width, cmd_h};
    d.status_bar = box_rect{0.0, height - status_h, width, status_h};

    const double body_y = cmd_h;
    const double body_h = std::max(100.0, height - cmd_h - status_h);
    const double left_w = 242.0;
    const double right_w = 324.0;
    const double center_w = std::max(200.0, width - left_w - right_w);

    d.left_nav_pane = box_rect{0.0, body_y, left_w, body_h};
    d.right_inspector = box_rect{width - right_w, body_y, right_w, body_h};

    if (show_bottom_console) {
        const double console_h = 156.0;
        const double main_h = std::max(100.0, body_h - console_h);
        d.center_work_area = box_rect{left_w, body_y, center_w, main_h}.margin(12.0);
        d.bottom_console = box_rect{left_w, body_y + main_h, center_w, console_h}.inset(12.0, 0.0, 12.0, 8.0);
    } else {
        d.center_work_area = box_rect{left_w, body_y, center_w, body_h}.margin(14.0);
        d.bottom_console = box_rect{0, 0, 0, 0};
    }
    return d;
}

} // namespace kinetix::photon
