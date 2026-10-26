#pragma once
// FSRS-4.5 / FSRS-5 Mathematical Memory Engine + Structure-of-Arrays (SoA) Batch Evaluator
// Implements Piotr Wozniak's Three-Component Model of Memory (DSR) as formalized in
// Jarrett Ye's Free Spaced Repetition Scheduler (FSRS-4.5/5), enhanced with cache-aligned
// Structure-of-Arrays (SoA) batch evaluation per `optimcpop.md`.

#include "kinetix/domain/types.hpp"
#include <array>
#include <utility>
#include <vector>

namespace kinetix::domain {

struct FsrsParameters {
    std::array<double, 19> w = {
        0.4872, 1.4003, 3.7145, 13.8206,
        5.1618,
        1.2298,
        0.8975,
        0.0310,
        1.6474,
        0.1367,
        1.0461,
        2.1072,
        0.0793,
        0.3246,
        1.5870,
        0.2272,
        2.8755,
        0.5425,
        0.0912
    };
    double desired_retention = 0.90;
    double maximum_interval_days = 3650.0;
};

struct FsrsScheduleResult {
    FsrsMemoryState next_state;
    double scheduled_interval_days = 0.0;
    double elapsed_days = 0.0;
    double pre_review_retrievability = 1.0;
    RevlogEntry log_entry;
};

// Cache-Line Aligned Structure-of-Arrays (SoA) Batch FSRS-4.5 Evaluator (`optimcpop.md`)
// Contiguous float/double vectors maximize CPU L1/L2 cache-line utilization and auto-vectorization.
struct alignas(64) ModalityStatesSoA {
    std::vector<NodeId> node_ids;
    std::vector<double> elapsed_days;
    std::vector<double> stabilities;
    std::vector<double> difficulties;
    std::vector<double> out_retrievabilities;
    std::vector<double> out_next_good_stabilities;

    void reserve(size_t n) {
        node_ids.reserve(n);
        elapsed_days.reserve(n);
        stabilities.reserve(n);
        difficulties.reserve(n);
        out_retrievabilities.resize(n, 0.0);
        out_next_good_stabilities.resize(n, 0.0);
    }

    size_t size() const noexcept { return stabilities.size(); }
};

class FsrsEngine {
public:
    static constexpr double kDecayFactorF = 19.0 / 81.0;
    static constexpr double kDecayPowerC = -0.5;

    explicit FsrsEngine(FsrsParameters params = FsrsParameters{});

    double compute_retrievability(double elapsed_days, double stability) const noexcept;
    double compute_next_interval(double stability, double target_retention = -1.0) const noexcept;
    double initial_stability(Rating rating) const noexcept;
    double initial_difficulty(Rating rating) const noexcept;
    double next_difficulty(double current_d, Rating rating) const noexcept;
    double next_recall_stability(double d, double s, double r, Rating rating) const noexcept;
    double next_forget_stability(double d, double s, double r) const noexcept;

    // Vectorized SoA Batch Evaluation (`optimcpop.md` pattern): returns execution duration in microseconds
    double evaluate_batch_soa(ModalityStatesSoA& batch) const noexcept;

    FsrsScheduleResult step_review(
        NodeId node_id,
        Modality modality,
        const FsrsMemoryState& current,
        Rating rating,
        TimestampSec review_timestamp,
        int32_t response_time_ms,
        const std::string& context_tag = "stage3_retrieval"
    ) const;

    static StabilityTier classify_stability_tier(double stability) noexcept;
    static FamiliarityState classify_familiarity(const FsrsMemoryState& rec_text_state) noexcept;

    std::vector<std::pair<double, double>> sample_forgetting_curve(
        double stability,
        double max_days,
        int num_samples = 60
    ) const;

    const FsrsParameters& parameters() const noexcept { return params_; }
    void set_desired_retention(double retention) noexcept;

private:
    FsrsParameters params_;
};

} // namespace kinetix::domain
