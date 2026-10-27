// =============================================================================
//  input.h - Lecture du joystick et des boutons
// =============================================================================
#pragma once

// Etat des commandes à un instant donné
struct InputState {
    float joystickX;     // De -1.0 (gauche) à +1.0 (droite), 0 au repos
    bool  buttonLeft;    // true tant que SW1 est maintenu
    bool  buttonRight;   // true tant que SW2 est maintenu
    bool  startPressed;  // true uniquement à l'image où le joystick est enfoncé
};

// Configure les broches et calibre le centre du joystick.
// Le joystick ne doit pas être touché pendant l'appel.
void inputBegin();

// Lit l'état actuel des commandes
InputState inputRead();
