#include "kinetix/domain/knowledge_graph.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace kinetix::domain {

KnowledgeGraph::KnowledgeGraph(FsrsEngine engine)
    : fsrs_(std::move(engine)) {}

void KnowledgeGraph::upsert_node(const LinguisticNode& node) {
    nodes_[node.id] = node;
}

const LinguisticNode* KnowledgeGraph::find_node(NodeId id) const noexcept {
    auto it = nodes_.find(id);
    return (it != nodes_.end()) ? &it->second : nullptr;
}

LinguisticNode* KnowledgeGraph::find_node_mutable(NodeId id) noexcept {
    auto it = nodes_.find(id);
    return (it != nodes_.end()) ? &it->second : nullptr;
}

std::vector<LinguisticNode> KnowledgeGraph::nodes_for_language(const std::string& language_code) const {
    std::vector<LinguisticNode> result;
    for (const auto& [id, node] : nodes_) {
        if (node.language_code == language_code) {
            result.push_back(node);
        }
    }
    std::sort(result.begin(), result.end(), [](const LinguisticNode& a, const LinguisticNode& b) {
        if (a.zipf_frequency_rank != b.zipf_frequency_rank) {
            return a.zipf_frequency_rank < b.zipf_frequency_rank;
        }
        return a.id < b.id;
    });
    return result;
}

bool KnowledgeGraph::is_productive_unlocked(NodeId id) const noexcept {
    const LinguisticNode* node = find_node(id);
    if (!node) {
        return false;
    }
    // First check that the node's own receptive foundation has been noticed (S_rec >= 1.0d)
    const double own_rec_text_s = node->modalities[static_cast<size_t>(Modality::ReceptiveText)].stability;
    const double own_rec_audio_s = node->modalities[static_cast<size_t>(Modality::ReceptiveAudio)].stability;
    if (std::max(own_rec_text_s, own_rec_audio_s) < 1.0) {
        return false;
    }

    // WaniKani Dependency Gating Rule (Section 3.1):
    // Every prerequisite node must have achieved Receptive Stability S_rec >= 3.0 days.
    for (NodeId prereq_id : node->prerequisites) {
        const LinguisticNode* prereq = find_node(prereq_id);
        if (!prereq) {
            continue;
        }
        const double rec_text_s = prereq->modalities[static_cast<size_t>(Modality::ReceptiveText)].stability;
        const double rec_audio_s = prereq->modalities[static_cast<size_t>(Modality::ReceptiveAudio)].stability;
        if (rec_text_s < kPrerequisiteStabilityFloorDays || rec_audio_s < kPrerequisiteStabilityFloorDays) {
            return false;
        }
    }
    return true;
}

std::vector<NodeId> KnowledgeGraph::blocking_prerequisites(NodeId id) const {
    std::vector<NodeId> blocking;
    const LinguisticNode* node = find_node(id);
    if (!node) {
        return blocking;
    }
    for (NodeId prereq_id : node->prerequisites) {
        const LinguisticNode* prereq = find_node(prereq_id);
        if (!prereq) {
            continue;
        }
        const double rec_text_s = prereq->modalities[static_cast<size_t>(Modality::ReceptiveText)].stability;
        const double rec_audio_s = prereq->modalities[static_cast<size_t>(Modality::ReceptiveAudio)].stability;
        if (rec_text_s < kPrerequisiteStabilityFloorDays || rec_audio_s < kPrerequisiteStabilityFloorDays) {
            blocking.push_back(prereq_id);
        }
    }
    return blocking;
}

