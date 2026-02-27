#include <iostream>
#include <chrono>

#include "blunderfish.h"

template <typename Func>
static void run_and_report(int runs, const std::string& title, Func&& func) {
    double sum = 0.0;

    for (int i = 0; i < runs; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        std::forward<Func>(func)(); 
        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

        double ms = (double)duration.count()/1000000.0;
        sum += ms;
    }

    double avg = sum/(double)runs;

    print("{}: {}ms\n", title, avg);
}

/*
template <typename Func>
static uint64_t run_and_time(Func&& func) {
    // Time perft search
    auto start = std::chrono::high_resolution_clock::now();
    std::forward<Func>(func)(); 
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    uint64_t ms = duration.count();

    return ms;
}

static void benchmark_perft() {
    Position pos = *Position::decode_fen_string("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    std::vector<uint64_t> times;

    times.push_back(run_and_time([&] {return perft_search(1, pos);}));
    times.push_back(run_and_time([&] {return perft_search(2, pos);}));
    times.push_back(run_and_time([&] {return perft_search(3, pos);}));
    times.push_back(run_and_time([&] {return perft_search(4, pos);}));
    times.push_back(run_and_time([&] {return perft_search(5, pos);}));
    times.push_back(run_and_time([&] {return perft_search(6, pos);}));

    size_t i = 0;
    std::vector<uint64_t> nodes = {20, 400, 8902, 197281, 4865609, 119060324};
    for (i = 0; i < times.size(); ++i) {
                    
        double seconds = times[i] / 1000.0;
        double nps = seconds > 0 ? nodes[i] / seconds : 0;
        std::cout << "Perft Depth " << i+1 << ": " << nodes[i] << " nodes in "
                    << seconds << "s (" << nps << " nodes/sec)\n";
        
    };
}
*/

template<typename Func>
static void benchmark_pos_method(const std::string& name, int max_depth, Func&& func) {
    for (int depth = 1; depth <= max_depth; ++depth) {
        run_and_report(1, std::format("{} Depth {}", name, depth), [&](){
            Position pos = *Position::decode_fen_string("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
            std::array<Move, 256> move_buf;
            std::span<Move> moves = pos.generate_moves(move_buf);
            pos.filter_moves(moves);
            std::forward<Func>(func)(pos, depth);
        });
    }
}

static void benchmark_best_move() {
    benchmark_pos_method("Best-move", 12, [](Position& pos, int depth){
        std::array<Move, 256> move_buf;
        std::span<Move> moves = pos.generate_moves(move_buf);
        pos.filter_moves(moves);
        pos.best_move(moves, depth);
    });
}

static void benchmark_raw_negamax() {
    benchmark_pos_method("Raw Negamax", 5, [](Position& pos, int depth){
        pos.negamax(depth, 1);
    });
}

static void benchmark_pruned_negamax() {
    benchmark_pos_method("Pruned Negamax", 12, [](Position& pos, int depth){
        KillerTable killers{};
        HistoryTable history{};
        pos.pruned_negamax(depth, history, killers, 1, true, INT32_MIN, INT32_MAX);
    });
}

int main() {
    //benchmark_perft();
    benchmark_best_move();
    benchmark_raw_negamax();
    benchmark_pruned_negamax();
    return 0;
}