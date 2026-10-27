// =============================================================================
//  renderer.h - Affichage du jeu sur l'écran TFT
// =============================================================================
#pragma once

#include "game.h"

// Initialise le bus SPI et l'écran
void rendererBegin();

// Met à jour l'écran pour qu'il corresponde à l'état du jeu
void rendererDraw(const Game& game);
