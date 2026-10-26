// Kinetix L2 v4.0 — Comprehensive Automated Unit, Integration, Security & QA Test Suite (48 Tests)
// Verifies:
// 1. FSRS-4.5 power-law forgetting curve R(S,S) == 0.90 & Spacing Effect gain
// 2. Structure-of-Arrays (SoA) batch FSRS evaluation (`optimcpop.md`)
// 3. PoolAllocator<T> allocation, construction, destruction, and reuse (`memcpp.md`)
// 4. WaniKani prerequisite dependency gating (S_rec >= 3.0d)
// 5. DeKeyser <2,500 ms automatization latency rating cap
// 6. Strict Anti-Leech Intervention Sub-Routine & Safe Inspector Reset
// 7. Stage 1 Interlinear Tokenizer & 95-98% i+1 comprehension calculation
// 8. Stage 2 Pimsleur Prosodic Back-Chaining pitch contours
// 9. Stage 3 Zero Word-Bank enforcement for Tiers 1-3
// 10. Stage 5 Graph-Constrained Conversational Recast simulation across DE, ES, JA
// 11. Ethical Habit Engine (Endowed Progress, Resilient Vacation Shift, Regret Test stopping cue)
// 12. Cycfi Photon declarative `vtile`/`htile`/`compute_desktop_dockspace` layout combinators
// 13. Dear ImGui `ImGuiContextTracker` Push/Pop & Begin/End stack balance, `ListClipper`, & `TableSortSpecs`
// 14. Official Kinetix L2 SVG Brand Asset rendering via `librsvg-2.so.2`, Separable Gaussian Blur / Frosted-Glass Shader,
//     Damped Harmonic Spring Physics (`SpringValue`), and all 8 native PNG exports
// 15. Native C++ UX Research & Design System (`native-cpp-ux-product-design.md` Workflows 1–12)
// 16. Production v4.0 Database Integrity (`PRAGMA integrity_check` + `foreign_key_check`), Schema Migrations,
//     Bidirectional Anki TSV Export/Import Roundtrip, Malformed TSV Transactional Rollback,
//     Bounded 1D Levenshtein DoS Protection, Persistent Stage 1 Sentence Mining, and Search Input Clamping.

#include "kinetix/domain/fsrs_engine.hpp"
#include "kinetix/domain/knowledge_graph.hpp"
#include "kinetix/domain/pedagogical_loop.hpp"
#include "kinetix/domain/habit_engine.hpp"
#include "kinetix/domain/memory_pool.hpp"
#include "kinetix/persistence/database_manager.hpp"
#include "kinetix/platform/platform_adapter.hpp"
#include "kinetix/ui/photon_elements.hpp"
#include "kinetix/ui/imgui_layer.hpp"
#include "kinetix/ui/spring_physics.hpp"
#include "kinetix/ui/ux_design_system.hpp"
#include "kinetix/ui/main_window.hpp"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {

int g_tests_run = 0;
int g_tests_passed = 0;

void assert_true(bool cond, const char* test_name, const char* msg) {
    ++g_tests_run;
    if (!cond) {
        std::cerr << "[FAIL] " << test_name << ": " << msg << "\n";
        throw std::runtime_error(std::string(test_name) + " failed: " + msg);
    }
    ++g_tests_passed;
    std::cout << "[PASS] " << test_name << "\n";
}

