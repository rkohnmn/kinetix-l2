#include "kinetix/persistence/database_manager.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

namespace kinetix::persistence {

DatabaseManager::DatabaseManager(const std::string& db_path)
    : db_path_(db_path) {
    if (sqlite3_open(db_path_.c_str(), &db_) != SQLITE_OK) {
        db_ = nullptr;
    } else {
        execute_sql("PRAGMA journal_mode = WAL;");
        execute_sql("PRAGMA synchronous = NORMAL;");
        execute_sql("PRAGMA foreign_keys = ON;");
        execute_sql("PRAGMA busy_timeout = 5000;");
        execute_sql("PRAGMA temp_store = MEMORY;");
        execute_sql("PRAGMA mmap_size = 67108864;");
    }
}

DatabaseManager::~DatabaseManager() {
    if (db_) {
        sqlite3_close_v2(db_);
        db_ = nullptr;
    }
}

bool DatabaseManager::execute_sql(const char* sql) const {
    if (!db_) return false;
    char* errmsg = nullptr;
    const int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        if (errmsg) {
            sqlite3_free(errmsg);
        }
        return false;
    }
    return true;
}

bool DatabaseManager::initialize_schema() {
    if (!db_) return false;

    const char* ddl = R"SQL(
        CREATE TABLE IF NOT EXISTS schema_migrations (
            version INTEGER PRIMARY KEY,
            applied_at_ts INTEGER NOT NULL,
            description TEXT NOT NULL
        );

        INSERT OR REPLACE INTO schema_migrations (version, applied_at_ts, description)
        VALUES (4, 1792580000, 'v4.0 Production BCNF Schema + WAL + 4-Modality FSRS-4.5 + Anki TSV Interop');

        CREATE TABLE IF NOT EXISTS linguistic_nodes (
            node_id INTEGER PRIMARY KEY,
            language_code TEXT NOT NULL CHECK(length(language_code) >= 2),
            node_type INTEGER NOT NULL CHECK(node_type BETWEEN 0 AND 4),
            target_form TEXT NOT NULL,
            phonetic_ipa TEXT NOT NULL,
            concept_gloss TEXT NOT NULL,
            morphological_note TEXT NOT NULL,
            mnemonic_hook TEXT NOT NULL,
            decoy_minimal_pair TEXT NOT NULL,
            example_context_l2 TEXT NOT NULL,
            example_context_gloss TEXT NOT NULL,
            zipf_frequency_rank INTEGER NOT NULL CHECK(zipf_frequency_rank >= 1)
        );

        CREATE INDEX IF NOT EXISTS idx_nodes_lang_rank
            ON linguistic_nodes(language_code, zipf_frequency_rank);

        CREATE TABLE IF NOT EXISTS node_prerequisites (
            node_id INTEGER NOT NULL,
            prerequisite_id INTEGER NOT NULL,
            PRIMARY KEY (node_id, prerequisite_id),
            FOREIGN KEY (node_id) REFERENCES linguistic_nodes(node_id) ON DELETE CASCADE,
            FOREIGN KEY (prerequisite_id) REFERENCES linguistic_nodes(node_id) ON DELETE CASCADE
        );

        CREATE INDEX IF NOT EXISTS idx_prereq_lookup
            ON node_prerequisites(prerequisite_id);

        CREATE TABLE IF NOT EXISTS modality_states (
            node_id INTEGER NOT NULL,
            modality INTEGER NOT NULL CHECK(modality BETWEEN 0 AND 3),
            difficulty REAL NOT NULL CHECK(difficulty BETWEEN 1.0 AND 10.0),
            stability REAL NOT NULL CHECK(stability >= 0.0),
            retrievability REAL NOT NULL CHECK(retrievability BETWEEN 0.0 AND 1.0),
            last_review_ts INTEGER NOT NULL,
            repetitions INTEGER NOT NULL CHECK(repetitions >= 0),
            lapses INTEGER NOT NULL CHECK(lapses >= 0),
            avg_latency_ms INTEGER NOT NULL CHECK(avg_latency_ms >= 0),
            PRIMARY KEY (node_id, modality),
            FOREIGN KEY (node_id) REFERENCES linguistic_nodes(node_id) ON DELETE CASCADE
        );

        CREATE INDEX IF NOT EXISTS idx_modality_stability
            ON modality_states(modality, stability);

        CREATE TABLE IF NOT EXISTS revlog (
            event_id INTEGER PRIMARY KEY AUTOINCREMENT,
            event_uuid TEXT NOT NULL UNIQUE,
            timestamp INTEGER NOT NULL,
            node_id INTEGER NOT NULL,
            modality INTEGER NOT NULL CHECK(modality BETWEEN 0 AND 3),
            rating INTEGER NOT NULL CHECK(rating BETWEEN 1 AND 4),
            response_time_ms INTEGER NOT NULL CHECK(response_time_ms >= 0),
            elapsed_days REAL NOT NULL,
            scheduled_days REAL NOT NULL,
            pre_stability REAL NOT NULL,
            post_stability REAL NOT NULL,
            pre_difficulty REAL NOT NULL,
            post_difficulty REAL NOT NULL,
            context_tag TEXT NOT NULL,
            FOREIGN KEY (node_id) REFERENCES linguistic_nodes(node_id) ON DELETE CASCADE
        );

        CREATE INDEX IF NOT EXISTS idx_revlog_node_ts
            ON revlog(node_id, timestamp DESC);

        CREATE TABLE IF NOT EXISTS immersion_passages (
            passage_id INTEGER PRIMARY KEY,
            language_code TEXT NOT NULL,
            title TEXT NOT NULL,
            cefr_level TEXT NOT NULL,
            body_l2 TEXT NOT NULL,
            pragmatic_summary TEXT NOT NULL
        );

        CREATE TABLE IF NOT EXISTS passage_nodes (
            passage_id INTEGER NOT NULL,
            node_id INTEGER NOT NULL,
            PRIMARY KEY (passage_id, node_id),
            FOREIGN KEY (passage_id) REFERENCES immersion_passages(passage_id) ON DELETE CASCADE,
            FOREIGN KEY (node_id) REFERENCES linguistic_nodes(node_id) ON DELETE CASCADE
        );

        CREATE TABLE IF NOT EXISTS habit_settings (
            profile_id INTEGER PRIMARY KEY CHECK(profile_id = 1),
            active_momentum_days INTEGER NOT NULL,
            endowed_cognate_nodes INTEGER NOT NULL,
            completed_tasks_today INTEGER NOT NULL,
            daily_target_tasks INTEGER NOT NULL,
            user_mined_sentences_count INTEGER NOT NULL,
            vacation_recovery_mode_active INTEGER NOT NULL,
            notifications_enabled INTEGER NOT NULL,
            preferred_reminder_window TEXT NOT NULL
        );
    )SQL";

    return execute_sql(ddl);
}

