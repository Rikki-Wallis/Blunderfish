#include <random>
#include <unordered_map>

#include "openings.h"

std::mt19937 rng(std::random_device{}());
std::uniform_int_distribution<int> twin_dist(0, 1);
std::uniform_int_distribution<size_t> opening_dist(0, std::size(openings)-1);

#include "blunderfish.h"

static const std::unordered_map<std::string, float> perturbations = {
    {"lmr_rate_divisor", 0.05f},
    {"singular_margin_factor", 0.1f},
    {"rfp_margin_factor", 30.0f},
    {"rfp_improving_bonus", 30.0f},
    {"fp_margin_factor", 30.0f},
    {"lmr_history_bonus_threshold", 100.0f},
    {"history_bonus_factor", 0.05f},
    {"history_malus_factor", 0.05f},
    {"qsearch_big_delta", 100.0f},
    {"qsearch_delta_margin", 30.0f},
};

// These are continuously updated but rounded to integers in most cases
struct Params {
    float lmr_rate_divisor = 1.35f;
    float singular_margin_factor = 2.0f;
    float rfp_margin_factor = 120.0f;
    float rfp_improving_bonus = 60.0f;
    float fp_margin_factor = 200.0f;
    float lmr_history_bonus_threshold = 1000.0f;
    float history_bonus_factor = 1.0f;
    float history_malus_factor = 1.0f;
    float qsearch_big_delta = 1100.0f;
    float qsearch_delta_margin = 200.0f;

    void clamp() {
        #define CLAMP(x, lo, hi) x = std::clamp(x, lo, hi)
        CLAMP(lmr_rate_divisor, 0.5f, 5.0f);
        CLAMP(singular_margin_factor, 0.5f, 5.0f);
        CLAMP(rfp_margin_factor, 10.0f, 1000.0f);
        CLAMP(rfp_improving_bonus, 0.0f, 1000.0f);
        CLAMP(fp_margin_factor, 10.0f, 1000.0f);
        CLAMP(lmr_history_bonus_threshold, 100.0f, 5000.0f);
        CLAMP(history_bonus_factor, 0.1f, 5.0f);
        CLAMP(history_malus_factor, 0.1f, 5.0f);
        CLAMP(qsearch_big_delta, 400.0f, 2000.0f);
        CLAMP(qsearch_delta_margin, 50.0f, 1000.0f);
        #undef CLAMP
    }

    void dump() {
        print("Params:\n");
        #define PRINT_PARAM(name) print("  {}: {:.4f}\n", #name, name)
        PRINT_PARAM(lmr_rate_divisor);
        PRINT_PARAM(singular_margin_factor);
        PRINT_PARAM(rfp_margin_factor);
        PRINT_PARAM(rfp_improving_bonus);
        PRINT_PARAM(fp_margin_factor);
        PRINT_PARAM(lmr_history_bonus_threshold);
        PRINT_PARAM(history_bonus_factor);
        PRINT_PARAM(history_malus_factor);
        PRINT_PARAM(qsearch_big_delta);
        PRINT_PARAM(qsearch_delta_margin);
        print("\n");
        #undef PRINT_PARAM
    }

    SearchParameters convert() {
        return SearchParameters {
            .lmr_rate_divisor = lmr_rate_divisor,
            .singular_margin_factor = singular_margin_factor,
            .rfp_margin_factor = int(rfp_margin_factor),
            .rfp_improving_bonus = int(rfp_improving_bonus),
            .fp_margin_factor = int(fp_margin_factor),
            .lmr_history_bonus_threshold = int(lmr_history_bonus_threshold),
            .history_bonus_factor = history_bonus_factor,
            .history_malus_factor = history_malus_factor,
            .qsearch_big_delta = int(qsearch_big_delta),
            .qsearch_delta_margin = int(qsearch_delta_margin),
        };
    }

    std::pair<Params, Params> perturb() {
        Params twins[2] = {
            *this, *this
        };

        #define perturb_param(param) do { \
            int i = twin_dist(rng); \
            float amount = perturbations.at(#param); \
            twins[i].param += amount;\
            twins[(i+1)&1].param -= amount; \
        } while(false)

        perturb_param(lmr_rate_divisor);
        perturb_param(singular_margin_factor);
        perturb_param(rfp_margin_factor);
        perturb_param(rfp_improving_bonus);
        perturb_param(fp_margin_factor);
        perturb_param(lmr_history_bonus_threshold);
        perturb_param(history_bonus_factor);
        perturb_param(history_malus_factor);
        perturb_param(qsearch_big_delta);
        perturb_param(qsearch_delta_margin);

        twins[0].clamp();
        twins[1].clamp();

        return {
            twins[0], twins[1]
        };
    }
};


int main() {
    Params params{};

    double time_limit_per_move = 0.05f; 

    for (int iteration = 0; iteration < 1000; ++iteration) {
        float ak = 0.5f / std::pow(iteration + 10, 0.6f);

        auto [p1, p2] = params.perturb();

        const char* opening = openings[opening_dist(rng)];

        print("Playing opening '{}'\n", opening);

        SearchParameters sides[2] = {
            p1.convert(),
            p2.convert()
        };

        int result_aggregate = 0;

        for (int round = 0; round < 2; ++round) { // play two rounds, switching sides with the opening
            Position pos = *Position::decode_fen_string(opening);

            std::optional<int> result;

            for (int hm = 0; hm < 200; ++hm) {
                result = pos.game_result();

                if (result.has_value()) { // game over
                    break;
                }

                Move move = pos.best_move_easy(20, time_limit_per_move, sides[pos.to_move]);
                pos.make_move(move);
            }

            if (!result) {
                result = 0;
            }

            print("Round {} result: {}\n", round+1, *result);

            if (round == 0) {
                result_aggregate += *result;
            }
            else {
                result_aggregate -= *result;
            }

            std::swap(sides[0], sides[1]);
        }

        #define UPDATE(param) do { \
            float diff = p1.param - p2.param; \
            if (std::abs(diff) > 1e-6f) { \
                float gradient = float(result_aggregate) / diff; \
                float pert = perturbations.at(#param); \
                params.param += ak * pert * gradient; \
            } \
        } while(false)

        UPDATE(lmr_rate_divisor);
        UPDATE(singular_margin_factor);
        UPDATE(rfp_margin_factor);
        UPDATE(rfp_improving_bonus);
        UPDATE(fp_margin_factor);
        UPDATE(lmr_history_bonus_threshold);
        UPDATE(history_bonus_factor);
        UPDATE(history_malus_factor);
        UPDATE(qsearch_big_delta);
        UPDATE(qsearch_delta_margin);

        params.clamp();

        #undef UPDATE

        print("Iteration: {}\n  ak: {:.4f}\n  result: {}\n", iteration, ak, result_aggregate);
        params.dump();
    }
}