#pragma once
// Kinetix L2 Core Domain Types & Enumerations
// Implements strong domain types for the 4-Modality Learner Knowledge Graph,
// FSRS-4.5/5 memory tuples, WaniKani-style dependency gating, and the 5-stage SLA loop.

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <optional>

namespace kinetix::domain {

using NodeId = int64_t;
using TimestampSec = int64_t;

// 1. Node Types in the Linguistic Knowledge Graph (Section 3.1)
enum class NodeType : int {
    PhonemeGrapheme = 0,       // Individual sounds, tonal contours, minimal pairs, script radicals
    LexicalLemma = 1,          // Root words linked to high-frequency inflected forms
    LexicalChunk = 2,          // Multi-word formulaic sequences & collocations (Lewis's Lexical Approach)
    MorphosyntacticRule = 3,   // Abstract grammar constructions (e.g., German verb-final weil clause)
    PragmaticRegister = 4      // Sociolinguistic rules (T-V distinction, keigo politeness tiers)
};

// 2. Four Independent Skill Modalities per Linguistic Node (Section 3.1)
enum class Modality : int {
    ReceptiveText = 0,         // M_rec_text: Comprehend when reading in context
    ReceptiveAudio = 1,        // M_rec_audio: Parse when spoken at natural speed without subtitles
    ProductiveText = 2,        // M_prod_text: Actively retrieve and spell/inflect without word banks
    ProductiveAudio = 3        // M_prod_audio: Retrieve and articulate within < 2500ms latency
};

constexpr std::array<Modality, 4> kAllModalities = {
    Modality::ReceptiveText,
    Modality::ReceptiveAudio,
    Modality::ProductiveText,
    Modality::ProductiveAudio
};

// 3. FSRS Review Rating (1 = Again/Forgot, 2 = Hard, 3 = Good, 4 = Easy)
enum class Rating : int {
    Again = 1,
    Hard = 2,
    Good = 3,
    Easy = 4
};

// 4. Token-Level Familiarity Gradient (LingQ / Readlang Stateful Model, Section 2.1.3)
enum class FamiliarityState : int {
    Unseen = 0,                // 0 = Unseen (Blue highlight in reader)
    Noticed = 1,               // 1 = Newly noticed / Tier 0 encoding (Amber-300)
    Fragile = 2,               // 2 = Fragile declarative 1<=S<7d (Amber-200)
    Consolidating = 3,         // 3 = Consolidating 7<=S<30d (Amber-100)
    Automatized = 4            // 4 = Known / Automatized S>=30d (Clean text, no highlight)
};

// 5. Dynamic Exercise Escalation Tiers by FSRS Stability S (Section 3.2 Stage 3)
enum class StabilityTier : int {
    Tier0_InitialEncoding = 0,     // S < 1.0 day: Audio-to-concept binding, back-chaining
    Tier1_FragileDeclarative = 1,  // 1.0 <= S < 7.0 days: Contextual cloze deletion, audio dictation
    Tier2_Consolidating = 2,       // 7.0 <= S < 30.0 days: Full sentence transformation
    Tier3_Automatizing = 3         // S >= 30.0 days: Timed oral/syntactic production (<2500ms)
};

// 6. The 5-Stage Pedagogical Loop (Section 3.2)
enum class PedagogicalStage : int {
    Stage1_ComprehensibleInput = 1,
    Stage2_JustInTimeDeconstruction = 2,
    Stage3_AdaptiveActiveRetrieval = 3,
    Stage4_PushedSyntacticOutput = 4,
    Stage5_UnscriptedSimulation = 5
};

// 7. FSRS Memory State Tuple <D, S, R, t_last, reps, lapses>
struct FsrsMemoryState {
    double difficulty = 5.0;           // D in [1.0, 10.0]
    double stability = 0.0;            // S > 0 (days for R to decay from 1.0 to 0.90)
    double retrievability = 0.0;       // R in [0.0, 1.0]
    TimestampSec last_review_ts = 0;   // Unix timestamp (seconds) of last review
    int32_t repetitions = 0;           // Total review count in this modality
    int32_t lapses = 0;                // Total lapse (Rating::Again) count
    int32_t avg_latency_ms = 0;        // Moving average production/retrieval latency in ms

    bool is_initialized() const noexcept { return repetitions > 0 && stability > 0.0; }
};

// 8. Linguistic Node in the Unified Learner Knowledge Graph
struct LinguisticNode {
    NodeId id = 0;
    std::string language_code;         // e.g., "es" (Spanish), "ja" (Japanese), "de" (German)
    NodeType type = NodeType::LexicalLemma;
    std::string target_form;           // L2 orthographic form (e.g., "Entschuldigung", "tener que")
    std::string phonetic_ipa;          // IPA / pitch accent / prosodic contour (e.g., "[ɛntˈʃʊldɪɡʊŋ]")
    std::string concept_gloss;         // Concise concept cue + usage note
    std::string morphological_note;    // Contrastive micro-grammar or etymology note
    std::string mnemonic_hook;         // Elaborative encoding / anti-leech decomposition mnemonic
    std::string decoy_minimal_pair;    // Contrastive minimal pair for anti-leech intervention
    std::string example_context_l2;    // Contextual i+1 sentence in L2
    std::string example_context_gloss; // Pragmatic meaning of context sentence
    int32_t zipf_frequency_rank = 100; // Frequency rank (1 = highest frequency)
    std::vector<NodeId> prerequisites; // Prerequisite NodeIds that must have S_rec >= 3.0d
    std::array<FsrsMemoryState, 4> modalities{}; // Indexed by static_cast<size_t>(Modality)
};

// 9. Append-Only Event-Sourced Review Log Entry (CRDT Sync Ready, Section 3.3.3)
struct RevlogEntry {
    int64_t event_id = 0;
    std::string event_uuid;
    TimestampSec timestamp = 0;
    NodeId node_id = 0;
    Modality modality = Modality::ReceptiveText;
    Rating rating = Rating::Good;
    int32_t response_time_ms = 0;
    double elapsed_days = 0.0;
    double scheduled_days = 0.0;
    double pre_stability = 0.0;
    double post_stability = 0.0;
    double pre_difficulty = 0.0;
    double post_difficulty = 0.0;
    std::string context_tag;
};

// 10. Immersion Passage for Stage 1 Comprehensible Input (i + 1)
struct ImmersionPassage {
    int64_t passage_id = 0;
    std::string language_code;
    std::string title;
    std::string cefr_level;            // e.g., "A2", "B1"
    std::string body_l2;
    std::string pragmatic_summary;
    std::vector<NodeId> linked_nodes;
};

const char* to_string(NodeType type) noexcept;
const char* to_string(Modality modality) noexcept;
const char* to_string(StabilityTier tier) noexcept;
const char* to_string(FamiliarityState state) noexcept;
const char* to_string(PedagogicalStage stage) noexcept;

} // namespace kinetix::domain
