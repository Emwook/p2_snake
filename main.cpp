#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>
#include <time.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

typedef struct {
    int squareSize;
    int screenWidth;
    int screenHeight;
    int margin;
    int width;
    int height;
    int statsHeight;
    int y;
    int x;
    int statsWidth;
    int speedFix;
    int maxSnakeLength;
    int minSnakeLength;
    int defaultSnakeSpeed;
    int defaultSnakeLength;
    float minSpawnInterval;
    float maxSpawnInterval;
    float displayDuration;
    int shortenLength;
    float slowdownFactor;
    int progressBarWidth;
    int progressBarHeight;
    int redDotBonus;
    int blueDotBonus;
    double speedUpTime;
    double speedUpAmount;
    char filename[128];
} config_t;

typedef enum {
    GAME_RUNNING,
    GAME_OVER
} game_state_t;

typedef struct {
    int x;
    int y;
} position_t;

typedef struct {
    position_t body[20];
    int length;
    int direction;
    double speed;
    int score;
} snake_t;

typedef struct {
    SDL_Point position;
    float spawnTime;
    float displayDuration;
    int isActive;
    SDL_Surface* red_dot;
} red_dot_t;

typedef struct {
    int black;
    int green;
    int red;
    int blue;
} colors_t;

typedef struct {
    float timeAccumulator;
    float delta;
    double nextIncrease;
    double worldTime;
    double fps;
    double fpsTimer;
    int frames;
    double t1;
    double t2;
} time_variables_t;

void randomizePosition(position_t &pos, snake_t& snake, config_t config);
void spawnRedDot(red_dot_t& redDot, snake_t* snake, float worldTime, position_t blueDotPos, config_t config);
void updateScore(int& score, char a,  config_t config);
void saveGameState(snake_t snake, red_dot_t redDot,time_variables_t timeVars,position_t blueDot,config_t config);
void loadGameState(snake_t& snake, red_dot_t& redDot, time_variables_t& timeVars, position_t& blueDotPos, config_t config);

void DrawString(SDL_Surface *screen, int x, int y, const char *text, SDL_Surface *charset) {
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while(*text) {
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitSurface(charset, &s, screen, &d);
		x += 8;
		text++;
	};
};

void DrawSurface(SDL_Surface *screen, SDL_Surface *sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x - sprite->w / 2;
	dest.y = y - sprite->h / 2;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
};

void DrawPixel(SDL_Surface *surface, int x, int y, Uint32 color) {
	int bpp = surface->format->BytesPerPixel;
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32 *)p = color;
	};

void DrawLine(SDL_Surface *screen, int x, int y, int l, int dx, int dy, Uint32 color) {
	for(int i = 0; i < l; i++) {
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
		};
	};

void DrawRectangle(SDL_Surface *screen, int x, int y, int l, int k, Uint32 outlineColor, Uint32 fillColor) {
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for(i = y + 1; i < y + k - 1; i++)
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
	};

void ResetGame(time_variables_t& timeVars, snake_t &snake, position_t& blueDotPos, red_dot_t& redDot, config_t config) {
	snake.length = config.minSnakeLength;
	for(int i = 0; i<snake.length; i++){
		snake.body[i].x = config.screenWidth / 2 - (i *config.squareSize);
    	snake.body[i].y  = config.screenHeight / 2;
	}
	snake.speed = 1;
	snake.direction = 1;
    timeVars.worldTime = 0;
    timeVars.timeAccumulator = 0;

    randomizePosition(blueDotPos,snake, config);
    spawnRedDot(redDot,&snake,timeVars.worldTime,blueDotPos, config); // not randomizing the red dot - BUG!
    snake.score = 0;
}

int isValidMove(position_t nextPos, config_t config) {
    int halfSquare = config.squareSize / 2;
    int nextX = nextPos.x;
    int nextY = nextPos.y;
    return (nextX - halfSquare >= config.margin &&
            nextX + halfSquare <= config.margin + config.width &&
            nextY - halfSquare >= config.margin &&
            nextY + halfSquare <= config.margin + config.height);
}

