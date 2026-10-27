// =============================================================================
//  main.cpp - Pong sur XIAO ESP32-S3 et écran TFT ST7789 240x320
// =============================================================================
//  Organisation du programme :
//
//    config.h     Brochage de la carte et réglages du jeu
//    input.cpp    Lecture du joystick et des boutons   -> InputState
//    game.cpp     Règles du jeu (positions, collisions, score), sans affichage
//    renderer.cpp Dessin de l'état du jeu sur l'écran
//    main.cpp     Boucle principale : lire -> calculer -> afficher
//
//  Commandes :
//    Joystick        Raquette du bas (joueur 1)
//    SW1 / SW2       Raquette du haut (joueur 2). Tant qu'ils ne sont pas
//                    utilisés, la raquette du haut est jouée par l'ordinateur.
//    Appui joystick  Démarrer une partie
// =============================================================================
#include <Arduino.h>

#include "config.h"
#include "input.h"
#include "game.h"
#include "renderer.h"

static Game game;

void setup() {
    inputBegin();
    rendererBegin();
    gameInit(game);
}

void loop() {
    // Le jeu avance à vitesse constante : une image toutes les FRAME_TIME_MS
    static uint32_t lastFrameMs = 0;
    uint32_t nowMs = millis();
    if (nowMs - lastFrameMs < FRAME_TIME_MS) {
        return;
    }
    lastFrameMs = nowMs;

    InputState input = inputRead();     // 1. Lire les commandes
    gameUpdate(game, input, nowMs);     // 2. Calculer la nouvelle image
    rendererDraw(game);                 // 3. L'afficher
}
