// =============================================================================
//  renderer.cpp - Affichage du jeu sur l'écran TFT ST7789
// =============================================================================
//  Redessiner tout l'écran à chaque image serait trop lent (240 x 320 pixels
//  à envoyer en SPI) et provoquerait un scintillement. On ne redessine donc
//  que ce qui a changé depuis l'image précédente :
//    - la balle : on efface l'ancienne position puis on dessine la nouvelle ;
//    - les raquettes : on efface / dessine uniquement la bande qui a bougé ;
//    - le score : seulement quand il change.
//  Pour cela, le module mémorise ce qui est actuellement affiché.
// =============================================================================
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#include "renderer.h"
#include "config.h"

constexpr uint16_t COLOR_BACKGROUND = ST77XX_BLACK;
constexpr uint16_t COLOR_FOREGROUND = ST77XX_WHITE;
constexpr uint16_t COLOR_HIGHLIGHT  = ST77XX_YELLOW;

// Caractères de la police par défaut : 6 x 8 pixels (multipliés par la taille)
constexpr int CHAR_WIDTH  = 6;
constexpr int CHAR_HEIGHT = 8;

// Le filet est une ligne pointillée horizontale au milieu du terrain
constexpr int NET_Y         = (FIELD_TOP + FIELD_BOTTOM) / 2 - 1;
constexpr int NET_THICKNESS = 2;

static Adafruit_ST7789 tft(&SPI, PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RESET);

// Ce qui est actuellement affiché à l'écran
static bool      screenIsValid = false;  // false = tout redessiner
static GameState drawnState;
static int       drawnBottomX;
static int       drawnTopX;
static int       drawnBallX;
static int       drawnBallY;
static int       drawnBottomScore;
static int       drawnTopScore;
static bool      drawnTopIsHuman;

// -----------------------------------------------------------------------------
//  Fonctions de dessin de base
// -----------------------------------------------------------------------------

// Affiche un texte centré horizontalement
static void drawCenteredText(const char* text, int y, int size, uint16_t color) {
    int width = strlen(text) * CHAR_WIDTH * size;
    tft.setTextSize(size);
    tft.setTextColor(color);
    tft.setCursor((SCREEN_WIDTH - width) / 2, y);
    tft.print(text);
}

static void drawNet() {
    const int dashLength = 8;
    for (int x = dashLength / 2; x < SCREEN_WIDTH; x += 2 * dashLength) {
        tft.fillRect(x, NET_Y, dashLength, NET_THICKNESS, COLOR_FOREGROUND);
    }
}

// Vrai si deux rectangles se chevauchent
static bool rectsOverlap(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
    return x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2;
}

// -----------------------------------------------------------------------------
//  Eléments du terrain
// -----------------------------------------------------------------------------

static void drawScoreBar(const Game& game) {
    tft.fillRect(0, 0, SCREEN_WIDTH, SCORE_BAR_HEIGHT, COLOR_BACKGROUND);

    // Ligne de séparation avec le terrain
    tft.drawFastHLine(0, SCORE_BAR_HEIGHT - 2, SCREEN_WIDTH, COLOR_FOREGROUND);

    // Score du joueur du haut à gauche, celui du joueur du bas à droite
    char text[24];
    snprintf(text, sizeof(text), "%s %d - %d J1",
             game.topIsHuman ? "J2" : "CPU", game.top.score, game.bottom.score);
    drawCenteredText(text, 4, 2, COLOR_FOREGROUND);

    drawnBottomScore = game.bottom.score;
    drawnTopScore    = game.top.score;
    drawnTopIsHuman  = game.topIsHuman;
}

// Déplace une raquette horizontalement de oldX à newX. Seules les colonnes qui
// changent sont envoyées à l'écran, ce qui évite le scintillement.
static void movePaddle(int y, int oldX, int newX) {
    if (newX == oldX) {
        return;
    }

    int distance = min(abs(newX - oldX), PADDLE_WIDTH);

    if (newX > oldX) {
        // Vers la droite : on efface à gauche et on dessine à droite
        tft.fillRect(oldX, y, distance, PADDLE_HEIGHT, COLOR_BACKGROUND);
        tft.fillRect(newX + PADDLE_WIDTH - distance, y, distance, PADDLE_HEIGHT, COLOR_FOREGROUND);
    } else {
        // Vers la gauche : on efface à droite et on dessine à gauche
        tft.fillRect(oldX + PADDLE_WIDTH - distance, y, distance, PADDLE_HEIGHT, COLOR_BACKGROUND);
        tft.fillRect(newX, y, distance, PADDLE_HEIGHT, COLOR_FOREGROUND);
    }
}

static void moveBall(const Game& game, int newX, int newY) {
    if (newX == drawnBallX && newY == drawnBallY) {
        return;
    }

    // 1. Effacer l'ancienne balle
    tft.fillRect(drawnBallX, drawnBallY, BALL_SIZE, BALL_SIZE, COLOR_BACKGROUND);

    // 2. Réparer ce que l'effacement a pu abîmer (filet, raquettes)
    if (rectsOverlap(drawnBallX, drawnBallY, BALL_SIZE, BALL_SIZE,
                     0, NET_Y, SCREEN_WIDTH, NET_THICKNESS)) {
        drawNet();
    }
    if (rectsOverlap(drawnBallX, drawnBallY, BALL_SIZE, BALL_SIZE,
                     drawnBottomX, game.bottom.y, PADDLE_WIDTH, PADDLE_HEIGHT)) {
        tft.fillRect(drawnBottomX, game.bottom.y, PADDLE_WIDTH, PADDLE_HEIGHT, COLOR_FOREGROUND);
    }
    if (rectsOverlap(drawnBallX, drawnBallY, BALL_SIZE, BALL_SIZE,
                     drawnTopX, game.top.y, PADDLE_WIDTH, PADDLE_HEIGHT)) {
        tft.fillRect(drawnTopX, game.top.y, PADDLE_WIDTH, PADDLE_HEIGHT, COLOR_FOREGROUND);
    }

    // 3. Dessiner la nouvelle balle
    tft.fillRect(newX, newY, BALL_SIZE, BALL_SIZE, COLOR_FOREGROUND);

    drawnBallX = newX;
    drawnBallY = newY;
}

