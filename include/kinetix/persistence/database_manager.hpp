#pragma once
// SQLite3 Transactional Persistence & Event-Sourced CRDT Sync Manager
// Uses native libsqlite3 in WAL mode with foreign key constraints, deterministic
// state reconstruction from append-only revlog events, schema migration tracking,
// database integrity verification, bidirectional Anki TSV deck import/export,
// and a seeded multi-language curriculum (Spanish, Japanese, German).

#include "kinetix/domain/types.hpp"
#include "kinetix/domain/knowledge_graph.hpp"
#include "kinetix/domain/habit_engine.hpp"
#include "third_party/sqlite3_abi.h"
#include <string>
#include <vector>

namespace kinetix::persistence {

class DatabaseManager {
public:
    static constexpr int kCurrentSchemaVersion = 4;

    explicit DatabaseManager(const std::string& db_path);
    ~DatabaseManager();

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    bool is_open() const noexcept { return db_ != nullptr; }
    const std::string& db_path() const noexcept { return db_path_; }

    // Initialize BCNF schema, indexes, schema_migrations, and WAL pragmas
    bool initialize_schema();

    // Query current applied schema migration version
    int schema_version() const;

    // Run PRAGMA integrity_check and PRAGMA foreign_key_check
    bool verify_database_integrity() const;

    // Seed rich default curriculum for Spanish ("es"), Japanese ("ja"), and German ("de")
    // if database is empty, including realistic FSRS history and prerequisite trees.
    bool seed_default_curriculum_if_empty(domain::KnowledgeGraph& graph);

    // Load all linguistic nodes, prerequisites, and 4-modality states into the KnowledgeGraph
    bool load_knowledge_graph(domain::KnowledgeGraph& graph);

    // Save a single linguistic node and its 4 modality states transactionally with statement reuse
    bool save_node(const domain::LinguisticNode& node);

    // Append an immutable review event to the `revlog` table and update `modality_states`
    // inside a single ACID transaction.
    bool record_review_transaction(
        const domain::LinguisticNode& updated_node,
        const domain::RevlogEntry& revlog_entry
    );

    // Load immersion passages for a language
    std::vector<domain::ImmersionPassage> load_passages(const std::string& language_code) const;

    // Load recent revlog entries for analytics & deterministic CRDT audit
    std::vector<domain::RevlogEntry> load_recent_revlog(size_t limit = 50) const;

    // Save and load EthicalHabitState settings
    bool save_habit_state(const domain::EthicalHabitState& state);
    domain::EthicalHabitState load_habit_state() const;

    // Export deterministic JSON backup of event-sourced revlog for offline-first backup/sync
    bool export_revlog_backup_json(const std::string& output_path) const;

    // Export Anki-compatible Tab-Separated Values (.tsv) deck of all 4-modality nodes
    bool export_anki_tsv(const std::string& output_tsv_path, const domain::KnowledgeGraph& graph) const;

    // Import Anki-compatible Tab-Separated Values (.tsv) deck transactionally with validation
    bool import_anki_tsv(const std::string& input_tsv_path, domain::KnowledgeGraph& graph);

private:
    bool execute_sql(const char* sql) const;
    std::string db_path_;
    sqlite3* db_ = nullptr;
};

} // namespace kinetix::persistence
