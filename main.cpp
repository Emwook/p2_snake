#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>
#include <time.h>
#include <stdlib.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#define SQUARE_SIZE 16
#define SPEED_FIX 100
#define MAX_SNAKE_LENGTH 20
#define MIN_SNAKE_LENGTH 3
#define DEFAULT_SNAKE_SPEED 1
#define DEFAULT_SNAKE_LENGTH 3
#define MIN_SPAWN_INTERVAL 5.0f
#define MAX_SPAWN_INTERVAL 10.0f
#define DISPLAY_DURATION 10.0f
#define SHORTEN_LENGTH 1
#define SLOWDOWN_FACTOR 0.3f
#define PROGRESS_BAR_WIDTH 16
#define PROGRESS_BAR_HEIGHT 100
#define RED_DOT_BONUS 150
#define BLUE_DOT_BONUS 100
#define SPEED_UP_TIME 3
#define SPEED_UP_AMOUNT 0.1
#define BUFFER_SIZE 128
#define SAVE_FILENAME "game_state.txt"
#define SCORE_FILENAME "scoreboard.txt"
#define LINE_HEIGHT 20
#define SCOREBOARD_AMOUNT 3
#define TEXT_MARGIN 8
#define DEFAULT_DIRECTION RIGHT
#define SPRITE_SIZE 8

#define SCREEN_WIDTH  (45 * SQUARE_SIZE)
#define SCREEN_HEIGHT (45 * SQUARE_SIZE)
#define MARGIN (5 * SQUARE_SIZE)
#define WIDTH (SCREEN_WIDTH - 2*MARGIN)
#define HEIGHT (SCREEN_HEIGHT - 2*MARGIN)
#define STATS_HEIGHT (4*SQUARE_SIZE)
#define STATS_BOTTOM_X (SCREEN_WIDTH - TEXT_MARGIN)
#define STATS_BOTTOM_Y (SCREEN_HEIGHT - STATS_HEIGHT - TEXT_MARGIN)

typedef enum {
    UP,
    RIGHT,
    DOWN,
    LEFT,
} directions_t;

typedef enum {
    JUSTIFY_LEFT,
    JUSTIFY_RIGHT,
    JUSTIFY_CENTER,
} justify_t;

typedef enum {
    GAME_RUNNING,
    GAME_OVER,
    ADD_SCORE,
} game_state_t;

typedef enum {
    SNAKE_HEAD,
    SNAKE_BODY_BIG,
    SNAKE_BODY_SMALL,
    SNAKE_TAIL,
} snake_part_t;



typedef struct {
    int x;
    int y;
} position_t;

typedef struct {
    position_t body[MAX_SNAKE_LENGTH];
    int length;
    directions_t direction[MAX_SNAKE_LENGTH];
    double speed;
    int score;
    char playerName[BUFFER_SIZE];
    int quit;
} snake_t;

typedef struct {
    position_t position;
    float spawnTime;
    float displayDuration;
    int displayMode;
    int isActive;
} red_dot_t;

typedef struct {
    red_dot_t redDot;
    snake_t snake;
    position_t blueDotPos;
} game_elements_t;

typedef struct {
    float timeAccumulator;
    float delta;
    double nextIncrease;
    double worldTime;
    int frames;
    double t1;
    double t2;
} time_variables_t;

typedef struct {
    char name[BUFFER_SIZE];
    int score;
} score_entry_t;

typedef struct {
    int black;
    int green;
    int red;
    int blue;
    int white;
    int grey;
} colors_t;

typedef struct{
    SDL_Event event;
    SDL_Surface *screen;
    SDL_Texture *charset;
    SDL_Rect srcRect;
    SDL_Texture *scrtex;
    SDL_Window  *window;
    SDL_Renderer *renderer;
    SDL_Surface *cr;
} graphics_t;


void randomizePosition(game_elements_t& gameElems);
void spawnRedDot(time_variables_t timeVars, game_elements_t& gameElems);
void updateScore(int& score, char a);
void saveGameState(time_variables_t timeVars, game_elements_t gameElems);
void loadGameState(time_variables_t& timeVars, game_elements_t& gameElems);
void updateScoreboard(snake_t snake);
void drawTopStats(colors_t colors, graphics_t graphics,time_variables_t timeVars, char* text, snake_t snake);