int DatabaseManager::schema_version() const {
    if (!db_) return 0;
    sqlite3_stmt* stmt = nullptr;
    int ver = 0;
    if (sqlite3_prepare_v2(db_, "SELECT MAX(version) FROM schema_migrations;", -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            ver = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    return ver;
}

bool DatabaseManager::verify_database_integrity() const {
    if (!db_) return false;

    // 1. Verify B-tree and page integrity via PRAGMA integrity_check
    sqlite3_stmt* ic_stmt = nullptr;
    bool integrity_ok = false;
    if (sqlite3_prepare_v2(db_, "PRAGMA integrity_check;", -1, &ic_stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(ic_stmt) == SQLITE_ROW) {
            const unsigned char* txt = sqlite3_column_text(ic_stmt, 0);
            if (txt && std::string(reinterpret_cast<const char*>(txt)) == "ok") {
                integrity_ok = true;
            }
        }
        sqlite3_finalize(ic_stmt);
    }
    if (!integrity_ok) return false;

    // 2. Verify zero referential integrity violations via PRAGMA foreign_key_check
    sqlite3_stmt* fk_stmt = nullptr;
    bool fk_ok = true;
    if (sqlite3_prepare_v2(db_, "PRAGMA foreign_key_check;", -1, &fk_stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(fk_stmt) == SQLITE_ROW) {
            fk_ok = false; // Any returned row indicates a broken foreign key
        }
        sqlite3_finalize(fk_stmt);
    } else {
        fk_ok = false;
    }
    return fk_ok;
}

bool DatabaseManager::save_node(const domain::LinguisticNode& node) {
    if (!db_) return false;

    const char* sql_node = R"SQL(
        INSERT INTO linguistic_nodes (
            node_id, language_code, node_type, target_form, phonetic_ipa,
            concept_gloss, morphological_note, mnemonic_hook, decoy_minimal_pair,
            example_context_l2, example_context_gloss, zipf_frequency_rank
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT(node_id) DO UPDATE SET
            language_code = excluded.language_code,
            node_type = excluded.node_type,
            target_form = excluded.target_form,
            phonetic_ipa = excluded.phonetic_ipa,
            concept_gloss = excluded.concept_gloss,
            morphological_note = excluded.morphological_note,
            mnemonic_hook = excluded.mnemonic_hook,
            decoy_minimal_pair = excluded.decoy_minimal_pair,
            example_context_l2 = excluded.example_context_l2,
            example_context_gloss = excluded.example_context_gloss,
            zipf_frequency_rank = excluded.zipf_frequency_rank;
    )SQL";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql_node, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int64(stmt, 1, node.id);
    sqlite3_bind_text(stmt, 2, node.language_code.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, static_cast<int>(node.type));
    sqlite3_bind_text(stmt, 4, node.target_form.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, node.phonetic_ipa.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, node.concept_gloss.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, node.morphological_note.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, node.mnemonic_hook.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, node.decoy_minimal_pair.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 10, node.example_context_l2.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 11, node.example_context_gloss.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 12, node.zipf_frequency_rank);
    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) return false;

    // Save prerequisites using prepared statement reuse (eliminates N+1 sqlite3_prepare_v2)
    if (!node.prerequisites.empty()) {
        const char* sql_prereq = "INSERT OR IGNORE INTO node_prerequisites (node_id, prerequisite_id) VALUES (?, ?);";
        sqlite3_stmt* p_stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql_prereq, -1, &p_stmt, nullptr) == SQLITE_OK) {
            for (domain::NodeId pre_id : node.prerequisites) {
                sqlite3_reset(p_stmt);
                sqlite3_clear_bindings(p_stmt);
                sqlite3_bind_int64(p_stmt, 1, node.id);
                sqlite3_bind_int64(p_stmt, 2, pre_id);
                sqlite3_step(p_stmt);
            }
            sqlite3_finalize(p_stmt);
        }
    }

    // Save 4 modality states using prepared statement reuse (1 prepare instead of 4 per node)
    const char* sql_mod = R"SQL(
        INSERT INTO modality_states (
            node_id, modality, difficulty, stability, retrievability,
            last_review_ts, repetitions, lapses, avg_latency_ms
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT(node_id, modality) DO UPDATE SET
            difficulty = excluded.difficulty,
            stability = excluded.stability,
            retrievability = excluded.retrievability,
            last_review_ts = excluded.last_review_ts,
            repetitions = excluded.repetitions,
            lapses = excluded.lapses,
            avg_latency_ms = excluded.avg_latency_ms;
    )SQL";

    sqlite3_stmt* m_stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql_mod, -1, &m_stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    for (size_t m = 0; m < 4; ++m) {
        const auto& st = node.modalities[m];
        sqlite3_reset(m_stmt);
        sqlite3_clear_bindings(m_stmt);
        sqlite3_bind_int64(m_stmt, 1, node.id);
        sqlite3_bind_int(m_stmt, 2, static_cast<int>(m));
        sqlite3_bind_double(m_stmt, 3, std::clamp(st.difficulty, 1.0, 10.0));
        sqlite3_bind_double(m_stmt, 4, std::max(0.0, st.stability));
        sqlite3_bind_double(m_stmt, 5, std::clamp(st.retrievability, 0.0, 1.0));
        sqlite3_bind_int64(m_stmt, 6, st.last_review_ts);
        sqlite3_bind_int(m_stmt, 7, std::max(0, st.repetitions));
        sqlite3_bind_int(m_stmt, 8, std::max(0, st.lapses));
        sqlite3_bind_int(m_stmt, 9, std::max(0, st.avg_latency_ms));
        if (sqlite3_step(m_stmt) != SQLITE_DONE) {
            sqlite3_finalize(m_stmt);
            return false;
        }
    }
    sqlite3_finalize(m_stmt);
    return true;
}