void autoTurn(snake_t &snake, config_t config) {
    int step = config.squareSize;
	int rightTurn = (snake.direction + 1) % 4;
    int leftTurn = (snake.direction + 3) % 4;
    
    position_t rightPos = {
        .x =  snake.body[0].x,
        .y = snake.body[0].y,
    };
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
    
    position_t leftPos = {
        .x =  snake.body[0].x,
        .y = snake.body[0].y,
    };
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
    
    if (isValidMove(rightPos, config)) {
        snake.direction = rightTurn;
        return;
    }
    
    if (isValidMove(leftPos, config)) {
        snake.direction = leftTurn;
        return;
    }
}

void drawGrid(SDL_Surface *screen, Uint32 color, config_t config) {
    for (int x = config.margin; x < config.margin + config.width; x += config.squareSize) {
        DrawLine(screen, x, config.margin, config.height, 0, 1, color);
    }
    
    for (int y = config.margin; y < config.margin + config.height; y += config.squareSize) {
        DrawLine(screen, config.margin, y, config.width, 1, 0, color);
    }
}

void UpdateSnake(snake_t &snake, config_t config) {
    for (int i = snake.length - 1; i > 0; --i) {
        snake.body[i] = snake.body[i - 1];
    }

    if (snake.direction == 0) {
        snake.body[0].y -= config.squareSize;
    } 
    else if (snake.direction == 1) {
        snake.body[0].x += config.squareSize;
    } 
    else if (snake.direction == 2) {
        snake.body[0].y += config.squareSize;
    } 
    else if (snake.direction == 3) {
        snake.body[0].x -= config.squareSize;
    }
}

int checkSnakeCollision(snake_t* snake, int nextX, int nextY) { //revise for short boy
    for (int i = 1; i < snake->length - 1; i++) {
        if (nextX == snake->body[i].x && nextY == snake->body[i].y) {
            return 1;
        }
    }
	return 0;
}

void randomizePosition(position_t &pos, snake_t& snake, config_t config) {
    int validPosition;
    int i;

    do {
        pos.x = config.margin + (rand() % (config.width / config.squareSize)) * config.squareSize + config.squareSize / 2;
        pos.y = config.margin + (rand() % (config.height / config.squareSize)) * config.squareSize + config.squareSize / 2;

        validPosition = 1;
        for (i = 0; i < snake.length; i++) {
            if (pos.x == snake.body[i].x && pos.y == snake.body[i].y) {
                validPosition = 0;
                break;
            }
        }
    } while (!validPosition);
}

void checkBlueDot(snake_t& snake, int nextX, int nextY, position_t &blue_dot_pos, config_t config) {
	if (nextX == blue_dot_pos.x && nextY == blue_dot_pos.y &&snake.length <=config.maxSnakeLength) {
		snake.length++;
        updateScore(snake.score, 'b', config);
		randomizePosition(blue_dot_pos, snake, config);
	}
}

void drawGameOver(SDL_Surface* screen, SDL_Surface* charset, int score, game_state_t* gameState, config_t config) {
    int windowWidth = config.screenWidth / 2;
    int windowHeight = config.screenHeight/ 3;
    int windowX = (config.screenWidth - windowWidth) / 2;
    int windowY = (config.screenHeight - windowHeight) / 2;
    
    Uint32 colorBlack = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
    Uint32 colorRed = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
    Uint32 colorWhite = SDL_MapRGB(screen->format, 0xFF, 0xFF, 0xFF);
    
    DrawRectangle(screen, 0, 0, config.screenWidth, config.screenHeight, 
                 SDL_MapRGB(screen->format, 0x00, 0x00, 0x00), 
                 SDL_MapRGB(screen->format, 0x00, 0x00, 0x00));
    
    DrawRectangle(screen, windowX, windowY, windowWidth, windowHeight, 
                 colorRed, colorBlack);
    
    char text[128];
    snprintf(text, sizeof(text), "GAME OVER!");
    DrawString(screen, windowX + (windowWidth - strlen(text) * 8) / 2, 
              windowY + 30, text, charset);
    
    snprintf(text, sizeof(text), "Final Score: %d", score);
    DrawString(screen, windowX + (windowWidth - strlen(text) * 8) / 2, 
              windowY + 60, text, charset);
    
    snprintf(text, sizeof(text), "Press N for New Game");
    DrawString(screen, windowX + (windowWidth - strlen(text) * 8) / 2, 
              windowY + 90, text, charset);
    
    snprintf(text, sizeof(text), "Press ESC to Quit");
    DrawString(screen, windowX + (windowWidth - strlen(text) * 8) / 2, 
              windowY + 120, text, charset);
}

