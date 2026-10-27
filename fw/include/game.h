// =============================================================================
//  game.h - Etat et règles du jeu
// =============================================================================
//  Ce module ne dessine rien : il ne fait que calculer les positions et les
//  scores. L'affichage est entièrement géré par le module "renderer".
// =============================================================================
#pragma once

#include <stdint.h>
#include "input.h"

// Les différents écrans du jeu
enum class GameState {
    Title,     // Ecran d'accueil
    Playing,   // Partie en cours
    GameOver,  // Un joueur a gagné
};

// Une raquette. (x, y) est le coin supérieur gauche.
struct Paddle {
    float x;
    float y;
    int   score;
};

// La balle. (x, y) est le coin supérieur gauche, (vx, vy) la vitesse en
// pixels par image.
struct Ball {
    float x;
    float y;
    float vx;
    float vy;
    float speed;  // Norme de la vitesse, augmente à chaque renvoi
};

struct Game {
    GameState state;
    Paddle    bottom;      // Joueur 1 (joystick)
    Paddle    top;         // Ordinateur ou joueur 2 (boutons SW1 / SW2)
    Ball      ball;
    bool      topIsHuman;  // Passe à true dès que SW1 ou SW2 est utilisé
    uint32_t  serveTimeMs; // Instant où la balle sera lancée
};

// Met le jeu dans son état initial (écran d'accueil)
void gameInit(Game& game);

// Fait avancer le jeu d'une image en fonction des commandes
void gameUpdate(Game& game, const InputState& input, uint32_t nowMs);
