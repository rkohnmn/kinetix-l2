#pragma once
// Unified Learner Knowledge Graph & WaniKani-Style Dependency Gating Engine
// Implements 4-modality state tracking, prerequisite gating (S_rec >= 3.0 days),
// i+1 comprehension percentage filtering, and the Strict Anti-Leech Intervention protocol.

#include "kinetix/domain/types.hpp"
#include "kinetix/domain/fsrs_engine.hpp"
#include <map>
#include <string>
#include <vector>

namespace kinetix::domain {

// Anti-Leech Intervention Sub-Routine Diagnostic (Section 3.2 Stage 3)
struct LeechInterventionPlan {
    bool is_leech = false;
    NodeId node_id = 0;
    Modality struggling_modality = Modality::ProductiveText;
    int32_t lapse_count = 0;
    double current_difficulty = 5.0;
    std::vector<NodeId> unstable_prerequisites;
    std::string decomposed_etymology_mnemonic;
    std::string contrastive_minimal_pair_drill;
    std::string recommended_remediation_action;
};

// Due Review Item in the Priority Queue
struct DueReviewTask {
    NodeId node_id = 0;
    Modality modality = Modality::ReceptiveText;
    StabilityTier tier = StabilityTier::Tier0_InitialEncoding;
    double current_retrievability = 0.0;
    double current_stability = 0.0;
    double urgency_score = 0.0;       // Higher when R drops below 0.90
    bool requires_leech_intervention = false;
};

// Graph-wide Mastery & Modality Summary Metrics
struct GraphTelemetrySummary {
    int32_t total_nodes = 0;
    int32_t unseen_nodes = 0;
    int32_t learning_nodes = 0;       // Familiarity 1..3
    int32_t automatized_nodes = 0;    // S >= 30d
    int32_t productive_unlocked_nodes = 0;
    int32_t active_leeches = 0;
    double average_retrievability = 0.0;
    std::array<double, 4> modality_mean_stability = {0.0, 0.0, 0.0, 0.0};
    std::array<double, 4> modality_retention_rate = {0.0, 0.0, 0.0, 0.0};
};

class KnowledgeGraph {
public:
    static constexpr double kPrerequisiteStabilityFloorDays = 3.0; // WaniKani Guru gating rule
    static constexpr int32_t kLeechLapseThreshold = 4;             // Lapses > 4 triggers intervention
    static constexpr double kLeechDifficultyThreshold = 8.5;       // D > 8.5 triggers intervention
    static constexpr int32_t kAutomatizationLatencyCeilingMs = 2500; // < 2.5s spoken latency rule

    explicit KnowledgeGraph(FsrsEngine engine = FsrsEngine{});

    void upsert_node(const LinguisticNode& node);
    const LinguisticNode* find_node(NodeId id) const noexcept;
    LinguisticNode* find_node_mutable(NodeId id) noexcept;
    std::vector<LinguisticNode> nodes_for_language(const std::string& language_code) const;
    const std::map<NodeId, LinguisticNode>& all_nodes() const noexcept { return nodes_; }

    // WaniKani Dependency Gating Rule (Section 3.1):
    // A node N_k cannot be scheduled for Productive practice until all prerequisite nodes
    // have achieved Receptive Stability S_rec >= 3.0 days.
    bool is_productive_unlocked(NodeId id) const noexcept;
    std::vector<NodeId> blocking_prerequisites(NodeId id) const;

    // Anti-Leech Protocol (Section 3.2 Stage 3):
    // Detects if lapses > 4 or D > 8.5 and builds a 3-step remediation plan.
    LeechInterventionPlan evaluate_leech_intervention(NodeId id, Modality modality) const;

    // Record a review event, enforcing latency penalties on Tier 3 Spoken/Written automatization
    FsrsScheduleResult record_review(
        NodeId id,
        Modality modality,
        Rating user_rating,
        TimestampSec timestamp_sec,
        int32_t response_time_ms,
        const std::string& context_tag = "interactive_session"
    );

    // Apply Resilient Momentum (Ethical Vacation Shift): shifts last_review_ts forward
    // by paused_days so learners returning from illness/travel are not crushed by backlogs.
    int32_t apply_resilient_momentum_shift(int32_t paused_days);

    // Build prioritized FSRS due queue for a given language and timestamp
    std::vector<DueReviewTask> build_due_queue(
        const std::string& language_code,
        TimestampSec current_ts,
        size_t max_items = 25
    ) const;

    // Compute quantifiable i+1 comprehension ratio (0.0 to 1.0) for an ImmersionPassage
    double compute_passage_comprehension_density(const ImmersionPassage& passage) const;

    // Compute aggregated telemetry across all 4 modalities
    GraphTelemetrySummary compute_summary(
        const std::string& language_code,
        TimestampSec current_ts
    ) const;

    const FsrsEngine& fsrs() const noexcept { return fsrs_; }

private:
    FsrsEngine fsrs_;
    std::map<NodeId, LinguisticNode> nodes_;
};

} // namespace kinetix::domain
