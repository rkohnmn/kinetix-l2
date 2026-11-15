// Batch 10 regression: run against actual kinetix_core, not a mocked database.
#include "kinetix/domain/knowledge_graph.hpp"
#include "kinetix/persistence/database_manager.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {
namespace fs = std::filesystem;
using kinetix::domain::LinguisticNode;
using kinetix::domain::KnowledgeGraph;
using kinetix::domain::Modality;
using kinetix::domain::Rating;
using kinetix::domain::RevlogEntry;
using kinetix::persistence::DatabaseManager;

void verify(bool condition, const char* description) {
    if (!condition) throw std::runtime_error(description);
}

LinguisticNode make_node(std::int64_t id) {
    LinguisticNode n;
    n.id = id;
    n.language_code = "de";
    n.target_form = "Audit card";
    n.modalities[0].stability = 5.0;
    n.modalities[1].stability = 5.0;
    return n;
}

struct TempFiles {
    fs::path db;
    fs::path tsv;
    ~TempFiles() {
        std::error_code error;
        fs::remove(db, error);
        fs::remove(db.string() + "-wal", error);
        fs::remove(db.string() + "-shm", error);
        fs::remove(tsv, error);
    }
};

void run_tests() {
    // Public SoA input is user-constructible; mismatched vector lengths must
    // return a failure instead of reading from a null/undersized data pointer.
    kinetix::domain::FsrsEngine fsrs;
    kinetix::domain::ModalityStatesSoA malformed{};
    malformed.stabilities.push_back(10.0);
    malformed.difficulties.push_back(5.0);
    malformed.node_ids.push_back(4);
    verify(fsrs.evaluate_batch_soa(malformed) < 0.0 &&
           malformed.out_retrievabilities.empty(),
           "malformed SoA vectors must fail closed without out-of-bounds access");

    const auto suffix = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    const fs::path base = fs::temp_directory_path() / ("kinetix-audit-integrity-" + suffix);
    TempFiles tmp{base.string() + ".sqlite3", base.string() + ".tsv"};
    {
        DatabaseManager db(tmp.db.string());
        verify(db.is_open() && db.initialize_schema(), "fresh database initializes");

        // A missing prerequisite is NOT a successful prerequisite. This must
        // hold both for productive unlocking and for the blocking list.
        KnowledgeGraph graph;
        auto unresolved = make_node(20);
        unresolved.prerequisites = {99999};
        graph.upsert_node(unresolved);
        verify(!graph.is_productive_unlocked(20), "missing prerequisite must block productive modality");
        verify(graph.blocking_prerequisites(20) == unresolved.prerequisites,
               "missing prerequisite must appear in blocking diagnostics");

        auto french = make_node(55);
        french.language_code = "fr";
        KnowledgeGraph mixed_graph;
        mixed_graph.upsert_node(french);
        verify(db.export_anki_tsv(tmp.tsv.string(), mixed_graph),
               "Anki export accepts custom-language node");
        {
            std::ifstream exported(tmp.tsv);
            const std::string contents((std::istreambuf_iterator<char>(exported)),
                                       std::istreambuf_iterator<char>());
            verify(contents.find("55\tfr\t") != std::string::npos,
                   "Anki export must preserve imported French nodes");
        }

        auto prerequisite = make_node(1);
        auto dependent = make_node(2);
        dependent.prerequisites = {1};
        verify(db.save_node(prerequisite), "save prerequisite row");
        verify(db.save_node(dependent), "save dependent row");
        dependent.prerequisites.clear();
        verify(db.save_node(dependent), "save removed prerequisite");
        KnowledgeGraph restored;
        verify(db.load_knowledge_graph(restored), "reload graph");
        const auto* reloaded = restored.find_node(2);
        verify(reloaded && reloaded->prerequisites.empty(), "removed prerequisite stays removed after restart");

        // Inserting a nonexistent FK must roll back ALL changes, not leave a
        // partially updated node or ignore the failed prerequisite statement.
        dependent.target_form = "This update must roll back";
        dependent.prerequisites = {777777};
        verify(!db.save_node(dependent), "missing prerequisite foreign key fails save_node");
        KnowledgeGraph after_failure;
        verify(db.load_knowledge_graph(after_failure), "reload after failed save");
        const auto* unchanged = after_failure.find_node(2);
        verify(unchanged && unchanged->target_form == "Audit card" && unchanged->prerequisites.empty(),
               "failed save leaves original node and prerequisite set unchanged");

        // Event UUID is immutable: replay with mismatched contents must be
        // rejected, and the first persisted event must survive unchanged.
        RevlogEntry event;
        event.event_uuid = "audit-review-unique-1";
        event.timestamp = 1700000000;
        event.node_id = 1;
        event.modality = Modality::ReceptiveText;
        event.rating = Rating::Good;
        event.response_time_ms = 800;
        event.elapsed_days = 1.5;
        event.scheduled_days = 4.0;
        event.pre_stability = 4.0;
        event.post_stability = 5.0;
        event.pre_difficulty = 5.0;
        event.post_difficulty = 4.0;
        event.context_tag = "original-event";
        verify(db.record_review_transaction(prerequisite, event), "original review persists");
        event.context_tag = "tampered-replay";
        event.rating = Rating::Again;
        verify(!db.record_review_transaction(prerequisite, event), "duplicate UUID must fail closed");
        const auto history = db.load_recent_revlog(10);
        verify(history.size() == 1 && history[0].context_tag == "original-event" &&
               history[0].rating == Rating::Good, "historical event remains immutable");

        // std::stod parses numeric prefixes; TSV import must demand the entire
        // field is consumed before mutating learner data.
        {
            std::ofstream out(tmp.tsv);
            verify(bool(out), "TSV fixture opens");
            out << "50\tde\tTSV\t[ipa]\tGloss\tMorph\tExL2\tExGloss\t2.0garbage\t2.0\t2.0\t2.0\n";
        }
        KnowledgeGraph imported;
        verify(!db.import_anki_tsv(tmp.tsv.string(), imported), "invalid numeric suffix rejects TSV import");
        verify(imported.find_node(50) == nullptr, "rejected TSV leaves memory unchanged");
        KnowledgeGraph imported_db;
        verify(db.load_knowledge_graph(imported_db) && imported_db.find_node(50) == nullptr,
               "rejected TSV leaves SQLite unchanged");
        verify(db.verify_database_integrity(), "SQLite integrity and foreign keys remain valid");
    }
}
} // namespace

int main() {
    try {
        run_tests();
        std::cout << "[PASS] Kinetix Batch 10 data integrity regressions\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] Kinetix Batch 10: " << error.what() << '\n';
        return 1;
    }
}
