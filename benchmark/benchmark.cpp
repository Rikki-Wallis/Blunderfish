#include <iostream>
#include <chrono>
#include <cmath>

#include "blunderfish.h"

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
static void benchmark_pos_method(const std::string& name, int start_depth, int max_depth, Func&& func)
{
    for (int depth = start_depth; depth <= max_depth; ++depth) {
        auto start = std::chrono::high_resolution_clock::now();

        Position pos = *Position::decode_fen_string("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

        pos.reset_benchmarking_statistics();
        std::forward<Func>(func)(pos, depth);

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

        double ms = (double)duration.count()/1000000.0;
        double nps = (double)pos.node_count/(ms/1000.0);
        double ebf = pow((double)pos.node_count, 1.0/(double)depth);
        double avg_cutoff_index = (double)pos.cutoff_index_sum/(double)pos.cutoff_index_count;

        print("{} (depth={}):\n", name, depth);
        print("  time: {}ms\n", ms);
        print("  nodes: {}\n", pos.node_count);
        print("  qnodes: {}\n", pos.qnode_count);
        print("  beta_cutoffs: {}\n", pos.beta_cutoffs);
        print("  null-prunes: {}\n", pos.null_prunes);
        print("  NPS: {:.2}\n", nps);
        print("  EBF: {:.2}\n", ebf);
        print("  avg-cutoff-idx: {:.2}\n", avg_cutoff_index);
        print("\n");
    }
}

static void benchmark_best_move() {
    benchmark_pos_method("Best-move", 7, 16, [](Position& pos, int depth){
        std::array<Move, 256> move_buf;
        std::span<Move> moves = pos.generate_moves(move_buf);
        pos.filter_moves(moves);
        pos.best_move(moves, depth);
    });
}

static void benchmark_pruned_negamax() {
    benchmark_pos_method("Pruned Negamax", 7, 12, [](Position& pos, int depth){
        KillerTable killers{};
        HistoryTable history{};
        TranspositionTable tt;
        pos.pruned_negamax(depth, tt, history, killers, 1, true, -INF, INF);
    });
}

int main() {
    //benchmark_perft();
    benchmark_pruned_negamax();
    benchmark_best_move();
    return 0;
}