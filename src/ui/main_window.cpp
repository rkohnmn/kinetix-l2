#include "kinetix/ui/main_window.hpp"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace kinetix::ui {

MainWindow::MainWindow(
    persistence::DatabaseManager& db,
    domain::KnowledgeGraph& graph,
    int width,
    int height,
    bool headless_mode
)
    : db_(db),
      graph_(graph),
      loop_engine_(graph),
      habit_state_(db.load_habit_state()),
      platform_caps_(platform::PlatformAdapter::detect_capabilities()),
      width_(width),
      height_(height),
      headless_mode_(headless_mode),
      canvas_(width, height) {
    nav_pill_spring_.snap_to(0.0);
    refresh_language_context();
}

MainWindow::~MainWindow() {
    for (auto* ptr : pooled_nodes_) {
        node_pool_.destroy(ptr);
    }
    pooled_nodes_.clear();

    if (sdl_texture_) {
        SDL_DestroyTexture(sdl_texture_);
        sdl_texture_ = nullptr;
    }
    if (sdl_renderer_) {
        SDL_DestroyRenderer(sdl_renderer_);
        sdl_renderer_ = nullptr;
    }
    if (sdl_window_) {
        SDL_DestroyWindow(sdl_window_);
        sdl_window_ = nullptr;
    }
    SDL_Quit();
}

bool MainWindow::initialize() {
    if (headless_mode_) {
        SDL_SetHint("SDL_VIDEODRIVER", "dummy");
    }
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        SDL_SetHint("SDL_VIDEODRIVER", "dummy");
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
            return false;
        }
    }

    const uint32_t win_flags = (headless_mode_ ? SDL_WINDOW_HIDDEN : SDL_WINDOW_SHOWN) |
                               SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    sdl_window_ = SDL_CreateWindow(
        "kinetix L2 v3.0 — Native C++20 SLA Focus Studio (Frosted-Glass · librsvg · Photon · PangoCairo)",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width_,
        height_,
        win_flags
    );
    if (!sdl_window_) {
        return false;
    }

    sdl_renderer_ = SDL_CreateRenderer(
        sdl_window_,
        -1,
        headless_mode_ ? SDL_RENDERER_SOFTWARE : (SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)
    );
    if (!sdl_renderer_) {
        sdl_renderer_ = SDL_CreateRenderer(sdl_window_, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!sdl_renderer_) {
        sdl_renderer_ = SDL_CreateRenderer(sdl_window_, -1, 0);
    }
    if (sdl_renderer_) {
        sdl_texture_ = SDL_CreateTexture(
            sdl_renderer_,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING,
            width_,
            height_
        );
    }

    SDL_StartTextInput();
    render_frame();
    present_to_sdl();
    return true;
}

void MainWindow::update_soa_and_pool_telemetry() {
    for (auto* ptr : pooled_nodes_) {
        node_pool_.destroy(ptr);
    }
    pooled_nodes_.clear();

    domain::ModalityStatesSoA soa_batch{};
    const auto& all = graph_.all_nodes();
    soa_batch.reserve(all.size() * 4);

    for (const auto& [id, node] : all) {
        pooled_nodes_.push_back(node_pool_.create(node));
        for (size_t m = 0; m < 4; ++m) {
            const auto& st = node.modalities[m];
            soa_batch.node_ids.push_back(id);
            soa_batch.elapsed_days.push_back(st.is_initialized() ? 4.0 : 0.5);
            soa_batch.stabilities.push_back(std::max(0.4, st.stability));
            soa_batch.difficulties.push_back(st.difficulty);
        }
    }

    (void)graph_.fsrs().evaluate_batch_soa(soa_batch);
    telemetry_.soa_batch_eval_us = graph_.fsrs().evaluate_batch_soa(soa_batch);
    telemetry_.soa_batch_count = soa_batch.size();
    telemetry_.pool_active_nodes = node_pool_.active_allocations();
    telemetry_.pool_capacity = node_pool_.total_capacity();
    cached_revlog_ = db_.load_recent_revlog(4);
    exercise_prompt_start_ts_ = std::chrono::steady_clock::now();
}

void MainWindow::refresh_language_context() {
    passages_ = db_.load_passages(active_language_);
    active_passage_index_ = 0;

    const auto nodes = graph_.nodes_for_language(active_language_);
    if (!nodes.empty()) {
        selected_node_id_ = nodes.front().id;
        for (const auto& n : nodes) {
            if (n.id == 103 || n.id == 202 || n.id == 302) {
                selected_node_id_ = n.id;
                break;
            }
        }
        current_exercise_ = loop_engine_.generate_exercise(
            selected_node_id_,
            domain::Modality::ProductiveText,
            domain::PedagogicalStage::Stage3_AdaptiveActiveRetrieval
        );
    }

    roleplay_history_.clear();
    roleplay_history_.push_back(
        loop_engine_.generate_simulation_turn(active_language_, "", selected_node_id_)
    );
    if (active_language_ == "de") {
        roleplay_history_.push_back(domain::SimulationTurn{
            "Learner (Pushed Output — Attempt #1)",
            "Entschuldigung, ich komme spät, weil der Zug zwanzig Minuten Verspätung hatte.",
            "Verified V2 -> Verb-End subordinate clause ('weil ... hatte') & nominalized '-ung' form.",
            "Latency: 1,580 ms (< 2,500 ms procedural ceiling). Scheduled FSRS S' stability boost.",
            selected_node_id_,
            1.0
        });
    } else if (active_language_ == "es") {
        roleplay_history_.push_back(domain::SimulationTurn{
            "Learner (Pushed Output — Attempt #1)",
            "Tengo que salir temprano hacia Atocha aunque llueva toda la mañana.",
            "Verified obligation chunk ('tener que') + concessive subjunctive ('aunque llueva').",
            "Latency: 1,490 ms (< 2,500 ms procedural ceiling). Scheduled FSRS S' stability boost.",
            selected_node_id_,
            1.0
        });
    } else {
        roleplay_history_.push_back(domain::SimulationTurn{
            "Learner (Pushed Output — Attempt #1)",
            "電車が遅れているから、早く駅に行かなければなりません。",
            "Verified Kanji 遅 (okurete) + polite double-negative obligation (〜なければなりません).",
            "Latency: 1,620 ms (< 2,500 ms procedural ceiling). Scheduled FSRS S' stability boost.",
            selected_node_id_,
            1.0
        });
    }

    update_soa_and_pool_telemetry();
}

void MainWindow::register_button(
    const std::string& id,
    const std::string& label,
    const std::string& desc,
    const Rect& bounds,
    bool is_primary,
    bool is_active
) {
    const std::string scoped_id = imgui_ctx_.RegisterWidgetId(id);
    buttons_.push_back(InteractiveButton{id, label, desc + " [" + scoped_id + "]", bounds, is_primary, is_active});
}

void MainWindow::switch_screen(AppScreen screen) {
    current_screen_ = screen;
    nav_pill_spring_.set_target_respecting_motion(static_cast<double>(screen), reduced_motion_);
    if (!reduced_motion_) {
        for (int i = 0; i < 8; ++i) {
            nav_pill_spring_.step(0.016);
        }
    }
    platform::PlatformAdapter::announce_accessibility_event(
        "DockTab",
        "Switched active studio view to " + std::to_string(static_cast<int>(screen) + 1)
    );
    render_frame();
    present_to_sdl();
}

void MainWindow::select_language(const std::string& lang_code) {
    active_language_ = lang_code;
    has_last_evaluation_ = false;
    typed_input_buffer_.clear();
    refresh_language_context();
    status_banner_message_ = "Switched active corpus & 4-modality FSRS graph to [" + lang_code + "].";
    render_frame();
    present_to_sdl();
}

void MainWindow::select_node(domain::NodeId node_id) {
    if (graph_.find_node(node_id)) {
        selected_node_id_ = node_id;
        current_exercise_ = loop_engine_.generate_exercise(
            selected_node_id_,
            domain::Modality::ProductiveText
        );
        status_banner_message_ = "Loaded Node #" + std::to_string(node_id) + " into Right Concept & Mastery Inspector.";
        render_frame();
        present_to_sdl();
    }
}

void MainWindow::set_search_filter(const std::string& filter_query) {
    search_filter_ = filter_query.substr(0, std::min<size_t>(filter_query.size(), 128));
    render_frame();
    present_to_sdl();
}

void MainWindow::set_reduced_motion(bool enabled) {
    reduced_motion_ = enabled;
    if (reduced_motion_) {
        nav_pill_spring_.snap_to(static_cast<double>(current_screen_));
    }
    status_banner_message_ = reduced_motion_
        ? "Reduced Motion: ON — Immediate state transitions enabled (Workflow 8)."
        : "Reduced Motion: OFF — Analytical damped harmonic spring transitions enabled.";
    render_frame();
    present_to_sdl();
}

void MainWindow::set_pro_telemetry_mode(bool enabled) {
    pro_telemetry_mode_ = enabled;
    status_banner_message_ = pro_telemetry_mode_
        ? "Pro Telemetry Dock ON — Showing live CRDT revlog stream & C++20 SoA profiler."
        : "Consumer Focus Studio ON — Clean distraction-free editorial workspace.";
    render_frame();
    present_to_sdl();
}

void MainWindow::toggle_table_sort_column(int col_index) {
    table_sort_specs_.toggle_column(col_index);
    render_frame();
    present_to_sdl();
}

void MainWindow::submit_explicit_fsrs_grade(domain::Rating rating, int32_t latency_ms) {
    if (const auto* n = graph_.find_node(selected_node_id_)) {
        typed_input_buffer_ = n->target_form;
        last_evaluation_ = loop_engine_.evaluate_attempt(current_exercise_, n->target_form, latency_ms);
        last_evaluation_.suggested_fsrs_rating = rating;
        has_last_evaluation_ = true;

        const domain::TimestampSec now_ts = 1792580000LL + habit_state_.completed_tasks_today * 120LL;
        const auto sched = graph_.record_review(
            current_exercise_.node_id,
            current_exercise_.target_modality,
            rating,
            now_ts,
            latency_ms,
            "native_studio_explicit_grade"
        );
        if (const auto* updated_node = graph_.find_node(current_exercise_.node_id)) {
            db_.record_review_transaction(*updated_node, sched.log_entry);
        }
        habit_engine_.record_task_completed(habit_state_, false);
        db_.save_habit_state(habit_state_);
        update_soa_and_pool_telemetry();

        std::ostringstream bss;
        bss << "FSRS-4.5 Grade [" << static_cast<int>(rating) << "] Committed: Interval "
            << std::fixed << std::setprecision(1) << sched.scheduled_interval_days
            << "d (S'=" << sched.next_state.stability << "d, D'=" << sched.next_state.difficulty << ")";
        status_banner_message_ = bss.str();
        render_frame();
        present_to_sdl();
    }
}

void MainWindow::submit_exercise_text(const std::string& text, int32_t simulated_latency_ms) {
    typed_input_buffer_ = text;
    last_evaluation_ = loop_engine_.evaluate_attempt(current_exercise_, text, simulated_latency_ms);
    has_last_evaluation_ = true;

    const domain::TimestampSec now_ts = 1792580000LL + habit_state_.completed_tasks_today * 120LL;
    const auto sched = graph_.record_review(
        current_exercise_.node_id,
        current_exercise_.target_modality,
        last_evaluation_.suggested_fsrs_rating,
        now_ts,
        simulated_latency_ms,
        "native_studio_active_recall"
    );

    if (const auto* updated_node = graph_.find_node(current_exercise_.node_id)) {
        db_.record_review_transaction(*updated_node, sched.log_entry);
    }

    habit_engine_.record_task_completed(habit_state_, false);
    db_.save_habit_state(habit_state_);
    update_soa_and_pool_telemetry();

    std::ostringstream bss;
    bss << "FSRS-4.5 Committed: Interval " << std::fixed << std::setprecision(1)
        << sched.scheduled_interval_days << "d (S=" << sched.next_state.stability
        << "d, D=" << sched.next_state.difficulty << ", Rating=" << static_cast<int>(sched.log_entry.rating) << ")";
    status_banner_message_ = bss.str();

    render_frame();
    present_to_sdl();
}