// -----------------------------------------------------------------------------
//  Ecrans complets (dessinés une seule fois, au changement d'état)
// -----------------------------------------------------------------------------

static void drawTitleScreen() {
    tft.fillScreen(COLOR_BACKGROUND);
    drawCenteredText("PONG", 60, 6, COLOR_FOREGROUND);
    drawCenteredText("Joystick : raquette du bas", 150, 1, COLOR_FOREGROUND);
    drawCenteredText("SW1 / SW2 : raquette du haut", 170, 1, COLOR_FOREGROUND);
    drawCenteredText("(joueur 2)", 182, 1, COLOR_FOREGROUND);
    drawCenteredText("Appuyez sur", 240, 2, COLOR_HIGHLIGHT);
    drawCenteredText("le joystick", 262, 2, COLOR_HIGHLIGHT);
}

static void drawGameOverScreen(const Game& game) {
    tft.fillScreen(COLOR_BACKGROUND);

    const char* winner;
    if (game.bottom.score > game.top.score) {
        winner = "JOUEUR 1 GAGNE";
    } else if (game.topIsHuman) {
        winner = "JOUEUR 2 GAGNE";
    } else {
        winner = "L'ORDINATEUR GAGNE";
    }
    drawCenteredText(winner, 80, 2, COLOR_FOREGROUND);

    char score[16];
    snprintf(score, sizeof(score), "%d - %d", game.top.score, game.bottom.score);
    drawCenteredText(score, 130, 4, COLOR_FOREGROUND);

    drawCenteredText("Appuyez sur", 240, 2, COLOR_HIGHLIGHT);
    drawCenteredText("le joystick", 262, 2, COLOR_HIGHLIGHT);
}

static void drawPlayingScreen(const Game& game) {
    tft.fillScreen(COLOR_BACKGROUND);
    drawScoreBar(game);
    drawNet();

    drawnBottomX = (int)game.bottom.x;
    drawnTopX    = (int)game.top.x;
    drawnBallX   = (int)game.ball.x;
    drawnBallY   = (int)game.ball.y;

    tft.fillRect(drawnBottomX, game.bottom.y, PADDLE_WIDTH, PADDLE_HEIGHT, COLOR_FOREGROUND);
    tft.fillRect(drawnTopX,    game.top.y,    PADDLE_WIDTH, PADDLE_HEIGHT, COLOR_FOREGROUND);
    tft.fillRect(drawnBallX,   drawnBallY,    BALL_SIZE,    BALL_SIZE,     COLOR_FOREGROUND);
}

// Met à jour uniquement ce qui a bougé pendant la partie
static void updatePlayingScreen(const Game& game) {
    int bottomX = (int)game.bottom.x;
    int topX    = (int)game.top.x;

    movePaddle(game.bottom.y, drawnBottomX, bottomX);
    movePaddle(game.top.y,    drawnTopX,    topX);
    drawnBottomX = bottomX;
    drawnTopX    = topX;

    moveBall(game, (int)game.ball.x, (int)game.ball.y);

    if (game.bottom.score != drawnBottomScore ||
        game.top.score    != drawnTopScore    ||
        game.topIsHuman   != drawnTopIsHuman) {
        drawScoreBar(game);
    }
}

// -----------------------------------------------------------------------------
//  Fonctions publiques
// -----------------------------------------------------------------------------

void rendererBegin() {
    // Les broches SPI de la carte ne sont pas celles par défaut de la XIAO :
    // on les indique explicitement (pas de MISO : l'écran ne renvoie rien).
    SPI.begin(PIN_TFT_SCK, -1, PIN_TFT_MOSI, PIN_TFT_CS);

    // Comme le CS de l'écran est toujours actif, on utilise le mode SPI 3
    // (horloge au repos à l'état haut) : c'est le mode recommandé pour les
    // écrans ST7789 sans CS.
    tft.init(240, 320, SPI_MODE3);
    tft.setSPISpeed(TFT_SPI_FREQUENCY);
    tft.setRotation(SCREEN_ROTATION);
    tft.fillScreen(COLOR_BACKGROUND);
}

void rendererDraw(const Game& game) {
    // Changement d'écran : on redessine tout
    if (!screenIsValid || game.state != drawnState) {
        switch (game.state) {
            case GameState::Title:    drawTitleScreen();        break;
            case GameState::Playing:  drawPlayingScreen(game);  break;
            case GameState::GameOver: drawGameOverScreen(game); break;
        }
        drawnState    = game.state;
        screenIsValid = true;
        return;
    }

    // Même écran : seule la partie en cours a besoin d'être mise à jour
    if (game.state == GameState::Playing) {
        updatePlayingScreen(game);
    }
}