void handleGameOverInput(SDL_Event& event, int& quit, game_state_t& gameState, snake_t& snake,  time_variables_t& timeVars, position_t& blueDotPos, red_dot_t& redDot, config_t config) {
    while(SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_KEYDOWN:
                if(event.key.keysym.sym == SDLK_ESCAPE) {
                    quit = 1;
                }
                else if(event.key.keysym.sym == SDLK_n) {
                    ResetGame(timeVars, snake, blueDotPos, redDot, config);
                    gameState = GAME_RUNNING;
                }
                break;
            case SDL_QUIT:
                quit = 1;
                break;
        }
    }
}

void drawCircle(SDL_Surface* surface, int centerX, int centerY, int radius, Uint32 color) {
    int x, y;
    
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            int distance = dx * dx + dy * dy;
				if (distance <= radius * radius) {
                x = centerX + dx;
                y = centerY + dy;
				if (x >= 0 && x < surface->w && y >= 0 && y < surface->h) {
                    DrawPixel(surface, x, y, color);
                }
            }
        }
    }
}

void spawnRedDot(red_dot_t& redDot, snake_t* snake, float worldTime, position_t blueDotPos, config_t config) { // add a check for no blue and red dot collision!
    if (redDot.isActive) return;

    SDL_Point newPos;
    int validPosition = 0;
    while (!validPosition) {
        newPos.x = config.margin + (rand() % (config.width / config.squareSize)) * config.squareSize + config.squareSize / 2;
        newPos.y = config.margin + (rand() % (config.width / config.squareSize)) * config.squareSize + config.squareSize / 2;
        validPosition = 1;
        if (newPos.x != blueDotPos.x && newPos.y != blueDotPos.y) {
            for (int i = 0; i < snake->length; i++) {
                if (newPos.x == snake->body[i].x && newPos.y == snake->body[i].y) {
                    validPosition = 1;
                    break;
                }
            }
        }
    }

    redDot.position = newPos;
    redDot.spawnTime = worldTime;
    redDot.isActive = 1;
}

void checkRedDot(snake_t* snake, red_dot_t& redDot, config_t config) {
    if (!redDot.isActive) return;

    if (redDot.position.x == snake->body[0].x && 
        snake->body[0].y == redDot.position.y) {
        if (rand() % 2 == 0) {
            int newLength = snake->length - config.shortenLength;
            if (newLength < config.minSnakeLength){
                newLength = config.minSnakeLength;
            } 
            snake->length = newLength;
        } 
        else {
            if(snake->speed - config.slowdownFactor>1){
                snake->speed -= config.slowdownFactor;
            }
            else {
                snake->speed = config.defaultSnakeSpeed;
            }
        }
        updateScore(snake->score, 'r', config);
        redDot.isActive = 0;
    }
}

void drawProgressBar(SDL_Surface* screen, red_dot_t* redDot, float worldTime, colors_t colors, SDL_Surface* charset, config_t config) { // bad progress bar height
    if (!redDot->isActive) return;

    float timeElapsed = worldTime - redDot->spawnTime;
    float remainingTime = redDot->displayDuration - timeElapsed;

    if (remainingTime <= 0) {
        redDot->isActive = 0;
        return;
    }

    SDL_Rect borderRect = {
        config.progressBarWidth *2,
        config.progressBarHeight *2,
        config.progressBarWidth,
        config.progressBarHeight
    };
    DrawRectangle(screen, borderRect.x, borderRect.y, 
                  borderRect.w, borderRect.h, colors.green, colors.black);

    float fillHeight = (remainingTime / redDot->displayDuration) * (config.progressBarHeight/3);
    SDL_Rect fillRect = {
        borderRect.x + 2,
        borderRect.y + 2 + (int)((config.progressBarHeight - 4) - fillHeight),
        borderRect.w - 4,
        (int)fillHeight
    };
    SDL_FillRect(screen, &fillRect, colors.green);

    char timeText[32];
    snprintf(timeText, sizeof(timeText), "%.1fs", remainingTime);

    DrawString(screen, borderRect.x, borderRect.y + config.progressBarHeight + 10, timeText, charset);
}

