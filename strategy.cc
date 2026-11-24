#include "strategy.h"
#include <climits>
#include <pthread.h>
#include <algorithm>

// profondeur pour minimax et alphabeta
#define PROF_MINIMAX   4
#define PROF_ALPHABETA 6

// nombre de threads pour le parallelisme
#define NB_THREADS 4


// =============================================
// applyMove : applique un mouvement sur le plateau
// =============================================

void Strategy::applyMove(const movement& mv) {

    // distance de chebyshev pour savoir si c'est un clone ou un saut
    int dx = (int)mv.nx - (int)mv.ox;
    int dy = (int)mv.ny - (int)mv.oy;
    int dist = std::max(std::abs(dx), std::abs(dy));

    if (dist == 1) {
        // clone : on pose un nouveau pion a la destination, l'ancien reste
        _blobs.set(mv.nx, mv.ny, _current_player);
    } else {
        // saut : le pion se deplace, on enleve l'ancien
        _blobs.set(mv.ox, mv.oy, -1);
        _blobs.set(mv.nx, mv.ny, _current_player);
    }

    // on capture tous les voisins autour de la case d'arrivee
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            int x = (int)mv.nx + i;
            int y = (int)mv.ny + j;
            // verifier qu'on est pas hors du plateau
            if (x < 0 || x > 7 || y < 0 || y > 7) continue;
            Sint16 case_val = _blobs.get(x, y);
            // si c'est un pion ennemi on le capture
            if (case_val != -1 && case_val != (Sint16)_current_player) {
                _blobs.set(x, y, _current_player);
            }
        }
    }
}


// =============================================
// computeValidMoves : liste tous les coups possibles
// =============================================

vector<movement>& Strategy::computeValidMoves(vector<movement>& coups_valides) const {
    coups_valides.clear();

    for (int ox = 0; ox < 8; ox++) {
        for (int oy = 0; oy < 8; oy++) {

            // on cherche nos pions
            if (_blobs.get(ox, oy) != (Sint16)_current_player) continue;

            // pour chaque pion, on regarde les cases a distance 1 ou 2
            for (int nx = std::max(0, ox - 2); nx <= std::min(7, ox + 2); nx++) {
                for (int ny = std::max(0, oy - 2); ny <= std::min(7, oy + 2); ny++) {

                    if (nx == ox && ny == oy) continue; // meme case, pas valide
                    if (_holes.get(nx, ny)) continue;   // trou, pas valide
                    if (_blobs.get(nx, ny) != -1) continue; // case occupee

                    coups_valides.push_back(movement(ox, oy, nx, ny));
                }
            }
        }
    }
    return coups_valides;
}


// =============================================
// estimateCurrentScore : heuristique
// pions de _root_player - pions adversaires
//
// Important : on compte toujours par rapport a _root_player (le joueur
// pour lequel la recherche a ete lancee), jamais par rapport a
// _current_player. _current_player change a chaque demi-coup pendant la
// recherche, donc si on notait par rapport a lui, une partie qui se termine
// tot (un joueur n'a plus de coup, voir computeValidMoves) serait notee du
// point de vue d'un joueur different des feuilles qui atteignent la
// profondeur maximale, et le score reviendrait parfois invertit.
// =============================================

Sint32 Strategy::estimateCurrentScore() const {
    Sint32 score = 0;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            Sint16 c = _blobs.get(i, j);
            if (c == (Sint16)_root_player) {
                score += 1;
            } else if (c != -1) {
                // pion adverse
                score -= 1;
            }
            // si c == -1 c'est une case vide, on fait rien
        }
    }
    return score;
}


// =============================================
// GLOUTON : on prend le meilleur coup immediat
// =============================================

void Strategy::computeBestMoveGreedy() {
    vector<movement> coups;
    computeValidMoves(coups);
    if (coups.empty()) return;

    Sint32 meilleur_score = INT_MIN;
    movement meilleur_coup = coups[0]; // par defaut le premier coup

    for (int i = 0; i < (int)coups.size(); i++) {
        Strategy copie(*this);
        copie.applyMove(coups[i]);
        Sint32 score = copie.estimateCurrentScore();
        if (score > meilleur_score) {
            meilleur_score = score;
            meilleur_coup = coups[i];
        }
    }
    _saveBestMove(meilleur_coup);
}


// =============================================
// MINIMAX
// maximizing = true quand c'est notre tour
// =============================================