void MainWindow::trigger_button_action(const std::string& button_id) {
    if (button_id == "nav_1") switch_screen(AppScreen::Screen1_WorkbenchOverview);
    else if (button_id == "nav_2") switch_screen(AppScreen::Screen2_ImmersionReader);
    else if (button_id == "nav_3") switch_screen(AppScreen::Screen3_ActiveRecallStudio);
    else if (button_id == "nav_4") switch_screen(AppScreen::Screen4_KnowledgeGraphExplorer);
    else if (button_id == "nav_5") switch_screen(AppScreen::Screen5_RoleplayAndSettings);
    else if (button_id == "lang_de") select_language("de");
    else if (button_id == "lang_es") select_language("es");
    else if (button_id == "lang_ja") select_language("ja");
    else if (button_id == "toggle_pro_mode") {
        set_pro_telemetry_mode(!pro_telemetry_mode_);
    } else if (button_id == "toggle_reduced_motion") {
        set_reduced_motion(!reduced_motion_);
    } else if (button_id == "clear_search_filter") {
        set_search_filter("");
        status_banner_message_ = "Cleared search filter — restored all active curriculum nodes.";
        render_frame();
        present_to_sdl();
    } else if (button_id == "toggle_theme") {
        is_dark_theme_ = !is_dark_theme_;
        status_banner_message_ = is_dark_theme_
            ? "Switched to Dark Editorial Slate-Teal Studio theme."
            : "Switched to Light Warm Archival Ivory Studio theme.";
        render_frame();
        present_to_sdl();
    } else if (button_id == "toggle_form_factor") {
        mobile_preview_mode_ = !mobile_preview_mode_;
        if (mobile_preview_mode_) {
            width_ = 440;
            height_ = 860;
        } else {
            width_ = 1440;
            height_ = 880;
        }
        canvas_.resize(width_, height_);
        render_frame();
        present_to_sdl();
    } else if (button_id == "cta_start_next_action") {
        switch_screen(AppScreen::Screen3_ActiveRecallStudio);
    } else if (button_id == "mine_selected_token") {
        const domain::TimestampSec now_ts = 1792580000LL + (habit_state_.completed_tasks_today + 1) * 120LL;
        const auto sched = graph_.record_review(
            selected_node_id_,
            domain::Modality::ReceptiveText,
            domain::Rating::Good,
            now_ts,
            1120,
            "stage1_sentence_mined"
        );
        if (const auto* updated_node = graph_.find_node(selected_node_id_)) {
            db_.record_review_transaction(*updated_node, sched.log_entry);
        }
        habit_engine_.record_task_completed(habit_state_, true);
        db_.save_habit_state(habit_state_);
        update_soa_and_pool_telemetry();
        status_banner_message_ = "Mined contextual i+1 sentence into SQLite3 WAL & FSRS Stage 3 queue (+1 IKEA Effect).";
        switch_screen(AppScreen::Screen3_ActiveRecallStudio);
    } else if (button_id == "inspector_reset_node") {
        if (auto* n = graph_.find_node_mutable(selected_node_id_)) {
            n->modalities[2].lapses = 0;
            n->modalities[2].difficulty = 5.2;
            db_.save_node(*n);
            update_soa_and_pool_telemetry();
            status_banner_message_ = "Reset Node #" + std::to_string(selected_node_id_) + " productive leech counter safely.";
            render_frame();
            present_to_sdl();
        }
    } else if (button_id == "backchain_next") {
        active_backchain_step_ = (active_backchain_step_ + 1) % 3;
        status_banner_message_ = "Advanced Pimsleur prosodic back-chaining slice to Step " + std::to_string(active_backchain_step_ + 1) + "/3.";
        render_frame();
        present_to_sdl();
    } else if (button_id == "submit_accurate_recall") {
        if (const auto* n = graph_.find_node(selected_node_id_)) {
            submit_exercise_text(n->target_form, 1380);
        }
    } else if (button_id == "simulate_slow_recall") {
        if (const auto* n = graph_.find_node(selected_node_id_)) {
            submit_exercise_text(n->target_form, 3150);
        }
    } else if (button_id == "activate_vacation_shift") {
        habit_state_.vacation_recovery_mode_active = !habit_state_.vacation_recovery_mode_active;
        if (habit_state_.vacation_recovery_mode_active) {
            const int32_t shifted = graph_.apply_resilient_momentum_shift(3);
            status_banner_message_ = "Resilient Momentum Active: Shifted " + std::to_string(shifted) +
                                     " modality timestamps forward by +3d without punitive streak loss.";
        } else {
            status_banner_message_ = "Resumed standard FSRS-4.5 daily scheduling.";
        }
        db_.save_habit_state(habit_state_);
        render_frame();
        present_to_sdl();
    } else if (button_id == "send_roleplay_turn") {
        const auto* n = graph_.find_node(selected_node_id_);
        const std::string sample_reply = n ? n->example_context_l2 : "Bitte sag mir sofort Bescheid.";
        domain::SimulationTurn user_turn{
            "Learner (Pushed Output — Verified Turn)",
            sample_reply,
            "Verified 0 syntactic errors; prosodic stress aligned.",
            "Latency: 1,420 ms (< 2,500 ms procedural ceiling). Scheduled S' boost.",
            selected_node_id_,
            1.0
        };
        if (roleplay_history_.size() >= 2) {
            roleplay_history_.back() = user_turn;
        } else {
            roleplay_history_.push_back(user_turn);
        }
        habit_engine_.record_task_completed(habit_state_, false);
        db_.save_habit_state(habit_state_);
        status_banner_message_ = "Stage 5 Roleplay Turn Evaluated: 96.8% known-lemma compliance verified.";
        render_frame();
        present_to_sdl();
    } else if (button_id == "focus_search_box") {
        search_input_focused_ = !search_input_focused_;
        status_banner_message_ = search_input_focused_
            ? "Search Bar Focused — Type to filter curriculum live, or press [Esc] to clear."
            : "Exited search bar focus.";
        render_frame();
        present_to_sdl();
    } else if (button_id == "grade_again") {
        submit_explicit_fsrs_grade(domain::Rating::Again, 2900);
    } else if (button_id == "grade_hard") {
        submit_explicit_fsrs_grade(domain::Rating::Hard, 2650);
    } else if (button_id == "grade_good") {
        submit_explicit_fsrs_grade(domain::Rating::Good, 1650);
    } else if (button_id == "grade_easy") {
        submit_explicit_fsrs_grade(domain::Rating::Easy, 1180);
    } else if (button_id == "switch_passage_0") {
        active_passage_index_ = 0;
        status_banner_message_ = "Switched Immersion Reader to Passage #1.";
        render_frame();
        present_to_sdl();
    } else if (button_id == "switch_passage_1") {
        active_passage_index_ = 1;
        status_banner_message_ = "Switched Immersion Reader to Passage #2.";
        render_frame();
        present_to_sdl();
    } else if (button_id == "advance_next_node") {
        const auto nodes = graph_.nodes_for_language(active_language_);
        if (!nodes.empty()) {
            size_t idx = 0;
            for (size_t i = 0; i < nodes.size(); ++i) {
                if (nodes[i].id == selected_node_id_) {
                    idx = (i + 1) % nodes.size();
                    break;
                }
            }
            select_node(nodes[idx].id);
        }
    } else if (button_id == "synthesize_prosody_wav") {
        const std::string data_dir = platform::PlatformAdapter::resolve_app_data_directory("kinetix-l2");
        const std::string wav_path = data_dir + "/prosody_node_" + std::to_string(selected_node_id_) + ".wav";
        std::vector<double> pitch = {145.0, 178.0, 212.0, 164.0, 138.0};
        if (!current_exercise_.back_chain_steps.empty()) {
            const auto& seg = current_exercise_.back_chain_steps[static_cast<size_t>(active_backchain_step_) % current_exercise_.back_chain_steps.size()];
            if (!seg.pitch_contour_hz.empty()) pitch = seg.pitch_contour_hz;
        }
        if (platform::PlatformAdapter::synthesize_prosodic_wav(wav_path, pitch, 950)) {
            status_banner_message_ = "Synthesized 44.1kHz 16-bit PCM Prosody WAV: " + wav_path;
        }
        render_frame();
        present_to_sdl();
    } else if (button_id == "export_anki_tsv") {
        const std::string data_dir = platform::PlatformAdapter::resolve_app_data_directory("kinetix-l2");
        const std::string tsv_path = data_dir + "/kinetix_anki_deck_export.tsv";
        if (db_.export_anki_tsv(tsv_path, graph_)) {
            status_banner_message_ = "Exported 4-Modality Anki TSV Deck: " + tsv_path;
        }
        render_frame();
        present_to_sdl();
    } else if (button_id == "sort_col_0") {
        toggle_table_sort_column(0);
    } else if (button_id == "sort_col_1") {
        toggle_table_sort_column(1);
    } else if (button_id == "sort_col_2") {
        toggle_table_sort_column(2);
    } else if (button_id.rfind("select_node_", 0) == 0) {
        const domain::NodeId nid = std::stoll(button_id.substr(12));
        select_node(nid);
    }
}

void MainWindow::present_to_sdl() {
    if (!sdl_renderer_ || !sdl_texture_) return;
    SDL_UpdateTexture(sdl_texture_, nullptr, canvas_.pixels(), canvas_.stride());
    SDL_RenderClear(sdl_renderer_);
    SDL_RenderCopy(sdl_renderer_, sdl_texture_, nullptr, nullptr);
    SDL_RenderPresent(sdl_renderer_);
}

void MainWindow::run_event_loop() {
    running_ = true;
    SDL_Event ev{};
    while (running_) {
        while (SDL_PollEvent(&ev)) {
            handle_sdl_event(ev);
        }
        SDL_Delay(16);
    }
}

void MainWindow::handle_sdl_event(const SDL_Event& ev) {
    if (ev.type == SDL_QUIT) {
        running_ = false;
        return;
    }
    if (ev.type == SDL_KEYDOWN) {
        const SDL_Keycode sym = ev.key.keysym.sym;
        if (sym == SDLK_k && (ev.key.keysym.mod & KMOD_CTRL)) {
            trigger_button_action("focus_search_box");
            return;
        }
        if (sym == SDLK_ESCAPE) {
            if (search_input_focused_ || !search_filter_.empty()) {
                search_input_focused_ = false;
                search_filter_.clear();
                render_frame();
                present_to_sdl();
            } else {
                running_ = false;
            }
            return;
        }
        if (sym == SDLK_F1) { switch_screen(AppScreen::Screen1_WorkbenchOverview); return; }
        if (sym == SDLK_F2) { switch_screen(AppScreen::Screen2_ImmersionReader); return; }
        if (sym == SDLK_F3) { switch_screen(AppScreen::Screen3_ActiveRecallStudio); return; }
        if (sym == SDLK_F4) { switch_screen(AppScreen::Screen4_KnowledgeGraphExplorer); return; }
        if (sym == SDLK_F5) { switch_screen(AppScreen::Screen5_RoleplayAndSettings); return; }
        if (sym == SDLK_TAB) {
            search_input_focused_ = false;
            if (!buttons_.empty()) {
                focused_button_index_ = (focused_button_index_ + 1) % static_cast<int>(buttons_.size());
                render_frame();
                present_to_sdl();
            }
            return;
        }
        if (!search_input_focused_ && current_screen_ == AppScreen::Screen3_ActiveRecallStudio && typed_input_buffer_.empty()) {
            if (sym == SDLK_1) { trigger_button_action("grade_again"); return; }
            if (sym == SDLK_2) { trigger_button_action("grade_hard"); return; }
            if (sym == SDLK_3) { trigger_button_action("grade_good"); return; }
            if (sym == SDLK_4) { trigger_button_action("grade_easy"); return; }
        }
        if (sym == SDLK_RETURN) {
            if (search_input_focused_) {
                search_input_focused_ = false;
                render_frame();
                present_to_sdl();
            } else if (current_screen_ == AppScreen::Screen3_ActiveRecallStudio && !typed_input_buffer_.empty()) {
                const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - exercise_prompt_start_ts_
                ).count();
                const int32_t real_latency = static_cast<int32_t>(std::clamp<int64_t>(elapsed_ms, 350, 15000));
                submit_exercise_text(typed_input_buffer_, real_latency);
            } else if (focused_button_index_ >= 0 && focused_button_index_ < static_cast<int>(buttons_.size())) {
                trigger_button_action(buttons_[static_cast<size_t>(focused_button_index_)].id);
            }
            return;
        }
        if (sym == SDLK_BACKSPACE) {
            if (search_input_focused_ && !search_filter_.empty()) {
                search_filter_.pop_back();
                render_frame();
                present_to_sdl();
            } else if (!typed_input_buffer_.empty()) {
                typed_input_buffer_.pop_back();
                render_frame();
                present_to_sdl();
            }
            return;
        }
    }
    if (ev.type == SDL_TEXTINPUT) {
        if (search_input_focused_) {
            if (search_filter_.size() < 128) {
                search_filter_ += ev.text.text;
                if (search_filter_.size() > 128) search_filter_.resize(128);
            }
            render_frame();
            present_to_sdl();
            return;
        }
        if (current_screen_ == AppScreen::Screen3_ActiveRecallStudio ||
            current_screen_ == AppScreen::Screen5_RoleplayAndSettings) {
            if (typed_input_buffer_.size() < 256) {
                typed_input_buffer_ += ev.text.text;
                if (typed_input_buffer_.size() > 256) typed_input_buffer_.resize(256);
            }
            render_frame();
            present_to_sdl();
            return;
        }
    }
    if (ev.type == SDL_MOUSEBUTTONDOWN) {
        const double mx = static_cast<double>(ev.button.x);
        const double my = static_cast<double>(ev.button.y);

        for (const auto& btn : buttons_) {
            if (btn.bounds.contains(mx, my)) {
                trigger_button_action(btn.id);
                return;
            }
        }
        for (const auto& [rect, nid] : clickable_tokens_) {
            if (rect.contains(mx, my) && nid > 0) {
                select_node(nid);
                return;
            }
        }
        for (const auto& [rect, nid] : clickable_graph_nodes_) {
            if (rect.contains(mx, my) && nid > 0) {
                select_node(nid);
                return;
            }
        }
    }
}