void updateBonusSystem(snake_t* snake, red_dot_t& redDot, float worldTime, position_t blueDotPos,int& score, config_t config) {
    if (!redDot.isActive && worldTime >= redDot.spawnTime) {
        spawnRedDot(redDot, snake, worldTime, blueDotPos, config);
        float spawnInterval = config.minSpawnInterval + 
            (rand()%2) * 
            (config.maxSpawnInterval- config.minSpawnInterval);
        redDot.spawnTime = worldTime + spawnInterval;
    }

    checkRedDot(snake, redDot, config);

    if (redDot.isActive && 
        (worldTime - redDot.spawnTime) >= redDot.displayDuration) {
        redDot.isActive = 0;
    }
}

void updateScore(int& score, char a,  config_t config){
    switch(a) {
        case 'r':
            score +=  config.redDotBonus;
            break;
        case 'b':
            score +=  config.blueDotBonus;
            break;
        default:
            break;
    }
}

void processSnakeMovement(snake_t& snake, config_t& config, time_variables_t& timeVars, position_t& blueDotPos, red_dot_t& redDot, game_state_t& gameState) {
    while (timeVars.timeAccumulator >= (config.squareSize / (snake.speed * config.speedFix))) {
        int stepX = 0;
        int stepY = 0;

        if (snake.direction == 0) {
            stepY -= config.squareSize;
        }
        else if (snake.direction == 1) {
            stepX += config.squareSize;
        }
        else if (snake.direction == 2) {
            stepY += config.squareSize;
        }
        else if (snake.direction == 3) {
            stepX -= config.squareSize;
        }

        int nextX = snake.body[0].x + stepX;
        int nextY = snake.body[0].y + stepY;
        position_t nextPos = { .x = nextX, .y = nextY };

        if (!isValidMove(nextPos, config)) {
            autoTurn(snake, config);
        }

        if (checkSnakeCollision(&snake, nextX, nextY)) {
            gameState = GAME_OVER;
            return;
        }

        checkBlueDot(snake, nextX, nextY, blueDotPos, config);
        updateBonusSystem(&snake, redDot, timeVars.worldTime, blueDotPos, snake.score, config);

        UpdateSnake(snake, config);

        timeVars.timeAccumulator -= (config.squareSize / (snake.speed * config.speedFix));
    }
}

void handleGameRunningInput(SDL_Event &event,int& quit, snake_t& snake,time_variables_t& timeVars, position_t& blueDotPos, red_dot_t& redDot,config_t config) {
    while(SDL_PollEvent(&event)) {
            switch(event.type) {
                case SDL_KEYDOWN:
                    if(event.key.keysym.sym == SDLK_ESCAPE) quit = 1;
                    else if(event.key.keysym.sym == SDLK_n) ResetGame(timeVars, snake, blueDotPos, redDot, config);
                    else if(event.key.keysym.sym == SDLK_s) saveGameState(snake, redDot,timeVars,blueDotPos, config);
                    else if(event.key.keysym.sym == SDLK_i) loadGameState(snake, redDot,timeVars,blueDotPos, config);
                    else if(event.key.keysym.sym == SDLK_UP){
						snake.direction = 0;
					} 
                    else if(event.key.keysym.sym == SDLK_RIGHT){
						snake.direction = 1;
					} 
                    else if(event.key.keysym.sym == SDLK_DOWN){
						snake.direction = 2;
					} 
                    else if(event.key.keysym.sym == SDLK_LEFT){
						snake.direction = 3;
					}
                    break;
                case SDL_QUIT:
                    quit = 1;
                    break;
            };
        }
}