Sint32 Strategy::minimax(int profondeur, bool maximizing) {
    vector<movement> coups;
    computeValidMoves(coups);

    // cas de base : profondeur 0 ou pas de coups
    if (profondeur == 0 || coups.empty()) {
        return estimateCurrentScore();
    }

    if (maximizing) {
        Sint32 meilleur = INT_MIN;
        for (int i = 0; i < (int)coups.size(); i++) {
            Strategy copie(*this);
            copie.applyMove(coups[i]);
            copie._current_player = 1 - copie._current_player; // on change de joueur
            Sint32 val = copie.minimax(profondeur - 1, false);
            if (val > meilleur) meilleur = val;
        }
        return meilleur;
    } else {
        Sint32 meilleur = INT_MAX;
        for (int i = 0; i < (int)coups.size(); i++) {
            Strategy copie(*this);
            copie.applyMove(coups[i]);
            copie._current_player = 1 - copie._current_player;
            Sint32 val = copie.minimax(profondeur - 1, true);
            if (val < meilleur) meilleur = val;
        }
        return meilleur;
    }
}

void Strategy::computeBestMoveMinimax() {
    vector<movement> coups;
    computeValidMoves(coups);
    if (coups.empty()) return;

    Sint32 meilleur_score = INT_MIN;
    movement meilleur_coup = coups[0];

    for (int i = 0; i < (int)coups.size(); i++) {
        Strategy copie(*this);
        copie.applyMove(coups[i]);
        copie._current_player = 1 - copie._current_player;
        // on appelle minimax pour les coups suivants (adversaire joue en premier donc false)
        Sint32 score = copie.minimax(PROF_MINIMAX - 1, false);
        if (score > meilleur_score) {
            meilleur_score = score;
            meilleur_coup = coups[i];
        }
    }
    _saveBestMove(meilleur_coup);
}


// =============================================
// ALPHA-BETA
// meme principe que minimax mais on eague des branches
// alpha = meilleur score garanti pour le maximisant
// beta  = meilleur score garanti pour le minimisant
// =============================================

Sint32 Strategy::alphabeta(int profondeur, Sint32 alpha, Sint32 beta, bool maximizing) {
    vector<movement> coups;
    computeValidMoves(coups);

    if (profondeur == 0 || coups.empty()) {
        return estimateCurrentScore();
    }

    if (maximizing) {
        Sint32 meilleur = INT_MIN;
        for (int i = 0; i < (int)coups.size(); i++) {
            Strategy copie(*this);
            copie.applyMove(coups[i]);
            copie._current_player = 1 - copie._current_player;
            Sint32 val = copie.alphabeta(profondeur - 1, alpha, beta, false);
            if (val > meilleur) meilleur = val;
            if (meilleur > alpha) alpha = meilleur;
            if (beta <= alpha) break; // coupure beta, on arrete
        }
        return meilleur;
    } else {
        Sint32 meilleur = INT_MAX;
        for (int i = 0; i < (int)coups.size(); i++) {
            Strategy copie(*this);
            copie.applyMove(coups[i]);
            copie._current_player = 1 - copie._current_player;
            Sint32 val = copie.alphabeta(profondeur - 1, alpha, beta, true);
            if (val < meilleur) meilleur = val;
            if (meilleur < beta) beta = meilleur;
            if (beta <= alpha) break; // coupure alpha
        }
        return meilleur;
    }
}

void Strategy::computeBestMoveAlphaBeta() {
    vector<movement> coups;
    computeValidMoves(coups);
    if (coups.empty()) return;

    Sint32 meilleur_score = INT_MIN;
    movement meilleur_coup = coups[0];

    for (int i = 0; i < (int)coups.size(); i++) {
        Strategy copie(*this);
        copie.applyMove(coups[i]);
        copie._current_player = 1 - copie._current_player;
        Sint32 score = copie.alphabeta(PROF_ALPHABETA - 1, INT_MIN, INT_MAX, false);
        if (score > meilleur_score) {
            meilleur_score = score;
            meilleur_coup = coups[i];
        }
    }
    _saveBestMove(meilleur_coup);
}


// =============================================
// MINIMAX PARALLELE
// on lance un thread par coup racine
// =============================================

struct ArgThreadMinimax {
    Strategy strat;
    int profondeur;
    Sint32 resultat;
    // constructeur
    ArgThreadMinimax(const Strategy& s, int p) : strat(s), profondeur(p), resultat(0) {}
};

