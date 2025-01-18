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
    int x;
    int y;
} position_t;

typedef struct {
    int squareSize;
    int screenWidth;
    int screenHeight;
    int margin;
    int width;
    int height;
    int statsHeight;
    position_t statsBottom;
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
    char saveFilename[128];
    char scoreFilename[128];
    int lineHeight;
    int buffersize;
    int scoreboardAmount;
    int textMargin;
} config_t;

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

typedef enum {
    UP,
    RIGHT,
    DOWN,
    LEFT,
} directions_t;

typedef struct {
    position_t body[100];
    int length;
    directions_t direction[100];
    double speed;
    int score;
    char playerName[128];
} snake_t;

typedef struct {
    position_t position;
    float spawnTime;
    float displayDuration;
    int displayMode;
    int isActive;
} red_dot_t;

typedef struct {
    int black;
    int green;
    int red;
    int blue;
    int white;
} colors_t;

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
    char name[128];
    int score;
} score_entry_t;

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

typedef enum {
    JUSTIFY_LEFT,
    JUSTIFY_RIGHT,
    JUSTIFY_CENTER,
} justify_t;

void randomizePosition(position_t &pos, snake_t& snake, config_t config, red_dot_t redDot);
void spawnRedDot(red_dot_t& redDot, snake_t& snake, float worldTime, position_t blueDotPos, config_t config);
void updateScore(int& score, char a,  config_t config);
void saveGameState(snake_t snake, red_dot_t redDot,time_variables_t timeVars,position_t blueDot,config_t config);
void loadGameState(snake_t& snake, red_dot_t& redDot, time_variables_t& timeVars, position_t& blueDotPos, config_t config);
void updateScoreboard(snake_t snake, config_t config);
void drawTopStats(config_t config, colors_t colors, graphics_t graphics,time_variables_t timeVars, char* text, snake_t snake);

void DrawSpriteScaled(graphics_t graphics, int destX, int destY, int spriteX, int spriteY, int scale, config_t config, int color) {
    SDL_Rect srcRect = {
        .x = spriteX * 8,
        .y = spriteY * 8,
        .w = 8,
        .h = 8
    };
    
    SDL_Rect destRect = {
        .x = destX - config.squareSize/2,
        .y = destY - config.squareSize/2,
        .w = 4 * scale,
        .h = 4 * scale
    };

    SDL_Surface* temp = SDL_CreateRGBSurface(0, destRect.w, destRect.h, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    Uint8 r, g, b;
    SDL_GetRGB(color, graphics.screen->format, &r, &g, &b);
    SDL_SetSurfaceColorMod(graphics.cr, r, g, b);

    SDL_Rect scaledRect = {
        .x = 0,
        .y = 0,
        .w = destRect.w,
        .h = destRect.h
    };

    int result = SDL_BlitScaled(graphics.cr, &srcRect, temp, &scaledRect);
    result = SDL_BlitSurface(temp, NULL, graphics.screen, &destRect);
    SDL_FreeSurface(temp);
    SDL_SetSurfaceColorMod(graphics.cr, 255, 255, 255);
}

int readScoreboard(config_t config, score_entry_t* entries) {
    FILE* file = fopen(config.scoreFilename, "r");
    if (!file) {
        perror("Failed to open file");
        return 0;
    }

    int count = 0;
    while (count < config.scoreboardAmount && fscanf(file, "%49s %d", entries[count].name, &entries[count].score) == 2) {
        count++;
    }

    fclose(file);
    return count;
}

void updateScoreboard(snake_t snake, const config_t config) {
    score_entry_t entries[config.scoreboardAmount];
    int count = readScoreboard(config, entries);
    int found = 0;
    for (int i = 0; i < count; i++) {
        if (strcmp(entries[i].name, snake.playerName) == 0) {
            entries[i].score = snake.score;
            found = 1;
            break;
        }
    }
    if (!found) {
        if (count < config.scoreboardAmount) {
            strncpy(entries[count].name, snake.playerName, config.buffersize - 1);
            entries[count].name[config.buffersize - 1] = '\0';
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
                strncpy(entries[lowestIndex].name, snake.playerName, config.buffersize - 1);
                entries[lowestIndex].name[config.buffersize - 1] = '\0';
                entries[lowestIndex].score = snake.score;
            }
        }
    }



     FILE* file = fopen(config.scoreFilename, "w");
    if (!file) {
        perror("Failed to open file");
        return;
    }

    for (int i = 0; i < count; i++) {
        fprintf(file, "%s %d\n", entries[i].name, entries[i].score);
    }

    fclose(file);
    // snake.playerName[0]='\0';
}

