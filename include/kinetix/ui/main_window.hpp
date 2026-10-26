#pragma once
// Kinetix L2 v3.0 — Main Native Application Window (Consumer Focus Studio + Dear ImGui Pro Dockspace + Photon Micro-GUI)
// Integrates:
// 1. Official Kinetix L2 SVG Brand Identity & Duotone Icons (`librsvg-2.so.2`)
// 2. Separable Gaussian Blur, Frosted-Glass (Acrylic/Mica) Cards, Ambient Radial Mesh Glows & Perceptual Grain
// 3. Consumer-First "Focus Studio" Layout with on-demand Toggleable Pro Telemetry Dock (`pro_telemetry_mode_`)
// 4. Damped Harmonic Spring Physics (`SpringValue`), `ModalityStatesSoA`, and `PoolAllocator`

#include "kinetix/domain/types.hpp"
#include "kinetix/domain/fsrs_engine.hpp"
#include "kinetix/domain/knowledge_graph.hpp"
#include "kinetix/domain/pedagogical_loop.hpp"
#include "kinetix/domain/habit_engine.hpp"
#include "kinetix/domain/memory_pool.hpp"
#include "kinetix/persistence/database_manager.hpp"
#include "kinetix/platform/platform_adapter.hpp"
#include "kinetix/ui/design_tokens.hpp"
#include "kinetix/ui/photon_elements.hpp"
#include "kinetix/ui/imgui_layer.hpp"
#include "kinetix/ui/spring_physics.hpp"
#include "kinetix/ui/ux_design_system.hpp"
#include "kinetix/ui/vector_canvas.hpp"
#include "third_party/sdl2_abi.h"
#include <chrono>
#include <string>
#include <utility>
#include <vector>

namespace kinetix::ui {

enum class AppScreen : int {
    Screen1_WorkbenchOverview = 0,
    Screen2_ImmersionReader = 1,
    Screen3_ActiveRecallStudio = 2,
    Screen4_KnowledgeGraphExplorer = 3,
    Screen5_RoleplayAndSettings = 4
};

struct InteractiveButton {
    std::string id;
    std::string label;
    std::string accessible_description;
    Rect bounds;
    bool is_primary = false;
    bool is_active = false;
};

struct ProfilerTelemetry {
    double last_frame_render_ms = 0.42;
    double soa_batch_eval_us = 1.35;
    size_t soa_batch_count = 0;
    size_t pool_active_nodes = 0;
    size_t pool_capacity = 0;
    bool imgui_stack_balanced = true;
    int imgui_widgets_submitted = 0;
};

class MainWindow {
public:
    MainWindow(
        persistence::DatabaseManager& db,
        domain::KnowledgeGraph& graph,
        int width = 1440,
        int height = 880,
        bool headless_mode = false
    );
    ~MainWindow();

    bool initialize();
    void run_event_loop();

    // Render and export all native UI snapshots (Consumer Focus Studio + Light Archival + Mobile + Pro Telemetry Dock)
    bool export_all_screen_snapshots(const std::string& output_dir);

    // Programmatic interaction helpers for comprehensive feature verification
    void switch_screen(AppScreen screen);
    void select_language(const std::string& lang_code);
    void select_node(domain::NodeId node_id);
    void set_search_filter(const std::string& filter_query);
    void set_pro_telemetry_mode(bool enabled);
    void set_reduced_motion(bool enabled);
    void toggle_table_sort_column(int col_index);
    void trigger_button_action(const std::string& button_id);
    void submit_exercise_text(const std::string& text, int32_t simulated_latency_ms = 1380);

    AppScreen current_screen() const noexcept { return current_screen_; }
    const std::string& active_language() const noexcept { return active_language_; }
    domain::NodeId selected_node_id() const noexcept { return selected_node_id_; }
    bool is_dark_theme() const noexcept { return is_dark_theme_; }
    bool is_mobile_mode() const noexcept { return mobile_preview_mode_; }
    bool is_pro_telemetry_mode() const noexcept { return pro_telemetry_mode_; }
    bool is_reduced_motion() const noexcept { return reduced_motion_; }
    bool is_search_focused() const noexcept { return search_input_focused_; }
    const std::string& search_filter() const noexcept { return search_filter_; }
    void submit_explicit_fsrs_grade(domain::Rating rating, int32_t latency_ms);
    const SpringValue& nav_pill_spring() const noexcept { return nav_pill_spring_; }
    const ProfilerTelemetry& profiler_telemetry() const noexcept { return telemetry_; }
    const domain::ExerciseEvaluation& last_evaluation() const noexcept { return last_evaluation_; }

private:
    void render_frame();
    void present_to_sdl();
    void handle_sdl_event(const SDL_Event& ev);
    void refresh_language_context();
    void update_soa_and_pool_telemetry();

