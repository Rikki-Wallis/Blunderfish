#include <random>
#include <cstdio>
#include <thread>
#include <mutex>

#include "blunderfish.h"

static thread_local std::mt19937 rng(std::random_device{}());

constexpr int NUM_MATCHES = 1000;
constexpr int RANDOM_HALF_MOVES = 10;

constexpr double TIME_LIMIT_PER_MOVE = 0.1;

static const char* START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

static std::mutex io_mutex;

template<typename...Args>
static int fprint(FILE* stream, const std::format_string<Args...>& fmt, Args&&...args) {
    std::string res = std::format(fmt, std::forward<Args>(args)...);
    return fprintf(stream, "%s", res.c_str());
}

static int run_match(FILE* file) {
    Position pos = *Position::decode_fen_string(START_FEN);

    std::array<Move, 256> move_buf;

    std::vector<std::pair<std::string, int64_t>> positions;

    int result = 67;

    for (int hm = 0;;++hm) {
        std::optional<int> gr = pos.game_result();

        if (gr) {
            result = *gr;
            break;
        }

        if (hm == 200) {
            result = 0; // draw
            break;
        }
        
        if (hm < RANDOM_HALF_MOVES) { // For the first n moves, play random moves, to diversify the position
            std::span<Move> moves = pos.generate_moves(move_buf);
            pos.filter_moves(moves);

            size_t move_idx = std::uniform_int_distribution<size_t>(0, moves.size() - 1)(rng);
            Move mv = moves[move_idx];

            pos.make_move(mv);
        }
        else { // after that, we let the engine make the move
            std::string fen = pos.fen();

            std::atomic<bool> should_stop = false;

            int64_t score = 0;
            Move mv = pos.best_move_easy(40, should_stop, TIME_LIMIT_PER_MOVE, std::nullopt, false, &score);

            if (pos.to_move == BLACK) {
                score *= -1;
            }

            if (std::abs(score) < (MATE_SCORE - 1000)) { // filter out mate scores
                positions.push_back({fen, score});
            }

            pos.make_move(mv);
        }
    }

    assert(result != 67);

    float outcome = float(result) * 0.5f + 0.5f;

    {
        std::lock_guard g(io_mutex);
        for (auto& [fen, score] : positions) {
            fprint(file, "\"{}\" {} {:.1f}\n", fen, score, outcome);
        }
    }

    return result;
}

int main() {
    std::string filename = std::format("data_{:%Y%m%d_%H%M%S}.txt", std::chrono::system_clock::now());
    FILE* file = fopen(filename.c_str(), "w");

    std::atomic<int> match_count = 0;

    int nthreads = std::thread::hardware_concurrency();

    std::vector<std::thread> threads;

    for (int t = 0; t < nthreads; ++t) {
        threads.push_back(std::thread([&match_count, file](){
            for (;;) {
                int mid = match_count.fetch_add(1);

                if (mid >= NUM_MATCHES) {
                    break;
                }

                int result = run_match(file);

                {
                    std::lock_guard guard(io_mutex);
                    print("Game {}/{} result: {}\n", mid+1, NUM_MATCHES, result);
                }
            }
        }));
    }

    for (auto& t : threads) {
        t.join();
    }

    fclose(file);

    return 0;
}