void DrawSnakePart(graphics_t graphics, int x, int y, snake_part_t type, config_t config, directions_t direction, colors_t colors) {
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

    switch(type) {
        case SNAKE_HEAD:
            DrawSpriteScaled(graphics, x, y, 0, spriteY, 4, config, colors.green);
            break;
        case SNAKE_BODY_SMALL:
            DrawSpriteScaled(graphics, x, y, 2, spriteY, 4, config, colors.green);
            break;
        case SNAKE_BODY_BIG:
            DrawSpriteScaled(graphics, x, y, 1, spriteY, 4, config, colors.green);
            break;
        case SNAKE_TAIL:
            DrawSpriteScaled(graphics, x, y, 3, spriteY, 4, config, colors.green);
            break;
    }
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

void DrawSurface(graphics_t graphics, SDL_Surface *sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x - sprite->w / 2;
	dest.y = y - sprite->h / 2;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, graphics.screen, &dest);
};

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

void ResetGame(time_variables_t& timeVars, snake_t &snake, position_t& blueDotPos, red_dot_t& redDot, config_t config) {
	snake.length = config.minSnakeLength;
	for(int i = 0; i<snake.length; i++){
		snake.body[i].x = config.screenWidth / 2 - (i *config.squareSize);
    	snake.body[i].y  = config.screenHeight / 2;
	}
	snake.speed = 1;
	snake.direction[0] = RIGHT;
    timeVars.worldTime = 0;
    timeVars.timeAccumulator = 0;

    randomizePosition(blueDotPos,snake, config, redDot);
    redDot.isActive = 0;
    spawnRedDot(redDot, snake, timeVars.worldTime,blueDotPos, config); 
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
	int rightTurn = (snake.direction[0] + 1) % 4;
    int leftTurn = (snake.direction[0] + 3) % 4;
    
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
        snake.direction[0] = (directions_t) rightTurn;
        return;
    }
    
    if (isValidMove(leftPos, config)) {
        snake.direction[0] = (directions_t) leftTurn;
        return;
    }
}

void drawGrid(graphics_t graphics, Uint32 color, config_t config) {
    for (int x = config.margin; x < config.margin + config.width; x += config.squareSize) {
        DrawLine(graphics, x, config.margin, config.height, 0, 1, color);
    }
    
    for (int y = config.margin; y < config.margin + config.height; y += config.squareSize) {
        DrawLine(graphics, config.margin, y, config.width, 1, 0, color);
    }
}

void UpdateSnake(snake_t &snake, config_t config, red_dot_t& redDot) {
    for (int i = snake.length - 1; i > 0; --i) {
        snake.body[i] = snake.body[i - 1];
        snake.direction[i] = snake.direction[i-1];
    }

    if (snake.direction[0] == UP) {
        snake.body[0].y -= config.squareSize;
    } 
    else if (snake.direction[0] == RIGHT) {
        snake.body[0].x += config.squareSize;
    } 
    else if (snake.direction[0] == DOWN) {
        snake.body[0].y += config.squareSize;
    } 
    else if (snake.direction[0] == LEFT) {
        snake.body[0].x -= config.squareSize;
    }
    redDot.displayMode = (redDot.displayMode + 1)%4;
}

int checkSnakeCollision(snake_t* snake, int nextX, int nextY) {
    for (int i = 1; i < snake->length - 1; i++) {
        if (nextX == snake->body[i].x && nextY == snake->body[i].y) {
            return 1;
        }
    }
	return 0;
}

void randomizePosition(position_t &pos, snake_t& snake, config_t config, red_dot_t redDot) {
    int validPosition;
    int i;

    do {
        pos.x = config.margin + (rand() % (config.width / config.squareSize)) * config.squareSize + config.squareSize / 2;
        pos.y = config.margin + (rand() % (config.height / config.squareSize)) * config.squareSize + config.squareSize / 2;

        validPosition = 1;
        if(pos.x != redDot.position.x && pos.y != redDot.position.y) {
            for (i = 0; i < snake.length; i++) {
                if (pos.x == snake.body[i].x && pos.y == snake.body[i].y) {
                    validPosition = 0;
                    break;
                }
            }
        }
    } while (!validPosition);
}

void checkBlueDot(snake_t& snake, int nextX, int nextY, position_t &blue_dot_pos, config_t config, red_dot_t redDot) {
	if (nextX == blue_dot_pos.x && nextY == blue_dot_pos.y) {
        if(snake.length < config.maxSnakeLength){
		    snake.length++;
        }
        updateScore(snake.score, 'b', config);
		randomizePosition(blue_dot_pos, snake, config, redDot);
	}
}
int isScoreGoodEnough(int score, config_t config) {
    score_entry_t entries[config.scoreboardAmount];
    int count = readScoreboard(config, entries);

    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (entries[j].score > entries[i].score) {
                score_entry_t temp = entries[i];
                entries[i] = entries[j];
                entries[j] = temp;
            }
        }
    }

    if (count < config.scoreboardAmount) {
        return 1;
    }

    if (score > entries[count - 1].score) {
        return 1;
    }

    return 0;
}