LeechInterventionPlan KnowledgeGraph::evaluate_leech_intervention(NodeId id, Modality modality) const {
    LeechInterventionPlan plan{};
    plan.node_id = id;
    plan.struggling_modality = modality;

    const LinguisticNode* node = find_node(id);
    if (!node) {
        return plan;
    }

    const FsrsMemoryState& state = node->modalities[static_cast<size_t>(modality)];
    plan.lapse_count = state.lapses;
    plan.current_difficulty = state.difficulty;

    if (state.lapses > kLeechLapseThreshold || state.difficulty > kLeechDifficultyThreshold) {
        plan.is_leech = true;
        plan.unstable_prerequisites = blocking_prerequisites(id);
        plan.decomposed_etymology_mnemonic = node->mnemonic_hook.empty()
            ? ("Decompose morphemes of '" + node->target_form + "' (" + node->morphological_note + ")")
            : node->mnemonic_hook;
        plan.contrastive_minimal_pair_drill = node->decoy_minimal_pair.empty()
            ? ("Contrast target '" + node->target_form + "' [" + node->phonetic_ipa + "] against nearest neighbor")
            : ("Contrast '" + node->target_form + "' vs. decoy '" + node->decoy_minimal_pair + "'");

        std::ostringstream oss;
        if (!plan.unstable_prerequisites.empty()) {
            oss << "1. Stabilize " << plan.unstable_prerequisites.size()
                << " prerequisite component(s) to S_rec >= 3.0d first. ";
        } else {
            oss << "1. Prerequisites stable (S_rec >= 3.0d). ";
        }
        oss << "2. Anchor via mnemonic: " << plan.decomposed_etymology_mnemonic << ". "
            << "3. Complete contrastive minimal-pair discrimination (" << plan.contrastive_minimal_pair_drill << ").";
        plan.recommended_remediation_action = oss.str();
    }
    return plan;
}

FsrsScheduleResult KnowledgeGraph::record_review(
    NodeId id,
    Modality modality,
    Rating user_rating,
    TimestampSec timestamp_sec,
    int32_t response_time_ms,
    const std::string& context_tag
) {
    LinguisticNode* node = find_node_mutable(id);
    if (!node) {
        return FsrsScheduleResult{};
    }

    FsrsMemoryState& current = node->modalities[static_cast<size_t>(modality)];
    Rating effective_rating = user_rating;

    // Pedagogical Non-Negotiable #8 (Section 3.4):
    // Enforce latency threshold (< 2500ms) for automatization in Productive Audio/Text when S >= 7d.
    // If learner takes > 2500ms to produce, cap rating at Rating::Hard so slow declarative
    // translation cannot masquerade as procedural automatization.
    if ((modality == Modality::ProductiveAudio || modality == Modality::ProductiveText) &&
        current.stability >= 7.0 &&
        response_time_ms > kAutomatizationLatencyCeilingMs &&
        (effective_rating == Rating::Good || effective_rating == Rating::Easy)) {
        effective_rating = Rating::Hard;
    }

    FsrsScheduleResult res = fsrs_.step_review(
        id,
        modality,
        current,
        effective_rating,
        timestamp_sec,
        response_time_ms,
        context_tag
    );

    current = res.next_state;
    return res;
}

int32_t KnowledgeGraph::apply_resilient_momentum_shift(int32_t paused_days) {
    if (paused_days <= 0) {
        return 0;
    }
    const int64_t shift_sec = static_cast<int64_t>(paused_days) * 86400LL;
    int32_t shifted_count = 0;
    for (auto& [id, node] : nodes_) {
        for (auto& mod_state : node.modalities) {
            if (mod_state.is_initialized() && mod_state.last_review_ts > 0) {
                mod_state.last_review_ts += shift_sec;
                ++shifted_count;
            }
        }
    }
    return shifted_count;
}

