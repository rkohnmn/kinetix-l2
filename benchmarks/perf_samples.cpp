#include "samples.hpp"
#include "kinetix/ui/main_window.hpp"
#include <memory>
int main() {
    try {
        perf::Directory directory;
        kinetix::domain::FsrsEngine engine;
        kinetix::domain::ModalityStatesSoA states;
        states.reserve(44);
        for (int i = 0; i < 44; ++i) {
            states.node_ids.push_back(i);
            states.stabilities.push_back(5.0);
            states.difficulties.push_back(5.0);
            states.elapsed_days.push_back(2.0);
        }
        engine.evaluate_batch_soa(states);
        perf::require(states.out_retrievabilities.size() == 44 && states.out_retrievabilities[0] > 0, "FSRS fixture differs");
        kinetix::persistence::DatabaseManager db((directory.path / "fixture.sqlite3").string());
        perf::require(db.initialize_schema(), "Benchmark database failed");
        kinetix::domain::KnowledgeGraph graph;
        db.seed_default_curriculum_if_empty(graph);
        auto window = std::make_unique<kinetix::ui::MainWindow>(db, graph, 1440, 880, true);
        perf::require(window->initialize(), "Native renderer failed");
        std::vector<perf::Metric> metrics;
        metrics.push_back(perf::measure("fsrs_batch_44_tuples", 2000, [&] { engine.evaluate_batch_soa(states); }));
        metrics.push_back(perf::measure("cpu_render_1440x880_warm_cache", 1, [&] { window->switch_screen(kinetix::ui::AppScreen::Screen1_WorkbenchOverview); }));
        perf::require(window->profiler_telemetry().imgui_stack_balanced, "Render scopes are unbalanced");
        perf::report(metrics);
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