void DrawSpriteScaled(graphics_t graphics, position_t dest, position_t sprite, int scale, int color) {
    SDL_Rect srcRect;
    srcRect.x = sprite.x * SPRITE_SIZE;
    srcRect.y = sprite.y * SPRITE_SIZE;
    srcRect.w = 8;
    srcRect.h = 8;

    SDL_Rect destRect;
    destRect.x = dest.x - SQUARE_SIZE / 2;
    destRect.y = dest.y - SQUARE_SIZE / 2;
    destRect.w = (SPRITE_SIZE / 2) * scale;
    destRect.h = (SPRITE_SIZE / 2) * scale;


    SDL_Surface* temp = SDL_CreateRGBSurface(0, destRect.w, destRect.h, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    Uint8 r, g, b;
    SDL_GetRGB(color, graphics.screen->format, &r, &g, &b);
    SDL_SetSurfaceColorMod(graphics.cr, r, g, b);

    SDL_Rect scaledRect;
    scaledRect.x = 0;
    scaledRect.y = 0;
    scaledRect.w = destRect.w;
    scaledRect.h = destRect.h;

    int result = SDL_BlitScaled(graphics.cr, &srcRect, temp, &scaledRect);
    result = SDL_BlitSurface(temp, NULL, graphics.screen, &destRect);
    SDL_FreeSurface(temp);
    SDL_SetSurfaceColorMod(graphics.cr, 255, 255, 255);
}

int readScoreboard(score_entry_t* entries) {
    FILE* file = fopen(SCORE_FILENAME, "r");
    if (!file) {
        perror("Failed to open file");
        return 0;
    }

    int count = 0;
    while (count < SCOREBOARD_AMOUNT && fscanf(file, "%49s %d", entries[count].name, &entries[count].score) == 2) {
        count++;
    }

    fclose(file);
    return count;
}

void updateScoreboard(snake_t snake) {
    score_entry_t entries[SCOREBOARD_AMOUNT];
    int count = readScoreboard(entries);
    int found = 0;
    for (int i = 0; i < count; i++) {
        if (strcmp(entries[i].name, snake.playerName) == 0 ) {
            if(snake.score > entries[i].score){
                entries[i].score = snake.score;
            }
            found = 1;
            break;
        }
    }
    if (!found) {
        if (count < SCOREBOARD_AMOUNT) {
            strncpy(entries[count].name, snake.playerName, BUFFER_SIZE - 1);
            entries[count].name[BUFFER_SIZE - 1] = '\0';
            entries[count].score = snake.score;
            count++;
        } else {
            int lowestIndex = 0;
            for (int i = 1; i < count; i++) {
                if (entries[i].score < entries[lowestIndex].score) {
                    lowestIndex = i;
                }
            }
            if (snake.score > entries[lowestIndex].score) {
                strncpy(entries[lowestIndex].name, snake.playerName, BUFFER_SIZE - 1);
                entries[lowestIndex].name[BUFFER_SIZE - 1] = '\0';
                entries[lowestIndex].score = snake.score;
            }
        }
    }



     FILE* file = fopen(SCORE_FILENAME, "w");
    if (!file) {
        perror("Failed to open file");
        return;
    }

    for (int i = 0; i < count; i++) {
        fprintf(file, "%s %d\n", entries[i].name, entries[i].score);
    }

    fclose(file);
}

void DrawSnakePart(graphics_t graphics, position_t pos, snake_part_t type, directions_t direction, colors_t colors) {
    int spriteY = 9;
    
    switch (direction) {
        case UP:
            spriteY = 8;
            break;
        case RIGHT:
            spriteY = 9;
            break;
        case DOWN:
            spriteY = 10;
            break;
        case LEFT:
            spriteY = 11;
            break;
    }
    position_t sprite;
    sprite.y = spriteY;
    sprite.x = type;


    DrawSpriteScaled(graphics,pos, sprite, 4, colors.green);
}

void DrawString(graphics_t graphics, int x, int y, const char *text, int maxWidth, justify_t justify) {
    int px, py, c;
    SDL_Rect s, d;
    s.w = 8;
    s.h = 8;
    d.w = 8;
    d.h = 8;

    int textWidth = strlen(text) * 8;

    if (justify == JUSTIFY_RIGHT) {
        x = x + maxWidth - textWidth;
    } else if (justify == JUSTIFY_CENTER) {
        x = x + (maxWidth - textWidth) / 2;
    }

    while (*text) {
        c = *text & 255;
        px = (c % 16) * 8;
        py = (c / 16) * 8;
        s.x = px;
        s.y = py;
        d.x = x;
        d.y = y;
        SDL_BlitSurface(graphics.cr, &s, graphics.screen, &d);
        x += 8;
        text++;
    }
}

void DrawPixel(SDL_Surface *surface, int x, int y, Uint32 color) {
	int bpp = surface->format->BytesPerPixel;
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32 *)p = color;
	};

void DrawLine(graphics_t graphics, int x, int y, int l, int dx, int dy, Uint32 color) {
	for(int i = 0; i < l; i++) {
		DrawPixel(graphics.screen, x, y, color);
		x += dx;
		y += dy;
		};
	};

