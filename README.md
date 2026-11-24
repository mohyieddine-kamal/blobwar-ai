# Blobwar AI

Four game-tree search strategies (greedy, minimax, alpha-beta, and parallel
versions of both with pthreads) for **Blobwar**, a territory-capture board
game in the spirit of Atax/Ataxx: two players place and jump blobs on an 8x8
board. Moving one cell clones a blob, moving two cells jumps it, and any
enemy blob adjacent to the landing square flips to the mover's color.

## What this project is

The game itself (board, SDL rendering, rules, networking) was the base code
provided for the course. **The AI in `strategy.cc` is the part implemented
here**: given a board position, it searches the game tree and picks a move.

## How the AI works

- **Evaluation function**: simple material count, `(my blobs) - (opponent's
  blobs)`, computed from the point of view of the player the search is
  running for.
- **Greedy** (`computeBestMoveGreedy`): looks one move ahead and plays
  whichever move maximizes the immediate score. Fast but short-sighted,
  used as a baseline and as a quick move generator for comparison.
- **Minimax** (`computeBestMoveMinimax`, depth 4): explores the full game
  tree down to a fixed depth, alternating a maximizing level (my best move)
  and a minimizing level (assuming the opponent plays their best response
  against me), and backs the score up to the root.
- **Alpha-beta** (`computeBestMoveAlphaBeta`, depth 6): the same search as
  minimax, but carries an `[alpha, beta]` window down the recursion and cuts
  a branch as soon as it proves it can no longer change the result. This
  lets it search two levels deeper than plain minimax for the same cost.
- **Parallel minimax / alpha-beta** (`computeBestMoveParallelMinimax`,
  `computeBestMoveParallelAlphaBeta`): the root's possible moves are
  independent sub-searches, so they're split across pthreads in batches of
  `NB_THREADS = 4`, each thread running the sequential search on its own
  copy of the board. `computeBestMove()` uses parallel alpha-beta by
  default.
- **Move classification**: `applyMove` tells a clone from a jump by the
  Chebyshev distance between the origin and destination cell (`dist == 1`
  keeps the source blob, `dist > 1` empties it), then flips every adjacent
  enemy blob to apply the capture rule.

While working on this I found and fixed a scoring bug in the shared
minimax/alpha-beta base case (a leaf could be evaluated from the wrong
player's point of view when a branch ended early because a player ran out
of legal moves); see `tests/` for the regression test that catches it.

## Build

The GUI (`blobwar` and `launchStrategy`) depends on **SDL 1.2**
(`SDL`, `SDL_image`, `SDL_ttf`, `SDL_net`), which is no longer packaged on
current Linux distributions:

```bash
sudo apt install libsdl1.2-dev libsdl-image1.2-dev libsdl-ttf2.0-dev libsdl-net1.2-dev
make
./blobwar
```

The AI logic itself (`strategy.cc`, `bidiarray.cc`) has no SDL dependency
beyond a few integer typedefs (`Sint16`, `Sint32`...) and builds and tests
fine on its own; see `tests/` for a standalone example that does not need
SDL 1.2 at all.

## Project structure

```
strategy.cc / strategy.h   # the AI: this project's contribution
board.cc, rules.cc, blobwar.cc, main.cc, ...   # provided base game (SDL GUI, rules)
launchStrategy.cc          # small executable the game spawns to compute a move
server/                    # network play server
data/                      # sprites, fonts, board layouts
docs/rapport.pdf           # project report (French)
```

## Context

Solo project, C++ / algorithms course at Grenoble INP - Ensimag, 2026. Base
game code (SDL rendering, board, rules, networking) was provided; the
minimax, alpha-beta, and parallel search strategies in `strategy.cc` were
implemented for this project.