void drawGameOver(graphics_t graphics, int score, config_t config, colors_t colors, snake_t snake) {
    int windowWidth = config.screenWidth / 2;
    int windowHeight = config.screenHeight/ 2;
    int windowX = (config.screenWidth - windowWidth) / 2;
    int windowY = (config.screenHeight - windowHeight)/ 2;
    
    DrawRectangle(graphics, 0, 0, config.screenWidth, config.screenHeight, 
                 SDL_MapRGB(graphics.screen->format, 0x00, 0x00, 0x00), 
                 SDL_MapRGB(graphics.screen->format, 0x00, 0x00, 0x00));
    
    DrawRectangle(graphics, windowX, windowY, windowWidth, windowHeight, 
                 colors.red, colors.black);
    
    char text[config.buffersize];
    snprintf(text, sizeof(text), "GAME OVER!");
    DrawString(graphics, windowX , 
              windowY + config.lineHeight, text, config.screenWidth/2, JUSTIFY_CENTER);

    snprintf(text, sizeof(text), "Final Score: %d, player %s", score, snake.playerName);
    DrawString(graphics, windowX , 
              windowY + 2*config.lineHeight, text, config.screenWidth/2, JUSTIFY_CENTER);

    snprintf(text, sizeof(text), "SCOREBOARD");
    DrawString(graphics, windowX + (windowWidth - strlen(text) * 8) / 2, 
               windowY + 3*config.lineHeight, text, config.screenWidth/2, JUSTIFY_LEFT);

    score_entry_t entries[config.scoreboardAmount];
    int count = readScoreboard(config, entries);

    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (entries[j].score > entries[i].score) {
                score_entry_t temp = entries[i];
                entries[i] = entries[j];
                entries[j] = temp;
            }
        }
    }

    for (int i = 0; i < count && i < config.scoreboardAmount; i++) {
        snprintf(text, sizeof(text), "%d. %s - %d", i + 1, entries[i].name, entries[i].score);
        DrawString(graphics, windowX + (windowWidth - strlen(text) * 8) / 2, 
                   windowY + config.lineHeight * (i + 5), text, config.screenWidth/2, JUSTIFY_LEFT);
    }
 
    snprintf(text, sizeof(text), "Press N for New Game");
    DrawString(graphics, windowX , 
              windowY + 12*config.lineHeight, text, config.screenWidth/2, JUSTIFY_CENTER);

    snprintf(text, sizeof(text), "Press I to load the last save");
    DrawString(graphics, windowX , 
              windowY + 13*config.lineHeight, text, config.screenWidth/2, JUSTIFY_CENTER);

    snprintf(text, sizeof(text), "Press ESC to Quit");
    DrawString(graphics, windowX , 
              windowY + 15*config.lineHeight, text, config.screenWidth/2, JUSTIFY_CENTER);
}

