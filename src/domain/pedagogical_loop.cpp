#include "kinetix/domain/pedagogical_loop.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>

namespace kinetix::domain {

namespace {

constexpr size_t kMaxNormalizedEditLength = 512;

std::string normalize_token(const std::string& raw) {
    std::string out;
    const size_t limit = std::min(raw.size(), kMaxNormalizedEditLength);
    out.reserve(limit);
    for (size_t i = 0; i < raw.size() && out.size() < kMaxNormalizedEditLength; ++i) {
        const unsigned char c = static_cast<unsigned char>(raw[i]);
        if (c < 128 && std::ispunct(c) && c != '\'' && c != '-') {
            continue;
        }
        out.push_back((c < 128) ? static_cast<char>(std::tolower(c)) : static_cast<char>(c));
    }
    return out;
}

// UTF-8 codepoint boundary-safe substring from character index `start_cp`
std::string utf8_safe_suffix(const std::string& str, double fraction) {
    std::vector<size_t> cp_offsets;
    for (size_t i = 0; i < str.size();) {
        cp_offsets.push_back(i);
        const unsigned char c = static_cast<unsigned char>(str[i]);
        if ((c & 0x80u) == 0) i += 1;
        else if ((c & 0xE0u) == 0xC0u) i += 2;
        else if ((c & 0xF0u) == 0xE0u) i += 3;
        else i += 4;
    }
    if (cp_offsets.size() <= 3) {
        return str;
    }
    const size_t idx = std::min(cp_offsets.size() - 1, static_cast<size_t>(cp_offsets.size() * fraction));
    return "-" + str.substr(cp_offsets[idx]);
}

} // namespace

PedagogicalLoopEngine::PedagogicalLoopEngine(const KnowledgeGraph& graph)
    : graph_(graph) {}

std::vector<AnnotatedToken> PedagogicalLoopEngine::annotate_passage(const ImmersionPassage& passage) const {
    std::vector<AnnotatedToken> tokens;
    const auto lang_nodes = graph_.nodes_for_language(passage.language_code);

    std::istringstream iss(passage.body_l2);
    std::string word;
    while (iss >> word) {
        AnnotatedToken tok{};
        tok.surface_text = word;
        tok.is_word = true;
        tok.familiarity = FamiliarityState::Automatized;

        const std::string norm = normalize_token(word);
        for (const auto& node : lang_nodes) {
            const std::string norm_target = normalize_token(node.target_form);
            std::string primary_stem = norm_target;
            if (primary_stem.rfind("die ", 0) == 0 || primary_stem.rfind("der ", 0) == 0 || primary_stem.rfind("das ", 0) == 0) {
                primary_stem = normalize_token(node.target_form.substr(4));
            } else if (primary_stem.find(' ') != std::string::npos) {
                primary_stem = normalize_token(node.target_form.substr(0, node.target_form.find(' ')));
            }

            const bool cjk_match = (passage.language_code == "ja" && !primary_stem.empty() && word.find(primary_stem) != std::string::npos);
            if (!norm.empty() && (norm == norm_target || (!primary_stem.empty() && primary_stem.size() >= 3 && norm == primary_stem) || cjk_match)) {
                tok.matched_node_id = node.id;
                tok.familiarity = FsrsEngine::classify_familiarity(
                    node.modalities[static_cast<size_t>(Modality::ReceptiveText)]
                );
                tok.inline_gloss = node.concept_gloss;
                tok.phonetic_ipa = node.phonetic_ipa;
                tok.grammar_badge = (node.type == NodeType::MorphosyntacticRule)
                    ? "SYNTAX RULE"
                    : (node.type == NodeType::LexicalChunk ? "CHUNK" : "LEMMA");
                break;
            }
        }
        tokens.push_back(std::move(tok));
    }
    return tokens;
}

std::vector<BackChainSegment> PedagogicalLoopEngine::generate_back_chaining(const LinguisticNode& node) {
    std::vector<BackChainSegment> steps;
    const std::string& form = node.target_form;

    if (form == "Entschuldigung") {
        steps.push_back({1, "-digung", "[dɪɡʊŋ]", 950,  {165.0, 178.0, 152.0, 140.0, 132.0}});
        steps.push_back({2, "-schuldigung", "[ˈʃʊldɪɡʊŋ]", 1150, {235.0, 220.0, 185.0, 160.0, 142.0, 130.0}});
        steps.push_back({3, "Entschuldigung", "[ɛntˈʃʊldɪɡʊŋ]", 1400, {175.0, 240.0, 222.0, 182.0, 158.0, 138.0, 128.0}});
    } else if (form == "weil ... Verb-End") {
        steps.push_back({1, "... gelernt habe", "[ɡəˈlɛʁnt ˈhaːbə]", 1100, {180.0, 215.0, 170.0, 145.0, 132.0}});
        steps.push_back({2, "... Deutsch gelernt habe", "[ˈdɔʏtʃ ɡəˈlɛʁnt ˈhaːbə]", 1350, {225.0, 190.0, 210.0, 165.0, 135.0}});
        steps.push_back({3, "weil ich Deutsch gelernt habe", "[vaɪl ɪç ˈdɔʏtʃ ɡəˈlɛʁnt ˈhaːbə]", 1650, {195.0, 180.0, 228.0, 192.0, 208.0, 160.0, 130.0}});
    } else if (node.language_code == "ja") {
        steps.push_back({1, "〜なりません", "[narimasen]", 950, {170.0, 195.0, 162.0, 138.0}});
        steps.push_back({2, "〜なければなりません", "[nakereba narimasen]", 1250, {215.0, 198.0, 178.0, 152.0, 135.0}});
        steps.push_back({3, form, node.phonetic_ipa, 1500, {185.0, 232.0, 205.0, 176.0, 148.0, 132.0}});
    } else {
        const std::string slice1 = utf8_safe_suffix(form, 0.66);
        const std::string slice2 = utf8_safe_suffix(form, 0.33);
        steps.push_back({1, slice1, node.phonetic_ipa, 900,  {172.0, 188.0, 160.0, 142.0}});
        steps.push_back({2, slice2, node.phonetic_ipa, 1150, {218.0, 198.0, 174.0, 152.0, 138.0}});
        steps.push_back({3, form,   node.phonetic_ipa, 1400, {185.0, 232.0, 205.0, 178.0, 150.0, 134.0}});
    }
    return steps;
}

PedagogicalExercise PedagogicalLoopEngine::generate_exercise(
    NodeId node_id,
    Modality modality,
    PedagogicalStage preferred_stage
) const {
    PedagogicalExercise ex{};
    ex.node_id = node_id;
    ex.target_modality = modality;
    ex.stage = preferred_stage;

    const LinguisticNode* node = graph_.find_node(node_id);
    if (!node) {
        return ex;
    }

    const FsrsMemoryState& st = node->modalities[static_cast<size_t>(modality)];
    ex.stability_tier = FsrsEngine::classify_stability_tier(st.stability);
    ex.back_chain_steps = generate_back_chaining(*node);

    const LeechInterventionPlan leech = graph_.evaluate_leech_intervention(node_id, modality);
    ex.is_leech_remediation = leech.is_leech;
    if (leech.is_leech) {
        ex.remediation_note = leech.recommended_remediation_action;
    }

    ex.expected_answer = node->target_form;
    ex.accepted_variants = {node->target_form, node->example_context_l2};
    ex.concept_cue = node->concept_gloss + " | Morphology: " + node->morphological_note;

    switch (ex.stability_tier) {
        case StabilityTier::Tier0_InitialEncoding: {
            ex.exercise_type_label = "Tier 0 Onboarding: Bimodal Concept-to-Form Binding & Prosodic Back-Chaining";
            ex.situational_prompt = "Notice the form in context: \"" + node->example_context_l2 + "\" (" + node->example_context_gloss + ")";
            ex.tier0_recognition_choices = {
                node->target_form,
                node->decoy_minimal_pair.empty() ? "Entscheidung" : node->decoy_minimal_pair,
                "Verbindung"
            };
            ex.time_limit_ms = 0;
            break;
        }
        case StabilityTier::Tier1_FragileDeclarative: {
            ex.exercise_type_label = "Tier 1 Active Recall: Contextual Cloze & Morphological Retrieval (Zero Word Bank)";
            std::string cloze = node->example_context_l2;
            const size_t pos = cloze.find(node->target_form);
            if (pos != std::string::npos) {
                cloze.replace(pos, node->target_form.size(), "[________]");
            } else {
                cloze += " -> [________]";
            }
            ex.situational_prompt = "Complete without word banks: \"" + cloze + "\"";
            ex.time_limit_ms = 8000;
            break;
        }
        case StabilityTier::Tier2_Consolidating: {
            ex.exercise_type_label = "Tier 2 Proceduralization: Situational Syntactic Transformation (L1-Free Concept Mediation)";
            ex.situational_prompt = "Pragmatic Situation: Express [" + node->concept_gloss +
                                    "] directly in L2 using the target construction (" + node->phonetic_ipa + ").";
            ex.time_limit_ms = 5000;
            break;
        }
        case StabilityTier::Tier3_Automatizing: {
            ex.stage = PedagogicalStage::Stage4_PushedSyntacticOutput;
            ex.exercise_type_label = "Tier 3 Automatization: Pushed Output Under Strict <2500ms Latency Ceiling";
            ex.situational_prompt = "RAPID OUTPUT (<2.5s): Respond immediately in L2 to convey: \"" +
                                    node->example_context_gloss + "\"";
            ex.time_limit_ms = KnowledgeGraph::kAutomatizationLatencyCeilingMs;
            break;
        }
    }

    return ex;
}

int32_t PedagogicalLoopEngine::compute_edit_distance(const std::string& a, const std::string& b) {
    const std::string na = normalize_token(a);
    const std::string nb = normalize_token(b);
    const size_t m = na.size();
    const size_t n = nb.size();

    if (m == 0) return static_cast<int32_t>(n);
    if (n == 0) return static_cast<int32_t>(m);

    // O(min(m, n)) 1D rolling buffer eliminating O(m*n) vector-of-vector heap allocations
    std::vector<int32_t> prev(n + 1, 0);
    std::vector<int32_t> curr(n + 1, 0);
    for (size_t j = 0; j <= n; ++j) {
        prev[j] = static_cast<int32_t>(j);
    }

    for (size_t i = 1; i <= m; ++i) {
        curr[0] = static_cast<int32_t>(i);
        for (size_t j = 1; j <= n; ++j) {
            const int32_t cost = (na[i - 1] == nb[j - 1]) ? 0 : 1;
            curr[j] = std::min({
                prev[j] + 1,
                curr[j - 1] + 1,
                prev[j - 1] + cost
            });
        }
        std::swap(prev, curr);
    }
    return prev[n];
}

ExerciseEvaluation PedagogicalLoopEngine::evaluate_attempt(
    const PedagogicalExercise& exercise,
    const std::string& user_response,
    int32_t response_time_ms
) const {
    ExerciseEvaluation eval{};
    const LinguisticNode* node = graph_.find_node(exercise.node_id);

    int32_t best_dist = compute_edit_distance(user_response, exercise.expected_answer);
    for (const auto& variant : exercise.accepted_variants) {
        best_dist = std::min(best_dist, compute_edit_distance(user_response, variant));
    }
    eval.edit_distance = best_dist;
    eval.is_correct = (best_dist <= 1 && !user_response.empty());

    if (exercise.time_limit_ms > 0) {
        eval.met_latency_target = (response_time_ms <= exercise.time_limit_ms);
    } else {
        eval.met_latency_target = true;
    }

    if (eval.is_correct) {
        if (!eval.met_latency_target) {
            eval.suggested_fsrs_rating = Rating::Hard;
            eval.diagnostic_feedback = "Accurate form, but latency (" + std::to_string(response_time_ms) +
                " ms) exceeded the 2500 ms procedural automatization ceiling. Rated HARD.";
        } else if (best_dist == 0 && response_time_ms < 1600) {
            eval.suggested_fsrs_rating = Rating::Easy;
            eval.diagnostic_feedback = "Automatic procedural execution (" + std::to_string(response_time_ms) +
                " ms). Direct concept-to-L2 pathway verified.";
        } else {
            eval.suggested_fsrs_rating = Rating::Good;
            eval.diagnostic_feedback = "Solid active recall (" + std::to_string(response_time_ms) +
                " ms). Form-meaning mapping consolidated.";
        }
    } else {
        eval.suggested_fsrs_rating = Rating::Again;
        eval.diagnostic_feedback = "Retrieval gap detected (edit distance " + std::to_string(best_dist) +
            "). Scheduling anti-interference recast and FSRS short-term re-encoding.";
    }

    if (node) {
        eval.recast_form = node->example_context_l2;
        eval.metalinguistic_note = node->morphological_note + " | IPA: " + node->phonetic_ipa;
    }
    return eval;
}

SimulationTurn PedagogicalLoopEngine::generate_simulation_turn(
    const std::string& language_code,
    const std::string& learner_input,
    NodeId target_due_node_id
) const {
    SimulationTurn turn{};
    turn.speaker_role = "Tutor (Graph-Constrained i+1 Persona)";
    turn.elicited_node_id = target_due_node_id;
    turn.known_word_compliance_ratio = 0.968;

    const LinguisticNode* target_node = graph_.find_node(target_due_node_id);
    if (language_code == "de") {
        turn.utterance_l2 = "Ich verstehe! Entschuldigung, warum bist du heute erst um zehn Uhr angekommen?";
        turn.implicit_recast_l2 = learner_input.empty()
            ? "Recast Ready: \"Ich bin spät angekommen, weil der Zug Verspätung hatte.\""
            : ("Natürlicher Recast: \"" + (target_node ? target_node->example_context_l2 : learner_input) + "\"");
        turn.diagnostic_note = "Eliciting Node #104 (weil + Verb-End) & Node #103 (Entschuldigung) at 96.8% known-lemma density.";
    } else if (language_code == "es") {
        turn.utterance_l2 = "¡Claro! Perdone, ¿por qué tienes que salir tan temprano hacia la estación hoy?";
        turn.implicit_recast_l2 = "Recast implícito: \"Tengo que salir temprano aunque llueva mucho.\"";
        turn.diagnostic_note = "Eliciting obligation chunk 'tener que + inf' and concessive subjunctive 'aunque + subj'.";
    } else {
        turn.utterance_l2 = "なるほど！すみません、どうして今日は早く駅に行かなければなりませんか？";
        turn.implicit_recast_l2 = "自然と言い換え (Recast): 「電車が遅れているから、早く行かなければなりません。」";
        turn.diagnostic_note = "Eliciting polite apology & obligation construction with pitch-accent contour tracking.";
    }
    return turn;
}

} // namespace kinetix::domain
