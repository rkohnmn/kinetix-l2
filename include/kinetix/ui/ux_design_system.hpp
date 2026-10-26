#pragma once
// Kinetix L2 v3.1 — Native C++20 UX Research, Design System & Visual Quality Engineering
// Implements Workflows 1–12 from `native-cpp-ux-product-design.md`:
// - Workflow 1 & 4: Normalized `ResearchRecord`, `EvidenceTag`, and `BehavioralArchetype` models
// - Workflow 2: User Journey Mapping & Opportunity Priority Heuristic (`F * S * I * C`)
// - Workflow 3: Usability Issue Severity Scale (1=Minor .. 4=Critical)
// - Workflow 5 & 9: Centralized `SemanticDesignTokens`, `ControlState` machine, and WCAG 2.2 relative luminance contrast verifier
// - Workflow 8: Purposeful Motion & `ReducedMotion` accessibility compliance
// - Workflow 10: Heuristic Written & Visual Quality Audit Scoring Model

#include "kinetix/ui/design_tokens.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace kinetix::ui::ux {

// Workflow 1: Behavioral Archetypes grounded in observed task patterns
enum class BehavioralArchetype : uint8_t {
    PowerUser = 0,              // High-frequency keyboard shortcuts, batch review, Pro Telemetry
    OccasionalUser = 1,         // Needs clear stopping cues, Resilient Vacation Shift, Focus Studio
    ProfessionalOperator = 2,   // Repeatable daily certification prep (CEFR B1/B2, JLPT N4/N3)
    TechnicalSpecialist = 3,    // Inspects FSRS-4.5 decay curves, WaniKani DAG, CRDT revlog
    AccessibilityDependent = 4  // Keyboard-only focus rings, WCAG AAA contrast, Reduced Motion
};

// Workflow 4: Evidence Coding Tags
enum class EvidenceTag : uint8_t {
    Goal = 0,          // [GOAL]
    Pain = 1,          // [PAIN]
    Behavior = 2,      // [BEHAVIOR]
    Context = 3,       // [CONTEXT]
    Quote = 4,         // [QUOTE]
    Error = 5,         // [ERROR]
    Accessibility = 6, // [ACCESSIBILITY]
    Performance = 7,   // [PERFORMANCE]
    Hypothesis = 8     // [HYPOTHESIS]
};

constexpr std::string_view to_tag_label(EvidenceTag tag) noexcept {
    switch (tag) {
        case EvidenceTag::Goal:          return "[GOAL]";
        case EvidenceTag::Pain:          return "[PAIN]";
        case EvidenceTag::Behavior:      return "[BEHAVIOR]";
        case EvidenceTag::Context:       return "[CONTEXT]";
        case EvidenceTag::Quote:         return "[QUOTE]";
        case EvidenceTag::Error:         return "[ERROR]";
        case EvidenceTag::Accessibility: return "[ACCESSIBILITY]";
        case EvidenceTag::Performance:   return "[PERFORMANCE]";
        case EvidenceTag::Hypothesis:    return "[HYPOTHESIS]";
    }
    return "[OBSERVATION]";
}

// Workflow 1: Normalized anonymized research record
struct ResearchRecord {
    std::string participant_id;
    std::string usage_frequency;
    std::string platform;
    int technical_proficiency{5}; // 1..10
    BehavioralArchetype archetype{BehavioralArchetype::ProfessionalOperator};
    std::vector<std::string> primary_tasks;
    std::vector<std::string> pain_points;
    std::string research_source;
};

// Workflow 2: Opportunity Prioritization Heuristic
// Priority = Frequency * Severity * UserImpact * Confidence (each 1..5 -> max 625)
constexpr int compute_opportunity_priority(
    int frequency_1_to_5,
    int severity_1_to_5,
    int user_impact_1_to_5,
    int confidence_1_to_5
) noexcept {
    const int f = std::clamp(frequency_1_to_5, 1, 5);
    const int s = std::clamp(severity_1_to_5, 1, 5);
    const int i = std::clamp(user_impact_1_to_5, 1, 5);
    const int c = std::clamp(confidence_1_to_5, 1, 5);
    return f * s * i * c;
}