void DrawRectangle(graphics_t& graphics, int x, int y, int l, int k, Uint32 outlineColor, Uint32 fillColor) {
	int i;
	DrawLine(graphics, x, y, k, 0, 1, outlineColor);
	DrawLine(graphics, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(graphics, x, y, l, 1, 0, outlineColor);
	DrawLine(graphics, x, y + k - 1, l, 1, 0, outlineColor);
	for(i = y + 1; i < y + k - 1; i++)
		DrawLine(graphics, x + 1, i, l - 2, 1, 0, fillColor);
	};

void ResetGame(time_variables_t& timeVars, game_elements_t& gameElems) {
    gameElems.snake.length = MIN_SNAKE_LENGTH;
	for(int i = 0; i<gameElems.snake.length; i++){
		gameElems.snake.body[i].x = SCREEN_WIDTH / 2 - (i *SQUARE_SIZE);
    	gameElems.snake.body[i].y  = SCREEN_HEIGHT / 2;
        gameElems.snake.direction[i] = DEFAULT_DIRECTION;
	}
	gameElems.snake.speed = 1;
	gameElems.snake.direction[0] = RIGHT;
    timeVars.worldTime = 0;
    timeVars.timeAccumulator = 0;

    randomizePosition(gameElems);
    gameElems.redDot.isActive = 0;
    spawnRedDot(timeVars, gameElems); 
    gameElems.snake.score = 0;
}
int isValidMove(position_t nextPos) {
    int halfSquare = SQUARE_SIZE / 2;
    int nextX = nextPos.x;
    int nextY = nextPos.y;
    return (nextX - halfSquare >= MARGIN &&
            nextX + halfSquare <= MARGIN + WIDTH &&
            nextY - halfSquare >= MARGIN &&
            nextY + halfSquare <= MARGIN + HEIGHT);
}

void autoTurn(snake_t &snake) {
    int step = SQUARE_SIZE;
	int rightTurn = (snake.direction[0] + 1) % 4;
    int leftTurn = (snake.direction[0] + 3) % 4;
    
    position_t rightPos;
    rightPos.x = snake.body[0].x;
    rightPos.y = snake.body[0].y;

    switch(rightTurn) {
        case 0:
            rightPos.y -= step;
            break;
        case 1:
            rightPos.x += step;
            break;
        case 2:
            rightPos.y += step;
            break;
        case 3:
            rightPos.x -= step;
            break;
    }
    
   position_t leftPos;
    leftPos.x = snake.body[0].x;
    leftPos.y = snake.body[0].y;

    switch(leftTurn) {
        case 0:
            leftPos.y -= step;
            break;
        case 1:
            leftPos.x += step;
            break;
        case 2:
            leftPos.y += step;
            break;
        case 3:
            leftPos.x -= step;
            break;
    }
    
    if (isValidMove(rightPos)) {
        snake.direction[0] = (directions_t) rightTurn;
        return;
    }
    
    if (isValidMove(leftPos)) {
        snake.direction[0] = (directions_t) leftTurn;
        return;
    }
}

void drawGrid(graphics_t graphics, Uint32 color) {
    for (int x = MARGIN; x < MARGIN + WIDTH; x += SQUARE_SIZE) {
        DrawLine(graphics, x, MARGIN, HEIGHT, 0, 1, color);
    }
    
    for (int y = MARGIN; y < MARGIN + HEIGHT; y += SQUARE_SIZE) {
        DrawLine(graphics, MARGIN, y, WIDTH, 1, 0, color);
    }
}

void UpdateSnake(game_elements_t& gameElems) {
    for (int i = gameElems.snake.length - 1; i > 0; --i) {
        gameElems.snake.body[i] = gameElems.snake.body[i - 1];
        gameElems.snake.direction[i] = gameElems.snake.direction[i-1];
    }

    if (gameElems.snake.direction[0] == UP) {
        gameElems.snake.body[0].y -= SQUARE_SIZE;
    } 
    else if (gameElems.snake.direction[0] == RIGHT) {
        gameElems.snake.body[0].x += SQUARE_SIZE;
    } 
    else if (gameElems.snake.direction[0] == DOWN) {
        gameElems.snake.body[0].y += SQUARE_SIZE;
    } 
    else if (gameElems.snake.direction[0] == LEFT) {
        gameElems.snake.body[0].x -= SQUARE_SIZE;
    }
    gameElems.redDot.displayMode = (gameElems.redDot.displayMode + 1)%4;
}

int checkSnakeCollision(snake_t* snake, int nextX, int nextY) {
    for (int i = 1; i < snake->length - 1; i++) {
        if (nextX == snake->body[i].x && nextY == snake->body[i].y) {
            return 1;
        }
    }
	return 0;
}

void randomizePosition(game_elements_t& gameElems) {
    int validPosition;
    int i;

    do {
        gameElems.blueDotPos.x = MARGIN + (rand() % (WIDTH / SQUARE_SIZE)) * SQUARE_SIZE + SQUARE_SIZE / 2;
        gameElems.blueDotPos.y = MARGIN + (rand() % (HEIGHT / SQUARE_SIZE)) * SQUARE_SIZE + SQUARE_SIZE / 2;

        validPosition = 1;
        if(gameElems.blueDotPos.x != gameElems.redDot.position.x && gameElems.blueDotPos.y != gameElems.redDot.position.y) {
            for (i = 0; i < gameElems.snake.length; i++) {
                if (gameElems.blueDotPos.x == gameElems.snake.body[i].x && gameElems.blueDotPos.y == gameElems.snake.body[i].y) {
                    validPosition = 0;
                    break;
                }
            }
        }
    } while (!validPosition);
}

void checkBlueDot(position_t next, game_elements_t& gameElems) {
	if (next.x == gameElems.blueDotPos.x && next.y == gameElems.blueDotPos.y) {
        if( gameElems.snake.length < MAX_SNAKE_LENGTH){
		    gameElems.snake.length++;
        }
        updateScore(gameElems.snake.score, 'b');
		randomizePosition(gameElems);
	}
}

int isScoreGoodEnough(int score) {
    score_entry_t entries[SCOREBOARD_AMOUNT];
    int count = readScoreboard(entries);

    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (entries[j].score > entries[i].score) {
                score_entry_t temp = entries[i];
                entries[i] = entries[j];
                entries[j] = temp;
            }
        }
    }

    if (count < SCOREBOARD_AMOUNT) {
        return 1;
    }

    if (score > entries[count - 1].score) {
        return 1;
    }

    return 0;
}

void drawScoreboard (graphics_t graphics,int windowWidth, int windowX, int windowY) {
    char text[BUFFER_SIZE];
    snprintf(text, sizeof(text), "SCOREBOARD");
    DrawString(graphics, windowX + (windowWidth - strlen(text) * 8) / 2, windowY + 6*LINE_HEIGHT, text, SCREEN_WIDTH/2, JUSTIFY_LEFT);

    score_entry_t entries[SCOREBOARD_AMOUNT];
    int count = readScoreboard(entries);

    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (entries[j].score > entries[i].score) {
                score_entry_t temp = entries[i];
                entries[i] = entries[j];
                entries[j] = temp;
            }
        }
    }

    for (int i = 0; i < count && i < SCOREBOARD_AMOUNT; i++) {
        snprintf(text, sizeof(text), "%d. %s - %d", i + 1, entries[i].name, entries[i].score);
        DrawString(graphics, windowX + (windowWidth - strlen(text) * 8) / 2, 
                   windowY + LINE_HEIGHT * (i + 7), text, SCREEN_WIDTH/2, JUSTIFY_LEFT);
    }
}

