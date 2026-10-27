// =============================================================================
//  config.h - Brochage de la carte et réglages du jeu
// =============================================================================
//  Toutes les valeurs "magiques" du projet sont regroupées ici.
//  Pour modifier le comportement du jeu (vitesse, taille des raquettes...),
//  il suffit en général de changer une constante dans ce fichier.
// =============================================================================
#pragma once

#include <Arduino.h>

// -----------------------------------------------------------------------------
//  Brochage (d'après le schéma ecad/ecad.kicad_sch)
// -----------------------------------------------------------------------------

// Ecran TFT ST7789 240x320 (connecteur J1)
constexpr int PIN_TFT_RESET = D7;
constexpr int PIN_TFT_DC    = D8;   // Data/Command : 0 = commande, 1 = donnée
constexpr int PIN_TFT_SCK   = D9;   // Horloge SPI
constexpr int PIN_TFT_MOSI  = D10;  // Données SPI (microcontrôleur -> écran)
constexpr int PIN_TFT_CS    = -1;   // CS est relié à GND sur la carte : l'écran
                                    // est toujours sélectionné (-1 = pas de broche)

// Joystick analogique (SW5)
constexpr int PIN_JOYSTICK_X      = D0;  // Axe horizontal
constexpr int PIN_JOYSTICK_Y      = D1;  // Axe vertical (non utilisé par le Pong)
constexpr int PIN_JOYSTICK_SELECT = D2;  // Appui sur le joystick (actif à l'état bas)

// Boutons poussoirs (actifs à l'état bas, reliés à GND)
constexpr int PIN_BUTTON_LEFT  = D5;  // SW1
constexpr int PIN_BUTTON_RIGHT = D6;  // SW2

// -----------------------------------------------------------------------------
//  Ecran
// -----------------------------------------------------------------------------

constexpr uint32_t TFT_SPI_FREQUENCY = 40000000;  // 40 MHz

// L'écran est utilisé en mode portrait.
// Rotation 0 = portrait, 2 = portrait retourné de 180 degrés.
constexpr int SCREEN_ROTATION = 2;
constexpr int SCREEN_WIDTH    = 240;
constexpr int SCREEN_HEIGHT   = 320;

// Bandeau des scores en haut de l'écran. Le terrain de jeu est en dessous.
constexpr int SCORE_BAR_HEIGHT = 24;
constexpr int FIELD_TOP        = SCORE_BAR_HEIGHT;
constexpr int FIELD_BOTTOM     = SCREEN_HEIGHT;
constexpr int FIELD_LEFT       = 0;
constexpr int FIELD_RIGHT      = SCREEN_WIDTH;

// -----------------------------------------------------------------------------
//  Entrées
// -----------------------------------------------------------------------------

// Zone morte autour du centre du joystick (sur 4095), pour ignorer le bruit
constexpr int JOYSTICK_DEADZONE = 300;

// Mettre à true si la raquette va à droite quand on pousse le joystick à gauche
constexpr bool JOYSTICK_INVERT_X = true;

// -----------------------------------------------------------------------------
//  Jeu
// -----------------------------------------------------------------------------

constexpr uint32_t FRAME_TIME_MS = 16;  // Durée d'une image : ~60 images/s

// Les raquettes sont horizontales : une en bas (joueur 1), une en haut
constexpr int   PADDLE_WIDTH     = 40;
constexpr int   PADDLE_HEIGHT    = 6;
constexpr int   PADDLE_MARGIN    = 10;    // Distance entre la raquette et le bord
constexpr float PADDLE_MAX_SPEED = 5.0f;  // En pixels par image
constexpr float AI_MAX_SPEED     = 3.2f;  // L'ordinateur est un peu plus lent

constexpr int   BALL_SIZE         = 6;
constexpr float BALL_START_SPEED  = 3.0f;   // En pixels par image
constexpr float BALL_SPEED_UP     = 0.25f;  // Gain de vitesse à chaque renvoi
constexpr float BALL_MAX_SPEED    = 8.0f;
constexpr float BALL_MAX_ANGLE    = 60.0f;  // Angle max de rebond sur une raquette (degrés)

constexpr uint32_t SERVE_DELAY_MS = 1000;  // Pause avant chaque service
constexpr int      WINNING_SCORE  = 7;

// Si la balle avance de plus que (balle + raquette) en une image, elle peut
// "traverser" la raquette sans que la collision soit détectée.
static_assert(BALL_MAX_SPEED < BALL_SIZE + PADDLE_HEIGHT,
              "BALL_MAX_SPEED trop grand : la balle pourrait traverser les raquettes");
