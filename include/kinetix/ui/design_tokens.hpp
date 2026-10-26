#pragma once
// Kinetix L2 v3.0 — Official Brand Identity & Photon Frosted-Glass Design Token System
// Harmonized with the official Kinetix L2 SVG Brand Kit:
// - Dark Editorial Slate-Teal (#16272B), Warm Archival Ivory (#F4EFE6),
//   Kinetic Mint (#91D1B5), and Terracotta Coral (#EE9A7C).
// - Light Warm Archival Ivory (#F5F0E6), Deep Slate-Teal (#16272B),
//   Deep Kinetic Emerald (#1C7B5F), and Terracotta Rust (#B6573D).

#include <cstdint>

namespace kinetix::ui {

struct ColorRGBA {
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;
    double a = 1.0;

    static constexpr ColorRGBA from_hex(uint32_t rgb, double alpha = 1.0) noexcept {
        return ColorRGBA{
            static_cast<double>((rgb >> 16) & 0xFF) / 255.0,
            static_cast<double>((rgb >> 8) & 0xFF) / 255.0,
            static_cast<double>(rgb & 0xFF) / 255.0,
            alpha
        };
    }

    constexpr ColorRGBA with_alpha(double new_alpha) const noexcept {
        return ColorRGBA{r, g, b, new_alpha};
    }
};

struct ThemePalette {
    bool is_dark = true;

    // Window & Studio Layered Surfaces
    ColorRGBA bg_canvas_top;
    ColorRGBA bg_canvas_bottom;
    ColorRGBA bg_dock_chrome;
    ColorRGBA bg_card_top;
    ColorRGBA bg_card_bottom;
    ColorRGBA bg_surface_2;
    ColorRGBA bg_elevated;

    // Borders & Specular Lighting (Frosted-Glass & Photon Micro-GUI edge)
    ColorRGBA border_subtle;
    ColorRGBA border_specular;     // Top 1px inner light catch
    ColorRGBA border_focus;

    // Typography (Shaped via PangoCairo with Inter, Inter Display & Noto Sans CJK)
    ColorRGBA text_primary;
    ColorRGBA text_secondary;
    ColorRGBA text_muted;

    // Official Kinetix L2 Brand Accents (Kinetic Mint + Terracotta Coral L2)
    ColorRGBA primary_accent;
    ColorRGBA primary_hover;
    ColorRGBA secondary_accent;

    // Semantic Status & FSRS Tiers
    ColorRGBA status_success;
    ColorRGBA status_warning;
    ColorRGBA status_danger;
    ColorRGBA status_unseen;

    // 4-Modality Distinct Series Colors (Harmonized with Kinetix L2 Brand Kit)
    ColorRGBA mod_rec_text;
    ColorRGBA mod_rec_audio;
    ColorRGBA mod_prod_text;
    ColorRGBA mod_prod_audio;

    // High-contrast ink color on primary_accent buttons (#16272B on Mint #91D1B5, #F4EFE6 on Emerald #1C7B5F)
    constexpr ColorRGBA text_on_primary() const noexcept {
        return is_dark ? ColorRGBA::from_hex(0x112024) : ColorRGBA::from_hex(0xF5F0E6);
    }

    static constexpr ThemePalette dark_obsidian() noexcept {
        return ThemePalette{
            true,
            ColorRGBA::from_hex(0x0D181B),          // bg_canvas_top (Deep Editorial Slate-Teal)
            ColorRGBA::from_hex(0x122125),          // bg_canvas_bottom
            ColorRGBA::from_hex(0x101D20),          // bg_dock_chrome
            ColorRGBA::from_hex(0x172A2E),          // bg_card_top (Official #16272B elevated)
            ColorRGBA::from_hex(0x132327),          // bg_card_bottom
            ColorRGBA::from_hex(0x1D3338),          // bg_surface_2
            ColorRGBA::from_hex(0x254046),          // bg_elevated
            ColorRGBA::from_hex(0x2B474E),          // border_subtle
            ColorRGBA::from_hex(0xF4EFE6, 0.16),    // border_specular (Warm Ivory top catch)
            ColorRGBA::from_hex(0x91D1B5),          // border_focus (Kinetic Mint)
            ColorRGBA::from_hex(0xF4EFE6),          // text_primary (Warm Archival Ivory 15.6:1 AAA)
            ColorRGBA::from_hex(0xC9D8D4),          // text_secondary (Soft Sage-Ivory 11.1:1 AAA)
            ColorRGBA::from_hex(0x8FA9A4),          // text_muted (Editorial Sage 5.8:1 AA)
            ColorRGBA::from_hex(0x91D1B5),          // primary_accent (Official Kinetic Mint #91D1B5)
            ColorRGBA::from_hex(0xA8DFC6),          // primary_hover
            ColorRGBA::from_hex(0xEE9A7C),          // secondary_accent (Official Terracotta Coral #EE9A7C)
            ColorRGBA::from_hex(0x91D1B5),          // status_success (Kinetic Mint)
            ColorRGBA::from_hex(0xF3B664),          // status_warning (Warm Amber Gold)
            ColorRGBA::from_hex(0xEE9A7C),          // status_danger (Terracotta Coral)
            ColorRGBA::from_hex(0x7CC6D6),          // status_unseen (Glacier Teal)
            ColorRGBA::from_hex(0x7CC6D6),          // mod_rec_text (Glacier Teal)
            ColorRGBA::from_hex(0x91D1B5),          // mod_rec_audio (Kinetic Mint)
            ColorRGBA::from_hex(0xF3B664),          // mod_prod_text (Warm Amber)
            ColorRGBA::from_hex(0xEE9A7C)           // mod_prod_audio (Terracotta Coral)
        };
    }