bool DatabaseManager::record_review_transaction(
    const domain::LinguisticNode& updated_node,
    const domain::RevlogEntry& revlog_entry
) {
    if (!db_) return false;
    if (!execute_sql("BEGIN TRANSACTION;")) return false;

    if (!save_node(updated_node)) {
        execute_sql("ROLLBACK;");
        return false;
    }

    const char* sql_rev = R"SQL(
        INSERT OR REPLACE INTO revlog (
            event_uuid, timestamp, node_id, modality, rating, response_time_ms,
            elapsed_days, scheduled_days, pre_stability, post_stability,
            pre_difficulty, post_difficulty, context_tag
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    )SQL";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql_rev, -1, &stmt, nullptr) != SQLITE_OK) {
        execute_sql("ROLLBACK;");
        return false;
    }
    sqlite3_bind_text(stmt, 1, revlog_entry.event_uuid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, revlog_entry.timestamp);
    sqlite3_bind_int64(stmt, 3, revlog_entry.node_id);
    sqlite3_bind_int(stmt, 4, static_cast<int>(revlog_entry.modality));
    sqlite3_bind_int(stmt, 5, static_cast<int>(revlog_entry.rating));
    sqlite3_bind_int(stmt, 6, revlog_entry.response_time_ms);
    sqlite3_bind_double(stmt, 7, revlog_entry.elapsed_days);
    sqlite3_bind_double(stmt, 8, revlog_entry.scheduled_days);
    sqlite3_bind_double(stmt, 9, revlog_entry.pre_stability);
    sqlite3_bind_double(stmt, 10, revlog_entry.post_stability);
    sqlite3_bind_double(stmt, 11, revlog_entry.pre_difficulty);
    sqlite3_bind_double(stmt, 12, revlog_entry.post_difficulty);
    sqlite3_bind_text(stmt, 13, revlog_entry.context_tag.c_str(), -1, SQLITE_TRANSIENT);

    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        execute_sql("ROLLBACK;");
        return false;
    }
    return execute_sql("COMMIT;");
}

bool DatabaseManager::load_knowledge_graph(domain::KnowledgeGraph& graph) {
    if (!db_) return false;

    const char* sql = R"SQL(
        SELECT node_id, language_code, node_type, target_form, phonetic_ipa,
               concept_gloss, morphological_note, mnemonic_hook, decoy_minimal_pair,
               example_context_l2, example_context_gloss, zipf_frequency_rank
        FROM linguistic_nodes
        ORDER BY node_id ASC;
    )SQL";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    auto safe_str = [](sqlite3_stmt* s, int col) -> std::string {
        const unsigned char* txt = sqlite3_column_text(s, col);
        return txt ? reinterpret_cast<const char*>(txt) : "";
    };

    std::vector<domain::LinguisticNode> loaded_nodes;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        domain::LinguisticNode n{};
        n.id = sqlite3_column_int64(stmt, 0);
        n.language_code = safe_str(stmt, 1);
        n.type = static_cast<domain::NodeType>(sqlite3_column_int(stmt, 2));
        n.target_form = safe_str(stmt, 3);
        n.phonetic_ipa = safe_str(stmt, 4);
        n.concept_gloss = safe_str(stmt, 5);
        n.morphological_note = safe_str(stmt, 6);
        n.mnemonic_hook = safe_str(stmt, 7);
        n.decoy_minimal_pair = safe_str(stmt, 8);
        n.example_context_l2 = safe_str(stmt, 9);
        n.example_context_gloss = safe_str(stmt, 10);
        n.zipf_frequency_rank = sqlite3_column_int(stmt, 11);
        loaded_nodes.push_back(std::move(n));
    }
    sqlite3_finalize(stmt);

    // Prepare prerequisite and modality statements once before iterating over nodes
    sqlite3_stmt* p_stmt = nullptr;
    sqlite3_prepare_v2(db_, "SELECT prerequisite_id FROM node_prerequisites WHERE node_id = ?;", -1, &p_stmt, nullptr);

    sqlite3_stmt* m_stmt = nullptr;
    const char* m_sql = R"SQL(
        SELECT modality, difficulty, stability, retrievability,
               last_review_ts, repetitions, lapses, avg_latency_ms
        FROM modality_states WHERE node_id = ?;
    )SQL";
    sqlite3_prepare_v2(db_, m_sql, -1, &m_stmt, nullptr);

    for (auto& node : loaded_nodes) {
        if (p_stmt) {
            sqlite3_reset(p_stmt);
            sqlite3_clear_bindings(p_stmt);
            sqlite3_bind_int64(p_stmt, 1, node.id);
            while (sqlite3_step(p_stmt) == SQLITE_ROW) {
                node.prerequisites.push_back(sqlite3_column_int64(p_stmt, 0));
            }
        }

        if (m_stmt) {
            sqlite3_reset(m_stmt);
            sqlite3_clear_bindings(m_stmt);
            sqlite3_bind_int64(m_stmt, 1, node.id);
            while (sqlite3_step(m_stmt) == SQLITE_ROW) {
                const int mod_idx = sqlite3_column_int(m_stmt, 0);
                if (mod_idx >= 0 && mod_idx < 4) {
                    auto& st = node.modalities[static_cast<size_t>(mod_idx)];
                    st.difficulty = sqlite3_column_double(m_stmt, 1);
                    st.stability = sqlite3_column_double(m_stmt, 2);
                    st.retrievability = sqlite3_column_double(m_stmt, 3);
                    st.last_review_ts = sqlite3_column_int64(m_stmt, 4);
                    st.repetitions = sqlite3_column_int(m_stmt, 5);
                    st.lapses = sqlite3_column_int(m_stmt, 6);
                    st.avg_latency_ms = sqlite3_column_int(m_stmt, 7);
                }
            }
        }
        graph.upsert_node(node);
    }

    if (p_stmt) sqlite3_finalize(p_stmt);
    if (m_stmt) sqlite3_finalize(m_stmt);

    return true;
}