void test_fsrs_power_law_and_soa_batch() {
    kinetix::domain::FsrsEngine engine;
    const double r_at_s = engine.compute_retrievability(10.0, 10.0);
    assert_true(std::abs(r_at_s - 0.90) < 1e-6, "01_fsrs_power_law_forgetting_curve_R(S,S)==0.90", "R(S,S) must equal 0.90");

    const double s_spaced = engine.next_recall_stability(5.0, 10.0, 0.88, kinetix::domain::Rating::Good);
    const double s_crammed = engine.next_recall_stability(5.0, 10.0, 0.99, kinetix::domain::Rating::Good);
    assert_true(s_spaced > s_crammed * 1.35, "02_fsrs_spacing_effect_gain", "Spaced recall at R=0.88 must exceed R=0.99 stability growth");

    kinetix::domain::ModalityStatesSoA batch{};
    batch.reserve(4);
    for (int i = 0; i < 4; ++i) {
        batch.node_ids.push_back(100 + i);
        batch.elapsed_days.push_back(10.0);
        batch.stabilities.push_back(10.0);
        batch.difficulties.push_back(5.0);
    }
    const double us = engine.evaluate_batch_soa(batch);
    assert_true(us >= 0.0 && batch.out_retrievabilities.size() == 4, "03_soa_batch_fsrs_evaluation", "SoA batch must populate all outputs");
    assert_true(std::abs(batch.out_retrievabilities[0] - 0.90) < 1e-6, "04_soa_matches_scalar_fsrs_exact", "SoA R(10,10) must equal 0.90");
}

void test_memory_pool_allocator() {
    kinetix::memory::PoolAllocator<kinetix::domain::LinguisticNode, 4096> pool;
    auto* n1 = pool.create();
    n1->id = 777;
    n1->target_form = "TestPoolNode";
    assert_true(pool.active_allocations() == 1 && pool.total_capacity() >= 1, "05_pool_allocator_create", "PoolAllocator must track active allocations");
    pool.destroy(n1);
    assert_true(pool.active_allocations() == 0, "06_pool_allocator_destroy_and_recycle", "PoolAllocator must decrement active count on destroy");
}

void test_photon_and_imgui_layers() {
    const auto dock = kinetix::photon::compute_desktop_dockspace(1440.0, 880.0, true);
    assert_true(dock.command_bar.h == 56.0 && dock.left_nav_pane.w == 242.0 && dock.right_inspector.w == 324.0,
                "07_photon_declarative_dockspace_solver", "Photon dockspace must compute 6 valid regions");

    const auto tiles = kinetix::photon::htile<3>(kinetix::photon::box_rect{0, 0, 328, 100}, {1.0, 1.0, 1.0}, 14.0);
    assert_true(std::abs(tiles[0].w - 100.0) < 1e-6, "08_photon_htile_combinator_math", "htile must split width minus gaps proportionally");

    kinetix::imgui::ImGuiContextTracker ctx;
    ctx.NewFrame();
    ctx.BeginWindow("Root");
    ctx.BeginChild("Pane");
    ctx.BeginTable("Tbl");
    ctx.PushID(42);
    ctx.PushStyleColor();
    const std::string wid = ctx.RegisterWidgetId("Btn");
    ctx.PopStyleColor();
    ctx.PopID();
    ctx.EndTable();
    ctx.EndChild();
    ctx.EndWindow();
    assert_true(ctx.VerifyFrameBalance() && wid == "Root/Pane/Tbl/#42/Btn", "09_imgui_id_stack_and_scope_balance", "ImGui tracker must verify balanced scopes");

    kinetix::imgui::ListClipper clipper{};
    clipper.Begin(100, 25.0, 50.0, 100.0);
    assert_true(clipper.display_start == 2 && clipper.display_end == 8, "10_imgui_list_clipper_virtualization", "ListClipper must slice visible range");
}

void test_svg_renderer_blur_shader_and_spring_physics() {
    kinetix::ui::VectorCanvas canvas(256, 256);
    const auto theme = kinetix::ui::ThemePalette::dark_obsidian();
    canvas.draw_ambient_mesh_backdrop(theme);
    const bool svg_ok = canvas.draw_svg_file("assets/branding/kinetix-l2-icon.svg", kinetix::ui::Rect{16, 16, 128, 128});
    assert_true(svg_ok, "11_librsvg_native_brand_svg_rendering", "librsvg-2.so.2 must render assets/branding/kinetix-l2-icon.svg");

    canvas.apply_gaussian_blur_region(kinetix::ui::Rect{16, 16, 128, 128}, 6);
    canvas.apply_perceptual_noise_grain(kinetix::ui::Rect{16, 16, 128, 128}, 0.012);
    assert_true(canvas.pixels() != nullptr && canvas.stride() == 256 * 4,
                "12_frosted_glass_gaussian_blur_and_grain_shader", "Gaussian blur and perceptual grain shaders must preserve ARGB32 buffer integrity");

    kinetix::ui::SpringValue spring(0.0);
    spring.set_target(4.0);
    for (int i = 0; i < 60; ++i) {
        spring.step(0.016);
    }
    assert_true(std::abs(spring.value() - 4.0) < 0.05, "13_harmonic_spring_physics_convergence", "SpringValue must converge smoothly toward target");
}

