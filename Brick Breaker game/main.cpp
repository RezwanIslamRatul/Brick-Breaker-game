#include <GL/glut.h>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <string>
#include <algorithm>


const int windowWidth = 800;
const int windowHeight = 600;
const int brickRows = 5;
const int brickCols = 10;
const int brickWidth = 70;
const int brickHeight = 20;
int paddleWidth = 100;
const int paddleHeight = 20;
const int maxBalls = 8;  // increased from 3 to 8
const float NORMAL_SPEED = 0.2f;
const float FAST_SPEED = 0.5f;
bool gameWon = false;

const int PADDLE_WIDTH_NORMAL = 100;
const float PADDLE_POWERUP_DURATION = 20000.0f; // 20 seconds in milliseconds

struct Color {
    float r, g, b;
};

struct Ball {
    float x, y;
    float dx, dy;
    Color color;
    bool active;
};

struct Brick {
    float x, y;
    bool visible;
    Color color;
    bool hasPower;
};

struct PowerUp {
    float x, y;
    bool active;
    int type;
    float speed;
    float size;
};

float paddleX = windowWidth / 2 - paddleWidth / 2;
float paddleY = 50;
int score = 0;
bool gameOver = false;
bool moveLeft = false, moveRight = false;
bool fastMoveLeft = false, fastMoveRight = false;
float paddleWidthTimer = 0.0f;
bool paddleWidthPowerUpActive = false;

std::vector<Ball> balls;
std::vector<Brick> bricks;
std::vector<PowerUp> powerUps;

Color colors[] = { {1, 0, 0}, {0, 1, 0} }; // Only 2 colors: Red and Green

bool colorsMatch(const Color& c1, const Color& c2) {
    const float eps = 0.01f;
    return (fabs(c1.r - c2.r) < eps) && (fabs(c1.g - c2.g) < eps) && (fabs(c1.b - c2.b) < eps);
}

void drawText(float x, float y, const std::string& text) {
    glRasterPos2f(x, y);
    for (char c : text) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
}