static void* fonctionThreadMinimax(void* arg) {
    ArgThreadMinimax* a = (ArgThreadMinimax*)arg;
    // chaque thread calcule son sous-arbre independamment
    a->resultat = a->strat.minimax(a->profondeur, false);
    return NULL;
}

void Strategy::computeBestMoveParallelMinimax() {
    vector<movement> coups;
    computeValidMoves(coups);
    if (coups.empty()) return;

    int nb_coups = (int)coups.size();
    vector<ArgThreadMinimax*> args;
    vector<pthread_t> threads;

    // on prepare les arguments pour chaque thread
    for (int i = 0; i < nb_coups; i++) {
        Strategy copie(*this);
        copie.applyMove(coups[i]);
        copie._current_player = 1 - copie._current_player;
        args.push_back(new ArgThreadMinimax(copie, PROF_MINIMAX - 1));
    }

    // on lance par batch de NB_THREADS
    int traites = 0;
    while (traites < nb_coups) {
        int taille_batch = std::min(NB_THREADS, nb_coups - traites);
        threads.resize(taille_batch);
        for (int t = 0; t < taille_batch; t++) {
            pthread_create(&threads[t], NULL, fonctionThreadMinimax, args[traites + t]);
        }
        // on attend que tous les threads du batch finissent
        for (int t = 0; t < taille_batch; t++) {
            pthread_join(threads[t], NULL);
        }
        traites += taille_batch;
    }

    // on recupere le meilleur resultat
    Sint32 meilleur_score = INT_MIN;
    movement meilleur_coup = coups[0];
    for (int i = 0; i < nb_coups; i++) {
        if (args[i]->resultat > meilleur_score) {
            meilleur_score = args[i]->resultat;
            meilleur_coup = coups[i];
        }
        delete args[i]; // ne pas oublier de liberer la memoire
    }
    _saveBestMove(meilleur_coup);
}


// =============================================
// ALPHA-BETA PARALLELE
// pareil mais avec alphabeta dans chaque thread
// note : les threads ne partagent pas les bornes alpha/beta
// c'est un compromis pour eviter les problemes de synchro
// =============================================

struct ArgThreadAlphaBeta {
    Strategy strat;
    int profondeur;
    Sint32 resultat;
    ArgThreadAlphaBeta(const Strategy& s, int p) : strat(s), profondeur(p), resultat(0) {}
};

static void* fonctionThreadAlphaBeta(void* arg) {
    ArgThreadAlphaBeta* a = (ArgThreadAlphaBeta*)arg;
    a->resultat = a->strat.alphabeta(a->profondeur, INT_MIN, INT_MAX, false);
    return NULL;
}

void Strategy::computeBestMoveParallelAlphaBeta() {
    vector<movement> coups;
    computeValidMoves(coups);
    if (coups.empty()) return;

    int nb_coups = (int)coups.size();
    vector<ArgThreadAlphaBeta*> args;
    vector<pthread_t> threads;

    for (int i = 0; i < nb_coups; i++) {
        Strategy copie(*this);
        copie.applyMove(coups[i]);
        copie._current_player = 1 - copie._current_player;
        args.push_back(new ArgThreadAlphaBeta(copie, PROF_ALPHABETA - 1));
    }

    int traites = 0;
    while (traites < nb_coups) {
        int taille_batch = std::min(NB_THREADS, nb_coups - traites);
        threads.resize(taille_batch);
        for (int t = 0; t < taille_batch; t++) {
            pthread_create(&threads[t], NULL, fonctionThreadAlphaBeta, args[traites + t]);
        }
        for (int t = 0; t < taille_batch; t++) {
            pthread_join(threads[t], NULL);
        }
        traites += taille_batch;
    }

    Sint32 meilleur_score = INT_MIN;
    movement meilleur_coup = coups[0];
    for (int i = 0; i < nb_coups; i++) {
        if (args[i]->resultat > meilleur_score) {
            meilleur_score = args[i]->resultat;
            meilleur_coup = coups[i];
        }
        delete args[i];
    }
    _saveBestMove(meilleur_coup);
}


// =============================================
// point d'entree principal
// on utilise alphabeta parallele par defaut
// =============================================

void Strategy::computeBestMove() {
    computeBestMoveParallelAlphaBeta();
    // computeBestMoveAlphaBeta();
    // computeBestMoveMinimax();
    // computeBestMoveGreedy();
}
