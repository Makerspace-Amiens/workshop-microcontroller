// =============================================================================
//  game.cpp - Règles du jeu : déplacements, collisions, score
// =============================================================================
#include <Arduino.h>
#include "game.h"
#include "config.h"

// -----------------------------------------------------------------------------
//  Fonctions utilitaires
// -----------------------------------------------------------------------------

// Position horizontale qui centre la raquette dans le terrain
static float paddleCenterX() {
    return (FIELD_LEFT + FIELD_RIGHT - PADDLE_WIDTH) / 2.0f;
}

// Empêche la raquette de sortir du terrain
static void clampPaddle(Paddle& paddle) {
    paddle.x = constrain(paddle.x, (float)FIELD_LEFT, (float)(FIELD_RIGHT - PADDLE_WIDTH));
}

// Vrai si le rectangle de la balle chevauche celui de la raquette
static bool ballTouchesPaddle(const Ball& ball, const Paddle& paddle) {
    return ball.x < paddle.x + PADDLE_WIDTH  && ball.x + BALL_SIZE > paddle.x &&
           ball.y < paddle.y + PADDLE_HEIGHT && ball.y + BALL_SIZE > paddle.y;
}

// Place la balle au centre et programme son lancement vers le haut
// (direction = -1) ou vers le bas (direction = +1)
static void serveBall(Game& game, int direction, uint32_t nowMs) {
    Ball& ball = game.ball;

    ball.x     = (FIELD_LEFT + FIELD_RIGHT - BALL_SIZE) / 2.0f;
    ball.y     = (FIELD_TOP + FIELD_BOTTOM - BALL_SIZE) / 2.0f;
    ball.speed = BALL_START_SPEED;
    ball.vx    = random(-150, 151) / 100.0f;  // Légère inclinaison aléatoire
    ball.vy    = direction * BALL_START_SPEED;

    game.serveTimeMs = nowMs + SERVE_DELAY_MS;
}

static void startMatch(Game& game, uint32_t nowMs) {
    game.bottom.score = 0;
    game.top.score    = 0;
    game.bottom.x     = paddleCenterX();
    game.top.x        = paddleCenterX();
    game.state        = GameState::Playing;

    serveBall(game, random(2) ? 1 : -1, nowMs);
}

// -----------------------------------------------------------------------------
//  Raquettes
// -----------------------------------------------------------------------------

// Raquette du bas : sa vitesse est proportionnelle à l'inclinaison du joystick
static void moveBottomPaddle(Game& game, const InputState& input) {
    game.bottom.x += input.joystickX * PADDLE_MAX_SPEED;
    clampPaddle(game.bottom);
}

// L'ordinateur suit la balle quand elle monte vers lui, sinon il revient au
// centre. Sa vitesse est limitée pour qu'il reste battable.
static void moveAiPaddle(Game& game) {
    float target;
    if (game.ball.vy < 0) {
        target = game.ball.x + BALL_SIZE / 2.0f - PADDLE_WIDTH / 2.0f;
    } else {
        target = paddleCenterX();
    }

    float step = constrain(target - game.top.x, -AI_MAX_SPEED, AI_MAX_SPEED);
    game.top.x += step;
}

// Raquette du haut : jouée par l'ordinateur jusqu'à ce qu'un deuxième joueur
// appuie sur SW1 ou SW2
static void moveTopPaddle(Game& game, const InputState& input) {
    if (input.buttonLeft || input.buttonRight) {
        game.topIsHuman = true;
    }

    if (game.topIsHuman) {
        int direction = (int)input.buttonRight - (int)input.buttonLeft;  // -1, 0 ou +1
        game.top.x += direction * PADDLE_MAX_SPEED;
    } else {
        moveAiPaddle(game);
    }

    clampPaddle(game.top);
}

// -----------------------------------------------------------------------------
//  Balle
// -----------------------------------------------------------------------------

