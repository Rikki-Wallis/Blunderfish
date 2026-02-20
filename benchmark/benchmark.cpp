#include <iostream>
#include <chrono>

#include "blunderfish.h"

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
        std::cout << "Depth " << i+1 << ": " << nodes[i] << " nodes in "
                    << seconds << "s (" << nps << " nodes/sec)\n";
        
    };
}

static void benchmark_best_move() {
    Position pos = *Position::decode_fen_string("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    std::vector<uint64_t> times;
    std::array<Move, 256> move_buf;
    std::span<Move> moves = pos.generate_moves(move_buf);
    pos.filter_moves(moves);

    times.push_back(run_and_time([&] {return pos.best_move(moves, 1);}));
    times.push_back(run_and_time([&] {return pos.best_move(moves, 2);}));
    times.push_back(run_and_time([&] {return pos.best_move(moves, 3);}));
    times.push_back(run_and_time([&] {return pos.best_move(moves, 4);}));
    times.push_back(run_and_time([&] {return pos.best_move(moves, 5);}));
    times.push_back(run_and_time([&] {return pos.best_move(moves, 6);}));

    size_t i = 0;
    std::vector<uint64_t> nodes = {20, 400, 8902, 197281, 4865609, 119060324};
    for (i = 0; i < times.size(); ++i) {
                    
        double seconds = times[i] / 1000.0;
        double nps = seconds > 0 ? nodes[i] / seconds : 0;
        std::cout << "Depth " << i+1 << ": " << nodes[i] << " nodes in "
                    << seconds << "s (" << nps << " nodes/sec)\n";
        
    };
}

int main() {
    benchmark_perft();
    benchmark_best_move();
    return 0;
}