    static constexpr ThemePalette light_archival() noexcept {
        return ThemePalette{
            false,
            ColorRGBA::from_hex(0xF5F0E6),          // bg_canvas_top (Official Brand Preview Cream #F5F0E6)
            ColorRGBA::from_hex(0xEDE8DD),          // bg_canvas_bottom
            ColorRGBA::from_hex(0xE8E4DB),          // bg_dock_chrome (Official Showcase Card #E8E4DB)
            ColorRGBA::from_hex(0xFCF9F2),          // bg_card_top
            ColorRGBA::from_hex(0xF3EFE6),          // bg_card_bottom (Official Light Icon Base #F3EFE6)
            ColorRGBA::from_hex(0xE8E4DB),          // bg_surface_2
            ColorRGBA::from_hex(0xDDD8CC),          // bg_elevated
            ColorRGBA::from_hex(0xCFC9BC),          // border_subtle
            ColorRGBA::from_hex(0xFFFFFF, 0.90),    // border_specular
            ColorRGBA::from_hex(0x1C7B5F),          // border_focus (Official Light Kinetic Emerald)
            ColorRGBA::from_hex(0x16272B),          // text_primary (Official Deep Slate-Teal 14.9:1 AAA)
            ColorRGBA::from_hex(0x2F464B),          // text_secondary (9.2:1 AAA)
            ColorRGBA::from_hex(0x526B70),          // text_muted (5.4:1 AA)
            ColorRGBA::from_hex(0x1C7B5F),          // primary_accent (Official Deep Kinetic Emerald #1C7B5F)
            ColorRGBA::from_hex(0x15634C),          // primary_hover
            ColorRGBA::from_hex(0xB6573D),          // secondary_accent (Official Terracotta Rust #B6573D)
            ColorRGBA::from_hex(0x1C7B5F),          // status_success
            ColorRGBA::from_hex(0xC87A19),          // status_warning
            ColorRGBA::from_hex(0xB6573D),          // status_danger
            ColorRGBA::from_hex(0x1E6F8C),          // status_unseen
            ColorRGBA::from_hex(0x1E6F8C),          // mod_rec_text
            ColorRGBA::from_hex(0x1C7B5F),          // mod_rec_audio
            ColorRGBA::from_hex(0xC87A19),          // mod_prod_text
            ColorRGBA::from_hex(0xB6573D)           // mod_prod_audio
        };
    }
};

struct SpacingTokens {
    static constexpr double xs  = 4.0;
    static constexpr double sm  = 8.0;
    static constexpr double md  = 14.0;
    static constexpr double lg  = 20.0;
    static constexpr double xl  = 28.0;
    static constexpr double xxl = 40.0;
    static constexpr double radius_sm = 8.0;
    static constexpr double radius_md = 12.0;
    static constexpr double radius_lg = 16.0;
    static constexpr double min_touch_target = 44.0;
};

struct TypographyTokens {
    static constexpr double micro   = 9.5;
    static constexpr double caption = 10.5;
    static constexpr double small   = 11.5;
    static constexpr double body    = 13.0;
    static constexpr double lead    = 15.5;
    static constexpr double h3      = 18.5;
    static constexpr double h2      = 23.0;
    static constexpr double h1      = 30.0;
};

} // namespace kinetix::ui