std::vector<DueReviewTask> KnowledgeGraph::build_due_queue(
    const std::string& language_code,
    TimestampSec current_ts,
    size_t max_items
) const {
    std::vector<DueReviewTask> queue;

    for (const auto& [id, node] : nodes_) {
        if (node.language_code != language_code) {
            continue;
        }
        const bool prod_unlocked = is_productive_unlocked(id);

        for (Modality mod : kAllModalities) {
            const bool is_prod = (mod == Modality::ProductiveText || mod == Modality::ProductiveAudio);
            if (is_prod && !prod_unlocked) {
                continue; // Strictly gated by WaniKani dependency rule
            }

            const FsrsMemoryState& st = node.modalities[static_cast<size_t>(mod)];
            if (!st.is_initialized()) {
                // Only schedule uninitialized receptive modalities if prerequisites are met
                if (!is_prod && blocking_prerequisites(id).empty()) {
                    queue.push_back(DueReviewTask{
                        id,
                        mod,
                        StabilityTier::Tier0_InitialEncoding,
                        0.0,
                        0.0,
                        0.50 + (1000.0 / std::max(10, node.zipf_frequency_rank)),
                        false
                    });
                }
                continue;
            }

            const double elapsed_days = (current_ts > st.last_review_ts)
                ? static_cast<double>(current_ts - st.last_review_ts) / 86400.0
                : 0.0;
            const double r = fsrs_.compute_retrievability(elapsed_days, st.stability);
            const LeechInterventionPlan leech = evaluate_leech_intervention(id, mod);

            // Item is due when Retrievability drops near or below target retention (0.90) or is a leech
            if (r <= 0.91 || leech.is_leech) {
                const double urgency = (1.0 - r) * 10.0 + (leech.is_leech ? 2.5 : 0.0);
                queue.push_back(DueReviewTask{
                    id,
                    mod,
                    FsrsEngine::classify_stability_tier(st.stability),
                    r,
                    st.stability,
                    urgency,
                    leech.is_leech
                });
            }
        }
    }

    std::sort(queue.begin(), queue.end(), [](const DueReviewTask& a, const DueReviewTask& b) {
        return a.urgency_score > b.urgency_score;
    });

    if (queue.size() > max_items) {
        queue.resize(max_items);
    }
    return queue;
}

double KnowledgeGraph::compute_passage_comprehension_density(const ImmersionPassage& passage) const {
    if (passage.linked_nodes.empty()) {
        return 0.96;
    }
    int known_or_consolidating = 0;
    for (NodeId nid : passage.linked_nodes) {
        const LinguisticNode* node = find_node(nid);
        if (!node) {
            continue;
        }
        const double s_rec = node->modalities[static_cast<size_t>(Modality::ReceptiveText)].stability;
        if (s_rec >= 3.0) {
            ++known_or_consolidating;
        }
    }
    // Surrounding baseline high-frequency function words represent 85% of running tokens
    const double target_node_ratio = static_cast<double>(known_or_consolidating) /
                                     static_cast<double>(passage.linked_nodes.size());
    return std::clamp(0.90 + 0.09 * target_node_ratio, 0.85, 0.99);
}

GraphTelemetrySummary KnowledgeGraph::compute_summary(
    const std::string& language_code,
    TimestampSec current_ts
) const {
    GraphTelemetrySummary summary{};
    double r_sum = 0.0;
    int r_count = 0;
    std::array<int, 4> mod_counts = {0, 0, 0, 0};

    for (const auto& [id, node] : nodes_) {
        if (node.language_code != language_code) {
            continue;
        }
        ++summary.total_nodes;

        const FsrsMemoryState& rec_text = node.modalities[static_cast<size_t>(Modality::ReceptiveText)];
        const FamiliarityState fam = FsrsEngine::classify_familiarity(rec_text);
        if (fam == FamiliarityState::Unseen) {
            ++summary.unseen_nodes;
        } else if (fam == FamiliarityState::Automatized) {
            ++summary.automatized_nodes;
        } else {
            ++summary.learning_nodes;
        }

        if (is_productive_unlocked(id)) {
            ++summary.productive_unlocked_nodes;
        }

        bool node_has_leech = false;
        for (size_t m = 0; m < 4; ++m) {
            const FsrsMemoryState& st = node.modalities[m];
            if (st.lapses > kLeechLapseThreshold || st.difficulty > kLeechDifficultyThreshold) {
                node_has_leech = true;
            }
            if (st.is_initialized()) {
                const double elapsed = (current_ts > st.last_review_ts)
                    ? static_cast<double>(current_ts - st.last_review_ts) / 86400.0
                    : 0.0;
                const double r = fsrs_.compute_retrievability(elapsed, st.stability);
                summary.modality_mean_stability[m] += st.stability;
                summary.modality_retention_rate[m] += r;
                mod_counts[m] += 1;
                r_sum += r;
                ++r_count;
            }
        }
        if (node_has_leech) {
            ++summary.active_leeches;
        }
    }

    summary.average_retrievability = (r_count > 0) ? (r_sum / r_count) : 0.92;
    for (size_t m = 0; m < 4; ++m) {
        if (mod_counts[m] > 0) {
            summary.modality_mean_stability[m] /= mod_counts[m];
            summary.modality_retention_rate[m] /= mod_counts[m];
        }
    }
    return summary;
}

} // namespace kinetix::domain