void handleGameOverInput(graphics_t graphics, int& quit, game_state_t& gameState, snake_t& snake,  time_variables_t& timeVars, position_t& blueDotPos, red_dot_t& redDot, config_t config) {
    while(SDL_PollEvent(&graphics.event)) {
        switch(graphics.event.type) {
            case SDL_KEYDOWN:
                if(graphics.event.key.keysym.sym == SDLK_ESCAPE) {
                    quit = 1;
                }
                else if(graphics.event.key.keysym.sym == SDLK_n) {
                    ResetGame(timeVars, snake, blueDotPos, redDot, config);
                    gameState = GAME_RUNNING;
                }
                // else if(graphics.event.key.keysym.sym == SDLK_b) {
                //     gameState = SCOREBOARD;
                // }
                else if(graphics.event.key.keysym.sym == SDLK_i) {
                    loadGameState(snake, redDot,timeVars,blueDotPos, config);
                    gameState = GAME_RUNNING;
                }
                break;
            case SDL_QUIT:
                // updateScoreboard(snake, config);
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

void spawnRedDot(red_dot_t& redDot, snake_t& snake, float worldTime, position_t blueDotPos, config_t config) {
    if (redDot.isActive) return;

    position_t newPos;
    int validPosition = 0;
    while (!validPosition) {
        newPos.x = config.margin + (rand() % (config.width / config.squareSize)) * config.squareSize + config.squareSize / 2;
        newPos.y = config.margin + (rand() % (config.width / config.squareSize)) * config.squareSize + config.squareSize / 2;
        validPosition = 1;
        if (newPos.x != blueDotPos.x && newPos.y != blueDotPos.y) {
            for (int i = 0; i < snake.length; i++) {
                if (newPos.x == snake.body[i].x && newPos.y == snake.body[i].y) {
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

void checkRedDot(snake_t& snake, red_dot_t& redDot, config_t config) {
    if (!redDot.isActive) return;

    if (redDot.position.x == snake.body[0].x && 
        snake.body[0].y == redDot.position.y) {
        if (rand() % 2 == 0) {
            int newLength = snake.length - config.shortenLength;
            if (newLength < config.minSnakeLength){
                newLength = config.minSnakeLength;
            } 
            snake.length = newLength;
        } 
        else {
            if(snake.speed - config.slowdownFactor>1){
                snake.speed -= config.slowdownFactor;
            }
            else {
                snake.speed = config.defaultSnakeSpeed;
            }
        }
        updateScore(snake.score, 'r', config);
        redDot.isActive = 0;
    }
}

void drawProgressBar(graphics_t graphics, red_dot_t* redDot, float worldTime, colors_t colors, config_t config) { // might be a bad progress bar height
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
    DrawRectangle(graphics, borderRect.x, borderRect.y, 
                  borderRect.w, borderRect.h, colors.green, colors.black);

    float fillHeight = (remainingTime / redDot->displayDuration) * (config.progressBarHeight/3);
    SDL_Rect fillRect = {
        borderRect.x + 2,
        borderRect.y + 2 + (int)((config.progressBarHeight - 4) - fillHeight),
        borderRect.w - 4,
        (int)fillHeight
    };
    SDL_FillRect(graphics.screen, &fillRect, colors.green);

    char timeText[32];
    snprintf(timeText, sizeof(timeText), "%.1fs", remainingTime);

    DrawString(graphics, borderRect.x, borderRect.y + config.progressBarHeight + 10, timeText, config.progressBarWidth, JUSTIFY_CENTER);
}

void updateBonusSystem(snake_t&snake, red_dot_t& redDot, float worldTime, position_t blueDotPos, config_t config) {
    if (!redDot.isActive && worldTime >= redDot.spawnTime) {
        spawnRedDot(redDot, snake, worldTime, blueDotPos, config);
        float spawnInterval = config.minSpawnInterval + (rand()%2) * (config.maxSpawnInterval- config.minSpawnInterval);
        redDot.spawnTime = worldTime + spawnInterval;
    }

    checkRedDot(snake, redDot, config);

    if (redDot.isActive && (worldTime - redDot.spawnTime) >= redDot.displayDuration) {
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

        if (snake.direction[0] == 0) {
            stepY -= config.squareSize;
        }
        else if (snake.direction[0] == 1) {
            stepX += config.squareSize;
        }
        else if (snake.direction[0] == 2) {
            stepY += config.squareSize;
        }
        else if (snake.direction[0] == 3) {
            stepX -= config.squareSize;
        }

        int nextX = snake.body[0].x + stepX;
        int nextY = snake.body[0].y + stepY;
        position_t nextPos = { .x = nextX, .y = nextY };

        if (!isValidMove(nextPos, config)) {
            autoTurn(snake, config);
        }

        if (checkSnakeCollision(&snake, nextX, nextY)) {
            if(isScoreGoodEnough(snake.score, config)){
                gameState = ADD_SCORE;
            }
            else{
                gameState = GAME_OVER;
            }
            // updateScoreboard(snake, config); //this is the spot
            return;
        }

        checkBlueDot(snake, nextX, nextY, blueDotPos, config, redDot);
        updateBonusSystem(snake, redDot, timeVars.worldTime, blueDotPos, config);

        UpdateSnake(snake, config, redDot);

        timeVars.timeAccumulator -= (config.squareSize / (snake.speed * config.speedFix));
    }
}

void handleGameRunningInput(graphics_t graphics,int& quit, snake_t& snake,time_variables_t& timeVars, position_t& blueDotPos, red_dot_t& redDot,config_t config) {
    while(SDL_PollEvent(&graphics.event)) {
            switch(graphics.event.type) {
                case SDL_KEYDOWN:
                    if(graphics.event.key.keysym.sym == SDLK_ESCAPE) quit = 1;
                    else if(graphics.event.key.keysym.sym == SDLK_n) ResetGame(timeVars, snake, blueDotPos, redDot, config);
                    else if(graphics.event.key.keysym.sym == SDLK_s) saveGameState(snake, redDot,timeVars,blueDotPos, config);
                    else if(graphics.event.key.keysym.sym == SDLK_i) loadGameState(snake, redDot,timeVars,blueDotPos, config);
                    else if(graphics.event.key.keysym.sym == SDLK_UP){
						snake.direction[0] = UP;
					} 
                    else if(graphics.event.key.keysym.sym == SDLK_RIGHT){
						snake.direction[0] = RIGHT;
					} 
                    else if(graphics.event.key.keysym.sym == SDLK_DOWN){
						snake.direction[0] = DOWN;
					} 
                    else if(graphics.event.key.keysym.sym == SDLK_LEFT){
						snake.direction[0] = LEFT;
					}
                    break;
                case SDL_QUIT:
                    // updateScoreboard(snake, config);
                    quit = 1;
                    break;
            };
        }
}

void renderScren (graphics_t graphics){
    SDL_UpdateTexture(graphics.scrtex, NULL, graphics.screen->pixels, graphics.screen->pitch);
    SDL_RenderCopy(graphics.renderer, graphics.scrtex, NULL, NULL);
    SDL_RenderPresent(graphics.renderer);
}

void drawBottomStats (config_t config, colors_t colors, graphics_t graphics,char* text) {
    DrawRectangle(graphics, config.textMargin, config.statsBottom.y,config.statsBottom.x, config.statsHeight, colors.green, colors.black);
    snprintf(text,config.screenWidth, "requirments:");
    DrawString(graphics, 2*config.textMargin, config.statsBottom.y + config.lineHeight, text, config.screenWidth/2, JUSTIFY_LEFT);
    snprintf(text,config.screenWidth, "1, 2, 3, 4, A, B, C, D, E, F, G");
    DrawString(graphics, 2*config.textMargin,config.statsBottom.y+2*config.lineHeight, text, config.screenWidth/2, JUSTIFY_LEFT);
    snprintf(text, config.screenWidth, "ESC - quit, N - new game");
    DrawString(graphics, config.screenWidth/2 - 2*config.textMargin, config.statsBottom.y + config.lineHeight, text, config.screenWidth/2, JUSTIFY_RIGHT);
    snprintf(text, config.screenWidth, "S - save, I - load last save");
    DrawString(graphics, config.screenWidth/2 - 2*config.textMargin,config.statsBottom.y+2*config.lineHeight, text, config.screenWidth/2, JUSTIFY_RIGHT);
}

void drawGameElements (config_t config, colors_t colors, graphics_t graphics, snake_t snake, red_dot_t& redDot,position_t blueDotPos, double worldTime) {
    DrawSnakePart(graphics, snake.body[0].x, snake.body[0].y, SNAKE_HEAD, config, snake.direction[0], colors);
    for (int i = 1; i < snake.length - 1; i++) {
        snake_part_t partType = (i % 2 == (redDot.displayMode%2)) ? SNAKE_BODY_SMALL : SNAKE_BODY_BIG;
        DrawSnakePart(graphics, snake.body[i].x, snake.body[i].y, partType, config, snake.direction[i], colors);
    }

    DrawSnakePart(graphics, snake.body[snake.length-1].x, snake.body[snake.length-1].y, SNAKE_TAIL, config, snake.direction[snake.length-1], colors);

    DrawSpriteScaled(graphics, blueDotPos.x, blueDotPos.y,  5, 8 + redDot.displayMode%4, 4, config, colors.blue);
    if(redDot.isActive == 1){
        SDL_SetSurfaceColorMod(graphics.cr, 255, 0, 0);
        DrawSpriteScaled(graphics, redDot.position.x, redDot.position.y,  5, 8 + redDot.displayMode%4, 4, config, colors.red);
        SDL_SetSurfaceColorMod(graphics.cr, 255, 255, 255);
        drawProgressBar(graphics, &redDot,worldTime,colors, config);
    }
}

void drawTopStats(config_t config, colors_t colors, graphics_t graphics, time_variables_t timeVars, char* text, snake_t snake) {
    DrawRectangle(graphics, config.textMargin,config.textMargin, config.statsBottom.x, config.statsHeight, colors.green, colors.black);
    snprintf(text, config.screenWidth/3, "time = %.1lf s ", timeVars.worldTime);
    DrawString(graphics,2*config.textMargin, config.lineHeight, text, config.screenWidth/2, JUSTIFY_LEFT);
    snprintf(text, config.screenWidth/3, "score: %d", snake.score);
    DrawString(graphics, 2*config.textMargin, config.lineHeight*2, text, config.screenWidth/2, JUSTIFY_LEFT);
    snprintf(text, config.screenWidth/3, "length: %d", snake.length);
    DrawString(graphics,config.screenWidth/2 - 2*config.textMargin, config.lineHeight, text, config.screenWidth/2, JUSTIFY_RIGHT);
    snprintf(text, config.screenWidth/3, "speed: %.1f", snake.speed);
    DrawString(graphics,config.screenWidth/2 - 2*config.textMargin, config.lineHeight*2, text, config.screenWidth/2, JUSTIFY_RIGHT);
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

void saveGameState(snake_t snake, red_dot_t redDot,time_variables_t timeVars,position_t blueDot,config_t config) {
    FILE* file = fopen(config.saveFilename, "w");
    if (!file) {
        perror("Error opening file for saving");
        return;
    }

    fprintf(file, "%d\n", snake.length);
    fprintf(file, "%d\n", snake.direction[0]);
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

int loadGameFromFile(snake_t& snake, red_dot_t& redDot, time_variables_t& timeVars, position_t& blueDotPos, config_t config) {
    FILE* file = fopen(config.saveFilename, "r");
    if (!file) {
        perror("Error opening file for loading");
        return 0;
    }
    ResetGame(timeVars, snake, blueDotPos,redDot, config);
    if (fscanf(file, "%d", &snake.length) != 1) return 0;
    if (fscanf(file, "%d", &snake.direction[0]) != 1) return 0;
    if (fscanf(file, "%lf", &snake.speed) != 1) return 0;
    if (fscanf(file, "%d", &snake.score) != 1) return 0;
    for (int i = 0; i < snake.length; i++) {
        if (fscanf(file, "%d %d", &snake.body[i].x, &snake.body[i].y) != 2) return 0;
    }

    if (fscanf(file, "%d %d", &redDot.position.x, &redDot.position.y) != 2) return 0;
    if (fscanf(file, "%f", &redDot.spawnTime) != 1) return 0;
    if (fscanf(file, "%f", &redDot.displayDuration) != 1) return 0;
    if (fscanf(file, "%d", &redDot.isActive) != 1) return 0;

    if (fscanf(file, "%lf", &timeVars.worldTime) != 1) return 0;
    if (fscanf(file, "%lf", &timeVars.nextIncrease) != 1) return 0;

    if (fscanf(file, "%d %d", &blueDotPos.x, &blueDotPos.y) != 2) return 0;

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

void loadGameState(snake_t& snake, red_dot_t& redDot, time_variables_t& timeVars, position_t& blueDotPos, config_t config){
    if (isFileNotEmpty(config.saveFilename)) {
        if (loadGameFromFile(snake, redDot, timeVars, blueDotPos, config)) {
            printf("Game state loaded successfully!\n");
        } else {
            printf("Failed to load game state.\n");
        }
    } else {
        printf("No data available to load.\n");
    }
}

void drawAddScore(graphics_t graphics, config_t config, colors_t colors,char* buffer, snake_t snake) {
    int windowWidth = config.screenWidth / 2;
    int windowHeight = config.screenHeight / 3;
    int windowX = (config.screenWidth - windowWidth) / 2;
    int windowY = (config.screenHeight - windowHeight) / 2;

    DrawRectangle(graphics, 0, 0, config.screenWidth, config.screenHeight, 
                  SDL_MapRGB(graphics.screen->format, 0x00, 0x00, 0x00), 
                  SDL_MapRGB(graphics.screen->format, 0x00, 0x00, 0x00));

    DrawRectangle(graphics, windowX, windowY, windowWidth, windowHeight,colors.red, colors.black);

    char text[128];
    snprintf(text, sizeof(text), "WOW! THAT'S A GREAT SCORE %s!", snake.playerName);
    DrawString(graphics, windowX , 
               windowY + config.lineHeight, text, config.screenWidth/2, JUSTIFY_CENTER);

    snprintf(text, sizeof(text), "Enter your name to add it to scoreboard:");
    DrawString(graphics, windowX , 
               windowY + config.lineHeight*2, text, config.screenWidth/2, JUSTIFY_CENTER);

    snprintf(text, sizeof(text), "%s", buffer);
    DrawString(graphics, windowX , 
               windowY + config.lineHeight*4, text, config.screenWidth/2, JUSTIFY_CENTER);

    DrawLine(graphics, windowWidth - windowWidth/4, windowY + config.lineHeight*5, windowWidth/2, 1, 0, colors.blue);


    snprintf(text, sizeof(text), "Press Enter to start typing");
    DrawString(graphics, windowX , 
               windowY + config.lineHeight*8, text, config.screenWidth/2, JUSTIFY_CENTER);

    snprintf(text, sizeof(text), "Press N to start a new game");
    DrawString(graphics, windowX , 
               windowY + config.lineHeight*9, text, config.screenWidth/2, JUSTIFY_CENTER);
    snprintf(text, sizeof(text), "Press ESC to Quit");
    DrawString(graphics, windowX , 
               windowY + config.lineHeight*10, text, config.screenWidth/2, JUSTIFY_CENTER);
}

void handleAddScore(graphics_t graphics, int& quit, game_state_t& gameState, snake_t& snake, time_variables_t& timeVars, position_t& blueDotPos, red_dot_t& redDot, config_t config, char* buffer) {
    static int isTyping = 0;          // potential change in future
    while (SDL_PollEvent(&graphics.event)) {
        switch (graphics.event.type) {
            case SDL_QUIT:
                quit = 1;
                break;
            case SDL_KEYDOWN:
                if (graphics.event.key.keysym.sym == SDLK_ESCAPE) {
                    quit = 1;
                }
                // else if (graphics.event.key.keysym.sym == SDLK_n) {
                //    ResetGame(timeVars, snake, blueDotPos, redDot, config);
                //    gameState = GAME_RUNNING;
                // } 
                else if (graphics.event.key.keysym.sym == SDLK_RETURN) {
                    if (isTyping) {
                        strncpy(snake.playerName, buffer, config.buffersize - 1);
                        snake.playerName[config.buffersize - 1] = '\0';
                        isTyping = 0;
                        updateScoreboard(snake, config);
                        buffer[0] = '\0';
                        // ResetGame(timeVars, snake, blueDotPos, redDot, config);
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
                if (isTyping && strlen(buffer) < config.buffersize - 1) {
                    strcat(buffer, graphics.event.text.text);
                }
                break;
            default:
                break;
        }
    }
}

// void drawTopScores(graphics_t graphics, config_t config, colors_t colors) {
//     int windowWidth = config.screenWidth / 2;
//     int windowHeight = config.screenHeight / 2;
//     int windowX = (config.screenWidth - windowWidth) / 2;
//     int windowY = (config.screenHeight - windowHeight);

//     DrawRectangle(graphics, 0, 0, config.screenWidth, config.screenHeight, 
//                   SDL_MapRGB(graphics.screen->format, 0x00, 0x00, 0x00), 
//                   SDL_MapRGB(graphics.screen->format, 0x00, 0x00, 0x00));

//     DrawRectangle(graphics, windowX, windowY, windowWidth, windowHeight, colors.red, colors.black);

//     char text[config.buffersize];
//     snprintf(text, sizeof(text), "SCOREBOARD");
//     DrawString(graphics, windowX + (windowWidth - strlen(text) * 8) / 2, 
//                windowY + config.lineHeight, text, config.screenWidth/2, JUSTIFY_LEFT);

//     score_entry_t entries[config.maxEntries];
//     int count = readScoreboard(config, entries);

//     for (int i = 0; i < count - 1; i++) {
//         for (int j = i + 1; j < count; j++) {
//             if (entries[j].score > entries[i].score) {
//                 score_entry_t temp = entries[i];
//                 entries[i] = entries[j];
//                 entries[j] = temp;
//             }
//         }
//     }

//     for (int i = 0; i < count && i < config.scoreboardAmount; i++) {
//         snprintf(text, sizeof(text), "%d. %s - %d", i + 1, entries[i].name, entries[i].score);
//         DrawString(graphics, windowX + (windowWidth - strlen(text) * 8) / 2, 
//                    windowY + config.lineHeight * (i + 2), text, config.screenWidth/2, JUSTIFY_LEFT);
//     }

//     snprintf(text, sizeof(text), "Press ESC to quit");
//     DrawString(graphics, windowX + (windowWidth - strlen(text) * 8) / 2, 
//                windowY + windowHeight - config.lineHeight * 2, text, config.screenWidth/2, JUSTIFY_LEFT);
//     snprintf(text, sizeof(text), "Press n to start new game");
//     DrawString(graphics, windowX + (windowWidth - strlen(text) * 8) / 2, 
//                windowY + windowHeight - config.lineHeight * 3, text, config.screenWidth/2, JUSTIFY_LEFT);
// }

// void handleScoreboardInput(graphics_t graphics, int& quit, game_state_t& gameState, snake_t& snake,  time_variables_t& timeVars, position_t& blueDotPos, red_dot_t& redDot, config_t config) {
//     while(SDL_PollEvent(&graphics.event)) {
//         switch(graphics.event.type) {
//             case SDL_KEYDOWN:
//                 if(graphics.event.key.keysym.sym == SDLK_ESCAPE) {
//                     quit = 1;
//                 }
//                 else if(graphics.event.key.keysym.sym == SDLK_n) {
//                     ResetGame(timeVars, snake, blueDotPos, redDot, config);
//                     gameState = GAME_RUNNING;
//                 }
//                 break;
//             case SDL_QUIT:
//                 updateScoreboard(snake, config);
//                 quit = 1;
//                 break;
//         }
//     }
// }

void handleGraphicsSetup (graphics_t& graphics, config_t config){
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize(graphics.renderer, config.screenWidth, config.screenHeight);
    SDL_SetRenderDrawColor(graphics.renderer, 0, 0, 0, 255);

    SDL_SetWindowTitle(graphics.window, "snek");
    graphics.screen = SDL_CreateRGBSurface(0, config.screenWidth, config.screenHeight, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    graphics.scrtex = SDL_CreateTexture(graphics.renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, config.screenWidth, config.screenHeight);

    SDL_ShowCursor(SDL_DISABLE);
    graphics.cr = SDL_LoadBMP("./cs8x8c.bmp");
    if (!graphics.cr) {
        printf("SDL_LoadBMP(cs8x8a.bmp) error: %s\n", SDL_GetError());
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
    
    config_t config= {
        .squareSize = 16,
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
        .progressBarWidth = 16,
        .progressBarHeight = 100,
        .redDotBonus = 150,
        .blueDotBonus = 100,
        .speedUpTime = 3,
	    .speedUpAmount= 0.1,
        .buffersize = 128,
        .saveFilename = "game_state.txt",
        .scoreFilename = "scoreboard.txt",
        .lineHeight = 20,
        .scoreboardAmount = 3,
        .textMargin = 8,
    };

        config.screenWidth = 45 * config.squareSize;
        config.screenHeight = 45 * config.squareSize;
        config.margin = 5 * config.squareSize;
        config.width = config.screenWidth - (2 * config.margin);
        config.height = config.screenHeight - (2 * config.margin);
        config.statsHeight = 4 * config.squareSize;

        config.statsBottom.x = config.screenWidth - 2*config.textMargin;
        config.statsBottom.y = config.screenHeight - config.statsHeight;

	game_state_t gameState = GAME_RUNNING;
    int quit, rc;
    // int stepX, stepY;
    char buffer[128];

    time_variables_t timeVars = {
        .nextIncrease = config.speedUpAmount,
        .worldTime = 0,
        .t1 = 0,
        .t2 = 0,
    };
    
    snake_t snake = {
        .direction[0] = RIGHT,
        .length = config.defaultSnakeLength,
        .speed = config.defaultSnakeSpeed,
    };
    

    red_dot_t redDot = {
        .isActive = 0,
        .displayDuration = 5,
        .displayMode = 1,
    };    

	position_t blueDotPos;
    graphics_t graphics;
   
    ResetGame(timeVars, snake, blueDotPos, redDot, config);

    if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }

    rc = SDL_CreateWindowAndRenderer(config.screenWidth, config.screenHeight, 0, &graphics.window, &graphics.renderer);

    if(rc != 0) {
        SDL_Quit();
        printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
        return 1;
    }
    handleGraphicsSetup(graphics, config);

    char text[256];
    colors_t colors = {
        .black = SDL_MapRGB(graphics.screen->format, 0x00, 0x00, 0x00),
        .green = SDL_MapRGB(graphics.screen->format, 0x00, 0xFF, 0x00),
        .red = SDL_MapRGB(graphics.screen->format, 0xFF, 0x00, 0x00),
        .blue = SDL_MapRGB(graphics.screen->format, 0x11, 0x11, 0xCC),
        .white = SDL_MapRGB(graphics.screen->format, 0xFF, 0xFF, 0xFF),
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
            SDL_FillRect(graphics.screen, NULL, colors.black);
            DrawRectangle(graphics, config.margin, config.margin, config.width, config.height, colors.green, colors.black); 
            drawGrid(graphics, SDL_MapRGB(graphics.screen->format, 0x33, 0x33, 0x33), config);
            drawGameElements(config,colors,graphics,snake,redDot,blueDotPos,timeVars.worldTime);
            drawTopStats(config,colors,graphics,timeVars,text, snake);
            drawBottomStats(config,colors,graphics,text);
            renderScren(graphics);
            handleGameRunningInput(graphics, quit, snake, timeVars, blueDotPos, redDot, config);
		}
		else if (gameState == GAME_OVER) {
			SDL_FillRect(graphics.screen, NULL, colors.black);
			drawGameOver(graphics, snake.score, config, colors, snake);
            // drawTopScores(graphics,config,colors);
			renderScren(graphics);
			handleGameOverInput(graphics, quit, gameState, snake, timeVars, blueDotPos, redDot, config);
		}
        // else if (gameState == SCOREBOARD) {
		// 	SDL_FillRect(graphics.screen, NULL, colors.black);
		// 	// drawTopScores(graphics, config, colors);
		// 	renderScren(graphics);
		// 	handleScoreboardInput(graphics, quit, gameState, snake, timeVars, blueDotPos, redDot, config);
		// }
        else if (gameState == ADD_SCORE) {
			SDL_FillRect(graphics.screen, NULL, colors.black);
            drawAddScore(graphics, config, colors, buffer, snake);
            handleAddScore(graphics, quit, gameState, snake,timeVars, blueDotPos, redDot, config, buffer);
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