// Workflow 3: Usability Issue Severity Classification
enum class UsabilitySeverity : uint8_t {
    Minor = 1,    // Cosmetic inconsistency or low-impact friction
    Moderate = 2, // Causes repeated hesitation, avoidable errors, or inefficient work
    Major = 3,    // Causes substantial difficulty or frequent workflow failure
    Critical = 4  // Blocks a critical task, risks data loss, or prevents essential access
};

// Workflow 5 & 9: Reusable Native Component States
enum class ControlState : uint8_t {
    Default = 0,
    Hover = 1,
    Pressed = 2,
    Focused = 3,
    Disabled = 4,
    Error = 5
};

// Workflow 8: Operating-System / Application Motion Preference
enum class MotionPreference : uint8_t {
    StandardSpring = 0, // Analytical damped harmonic spring transitions
    ReducedMotion = 1   // Immediate state transitions preserving state clarity without movement
};

// Workflow 5: WCAG 2.2 Relative Luminance & Contrast Ratio Calculator
inline double srgb_channel_to_linear(double c) noexcept {
    const double v = std::clamp(c, 0.0, 1.0);
    return (v <= 0.04045) ? (v / 12.92) : std::pow((v + 0.055) / 1.055, 2.4);
}

inline double compute_relative_luminance(const ColorRGBA& color) noexcept {
    const double r = srgb_channel_to_linear(color.r);
    const double g = srgb_channel_to_linear(color.g);
    const double b = srgb_channel_to_linear(color.b);
    return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}

inline double compute_wcag_contrast_ratio(const ColorRGBA& fg, const ColorRGBA& bg) noexcept {
    const double l1 = compute_relative_luminance(fg);
    const double l2 = compute_relative_luminance(bg);
    const double lighter = std::max(l1, l2);
    const double darker = std::min(l1, l2);
    return (lighter + 0.05) / (darker + 0.05);
}

inline bool meets_wcag_aa_normal_text(const ColorRGBA& fg, const ColorRGBA& bg) noexcept {
    return compute_wcag_contrast_ratio(fg, bg) >= 4.5;
}

inline bool meets_wcag_aaa_normal_text(const ColorRGBA& fg, const ColorRGBA& bg) noexcept {
    return compute_wcag_contrast_ratio(fg, bg) >= 7.0;
}

// Workflow 10: AI-Style & Visual-Quality Heuristic Audit Model
// Score = Sum(category_score / 10.0 * category_weight) where 0..20 = Few warning signs observed
struct VisualQualityAuditInput {
    // Written categories (0 = clean/specific, 10 = severe generic/unverified warning signs)
    double lexical_repetition_score{1.0};      // Weight: 25
    double sentence_monotony_score{1.0};       // Weight: 25
    double unverified_claims_score{0.0};       // Weight: 25
    double structural_repetition_score{1.0};   // Weight: 25

    // Visual categories (0 = clean/accurate, 10 = severe geometry/data contradictions)
    double geometry_inconsistency_score{0.5};  // Weight: 35
    double detail_degradation_score{0.5};      // Weight: 30
    double label_data_mismatch_score{0.0};     // Weight: 35
};

struct VisualQualityAuditResult {
    double written_warning_score{0.0}; // 0..100 (<= 20 = Few warning signs)
    double visual_warning_score{0.0};  // 0..100 (<= 20 = Few warning signs)
    bool passes_quality_gate{true};
};

constexpr VisualQualityAuditResult evaluate_quality_audit(const VisualQualityAuditInput& in) noexcept {
    const double written =
        (std::clamp(in.lexical_repetition_score, 0.0, 10.0) / 10.0) * 25.0 +
        (std::clamp(in.sentence_monotony_score, 0.0, 10.0) / 10.0) * 25.0 +
        (std::clamp(in.unverified_claims_score, 0.0, 10.0) / 10.0) * 25.0 +
        (std::clamp(in.structural_repetition_score, 0.0, 10.0) / 10.0) * 25.0;

    const double visual =
        (std::clamp(in.geometry_inconsistency_score, 0.0, 10.0) / 10.0) * 35.0 +
        (std::clamp(in.detail_degradation_score, 0.0, 10.0) / 10.0) * 30.0 +
        (std::clamp(in.label_data_mismatch_score, 0.0, 10.0) / 10.0) * 35.0;

    return VisualQualityAuditResult{
        written,
        visual,
        (written <= 20.0 && visual <= 20.0)
    };
}

} // namespace kinetix::ui::ux
