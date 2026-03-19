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

struct Record {
    uint64_t bbs[12];
    int32_t score;
};

static int run_match(FILE* file) {
    Position pos = *Position::decode_fen_string(START_FEN);

    std::array<Move, 256> move_buf;

    std::vector<Record> records;

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
            Record r;

            Piece pieces[6] = {
                PIECE_PAWN,
                PIECE_KNIGHT,
                PIECE_BISHOP,
                PIECE_ROOK,
                PIECE_QUEEN,
                PIECE_KING
            };

            for (int side = 0; side < 2; ++side) {
                for (int pid = 0; pid < 6; ++pid) {
                    r.bbs[side*6+pid] = pos.sides[side].bb[pieces[pid]];
                }
            }

            std::atomic<bool> should_stop = false;

            int64_t score = 0;
            Move mv = pos.best_move_easy(40, should_stop, TIME_LIMIT_PER_MOVE, std::nullopt, false, &score);

            if (pos.to_move == BLACK) {
                score *= -1;
            }

            r.score = int32_t(score);

            if (std::abs(score) < (MATE_SCORE - 1000)) { // filter out mate scores
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
            fwrite(&r.bbs, sizeof(r.bbs), 1, file);
            fwrite(&r.score, sizeof(r.score), 1, file);
            fwrite(&outcome, sizeof(outcome), 1, file);
        }
    }

    return result;
}

int main() {
    std::string filename = std::format("data_{:%Y%m%d_%H%M%S}.bin", std::chrono::system_clock::now());
    FILE* file = fopen(filename.c_str(), "wb");

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