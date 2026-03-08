#include <random>
#include <atomic>
#include <unordered_map>
#include <thread>
#include <mutex>

#include "balanced_openings.h"

std::mt19937 rng(std::random_device{}());
std::uniform_int_distribution<int> twin_dist(0, 1);
std::uniform_int_distribution<size_t> opening_dist(0, std::size(openings)-1);

constexpr double time_limit_per_move = 0.04f; 

#include "blunderfish.h"

float perturb_amount(float param) {
    return std::max(std::abs(param) * 0.05f, 0.01f);
}

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
    float asp_initial_window_size = 30.0f;
    float asp_window_growth_factor = 2.0f;
    float nmr_r_base = 3.0f;
    float nmr_r_divisor = 6.0f;
    float lmp_index_base = 3.0f;
    float lmp_index_factor = 2.0f;

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
        CLAMP(asp_initial_window_size, 5.0f, 10000.0f);
        CLAMP(asp_window_growth_factor, 1.1f, 100.0f);
        CLAMP(nmr_r_base, 1.0f, 6.0f);
        CLAMP(nmr_r_divisor, 1.0f, 12.0f);
        CLAMP(lmp_index_base, 1.0f, 5.0f);
        CLAMP(lmp_index_factor, 0.5f, 5.0f);
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
        PRINT_PARAM(asp_initial_window_size);
        PRINT_PARAM(asp_window_growth_factor);
        PRINT_PARAM(nmr_r_base);
        PRINT_PARAM(nmr_r_divisor);
        PRINT_PARAM(lmp_index_base);
        PRINT_PARAM(lmp_index_factor);
        print("\n");
        #undef PRINT_PARAM
    }

    SearchParameters convert() const {
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
            .asp_initial_window_size = int(asp_initial_window_size),
            .asp_window_growth_factor = asp_window_growth_factor,
            .nmr_r_base = nmr_r_base,
            .nmr_r_divisor = nmr_r_divisor,
            .lmp_index_base = lmp_index_base,
            .lmp_index_factor = lmp_index_factor,
        };
    }

    std::pair<Params, Params> perturb() {
        Params twins[2] = {
            *this, *this
        };

        #define perturb_param(param) do { \
            int i = twin_dist(rng); \
            float amount = perturb_amount(param); \
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
        perturb_param(asp_initial_window_size);
        perturb_param(asp_window_growth_factor);
        perturb_param(nmr_r_base);
        perturb_param(nmr_r_divisor);
        perturb_param(lmp_index_base);
        perturb_param(lmp_index_factor);

        twins[0].clamp();
        twins[1].clamp();

        return {
            twins[0], twins[1]
        };
    }
};

std::mutex print_mutex;

static int run_double_sided_game(size_t game_index, const char* opening, const Params& p1, const Params& p2) {
    SearchParameters sides[2] = {
        p1.convert(),
        p2.convert()
    };

    int aggregate = 0;

    int results[2];

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

        results[round] = *result;

        if (round == 0) {
            aggregate += float(*result);
        }
        else {
            aggregate -= float(*result);
        }

        std::swap(sides[0], sides[1]);
    }

    {
        std::lock_guard<std::mutex> lock(print_mutex);
        print("Game {}: ({} {})\n", game_index, results[0], results[1]);
    }

    return aggregate;
}

int main() {
    Params params{};

    std::vector<const char*> games;

    int nthreads = std::max((unsigned int)1, std::thread::hardware_concurrency());

    for (int iteration = 0; iteration < 1000; ++iteration) {
        if (iteration % 16 == 0)
        {
            games.clear();

            for (int i = 0; i < 16; ++i) {
                const char* opening = openings[opening_dist(rng)];
                games.push_back(opening);
            }
        }

        float ak = 0.5f / std::pow(iteration + 10, 0.6f);

        auto [p1, p2] = params.perturb();

        std::vector<std::thread> threads;
        threads.reserve(nthreads);

        std::atomic<size_t> opening_index{0};
        std::vector<int> thread_results(nthreads, 0);

        for (int t = 0; t < nthreads; ++t) {
            threads.emplace_back([&, t](){
                int local_sum = 0;

                while (true) {
                    size_t i = opening_index.fetch_add(1);

                    if (i >= games.size()) {
                        break;
                    }

                    local_sum += run_double_sided_game(i, games[i], p1, p2);
                }

                thread_results[t] = local_sum;
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        int result_aggregate = 0;

        for (auto x : thread_results) {
            result_aggregate += x;
        }

        float avg_result = float(result_aggregate) / float(games.size()*2);

        #define UPDATE(param) do { \
            float diff = p1.param - p2.param; \
            if (std::abs(diff) > 1e-9f) { \
                float grad = avg_result / diff; \
                params.param += ak * grad; \
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
        UPDATE(asp_initial_window_size);
        UPDATE(asp_window_growth_factor);
        UPDATE(nmr_r_base);
        UPDATE(nmr_r_divisor);
        UPDATE(lmp_index_base);
        UPDATE(lmp_index_factor);

        params.clamp();

        #undef UPDATE

        print("Iteration: {}\n  ak: {:.4f}\n  result: {}\n", iteration, ak, avg_result);
        params.dump();
    }
}