std::vector<domain::ImmersionPassage> DatabaseManager::load_passages(const std::string& language_code) const {
    std::vector<domain::ImmersionPassage> passages;
    if (!db_) return passages;

    const char* sql = R"SQL(
        SELECT passage_id, language_code, title, cefr_level, body_l2, pragmatic_summary
        FROM immersion_passages WHERE language_code = ? ORDER BY passage_id ASC;
    )SQL";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return passages;
    }
    sqlite3_bind_text(stmt, 1, language_code.c_str(), -1, SQLITE_TRANSIENT);

    auto safe_str = [](sqlite3_stmt* s, int col) -> std::string {
        const unsigned char* txt = sqlite3_column_text(s, col);
        return txt ? reinterpret_cast<const char*>(txt) : "";
    };

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        domain::ImmersionPassage p{};
        p.passage_id = sqlite3_column_int64(stmt, 0);
        p.language_code = safe_str(stmt, 1);
        p.title = safe_str(stmt, 2);
        p.cefr_level = safe_str(stmt, 3);
        p.body_l2 = safe_str(stmt, 4);
        p.pragmatic_summary = safe_str(stmt, 5);
        passages.push_back(std::move(p));
    }
    sqlite3_finalize(stmt);

    sqlite3_stmt* n_stmt = nullptr;
    if (sqlite3_prepare_v2(db_, "SELECT node_id FROM passage_nodes WHERE passage_id = ?;", -1, &n_stmt, nullptr) == SQLITE_OK) {
        for (auto& p : passages) {
            sqlite3_reset(n_stmt);
            sqlite3_clear_bindings(n_stmt);
            sqlite3_bind_int64(n_stmt, 1, p.passage_id);
            while (sqlite3_step(n_stmt) == SQLITE_ROW) {
                p.linked_nodes.push_back(sqlite3_column_int64(n_stmt, 0));
            }
        }
        sqlite3_finalize(n_stmt);
    }
    return passages;
}

std::vector<domain::RevlogEntry> DatabaseManager::load_recent_revlog(size_t limit) const {
    std::vector<domain::RevlogEntry> logs;
    if (!db_) return logs;

    const char* sql = R"SQL(
        SELECT event_id, event_uuid, timestamp, node_id, modality, rating,
               response_time_ms, elapsed_days, scheduled_days,
               pre_stability, post_stability, pre_difficulty, post_difficulty, context_tag
        FROM revlog ORDER BY timestamp DESC, event_id DESC LIMIT ?;
    )SQL";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return logs;
    }
    sqlite3_bind_int(stmt, 1, static_cast<int>(limit));

    auto safe_str = [](sqlite3_stmt* s, int col) -> std::string {
        const unsigned char* txt = sqlite3_column_text(s, col);
        return txt ? reinterpret_cast<const char*>(txt) : "";
    };

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        domain::RevlogEntry e{};
        e.event_id = sqlite3_column_int64(stmt, 0);
        e.event_uuid = safe_str(stmt, 1);
        e.timestamp = sqlite3_column_int64(stmt, 2);
        e.node_id = sqlite3_column_int64(stmt, 3);
        e.modality = static_cast<domain::Modality>(sqlite3_column_int(stmt, 4));
        e.rating = static_cast<domain::Rating>(sqlite3_column_int(stmt, 5));
        e.response_time_ms = sqlite3_column_int(stmt, 6);
        e.elapsed_days = sqlite3_column_double(stmt, 7);
        e.scheduled_days = sqlite3_column_double(stmt, 8);
        e.pre_stability = sqlite3_column_double(stmt, 9);
        e.post_stability = sqlite3_column_double(stmt, 10);
        e.pre_difficulty = sqlite3_column_double(stmt, 11);
        e.post_difficulty = sqlite3_column_double(stmt, 12);
        e.context_tag = safe_str(stmt, 13);
        logs.push_back(std::move(e));
    }
    sqlite3_finalize(stmt);
    return logs;
}

bool DatabaseManager::save_habit_state(const domain::EthicalHabitState& state) {
    if (!db_) return false;
    const char* sql = R"SQL(
        INSERT OR REPLACE INTO habit_settings (
            profile_id, active_momentum_days, endowed_cognate_nodes,
            completed_tasks_today, daily_target_tasks, user_mined_sentences_count,
            vacation_recovery_mode_active, notifications_enabled, preferred_reminder_window
        ) VALUES (1, ?, ?, ?, ?, ?, ?, ?, ?);
    )SQL";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, state.active_momentum_days);
    sqlite3_bind_int(stmt, 2, state.endowed_cognate_nodes);
    sqlite3_bind_int(stmt, 3, state.completed_tasks_today);
    sqlite3_bind_int(stmt, 4, state.daily_target_tasks);
    sqlite3_bind_int(stmt, 5, state.user_mined_sentences_count);
    sqlite3_bind_int(stmt, 6, state.vacation_recovery_mode_active ? 1 : 0);
    sqlite3_bind_int(stmt, 7, state.notifications_enabled ? 1 : 0);
    sqlite3_bind_text(stmt, 8, state.preferred_reminder_window.c_str(), -1, SQLITE_TRANSIENT);
    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