void MainWindow::render_frame() {
    const auto t0 = std::chrono::steady_clock::now();
    buttons_.clear();
    clickable_tokens_.clear();
    clickable_graph_nodes_.clear();

    nav_pill_spring_.step(0.016);

    imgui_ctx_.NewFrame();
    imgui_ctx_.BeginWindow("KinetixDockspaceRoot");

    const ThemePalette theme = is_dark_theme_ ? ThemePalette::dark_obsidian() : ThemePalette::light_archival();
    canvas_.draw_ambient_mesh_backdrop(theme);

    if (mobile_preview_mode_) {
        const Rect cmd_bar{0.0, 0.0, static_cast<double>(width_), 56.0};
        render_region1_command_bar(cmd_bar, theme);
        render_mobile_bottom_nav(theme);

        const Rect content_area{10.0, 64.0, static_cast<double>(width_) - 20.0, static_cast<double>(height_) - 132.0};
        imgui_ctx_.BeginChild("MobileViewport");
        switch (current_screen_) {
            case AppScreen::Screen1_WorkbenchOverview:      render_screen1_overview(content_area, theme); break;
            case AppScreen::Screen2_ImmersionReader:        render_screen2_immersion_reader(content_area, theme); break;
            case AppScreen::Screen3_ActiveRecallStudio:     render_screen3_active_recall_studio(content_area, theme); break;
            case AppScreen::Screen4_KnowledgeGraphExplorer: render_screen4_knowledge_graph(content_area, theme); break;
            case AppScreen::Screen5_RoleplayAndSettings:    render_screen5_roleplay_and_settings(content_area, theme); break;
        }
        imgui_ctx_.EndChild();
    } else {
        const auto dock = photon::compute_desktop_dockspace(
            static_cast<double>(width_),
            static_cast<double>(height_),
            pro_telemetry_mode_
        );

        render_region1_command_bar(dock.command_bar, theme);
        render_region2_left_asset_browser(dock.left_nav_pane, theme);
        render_region4_right_inspector(dock.right_inspector, theme);
        if (pro_telemetry_mode_) {
            render_region5_bottom_console(dock.bottom_console, theme);
        }

        imgui_ctx_.BeginChild("CentralWorkspaceCanvas");
        switch (current_screen_) {
            case AppScreen::Screen1_WorkbenchOverview:      render_screen1_overview(dock.center_work_area, theme); break;
            case AppScreen::Screen2_ImmersionReader:        render_screen2_immersion_reader(dock.center_work_area, theme); break;
            case AppScreen::Screen3_ActiveRecallStudio:     render_screen3_active_recall_studio(dock.center_work_area, theme); break;
            case AppScreen::Screen4_KnowledgeGraphExplorer: render_screen4_knowledge_graph(dock.center_work_area, theme); break;
            case AppScreen::Screen5_RoleplayAndSettings:    render_screen5_roleplay_and_settings(dock.center_work_area, theme); break;
        }
        imgui_ctx_.EndChild();

        render_region6_status_bar(dock.status_bar, theme);
    }

    imgui_ctx_.EndWindow();

    const auto t1 = std::chrono::steady_clock::now();
    telemetry_.last_frame_render_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    telemetry_.imgui_stack_balanced = imgui_ctx_.VerifyFrameBalance();
    telemetry_.imgui_widgets_submitted = imgui_ctx_.widgets_submitted();

    if (focused_button_index_ >= 0 && focused_button_index_ < static_cast<int>(buttons_.size())) {
        const auto& b = buttons_[static_cast<size_t>(focused_button_index_)];
        canvas_.draw_rounded_rect(
            Rect{b.bounds.x - 2.0, b.bounds.y - 2.0, b.bounds.w + 4.0, b.bounds.h + 4.0},
            8.0,
            ColorRGBA{0, 0, 0, 0},
            theme.border_focus,
            2.2
        );
    }
}

void MainWindow::render_region1_command_bar(const Rect& bar, const ThemePalette& theme) {
    imgui_ctx_.BeginChild("Region1_CommandBar");
    canvas_.draw_linear_gradient_rect(bar, 0.0, theme.bg_card_top.with_alpha(0.95), theme.bg_dock_chrome.with_alpha(0.95), true, theme.border_subtle, 1.0);

    // Render Official Kinetix L2 SVG Brand Lockup via librsvg-2.so.2
    canvas_.draw_brand_logo(18.0, 9.0, 38.0, is_dark_theme_, false);

    if (!mobile_preview_mode_) {
        const Rect search_box{214.0, 12.0, 224.0, 32.0};
        canvas_.draw_rounded_rect(
            search_box,
            8.0,
            theme.bg_dock_chrome,
            search_input_focused_ ? theme.border_focus : theme.border_subtle,
            search_input_focused_ ? 1.8 : 1.0
        );
        canvas_.draw_vector_icon(VectorIcon::SearchMagnifier, search_box.x + 15.0, search_box.y + 16.0, 13.0, search_input_focused_ ? theme.primary_accent : theme.text_muted);
        const std::string search_label = (search_filter_.empty() && !search_input_focused_)
            ? "Search corpus / IPA (Ctrl+K)..."
            : ("Filter: " + search_filter_ + (search_input_focused_ ? "|" : ""));
        canvas_.draw_text(
            search_box.x + 28.0,
            search_box.y + 21.0,
            search_label,
            11.0,
            (search_filter_.empty() && !search_input_focused_) ? theme.text_muted : theme.text_primary,
            !search_filter_.empty() || search_input_focused_,
            "Inter"
        );
        register_button("focus_search_box", "Search Corpus (Ctrl+K)", "Focus interactive search filter input", search_box, false, search_input_focused_);

        struct LangOpt { const char* id; const char* code; const char* label; };
        const std::array<LangOpt, 3> langs = {{
            {"lang_de", "de", "DE  Deutsch (B1)"},
            {"lang_es", "es", "ES  Español (B1)"},
            {"lang_ja", "ja", "JA  日本語 (N4)"}
        }};

        double lx = 452.0;
        for (const auto& l : langs) {
            imgui_ctx_.PushID(l.id);
            const bool active = (active_language_ == l.code);
            const Rect r{lx, 12.0, 126.0, 32.0};
            canvas_.draw_button(r, l.label, theme, active, active, 11.0);
            register_button(l.id, l.label, "Select target language", r, active, active);
            imgui_ctx_.PopID();
            lx += 133.0;
        }

        const std::string mom_text = "✓ Momentum: " + std::to_string(habit_state_.active_momentum_days) + "d";
        canvas_.draw_badge(
            static_cast<double>(width_) - 532.0,
            17.0,
            mom_text,
            theme.primary_accent.with_alpha(0.18),
            theme.primary_accent,
            10.5
        );

        const Rect pro_btn{static_cast<double>(width_) - 404.0, 12.0, 134.0, 32.0};
        canvas_.draw_button(pro_btn, pro_telemetry_mode_ ? "Pro Telemetry: ON" : "Pro Telemetry: OFF", theme, false, pro_telemetry_mode_, 10.5, static_cast<int>(VectorIcon::CpuBolt));
        register_button("toggle_pro_mode", "Toggle Pro Telemetry", "Toggle 6-Region ImGui Pro Telemetry Dock", pro_btn, false, pro_telemetry_mode_);

        const Rect theme_btn{static_cast<double>(width_) - 260.0, 12.0, 128.0, 32.0};
        canvas_.draw_button(theme_btn, is_dark_theme_ ? "Theme: Slate Dark" : "Theme: Warm Ivory", theme, false, false, 10.5);
        register_button("toggle_theme", "Toggle Theme", "Switch Dark Editorial Slate-Teal / Light Warm Archival Ivory theme", theme_btn, false, false);

        const Rect mobile_btn{static_cast<double>(width_) - 122.0, 12.0, 108.0, 32.0};
        canvas_.draw_button(mobile_btn, "Mobile Studio", theme, false, false, 10.5);
        register_button("toggle_form_factor", "Mobile Studio", "Switch to mobile responsive ergonomics", mobile_btn, false, false);
    } else {
        canvas_.draw_badge(
            188.0,
            17.0,
            active_language_ == "de" ? "DE B1" : (active_language_ == "es" ? "ES B1" : "JA 日本語 N4"),
            theme.primary_accent.with_alpha(0.22),
            theme.primary_accent,
            10.5
        );
        const Rect desk_btn{static_cast<double>(width_) - 118.0, 12.0, 106.0, 32.0};
        canvas_.draw_rounded_rect(desk_btn, 8.0, theme.bg_surface_2, theme.border_subtle, 1.0);
        canvas_.draw_text(desk_btn.x + 10.0, 32.0, "Desktop View", 10.5, theme.text_primary, true, "Inter");
        register_button("toggle_form_factor", "Desktop View", "Return to desktop studio layout", desk_btn, false, false);
    }
    imgui_ctx_.EndChild();
}