void drawGameOver(graphics_t graphics, colors_t colors, snake_t snake) {
    int windowWidth = SCREEN_WIDTH / 2;
    int windowHeight = SCREEN_HEIGHT/ 2;
    int windowX = (SCREEN_WIDTH - windowWidth) / 2;
    int windowY = (SCREEN_HEIGHT - windowHeight)/ 2;
    
    DrawRectangle(graphics, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, colors.black, colors.black);
    DrawRectangle(graphics, windowX, windowY, windowWidth, windowHeight, colors.red, colors.black);
    
    char text[BUFFER_SIZE];
    snprintf(text, sizeof(text), "GAME OVER!");
    DrawString(graphics, windowX, windowY + LINE_HEIGHT, text, SCREEN_WIDTH/2, JUSTIFY_CENTER);

    snprintf(text, sizeof(text), "Final Score: %d", snake.score);
    DrawString(graphics, windowX, windowY + 2*LINE_HEIGHT, text, SCREEN_WIDTH/2, JUSTIFY_CENTER);

    drawScoreboard(graphics, windowWidth, windowX, windowY);
 
    snprintf(text, sizeof(text), "Press N for New Game");
    DrawString(graphics, windowX, windowY + 12*LINE_HEIGHT, text, SCREEN_WIDTH/2, JUSTIFY_CENTER);

    snprintf(text, sizeof(text), "Press I to load the last save");
    DrawString(graphics, windowX, windowY + 13*LINE_HEIGHT, text, SCREEN_WIDTH/2, JUSTIFY_CENTER);

    snprintf(text, sizeof(text), "Press ESC to Quit");
    DrawString(graphics, windowX, windowY + 15*LINE_HEIGHT, text, SCREEN_WIDTH/2, JUSTIFY_CENTER);
}

void handleGameOverInput(graphics_t graphics, game_state_t& gameState,  time_variables_t& timeVars, game_elements_t& gameElems ) {
    while(SDL_PollEvent(&graphics.event)) {
        switch(graphics.event.type) {
            case SDL_KEYDOWN:
                if(graphics.event.key.keysym.sym == SDLK_ESCAPE) {
                    gameElems.snake.quit = 1;
                }
                else if(graphics.event.key.keysym.sym == SDLK_n) {
                    ResetGame(timeVars, gameElems);
                    gameState = GAME_RUNNING;
                }
                else if(graphics.event.key.keysym.sym == SDLK_i) {
                    loadGameState(timeVars, gameElems);
                    gameState = GAME_RUNNING;
                }
                break;
            case SDL_QUIT:
                gameElems.snake.quit = 1;
                break;
        }
    }
}

void spawnRedDot(time_variables_t timeVars, game_elements_t& gameElems) {
    if (gameElems.redDot.isActive) return;

    position_t newPos;
    int validPosition = 0;
    while (!validPosition) {
        newPos.x = MARGIN + (rand() % (WIDTH / SQUARE_SIZE)) * SQUARE_SIZE + SQUARE_SIZE / 2;
        newPos.y = MARGIN + (rand() % (WIDTH / SQUARE_SIZE)) * SQUARE_SIZE + SQUARE_SIZE / 2;
        validPosition = 1;
        if (newPos.x != gameElems.blueDotPos.x && newPos.y != gameElems.blueDotPos.y) {
            for (int i = 0; i < gameElems.snake.length; i++) {
                if (newPos.x == gameElems.snake.body[i].x && newPos.y == gameElems.snake.body[i].y) {
                    validPosition = 1;
                    break;
                }
            }
        }
    }

    gameElems.redDot.position = newPos;
    gameElems.redDot.spawnTime = timeVars.worldTime;
    gameElems.redDot.isActive = 1;
}

void checkRedDot(game_elements_t& gameElems) {
    if (!gameElems.redDot.isActive) return;

    if (gameElems.redDot.position.x == gameElems.snake.body[0].x && 
        gameElems.snake.body[0].y == gameElems.redDot.position.y) {
        if (rand() % 2 == 0) {
            int newLength = gameElems.snake.length - SHORTEN_LENGTH;
            if (newLength < MIN_SNAKE_LENGTH){
                newLength = MIN_SNAKE_LENGTH;
            } 
            gameElems.snake.length = newLength;
        } 
        else {
            if(gameElems.snake.speed - SLOWDOWN_FACTOR>1){
                gameElems.snake.speed -= SLOWDOWN_FACTOR;
            }
            else {
                gameElems.snake.speed = DEFAULT_SNAKE_SPEED;
            }
        }
        updateScore(gameElems.snake.score, 'r');
        gameElems.redDot.isActive = 0;
    }
}