domain::EthicalHabitState DatabaseManager::load_habit_state() const {
    domain::EthicalHabitState state{};
    if (!db_) return state;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = R"SQL(
        SELECT active_momentum_days, endowed_cognate_nodes, completed_tasks_today,
               daily_target_tasks, user_mined_sentences_count,
               vacation_recovery_mode_active, notifications_enabled, preferred_reminder_window
        FROM habit_settings WHERE profile_id = 1;
    )SQL";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            state.active_momentum_days = sqlite3_column_int(stmt, 0);
            state.endowed_cognate_nodes = sqlite3_column_int(stmt, 1);
            state.completed_tasks_today = sqlite3_column_int(stmt, 2);
            state.daily_target_tasks = sqlite3_column_int(stmt, 3);
            state.user_mined_sentences_count = sqlite3_column_int(stmt, 4);
            state.vacation_recovery_mode_active = sqlite3_column_int(stmt, 5) != 0;
            state.notifications_enabled = sqlite3_column_int(stmt, 6) != 0;
            const unsigned char* win = sqlite3_column_text(stmt, 7);
            if (win) state.preferred_reminder_window = reinterpret_cast<const char*>(win);
        }
        sqlite3_finalize(stmt);
    }
    return state;
}

bool DatabaseManager::export_revlog_backup_json(const std::string& output_path) const {
    const auto logs = load_recent_revlog(500);
    std::ofstream out(output_path);
    if (!out.is_open()) return false;
    out << "{\n  \"schema_version\": \"kinetix-crdt-revlog-v1\",\n  \"events\": [\n";
    for (size_t i = 0; i < logs.size(); ++i) {
        const auto& e = logs[i];
        out << "    {\"uuid\": \"" << e.event_uuid << "\", \"timestamp\": " << e.timestamp
            << ", \"node_id\": " << e.node_id << ", \"modality\": " << static_cast<int>(e.modality)
            << ", \"rating\": " << static_cast<int>(e.rating)
            << ", \"response_time_ms\": " << e.response_time_ms
            << ", \"post_stability\": " << e.post_stability
            << ", \"post_difficulty\": " << e.post_difficulty << "}"
            << (i + 1 < logs.size() ? "," : "") << "\n";
    }
    out << "  ]\n}\n";
    return true;
}

bool DatabaseManager::export_anki_tsv(const std::string& output_tsv_path, const domain::KnowledgeGraph& graph) const {
    if (output_tsv_path.empty()) return false;
    std::ofstream out(output_tsv_path, std::ios::trunc);
    if (!out.is_open()) return false;
    out << "#separator:tab\n#html:false\n#columns:NodeID\tLanguage\tTargetForm\tIPA\tConceptGloss\tMorphologicalNote\tExampleL2\tExampleGloss\tS_RecText\tS_RecAudio\tS_ProdText\tS_ProdAudio\n";
    for (const char* lang : {"de", "es", "ja"}) {
        for (const auto& n : graph.nodes_for_language(lang)) {
            out << n.id << "\t" << n.language_code << "\t" << n.target_form << "\t"
                << n.phonetic_ipa << "\t" << n.concept_gloss << "\t"
                << n.morphological_note << "\t" << n.example_context_l2 << "\t"
                << n.example_context_gloss << "\t"
                << n.modalities[0].stability << "\t" << n.modalities[1].stability << "\t"
                << n.modalities[2].stability << "\t" << n.modalities[3].stability << "\n";
        }
    }
    out.flush();
    return out.good();
}

bool DatabaseManager::import_anki_tsv(const std::string& input_tsv_path, domain::KnowledgeGraph& graph) {
    if (!db_ || input_tsv_path.empty()) return false;
    std::ifstream in(input_tsv_path);
    if (!in.is_open()) return false;

    if (!execute_sql("BEGIN TRANSACTION;")) return false;

    std::string line;
    int imported_count = 0;
    std::vector<domain::LinguisticNode> staged_nodes;

    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::vector<std::string> cols;
        std::stringstream ss(line);
        std::string cell;
        while (std::getline(ss, cell, '\t')) {
            cols.push_back(cell);
        }
        if (cols.size() < 12) {
            execute_sql("ROLLBACK;");
            return false;
        }

        domain::LinguisticNode node{};
        try {
            node.id = std::stoll(cols[0]);
            node.language_code = cols[1];
            node.type = domain::NodeType::LexicalChunk;
            node.target_form = cols[2];
            node.phonetic_ipa = cols[3];
            node.concept_gloss = cols[4];
            node.morphological_note = cols[5];
            node.mnemonic_hook = "Imported via Anki TSV Open-Format Deck";
            node.decoy_minimal_pair = cols[2];
            node.example_context_l2 = cols[6];
            node.example_context_gloss = cols[7];
            node.zipf_frequency_rank = 100;

            if (node.id <= 0 || node.language_code.size() < 2 || node.target_form.empty()) {
                execute_sql("ROLLBACK;");
                return false;
            }

            for (size_t m = 0; m < 4; ++m) {
                const double s_val = std::stod(cols[8 + m]);
                if (!std::isfinite(s_val) || s_val < 0.0 || s_val > 36500.0) {
                    execute_sql("ROLLBACK;");
                    return false;
                }
                node.modalities[m].difficulty = 5.0;
                node.modalities[m].stability = s_val;
                node.modalities[m].retrievability = (s_val > 0.0) ? 0.90 : 0.0;
                node.modalities[m].last_review_ts = (s_val > 0.0) ? 1792580000LL : 0;
                node.modalities[m].repetitions = (s_val > 0.0) ? 3 : 0;
                node.modalities[m].lapses = 0;
                node.modalities[m].avg_latency_ms = (s_val > 0.0) ? 1450 : 0;
            }
        } catch (...) {
            execute_sql("ROLLBACK;");
            return false;
        }

        if (!save_node(node)) {
            execute_sql("ROLLBACK;");
            return false;
        }
        staged_nodes.push_back(std::move(node));
        ++imported_count;
    }

    if (imported_count == 0 || !execute_sql("COMMIT;")) {
        execute_sql("ROLLBACK;");
        return false;
    }

    for (const auto& n : staged_nodes) {
        graph.upsert_node(n);
    }
    return true;
}

