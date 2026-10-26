#include "kinetix/domain/habit_engine.hpp"
#include <algorithm>

namespace kinetix::domain {

double HabitEngine::compute_endowed_progress_ratio(const EthicalHabitState& state) const noexcept {
    // Endowed Progress Effect (Section 3 of the_anatomy_of_addictive_apps.md):
    // Give the learner a 2-step head start representing pre-credited foundational cognates/phonemes
    const double endowed_bonus = 2.0;
    const double numerator = endowed_bonus + static_cast<double>(state.completed_tasks_today);
    const double denominator = endowed_bonus + static_cast<double>(std::max(1, state.daily_target_tasks));
    return std::clamp(numerator / denominator, 0.0, 1.0);
}

void HabitEngine::record_task_completed(EthicalHabitState& state, bool mined_new_sentence) noexcept {
    state.completed_tasks_today += 1;
    if (mined_new_sentence) {
        state.user_mined_sentences_count += 1;
    }
    if (state.completed_tasks_today == state.daily_target_tasks) {
        state.active_momentum_days += 1;
    }
}

NextActionRecommendation HabitEngine::recommend_next_action(
    const EthicalHabitState& state,
    const std::vector<NodeId>& due_node_ids,
    NodeId active_story_unnoticed_node_id
) const {
    NextActionRecommendation rec{};
    rec.zeigarnik_completion_ratio = compute_endowed_progress_ratio(state);
    rec.fogg_ability_score = 0.96;

    // Ethical Stopping Cue (The 5-Minute Regret Test, Section 6 of the_anatomy_of_addictive_apps.md):
    // When the learner completes their daily target or clears due items, provide an explicit
    // "All Caught Up" stopping cue instead of infinite doomscrolling.
    if (state.completed_tasks_today >= state.daily_target_tasks && due_node_ids.empty()) {
        rec.session_complete_stopping_cue = true;
        rec.recommended_stage = PedagogicalStage::Stage5_UnscriptedSimulation;
        rec.headline = "Daily Cognitive Target Complete — Optimal Consolidation Reached";
        rec.rationale = "Working memory and hippocampal encoding are saturated for today's target. "
                        "Rest now to allow offline consolidation, or explore an optional Stage 5 dialogue.";
        return rec;
    }

    if (!due_node_ids.empty()) {
        rec.recommended_stage = PedagogicalStage::Stage3_AdaptiveActiveRetrieval;
        rec.target_node_id = due_node_ids.front();
        rec.target_modality = Modality::ProductiveText;
        rec.headline = "High-Yield Spacing Window Open (" + std::to_string(due_node_ids.size()) + " nodes at R ~ 90%)";
        rec.rationale = "Reviewing right before forgetting maximizes post-review stability growth S' (FSRS Spacing Effect).";
        return rec;
    }

    rec.recommended_stage = PedagogicalStage::Stage1_ComprehensibleInput;
    rec.target_node_id = active_story_unnoticed_node_id;
    rec.target_modality = Modality::ReceptiveText;
    rec.headline = "Read Next 96% Comprehensible Dialogue (Stage 1 i+1 Noticing)";
    rec.rationale = "Unlock new target expressions inside high-comprehension narrative context before active recall.";
    return rec;
}

} // namespace kinetix::domain