void drawProgressBar(graphics_t graphics, red_dot_t& redDot, time_variables_t timeVars, colors_t colors) {
    if (!redDot.isActive) return;

    float timeElapsed = timeVars.worldTime - redDot.spawnTime;
    float remainingTime = redDot.displayDuration - timeElapsed;

    if (remainingTime <= 0) {
        redDot.isActive = 0;
        return;
    }

    SDL_Rect borderRect = {
        PROGRESS_BAR_WIDTH *2,
        PROGRESS_BAR_HEIGHT *2,
        PROGRESS_BAR_WIDTH,
        PROGRESS_BAR_HEIGHT
    };
    DrawRectangle(graphics, borderRect.x, borderRect.y, 
                  borderRect.w, borderRect.h, colors.green, colors.black);

    float fillHeight = (remainingTime / redDot.displayDuration) * (PROGRESS_BAR_HEIGHT/3);
    SDL_Rect fillRect = {
        borderRect.x + 2,
        borderRect.y + 2 + (int)((PROGRESS_BAR_HEIGHT - 4) - fillHeight),
        borderRect.w - 4,
        (int)fillHeight
    };
    SDL_FillRect(graphics.screen, &fillRect, colors.green);

    char timeText[32];
    snprintf(timeText, sizeof(timeText), "%.1fs", remainingTime);

    DrawString(graphics, borderRect.x, borderRect.y + PROGRESS_BAR_HEIGHT + 10, timeText, PROGRESS_BAR_WIDTH, JUSTIFY_CENTER);
}

void updateBonusSystem(time_variables_t timeVars, game_elements_t& gameElems) {
    if (!gameElems.redDot.isActive && timeVars.worldTime >= gameElems.redDot.spawnTime) {
        spawnRedDot(timeVars, gameElems);
        float spawnInterval = MIN_SPAWN_INTERVAL + (rand()%2) * (MAX_SPAWN_INTERVAL- MIN_SPAWN_INTERVAL);
        gameElems.redDot.spawnTime = timeVars.worldTime + spawnInterval;
    }

    checkRedDot(gameElems);

    if (gameElems.redDot.isActive && (timeVars.worldTime - gameElems.redDot.spawnTime) >= gameElems.redDot.displayDuration) {
        gameElems.redDot.isActive = 0;
    }
}

void updateScore(int& score, char a ){
    switch(a) {
        case 'r':
            score +=  RED_DOT_BONUS;
            break;
        case 'b':
            score +=  BLUE_DOT_BONUS;
            break;
        default:
            break;
    }
}

void processSnakeMovement(time_variables_t& timeVars, game_state_t& gameState, game_elements_t& gameElems, char* buffer) {
    while (timeVars.timeAccumulator >= (SQUARE_SIZE / (gameElems.snake.speed * SPEED_FIX))) {
        if ((gameElems.snake.direction[0] == UP && gameElems.snake.direction[1] == DOWN) ||
            (gameElems.snake.direction[0] == DOWN && gameElems.snake.direction[1] == UP) ||
            (gameElems.snake.direction[0] == LEFT && gameElems.snake.direction[1] == RIGHT) ||
            (gameElems.snake.direction[0] == RIGHT && gameElems.snake.direction[1] == LEFT)) {
            gameElems.snake.direction[0] = gameElems.snake.direction[1];
        }

        int stepX = 0;
        int stepY = 0;

        if (gameElems.snake.direction[0] == UP) {
            stepY -= SQUARE_SIZE;
        } else if (gameElems.snake.direction[0] == RIGHT) {
            stepX += SQUARE_SIZE;
        } else if (gameElems.snake.direction[0] == DOWN) {
            stepY += SQUARE_SIZE;
        } else if (gameElems.snake.direction[0] == LEFT) {
            stepX -= SQUARE_SIZE;
        }

        int nextX = gameElems.snake.body[0].x + stepX;
        int nextY = gameElems.snake.body[0].y + stepY;
        position_t nextPos;
        nextPos.x = nextX;
        nextPos.y = nextY;

        if (!isValidMove(nextPos)) {
            autoTurn(gameElems.snake);
        }

        if (checkSnakeCollision(&gameElems.snake, nextX, nextY)) {
            buffer[0] = '\0';
            if (isScoreGoodEnough(gameElems.snake.score)) {
                gameState = ADD_SCORE;
            } else {
                gameState = GAME_OVER;
            }
            return;
        }

        checkBlueDot(nextPos,gameElems);
        updateBonusSystem(timeVars, gameElems);
        UpdateSnake(gameElems);

        timeVars.timeAccumulator -= (SQUARE_SIZE / (gameElems.snake.speed * SPEED_FIX));
    }
}

void handleGameRunningInput(graphics_t graphics, time_variables_t& timeVars, game_elements_t& gameElems) {
    while(SDL_PollEvent(&graphics.event)) {
            switch(graphics.event.type) {
                case SDL_KEYDOWN:
                    if(graphics.event.key.keysym.sym == SDLK_ESCAPE) gameElems.snake.quit = 1;
                    else if(graphics.event.key.keysym.sym == SDLK_n) ResetGame(timeVars, gameElems);
                    else if(graphics.event.key.keysym.sym == SDLK_s) saveGameState(timeVars,gameElems);
                    else if(graphics.event.key.keysym.sym == SDLK_i) loadGameState(timeVars,gameElems);
                    else if(graphics.event.key.keysym.sym == SDLK_UP){
						gameElems.snake.direction[0] = UP;
					} 
                    else if(graphics.event.key.keysym.sym == SDLK_RIGHT){
						gameElems.snake.direction[0] = RIGHT;
					} 
                    else if(graphics.event.key.keysym.sym == SDLK_DOWN){
						gameElems.snake.direction[0] = DOWN;
					} 
                    else if(graphics.event.key.keysym.sym == SDLK_LEFT){
						gameElems.snake.direction[0] = LEFT;
					}
                    break;
                case SDL_QUIT:
                    gameElems.snake.quit = 1;
                    break;
            };
        }
}

void renderScren (graphics_t graphics){
    SDL_UpdateTexture(graphics.scrtex, NULL, graphics.screen->pixels, graphics.screen->pitch);
    SDL_RenderCopy(graphics.renderer, graphics.scrtex, NULL, NULL);
    SDL_RenderPresent(graphics.renderer);
}