    // Studio & 6-Region Dockspace Shell Renderers (`CPPUIUX.md` & `photon_elements.hpp`)
    void render_region1_command_bar(const Rect& bar, const ThemePalette& theme);
    void render_region2_left_asset_browser(const Rect& pane, const ThemePalette& theme);
    void render_region4_right_inspector(const Rect& pane, const ThemePalette& theme);
    void render_region5_bottom_console(const Rect& pane, const ThemePalette& theme);
    void render_region6_status_bar(const Rect& bar, const ThemePalette& theme);
    void render_mobile_bottom_nav(const ThemePalette& theme);

    // Region 3 Central Work Area Renderers
    void render_screen1_overview(const Rect& area, const ThemePalette& theme);
    void render_screen2_immersion_reader(const Rect& area, const ThemePalette& theme);
    void render_screen3_active_recall_studio(const Rect& area, const ThemePalette& theme);
    void render_screen4_knowledge_graph(const Rect& area, const ThemePalette& theme);
    void render_screen5_roleplay_and_settings(const Rect& area, const ThemePalette& theme);

    void register_button(
        const std::string& id,
        const std::string& label,
        const std::string& desc,
        const Rect& bounds,
        bool is_primary = false,
        bool is_active = false
    );

    persistence::DatabaseManager& db_;
    domain::KnowledgeGraph& graph_;
    domain::PedagogicalLoopEngine loop_engine_;
    domain::HabitEngine habit_engine_;
    domain::EthicalHabitState habit_state_;
    platform::PlatformCapabilities platform_caps_;

    memory::PoolAllocator<domain::LinguisticNode, 4096> node_pool_;
    std::vector<domain::LinguisticNode*> pooled_nodes_;
    imgui::ImGuiContextTracker imgui_ctx_;
    imgui::TableSortSpecs table_sort_specs_;
    SpringValue nav_pill_spring_{0.0};
    ProfilerTelemetry telemetry_;

    int width_ = 1440;
    int height_ = 880;
    bool headless_mode_ = false;
    bool running_ = false;
    bool is_dark_theme_ = true;
    bool mobile_preview_mode_ = false;
    bool pro_telemetry_mode_ = false;
    bool reduced_motion_ = false;
    bool search_input_focused_ = false;
    std::chrono::steady_clock::time_point exercise_prompt_start_ts_{};
    std::vector<domain::RevlogEntry> cached_revlog_;

    SDL_Window* sdl_window_ = nullptr;
    SDL_Renderer* sdl_renderer_ = nullptr;
    SDL_Texture* sdl_texture_ = nullptr;
    VectorCanvas canvas_;

    AppScreen current_screen_ = AppScreen::Screen1_WorkbenchOverview;
    std::string active_language_ = "de";
    std::string search_filter_;
    domain::NodeId selected_node_id_ = 103;
    int32_t active_backchain_step_ = 0;

    domain::PedagogicalExercise current_exercise_;
    std::string typed_input_buffer_;
    bool has_last_evaluation_ = false;
    domain::ExerciseEvaluation last_evaluation_;
    std::string status_banner_message_ = "Ready — All 4 FSRS-4.5 modalities synced with SQLite3 WAL.";

    std::vector<domain::ImmersionPassage> passages_;
    size_t active_passage_index_ = 0;
    std::vector<std::pair<Rect, domain::NodeId>> clickable_tokens_;

    std::vector<domain::SimulationTurn> roleplay_history_;

    std::vector<InteractiveButton> buttons_;
    std::vector<std::pair<Rect, domain::NodeId>> clickable_graph_nodes_;
    int focused_button_index_ = -1;
};

} // namespace kinetix::ui