void drawRect(float x, float y, float w, float h, Color c) {
    glColor3f(c.r, c.g, c.b);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void resetGame() {
    gameWon = false;
    balls.clear();
    powerUps.clear();
    balls.push_back({windowWidth / 2.0f, windowHeight / 2.0f, 4.0f, 4.0f, colors[rand() % 2], true});
    paddleX = windowWidth / 2 - paddleWidth / 2;
    score = 0;
    gameOver = false;
    bricks.clear();
    paddleWidth = PADDLE_WIDTH_NORMAL;  // Reset paddle to normal width
    paddleWidthPowerUpActive = false;
    paddleWidthTimer = 0.0f;

    for (int i = 0; i < brickRows; ++i) {
        std::vector<Brick> rowBricks;
        int redCount = 0;
        int greenCount = 0;


        for (int j = 0; j < brickCols; ++j) {
            Brick b;
            b.x = j * (brickWidth + 15) + 35;
            b.y = windowHeight - (i + 1) * (brickHeight + 15);
            b.visible = true;
            b.hasPower = true;

            b.color = colors[rand() % 2];
            if (b.color.r > 0.5f) redCount++;
            else greenCount++;

            rowBricks.push_back(b);
        }


        while (redCount < brickCols * 0.4 || greenCount < brickCols * 0.4) {
            int j = rand() % brickCols;
            if (redCount < brickCols * 0.4 && rowBricks[j].color.g > 0.5f) {
                rowBricks[j].color = colors[0];  // Make it red
                redCount++;
                greenCount--;
            }
            else if (greenCount < brickCols * 0.4 && rowBricks[j].color.r > 0.5f) {
                rowBricks[j].color = colors[1];  // Make it green
                greenCount++;
                redCount--;
            }
        }

        // Add row bricks to main vector
        bricks.insert(bricks.end(), rowBricks.begin(), rowBricks.end());
    }
}

void spawnPowerUp(float x, float y) {

    if (rand() % 100 < 40) {
        PowerUp p;
        p.x = x;
        p.y = y;
        p.active = true;
        p.type = rand() % 3;  // 0: speed, 1: multi-ball, 2: wider paddle
        p.speed = 3.0f;
        p.size = 20.0f;
        powerUps.push_back(p);
    }
}

void splitBalls() {
    if (balls.empty() || balls.size() >= maxBalls) return;

    std::vector<Ball> newBalls;
    for (const auto& ball : balls) {
        if (ball.active && newBalls.size() + balls.size() < maxBalls) {
            // Create copy with slightly different angle
            Ball newBall = ball;
            newBall.dx = -ball.dx;  // Reverse x direction
            newBall.dy = ball.dy;   // Keep same y direction
            newBalls.push_back(newBall);
        }
    }
    balls.insert(balls.end(), newBalls.begin(), newBalls.end());
}

void changeBallColor(Ball& ball) {
    ball.color = colors[rand() % 2];
}

void applyPowerUp(int type) {
    switch (type) {
        case 0: // Speed boost
            for (auto& ball : balls) {
                if (ball.active) {
                    // Increase speed by 30%
                    float currentSpeed = sqrt(ball.dx * ball.dx + ball.dy * ball.dy);
                    float speedMultiplier = 1.3f;
                    ball.dx = (ball.dx / currentSpeed) * (currentSpeed * speedMultiplier);
                    ball.dy = (ball.dy / currentSpeed) * (currentSpeed * speedMultiplier);
                }
            }
            break;
        case 1: // Multi-ball
            if (balls.size() * 2 <= maxBalls) {
                splitBalls();
            }
            break;
        case 2: // Wider paddle
            paddleWidth = std::min(paddleWidth + 50, 200);
            paddleWidthPowerUpActive = true;
            paddleWidthTimer = glutGet(GLUT_ELAPSED_TIME);
            break;
    }
}

void update(int value) {
    if (gameOver) {
        paddleWidth = PADDLE_WIDTH_NORMAL;  // Reset paddle width when game is over
        return;
    }

    // Handle paddle width power-up timer
    if (paddleWidthPowerUpActive) {
        float currentTime = glutGet(GLUT_ELAPSED_TIME);
        if (currentTime - paddleWidthTimer >= PADDLE_POWERUP_DURATION) {
            paddleWidth = PADDLE_WIDTH_NORMAL;
            paddleWidthPowerUpActive = false;
        }
    }

    bool brickBroken = false;

    for (auto& ball : balls) {
        if (!ball.active) continue;

        // Store previous position
        float prevX = ball.x;
        float prevY = ball.y;

        // Update position
        ball.x += ball.dx;
        ball.y += ball.dy;

        // Improved wall collision detection
        const float ballRadius = 10.0f;
        if (ball.x - ballRadius <= 0) {
            ball.x = ballRadius;
            ball.dx = fabs(ball.dx);  // Ensure positive x velocity
        }
        if (ball.x + ballRadius >= windowWidth) {
            ball.x = windowWidth - ballRadius;
            ball.dx = -fabs(ball.dx);  // Ensure negative x velocity
        }
        if (ball.y + ballRadius >= windowHeight) {
            ball.y = windowHeight - ballRadius;
            ball.dy = -fabs(ball.dy);  // Ensure negative y velocity
        }
        if (ball.y <= 0) {
            ball.active = false;
        }

        if (ball.y <= paddleY + paddleHeight && ball.y >= paddleY &&
            ball.x >= paddleX && ball.x <= paddleX + paddleWidth) {
            ball.dy = fabs(ball.dy);
            // Add slight angle change based on where the ball hits the paddle
            float hitPoint = (ball.x - paddleX) / paddleWidth;
            ball.dx = ball.dx * 0.8f + (hitPoint - 0.5f) * 4.0f;
        }


        for (auto& brick : bricks) {
            if (!brick.visible) continue;

            // Improved collision detection
            if (ball.x + ballRadius >= brick.x && ball.x - ballRadius <= brick.x + brickWidth &&
                ball.y + ballRadius >= brick.y && ball.y - ballRadius <= brick.y + brickHeight) {

                // Determine which side of the brick was hit by comparing previous position
                bool hitVertical = (prevX + ballRadius < brick.x || prevX - ballRadius > brick.x + brickWidth);
                bool hitHorizontal = (prevY + ballRadius < brick.y || prevY - ballRadius > brick.y + brickHeight);

                if (hitVertical) {
                    ball.dx *= -1;
                    // Move ball outside brick
                    if (prevX < brick.x) ball.x = brick.x - ballRadius;
                    else ball.x = brick.x + brickWidth + ballRadius;
                }
                if (hitHorizontal) {
                    ball.dy *= -1;
                    // Move ball outside brick
                    if (prevY < brick.y) ball.y = brick.y - ballRadius;
                    else ball.y = brick.y + brickHeight + ballRadius;
                }

                if (colorsMatch(ball.color, brick.color)) {
                    brick.visible = false;
                    score += 10;
                    brickBroken = true;
                    if (brick.hasPower) spawnPowerUp(brick.x + brickWidth / 2, brick.y);
                }
                bool allBricksDestroyed = std::all_of(bricks.begin(), bricks.end(),
                                      [](const Brick& b) { return !b.visible; });

if (allBricksDestroyed) {
    gameWon = true;
    gameOver = true; // stop the game loop
}
                break;
            }
        }
    }

    if (brickBroken) {
        for (auto& ball : balls) {
            if (ball.active) {
                changeBallColor(ball);
            }
        }
    }

    // Update power-ups with improved collision detection
    for (auto& power : powerUps) {
        if (!power.active) continue;

        power.y -= power.speed;

        // Check collision with paddle
        if (power.y <= paddleY + paddleHeight &&
            power.y >= paddleY - power.size &&
            power.x >= paddleX - power.size &&
            power.x <= paddleX + paddleWidth + power.size) {

            power.active = false;
            applyPowerUp(power.type);
        }

        // Remove if falls below screen
        if (power.y < 0) {
            power.active = false;
        }
    }

    // Remove inactive power-ups
    powerUps.erase(
        std::remove_if(powerUps.begin(), powerUps.end(),
            [](const PowerUp& p) { return !p.active; }),
        powerUps.end()
    );

    bool anyActive = false;
    for (auto& ball : balls) if (ball.active) anyActive = true;
    if (!anyActive) gameOver = true;

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    drawRect(paddleX, paddleY, paddleWidth, paddleHeight, {0.3f, 0.3f, 0.3f});

    for (auto& brick : bricks) {
        if (brick.visible)
            drawRect(brick.x, brick.y, brickWidth, brickHeight, brick.color);
    }

    for (auto& ball : balls) {
        if (!ball.active) continue;
        glColor3f(ball.color.r, ball.color.g, ball.color.b);
        glBegin(GL_POLYGON);
        for (int i = 0; i < 360; i += 20) {
            float rad = i * 3.14159f / 180;
            glVertex2f(ball.x + cos(rad) * 10, ball.y + sin(rad) * 10);
        }
        glEnd();
    }

    // Draw power-ups with improved visibility
    for (const auto& power : powerUps) {
        if (!power.active) continue;

        Color powerColor;
        switch (power.type) {
            case 0: powerColor = {1.0f, 0.0f, 0.0f}; break;  // Red for speed
            case 1: powerColor = {0.0f, 1.0f, 0.0f}; break;  // Green for multi-ball
            case 2: powerColor = {0.0f, 0.0f, 1.0f}; break;  // Blue for wider paddle
        }

        // Draw power-up symbol
        drawRect(power.x - power.size/2, power.y - power.size/2,
                power.size, power.size, powerColor);

        // Draw outline
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(power.x - power.size/2, power.y - power.size/2);
        glVertex2f(power.x + power.size/2, power.y - power.size/2);
        glVertex2f(power.x + power.size/2, power.y + power.size/2);
        glVertex2f(power.x - power.size/2, power.y + power.size/2);
        glEnd();
    }

    // Draw score in top-right corner with padding
    glColor3f(0, 0, 0);
    std::string scoreText = "Score: " + std::to_string(score);
    drawText(windowWidth - 120, windowHeight - 30, scoreText);

    if (gameOver) {
        // Center the game over text
        std::string gameOverText = "Game Over! Press R to restart.";
        drawText(windowWidth/2 - 100, windowHeight/2, gameOverText);
    }
    if (gameWon) {
    drawText(windowWidth/2 - 100, windowHeight/2 + 30, "Congratulations! You Win!");
}


    glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 'a') fastMoveLeft = true;
    if (key == 'd') fastMoveRight = true;
    if (key == 'r' && gameOver) {
        resetGame();
        glutTimerFunc(0, update, 0);
    }
    glutPostRedisplay();
}

