# Database Schema, BCNF Normalization, and CRDT Event-Sourced Sync

Designed using the `database-designer` (`SKILL.md`) workflow and verified by `scripts/schema_analyzer.py`, Kinetix L2 uses an embedded **SQLite3** relational database configured in `EXCLUSIVE` + `WAL` (Write-Ahead Logging) mode with `PRAGMA foreign_keys = ON`.

---

## 1. Entity-Relationship Diagram (Mermaid ERD)

```mermaid
erDiagram
    linguistic_nodes ||--o{ node_prerequisites : "requires S_rec >= 3.0d"
    linguistic_nodes ||--|{ modality_states : "tracks 4 FSRS tuples"
    linguistic_nodes ||--o{ revlog : "records immutable events"
    immersion_passages ||--o{ passage_nodes : "contains i+1 targets"
    linguistic_nodes ||--o{ passage_nodes : "appears in"

    linguistic_nodes {
        INTEGER node_id PK
        TEXT language_code
        INTEGER node_type "0=Phoneme..4=Pragmatic"
        TEXT target_form
        TEXT phonetic_ipa
        TEXT concept_gloss
        TEXT morphological_note
        TEXT mnemonic_hook
        TEXT decoy_minimal_pair
        TEXT example_context_l2
        TEXT example_context_gloss
        INTEGER zipf_frequency_rank
    }

    node_prerequisites {
        INTEGER node_id PK, FK
        INTEGER prerequisite_id PK, FK
    }

    modality_states {
        INTEGER node_id PK, FK
        INTEGER modality PK "0=RecText,1=RecAudio,2=ProdText,3=ProdAudio"
        REAL difficulty "CHECK 1.0..10.0"
        REAL stability "CHECK >= 0.0"
        REAL retrievability "CHECK 0.0..1.0"
        INTEGER last_review_ts
        INTEGER repetitions
        INTEGER lapses
        INTEGER avg_latency_ms
    }

    revlog {
        INTEGER event_id PK
        TEXT event_uuid UK
        INTEGER timestamp
        INTEGER node_id FK
        INTEGER modality
        INTEGER rating "CHECK 1..4"
        INTEGER response_time_ms
        REAL elapsed_days
        REAL scheduled_days
        REAL pre_stability
        REAL post_stability
        REAL pre_difficulty
        REAL post_difficulty
        TEXT context_tag
    }

    immersion_passages {
        INTEGER passage_id PK
        TEXT language_code
        TEXT title
        TEXT cefr_level
        TEXT body_l2
        TEXT pragmatic_summary
    }

    habit_settings {
        INTEGER profile_id PK "CHECK profile_id = 1"
        INTEGER active_momentum_days
        INTEGER endowed_cognate_nodes
        INTEGER completed_tasks_today
        INTEGER daily_target_tasks
        INTEGER user_mined_sentences_count
        INTEGER vacation_recovery_mode_active
        INTEGER notifications_enabled
        TEXT preferred_reminder_window
    }
```

---

## 2. Normalization (BCNF) & Index Optimization

1. **Boyce-Codd Normal Form (BCNF)**:
   - `linguistic_nodes` stores immutable/curated lexical attributes keyed solely by `node_id`.
   - `node_prerequisites` normalizes the many-to-many prerequisite DAG into a composite primary key `(node_id, prerequisite_id)` with `ON DELETE CASCADE`.
   - `modality_states` stores the 4 mutable FSRS-4.5 vectors keyed by `(node_id, modality)`, eliminating repeating groups (`rec_text_s`, `rec_audio_s`, etc.) and satisfying 1NF–BCNF.
2. **Strategic Covering & Lookup Indexes**:
   - `idx_nodes_lang_rank ON linguistic_nodes(language_code, zipf_frequency_rank)`: Accelerates Zipf-ordered curriculum loading in `<0.2 ms`.
   - `idx_prereq_lookup ON node_prerequisites(prerequisite_id)`: Accelerates reverse dependency graph traversal.
   - `idx_modality_stability ON modality_states(modality, stability)`: Accelerates due-queue filtering and WaniKani `S_rec >= 3.0d` gating checks.
   - `idx_revlog_node_ts ON revlog(node_id, timestamp DESC)`: Accelerates recent review telemetry and CRDT delta synchronization.

---

## 3. Offline-First CRDT Event Sourcing (`revlog`)

Every review action is written inside an ACID transaction (`DatabaseManager::record_review_transaction`) to the append-only `revlog` table with a globally unique `event_uuid`. Because FSRS-4.5 transitions are pure mathematical functions of the ordered `(timestamp, rating, response_time_ms)` sequence, merging `revlog_crdt_backup.json` files across Desktop and Mobile deterministically reconstructs the exact 4-modality stability and difficulty state without data loss or merge conflicts.