bool DatabaseManager::seed_default_curriculum_if_empty(domain::KnowledgeGraph& graph) {
    if (!db_) return false;

    sqlite3_stmt* check_stmt = nullptr;
    int existing_count = 0;
    if (sqlite3_prepare_v2(db_, "SELECT COUNT(*) FROM linguistic_nodes;", -1, &check_stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(check_stmt) == SQLITE_ROW) {
            existing_count = sqlite3_column_int(check_stmt, 0);
        }
        sqlite3_finalize(check_stmt);
    }

    if (existing_count > 0) {
        return load_knowledge_graph(graph);
    }

    const domain::TimestampSec base_ts = 1792580000LL; // Oct 2026 anchor

    auto make_mod = [&](double d, double s, double r, int64_t days_ago, int reps, int lapses, int latency) {
        return domain::FsrsMemoryState{
            d, s, r,
            (reps > 0) ? (base_ts - days_ago * 86400LL) : 0,
            reps, lapses, latency
        };
    };

    std::vector<domain::LinguisticNode> seed_nodes = {
        // --- GERMAN (de) Dependency Tree & 4-Modality Graph ---
        {
            101, "de", domain::NodeType::PhonemeGrapheme,
            "sch / -ung", "[ʃ / ʊŋ]",
            "Voiceless postalveolar fricative + nominalizing suffix '-ung' (always feminine die)",
            "Every German noun ending in '-ung' is feminine (die) and places stress on the root syllable.",
            "Think 'SHHH' + '-UNG' turns an action verb into a concrete feminine noun.",
            "ch [ç] vs. sch [ʃ]",
            "Die Übung und die Rechnung sind fertig.",
            "The exercise and the bill are ready.",
            15, {},
            {
                make_mod(3.8, 42.5, 0.96, 4, 9, 0, 920),
                make_mod(4.1, 38.0, 0.95, 4, 8, 0, 980),
                make_mod(4.4, 31.2, 0.93, 5, 7, 0, 1340),
                make_mod(4.6, 28.4, 0.91, 6, 6, 0, 1620)
            }
        },
        {
            102, "de", domain::NodeType::LexicalLemma,
            "die Schuld", "[diː ʃʊlt]",
            "Guilt, debt, or fault (root noun inside Entschuldigung)",
            "Feminine root noun; combines with prefix 'ent-' (removal) + '-ig-' + '-ung'.",
            "ent- (remove) + Schuld (fault) = removing fault -> apology.",
            "die Geduld [ɡəˈdʊlt]",
            "Es war nicht meine Schuld, dass der Zug spät kam.",
            "It wasn't my fault that the train came late.",
            85, {101},
            {
                make_mod(4.5, 18.4, 0.92, 4, 6, 0, 1180),
                make_mod(4.8, 14.2, 0.90, 5, 5, 0, 1290),
                make_mod(5.4, 8.6,  0.88, 4, 4, 1, 1890),
                make_mod(5.9, 6.2,  0.86, 3, 3, 1, 2150)
            }
        },
        {
            103, "de", domain::NodeType::LexicalChunk,
            "Entschuldigung", "[ɛntˈʃʊldɪɡʊŋ]",
            "Excuse me / I apologize (pragmatic attention-getter & apology)",
            "Pimsleur back-chain target: -digung -> -schuldigung -> Entschuldigung.",
            "ent- (unburden) + Schuld (blame) + -igung: 'un-blaming oneself'.",
            "Entscheidung [ɛntˈʃaɪdʊŋ] (decision)",
            "Entschuldigung, fährt dieser Zug direkt nach München?",
            "Excuse me, does this train go directly to Munich?",
            42, {101, 102},
            {
                make_mod(4.9, 12.6, 0.89, 5, 6, 0, 1240),
                make_mod(5.2, 9.4,  0.87, 4, 5, 1, 1480),
                make_mod(8.7, 3.4,  0.82, 3, 7, 5, 2740), // Active Leech in Productive Text (lapses=5, D=8.7)
                make_mod(6.8, 4.8,  0.85, 3, 4, 2, 2380)
            }
        },
        {
            104, "de", domain::NodeType::MorphosyntacticRule,
            "weil ... Verb-End", "[vaɪl ... ˈvɛʁp ɛnt]",
            "Subordinate causal clause ('because'): pushes conjugated verb to the final position",
            "In main clauses the finite verb sits in Position 2 (V2); after 'weil', it moves to the end.",
            "Picture 'weil' as a magnet at the clause entrance that repels the conjugated verb to the far wall.",
            "denn + V2 main clause order",
            "Ich rufe dich an, weil der Zug heute zwanzig Minuten Verspätung hat.",
            "I am calling you because the train has a twenty-minute delay today.",
            28, {101, 102},
            {
                make_mod(5.1, 15.8, 0.91, 4, 6, 0, 1320),
                make_mod(5.5, 11.2, 0.89, 4, 5, 0, 1510),
                make_mod(6.1, 6.4,  0.86, 3, 4, 1, 2120),
                make_mod(6.4, 4.1,  0.84, 2, 3, 1, 2410)
            }
        },
        {
            105, "de", domain::NodeType::LexicalLemma,
            "die Verspätung", "[fɛɐ̯ˈʃpɛːtʊŋ]",
            "Delay / lateness (transit, schedule, appointment)",
            "Inseparable prefix 'ver-' + 'spät' (late) + '-ung' (feminine nominalizer).",
            "ver- + spät (late) + -ung = the state of being delayed.",
            "die Verabredung [fɛɐ̯ˈʔapʁeːdʊŋ] (appointment)",
            "Wegen des Sturms hat die Bahn eine große Verspätung.",
            "Because of the storm, the train has a major delay.",
            110, {103, 104},
            {
                make_mod(5.4, 2.2,  0.88, 1, 2, 0, 1540), // S_rec < 3.0d -> gates dependent productive nodes
                make_mod(5.6, 1.8,  0.86, 1, 2, 0, 1690),
                make_mod(5.0, 0.0,  0.0,  0, 0, 0, 0),
                make_mod(5.0, 0.0,  0.0,  0, 0, 0, 0)
            }
        },
        {
            106, "de", domain::NodeType::PragmaticRegister,
            "Bescheid sagen", "[bəˈʃaɪt ˈzaːɡn̩]",
            "To let someone know / give notice (idiomatic everyday collocation)",
            "Takes dative recipient: 'Sag mir bitte Bescheid, wenn du ankommst.'",
            "Pair 'Bescheid' (notice) with 'sagen' as one inseparable conversational unit.",
            "Abschied nehmen (to say goodbye)",
            "Bitte sag mir sofort Bescheid, weil unser Zug Verspätung hat.",
            "Please let me know right away because our train is delayed.",
            140, {105}, // Blocked for Productive practice until Node #105 reaches S_rec >= 3.0d!
            {
                make_mod(5.2, 0.6,  0.85, 1, 1, 0, 1750),
                make_mod(5.3, 0.5,  0.84, 1, 1, 0, 1820),
                make_mod(5.0, 0.0,  0.0,  0, 0, 0, 0),
                make_mod(5.0, 0.0,  0.0,  0, 0, 0, 0)
            }
        },

        // --- SPANISH (es) Dependency Tree & 4-Modality Graph ---
        {
            201, "es", domain::NodeType::LexicalChunk,
            "tener que", "[teˈneɾ ke]",
            "To have to / must + [infinitive] (personal obligation frame)",
            "Conjugate 'tener' (tengo, tienes, tiene) + 'que' + uninflected infinitive verb.",
            "Bind 'tener que' as a single formulaic slot before any action verb.",
            "hay que (impersonal obligation)",
            "Tengo que salir temprano hacia la estación central.",
            "I have to leave early toward the central station.",
            18, {},
            {
                make_mod(3.9, 34.0, 0.94, 5, 8, 0, 980),
                make_mod(4.2, 29.5, 0.92, 5, 7, 0, 1120),
                make_mod(4.8, 16.2, 0.89, 4, 6, 0, 1580),
                make_mod(5.1, 11.4, 0.87, 4, 5, 1, 1950)
            }
        },
        {
            202, "es", domain::NodeType::MorphosyntacticRule,
            "aunque + subjuntivo", "[ˈaʊŋke]",
            "Even if / although (hypothetical or unverified concession triggers subjunctive)",
            "Fact ('aunque llueve' = even though it is raining) vs. Hypothesis ('aunque llueva' = even if it rains).",
            "Vowel flip (-ar -> -e, -er/-ir -> -a) signals non-factual future/hypothetical concession.",
            "aunque + indicativo (known fact)",
            "Tengo que llegar a tiempo aunque llueva toda la mañana.",
            "I have to arrive on time even if it rains all morning.",
            54, {201},
            {
                make_mod(5.3, 9.8,  0.88, 3, 5, 1, 1420),
                make_mod(5.6, 7.4,  0.86, 3, 4, 1, 1640),
                make_mod(6.4, 4.2,  0.84, 2, 4, 2, 2280),
                make_mod(6.9, 3.1,  0.83, 2, 3, 1, 2460)
            }
        },
        {
            203, "es", domain::NodeType::PragmaticRegister,
            "darse cuenta de", "[ˈdaɾse ˈkwenta ðe]",
            "To realize / notice a fact suddenly (pronominal chunk)",
            "Requires reflexive pronoun (me di cuenta de que... = I realized that...).",
            "Literally 'to give oneself account of' -> sudden cognitive realization.",
            "tener en cuenta (to keep in mind)",
            "No me di cuenta de que el tren salía del andén cuatro.",
            "I didn't realize that the train was leaving from platform four.",
            72, {201, 202},
            {
                make_mod(5.0, 4.5,  0.89, 2, 3, 0, 1390),
                make_mod(5.2, 3.6,  0.87, 2, 3, 0, 1520),
                make_mod(5.8, 1.9,  0.85, 1, 2, 0, 2090),
                make_mod(6.0, 1.4,  0.84, 1, 2, 0, 2320)
            }
        },

        // --- JAPANESE (ja) Dependency Tree & 4-Modality Graph ---
        {
            301, "ja", domain::NodeType::PhonemeGrapheme,
            "遅 (おそ / おく)", "[o-so / o-ku (LH / LHH)]",
            "Kanji radical: Movement 辶 + Sheep/犀 -> Slow / Late (遅い osoi, 遅れる okureru)",
            "WaniKani radical dependency: Road (辶) + Rhino/Slow component -> delayed motion.",
            "A rhino walking down the road (辶) moves slowly and arrives late.",
            "速 (はやい - fast)",
            "朝の電車が少し遅れています。",
            "The morning train is slightly delayed.",
            35, {},
            {
                make_mod(4.2, 26.4, 0.93, 4, 7, 0, 1050),
                make_mod(4.5, 21.0, 0.91, 4, 6, 0, 1180),
                make_mod(5.1, 12.8, 0.89, 3, 5, 0, 1690),
                make_mod(5.4, 9.2,  0.87, 3, 4, 1, 1980)
            }
        },
        {
            302, "ja", domain::NodeType::MorphosyntacticRule,
            "〜なければなりません", "[nakereba narimasen]",
            "Must / have to do (Double negative obligation construction)",
            "Negative stem (-nai -> -nakereba 'if not') + 'narimasen' ('does not become/work').",
            "Literally: 'If I do not do X, it will not do' -> polite obligation.",
            "〜てもいいです (may / permission)",
            "電車が遅れているから、早く駅に行かなければなりません。",
            "Because the train is delayed, I have to go to the station early.",
            48, {301},
            {
                make_mod(5.4, 8.6,  0.88, 3, 5, 1, 1460),
                make_mod(5.7, 6.8,  0.86, 3, 4, 1, 1620),
                make_mod(6.5, 3.9,  0.84, 2, 3, 1, 2240),
                make_mod(6.8, 2.8,  0.82, 2, 3, 2, 2490)
            }
        }
    };

    if (!execute_sql("BEGIN TRANSACTION;")) return false;
    for (const auto& n : seed_nodes) {
        save_node(n);
        graph.upsert_node(n);
    }

    // Seed Immersion Passages (95-98% comprehensible input sweet spot)
    const char* sql_pass = R"SQL(
        INSERT OR REPLACE INTO immersion_passages
            (passage_id, language_code, title, cefr_level, body_l2, pragmatic_summary)
        VALUES
            (1, 'de', 'Am Münchner Hauptbahnhof (ICE-Dialog A2-B1)', 'A2-B1',
             'Entschuldigung, fährt dieser Zug direkt nach München? Heute Morgen war es leider nicht meine Schuld, weil der Regionalzug schon zwanzig Minuten Verspätung hatte. Die Übung für unser Seminar beginnt um neun Uhr, deshalb musst du mir bitte sofort Bescheid sagen, sobald wir am Gleis ankommen.',
             'Multi-turn commuter dialogue integrating polite attention-getting (Entschuldigung), subordinate verb-final syntax (weil ... hatte), and nominalized -ung nouns (Verspätung, Übung) at 96.4% known-token density.'),
            (4, 'de', 'Passage 2 · Projektbesprechung im Berliner Studio (B1)', 'B1',
             'Entschuldigung für die kurze Verspätung! Es ist meine Schuld, weil die S-Bahn heute Morgen am Ostkreuz stand. Sag mir bitte Bescheid, ob die Rechnung für das Projekt schon fertig ist.',
             'Workplace stand-up dialogue reinforcing causal subordinate clauses (weil ... stand) and idiomatic dative collocations (mir Bescheid sagen).'),
            (2, 'es', 'Passage 1 · Cambio de Andén en Madrid Atocha (A2-B1)', 'A2-B1',
             'Tengo que salir temprano hacia la estación central aunque llueva toda la mañana en Madrid. No me di cuenta de que el tren rápido salía del andén cuatro, pero tenemos que llegar a tiempo.',
             'Transit scenario pairing obligation chunks (tener que) with hypothetical subjunctive concession (aunque llueva) and sudden realization (darse cuenta de).'),
            (5, 'es', 'Passage 2 · Reunión de Equipo en Barcelona (B1)', 'B1',
             'Me di cuenta de que tenemos que terminar el informe hoy aunque llueva o haya tráfico. Tienes que avisarme antes de las cinco.',
             'Professional coordination dialogue contrasting factual and hypothetical clauses with reflexive realization chunks.'),
            (3, 'ja', 'Passage 1 · 朝の東京駅ホームで (JLPT N4-N3)', 'N4-N3',
             'すみません、朝の電車が少し遅れています。今日は大切な会議があるから、早く駅に行かなければなりません。次の新幹線に間に合うように急ぎましょう。',
             'Morning station exchange linking kanji radical recognition (遅) with spoken polite obligation (〜なければなりません).'),
            (6, 'ja', 'Passage 2 · オフィスでの連絡とスケジュール調整 (JLPT N4)', 'N4',
             '電車が遅れているので、会議の前に資料を確認しなければなりません。少し遅れるかもしれません。',
             'Office coordination dialogue practicing causal clauses and double-negative obligation structures.');
    )SQL";
    execute_sql(sql_pass);

    execute_sql("INSERT OR IGNORE INTO passage_nodes VALUES (1, 101), (1, 102), (1, 103), (1, 104), (1, 105), (1, 106), (4, 101), (4, 102), (4, 103), (4, 104), (4, 105), (4, 106);");
    execute_sql("INSERT OR IGNORE INTO passage_nodes VALUES (2, 201), (2, 202), (2, 203), (5, 201), (5, 202), (5, 203);");
    execute_sql("INSERT OR IGNORE INTO passage_nodes VALUES (3, 301), (3, 302), (6, 301), (6, 302);");

    // Seed sample CRDT revlog events
    const char* sql_rev_seed = R"SQL(
        INSERT OR IGNORE INTO revlog
            (event_uuid, timestamp, node_id, modality, rating, response_time_ms,
             elapsed_days, scheduled_days, pre_stability, post_stability,
             pre_difficulty, post_difficulty, context_tag)
        VALUES
            ('ev-101-0-1792234400-9', 1792234400, 101, 0, 3, 920,  18.0, 42.5, 21.0, 42.5, 3.9, 3.8, 'stage1_reading'),
            ('ev-103-2-1792320800-7', 1792320800, 103, 2, 1, 2740,  5.0,  3.4, 11.2,  3.4, 8.1, 8.7, 'stage3_leech_detect'),
            ('ev-104-3-1792407200-3', 1792407200, 104, 3, 3, 2120,  2.5,  4.1,  2.1,  4.1, 6.4, 6.4, 'stage4_pushed_output');
    )SQL";
    execute_sql(sql_rev_seed);

    domain::EthicalHabitState default_habit{};
    save_habit_state(default_habit);

    return execute_sql("COMMIT;");
}

} // namespace kinetix::persistence
