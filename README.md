# Blunderfish

A modern chess engine written in C++20 with UCI protocol support, a web-based GUI, SPSA parameter tuning, and comprehensive testing.

## Features

### Search
- Negamax with alpha-beta pruning and aspiration windows
- Transposition table (clustered buckets, Zobrist hashing)
- Null move pruning with adaptive reduction
- Singular extensions
- Reverse futility pruning
- Late move reductions (LMR) with history-based adjustments
- Late move pruning
- Futility pruning
- Quiescence search with delta pruning
- Killer moves, history heuristic, and continuation history
- MVV-LVA with static exchange evaluation (SEE) for capture ordering
- Threefold repetition detection

### Evaluation
- Tapered evaluation (middlegame/endgame interpolation)
- Piece-square tables
- King safety (castling rights, pawn shelter, open files)
- Pawn structure (isolated, doubled, connected, passed pawns)
- Bishop pair bonus

### Board Representation
- Bitboards with magic bitboard move generation
- Pre-computed lookup tables for king, knight, and sliding piece moves

### Other
- UCI protocol support
- Web UI for playing against the engine in a browser
- SPSA parameter tuning via self-play
- Opening book generation
- Perft testing
- Benchmarking suite

## Building

Requires CMake 3.16+ and a C++20 compiler.

```bash
mkdir build && cd build
cmake .. -G Ninja
ninja
```

This builds the following targets:

| Target | Description |
|---|---|
| `uci` | UCI protocol interface |
| `benchmark` | Performance benchmarking tool |
| `spsa` | SPSA parameter tuning via self-play |
| `opening_gen` | Random opening position generator |
| `precompute_tables` | Magic bitboard table generator (runs at build time) |

Tests are built automatically and can be run with:

```bash
ctest
```

## Web UI

The web UI lets you play against Blunderfish in a browser with drag-and-drop piece movement, adjustable engine depth, move history, FEN input/output, PGN download, and board flipping.

```bash
cd webui
npm install
npm start
```

The server runs on `localhost:3000` and communicates with the engine binary via a REST API.

## Testing

The test suite uses [Catch2](https://github.com/catchorg/Catch2) and covers:

- **Perft** — move generation correctness against known node counts
- **FEN** — encoding/decoding roundtrip verification
- **Zobrist** — hash function validation
- **Eval** — evaluation function output verification
- **Best move** — search result correctness
- **Misc** — chess rules and utility functions

## SPSA Tuning

The SPSA tuner optimizes search parameters through self-play tournaments. It tunes LMR rates, pruning margins, history bonus factors, aspiration window sizes, null move pruning depth, and more.

```bash
./build/spsa/spsa
```

## License

MIT — see [LICENSE](LICENSE) for details.