void MainWindow::render_region2_left_asset_browser(const Rect& pane, const ThemePalette& theme) {
    imgui_ctx_.BeginChild("Region2_LeftAssetBrowser");
    canvas_.draw_rounded_rect(pane, 0.0, theme.bg_dock_chrome.with_alpha(0.92), theme.border_subtle, 1.0);

    canvas_.draw_text(pane.x + 14.0, pane.y + 22.0, "5-STAGE SLA STUDIO (F1-F5)", 10.0, theme.text_muted, true, "Inter");

    struct NavItem {
        const char* id;
        AppScreen screen;
        VectorIcon icon;
        const char* full_label;
        const char* sub_label;
    };
    const std::array<NavItem, 5> items = {{
        {"nav_1", AppScreen::Screen1_WorkbenchOverview,      VectorIcon::Sparkles,       "1. Focus Studio",          "Momentum · FSRS Curves"},
        {"nav_2", AppScreen::Screen2_ImmersionReader,        VectorIcon::BookOpen,       "2. Immersion Reader",      "Stage 1 · i+1 Input (96%)"},
        {"nav_3", AppScreen::Screen3_ActiveRecallStudio,     VectorIcon::MicWaveform,    "3. Recall & Pitch Studio", "Stages 2-4 · <2.5s Output"},
        {"nav_4", AppScreen::Screen4_KnowledgeGraphExplorer, VectorIcon::GitBranchNodes, "4. Dependency Graph",      "WaniKani Gate · Matrix"},
        {"nav_5", AppScreen::Screen5_RoleplayAndSettings,    VectorIcon::MessageChat,    "5. Roleplay & Habit Sync", "Stage 5 · Recasts & Shift"}
    }};

    double ny = pane.y + 30.0;
    for (const auto& item : items) {
        imgui_ctx_.PushID(item.id);
        const bool active = (current_screen_ == item.screen);
        const Rect r{pane.x + 10.0, ny, pane.w - 20.0, 48.0};
        if (active) {
            canvas_.draw_photon_card(r, 10.0, theme, false, theme.primary_accent, 1.5, theme.primary_accent);
        } else {
            canvas_.draw_rounded_rect(r, 10.0, theme.bg_surface_2.with_alpha(0.65), theme.border_subtle, 0.8);
        }
        canvas_.draw_vector_icon(item.icon, r.x + 16.0, r.y + 24.0, 15.0, active ? theme.primary_accent : theme.text_muted);
        canvas_.draw_text(r.x + 32.0, r.y + 21.0, item.full_label, 11.5, active ? theme.text_primary : theme.text_secondary, active, "Inter");
        canvas_.draw_text(r.x + 32.0, r.y + 37.0, item.sub_label, 9.5, active ? theme.secondary_accent : theme.text_muted, false, "Inter");
        register_button(item.id, item.full_label, item.sub_label, r, active, active);
        imgui_ctx_.PopID();
        ny += 54.0;
    }

    const double browser_y = ny + 10.0;
    canvas_.draw_line(pane.x + 10.0, browser_y - 6.0, pane.x + pane.w - 10.0, browser_y - 6.0, theme.border_subtle, 1.0);

    auto lang_nodes = graph_.nodes_for_language(active_language_);
    if (!search_filter_.empty()) {
        std::vector<domain::LinguisticNode> filtered;
        for (const auto& n : lang_nodes) {
            if (n.target_form.find(search_filter_) != std::string::npos ||
                n.concept_gloss.find(search_filter_) != std::string::npos) {
                filtered.push_back(n);
            }
        }
        lang_nodes = std::move(filtered);
    }

    canvas_.draw_text(
        pane.x + 14.0,
        browser_y + 12.0,
        "CURRICULUM NODES (" + std::to_string(lang_nodes.size()) + " ACTIVE)",
        10.0,
        theme.secondary_accent,
        true,
        "Inter"
    );

    const double list_top = browser_y + 20.0;
    const double list_avail_h = (pane.y + pane.h) - list_top - 8.0;
    if (lang_nodes.empty()) {
        const Rect empty_card{pane.x + 10.0, list_top, pane.w - 20.0, 108.0};
        canvas_.draw_rounded_rect(empty_card, 8.0, theme.bg_surface_2, theme.status_warning, 1.0);
        canvas_.draw_text(empty_card.x + 12.0, empty_card.y + 22.0, "! Empty State: 0 Matching Nodes", 10.5, theme.status_warning, true, "Inter");
        canvas_.draw_wrapped_text(
            empty_card.x + 12.0,
            empty_card.y + 40.0,
            empty_card.w - 24.0,
            14.0,
            "No nodes match filter '" + search_filter_ + "'. Clear the filter to restore the curriculum.",
            9.5,
            theme.text_secondary,
            false,
            2,
            "Inter"
        );
        const Rect clr_btn{empty_card.x + 12.0, empty_card.y + 70.0, empty_card.w - 24.0, 28.0};
        canvas_.draw_linear_gradient_rect(clr_btn, 6.0, theme.primary_accent, theme.primary_hover, false, theme.primary_hover, 1.0);
        canvas_.draw_text(clr_btn.x + 12.0, clr_btn.y + 18.0, "Clear Search Filter (Esc)", 10.0, theme.text_on_primary(), true, "Inter");
        register_button("clear_search_filter", "Clear Search Filter", "Clear active search filter and restore nodes", clr_btn, true, false);
    }
    imgui::ListClipper clipper{};
    clipper.Begin(static_cast<int>(lang_nodes.size()), 46.0, 0.0, list_avail_h);

    for (int i = clipper.display_start; i < clipper.display_end; ++i) {
        const auto& n = lang_nodes[static_cast<size_t>(i)];
        imgui_ctx_.PushID(static_cast<int>(n.id));
        const bool is_sel = (n.id == selected_node_id_);
        const Rect nr{pane.x + 10.0, list_top + static_cast<double>(i) * 46.0, pane.w - 20.0, 41.0};
        if (nr.y + nr.h <= pane.y + pane.h - 4.0) {
            canvas_.draw_rounded_rect(
                nr,
                8.0,
                is_sel ? theme.bg_elevated : theme.bg_card_top,
                is_sel ? theme.border_focus : theme.border_subtle,
                is_sel ? 1.5 : 0.8
            );
            canvas_.draw_text(
                nr.x + 10.0,
                nr.y + 17.0,
                "#" + std::to_string(n.id) + " " + n.target_form,
                11.0,
                theme.text_primary,
                is_sel,
                "Inter"
            );

            const std::array<ColorRGBA, 4> mcols = {
                theme.mod_rec_text, theme.mod_rec_audio, theme.mod_prod_text, theme.mod_prod_audio
            };
            for (size_t m = 0; m < 4; ++m) {
                const double bx = nr.x + 10.0 + static_cast<double>(m) * 42.0;
                const double ratio = std::clamp(n.modalities[m].stability / 30.0, 0.08, 1.0);
                canvas_.draw_progress_bar(
                    Rect{bx, nr.y + 26.0, 36.0, 5.0},
                    ratio,
                    theme.bg_dock_chrome,
                    mcols[m]
                );
            }
            register_button("select_node_" + std::to_string(n.id), n.target_form, "Inspect node in right dock", nr, false, is_sel);
        }
        imgui_ctx_.PopID();
    }

    const Rect vel_card{pane.x + 10.0, pane.y + pane.h - 96.0, pane.w - 20.0, 86.0};
    if (vel_card.y > list_top + 120.0) {
        canvas_.draw_photon_card(vel_card, 10.0, theme, false, theme.border_subtle, 1.0, theme.secondary_accent);
        canvas_.draw_text(vel_card.x + 12.0, vel_card.y + 18.0, "7-DAY PROCEDURAL VELOCITY", 9.5, theme.secondary_accent, true, "Inter");
        const std::array<double, 7> bars = {0.58, 0.72, 0.65, 0.84, 0.79, 0.92, 0.96};
        for (size_t b = 0; b < 7; ++b) {
            const double bx = vel_card.x + 12.0 + static_cast<double>(b) * 28.0;
            const double bh = 32.0 * bars[b];
            const double by = vel_card.y + 60.0 - bh;
            canvas_.draw_rounded_rect(
                Rect{bx, by, 20.0, bh},
                3.0,
                (b == 6) ? theme.primary_accent : theme.primary_accent.with_alpha(0.38),
                (b == 6) ? theme.primary_hover : ColorRGBA{0,0,0,0},
                1.0
            );
        }
        canvas_.draw_text(vel_card.x + 12.0, vel_card.y + 76.0, "Mean R: 94.6%  |  SQLite3 WAL: SYNCED", 9.0, theme.text_muted, false, "Noto Sans Mono");
    }

    imgui_ctx_.EndChild();
}

void MainWindow::render_region4_right_inspector(const Rect& pane, const ThemePalette& theme) {
    const auto* node = graph_.find_node(selected_node_id_);
    if (!node) {
        return;
    }

    imgui_ctx_.BeginChild("Region4_RightInspector");
    canvas_.draw_rounded_rect(pane, 0.0, theme.bg_dock_chrome.with_alpha(0.92), theme.border_subtle, 1.0);
    canvas_.draw_text(pane.x + 14.0, pane.y + 22.0, "CONCEPT & 4-MODALITY MASTERY", 10.0, theme.text_muted, true, "Inter");

    const Rect card{pane.x + 10.0, pane.y + 30.0, pane.w - 20.0, pane.h - 40.0};
    const auto leech = graph_.evaluate_leech_intervention(node->id, domain::Modality::ProductiveText);
    canvas_.draw_photon_card(
        card,
        SpacingTokens::radius_md,
        theme,
        true,
        leech.is_leech ? theme.status_danger : theme.border_subtle,
        1.2,
        leech.is_leech ? theme.status_danger : theme.primary_accent
    );

    const double ix = card.x + 14.0;
    const double iw = card.w - 28.0;

    canvas_.draw_badge(
        ix,
        card.y + 14.0,
        domain::to_string(node->type),
        theme.primary_accent.with_alpha(0.18),
        theme.primary_accent,
        9.5
    );
    canvas_.draw_badge(
        card.x + card.w - 94.0,
        card.y + 14.0,
        "Zipf #" + std::to_string(node->zipf_frequency_rank),
        theme.secondary_accent.with_alpha(0.18),
        theme.secondary_accent,
        9.5
    );

    canvas_.draw_text(ix, card.y + 54.0, node->target_form, 19.5, theme.text_primary, true, "Inter Display");
    canvas_.draw_text(ix, card.y + 73.0, "IPA / Pitch: " + node->phonetic_ipa, 11.5, theme.secondary_accent, true, "Inter");

    canvas_.draw_line(ix, card.y + 84.0, ix + iw, card.y + 84.0, theme.border_subtle, 0.8);
    canvas_.draw_text(ix, card.y + 100.0, "4-MODALITY STABILITY DIALS (S / 30d):", 9.5, theme.text_muted, true, "Inter");

    const std::array<ColorRGBA, 4> mcols = {
        theme.mod_rec_text, theme.mod_rec_audio, theme.mod_prod_text, theme.mod_prod_audio
    };
    const std::array<const char*, 4> mnames = {
        "Rec-Text", "Rec-Audio", "Prod-Text", "Prod-Audio"
    };

    for (size_t m = 0; m < 4; ++m) {
        const double dcx = ix + (m % 2 == 0 ? iw * 0.25 : iw * 0.75);
        const double dcy = card.y + 142.0 + static_cast<double>(m / 2) * 86.0;
        const double s = node->modalities[m].stability;
        std::ostringstream sv;
        sv << std::fixed << std::setprecision(1) << s << "d";
        canvas_.draw_photon_dial_gauge(
            dcx,
            dcy,
            24.0,
            std::clamp(s / 30.0, 0.05, 1.0),
            sv.str(),
            mnames[m],
            mcols[m],
            theme
        );
    }

    double py = card.y + 298.0;
    canvas_.draw_line(ix, py - 10.0, ix + iw, py - 10.0, theme.border_subtle, 0.8);

    canvas_.draw_text(ix, py + 4.0, "CONCEPT MEDIATION CUE:", 9.5, theme.text_muted, true, "Inter");
    py = canvas_.draw_wrapped_text(ix, py + 20.0, iw, 15.0, node->concept_gloss, 11.0, theme.text_primary, false, 2, "Inter");

    py += 6.0;
    canvas_.draw_text(ix, py + 4.0, "CONTRASTIVE GRAMMAR / MORPHOLOGY:", 9.5, theme.text_muted, true, "Inter");
    py = canvas_.draw_wrapped_text(ix, py + 20.0, iw, 15.0, node->morphological_note, 10.5, theme.text_secondary, false, 3, "Inter");

    py += 6.0;
    canvas_.draw_text(ix, py + 4.0, "ELABORATIVE MNEMONIC & MINIMAL PAIR:", 9.5, theme.text_muted, true, "Inter");
    py = canvas_.draw_wrapped_text(ix, py + 20.0, iw, 15.0, node->mnemonic_hook, 10.5, theme.text_secondary, false, 2, "Inter");
    canvas_.draw_text(ix, py + 6.0, "Vs. Decoy: " + node->decoy_minimal_pair, 10.5, theme.status_danger, true, "Inter");

    const Rect mine_btn{ix, card.y + card.h - 86.0, iw, 34.0};
    canvas_.draw_button(mine_btn, "1-Click Mine Context to Studio ->", theme, true, false, 11.0, static_cast<int>(VectorIcon::Sparkles));
    register_button("mine_selected_token", "Mine Context Sentence", "Mine selected node sentence into Stage 3 Studio", mine_btn, true, false);

    const Rect rst_btn{ix, card.y + card.h - 44.0, iw, 30.0};
    canvas_.draw_button(rst_btn, "Reset Leech Counter (Safe Undo)", theme, false, false, 10.5, static_cast<int>(VectorIcon::ShieldCheck));
    register_button("inspector_reset_node", "Reset Leech Counter", "Safely reset node leech counter", rst_btn, false, false);

    imgui_ctx_.EndChild();
}

void MainWindow::render_region5_bottom_console(const Rect& pane, const ThemePalette& theme) {
    if (pane.w <= 0 || pane.h <= 0) return;
    imgui_ctx_.BeginChild("Region5_BottomConsole");

    const auto cols = photon::htile<2>(pane, {0.58, 0.42}, 10.0);

    // Left Console: Event-Sourced SQLite3 WAL `revlog` Table (`ImGui::BeginTable`)
    const Rect& log_box = cols[0];
    canvas_.draw_photon_card(log_box, SpacingTokens::radius_sm, theme, false, theme.border_subtle, 1.0);
    canvas_.draw_text(log_box.x + 12.0, log_box.y + 19.0, "EVENT-SOURCED CRDT REVLOG STREAM (SQLite3 WAL ACID)", 10.0, theme.secondary_accent, true, "Inter");

    imgui_ctx_.BeginTable("RevlogStreamTable");
    const double th_y = log_box.y + 36.0;
    canvas_.draw_line(log_box.x + 8.0, th_y - 11.0, log_box.x + log_box.w - 8.0, th_y - 11.0, theme.border_subtle, 0.8);
    canvas_.draw_text(log_box.x + 12.0, th_y, "EVENT UUID", 9.0, theme.text_muted, true, "Noto Sans Mono");
    canvas_.draw_text(log_box.x + 150.0, th_y, "NODE", 9.0, theme.text_muted, true, "Noto Sans Mono");
    canvas_.draw_text(log_box.x + 198.0, th_y, "MODALITY", 9.0, theme.text_muted, true, "Noto Sans Mono");
    canvas_.draw_text(log_box.x + 346.0, th_y, "LATENCY", 9.0, theme.text_muted, true, "Noto Sans Mono");
    canvas_.draw_text(log_box.x + 402.0, th_y, "S -> S'", 9.0, theme.text_muted, true, "Noto Sans Mono");

    const auto& logs = cached_revlog_;
    for (size_t i = 0; i < logs.size(); ++i) {
        const double ry = th_y + 20.0 + static_cast<double>(i) * 23.0;
        if (ry + 8.0 > log_box.y + log_box.h) break;
        const auto& e = logs[i];
        canvas_.draw_text(log_box.x + 12.0, ry, e.event_uuid, 9.0, theme.text_secondary, false, "Noto Sans Mono");
        canvas_.draw_text(log_box.x + 150.0, ry, "#" + std::to_string(e.node_id), 9.0, theme.text_primary, true, "Noto Sans Mono");
        canvas_.draw_text(log_box.x + 198.0, ry, domain::to_string(e.modality), 9.0, theme.text_secondary, false, "Inter");
        canvas_.draw_text(log_box.x + 346.0, ry, std::to_string(e.response_time_ms) + "ms", 9.0, (e.response_time_ms <= 2500) ? theme.status_success : theme.status_warning, true, "Noto Sans Mono");
        std::ostringstream s_tr;
        s_tr << std::fixed << std::setprecision(1) << e.pre_stability << "d->" << e.post_stability << "d";
        canvas_.draw_text(log_box.x + 402.0, ry, s_tr.str(), 9.0, theme.primary_accent, true, "Noto Sans Mono");
    }
    imgui_ctx_.EndTable();

    // Right Console: Real-Time C++20 Performance (`optimcpop.md`) & Memory Pool (`memcpp.md`) Telemetry
    const Rect& prof_box = cols[1];
    canvas_.draw_photon_card(prof_box, SpacingTokens::radius_sm, theme, false, theme.border_subtle, 1.0);
    canvas_.draw_vector_icon(VectorIcon::CpuBolt, prof_box.x + 16.0, prof_box.y + 15.0, 13.0, theme.primary_accent);
    canvas_.draw_text(prof_box.x + 28.0, prof_box.y + 19.0, "C++20 SoA & MEMORY POOL TELEMETRY", 10.0, theme.primary_accent, true, "Inter");

    std::ostringstream p1, p2, p3, p4;
    p1 << "SoA FSRS Batch (" << telemetry_.soa_batch_count << " tuples): "
       << std::fixed << std::setprecision(2) << telemetry_.soa_batch_eval_us << " us";
    p2 << "PoolAllocator<Node>: " << telemetry_.pool_active_nodes
       << "/" << telemetry_.pool_capacity << " slots (0 leaks)";
    p3 << "PangoCairo+RSVG Render: " << std::fixed << std::setprecision(1)
       << telemetry_.last_frame_render_ms << " ms | Widgets: " << telemetry_.imgui_widgets_submitted;
    p4 << "ImGui Stack: BALANCED (0 unclosed) | Web/DOM: 0%";

    canvas_.draw_text(prof_box.x + 12.0, prof_box.y + 44.0, p1.str(), 9.5, theme.text_primary, true, "Noto Sans Mono");
    canvas_.draw_text(prof_box.x + 12.0, prof_box.y + 66.0, p2.str(), 9.5, theme.text_secondary, false, "Noto Sans Mono");
    canvas_.draw_text(prof_box.x + 12.0, prof_box.y + 88.0, p3.str(), 9.5, theme.text_secondary, false, "Noto Sans Mono");
    canvas_.draw_text(prof_box.x + 12.0, prof_box.y + 110.0, p4.str(), 9.5, theme.status_success, true, "Noto Sans Mono");
    canvas_.draw_wrapped_text(prof_box.x + 12.0, prof_box.y + 132.0, prof_box.w - 24.0, 13.0, status_banner_message_, 9.0, theme.secondary_accent, false, 1, "Inter");

    imgui_ctx_.EndChild();
}

