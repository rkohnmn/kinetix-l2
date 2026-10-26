# Workflow 7: Native Desktop & Mobile Layout Architecture (`kinetix L2`)

## 1. Progressive Disclosure: Consumer Focus Studio vs. Pro Telemetry Dock
- **Consumer Focus Studio (`pro_telemetry_mode_ == false`)**: Default view maximizing vertical space (`794px` central canvas height) with zero developer clutter.
- **Pro Telemetry Dock (`pro_telemetry_mode_ == true`)**: Toggled via `[Pro Telemetry: ON/OFF]` in the top command bar (`Region1_CommandBar`), revealing Region 5 (`156px` bottom console with sortable CRDT `revlog` table and C++20 SoA/PoolAllocator profiler).
- **Responsive Mobile Layout (`mobile_preview_mode_ == true`, `440x860`)**: Replaces desktop side rails with a 5-tab thumb-zone bottom navigation bar (`62px` height).