void keyboardUp(unsigned char key, int x, int y) {
    if (key == 'a') fastMoveLeft = false;
    if (key == 'd') fastMoveRight = false;
}

void idle() {
    // Handle fast movement (A/D keys)
    if (fastMoveLeft) paddleX -= FAST_SPEED;
    if (fastMoveRight) paddleX += FAST_SPEED;

    // Handle normal movement (arrow keys)
    if (moveLeft && !fastMoveLeft) paddleX -= NORMAL_SPEED;
    if (moveRight && !fastMoveRight) paddleX += NORMAL_SPEED;

    if (paddleX < 0) paddleX = 0;
    if (paddleX + paddleWidth > windowWidth) paddleX = windowWidth - paddleWidth;

    glutPostRedisplay();
}

void init() {
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, windowWidth, 0.0, windowHeight);
    srand((unsigned int)time(0));
    resetGame();
}
void specialKeys(int key, int x, int y);
void specialKeysUp(int key, int x, int y);
void specialKeys(int key, int x, int y) {
    if (key == GLUT_KEY_LEFT) moveLeft = true;
    if (key == GLUT_KEY_RIGHT) moveRight = true;
}

void specialKeysUp(int key, int x, int y) {
    if (key == GLUT_KEY_LEFT) moveLeft = false;
    if (key == GLUT_KEY_RIGHT) moveRight = false;
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow(" Color Match Brick Breaker");
    init();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialKeys);
    glutSpecialUpFunc(specialKeysUp);

    glutIdleFunc(idle);
    glutTimerFunc(0, update, 0);
    glutMainLoop();
    return 0;
}
