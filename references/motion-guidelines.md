# Workflow 8: Purposeful Motion & Reduced-Motion Accessibility (`kinetix L2`)

## 1. Analytical Damped Harmonic Spring Physics (`include/kinetix/ui/spring_physics.hpp`)
- `SpringValue` integrates `F = -k(x - target) - c*v` (`k = 260`, `c = 24`, `m = 1`) using 4x sub-stepped symplectic Euler integration for unconditional stability.
- Used exclusively for purposeful spatial continuity (navigation pill transitions and active card focus).

## 2. Reduced-Motion Preference (`ux::MotionPreference::ReducedMotion`)
- When `reduced_motion_` is enabled via `MainWindow::set_reduced_motion(true)` or the `[Motion: Reduced]` command-bar toggle (`toggle_reduced_motion`), `SpringValue::set_target_respecting_motion(target, reduced_motion)` snaps immediately (`snap_to(target)`) with zero oscillation.
