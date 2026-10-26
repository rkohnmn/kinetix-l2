#pragma once
// 5-Stage Second Language Acquisition (SLA) Pedagogical Loop Engine
// Implements Stage 1 (Interlinear i+1 Noticing), Stage 2 (Deconstruction & Pimsleur Back-Chaining),
// Stage 3 (Dynamic FSRS Active Retrieval), Stage 4 (Pushed Syntactic Output),
// and Stage 5 (Graph-Constrained Conversational Recast Simulation).

#include "kinetix/domain/types.hpp"
#include "kinetix/domain/knowledge_graph.hpp"
#include <string>
#include <vector>

namespace kinetix::domain {

// Annotated Token for Stage 1 Interlinear Immersion Reader
struct AnnotatedToken {
    std::string surface_text;
    bool is_word = false;
    NodeId matched_node_id = 0;
    FamiliarityState familiarity = FamiliarityState::Automatized;
    std::string inline_gloss;
    std::string phonetic_ipa;
    std::string grammar_badge;        // Schmidt's Noticing enhancement badge
};

// Prosodic Back-Chaining Step (Pimsleur Auditory-Motor Loop, Section 2.1.1)
struct BackChainSegment {
    int32_t step_index = 0;
    std::string syllable_slice;       // e.g., "-chuldigung" -> "-schuldigung" -> "Entschuldigung"
    std::string ipa_slice;
    int32_t target_duration_ms = 1200;
    std::vector<double> pitch_contour_hz; // Native pitch envelope for visual prosody matching
};

// Dynamic Exercise Prompt (Stages 2, 3, 4)
struct PedagogicalExercise {
    NodeId node_id = 0;
    Modality target_modality = Modality::ProductiveText;
    PedagogicalStage stage = PedagogicalStage::Stage3_AdaptiveActiveRetrieval;
    StabilityTier stability_tier = StabilityTier::Tier1_FragileDeclarative;
    std::string exercise_type_label;  // e.g., "Contextual Cloze Recall (Zero Word Bank)"
    std::string situational_prompt;   // Pragmatic scenario or context sentence with [___]
    std::string concept_cue;          // Non-translation concept cue / morphological stem
    std::string expected_answer;
    std::vector<std::string> accepted_variants;
    std::vector<std::string> tier0_recognition_choices; // ONLY populated when StabilityTier == Tier0
    std::vector<BackChainSegment> back_chain_steps;
    int32_t time_limit_ms = 0;        // 0 = untimed; 2500ms for Tier 3 / Stage 4 Automatization
    bool is_leech_remediation = false;
    std::string remediation_note;
};

// Evaluation Outcome of a Learner's Productive Attempt
struct ExerciseEvaluation {
    bool is_correct = false;
    bool met_latency_target = true;
    int32_t edit_distance = 0;
    Rating suggested_fsrs_rating = Rating::Good;
    std::string diagnostic_feedback;
    std::string recast_form;          // Long's Interaction Hypothesis implicit recast
    std::string metalinguistic_note;  // Swain's Output Hypothesis structural note
};

// Stage 5 Graph-Constrained Conversational Turn
struct SimulationTurn {
    std::string speaker_role;         // "Tutor (Graph-Constrained)" or "Learner"
    std::string utterance_l2;
    std::string implicit_recast_l2;   // Non-intrusive recast if learner made an error
    std::string diagnostic_note;      // Sub-surface explicit grammar/prosody diagnostic
    NodeId elicited_node_id = 0;
    double known_word_compliance_ratio = 1.0; // Enforces >= 95% known-lemma constraint
};

class PedagogicalLoopEngine {
public:
    explicit PedagogicalLoopEngine(const KnowledgeGraph& graph);

    // Stage 1: Tokenize an immersion passage and annotate with Stateful Familiarity (0..4)
    // and Schmidt's Noticing highlights.
    std::vector<AnnotatedToken> annotate_passage(const ImmersionPassage& passage) const;

    // Stage 2: Generate Pimsleur-style Prosodic Back-Chaining segments from the end of the word
    // to the beginning, with synthetic native pitch contours (Hz).
    static std::vector<BackChainSegment> generate_back_chaining(const LinguisticNode& node);

    // Stage 3 & 4: Generate a dynamic exercise strictly governed by the node's StabilityTier
    // and Anti-Leech status (forbids word banks for Tier 1, Tier 2, Tier 3).
    PedagogicalExercise generate_exercise(
        NodeId node_id,
        Modality modality,
        PedagogicalStage preferred_stage = PedagogicalStage::Stage3_AdaptiveActiveRetrieval
    ) const;

    // Evaluate learner's written or spoken-transcribed response with Levenshtein distance,
    // morphological stem checking, and DeKeyser automatization latency enforcement (<2500ms).
    ExerciseEvaluation evaluate_attempt(
        const PedagogicalExercise& exercise,
        const std::string& user_response,
        int32_t response_time_ms
    ) const;

    // Stage 5: Generate a graph-constrained conversational simulation exchange that steers
    // dialogue toward due FSRS nodes while keeping >=95% of tokens within known lemmas.
    SimulationTurn generate_simulation_turn(
        const std::string& language_code,
        const std::string& learner_input,
        NodeId target_due_node_id
    ) const;

    // Compute UTF-8-aware Levenshtein edit distance for orthographic/phonetic scoring
    static int32_t compute_edit_distance(const std::string& a, const std::string& b);

private:
    const KnowledgeGraph& graph_;
};

} // namespace kinetix::domain
