# Blobwar AI

Four game-tree search strategies (greedy, minimax, alpha-beta, and parallel
versions of both with pthreads) for **Blobwar**, a territory-capture board
game in the spirit of Atax/Ataxx: two players place and jump blobs on an 8x8
board, capturing every enemy blob adjacent to the destination square.


## What this project is

The game itself (board, SDL rendering, rules, networking) was the base code
provided for the course. **The AI in `strategy.cc` is the part implemented
here**: given a board position, it searches the game tree and picks a move.

| Strategy | Idea | Depth |
| --- | --- | --- |
| Greedy | Play the move that maximizes the immediate score | 1 |
| Minimax | Explore the full game tree, assuming the opponent plays optimally against you | 4 |
| Alpha-beta | Minimax with branches pruned once they can no longer change the result | 6 |
| Parallel minimax / alpha-beta | Same search, one pthread per root move, run in batches of 4 | same depths |

The evaluation function is simple material count: `(my blobs) - (opponent's
blobs)`.

## A bug this project fixes: scoring from the wrong player

The leaf evaluation must always be computed **from the point of view of the
player the search was started for** (call it the root player), not from
whoever's turn it happens to be at that leaf. This matters because a branch
can end early: if a player runs out of legal moves (boxed in), the search
stops there immediately, before reaching the intended depth, and that
premature leaf can belong to either player depending on the path taken.

The original code scored every leaf from `_current_player`'s point of view,
which flips at every ply. Nodes that stopped early because a player was
boxed in ended up scored from a different player's perspective than nodes
that reached the depth limit normally, silently flipping the sign of the
evaluation for that branch.

Concrete repro (`strategy.cc`'s own evaluation, used directly, see the test
below): a position where player 0 has 9 blobs against player 1's single
blob, fully boxed into a corner. From player 0's point of view this is
obviously winning. A 2-ply search:

```
Original code:  minimax(depth=2) = 2
Fixed code:     minimax(depth=2) = 9   (correct: 9 blobs vs 1, heavily in player 0's favor)
```

The fix adds a `_root_player` field, set once when the search starts and
carried unchanged through every recursive copy, and scores every leaf
against it instead of against `_current_player`:

```cpp
// Before: flips meaning depending on whose turn it is at this node
if (c == (Sint16)_current_player) score += 1;

// After: always scored from the player the search is being run for
if (c == (Sint16)_root_player) score += 1;
```

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