void drawBottomStats (colors_t colors, graphics_t graphics,char* text) {
    DrawRectangle(graphics, TEXT_MARGIN, STATS_BOTTOM_Y,STATS_BOTTOM_X, STATS_HEIGHT, colors.green, colors.black);
    snprintf(text,SCREEN_WIDTH, "requirments:");
    DrawString(graphics, 2*TEXT_MARGIN, STATS_BOTTOM_Y + LINE_HEIGHT, text, SCREEN_WIDTH/2, JUSTIFY_LEFT);
    snprintf(text,SCREEN_WIDTH, "1, 2, 3, 4, A, B, C, D, E, F, G");
    DrawString(graphics, 2*TEXT_MARGIN,STATS_BOTTOM_Y+2*LINE_HEIGHT, text, SCREEN_WIDTH/2, JUSTIFY_LEFT);
    snprintf(text, SCREEN_WIDTH, "ESC - quit, N - new game");
    DrawString(graphics, SCREEN_WIDTH/2 - 2*TEXT_MARGIN, STATS_BOTTOM_Y + LINE_HEIGHT, text, SCREEN_WIDTH/2, JUSTIFY_RIGHT);
    snprintf(text, SCREEN_WIDTH, "S - save, I - load last save");
    DrawString(graphics, SCREEN_WIDTH/2 - 2*TEXT_MARGIN,STATS_BOTTOM_Y+2*LINE_HEIGHT, text, SCREEN_WIDTH/2, JUSTIFY_RIGHT);
}

void drawGameElements (colors_t colors, graphics_t graphics, time_variables_t timeVars, game_elements_t& gameElems) {
    DrawSnakePart(graphics, gameElems.snake.body[0], SNAKE_HEAD, gameElems.snake.direction[0], colors);
    for (int i = 1; i < gameElems.snake.length - 1; i++) {
        snake_part_t partType = (i % 2 == (gameElems.redDot.displayMode%2)) ? SNAKE_BODY_SMALL : SNAKE_BODY_BIG;
        DrawSnakePart(graphics, gameElems.snake.body[i], partType, gameElems.snake.direction[i], colors);
    }

    DrawSnakePart(graphics, gameElems.snake.body[gameElems.snake.length-1], SNAKE_TAIL, gameElems.snake.direction[gameElems.snake.length-1], colors);

    position_t dotSprite;
    dotSprite.x = 5;
    dotSprite.y = 8 + gameElems.redDot.displayMode % 4;
    DrawSpriteScaled(graphics, gameElems.blueDotPos,  dotSprite, 4, colors.blue);

    if(gameElems.redDot.isActive == 1){
        SDL_SetSurfaceColorMod(graphics.cr, 255, 0, 0);
        DrawSpriteScaled(graphics, gameElems.redDot.position, dotSprite,4, colors.red);
        SDL_SetSurfaceColorMod(graphics.cr, 255, 255, 255);
        drawProgressBar(graphics,gameElems.redDot,timeVars,colors);
    }
}

void drawTopStats(colors_t colors, graphics_t graphics, time_variables_t timeVars, char* text, snake_t snake) {
    DrawRectangle(graphics, TEXT_MARGIN,TEXT_MARGIN, STATS_BOTTOM_X, STATS_HEIGHT, colors.green, colors.black);
    snprintf(text, SCREEN_WIDTH/3, "time = %.1lf s ", timeVars.worldTime);
    DrawString(graphics,2*TEXT_MARGIN, LINE_HEIGHT, text, SCREEN_WIDTH/2, JUSTIFY_LEFT);
    snprintf(text, SCREEN_WIDTH/3, "score: %d", snake.score);
    DrawString(graphics, 2*TEXT_MARGIN, LINE_HEIGHT*2, text, SCREEN_WIDTH/2, JUSTIFY_LEFT);
    snprintf(text, SCREEN_WIDTH/3, "length: %d", snake.length);
    DrawString(graphics,SCREEN_WIDTH/2 - 2*TEXT_MARGIN, LINE_HEIGHT, text, SCREEN_WIDTH/2, JUSTIFY_RIGHT);
    snprintf(text, SCREEN_WIDTH/3, "speed: %.1f", snake.speed);
    DrawString(graphics,SCREEN_WIDTH/2 - 2*TEXT_MARGIN, LINE_HEIGHT*2, text, SCREEN_WIDTH/2, JUSTIFY_RIGHT);
}

void handleTimeIncrease(time_variables_t& timeVars, snake_t& snake) {
    timeVars.timeAccumulator += timeVars.delta;
    if(timeVars.worldTime > timeVars.nextIncrease){
        snake.speed += SPEED_UP_AMOUNT;
        timeVars.nextIncrease += SPEED_UP_TIME;
    }
}

void handleTimeUpdate(time_variables_t& timeVars) {
    timeVars.t2 = SDL_GetTicks();
    timeVars.delta = (timeVars.t2 - timeVars.t1) * 0.001f;
    timeVars.t1 = timeVars.t2;
    timeVars.worldTime += timeVars.delta;
}

