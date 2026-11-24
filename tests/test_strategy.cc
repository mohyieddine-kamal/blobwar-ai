// Regression test for the "scored from the wrong player" bug described in
// the README. Builds only the game-logic files (strategy.cc, bidiarray.cc),
// not the SDL GUI, so it has no SDL 1.2 dependency: see tests/README.md.
#include <cstdio>
#include <climits>
#include <cassert>
#include "../strategy.h"
#include "../bidiarray.h"

static movement g_saved;
static void save(movement& m) { g_saved = m; }

int main() {
    bidiarray<bool> holes;
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++)
            holes.set(i, j, false);

    bidiarray<Sint16> blobs;
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++)
            blobs.set(i, j, -1);

    // Player 1 (opponent) has a single blob trapped in the corner by a wall
    // of player 0 blobs: after player 0's first move, player 1 has no legal
    // move left, so the search for that branch stops early at depth 1
    // instead of running to the full requested depth.
    blobs.set(0, 0, 1);
    blobs.set(0, 1, 0); blobs.set(1, 0, 0); blobs.set(1, 1, 0);
    blobs.set(0, 2, 0); blobs.set(2, 0, 0); blobs.set(2, 2, 0);
    blobs.set(1, 2, 0); blobs.set(2, 1, 0);
    // player 0 also has a free blob elsewhere, so player 0 (root, to move)
    // has legal moves and the search actually has a choice to make.
    blobs.set(7, 7, 0);

    Strategy s(blobs, holes, /*current_player=*/0, save);
    Sint32 val = s.minimax(2, true);

    // From the root player's (player 0) point of view this is a dominant
    // position: 9 blobs against player 1's single trapped blob. The search
    // must stay strongly positive, whatever path it explores to get there.
    printf("minimax(depth=2) root value = %d (expect a large positive number)\n", val);
    assert(val > 0);
    printf("OK: the early-ending branch was scored from the root player's "
           "point of view, not the opponent's.\n");
    return 0;
}