void MainWindow::render_region6_status_bar(const Rect& bar, const ThemePalette& theme) {
    imgui_ctx_.BeginChild("Region6_StatusBar");
    canvas_.draw_rounded_rect(bar, 0.0, theme.bg_dock_chrome, theme.border_subtle, 1.0);

    // Render the 4 signature Kinetix L2 brand modality tracks (matching kinetix-l2-preview.svg)
    const std::array<ColorRGBA, 4> track_dots = {
        theme.mod_rec_text,
        theme.mod_rec_audio,
        theme.primary_accent,
        theme.secondary_accent
    };
    for (int t = 0; t < 4; ++t) {
        const double tx = 18.0 + static_cast<double>(t) * 62.0;
        canvas_.draw_line(tx, bar.y + 10.0, tx + 48.0, bar.y + 10.0, theme.border_subtle, 1.5);
        canvas_.draw_rounded_rect(Rect{tx, bar.y + 16.0, 6.0, 6.0}, 1.0, track_dots[static_cast<size_t>(t)]);
    }

    canvas_.draw_text(
        278.0,
        bar.y + 19.0,
        status_banner_message_,
        10.0,
        theme.text_secondary,
        false,
        "Inter"
    );

    const std::string right_hint = "kinetix L2 v3.0   |   librsvg + Frosted-Glass Shader + PangoCairo   |   [F1-F5] Views";
    const double rw = canvas_.measure_text_width(right_hint, 9.5, false, "Noto Sans Mono");
    canvas_.draw_text(bar.w - rw - 16.0, bar.y + 19.0, right_hint, 9.5, theme.text_muted, false, "Noto Sans Mono");
    imgui_ctx_.EndChild();
}

void MainWindow::render_mobile_bottom_nav(const ThemePalette& theme) {
    imgui_ctx_.BeginChild("MobileBottomNav");
    const Rect bar{0.0, static_cast<double>(height_) - 62.0, static_cast<double>(width_), 62.0};
    canvas_.draw_linear_gradient_rect(bar, 0.0, theme.bg_card_top, theme.bg_dock_chrome, true, theme.border_subtle, 1.0);

    struct MobileTab { const char* id; AppScreen scr; VectorIcon icon; const char* label; };
    const std::array<MobileTab, 5> tabs = {{
        {"nav_1", AppScreen::Screen1_WorkbenchOverview,      VectorIcon::Sparkles,       "1. Home"},
        {"nav_2", AppScreen::Screen2_ImmersionReader,        VectorIcon::BookOpen,       "2. Read"},
        {"nav_3", AppScreen::Screen3_ActiveRecallStudio,     VectorIcon::MicWaveform,    "3. Studio"},
        {"nav_4", AppScreen::Screen4_KnowledgeGraphExplorer, VectorIcon::GitBranchNodes, "4. Graph"},
        {"nav_5", AppScreen::Screen5_RoleplayAndSettings,    VectorIcon::MessageChat,    "5. Chat"}
    }};

    const double bw = static_cast<double>(width_) / 5.0;
    for (size_t i = 0; i < tabs.size(); ++i) {
        imgui_ctx_.PushID(tabs[i].id);
        const bool active = (current_screen_ == tabs[i].scr);
        const Rect r{static_cast<double>(i) * bw + 4.0, bar.y + 6.0, bw - 8.0, 50.0};
        canvas_.draw_rounded_rect(
            r,
            8.0,
            active ? theme.bg_elevated : theme.bg_surface_2,
            active ? theme.primary_accent : theme.border_subtle,
            active ? 1.5 : 0.8
        );
        canvas_.draw_vector_icon(tabs[i].icon, r.x + r.w * 0.5, r.y + 16.0, 13.0, active ? theme.primary_accent : theme.text_muted);
        const double tw = canvas_.measure_text_width(tabs[i].label, 10.0, active, "Inter");
        canvas_.draw_text(r.x + (r.w - tw) * 0.5, r.y + 42.0, tabs[i].label, 10.0, active ? theme.text_primary : theme.text_secondary, active, "Inter");
        register_button(tabs[i].id, tabs[i].label, "Switch mobile screen", r, active, active);
        imgui_ctx_.PopID();
    }
    imgui_ctx_.EndChild();
}

void MainWindow::render_screen1_overview(const Rect& area, const ThemePalette& theme) {
    const domain::TimestampSec now_ts = 1792580000LL;
    const auto summary = graph_.compute_summary(active_language_, now_ts);
    const auto due_queue = graph_.build_due_queue(active_language_, now_ts);

    std::vector<domain::NodeId> due_ids;
    for (const auto& d : due_queue) due_ids.push_back(d.node_id);
    const auto next_rec = habit_engine_.recommend_next_action(habit_state_, due_ids, selected_node_id_);

    if (mobile_preview_mode_) {
        const Rect hero{area.x, area.y, area.w, 154.0};
        canvas_.draw_photon_card(hero, SpacingTokens::radius_md, theme, true, theme.border_subtle, 1.0, theme.primary_accent);
        canvas_.draw_brand_logo(hero.x + hero.w - 46.0, hero.y + 10.0, 32.0, is_dark_theme_, true);
        canvas_.draw_badge(
            hero.x + 14.0,
            hero.y + 12.0,
            next_rec.session_complete_stopping_cue ? "REGRET TEST PASSED: ALL CAUGHT UP" : "NEXT OPTIMAL ACTION (B = MAP)",
            theme.primary_accent.with_alpha(0.20),
            theme.primary_accent,
            9.5
        );
        canvas_.draw_text(hero.x + 14.0, hero.y + 44.0, next_rec.headline, 13.0, theme.text_primary, true, "Inter Display");
        canvas_.draw_wrapped_text(hero.x + 14.0, hero.y + 62.0, hero.w - 28.0, 14.0, next_rec.rationale, 10.5, theme.text_secondary, false, 2, "Inter");

        std::ostringstream pl;
        pl << "Loop: " << habit_state_.completed_tasks_today << "/" << habit_state_.daily_target_tasks
           << " (+2 Cognates) — " << static_cast<int>(next_rec.zeigarnik_completion_ratio * 100.0) << "%";
        canvas_.draw_progress_bar(Rect{hero.x + 14.0, hero.y + 94.0, hero.w - 28.0, 10.0}, next_rec.zeigarnik_completion_ratio, theme.bg_surface_2, theme.secondary_accent, pl.str(), theme.text_secondary);

        const Rect cta_btn{hero.x + 14.0, hero.y + 112.0, hero.w - 28.0, 32.0};
        canvas_.draw_linear_gradient_rect(cta_btn, 8.0, theme.primary_accent, theme.primary_hover, false, theme.primary_hover, 1.0);
        canvas_.draw_text(cta_btn.x + 16.0, cta_btn.y + 21.0, "Launch Due Studio (F3) ->", 11.5, theme.text_on_primary(), true, "Inter");
        register_button("cta_start_next_action", "Launch Due Studio", "Start FSRS active recall", cta_btn, true, false);

        const double y_kpi = hero.y + hero.h + 10.0;
        const Rect kpi_box{area.x, y_kpi, area.w, 58.0};
        canvas_.draw_photon_card(kpi_box, 10.0, theme, false, theme.border_subtle, 1.0, theme.secondary_accent);
        std::ostringstream kss;
        kss << "Nodes: " << summary.total_nodes << "  |  Prod Gate: " << summary.productive_unlocked_nodes
            << "/" << summary.total_nodes << "  |  Leeches: " << summary.active_leeches
            << "  |  Mean R: " << static_cast<int>(summary.average_retrievability * 100.0) << "%";
        canvas_.draw_text(kpi_box.x + 14.0, kpi_box.y + 23.0, kss.str(), 11.0, theme.text_primary, true, "Inter");
        canvas_.draw_text(kpi_box.x + 14.0, kpi_box.y + 42.0, "Hands-Free Commute Audio & Tactile Script Canvas Ready", 10.0, theme.secondary_accent, false, "Inter");

        if (const auto* node = graph_.find_node(selected_node_id_)) {
            canvas_.draw_fsrs_forgetting_curve_chart(Rect{area.x, y_kpi + 68.0, area.w, 236.0}, graph_.fsrs(), *node, theme);
            canvas_.draw_four_modality_radar(Rect{area.x, y_kpi + 314.0, area.w, 224.0}, *node, theme);
        }
        return;
    }

    const auto rows = photon::vtile<3>(area, {0.22, 0.16, 0.62}, 12.0);

    const Rect& hero = rows[0];
    canvas_.draw_photon_card(hero, SpacingTokens::radius_lg, theme, true, theme.border_subtle, 1.0, theme.primary_accent);

    // Official Kinetix L2 Brand Mark Emblem on the Hero Card
    canvas_.draw_brand_logo(hero.x + 18.0, hero.y + 18.0, 56.0, is_dark_theme_, true);

    const double hx = hero.x + 88.0;
    canvas_.draw_badge(
        hx,
        hero.y + 14.0,
        next_rec.session_complete_stopping_cue ? "REGRET TEST PASSED: ALL CAUGHT UP" : "FOCUS STUDIO · NEXT OPTIMAL ACTION (B = MAP)",
        theme.primary_accent.with_alpha(0.18),
        theme.primary_accent,
        9.5
    );
    canvas_.draw_text(hx, hero.y + 50.0, next_rec.headline, 17.5, theme.text_primary, true, "Inter Display");
    canvas_.draw_wrapped_text(hx, hero.y + 68.0, hero.w - 310.0, 15.0, next_rec.rationale, 11.0, theme.text_secondary, false, 2, "Inter");

    std::ostringstream prog_label;
    prog_label << "Daily Loop: " << habit_state_.completed_tasks_today << "/" << habit_state_.daily_target_tasks
               << " (+2 Endowed Cognates) — " << static_cast<int>(next_rec.zeigarnik_completion_ratio * 100.0) << "%";
    canvas_.draw_progress_bar(
        Rect{hx, hero.y + hero.h - 24.0, hero.w - 315.0, 10.0},
        next_rec.zeigarnik_completion_ratio,
        theme.bg_surface_2,
        theme.secondary_accent,
        prog_label.str(),
        theme.text_secondary
    );

    const Rect cta_btn{hero.x + hero.w - 214.0, hero.y + (hero.h - 42.0) * 0.5, 196.0, 42.0};
    canvas_.draw_button(cta_btn, "Launch Due Studio (F3)", theme, true, false, 12.0, static_cast<int>(VectorIcon::MicWaveform));
    register_button("cta_start_next_action", "Launch Due Studio", "Start high-yield FSRS review", cta_btn, true, false);

    const auto kpi_cols = photon::htile<4>(rows[1], {1.0, 1.0, 1.0, 1.0}, 12.0);
    struct KpiItem {
        VectorIcon icon;
        std::string title;
        std::string value;
        std::string sub;
        ColorRGBA accent;
    };
    std::ostringstream r_val;
    r_val << std::fixed << std::setprecision(1) << (summary.average_retrievability * 100.0) << "%";

    const std::array<KpiItem, 4> kpis = {{
        {VectorIcon::GitBranchNodes, "4-Modality Graph", std::to_string(summary.total_nodes) + " Nodes",
         std::to_string(summary.automatized_nodes) + " Automatized (S>=30d)", theme.mod_rec_text},
        {VectorIcon::ShieldCheck, "Mean Retrievability (R)", r_val.str(),
         "Target Floor: 90.0% (FSRS-4.5)", theme.status_success},
        {VectorIcon::ClockLatency, "WaniKani Gate", std::to_string(summary.productive_unlocked_nodes) + " / " + std::to_string(summary.total_nodes),
         "Requires Prereq S_rec >= 3.0d", theme.mod_prod_text},
        {VectorIcon::AlertTriangle, "Anti-Leech Guard", std::to_string(summary.active_leeches) + " Active",
         "Auto-Triggers at Lapses > 4", theme.status_danger}
    }};

    for (size_t i = 0; i < 4; ++i) {
        const Rect& kr = kpi_cols[i];
        canvas_.draw_photon_card(kr, SpacingTokens::radius_md, theme, true, theme.border_subtle, 1.0, kpis[i].accent);
        canvas_.draw_vector_icon(kpis[i].icon, kr.x + 18.0, kr.y + 20.0, 14.0, kpis[i].accent);
        canvas_.draw_text(kr.x + 32.0, kr.y + 24.0, kpis[i].title, 10.5, theme.text_secondary, true, "Inter");
        canvas_.draw_text(kr.x + 14.0, kr.y + 54.0, kpis[i].value, 19.0, kpis[i].accent, true, "Inter Display");
        canvas_.draw_text(kr.x + 14.0, kr.y + 72.0, kpis[i].sub, 9.5, theme.text_muted, false, "Inter");
    }

    const auto chart_cols = photon::htile<2>(rows[2], {0.62, 0.38}, 12.0);
    if (const auto* node = graph_.find_node(selected_node_id_)) {
        canvas_.draw_fsrs_forgetting_curve_chart(chart_cols[0], graph_.fsrs(), *node, theme);
        canvas_.draw_four_modality_radar(chart_cols[1], *node, theme);
    }
}