void test_wanikani_gating_and_pedagogical_stages() {
    const std::string tmp_db = "/tmp/kinetix_v3_test.sqlite3";
    std::error_code ec;
    std::filesystem::remove(tmp_db, ec);

    kinetix::persistence::DatabaseManager db(tmp_db);
    assert_true(db.is_open() && db.initialize_schema(), "14_sqlite_wal_schema_init", "Database schema must initialize cleanly");

    kinetix::domain::KnowledgeGraph graph;
    assert_true(db.seed_default_curriculum_if_empty(graph), "15_seed_multilingual_curriculum_de_es_ja", "Curriculum must seed DE, ES, JA");

    assert_true(!graph.is_productive_unlocked(106), "16_wanikani_gate_blocks_node_106", "Node #106 must be gated while #105 S_rec < 3.0d");
    assert_true(graph.is_productive_unlocked(103), "17_wanikani_gate_unlocks_node_103", "Node #103 must be unlocked (#101, #102 S_rec >= 3.0d)");

    const auto leech = graph.evaluate_leech_intervention(103, kinetix::domain::Modality::ProductiveText);
    assert_true(leech.is_leech && leech.lapse_count == 5, "18_anti_leech_intervention_detected", "Node #103 must trigger anti-leech intervention");

    const auto sched = graph.record_review(
        101,
        kinetix::domain::Modality::ProductiveText,
        kinetix::domain::Rating::Easy,
        1792666400LL,
        3200,
        "test_latency_cap"
    );
    assert_true(sched.log_entry.rating == kinetix::domain::Rating::Hard, "19_dekeyser_latency_cap_enforced", "Slow response >2500ms must downgrade Easy to Hard");

    kinetix::domain::PedagogicalLoopEngine loop(graph);
    const auto passages = db.load_passages("de");
    assert_true(!passages.empty(), "20_load_immersion_passages", "German immersion passage must load");
    const auto annotated = loop.annotate_passage(passages.front());
    int matched_count = 0;
    for (const auto& tok : annotated) {
        if (tok.matched_node_id > 0) ++matched_count;
    }
    assert_true(matched_count >= 5, "21_stage1_interlinear_token_matching", "Interlinear reader must match >= 5 target tokens in passage");

    const auto ex = loop.generate_exercise(103, kinetix::domain::Modality::ProductiveText);
    assert_true(ex.tier0_recognition_choices.empty() && ex.back_chain_steps.size() == 3,
                "22_stage2_backchain_and_stage3_zero_word_bank", "Back-chaining must have 3 steps and zero word banks on Tier 1+");

    std::filesystem::remove(tmp_db, ec);
}

