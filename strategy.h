#ifndef __STRATEGY_H
#define __STRATEGY_H

#include <climits>
#include <vector>
#include "bidiarray.h"
#include "move.h"

class Strategy {

private:
    //! array containing all blobs on the board
    bidiarray<Sint16> _blobs;
    //! an array of booleans indicating for each cell whether it is a hole or not.
    const bidiarray<bool>& _holes;
    //! Current player
    Uint16 _current_player;
    //! Player the search is being run for: the evaluation function always
    //! scores a position from this player's point of view, regardless of
    //! whose turn it is at a given node. Fixed for the whole search tree.
    Uint16 _root_player;
    //! Call this function to save your best move.
    void (*_saveBestMove)(movement&);

    /** Greedy: pick move maximising immediate score */
    void computeBestMoveGreedy();
    /** Classic minimax to MINIMAX_DEPTH */
    void computeBestMoveMinimax();
    /** Alpha-beta to ALPHABETA_DEPTH */
    void computeBestMoveAlphaBeta();
    /** Parallel minimax (pthread, one thread per root move) */
    void computeBestMoveParallelMinimax();
    /** Parallel alpha-beta (pthread, one thread per root move) */
    void computeBestMoveParallelAlphaBeta();

public:
    // Constructor from a current situation
    Strategy(bidiarray<Sint16>& blobs,
             const bidiarray<bool>& holes,
             const Uint16 current_player,
             void (*saveBestMove)(movement&))
        : _blobs(blobs), _holes(holes), _current_player(current_player),
          _root_player(current_player), _saveBestMove(saveBestMove)
    {}

    // Copy constructor (needed for recursive tree search)
    Strategy(const Strategy& St)
        : _blobs(St._blobs), _holes(St._holes), _current_player(St._current_player),
          _root_player(St._root_player), _saveBestMove(St._saveBestMove)
    {}

    ~Strategy() {}

    /** Apply a move to the current state of blobs. Assumes the move is valid. */
    void applyMove(const movement& mv);

    /** Compute the vector containing every possible moves. */
    std::vector<movement>& computeValidMoves(std::vector<movement>& valid_moves) const;

    /** Estimate the score of the current state, from the point of view of the
     *  player the search was started for (_root_player), not whoever's turn
     *  it currently is. Positive = good for that player. */
    Sint32 estimateCurrentScore() const;

    /** Minimax recursive — public so thread functions can access it. */
    Sint32 minimax(int depth, bool maximizing);

    /** Alpha-Beta recursive — public so thread functions can access it. */
    Sint32 alphabeta(int depth, Sint32 alpha, Sint32 beta, bool maximizing);

    /**
     * Find the best move and save it via _saveBestMove.
     * Uses parallel alpha-beta by default.
     */
    void computeBestMove();
};

#endif
