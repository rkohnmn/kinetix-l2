#include "kinetix/domain/fsrs_engine.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <sstream>

namespace kinetix::domain {

const char* to_string(NodeType type) noexcept {
    switch (type) {
        case NodeType::PhonemeGrapheme:     return "Phoneme / Grapheme";
        case NodeType::LexicalLemma:        return "Lexical Lemma";
        case NodeType::LexicalChunk:        return "Formulaic Chunk";
        case NodeType::MorphosyntacticRule: return "Morphosyntactic Rule";
        case NodeType::PragmaticRegister:   return "Pragmatic Register";
    }
    return "Lexical Lemma";
}

const char* to_string(Modality modality) noexcept {
    switch (modality) {
        case Modality::ReceptiveText:   return "Receptive Text (Read)";
        case Modality::ReceptiveAudio:  return "Receptive Audio (Listen)";
        case Modality::ProductiveText:  return "Productive Text (Write)";
        case Modality::ProductiveAudio: return "Productive Audio (Speak <2.5s)";
    }
    return "Receptive Text";
}

const char* to_string(StabilityTier tier) noexcept {
    switch (tier) {
        case StabilityTier::Tier0_InitialEncoding:    return "Tier 0: Initial Encoding (S < 1d)";
        case StabilityTier::Tier1_FragileDeclarative: return "Tier 1: Fragile Declarative (1d <= S < 7d)";
        case StabilityTier::Tier2_Consolidating:      return "Tier 2: Consolidating (7d <= S < 30d)";
        case StabilityTier::Tier3_Automatizing:       return "Tier 3: Automatizing (S >= 30d)";
    }
    return "Tier 0";
}

const char* to_string(FamiliarityState state) noexcept {
    switch (state) {
        case FamiliarityState::Unseen:        return "0: Unseen (i+1 Candidate)";
        case FamiliarityState::Noticed:       return "1: Noticed (Encoding)";
        case FamiliarityState::Fragile:       return "2: Fragile (Declarative)";
        case FamiliarityState::Consolidating: return "3: Consolidating (Procedural)";
        case FamiliarityState::Automatized:   return "4: Automatized (Known)";
    }
    return "0: Unseen";
}

const char* to_string(PedagogicalStage stage) noexcept {
    switch (stage) {
        case PedagogicalStage::Stage1_ComprehensibleInput:      return "Stage 1: Comprehensible Input (i+1)";
        case PedagogicalStage::Stage2_JustInTimeDeconstruction: return "Stage 2: Prosodic & Rule Deconstruction";
        case PedagogicalStage::Stage3_AdaptiveActiveRetrieval:  return "Stage 3: Adaptive FSRS Active Recall";
        case PedagogicalStage::Stage4_PushedSyntacticOutput:    return "Stage 4: Pushed Syntactic Output";
        case PedagogicalStage::Stage5_UnscriptedSimulation:     return "Stage 5: Graph-Constrained Roleplay";
    }
    return "Stage 3: Adaptive FSRS Active Recall";
}

FsrsEngine::FsrsEngine(FsrsParameters params)
    : params_(params) {}

double FsrsEngine::compute_retrievability(double elapsed_days, double stability) const noexcept {
    if (stability <= 0.0001) [[unlikely]] {
        return 0.0;
    }
    if (elapsed_days <= 0.0) [[unlikely]] {
        return 1.0;
    }
    const double base = 1.0 + kDecayFactorF * (elapsed_days / stability);
    const double r = std::pow(base, kDecayPowerC);
    return std::clamp(r, 0.0, 1.0);
}

double FsrsEngine::compute_next_interval(double stability, double target_retention) const noexcept {
    if (stability <= 0.0001) [[unlikely]] {
        return 0.0;
    }
    const double r_target = (target_retention > 0.0 && target_retention < 1.0)
        ? target_retention
        : params_.desired_retention;
    const double factor = (std::pow(r_target, 1.0 / kDecayPowerC) - 1.0) / kDecayFactorF;
    const double raw_interval = stability * factor;
    return std::clamp(raw_interval, 0.25, params_.maximum_interval_days);
}

double FsrsEngine::initial_stability(Rating rating) const noexcept {
    const int idx = std::clamp(static_cast<int>(rating) - 1, 0, 3);
    return std::max(0.1, params_.w[static_cast<size_t>(idx)]);
}

double FsrsEngine::initial_difficulty(Rating rating) const noexcept {
    const double g = static_cast<double>(rating);
    const double d0 = params_.w[4] - std::exp(params_.w[5] * (g - 1.0)) + 1.0;
    return std::clamp(d0, 1.0, 10.0);
}

double FsrsEngine::next_difficulty(double current_d, Rating rating) const noexcept {
    const double g = static_cast<double>(rating);
    const double d0_good = initial_difficulty(Rating::Good);
    const double delta_d = -params_.w[6] * (g - 3.0);
    const double damped = current_d + delta_d * ((10.0 - current_d) / 9.0);
    const double reverted = params_.w[7] * d0_good + (1.0 - params_.w[7]) * damped;
    return std::clamp(reverted, 1.0, 10.0);
}

double FsrsEngine::next_recall_stability(double d, double s, double r, Rating rating) const noexcept {
    const double safe_s = std::max(0.1, s);
    const double safe_d = std::clamp(d, 1.0, 10.0);
    const double safe_r = std::clamp(r, 0.01, 1.0);

    const double hard_penalty = (rating == Rating::Hard) ? params_.w[15] : 1.0;
    const double easy_bonus = (rating == Rating::Easy) ? params_.w[16] : 1.0;

    const double growth = std::exp(params_.w[8]) *
                          (11.0 - safe_d) *
                          std::pow(safe_s, -params_.w[9]) *
                          (std::exp(params_.w[10] * (1.0 - safe_r)) - 1.0) *
                          hard_penalty *
                          easy_bonus;

    const double next_s = safe_s * (1.0 + std::max(0.01, growth));
    return std::clamp(next_s, 0.1, params_.maximum_interval_days);
}

double FsrsEngine::next_forget_stability(double d, double s, double r) const noexcept {
    const double safe_s = std::max(0.1, s);
    const double safe_d = std::clamp(d, 1.0, 10.0);
    const double safe_r = std::clamp(r, 0.01, 1.0);

    const double s_forget = params_.w[11] *
                            std::pow(safe_d, -params_.w[12]) *
                            (std::pow(safe_s + 1.0, params_.w[13]) - 1.0) *
                            std::exp(params_.w[14] * (1.0 - safe_r));

    return std::clamp(std::min(safe_s, s_forget), 0.1, safe_s);
}

double FsrsEngine::evaluate_batch_soa(ModalityStatesSoA& batch) const noexcept {
    const auto t0 = std::chrono::steady_clock::now();
    const size_t n = batch.size();
    batch.out_retrievabilities.resize(n);
    batch.out_next_good_stabilities.resize(n);

    const double* __restrict t_ptr = batch.elapsed_days.data();
    const double* __restrict s_ptr = batch.stabilities.data();
    const double* __restrict d_ptr = batch.difficulties.data();
    double* __restrict r_out = batch.out_retrievabilities.data();
    double* __restrict s_out = batch.out_next_good_stabilities.data();

    const double exp_w8 = std::exp(params_.w[8]);
    const double w9 = params_.w[9];
    const double w10 = params_.w[10];

    for (size_t i = 0; i < n; ++i) {
        const double s = std::max(0.1, s_ptr[i]);
        const double t = std::max(0.0, t_ptr[i]);
        const double d = std::clamp(d_ptr[i], 1.0, 10.0);
        const double r = 1.0 / std::sqrt(1.0 + kDecayFactorF * (t / s));
        r_out[i] = r;
        const double growth = exp_w8 * (11.0 - d) * std::pow(s, -w9) * (std::exp(w10 * (1.0 - r)) - 1.0);
        s_out[i] = s * (1.0 + std::max(0.01, growth));
    }

    const auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::micro>(t1 - t0).count();
}

FsrsScheduleResult FsrsEngine::step_review(
    NodeId node_id,
    Modality modality,
    const FsrsMemoryState& current,
    Rating rating,
    TimestampSec review_timestamp,
    int32_t response_time_ms,
    const std::string& context_tag
) const {
    FsrsScheduleResult result{};
    FsrsMemoryState next = current;

    double elapsed_days = 0.0;
    if (current.is_initialized() && current.last_review_ts > 0 && review_timestamp > current.last_review_ts) {
        elapsed_days = static_cast<double>(review_timestamp - current.last_review_ts) / 86400.0;
    }

    const double pre_s = current.stability;
    const double pre_d = current.difficulty;
    double pre_r = 1.0;

    if (!current.is_initialized()) {
        next.stability = initial_stability(rating);
        next.difficulty = initial_difficulty(rating);
        next.retrievability = 1.0;
        next.repetitions = 1;
        next.lapses = (rating == Rating::Again) ? 1 : 0;
    } else {
        pre_r = compute_retrievability(elapsed_days, current.stability);
        next.difficulty = next_difficulty(current.difficulty, rating);
        if (rating == Rating::Again) {
            next.stability = next_forget_stability(current.difficulty, current.stability, pre_r);
            next.lapses = current.lapses + 1;
        } else {
            if (elapsed_days < 0.05) {
                const double g = static_cast<double>(rating);
                const double s_short = current.stability * std::exp(params_.w[17] * (g - 3.0 + params_.w[18]));
                next.stability = std::clamp(s_short, 0.1, params_.maximum_interval_days);
            } else {
                next.stability = next_recall_stability(current.difficulty, current.stability, pre_r, rating);
            }
        }
        next.retrievability = 1.0;
        next.repetitions = current.repetitions + 1;
    }

    next.last_review_ts = review_timestamp;
    if (current.avg_latency_ms <= 0) {
        next.avg_latency_ms = response_time_ms;
    } else {
        next.avg_latency_ms = static_cast<int32_t>(0.7 * current.avg_latency_ms + 0.3 * response_time_ms);
    }

    result.next_state = next;
    result.elapsed_days = elapsed_days;
    result.pre_review_retrievability = pre_r;
    result.scheduled_interval_days = compute_next_interval(next.stability);

    std::ostringstream uuid_ss;
    uuid_ss << "ev-" << node_id << "-" << static_cast<int>(modality) << "-" << review_timestamp << "-" << next.repetitions;

    result.log_entry = RevlogEntry{
        0,
        uuid_ss.str(),
        review_timestamp,
        node_id,
        modality,
        rating,
        response_time_ms,
        elapsed_days,
        result.scheduled_interval_days,
        pre_s,
        next.stability,
        pre_d,
        next.difficulty,
        context_tag
    };

    return result;
}

StabilityTier FsrsEngine::classify_stability_tier(double stability) noexcept {
    if (stability < 1.0) return StabilityTier::Tier0_InitialEncoding;
    if (stability < 7.0) return StabilityTier::Tier1_FragileDeclarative;
    if (stability < 30.0) return StabilityTier::Tier2_Consolidating;
    return StabilityTier::Tier3_Automatizing;
}

FamiliarityState FsrsEngine::classify_familiarity(const FsrsMemoryState& rec_text_state) noexcept {
    if (!rec_text_state.is_initialized()) return FamiliarityState::Unseen;
    const double s = rec_text_state.stability;
    if (s < 1.0)  return FamiliarityState::Noticed;
    if (s < 7.0)  return FamiliarityState::Fragile;
    if (s < 30.0) return FamiliarityState::Consolidating;
    return FamiliarityState::Automatized;
}

std::vector<std::pair<double, double>> FsrsEngine::sample_forgetting_curve(
    double stability,
    double max_days,
    int num_samples
) const {
    std::vector<std::pair<double, double>> points;
    const int n = std::max(2, num_samples);
    points.reserve(static_cast<size_t>(n));
    const double safe_max = std::max(1.0, max_days);
    for (int i = 0; i < n; ++i) {
        const double t = safe_max * (static_cast<double>(i) / static_cast<double>(n - 1));
        const double r = compute_retrievability(t, stability);
        points.emplace_back(t, r);
    }
    return points;
}

void FsrsEngine::set_desired_retention(double retention) noexcept {
    params_.desired_retention = std::clamp(retention, 0.75, 0.97);
}

} // namespace kinetix::domain
