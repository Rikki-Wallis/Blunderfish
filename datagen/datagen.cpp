#include <random>
#include <cstdio>
#include <thread>
#include <mutex>

#include "blunderfish.h"

static thread_local std::mt19937 rng(std::random_device{}());

constexpr int NUM_ITERATIONS = 100;
constexpr int NUM_MATCHES = 10000;
constexpr int RANDOM_HALF_MOVES = 10;
constexpr int SEARCH_DEPTH = 6;

static const char* START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

static std::mutex io_mutex;

struct Record {
    std::array<uint64_t, 12> bbs;
    int32_t score;
};

static int run_match(FILE* file) {
    Position pos = *Position::parse_fen(START_FEN);

    std::vector<Record> records;

    int result = 67;

    for (int hm = 0;;++hm) {
        std::optional<int> gr = pos.game_result();

        if (gr) {
            result = *gr;
            break;
        }

        if (hm < RANDOM_HALF_MOVES) { // For the first n moves, play random moves, to diversify the position
            MoveList moves = pos.generate_moves();
            pos.filter_moves(moves);

            size_t move_idx = std::uniform_int_distribution<size_t>(0, moves.count - 1)(rng);
            Move mv = moves.data[move_idx];

            pos.make_move(mv);
        }
        else { // after that, we let the engine make the move

            std::atomic<bool> should_stop = false;

            int64_t score = 0;
            Move mv = pos.best_move(SEARCH_DEPTH, should_stop, std::nullopt, std::nullopt, false, &score);

            if (pos.to_move == BLACK) {
                score *= -1;
            }

            if (pos.is_quiescent() && std::abs(score) < (MATE_SCORE - 1000)) { // filter out mate scores
                Record r;
                r.score = int32_t(score);
                r.bbs = pos.to_bitboards();
                records.push_back(r);
            }

            pos.make_move(mv);
        }
    }

    assert(result != 67);

    int32_t outcome = int32_t(result);

    {
        std::lock_guard g(io_mutex);
        for (Record& r : records) {
            fwrite(r.bbs.data(), r.bbs.size() * sizeof(uint64_t), 1, file);
            fwrite(&r.score, sizeof(r.score), 1, file);
            fwrite(&outcome, sizeof(outcome), 1, file);
        }
    }

    return result;
}

int main() {
    for (int iter = 0; iter < NUM_ITERATIONS; ++iter) {
        std::string filename = std::format("data_{:%Y%m%d_%H%M%S}.bin", std::chrono::system_clock::now());
        FILE* file = fopen(filename.c_str(), "wb");

        std::atomic<int> match_count = 0;

        int nthreads = std::thread::hardware_concurrency();

        std::vector<std::thread> threads;

        for (int t = 0; t < nthreads; ++t) {
            threads.push_back(std::thread([&match_count, file, iter](){
                for (;;) {
                    int mid = match_count.fetch_add(1);

                    if (mid >= NUM_MATCHES) {
                        break;
                    }

                    int result = run_match(file);

                    {
                        std::lock_guard guard(io_mutex);
                        print("Batch {}/{}: Game {}/{}: {}\n", iter+1, NUM_ITERATIONS, mid+1, NUM_MATCHES, result);
                    }
                }
            }));
        }

        for (auto& t : threads) {
            t.join();
        }

        fclose(file);
    }

    return 0;
}