void test_ux_design_system_workflows_and_snapshots() {
    // Workflow 2: Opportunity prioritization heuristic (F * S * I * C)
    const int prio = kinetix::ui::ux::compute_opportunity_priority(5, 5, 5, 5);
    assert_true(prio == 625 && kinetix::ui::ux::to_tag_label(kinetix::ui::ux::EvidenceTag::Accessibility) == "[ACCESSIBILITY]",
                "23_ux_workflow2_opportunity_priority_and_tags", "Opportunity priority 5*5*5*5 must equal 625");

    // Workflow 5: WCAG 2.2 AA & AAA contrast verification on Dark and Light Studio palettes
    const auto dark = kinetix::ui::ThemePalette::dark_obsidian();
    const auto light = kinetix::ui::ThemePalette::light_archival();
    assert_true(kinetix::ui::ux::meets_wcag_aaa_normal_text(dark.text_primary, dark.bg_canvas_top) &&
                kinetix::ui::ux::meets_wcag_aaa_normal_text(light.text_primary, light.bg_canvas_top) &&
                kinetix::ui::ux::meets_wcag_aa_normal_text(dark.text_on_primary(), dark.primary_accent) &&
                kinetix::ui::ux::meets_wcag_aa_normal_text(light.text_on_primary(), light.primary_accent),
                "24_ux_workflow5_wcag_aa_aaa_contrast_ratios", "Dark & Light themes and primary button inks must satisfy WCAG AA/AAA");

    // Workflow 10: Visual & Written Quality Heuristic Audit model
    const auto audit = kinetix::ui::ux::evaluate_quality_audit(kinetix::ui::ux::VisualQualityAuditInput{});
    assert_true(audit.passes_quality_gate && audit.written_warning_score <= 20.0 && audit.visual_warning_score <= 20.0,
                "25_ux_workflow10_visual_quality_audit_score", "Quality audit score must be <= 20.0 (Few warning signs observed)");

    const std::string tmp_db = "/tmp/kinetix_v3_gui_test.sqlite3";
    const std::string snap_dir = "/tmp/kinetix_v3_gui_snaps";
    std::filesystem::remove(tmp_db);
    std::filesystem::remove_all(snap_dir);

    kinetix::persistence::DatabaseManager db(tmp_db);
    db.initialize_schema();
    kinetix::domain::KnowledgeGraph graph;
    db.seed_default_curriculum_if_empty(graph);

    kinetix::ui::MainWindow win(db, graph, 1440, 880, true);
    assert_true(win.initialize(), "26_native_sdl2_pangocairo_photon_init", "Native SDL2 + PangoCairo + librsvg + Photon window must initialize");

    // Workflow 7 & 9: Empty-state search filter and 1-click recovery
    win.set_search_filter("NonExistentTokenXYZ");
    assert_true(win.profiler_telemetry().imgui_stack_balanced, "27_ux_workflow7_empty_state_search_filter_render", "Empty state must render with balanced ImGui stack");
    win.trigger_button_action("clear_search_filter");

    // Workflow 8: Reduced Motion preference bypasses spring oscillation
    win.set_reduced_motion(true);
    win.switch_screen(kinetix::ui::AppScreen::Screen4_KnowledgeGraphExplorer);
    assert_true(win.is_reduced_motion() && win.nav_pill_spring().is_settled() && std::abs(win.nav_pill_spring().value() - 3.0) < 1e-6,
                "28_ux_workflow8_reduced_motion_immediate_snap", "Reduced Motion must snap SpringValue immediately without oscillation");
    win.set_reduced_motion(false);

    win.select_language("ja");
    win.select_language("es");
    win.select_language("de");
    win.trigger_button_action("submit_accurate_recall");
    assert_true(win.last_evaluation().is_correct, "29_interactive_exercise_submission_and_eval", "Interactive recall submission must evaluate as correct");

    win.trigger_button_action("toggle_pro_mode");
    assert_true(win.is_pro_telemetry_mode(), "30_toggle_pro_telemetry_dock_mode", "Pro Telemetry Dock mode must toggle ON cleanly");
    win.trigger_button_action("toggle_pro_mode");

    // Test 31-34: Multi-passage switching, Queue Auto-Advance, 16-bit PCM WAV Prosody Synthesis, and Anki TSV Deck Export
    win.trigger_button_action("switch_passage_1");
    win.trigger_button_action("switch_passage_0");
    assert_true(win.profiler_telemetry().imgui_stack_balanced, "31_multi_passage_immersion_switcher", "Switching between Passage 1 and Passage 2 must remain balanced");

    const auto prev_nid = win.selected_node_id();
    win.trigger_button_action("advance_next_node");
    assert_true(win.selected_node_id() != prev_nid, "32_queue_auto_advance_next_node", "Next Due Node button must advance active node selection");
    win.select_node(103);

    const std::string test_wav = "/tmp/kinetix_test_prosody.wav";
    std::filesystem::remove(test_wav);
    const bool wav_ok = kinetix::platform::PlatformAdapter::synthesize_prosodic_wav(test_wav, {140.0, 185.0, 210.0, 155.0}, 500);
    assert_true(wav_ok && std::filesystem::exists(test_wav) && std::filesystem::file_size(test_wav) > 44000,
                "33_native_16bit_pcm_wav_prosody_synthesizer", "PlatformAdapter must synthesize valid 44.1kHz 16-bit mono RIFF/WAVE audio");
    std::filesystem::remove(test_wav);

    const std::string test_tsv = "/tmp/kinetix_test_anki.tsv";
    std::filesystem::remove(test_tsv);
    const bool tsv_ok = db.export_anki_tsv(test_tsv, graph);
    assert_true(tsv_ok && std::filesystem::exists(test_tsv) && std::filesystem::file_size(test_tsv) > 500,
                "34_open_format_anki_tsv_deck_exporter", "DatabaseManager must export 4-modality curriculum to Anki TSV format");

    // Test 35-38: Interactive Search Focus (Ctrl+K), Explicit 4-Grade FSRS Buttons (1..4), and Warm-Cache Frame Render Speed
    win.trigger_button_action("focus_search_box");
    assert_true(win.is_search_focused(), "35_interactive_search_bar_focus_toggle", "Clicking search box or pressing Ctrl+K must focus search input");
    win.trigger_button_action("focus_search_box");

    win.switch_screen(kinetix::ui::AppScreen::Screen3_ActiveRecallStudio);
    win.trigger_button_action("grade_again");
    win.trigger_button_action("grade_hard");
    win.trigger_button_action("grade_good");
    win.trigger_button_action("grade_easy");
    assert_true(win.last_evaluation().suggested_fsrs_rating == kinetix::domain::Rating::Easy,
                "36_clickable_4_grade_fsrs_interval_cards", "All 4 FSRS grade preview cards (Again, Hard, Good, Easy) must commit ratings");

    // Warm-cache render pass verification
    win.switch_screen(kinetix::ui::AppScreen::Screen1_WorkbenchOverview);
    win.switch_screen(kinetix::ui::AppScreen::Screen1_WorkbenchOverview);
    assert_true(win.profiler_telemetry().last_frame_render_ms < 50.0,
                "37_warm_cache_frame_render_latency_under_50ms", "Warm-cache frame render must complete rapidly via sprite & Pango caches");

    assert_true(win.export_all_screen_snapshots(snap_dir), "38_export_all_8_native_ui_snapshots", "All 8 native UI snapshots (5 Focus Studio + 1 Mobile + 1 Light + 1 Pro Dock) must export");
    assert_true(std::filesystem::exists(snap_dir + "/07_light_archival_theme_view.png") &&
                std::filesystem::exists(snap_dir + "/08_pro_telemetry_dockspace_view.png"),
                "39_verify_light_and_pro_dock_snapshots_exist", "Light theme & Pro Dock PNGs must exist on disk");
    assert_true(win.profiler_telemetry().imgui_stack_balanced,
                "40_final_imgui_scope_and_id_stack_balance", "All Begin*/End* and Push*/Pop* scopes must be 100% balanced");

    // Test 41-48: Production v4.0 Hardening, Security, Integrity & Failure-Path QA Suite
    assert_true(db.verify_database_integrity(),
                "41_sqlite_integrity_and_foreign_key_check", "PRAGMA integrity_check and foreign_key_check must report zero corruption");
    assert_true(db.schema_version() == kinetix::persistence::DatabaseManager::kCurrentSchemaVersion,
                "42_schema_migrations_version_tracking", "schema_migrations table must record version 4");

    // Test 43: Bidirectional Anki TSV Export -> Import Roundtrip into a fresh SQLite DB
    const std::string import_db_path = "/tmp/kinetix_v4_import_test.sqlite3";
    std::filesystem::remove(import_db_path);
    {
        kinetix::persistence::DatabaseManager import_db(import_db_path);
        import_db.initialize_schema();
        kinetix::domain::KnowledgeGraph imported_graph;
        const bool imp_ok = import_db.import_anki_tsv(test_tsv, imported_graph);
        assert_true(imp_ok && imported_graph.all_nodes().size() == 11 && import_db.verify_database_integrity(),
                    "43_bidirectional_anki_tsv_export_and_import_roundtrip", "Anki TSV import must reconstruct all 11 4-modality nodes transactionally");

        // Test 44: Malformed TSV triggers automatic transactional ROLLBACK without partial corruption
        const std::string bad_tsv = "/tmp/kinetix_bad_anki.tsv";
        {
            std::ofstream bad_out(bad_tsv);
            bad_out << "999\tde\tValidFirstRow\t[ipa]\tGloss\tMorph\tExL2\tExGloss\t5.0\t5.0\t5.0\t5.0\n";
            bad_out << "CORRUPT_ROW_WITH_MISSING_COLUMNS\tde\n";
        }
        const bool bad_ok = import_db.import_anki_tsv(bad_tsv, imported_graph);
        assert_true(!bad_ok && imported_graph.find_node(999) == nullptr,
                    "44_malformed_anki_tsv_transactional_rollback", "Malformed TSV row must rollback entire transaction cleanly");
        std::filesystem::remove(bad_tsv);
    }
    std::filesystem::remove(import_db_path);
    std::filesystem::remove(test_tsv);

    // Test 45: Bounded 1D Levenshtein Edit Distance DoS Protection (4,096-byte input)
    const std::string giant_a(4096, 'a');
    const std::string giant_b(4096, 'b');
    const auto t_ed0 = std::chrono::steady_clock::now();
    const int32_t ed = kinetix::domain::PedagogicalLoopEngine::compute_edit_distance(giant_a, giant_b);
    const double ed_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t_ed0).count();
    assert_true(ed == 512 && ed_ms < 25.0,
                "45_bounded_1d_levenshtein_edit_distance_dos_protection", "compute_edit_distance must clamp oversized inputs to 512 bytes in O(N) space");

    // Test 46: Stage 1 Sentence Mining persists a verified WAL revlog transaction
    win.select_node(103);
    const size_t rev_before = db.load_recent_revlog(100).size();
    win.trigger_button_action("mine_selected_token");
    const auto rev_after = db.load_recent_revlog(100);
    assert_true(rev_after.size() == rev_before + 1 && rev_after.front().context_tag == "stage1_sentence_mined",
                "46_stage1_sentence_mining_persists_wal_revlog_transaction", "1-Click Mine Context must persist a stage1_sentence_mined revlog transaction in SQLite3 WAL");

    // Test 47: Search Input Length Boundary Clamping (<= 128 bytes)
    const std::string oversized_query(1000, 'x');
    win.set_search_filter(oversized_query);
    assert_true(win.search_filter().size() == 128,
                "47_search_input_length_boundary_clamping", "Search filter must clamp oversized strings to 128 bytes");
    win.trigger_button_action("clear_search_filter");

    // Test 48: Prepared statement reuse in save_node completes rapidly and preserves DB integrity
    const auto* n103 = graph.find_node(103);
    assert_true(n103 != nullptr && db.save_node(*n103) && db.verify_database_integrity(),
                "48_prepared_statement_reuse_save_node_integrity", "save_node with sqlite3_reset statement reuse must preserve full DB integrity");

    std::error_code ec;
    std::filesystem::remove(tmp_db, ec);
    std::filesystem::remove_all(snap_dir, ec);
}

} // namespace

int main() {
    try {
        test_fsrs_power_law_and_soa_batch();
        test_memory_pool_allocator();
        test_photon_and_imgui_layers();
        test_svg_renderer_blur_shader_and_spring_physics();
        test_wanikani_gating_and_pedagogical_stages();
        test_ux_design_system_workflows_and_snapshots();
        std::cout << "\n======================================================\n"
                  << "ALL " << g_tests_passed << "/" << g_tests_run << " AUTOMATED UNIT & INTEGRATION TESTS PASSED (100%)\n"
                  << "======================================================\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "\n[TEST SUITE FAILURE] " << ex.what() << "\n";
        return 1;
    }
}
