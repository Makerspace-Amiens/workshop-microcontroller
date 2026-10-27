// =============================================================================
//  input.cpp - Lecture du joystick et des boutons
// =============================================================================
#include "input.h"
#include "config.h"

// Valeur lue sur l'axe X quand le joystick est au repos (mesurée au démarrage)
static int joystickCenterX = 2048;

// Etat du bouton "select" à l'appel précédent, pour détecter l'appui
static bool selectWasPressed = false;

// Les boutons relient la broche à GND : appuyé = niveau bas
static bool isPressed(int pin) {
    return digitalRead(pin) == LOW;
}

// Convertit une lecture brute (0..4095) en valeur entre -1.0 et +1.0
static float normalizeJoystick(int raw, int center) {
    int offset = raw - center;

    if (abs(offset) < JOYSTICK_DEADZONE) {
        return 0.0f;
    }

    // Le centre n'est pas forcément à 2048 : on divise par la course
    // disponible du côté où se trouve le joystick
    int range = (offset > 0) ? (4095 - center) : center;
    return constrain((float)offset / range, -1.0f, 1.0f);
}

void inputBegin() {
    // Résistances de tirage internes : la broche est à l'état haut au repos
    pinMode(PIN_JOYSTICK_SELECT, INPUT_PULLUP);
    pinMode(PIN_BUTTON_LEFT, INPUT_PULLUP);
    pinMode(PIN_BUTTON_RIGHT, INPUT_PULLUP);

    analogReadResolution(12);  // Lectures entre 0 et 4095

    // Calibration : moyenne de plusieurs lectures au repos
    const int samples = 16;
    long sum = 0;
    for (int i = 0; i < samples; i++) {
        sum += analogRead(PIN_JOYSTICK_X);
        delay(2);
    }
    joystickCenterX = sum / samples;
}

InputState inputRead() {
    InputState input;

    input.joystickX = normalizeJoystick(analogRead(PIN_JOYSTICK_X), joystickCenterX);
    if (JOYSTICK_INVERT_X) {
        input.joystickX = -input.joystickX;
    }

    input.buttonLeft  = isPressed(PIN_BUTTON_LEFT);
    input.buttonRight = isPressed(PIN_BUTTON_RIGHT);

    // Détection de front : on ne signale l'appui qu'une seule fois, au moment
    // où le bouton passe de "relâché" à "appuyé". Comme on ne lit les entrées
    // que toutes les 16 ms, les rebonds mécaniques du bouton sont ignorés.
    bool selectIsPressed = isPressed(PIN_JOYSTICK_SELECT);
    input.startPressed = selectIsPressed && !selectWasPressed;
    selectWasPressed = selectIsPressed;

    return input;
}
