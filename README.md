# Blunderfish

A chess engine written in C++20, featuring bitboard-based board representation, magic bitboard move generation, and a negamax search with alpha-beta pruning and several modern search enhancements. Comes with a command-line interface, and a web-based UI for playing in the browser.

## Features

### Board Representation
- **Bitboards** — each side maintains separate 64-bit bitboards for every piece type, enabling fast bulk operations via bitwise logic
- **Piece-at array** — redundant `piece_at[64]` map for O(1) square lookups
- **FEN parsing** — full FEN string support for loading arbitrary positions

### Move Generation
- **Magic bitboards** for sliding pieces (rooks, bishops, queens) — precomputed attack tables indexed via magic multiplication
- **Precomputed lookup tables** for knight and king moves, and pawn attacks
- **Pawn move generation** via bitboard shifts (single push, double push, captures, en passant, promotions)
- **Capture-only generation** for use in quiescence search
- **Legal move filtering** via make/unmake with in-check detection
- **Move encoding** — compact 32-bit representation: from (6 bits), to (6 bits), move type (3 bits), end piece (3 bits), side (1 bit)

### Search
- **Negamax** with alpha-beta pruning
- **Iterative deepening** — searches from depth 1 up to the target, reusing the best move from the previous iteration for move ordering
- **Aspiration windows** — after the first iteration, subsequent searches use a narrow window (±50 cp) around the previous score; on fail-high or fail-low the window doubles and re-searches until the true score is found
- **Transposition table** — a hash table (2²⁰ entries) indexed by Zobrist key stores exact, upper-bound, or lower-bound scores with depth, best move, and mate-score adjustment; used for cutoffs and move ordering (TT move searched first)
- **Quiescence search** — extends captures (and checks when in check) beyond the horizon to mitigate the horizon effect
- **Null move pruning** — if giving the opponent a free move still results in a beta cutoff, the branch is pruned (R=2 reduction, skipped when in check or no non-pawn material)
- **Late move reductions (LMR)** — quiet moves searched after the first 3 are reduced by 1 ply; re-searched at full depth if they beat alpha
- **Futility pruning** — at depth 1, quiet moves are skipped if the static eval + 200 centipawns can't reach alpha
- **Killer move heuristic** — two killer moves stored per ply for move ordering of quiet moves that caused beta cutoffs
- **History heuristic** — piece-to-square history table updated on quiet beta cutoffs, used for move ordering
- **MVV-LVA** (Most Valuable Victim – Least Valuable Attacker) ordering for captures

### Evaluation
- **Material counting** — Pawn: 100, Knight: 300, Bishop: 300, Rook: 500, Queen: 900 (centipawns)
- **Piece-square tables** — positional bonuses from the [Simplified Evaluation Function](https://www.chessprogramming.org/Simplified_Evaluation_Function)
- **Side-relative scoring** — evaluation returned from the perspective of the side to move (negamax-friendly)

### Zobrist Hashing
- **Incremental Zobrist hashing** — updated on every make/unmake for efficient position identification
- **PCG32 PRNG** for deterministic generation of random keys
- Hashes pieces, side to move, castling flags, and en passant square

### Precomputed Tables
A separate build-time executable (`precompute_tables`) generates `generated_tables.h`, containing:
- Magic bitboard tables for rooks and bishops (masks, magic numbers, shifts, attack tables)
- Knight and king move tables
- Pawn attack tables (per square, per side)

## Building

### Prerequisites
- C++20 compatible compiler (GCC 11+, Clang 14+, or MSVC 2022+)
- CMake 3.16+

### Build Steps

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

This will:
1. Build the `precompute_tables` tool and generate the magic bitboard lookup tables
2. Build the `blunderfish` static library
3. Build the `cli` executable
4. Build the test and benchmark executables

## Usage

### CLI

```bash
# Interactive play (you are White, engine plays Black at depth 12)
./build/cli/cli play

# Evaluate a position (returns centipawn score from side-to-move perspective)
./build/cli/cli eval "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

# Find the best move (default depth 10, minimum 6)
./build/cli/cli best "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" 10
```

### Web UI

The web UI lets you play against Blunderfish in the browser with a drag-and-drop chessboard.

```bash
# First, ensure the CLI is built and copy/link it to engine/
mkdir -p engine
cp build/cli/cli engine/cli

# Start the backend server
cd webui/server
npm install
node server.js  # Runs on http://localhost:3000

# In another terminal, start the frontend dev server
cd webui
npm install
npm run dev     # Runs on http://localhost:5173 (Vite)
```

Features of the web UI:
- Play as White against the engine (Black)
- Adjustable search depth (6–12)
- FEN string loading
- PGN download
- Move list display
- Game status indicator

## Testing

Tests use [Catch2](https://github.com/catchorg/Catch2) v3.5.2 (auto-fetched via CMake FetchContent).

```bash
cd build
ctest
```

Test suites:
- **Perft tests** — verifies move generation correctness against known node counts for the starting position (depths 1–6) and the Kiwipete position (depths 1–5)
- **Zobrist tests** — verifies that incremental Zobrist hash updates match full recomputation through deep game trees
- **Misc tests** — validates that capture-only generation matches filtered full generation, and that pruned negamax returns the same scores as plain negamax

## Benchmarks

```bash
./build/benchmark/benchmark
```

Benchmarks best-move search (depths 1–12), raw negamax (depths 1–5), and pruned negamax (depths 1–12) from the starting position, reporting average time in milliseconds.