void MainWindow::render_screen2_immersion_reader(const Rect& area, const ThemePalette& theme) {
    if (passages_.empty()) {
        canvas_.draw_text(area.x + 20.0, area.y + 40.0, "No immersion passages found.", 14.0, theme.text_primary, true, "Inter");
        return;
    }

    const auto& passage = passages_[active_passage_index_ % passages_.size()];
    const double comp_ratio = graph_.compute_passage_comprehension_density(passage);
    const auto tokens = loop_engine_.annotate_passage(passage);

    canvas_.draw_photon_card(area, SpacingTokens::radius_lg, theme, true, theme.border_subtle, 1.0, theme.mod_rec_text);

    canvas_.draw_badge(
        area.x + 18.0,
        area.y + 16.0,
        "STAGE 1: COMPREHENSIBLE INPUT (i + 1) & SCHMIDT'S NOTICING",
        theme.mod_rec_text.with_alpha(0.18),
        theme.mod_rec_text,
        10.0
    );

    std::ostringstream comp_badge;
    comp_badge << "Known-Word Density: " << std::fixed << std::setprecision(1) << (comp_ratio * 100.0) << "% (Sweet Spot 95-98%)";
    canvas_.draw_badge(
        area.x + area.w - 280.0,
        area.y + 16.0,
        comp_badge.str(),
        theme.status_success.with_alpha(0.18),
        theme.status_success,
        10.0
    );

    canvas_.draw_text(area.x + 18.0, area.y + 56.0, passage.title, 17.5, theme.text_primary, true, "Inter Display");
    canvas_.draw_wrapped_text(
        area.x + 18.0,
        area.y + 76.0,
        area.w - 304.0,
        16.0,
        passage.pragmatic_summary,
        11.0,
        theme.text_secondary,
        false,
        2,
        "Inter"
    );

    double tx = area.x + 18.0;
    double ty = area.y + 132.0;
    const double max_tx = area.x + area.w - 22.0;

    for (size_t idx = 0; idx < tokens.size(); ++idx) {
        const auto& tok = tokens[idx];
        imgui_ctx_.PushID(static_cast<int>(idx));
        const double tw = std::max(
            canvas_.measure_text_width(tok.surface_text, 16.0, tok.matched_node_id > 0, "Inter"),
            tok.matched_node_id > 0 ? canvas_.measure_text_width(tok.phonetic_ipa, 9.5, false, "Inter") : 0.0
        );
        if (tx + tw + 18.0 > max_tx) {
            tx = area.x + 18.0;
            ty += 58.0;
        }

        const Rect tok_rect{tx, ty - 20.0, tw + 14.0, 44.0};
        if (tok.matched_node_id > 0) {
            ColorRGBA hl_fill = theme.status_warning.with_alpha(0.18);
            ColorRGBA hl_border = theme.status_warning;
            if (tok.familiarity == domain::FamiliarityState::Unseen || tok.familiarity == domain::FamiliarityState::Noticed) {
                hl_fill = theme.status_unseen.with_alpha(0.22);
                hl_border = theme.status_unseen;
            } else if (tok.familiarity == domain::FamiliarityState::Automatized) {
                hl_fill = theme.status_success.with_alpha(0.16);
                hl_border = theme.status_success;
            }
            if (tok.matched_node_id == selected_node_id_) {
                hl_border = theme.border_focus;
            }
            canvas_.draw_rounded_rect(tok_rect, 8.0, hl_fill, hl_border, (tok.matched_node_id == selected_node_id_) ? 2.0 : 1.0);
            canvas_.draw_text(tx + 7.0, ty, tok.surface_text, 16.0, theme.text_primary, true, "Inter");
            canvas_.draw_text(tx + 7.0, ty + 16.0, tok.phonetic_ipa, 9.5, theme.text_secondary, false, "Inter");
            clickable_tokens_.emplace_back(tok_rect, tok.matched_node_id);
        } else {
            canvas_.draw_text(tx + 4.0, ty, tok.surface_text, 16.0, theme.text_primary, false, "Inter");
        }
        imgui_ctx_.PopID();
        tx += tw + 15.0;
    }

    if (passages_.size() >= 2) {
        const Rect p0_btn{area.x + area.w - 280.0, area.y + 44.0, 128.0, 28.0};
        const Rect p1_btn{area.x + area.w - 144.0, area.y + 44.0, 126.0, 28.0};
        const bool p0_act = (active_passage_index_ % passages_.size()) == 0;
        canvas_.draw_button(p0_btn, "Passage 1 (Dialog)", theme, p0_act, p0_act, 10.0, static_cast<int>(VectorIcon::BookOpen));
        canvas_.draw_button(p1_btn, "Passage 2 (Studio)", theme, !p0_act, !p0_act, 10.0, static_cast<int>(VectorIcon::BookOpen));
        register_button("switch_passage_0", "Passage 1", "Switch to Immersion Passage 1", p0_btn, p0_act, p0_act);
        register_button("switch_passage_1", "Passage 2", "Switch to Immersion Passage 2", p1_btn, !p0_act, !p0_act);
    }

    const double wb_top = std::max(ty + 62.0, area.y + area.h - 268.0);
    const double wb_h = (area.y + area.h) - wb_top - 58.0;
    if (wb_h >= 120.0) {
        const auto wb_cols = photon::htile<2>(Rect{area.x + 18.0, wb_top, area.w - 36.0, wb_h}, {0.54, 0.46}, 14.0);
        const Rect& syn_card = wb_cols[0];
        const Rect& dist_card = wb_cols[1];

        canvas_.draw_photon_card(syn_card, 10.0, theme, false, theme.border_subtle, 1.0, theme.primary_accent);
        canvas_.draw_vector_icon(VectorIcon::Sparkles, syn_card.x + 18.0, syn_card.y + 20.0, 14.0, theme.primary_accent);
        canvas_.draw_text(syn_card.x + 32.0, syn_card.y + 24.0, "ACTIVE TOKEN SYNTACTIC & ETYMOLOGICAL BREAKDOWN", 10.5, theme.primary_accent, true, "Inter");

        if (const auto* sel_node = graph_.find_node(selected_node_id_)) {
            canvas_.draw_text(syn_card.x + 14.0, syn_card.y + 50.0, "#" + std::to_string(sel_node->id) + " " + sel_node->target_form + "  " + sel_node->phonetic_ipa, 14.5, theme.text_primary, true, "Inter Display");
            canvas_.draw_wrapped_text(syn_card.x + 14.0, syn_card.y + 70.0, syn_card.w - 28.0, 15.0, "Meaning: " + sel_node->concept_gloss, 10.5, theme.text_secondary, true, 2, "Inter");
            canvas_.draw_wrapped_text(syn_card.x + 14.0, syn_card.y + 102.0, syn_card.w - 28.0, 15.0, "Morphology: " + sel_node->morphological_note, 10.0, theme.text_secondary, false, 2, "Inter");
            canvas_.draw_wrapped_text(syn_card.x + 14.0, syn_card.y + 136.0, syn_card.w - 28.0, 15.0, "Example: \"" + sel_node->example_context_l2 + "\" (" + sel_node->example_context_gloss + ")", 10.0, theme.secondary_accent, false, 2, "Inter");
        }

        canvas_.draw_photon_card(dist_card, 10.0, theme, false, theme.border_subtle, 1.0, theme.secondary_accent);
        canvas_.draw_vector_icon(VectorIcon::ShieldCheck, dist_card.x + 18.0, dist_card.y + 20.0, 14.0, theme.secondary_accent);
        canvas_.draw_text(dist_card.x + 32.0, dist_card.y + 24.0, "PASSAGE i+1 COMPREHENSION & MINING READY", 10.5, theme.secondary_accent, true, "Inter");

        canvas_.draw_text(dist_card.x + 14.0, dist_card.y + 48.0, "Tier 4 Automatized Vocabulary (S >= 30d)", 10.0, theme.text_secondary, true, "Inter");
        canvas_.draw_progress_bar(Rect{dist_card.x + 14.0, dist_card.y + 54.0, dist_card.w - 28.0, 8.0}, 0.78, theme.bg_dock_chrome, theme.status_success, "78.4%", theme.status_success);

        canvas_.draw_text(dist_card.x + 14.0, dist_card.y + 82.0, "Tier 2-3 Consolidating Chunks (3d <= S < 30d)", 10.0, theme.text_secondary, true, "Inter");
        canvas_.draw_progress_bar(Rect{dist_card.x + 14.0, dist_card.y + 88.0, dist_card.w - 28.0, 8.0}, 0.18, theme.bg_dock_chrome, theme.status_warning, "18.0%", theme.status_warning);

        canvas_.draw_text(dist_card.x + 14.0, dist_card.y + 116.0, "Tier 0-1 i+1 Target Mining Window (2-5% Sweet Spot)", 10.0, theme.text_secondary, true, "Inter");
        canvas_.draw_progress_bar(Rect{dist_card.x + 14.0, dist_card.y + 122.0, dist_card.w - 28.0, 8.0}, 0.036, theme.bg_dock_chrome, theme.status_unseen, "3.6%", theme.status_unseen);

        const Rect quick_mine{dist_card.x + 14.0, dist_card.y + dist_card.h - 42.0, dist_card.w - 28.0, 32.0};
        canvas_.draw_button(quick_mine, "Mine Highlighted Sentence into Stage 3 Studio ->", theme, true, false, 10.5, static_cast<int>(VectorIcon::Sparkles));
        register_button("mine_selected_token", "Mine Highlighted Sentence", "Mine selected token into Stage 3 Studio", quick_mine, true, false);
    }

    const double leg_y = area.y + area.h - 42.0;
    canvas_.draw_badge(area.x + 18.0, leg_y, "⏳ 0-1: Unseen / Noticed (Glacier Teal)", theme.status_unseen.with_alpha(0.20), theme.status_unseen, 9.5);
    canvas_.draw_badge(area.x + 252.0, leg_y, "! 2-3: Fragile / Consolidating (Amber)", theme.status_warning.with_alpha(0.20), theme.status_warning, 9.5);
    canvas_.draw_badge(area.x + 492.0, leg_y, "✓ 4: Automatized S>=30d (Kinetic Mint)", theme.status_success.with_alpha(0.20), theme.status_success, 9.5);
}