void saveGameState(time_variables_t timeVars,game_elements_t gameElems) {
    FILE* file = fopen(SAVE_FILENAME, "w");
    if (!file) {
        perror("Error opening file for saving");
        return;
    }

    fprintf(file, "%d\n", gameElems.snake.length);
    fprintf(file, "%d\n", gameElems.snake.direction[0]);
    fprintf(file, "%lf\n", gameElems.snake.speed);
    fprintf(file, "%d\n", gameElems.snake.score);
    for (int i = 0; i < gameElems.snake.length; i++) {
        fprintf(file, "%d %d %d\n", gameElems.snake.body[i].x, gameElems.snake.body[i].y, gameElems.snake.direction[i]);
    }
    fprintf(file, "%d %d\n", gameElems.redDot.position.x, gameElems.redDot.position.y);
    fprintf(file, "%f\n", gameElems.redDot.spawnTime);
    fprintf(file, "%f\n", gameElems.redDot.displayDuration);
    fprintf(file, "%d\n", gameElems.redDot.isActive);

    fprintf(file, "%lf\n", timeVars.worldTime);
    fprintf(file, "%lf\n", timeVars.nextIncrease);

    fprintf(file, "%d %d\n", gameElems.blueDotPos.x, gameElems.blueDotPos.y);

    fclose(file);
}

int loadGameFromFile(time_variables_t& timeVars, game_elements_t& gameElems) {
    FILE* file = fopen(SAVE_FILENAME, "r");
    if (!file) {
        perror("Error opening file for loading");
        return 0;
    }
    ResetGame(timeVars, gameElems);
    if (fscanf(file, "%d", &gameElems.snake.length) != 1) return 0;
    if (fscanf(file, "%d", &gameElems.snake.direction[0]) != 1) return 0;
    if (fscanf(file, "%lf",&gameElems.snake.speed) != 1) return 0;
    if (fscanf(file, "%d", &gameElems.snake.score) != 1) return 0;
    for (int i = 0; i < gameElems.snake.length; i++) {
        if (fscanf(file, "%d %d %d", &gameElems.snake.body[i].x, &gameElems.snake.body[i].y, &gameElems.snake.direction[i]) != 3) return 0;
    }

    if (fscanf(file, "%d %d", &gameElems.redDot.position.x, &gameElems.redDot.position.y) != 2) return 0;
    if (fscanf(file, "%f", &gameElems.redDot.spawnTime) != 1) return 0;
    if (fscanf(file, "%f", &gameElems.redDot.displayDuration) != 1) return 0;
    if (fscanf(file, "%d", &gameElems.redDot.isActive) != 1) return 0;

    if (fscanf(file, "%lf", &timeVars.worldTime) != 1) return 0;
    if (fscanf(file, "%lf", &timeVars.nextIncrease) != 1) return 0;

    if (fscanf(file, "%d %d", &gameElems.blueDotPos.x, &gameElems.blueDotPos.y) != 2) return 0;

    fclose(file);
    return 1;
}

int isFileNotEmpty(const char* saveFilename) {
    FILE* file = fopen(saveFilename, "r");
    if (!file) {
        return 0;
    }
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fclose(file);
    return fileSize > 0;
}

void loadGameState(time_variables_t& timeVars, game_elements_t& gameElems){
    if (isFileNotEmpty(SAVE_FILENAME)) {
        loadGameFromFile(timeVars, gameElems);
    } else {
        printf("No data available to load.\n");
    }
}

void drawAddScore(graphics_t graphics, colors_t colors,char* buffer, snake_t snake) {
    int windowWidth = SCREEN_WIDTH / 2;
    int windowHeight = SCREEN_HEIGHT / 3;
    int windowX = (SCREEN_WIDTH - windowWidth) / 2;
    int windowY = (SCREEN_HEIGHT - windowHeight) / 2;

    DrawRectangle(graphics, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 
                  SDL_MapRGB(graphics.screen->format, 0x00, 0x00, 0x00), 
                  SDL_MapRGB(graphics.screen->format, 0x00, 0x00, 0x00));

    DrawRectangle(graphics, windowX, windowY, windowWidth, windowHeight,colors.red, colors.black);

    char text[BUFFER_SIZE];
    snprintf(text, sizeof(text), "WOW! THAT'S A GREAT SCORE %s!", snake.playerName);
    DrawString(graphics, windowX , 
               windowY + LINE_HEIGHT, text, SCREEN_WIDTH/2, JUSTIFY_CENTER);

    snprintf(text, sizeof(text), "Enter your name to add it to scoreboard:");
    DrawString(graphics, windowX , 
               windowY + LINE_HEIGHT*2, text, SCREEN_WIDTH/2, JUSTIFY_CENTER);

    snprintf(text, sizeof(text), "%s", buffer);
    DrawString(graphics, windowX , 
               windowY + LINE_HEIGHT*4, text, SCREEN_WIDTH/2, JUSTIFY_CENTER);

    DrawLine(graphics, windowWidth - windowWidth/4, windowY + LINE_HEIGHT*5, windowWidth/2, 1, 0, colors.blue);


    snprintf(text, sizeof(text), "Press Enter to start typing");
    DrawString(graphics, windowX , 
               windowY + LINE_HEIGHT*8, text, SCREEN_WIDTH/2, JUSTIFY_CENTER);

    snprintf(text, sizeof(text), "Press N to start a new game");
    DrawString(graphics, windowX , 
               windowY + LINE_HEIGHT*9, text, SCREEN_WIDTH/2, JUSTIFY_CENTER);
    snprintf(text, sizeof(text), "Press ESC to Quit");
    DrawString(graphics, windowX , 
               windowY + LINE_HEIGHT*10, text, SCREEN_WIDTH/2, JUSTIFY_CENTER);
}

