#pragma once
// Ethical Behavioral Retention & Habit Formation Engine
// Synthesizes B.J. Fogg's Behavior Model (B = MAP), Nir Eyal's Hook Model,
// Zeigarnik Open-Loop Tension, Endowed Progress, and the Ethical "Regret Test" Stopping Cue.

#include "kinetix/domain/types.hpp"
#include <string>
#include <vector>

namespace kinetix::domain {

struct EthicalHabitState {
    int32_t active_momentum_days = 7;          // Resilient momentum counter (never resets to 0 on 1 missed day)
    int32_t endowed_cognate_nodes = 5;         // Endowed Progress: pre-credited foundational cognates
    int32_t completed_tasks_today = 0;
    int32_t daily_target_tasks = 12;           // Achievable 4-minute daily micro-loop target
    int32_t user_mined_sentences_count = 3;    // IKEA Effect investment counter
    bool vacation_recovery_mode_active = false;
    bool notifications_enabled = false;        // Opt-in only by default (ethical retention)
    std::string preferred_reminder_window = "08:30 Local (User-Controlled)";
};

struct NextActionRecommendation {
    PedagogicalStage recommended_stage = PedagogicalStage::Stage1_ComprehensibleInput;
    NodeId target_node_id = 0;
    Modality target_modality = Modality::ReceptiveText;
    std::string headline;
    std::string rationale;
    double fogg_ability_score = 0.95;          // 0.0..1.0 measure of low cognitive/UI friction
    double zeigarnik_completion_ratio = 0.0;   // Progress bar ratio including endowed head-start
    bool session_complete_stopping_cue = false;// True when daily goal met (passes 5-Minute Regret Test)
};

class HabitEngine {
public:
    HabitEngine() = default;

    // Compute progress ratio incorporating the Endowed Progress Effect (2 base steps pre-filled)
    double compute_endowed_progress_ratio(const EthicalHabitState& state) const noexcept;

    // Record completion of a learning action and update momentum without punitive streak anxiety
    void record_task_completed(EthicalHabitState& state, bool mined_new_sentence = false) noexcept;

    // Determine the single clearest next action (Core Engagement Loop Steps 1-6, Section 6)
    NextActionRecommendation recommend_next_action(
        const EthicalHabitState& state,
        const std::vector<NodeId>& due_node_ids,
        NodeId active_story_unnoticed_node_id
    ) const;
};

} // namespace kinetix::domain