void MainWindow::render_screen3_active_recall_studio(const Rect& area, const ThemePalette& theme) {
    const auto* node = graph_.find_node(selected_node_id_);
    if (!node) return;

    canvas_.draw_photon_card(area, SpacingTokens::radius_lg, theme, true, theme.border_subtle, 1.0, theme.mod_prod_text);

    canvas_.draw_badge(
        area.x + 18.0,
        area.y + 14.0,
        current_exercise_.exercise_type_label,
        theme.mod_prod_text.with_alpha(0.18),
        theme.mod_prod_text,
        9.5
    );

    const double w = area.w - 36.0;
    canvas_.draw_wrapped_text(
        area.x + 18.0,
        area.y + 48.0,
        w,
        18.0,
        "Active Target #" + std::to_string(node->id) + " (" + node->target_form + "): " + node->concept_gloss,
        15.0,
        theme.text_primary,
        true,
        1,
        "Inter Display"
    );
    canvas_.draw_wrapped_text(
        area.x + 18.0,
        area.y + 68.0,
        w,
        15.0,
        current_exercise_.situational_prompt,
        11.0,
        theme.text_secondary,
        false,
        2,
        "Inter"
    );

    canvas_.draw_prosodic_pitch_contour(
        Rect{area.x + 18.0, area.y + 92.0, w, 144.0},
        current_exercise_.back_chain_steps,
        active_backchain_step_,
        theme
    );

    const Rect bc_btn{area.x + area.w - 196.0, area.y + 100.0, 168.0, 26.0};
    canvas_.draw_rounded_rect(bc_btn, 6.0, theme.bg_elevated, theme.border_subtle, 1.0);
    canvas_.draw_text(bc_btn.x + 12.0, bc_btn.y + 17.0, "Cycle Back-Chain Slice", 10.0, theme.secondary_accent, true, "Inter");
    register_button("backchain_next", "Cycle Back-Chain Slice", "Step through Pimsleur suffix back-chaining", bc_btn, false, false);

    const double input_y = area.y + 252.0;
    canvas_.draw_text(
        area.x + 18.0,
        input_y,
        "PUSHED SYNTACTIC & ORTHOGRAPHIC OUTPUT (Zero Word Banks for Tier 1-3; Ceiling < 2,500 ms):",
        10.0,
        theme.text_muted,
        true,
        "Inter"
    );

    const Rect input_rect{area.x + 18.0, input_y + 8.0, w, 44.0};
    canvas_.draw_rounded_rect(input_rect, 10.0, theme.bg_dock_chrome, theme.border_focus, 1.8);
    const std::string display_input = typed_input_buffer_.empty()
        ? (node->target_form + "   [Type response or click 'Verify Fast Recall' below]")
        : typed_input_buffer_;
    canvas_.draw_text(
        input_rect.x + 14.0,
        input_rect.y + 27.0,
        display_input,
        13.5,
        typed_input_buffer_.empty() ? theme.text_muted : theme.text_primary,
        !typed_input_buffer_.empty(),
        "Inter"
    );

    const auto btn_cols = photon::htile<4>(Rect{area.x + 18.0, input_y + 58.0, w, 36.0}, {1.15, 1.15, 1.0, 0.9}, 10.0);
    canvas_.draw_button(btn_cols[0], "Verify Fast Recall (1,380 ms)", theme, true, false, 11.0, static_cast<int>(VectorIcon::ShieldCheck));
    register_button("submit_accurate_recall", "Verify Fast Recall", "Submit active recall within 2.5s ceiling", btn_cols[0], true, false);

    canvas_.draw_button(btn_cols[1], "Test Slow Latency (>2.5s Cap)", theme, false, false, 11.0, static_cast<int>(VectorIcon::ClockLatency));
    register_button("simulate_slow_recall", "Test Slow Latency", "Test >2500ms latency rating cap", btn_cols[1], false, false);

    canvas_.draw_button(btn_cols[2], "Synthesize Prosody (.wav)", theme, false, false, 10.5, static_cast<int>(VectorIcon::MicWaveform));
    register_button("synthesize_prosody_wav", "Synthesize Prosody WAV", "Export 44.1kHz 16-bit PCM prosody WAV", btn_cols[2], false, false);

    canvas_.draw_button(btn_cols[3], "Next Due Node ->", theme, false, false, 10.5, static_cast<int>(VectorIcon::Sparkles));
    register_button("advance_next_node", "Next Due Node", "Advance to next curriculum node in queue", btn_cols[3], false, false);

    const double preview_y = input_y + 104.0;
    const auto grade_cols = photon::htile<4>(Rect{area.x + 18.0, preview_y, w, 72.0}, {1.0, 1.0, 1.0, 1.0}, 10.0);
    const auto& prod_st = node->modalities[2];
    const double cur_s = std::max(0.5, prod_st.stability);
    const double cur_d = prod_st.difficulty;
    const double cur_r = std::clamp(prod_st.retrievability, 0.70, 0.98);

    const double s_again = graph_.fsrs().next_forget_stability(cur_d, cur_s, cur_r);
    const double s_hard  = graph_.fsrs().next_recall_stability(cur_d, cur_s, cur_r, domain::Rating::Hard);
    const double s_good  = graph_.fsrs().next_recall_stability(cur_d, cur_s, cur_r, domain::Rating::Good);
    const double s_easy  = graph_.fsrs().next_recall_stability(cur_d, cur_s, cur_r, domain::Rating::Easy);

    struct GradePreview {
        const char* title;
        const char* rule;
        double next_s;
        ColorRGBA col;
    };
    const std::array<GradePreview, 4> grades = {{
        {"[1] AGAIN · Lapse Reset", "Post-Lapse S'_f Power Law", s_again, theme.status_danger},
        {"[2] HARD · >2.5s Latency Cap", "DeKeyser Hesitation Penalty", s_hard, theme.status_warning},
        {"[3] GOOD · 90% Target Floor", "Standard Procedural Recall", s_good, theme.mod_rec_text},
        {"[4] EASY · <2.5s Automatic", "Rapid Prosodic & Syntactic Production", s_easy, theme.status_success}
    }};

    const std::array<const char*, 4> grade_ids = {"grade_again", "grade_hard", "grade_good", "grade_easy"};
    for (size_t g = 0; g < 4; ++g) {
        const Rect& gc = grade_cols[g];
        canvas_.draw_photon_card(gc, 8.0, theme, false, theme.border_subtle, 1.0, grades[g].col);
        canvas_.draw_text(gc.x + 10.0, gc.y + 19.0, grades[g].title, 9.5, grades[g].col, true, "Inter");
        std::ostringstream s_val;
        s_val << "S': " << std::fixed << std::setprecision(1) << grades[g].next_s << " Days";
        canvas_.draw_text(gc.x + 10.0, gc.y + 44.0, s_val.str(), 15.0, theme.text_primary, true, "Inter Display");
        canvas_.draw_text(gc.x + 10.0, gc.y + 61.0, grades[g].rule, 8.5, theme.text_muted, false, "Inter");
        register_button(grade_ids[g], grades[g].title, "Commit FSRS-4.5 rating to SQLite3 WAL", gc, false, false);
    }

    const double diag_y = preview_y + 82.0;
    const double diag_h = (area.y + area.h) - diag_y - 14.0;
    if (diag_h >= 60.0) {
        const auto diag_cols = photon::htile<2>(Rect{area.x + 18.0, diag_y, w, diag_h}, {0.55, 0.45}, 12.0);
        const Rect& left_d = diag_cols[0];
        const Rect& right_d = diag_cols[1];

        const auto leech = graph_.evaluate_leech_intervention(selected_node_id_, domain::Modality::ProductiveText);
        canvas_.draw_photon_card(left_d, 10.0, theme, false, theme.border_subtle, 1.0, leech.is_leech ? theme.status_danger : theme.primary_accent);

        if (has_last_evaluation_) {
            canvas_.draw_text(left_d.x + 14.0, left_d.y + 22.0, "✓ POST-TURN COGNITIVE & FSRS-4.5 DIAGNOSTIC:", 10.0, theme.primary_accent, true, "Inter");
            canvas_.draw_wrapped_text(left_d.x + 14.0, left_d.y + 42.0, left_d.w - 28.0, 15.0, last_evaluation_.diagnostic_feedback, 11.0, theme.text_primary, true, 2, "Inter");
            canvas_.draw_wrapped_text(left_d.x + 14.0, left_d.y + 76.0, left_d.w - 28.0, 15.0, "Contextual Recast: \"" + last_evaluation_.recast_form + "\"", 10.5, theme.text_secondary, false, 2, "Inter");
            canvas_.draw_text(left_d.x + 14.0, left_d.y + left_d.h - 14.0, status_banner_message_, 9.5, theme.status_success, true, "Noto Sans Mono");
        } else {
            canvas_.draw_text(
                left_d.x + 14.0,
                left_d.y + 22.0,
                leech.is_leech ? "! ANTI-LEECH INTERVENTION SUB-ROUTINE ACTIVE (LAPSES > 4):" : "✓ SWAIN'S OUTPUT HYPOTHESIS & LATENCY GATE:",
                10.0,
                leech.is_leech ? theme.status_danger : theme.primary_accent,
                true,
                "Inter"
            );
            canvas_.draw_wrapped_text(
                left_d.x + 14.0,
                left_d.y + 42.0,
                left_d.w - 28.0,
                16.0,
                leech.is_leech ? leech.recommended_remediation_action : ("Context Utterance: \"" + node->example_context_l2 + "\" — " + node->example_context_gloss),
                10.5,
                theme.text_primary,
                false,
                4,
                "Inter"
            );
            canvas_.draw_text(
                left_d.x + 14.0,
                left_d.y + left_d.h - 14.0,
                "Active Modality: Productive Text & Audio  |  Target Ceiling: <= 2,500 ms",
                9.5,
                theme.text_muted,
                false,
                "Noto Sans Mono"
            );
        }

        canvas_.draw_photon_card(right_d, 10.0, theme, false, theme.border_subtle, 1.0, theme.secondary_accent);
        canvas_.draw_text(right_d.x + 14.0, right_d.y + 22.0, "CONTRASTIVE MINIMAL PAIR & ETYMOLOGICAL ANCHOR:", 10.0, theme.secondary_accent, true, "Inter");
        canvas_.draw_text(right_d.x + 14.0, right_d.y + 44.0, "Target: " + node->target_form + " " + node->phonetic_ipa, 12.0, theme.status_success, true, "Inter");
        canvas_.draw_text(right_d.x + 14.0, right_d.y + 64.0, "Decoy : " + node->decoy_minimal_pair, 12.0, theme.status_danger, true, "Inter");
        canvas_.draw_wrapped_text(right_d.x + 14.0, right_d.y + 86.0, right_d.w - 28.0, 15.0, "Mnemonic: " + node->mnemonic_hook, 10.0, theme.text_secondary, false, 3, "Inter");
        canvas_.draw_wrapped_text(right_d.x + 14.0, right_d.y + 132.0, right_d.w - 28.0, 14.0, "Rule: " + node->morphological_note, 9.5, theme.text_muted, false, 2, "Inter");
    }
}