void handleAddScore(graphics_t graphics, game_state_t& gameState, time_variables_t& timeVars, game_elements_t& gameElems, char* buffer) {
    static int isTyping = 0;
    while (SDL_PollEvent(&graphics.event)) {
        switch (graphics.event.type) {
            case SDL_QUIT:
                gameElems.snake.quit = 1;
                break;
            case SDL_KEYDOWN:
                if (graphics.event.key.keysym.sym == SDLK_ESCAPE) {
                    gameElems.snake.quit = 1;
                }
                else if (graphics.event.key.keysym.sym == SDLK_RETURN) {
                    if (isTyping && buffer[0] !='\0') {
                        strncpy(gameElems.snake.playerName, buffer, BUFFER_SIZE - 1);
                        gameElems.snake.playerName[BUFFER_SIZE - 1] = '\0';
                        isTyping = 0;
                        updateScoreboard(gameElems.snake);
                        buffer[0] = '\0';
                        gameElems.snake.playerName[0] = '\0';
                        gameState = GAME_OVER;
                    } else {
                        isTyping = 1;
                        buffer[0] = '\0';
                    }
                } 
                else if (isTyping && graphics.event.key.keysym.sym == SDLK_BACKSPACE) {
                    int nameLength = strlen(buffer);
                    if (nameLength > 0) {
                        buffer[nameLength - 1] = '\0';
                    }
                }
                break;
            case SDL_TEXTINPUT:
                if (isTyping && strlen(buffer) < BUFFER_SIZE - 1) {
                    strcat(buffer, graphics.event.text.text);
                }
                break;
            default:
                break;
        }
    }
}

void handleGraphicsSetup (graphics_t& graphics){
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize(graphics.renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_SetRenderDrawColor(graphics.renderer, 0, 0, 0, 255);

    SDL_SetWindowTitle(graphics.window, "snake - EL");
    graphics.screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    graphics.scrtex = SDL_CreateTexture(graphics.renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

    SDL_ShowCursor(SDL_DISABLE);
    graphics.cr = SDL_LoadBMP("./cs8x8c.bmp");
    if (!graphics.cr) {
        exit(1);
    }

    SDL_SetColorKey(graphics.cr, true, 0x000000);
    graphics.charset = SDL_CreateTextureFromSurface(graphics.renderer, graphics.cr);
}


#ifdef __cplusplus
extern "C"
#endif

int main() {
    srand(time(NULL));   

	game_state_t gameState = GAME_RUNNING;
    int rc;

    char buffer[BUFFER_SIZE];

    time_variables_t timeVars;
    timeVars.nextIncrease = SPEED_UP_AMOUNT;
    timeVars.worldTime = 0;
    timeVars.t1 = 0;
    timeVars.t2 = 0;

    snake_t snake;
    snake.direction[0] = RIGHT;
    snake.length = DEFAULT_SNAKE_LENGTH;
    snake.speed = DEFAULT_SNAKE_SPEED;

    red_dot_t redDot;
    redDot.isActive = 0;
    redDot.displayDuration = 5;
    redDot.displayMode = 1;

    position_t blueDotPos;
    graphics_t graphics;

    game_elements_t gameElems;
    gameElems.snake = snake;
    gameElems.redDot = redDot;
    gameElems.blueDotPos = blueDotPos;


    ResetGame(timeVars, gameElems);

    if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }

    rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &graphics.window, &graphics.renderer);

    if(rc != 0) {
        SDL_Quit();
        printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
        return 1;
    }
    handleGraphicsSetup(graphics);

    colors_t colors;
    colors.black = SDL_MapRGB(graphics.screen->format, 0x00, 0x00, 0x00);
    colors.green = SDL_MapRGB(graphics.screen->format, 0x00, 0xFF, 0x00);
    colors.red = SDL_MapRGB(graphics.screen->format, 0xFF, 0x00, 0x00);
    colors.blue = SDL_MapRGB(graphics.screen->format, 0x11, 0x11, 0xCC);
    colors.white = SDL_MapRGB(graphics.screen->format, 0xFF, 0xFF, 0xFF);
    colors.grey = SDL_MapRGB(graphics.screen->format, 0x33, 0x33, 0x33);

    
    char text[BUFFER_SIZE];
    timeVars.t1 = SDL_GetTicks();
    timeVars.frames = 0;
    gameElems.snake.quit = 0;
    while(!gameElems.snake.quit) {
        handleTimeUpdate(timeVars);
		if (gameState == GAME_RUNNING) {
            handleTimeIncrease(timeVars,gameElems.snake);
            processSnakeMovement(timeVars, gameState, gameElems, buffer);
            SDL_FillRect(graphics.screen, NULL, colors.black);
            DrawRectangle(graphics, MARGIN, MARGIN, WIDTH, HEIGHT, colors.green, colors.black); 
            drawGrid(graphics, colors.grey);
            drawGameElements(colors,graphics,timeVars, gameElems);
            drawTopStats(colors,graphics,timeVars,buffer, gameElems.snake);
            drawBottomStats(colors,graphics,buffer);
            renderScren(graphics);
            handleGameRunningInput(graphics, timeVars, gameElems);
		}
		else if (gameState == GAME_OVER) {
			SDL_FillRect(graphics.screen, NULL, colors.black);
			drawGameOver(graphics, colors, gameElems.snake);
			renderScren(graphics);
			handleGameOverInput(graphics, gameState, timeVars, gameElems);
		}
        else if (gameState == ADD_SCORE) {
			SDL_FillRect(graphics.screen, NULL, colors.black);
            drawAddScore(graphics, colors, text, gameElems.snake);
            handleAddScore(graphics, gameState,timeVars, gameElems, text);
            renderScren(graphics);
		}
        timeVars.frames++;
    }

    SDL_FreeSurface(graphics.cr);
    SDL_FreeSurface(graphics.screen);
    SDL_DestroyTexture(graphics.scrtex);
    SDL_DestroyRenderer(graphics.renderer);
    SDL_DestroyWindow(graphics.window);

    SDL_Quit();
    return 0;
};