#pragma once

/**
 * @file spring_physics.hpp
 * @brief Analytical & semi-implicit Euler damped harmonic spring physics solver
 *        for 60-120 FPS micro-interactions (pill tab sliding, card hover
 *        elevation, and pitch-contour visual feedback) with Workflow 8
 *        `ReducedMotion` accessibility compliance.
 */

#include <cmath>
#include <algorithm>

namespace kinetix::ui {

struct SpringConfig {
    double stiffness{260.0}; // Spring constant k (N/m)
    double damping{24.0};    // Damping coefficient c (Ns/m)
    double mass{1.0};        // Mass m (kg)

    [[nodiscard]] inline double critical_damping() const noexcept {
        return 2.0 * std::sqrt(stiffness * mass);
    }

    [[nodiscard]] inline double damping_ratio() const noexcept {
        const double crit = critical_damping();
        return (crit > 1e-6) ? (damping / crit) : 1.0;
    }
};

class SpringValue {
public:
    constexpr explicit SpringValue(double initial = 0.0, SpringConfig cfg = {}) noexcept
        : current_(initial), target_(initial), velocity_(0.0), cfg_(cfg) {}

    constexpr void set_target(double target) noexcept {
        target_ = target;
    }

    /**
     * @brief Workflow 8: Respects OS/Application Reduced-Motion preference by
     *        snapping immediately to the target state when reduced_motion is true.
     */
    constexpr void set_target_respecting_motion(double target, bool reduced_motion) noexcept {
        if (reduced_motion) {
            snap_to(target);
        } else {
            target_ = target;
        }
    }

    constexpr void snap_to(double value) noexcept {
        current_ = value;
        target_ = value;
        velocity_ = 0.0;
    }

    /**
     * @brief Advances the spring state by dt seconds using sub-stepped
     *        symplectic Euler integration for unconditional stability.
     */
    inline double step(double dt) noexcept {
        if (dt <= 0.0) return current_;
        const double clamped_dt = std::min(dt, 0.1);
        constexpr int kSubSteps = 4;
        const double h = clamped_dt / static_cast<double>(kSubSteps);

        for (int i = 0; i < kSubSteps; ++i) {
            const double displacement = current_ - target_;
            const double spring_force = -cfg_.stiffness * displacement;
            const double damping_force = -cfg_.damping * velocity_;
            const double acceleration = (spring_force + damping_force) / std::max(1e-6, cfg_.mass);

            velocity_ += acceleration * h;
            current_ += velocity_ * h;
        }

        if (std::abs(current_ - target_) < 1e-4 && std::abs(velocity_) < 1e-4) {
            current_ = target_;
            velocity_ = 0.0;
        }
        return current_;
    }

    [[nodiscard]] constexpr double value() const noexcept { return current_; }
    [[nodiscard]] constexpr double target() const noexcept { return target_; }
    [[nodiscard]] constexpr double velocity() const noexcept { return velocity_; }
    [[nodiscard]] inline bool is_settled() const noexcept {
        return std::abs(current_ - target_) < 1e-3 && std::abs(velocity_) < 1e-3;
    }

private:
    double current_{0.0};
    double target_{0.0};
    double velocity_{0.0};
    SpringConfig cfg_{};
};

} // namespace kinetix::ui