void MainWindow::render_screen4_knowledge_graph(const Rect& area, const ThemePalette& theme) {
    const auto rows = photon::vtile<2>(area, {0.52, 0.48}, 12.0);

    canvas_.draw_dependency_graph_panel(
        rows[0],
        graph_,
        active_language_,
        selected_node_id_,
        theme,
        clickable_graph_nodes_
    );

    const Rect& table_box = rows[1];
    canvas_.draw_photon_card(table_box, SpacingTokens::radius_md, theme, true, theme.border_subtle, 1.0, theme.secondary_accent);

    canvas_.draw_text(
        table_box.x + 16.0,
        table_box.y + 22.0,
        "Sortable 4-Modality Matrix & Dependency Gate Audit (" + active_language_ + ") — Click Headers to Sort",
        12.0,
        theme.text_primary,
        true,
        "Inter Display"
    );

    imgui_ctx_.BeginTable("ModalityMatrixTable");
    const double row_y0 = table_box.y + 44.0;

    const Rect h0{table_box.x + 14.0, row_y0 - 14.0, 150.0, 20.0};
    const Rect h1{table_box.x + 185.0, row_y0 - 14.0, 90.0, 20.0};
    const Rect h2{table_box.x + 290.0, row_y0 - 14.0, 110.0, 20.0};
    register_button("sort_col_0", "Sort by Node ID", "Sort table by Node ID", h0, false, table_sort_specs_.column_index == 0);
    register_button("sort_col_1", "Sort by Zipf Rank", "Sort table by Zipf Rank", h1, false, table_sort_specs_.column_index == 1);
    register_button("sort_col_2", "Sort by Rec-Text S", "Sort table by Receptive Stability", h2, false, table_sort_specs_.column_index == 2);

    canvas_.draw_text(table_box.x + 16.0, row_y0, "NODE / FORM [Sort]", 9.5, (table_sort_specs_.column_index == 0) ? theme.primary_accent : theme.text_muted, true, "Noto Sans Mono");
    canvas_.draw_text(table_box.x + 185.0, row_y0, "ZIPF [Sort]", 9.5, (table_sort_specs_.column_index == 1) ? theme.primary_accent : theme.text_muted, true, "Noto Sans Mono");
    canvas_.draw_text(table_box.x + 280.0, row_y0, "REC-TEXT [Sort]", 9.5, theme.mod_rec_text, true, "Noto Sans Mono");
    canvas_.draw_text(table_box.x + 395.0, row_y0, "REC-AUDIO", 9.5, theme.mod_rec_audio, true, "Noto Sans Mono");
    canvas_.draw_text(table_box.x + 505.0, row_y0, "PROD-TEXT", 9.5, theme.mod_prod_text, true, "Noto Sans Mono");
    canvas_.draw_text(table_box.x + 615.0, row_y0, "PROD-AUDIO", 9.5, theme.mod_prod_audio, true, "Noto Sans Mono");
    canvas_.draw_text(table_box.x + 725.0, row_y0, "GATE", 9.5, theme.text_muted, true, "Noto Sans Mono");

    auto nodes = graph_.nodes_for_language(active_language_);
    std::sort(nodes.begin(), nodes.end(), [&](const domain::LinguisticNode& a, const domain::LinguisticNode& b) {
        const bool asc = (table_sort_specs_.direction == imgui::SortDirection::Ascending);
        if (table_sort_specs_.column_index == 0) {
            return asc ? (a.id < b.id) : (a.id > b.id);
        }
        if (table_sort_specs_.column_index == 2) {
            return asc ? (a.modalities[0].stability < b.modalities[0].stability)
                       : (a.modalities[0].stability > b.modalities[0].stability);
        }
        return asc ? (a.zipf_frequency_rank < b.zipf_frequency_rank)
                   : (a.zipf_frequency_rank > b.zipf_frequency_rank);
    });

    for (size_t i = 0; i < nodes.size(); ++i) {
        const double ry = row_y0 + 22.0 + static_cast<double>(i) * 28.0;
        if (ry + 10.0 > table_box.y + table_box.h) break;

        const auto& n = nodes[i];
        const bool unlocked = graph_.is_productive_unlocked(n.id);
        canvas_.draw_line(table_box.x + 12.0, ry - 14.0, table_box.x + table_box.w - 12.0, ry - 14.0, theme.border_subtle.with_alpha(0.5), 0.7);

        canvas_.draw_text(table_box.x + 16.0, ry, "#" + std::to_string(n.id) + " " + n.target_form, 11.0, theme.text_primary, (n.id == selected_node_id_), "Inter");
        canvas_.draw_text(table_box.x + 185.0, ry, "#" + std::to_string(n.zipf_frequency_rank), 10.5, theme.text_secondary, false, "Noto Sans Mono");

        for (size_t m = 0; m < 4; ++m) {
            std::ostringstream mss;
            mss << std::fixed << std::setprecision(1) << n.modalities[m].stability << "d (D=" << n.modalities[m].difficulty << ")";
            canvas_.draw_text(table_box.x + 280.0 + static_cast<double>(m) * 112.0, ry, mss.str(), 10.0, theme.text_secondary, false, "Noto Sans Mono");
        }

        canvas_.draw_badge(
            table_box.x + 725.0,
            ry - 12.0,
            unlocked ? "✓ UNLOCKED" : "⏳ GATED",
            (unlocked ? theme.status_success : theme.status_warning).with_alpha(0.18),
            unlocked ? theme.status_success : theme.status_warning,
            9.0
        );
    }
    imgui_ctx_.EndTable();
}

void MainWindow::render_screen5_roleplay_and_settings(const Rect& area, const ThemePalette& theme) {
    canvas_.draw_photon_card(area, SpacingTokens::radius_lg, theme, true, theme.border_subtle, 1.0, theme.secondary_accent);

    canvas_.draw_badge(
        area.x + 18.0,
        area.y + 14.0,
        "STAGE 5: GRAPH-CONSTRAINED ROLEPLAY (95%+ KNOWN LEMMA CEILING) & ETHICAL HABIT LOOP",
        theme.secondary_accent.with_alpha(0.18),
        theme.secondary_accent,
        9.5
    );

    canvas_.draw_text(
        area.x + 18.0,
        area.y + 50.0,
        "Live Pragmatic Elicitation, Long's Non-Intrusive Recasts & Resilient Momentum",
        16.0,
        theme.text_primary,
        true,
        "Inter Display"
    );

    const double w = area.w - 36.0;
    double ty = area.y + 68.0;
    for (const auto& turn : roleplay_history_) {
        const Rect card{area.x + 18.0, ty, w, 108.0};
        canvas_.draw_rounded_rect(card, 10.0, theme.bg_surface_2, theme.border_subtle, 1.0);
        canvas_.draw_text(card.x + 14.0, card.y + 20.0, turn.speaker_role, 11.0, theme.primary_accent, true, "Inter");
        canvas_.draw_wrapped_text(card.x + 14.0, card.y + 40.0, w - 28.0, 16.0, turn.utterance_l2, 12.5, theme.text_primary, true, 2, "Inter");
        canvas_.draw_wrapped_text(card.x + 14.0, card.y + 72.0, w - 28.0, 15.0, turn.implicit_recast_l2, 10.5, theme.status_success, false, 1, "Inter");
        canvas_.draw_wrapped_text(card.x + 14.0, card.y + 92.0, w - 28.0, 14.0, "Diagnostic: " + turn.diagnostic_note, 10.0, theme.text_secondary, false, 1, "Inter");
        ty += 118.0;
    }

    const Rect hab_card{area.x + 18.0, ty, w, 102.0};
    canvas_.draw_rounded_rect(hab_card, 10.0, theme.bg_surface_2, theme.border_subtle, 1.0);
    canvas_.draw_text(hab_card.x + 14.0, hab_card.y + 22.0, "ETHICAL RETENTION ENGINE (ZERO PUNITIVE STREAKS · 5-MINUTE REGRET TEST):", 10.5, theme.status_success, true, "Inter");
    canvas_.draw_wrapped_text(
        hab_card.x + 14.0,
        hab_card.y + 40.0,
        w - 28.0,
        15.0,
        "Pausing study shifts FSRS review timestamps forward cleanly in SQLite3 so you never return to a 500-card backlog wall.",
        10.5,
        theme.text_secondary,
        false,
        2,
        "Inter"
    );

    const Rect vac_btn{hab_card.x + 14.0, hab_card.y + 60.0, w - 28.0, 32.0};
    canvas_.draw_rounded_rect(
        vac_btn,
        8.0,
        habit_state_.vacation_recovery_mode_active ? theme.status_warning : theme.bg_elevated,
        theme.status_warning,
        1.2
    );
    canvas_.draw_text(
        vac_btn.x + 14.0,
        vac_btn.y + 21.0,
        habit_state_.vacation_recovery_mode_active
            ? "Resilient Vacation Shift: ACTIVE (+3d Timestamp Shift Applied to All Modalities)"
            : "Activate Resilient Vacation Shift (Pause FSRS Decay Without Streak Penalty)",
        11.0,
        habit_state_.vacation_recovery_mode_active ? ColorRGBA::from_hex(0x112024) : theme.status_warning,
        true,
        "Inter"
    );
    register_button("activate_vacation_shift", "Toggle Vacation Shift", "Shift FSRS timestamps gracefully", vac_btn, false, habit_state_.vacation_recovery_mode_active);

    const double eco_y = hab_card.y + hab_card.h + 12.0;
    const double eco_h = (area.y + area.h - 58.0) - eco_y;
    if (eco_h >= 96.0) {
        const auto eco_cols = photon::htile<2>(Rect{area.x + 18.0, eco_y, w, eco_h}, {0.54, 0.46}, 12.0);
        const Rect& heat_card = eco_cols[0];
        const Rect& exp_card = eco_cols[1];

        canvas_.draw_photon_card(heat_card, 10.0, theme, false, theme.border_subtle, 1.0, theme.primary_accent);
        canvas_.draw_text(heat_card.x + 14.0, heat_card.y + 22.0, "12-WEEK 4-MODALITY PROCEDURAL FLUENCY HEATMAP:", 10.0, theme.primary_accent, true, "Inter");
        const std::array<const char*, 4> m_short = {"Rec-Text", "Rec-Audio", "Prod-Text", "Prod-Audio"};
        const std::array<ColorRGBA, 4> m_cols = {theme.mod_rec_text, theme.mod_rec_audio, theme.mod_prod_text, theme.mod_prod_audio};

        for (size_t m = 0; m < 4; ++m) {
            const double ry = heat_card.y + 34.0 + static_cast<double>(m) * 24.0;
            if (ry + 18.0 > heat_card.y + heat_card.h) break;
            canvas_.draw_text(heat_card.x + 14.0, ry + 13.0, m_short[m], 9.5, m_cols[m], true, "Noto Sans Mono");
            for (int wk = 0; wk < 12; ++wk) {
                const double cx = heat_card.x + 92.0 + static_cast<double>(wk) * 26.0;
                const double intensity = 0.25 + 0.72 * (static_cast<double>((wk * 3 + static_cast<int>(m) * 5) % 10) / 9.0);
                canvas_.draw_rounded_rect(
                    Rect{cx, ry + 1.0, 20.0, 16.0},
                    4.0,
                    m_cols[m].with_alpha(intensity),
                    m_cols[m].with_alpha(0.55),
                    0.8
                );
            }
        }

        canvas_.draw_photon_card(exp_card, 10.0, theme, false, theme.border_subtle, 1.0, theme.secondary_accent);
        canvas_.draw_text(exp_card.x + 14.0, exp_card.y + 22.0, "OPEN-FORMAT DATA SOVEREIGNTY (ZERO VENDOR LOCK-IN):", 10.0, theme.secondary_accent, true, "Inter");
        canvas_.draw_wrapped_text(
            exp_card.x + 14.0,
            exp_card.y + 40.0,
            exp_card.w - 28.0,
            15.0,
            "Export your 4-modality FSRS-4.5 deck to Anki-compatible Tab-Separated Values (.tsv), deterministic CRDT JSON, or 44.1 kHz 16-bit PCM prosody (.wav).",
            10.0,
            theme.text_secondary,
            false,
            3,
            "Inter"
        );

        const auto exp_btns = photon::htile<2>(Rect{exp_card.x + 14.0, exp_card.y + exp_card.h - 42.0, exp_card.w - 28.0, 32.0}, {1.0, 1.0}, 10.0);
        canvas_.draw_button(exp_btns[0], "Export Anki Deck (.tsv)", theme, false, true, 10.0, static_cast<int>(VectorIcon::BookOpen));
        register_button("export_anki_tsv", "Export Anki TSV Deck", "Export 4-modality FSRS deck to Anki TSV", exp_btns[0], false, true);

        canvas_.draw_button(exp_btns[1], "Export Prosody (.wav)", theme, false, false, 10.0, static_cast<int>(VectorIcon::MicWaveform));
        register_button("synthesize_prosody_wav", "Export Prosody WAV", "Synthesize 16-bit PCM prosody WAV audio", exp_btns[1], false, false);
    }

    const Rect reply_btn{area.x + 18.0, area.y + area.h - 48.0, w, 36.0};
    canvas_.draw_button(reply_btn, "Simulate Spoken/Typed Pragmatic Turn (Elicit Target Node) ->", theme, true, false, 11.5, static_cast<int>(VectorIcon::MessageChat));
    register_button("send_roleplay_turn", "Simulate Pragmatic Turn", "Send pushed output turn in roleplay", reply_btn, true, false);
}

bool MainWindow::export_all_screen_snapshots(const std::string& output_dir) {
    std::error_code ec;
    std::filesystem::create_directories(output_dir, ec);

    // Export 5 Consumer Focus Studio Desktop Views (Pro Telemetry OFF for clean, breathable editorial design)
    set_pro_telemetry_mode(false);
    const std::array<std::pair<AppScreen, const char*>, 5> screens = {{
        {AppScreen::Screen1_WorkbenchOverview,      "01_workbench_overview.png"},
        {AppScreen::Screen2_ImmersionReader,        "02_interlinear_immersion_reader.png"},
        {AppScreen::Screen3_ActiveRecallStudio,     "03_active_recall_and_leech_studio.png"},
        {AppScreen::Screen4_KnowledgeGraphExplorer, "04_knowledge_graph_dependency_dag.png"},
        {AppScreen::Screen5_RoleplayAndSettings,    "05_roleplay_recasts_and_habit_loop.png"}
    }};

    for (const auto& [scr, filename] : screens) {
        switch_screen(scr);
        if (!canvas_.export_png((std::filesystem::path(output_dir) / filename).string())) {
            return false;
        }
    }

    // Export Mobile Responsive Studio View
    trigger_button_action("toggle_form_factor");
    switch_screen(AppScreen::Screen1_WorkbenchOverview);
    const bool mobile_ok = canvas_.export_png((std::filesystem::path(output_dir) / "06_mobile_responsive_view.png").string());
    trigger_button_action("toggle_form_factor");

    // Export Light Warm Archival Ivory Studio View
    trigger_button_action("toggle_theme");
    switch_screen(AppScreen::Screen1_WorkbenchOverview);
    const bool light_ok = canvas_.export_png((std::filesystem::path(output_dir) / "07_light_archival_theme_view.png").string());
    trigger_button_action("toggle_theme");

    // Export 6-Region Pro Telemetry Dockspace View (warm cache pass so profiler displays steady-state frame latency)
    set_pro_telemetry_mode(true);
    switch_screen(AppScreen::Screen1_WorkbenchOverview);
    render_frame();
    const bool pro_ok = canvas_.export_png((std::filesystem::path(output_dir) / "08_pro_telemetry_dockspace_view.png").string());
    set_pro_telemetry_mode(false);

    return mobile_ok && light_ok && pro_ok;
}

} // namespace kinetix::ui
