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

void randomizePosition(position_t &pos, snake_t *snake, config_t config);
void spawnRedDot(red_dot_t* redDot, snake_t* snake, float worldTime, position_t blueDotPos, config_t config);
void updateScore(int& score, char a,  config_t config);

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

void ResetGame(double &worldTime, snake_t &snake, position_t& blueDotPos, red_dot_t& redDot, int &score, config_t config) {
	snake.length = config.minSnakeLength;
	for(int i = 0; i<snake.length; i++){
		snake.body[i].x = config.screenWidth / 2 - (i *config.squareSize);
    	snake.body[i].y  = config.screenHeight / 2;
	}
	snake.speed = 1;
	snake.direction = 1;
    worldTime = 0;

    randomizePosition(blueDotPos,&snake, config);
    spawnRedDot(&redDot,&snake,worldTime,blueDotPos, config);
    score = 0;
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

void randomizePosition(position_t &pos, snake_t *snake, config_t config) {
    int validPosition;
    int i;

    do {
        pos.x = config.margin + (rand() % (config.width / config.squareSize)) * config.squareSize + config.squareSize / 2;
        pos.y = config.margin + (rand() % (config.height / config.squareSize)) * config.squareSize + config.squareSize / 2;

        validPosition = 1;
        for (i = 0; i < snake->length; i++) {
            if (pos.x == snake->body[i].x && pos.y == snake->body[i].y) {
                validPosition = 0;
                break;
            }
        }
    } while (!validPosition);
}

void checkBlueDot(snake_t* snake, int nextX, int nextY, position_t &blue_dot_pos,int& score, config_t config) {
	if (nextX == blue_dot_pos.x && nextY == blue_dot_pos.y &&snake->length <=config.maxSnakeLength) {
		snake->length++;
        updateScore(score, 'b', config);
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

void handleGameOverInput(SDL_Event& event, int& quit, game_state_t& gameState, snake_t& snake,  double& worldTime, position_t blueDotPos, red_dot_t redDot,int& score,config_t config) {
    while(SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_KEYDOWN:
                if(event.key.keysym.sym == SDLK_ESCAPE) {
                    quit = 1;
                }
                else if(event.key.keysym.sym == SDLK_n) {
                    ResetGame(worldTime, snake, blueDotPos, redDot, score, config);
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

void spawnRedDot(red_dot_t* redDot, snake_t* snake, float worldTime, position_t blueDotPos, config_t config) { // add a check for no blue and red dot collision!
    if (redDot->isActive) return;

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

    redDot->position = newPos;
    redDot->spawnTime = worldTime;
    redDot->isActive = 1;
}

void checkRedDot(snake_t* snake, red_dot_t* redDot, int& score, config_t config) {
    if (!redDot->isActive) return;

    if (redDot->position.x == snake->body[0].x && 
        snake->body[0].y == redDot->position.y) {
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
        updateScore(score, 'r', config);
        redDot->isActive = 0;
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

void updateBonusSystem(snake_t* snake, red_dot_t* redDot, float worldTime, position_t blueDotPos,int& score, config_t config) {
    if (!redDot->isActive && worldTime >= redDot->spawnTime) {
        spawnRedDot(redDot, snake, worldTime, blueDotPos, config);
        float spawnInterval = config.minSpawnInterval + 
            (rand()%2) * 
            (config.maxSpawnInterval- config.minSpawnInterval);
        redDot->spawnTime = worldTime + spawnInterval;
    }

    checkRedDot(snake, redDot, score, config);

    if (redDot->isActive && 
        (worldTime - redDot->spawnTime) >= redDot->displayDuration) {
        redDot->isActive = 0;
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

void processSnakeMovement(snake_t& snake, const config_t& config, float& timeAccumulator, position_t& blueDotPos, int& score, red_dot_t& redDot, int worldTime, game_state_t& gameState) {
    while (timeAccumulator >= (config.squareSize / (snake.speed * config.speedFix))) {
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

        checkBlueDot(&snake, nextX, nextY, blueDotPos, score, config);
        updateBonusSystem(&snake, &redDot, worldTime, blueDotPos, score, config);

        UpdateSnake(snake, config);

        timeAccumulator -= (config.squareSize / (snake.speed * config.speedFix));
    }
}

void handleGameRunningInput(SDL_Event &event,int& quit, snake_t& snake,  double& worldTime, position_t blueDotPos, red_dot_t redDot,int& score,config_t config) {
    while(SDL_PollEvent(&event)) {
            switch(event.type) {
                case SDL_KEYDOWN:
                    if(event.key.keysym.sym == SDLK_ESCAPE) quit = 1;
                    else if(event.key.keysym.sym == SDLK_n) ResetGame(worldTime, snake, blueDotPos, redDot, score, config);
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

void drawBottomStats (config_t config, colors_t colors, SDL_Surface *screen,int& score, snake_t snake, SDL_Surface* charset,char* text) {
    DrawRectangle(screen, config.x, config.y, config.statsWidth, config.statsHeight, colors.black, colors.green);

    snprintf(text, sizeof(text), "Score: %d, speed: %f", score, snake.speed);
    DrawString(screen, config.x + (config.statsWidth/2) - config.margin, config.y + config.margin/2, text, charset);

    snprintf(text, sizeof(text), "Length: %d", snake.length);
    DrawString(screen, config.x + config.statsWidth - config.margin, config.y + config.margin/2, text, charset);

    snprintf(text, sizeof(text), "ESC - quit, N - new game");
    DrawString(screen, config.x, config.y + config.margin/2, text, charset);
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

void drawTopStats(config_t config, colors_t colors, SDL_Surface *screen, SDL_Surface* charset, double& fpsTimer, int delta, double fps, int frames, double worldTime, char* text) {
    
    fpsTimer += delta;
    if(fpsTimer > 0.5) {
        fps = frames * 2;
        frames = 0;
        fpsTimer -= 0.5;
    }
    DrawRectangle(screen, 4, 4, config.screenWidth - 8, 36, colors.green, colors.black);
    snprintf(text, sizeof(text), "czas trwania = %.1lf s  %.0lf klatek / s", worldTime, fps);
    DrawString(screen,10 , 20, text, charset);
    snprintf(text, sizeof(text), "wymagania: 1, 2, 3, 4, A, B");
    DrawString(screen,2*config.screenWidth/3 ,20, text, charset);
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
        .x = 4,
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
    };

	srand(time(NULL));
	game_state_t gameState = GAME_RUNNING;
    int t1, t2, quit, frames, rc;
    double delta, worldTime, fpsTimer, fps;
    int stepX, stepY;
	double speedUpTime = 3;
	double speedUpAmount= 0.1;
	double nextIncrease = speedUpAmount;
    int score = 0;

    snake_t snake;
    snake.direction = 1;
    snake.length = config.defaultSnakeLength;
    snake.speed = config.defaultSnakeSpeed;

	position_t blueDotPos;
	randomizePosition(blueDotPos, &snake, config);

    red_dot_t redDot = {
        .isActive = 0,
        .displayDuration = 5,
    };    
    ResetGame(worldTime, snake, blueDotPos, redDot, score, config);


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

    char text[128];
    colors_t colors = {
        .black = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00),
        .green = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00),
        .red = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00),
        .blue = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC),
    };
    

    t1 = SDL_GetTicks();

    frames = 0;
    fpsTimer = 0;
    fps = 0;
    quit = 0;
    ResetGame(worldTime, snake, blueDotPos, redDot, score, config);

    static float timeAccumulator = 0.0f;

    while(!quit) {
        t2 = SDL_GetTicks();
        delta = (t2 - t1) * 0.001f;
        t1 = t2;
		worldTime += delta;
		if (gameState == GAME_RUNNING) {
            timeAccumulator += delta;

            if(worldTime > nextIncrease){
                snake.speed +=speedUpAmount;
                nextIncrease += speedUpTime;
            }

            processSnakeMovement(snake, config, timeAccumulator, blueDotPos, score, redDot, worldTime, gameState);

            SDL_FillRect(screen, NULL, colors.black);
            DrawRectangle(screen, config.margin, config.margin, config.width, config.height, colors.green, colors.black); 
            drawGrid(screen, SDL_MapRGB(screen->format, 0x33, 0x33, 0x33), config);
            drawGameElements(config,colors,screen,snake,blue_dot,red_dot,blueDotPos,eti,ite,redDot,charset,worldTime,text);

            // draw top stats
            fpsTimer += delta;
            if(fpsTimer > 0.5) {
                fps = frames * 2;
                frames = 0;
                fpsTimer -= 0.5;
            }
            DrawRectangle(screen, 4, 4, config.screenWidth - 8, 36, colors.green, colors.black);
            snprintf(text, sizeof(text), "czas trwania = %.1lf s  %.0lf klatek / s", worldTime, fps);
            DrawString(screen,10 , 20, text, charset);
            snprintf(text, sizeof(text), "wymagania: 1, 2, 3, 4, A, B");
            DrawString(screen,2*config.screenWidth/3 ,20, text, charset);

            // drawBottomStats(config,colors,screen,score,snake,charset,text);
            DrawRectangle(screen, config.x, config.y, config.statsWidth, config.statsHeight, colors.green, colors.black);
            snprintf(text, sizeof(text), "Score: %d, speed: %f", score, snake.speed);
            DrawString(screen, config.x + (config.statsWidth/2) - config.margin, config.y + config.margin/2, text, charset);
            snprintf(text, sizeof(text), "Length: %d", snake.length);
            DrawString(screen, config.x + config.statsWidth - config.margin, config.y + config.margin/2, text, charset);
            snprintf(text, sizeof(text), "ESC - quit, N - new game");
            DrawString(screen, config.x, config.y + config.margin/2, text, charset);

            renderScren(scrtex,screen,renderer);
            handleGameRunningInput(event, quit, snake, worldTime, blueDotPos, redDot, score, config);
		}
		else if (gameState == GAME_OVER) {
			SDL_FillRect(screen, NULL, colors.black);
			drawGameOver(screen,charset, score,&gameState, config);
			renderScren(scrtex, screen,renderer);
			handleGameOverInput(event, quit, gameState, snake, worldTime, blueDotPos, redDot, score, config);
		}
		
        frames++;
    }

    SDL_FreeSurface(charset);
    SDL_FreeSurface(screen);
    SDL_DestroyTexture(scrtex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
    return 0;
};