void renderScren (SDL_Texture *scrtex, SDL_Surface *screen,SDL_Renderer *renderer){
    SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
    SDL_RenderCopy(renderer, scrtex, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void drawBottomStats (config_t config, colors_t colors, SDL_Surface *screen, SDL_Surface* charset,char* text) {
    DrawRectangle(screen, config.x/4, config.y, config.statsWidth, config.statsHeight, colors.green, colors.black);
    snprintf(text,config.screenWidth, "wymagania: 1, 2, 3, 4, A, B, C, D");
    DrawString(screen,2*config.x,config.screenHeight-config.statsHeight, text, charset);
    snprintf(text, config.screenWidth, "ESC - wyjdz, N - nowa gra");
    DrawString(screen,config.screenWidth/2 + 2*config.x,config.screenHeight-config.statsHeight, text, charset);
    snprintf(text, config.screenWidth, "S - zapisz, I - zaladuj zapisana");
    DrawString(screen,config.screenWidth/2 + 2*config.x,config.screenHeight-config.statsHeight/2, text, charset);
}

void drawGameElements (config_t config, colors_t colors, SDL_Surface *screen, snake_t snake, SDL_Surface* blue_dot,SDL_Surface* red_dot, position_t blueDotPos, SDL_Surface* eti, SDL_Surface* ite, red_dot_t redDot, SDL_Surface* charset,double worldTime, char* text) {
    DrawSurface(screen, eti, snake.body[0].x, snake.body[0].y);
    for (int i = 1; i <snake.length; i++) {
        DrawSurface(screen, ite, snake.body[i].x, snake.body[i].y);
    }

    DrawSurface(screen, blue_dot, blueDotPos.x, blueDotPos.y);

    if(redDot.isActive == 1){
        DrawSurface(screen, red_dot, redDot.position.x, redDot.position.y);
        drawProgressBar(screen,&redDot,worldTime,colors, charset, config);
    }
}

void drawTopStats(config_t config, colors_t colors, SDL_Surface *screen, SDL_Surface* charset, time_variables_t timeVars, char* text, snake_t snake) {
    DrawRectangle(screen, 4, 4, config.screenWidth-8, 60, colors.green, colors.black);
    snprintf(text, config.screenWidth, "czas trwania = %.1lf s  %.0lf klatek / s", timeVars.worldTime, timeVars.fps);
    DrawString(screen,16 , 20, text, charset);
    snprintf(text, config.screenWidth, "dlugoscs: %d", snake.length);
    DrawString(screen,5*config.screenWidth/6, 40, text, charset);
    snprintf(text, config.screenWidth, "wynik: %d, predkosc: %f", snake.score, snake.speed);
    DrawString(screen, 16, 40, text, charset);
}

void handleTimeIncrease(time_variables_t& timeVars, snake_t& snake, config_t config) {
    timeVars.timeAccumulator += timeVars.delta;
    if(timeVars.worldTime > timeVars.nextIncrease){
        snake.speed += config.speedUpAmount;
        timeVars.nextIncrease += config.speedUpTime;
    }
}

void handleTimeUpdate(time_variables_t& timeVars) {
    timeVars.t2 = SDL_GetTicks();
    timeVars.delta = (timeVars.t2 - timeVars.t1) * 0.001f;
    timeVars.t1 = timeVars.t2;
    timeVars.worldTime += timeVars.delta;
}

// void updateFPS(time_variables_t& timeVars) {
//     timeVars.fpsTimer += timeVars.delta;
//     if(timeVars.fpsTimer > 0.5) {
//         timeVars.fps = timeVars.frames * 2;
//         timeVars.frames = 0;
//         timeVars.fpsTimer -= 0.5;
//     }
// }

void saveGameState(snake_t snake, red_dot_t redDot,time_variables_t timeVars,position_t blueDot,config_t config) {
    FILE* file = fopen(config.filename, "w");
    if (!file) {
        perror("Error opening file for saving");
        return;
    }

    fprintf(file, "%d\n", snake.length);
    fprintf(file, "%d\n", snake.direction);
    fprintf(file, "%lf\n", snake.speed);
    fprintf(file, "%d\n", snake.score);
    for (int i = 0; i < snake.length; i++) {
        fprintf(file, "%d %d\n", snake.body[i].x, snake.body[i].y);
    }

    fprintf(file, "%d %d\n", redDot.position.x, redDot.position.y);
    fprintf(file, "%f\n", redDot.spawnTime);
    fprintf(file, "%f\n", redDot.displayDuration);
    fprintf(file, "%d\n", redDot.isActive);

    fprintf(file, "%lf\n", timeVars.worldTime);
    fprintf(file, "%lf\n", timeVars.nextIncrease);

    fprintf(file, "%d %d\n", blueDot.x, blueDot.y);

    fclose(file);
}

int loadFromFile(snake_t& snake, red_dot_t& redDot, time_variables_t& timeVars, position_t& blueDotPos, config_t config) {
    FILE* file = fopen(config.filename, "r");
    if (!file) {
        perror("Error opening file for loading");
        return 0;
    }
    ResetGame(timeVars, snake, blueDotPos,redDot, config);
    if (fscanf(file, "%d", &snake.length) != 1) return 0;
    if (fscanf(file, "%d", &snake.direction) != 1) return 0;
    if (fscanf(file, "%lf", &snake.speed) != 1) return 0;
    if (fscanf(file, "%d", &snake.score) != 1) return 0;
    for (int i = 0; i < snake.length; i++) {
        if (fscanf(file, "%d %d", &snake.body[i].x, &snake.body[i].y) != 2) return 0;
    }

    // Load red_dot not correct
    if (fscanf(file, "%d %d", &redDot.position.x, &redDot.position.y) != 2) return 0;
    if (fscanf(file, "%f", &redDot.spawnTime) != 1) return 0;
    if (fscanf(file, "%f", &redDot.displayDuration) != 1) return 0;
    if (fscanf(file, "%d", &redDot.isActive) != 1) return 0;

    // Load worldTime and nextIncrease to check
    if (fscanf(file, "%lf", &timeVars.worldTime) != 1) return 0;
    if (fscanf(file, "%lf", &timeVars.nextIncrease) != 1) return 0;

    if (fscanf(file, "%d %d", &blueDotPos.x, &blueDotPos.y) != 2) return 0;

    fclose(file);
    return 1;
}

int isFileNotEmpty(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        return 0;
    }
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fclose(file);
    return fileSize > 0;
}

void loadGameState(snake_t& snake, red_dot_t& redDot, time_variables_t& timeVars, position_t& blueDotPos, config_t config){
    if (isFileNotEmpty(config.filename)) {
        if (loadFromFile(snake, redDot, timeVars, blueDotPos, config)) {
            printf("Game state loaded successfully!\n");
        } else {
            printf("Failed to load game state.\n");
        }
    } else {
        printf("No data available to load.\n");
    }
}

#ifdef __cplusplus
extern "C"
#endif

int main(int argc, char **argv) {

    config_t config= {
        .squareSize = 20,
        .screenWidth = 31 * 20,
        .screenHeight = 31 * 20,
        .margin = 4 * 20,
        .width = (31 * 20) - (2 * (4 * 20)),
        .height = (31 * 20) - (2 * (4 * 20)),
        .statsHeight = 3 * 20,
        .y = (31 * 20) - (3 * 20) - 4,
        .x = 16,
        .statsWidth = (31 * 20) - 8,
        .speedFix = 100,
        .maxSnakeLength = 20,
        .minSnakeLength = 3,
        .defaultSnakeSpeed = 1,
        .defaultSnakeLength = 3,
        .minSpawnInterval = 5.0f,
        .maxSpawnInterval = 10.0f,
        .displayDuration = 10.0f,
        .shortenLength = 1,
        .slowdownFactor = 0.3f,
        .progressBarWidth = 20,
        .progressBarHeight = 100,
        .redDotBonus = 150,
        .blueDotBonus = 100,
        .speedUpTime = 3,
	    .speedUpAmount= 0.1,
        .filename = "game_state.txt",
    };

	srand(time(NULL));
	game_state_t gameState = GAME_RUNNING;
    int quit, rc;
    int stepX, stepY;

    time_variables_t timeVars = {
        .nextIncrease = config.speedUpAmount,
        .worldTime = 0,
        .fpsTimer = 0,
        .fps = 0,
        .t1 = 0,
        .t2 = 0,
    };
    
    snake_t snake;
    snake.direction = 1;
    snake.length = config.defaultSnakeLength;
    snake.speed = config.defaultSnakeSpeed;

	position_t blueDotPos;
	randomizePosition(blueDotPos, snake, config);

    red_dot_t redDot = {
        .isActive = 0,
        .displayDuration = 5,
    };    
    ResetGame(timeVars, snake, blueDotPos, redDot, config);

    SDL_Event event;
    SDL_Surface *screen, *charset;
    SDL_Surface *eti;
	SDL_Surface *ite;
	SDL_Surface *blue_dot;
    SDL_Surface *red_dot;
    SDL_Texture *scrtex;
    SDL_Window *window;
    SDL_Renderer *renderer;

    printf("wyjscie printfa trafia do tego okienka\n");
    printf("printf output goes here\n");

    if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }

    rc = SDL_CreateWindowAndRenderer(config.screenWidth, config.screenHeight, 0,
                                     &window, &renderer);
    if(rc != 0) {
        SDL_Quit();
        printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize(renderer, config.screenWidth, config.screenHeight);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    SDL_SetWindowTitle(window, "snek");

    screen = SDL_CreateRGBSurface(0, config.screenWidth, config.screenHeight, 32,
                                  0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING,
                               config.screenWidth, config.screenHeight);

    SDL_ShowCursor(SDL_DISABLE);

    charset = SDL_LoadBMP("./cs8x8.bmp");
    if(charset == NULL) {
        printf("SDL_LoadBMP(cs8x8.bmp) error: %s\n", SDL_GetError());
        SDL_FreeSurface(screen);
        SDL_DestroyTexture(scrtex);
        SDL_DestroyWindow(window);
        SDL_DestroyRenderer(renderer);
        SDL_Quit();
        return 1;
    }
    SDL_SetColorKey(charset, true, 0x000000);

    eti = SDL_CreateRGBSurface(0, config.squareSize, config.squareSize, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	ite = SDL_CreateRGBSurface(0, config.squareSize, config.squareSize, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	blue_dot = SDL_CreateRGBSurface(0,config.squareSize, config.squareSize, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    red_dot = SDL_CreateRGBSurface(0, config.squareSize, config.squareSize, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    SDL_FillRect(eti, NULL, SDL_MapRGB(eti->format, 255, 255, 0)); 
	SDL_FillRect(ite, NULL, SDL_MapRGB(ite->format, 0, 255, 0)); 
	drawCircle(blue_dot, config.squareSize / 2,config.squareSize / 2,config.squareSize / 3, SDL_MapRGB(blue_dot->format, 0, 0, 255));
    drawCircle(red_dot, config.squareSize / 2, config.squareSize / 2, config.squareSize / 3, SDL_MapRGB(red_dot->format, 255, 0, 0));
	SDL_Texture* blueTex = SDL_CreateTextureFromSurface(renderer, blue_dot);
    SDL_Texture* redTex = SDL_CreateTextureFromSurface(renderer, red_dot);

    if(eti == NULL) {
        printf("SDL_LoadBMP(eti.bmp) error: %s\n", SDL_GetError());
        SDL_FreeSurface(charset);
        SDL_FreeSurface(screen);
        SDL_DestroyTexture(scrtex);
        SDL_DestroyWindow(window);
        SDL_DestroyRenderer(renderer);
        SDL_Quit();
        return 1;
    }

    char text[256];
    colors_t colors = {
        .black = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00),
        .green = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00),
        .red = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00),
        .blue = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC),
    };
    

    timeVars.t1 = SDL_GetTicks();
    timeVars.frames = 0;
    quit = 0;
    ResetGame(timeVars, snake, blueDotPos, redDot, config);
    while(!quit) {
        handleTimeUpdate(timeVars);
		if (gameState == GAME_RUNNING) {
            handleTimeIncrease(timeVars,snake,config);
            processSnakeMovement(snake, config, timeVars, blueDotPos, redDot, gameState);
            // updateFPS(timeVars);
            SDL_FillRect(screen, NULL, colors.black);
            DrawRectangle(screen, config.margin, config.margin, config.width, config.height, colors.green, colors.black); 
            drawGrid(screen, SDL_MapRGB(screen->format, 0x33, 0x33, 0x33), config);
            drawGameElements(config,colors,screen,snake,blue_dot,red_dot,blueDotPos,eti,ite,redDot,charset,timeVars.worldTime,text);
            drawTopStats(config,colors,screen, charset,timeVars,text, snake);
            drawBottomStats(config,colors,screen,charset,text);
            renderScren(scrtex,screen,renderer);
            handleGameRunningInput(event, quit, snake, timeVars, blueDotPos, redDot, config);
		}
		else if (gameState == GAME_OVER) {
			SDL_FillRect(screen, NULL, colors.black);
			drawGameOver(screen,charset, snake.score,&gameState, config);
			renderScren(scrtex, screen,renderer);
			handleGameOverInput(event, quit, gameState, snake, timeVars, blueDotPos, redDot, config);
		}
		
        timeVars.frames++;
    }

    SDL_FreeSurface(charset);
    SDL_FreeSurface(screen);
    SDL_DestroyTexture(scrtex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
    return 0;
};