// Renvoie la balle depuis une raquette. L'angle de rebond dépend de l'endroit
// où la balle touche : au centre elle repart tout droit, sur les bords elle
// repart en biais (jusqu'à BALL_MAX_ANGLE).
static void bounceOnPaddle(Ball& ball, const Paddle& paddle, int direction) {
    float ballCenter   = ball.x + BALL_SIZE / 2.0f;
    float paddleCenter = paddle.x + PADDLE_WIDTH / 2.0f;

    // Position de l'impact : -1 = bord gauche de la raquette, +1 = bord droit
    float hitPosition = (ballCenter - paddleCenter) / (PADDLE_WIDTH / 2.0f);
    hitPosition = constrain(hitPosition, -1.0f, 1.0f);

    float angle = radians(hitPosition * BALL_MAX_ANGLE);

    ball.speed = min(ball.speed + BALL_SPEED_UP, BALL_MAX_SPEED);
    ball.vx    = ball.speed * sinf(angle);
    ball.vy    = direction * ball.speed * cosf(angle);
}

static void moveBall(Game& game) {
    Ball& ball = game.ball;

    ball.x += ball.vx;
    ball.y += ball.vy;

    // Rebond sur les murs de gauche et de droite
    if (ball.x < FIELD_LEFT) {
        ball.x  = FIELD_LEFT;
        ball.vx = -ball.vx;
    } else if (ball.x + BALL_SIZE > FIELD_RIGHT) {
        ball.x  = FIELD_RIGHT - BALL_SIZE;
        ball.vx = -ball.vx;
    }

    // Rebond sur les raquettes. On vérifie le sens de la balle pour ne pas la
    // renvoyer deux fois de suite si elle touche encore la raquette.
    if (ball.vy > 0 && ballTouchesPaddle(ball, game.bottom)) {
        ball.y = game.bottom.y - BALL_SIZE;
        bounceOnPaddle(ball, game.bottom, -1);
    } else if (ball.vy < 0 && ballTouchesPaddle(ball, game.top)) {
        ball.y = game.top.y + PADDLE_HEIGHT;
        bounceOnPaddle(ball, game.top, +1);
    }
}

// Vérifie si la balle a atteint le haut ou le bas du terrain et met à jour
// le score
static void checkPointScored(Game& game, uint32_t nowMs) {
    const Ball& ball = game.ball;
    int serveDirection;

    if (ball.y < FIELD_TOP) {
        game.bottom.score++;
        serveDirection = -1;  // La balle repart vers celui qui a perdu le point
    } else if (ball.y + BALL_SIZE > FIELD_BOTTOM) {
        game.top.score++;
        serveDirection = +1;
    } else {
        return;  // Balle toujours en jeu
    }

    if (game.bottom.score >= WINNING_SCORE || game.top.score >= WINNING_SCORE) {
        game.state = GameState::GameOver;
    } else {
        serveBall(game, serveDirection, nowMs);
    }
}

// -----------------------------------------------------------------------------
//  Fonctions publiques
// -----------------------------------------------------------------------------

void gameInit(Game& game) {
    game.state      = GameState::Title;
    game.bottom     = { paddleCenterX(), FIELD_BOTTOM - PADDLE_MARGIN - PADDLE_HEIGHT, 0 };
    game.top        = { paddleCenterX(), FIELD_TOP + PADDLE_MARGIN, 0 };
    game.topIsHuman = false;
    serveBall(game, 1, 0);
}

void gameUpdate(Game& game, const InputState& input, uint32_t nowMs) {
    switch (game.state) {
        case GameState::Title:
            if (input.startPressed) {
                startMatch(game, nowMs);
            }
            break;

        case GameState::Playing:
            moveBottomPaddle(game, input);
            moveTopPaddle(game, input);

            // Pendant la pause avant le service, seules les raquettes bougent
            if (nowMs >= game.serveTimeMs) {
                moveBall(game);
                checkPointScored(game, nowMs);
            }
            break;

        case GameState::GameOver:
            if (input.startPressed) {
                gameInit(game);
            }
            break;
    }
}
