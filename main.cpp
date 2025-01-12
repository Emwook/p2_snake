#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>
#include <time.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#define SQUARE_SIZE 20
#define SCREEN_WIDTH 31*SQUARE_SIZE
#define SCREEN_HEIGHT 31*SQUARE_SIZE

#define BOARD_MARGIN 4*SQUARE_SIZE
#define BOARD_WIDTH (SCREEN_WIDTH - (2 * BOARD_MARGIN))
#define BOARD_HEIGHT (SCREEN_HEIGHT - (2 * BOARD_MARGIN))

#define STATS_HEIGHT 3*SQUARE_SIZE
#define STATS_Y (SCREEN_HEIGHT - STATS_HEIGHT - 4)
#define STATS_X 4
#define STATS_WIDTH (SCREEN_WIDTH - 8)
#define SPEED_FIX 100
#define MAX_SNAKE_LENGTH 20
#define MIN_SNAKE_LENGTH 3

#define DEFAULT_SNAKE_SPEED 1
#define DEFAULT_SNAKE_LENGTH 3
#define MIN_SPAWN_INTERVAL 5.0f
#define MAX_SPAWN_INTERVAL 10.0f
#define DISPLAY_DURATION   10.0f
#define SHORTEN_LENGTH     1
#define SLOWDOWN_FACTOR    0.3
#define PROGRESS_BAR_WIDTH 20
#define PROGRESS_BAR_HEIGHT 100

typedef enum {
    GAME_RUNNING,
    GAME_OVER
} GameState;

typedef struct {
    int x;
    int y;
} position_t;

typedef struct {
    position_t body[MAX_SNAKE_LENGTH];
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

void ResetGame(double &worldTime, snake_t &snake) { // add red and blue dot reset
	snake.length = MIN_SNAKE_LENGTH;
	for(int i = 0; i<snake.length; i++){
		snake.body[i].x = SCREEN_WIDTH / 2 - (i *SQUARE_SIZE);
    	snake.body[i].y  = SCREEN_HEIGHT / 2;
	}
	snake.speed = 1;
	snake.direction = 1;
    worldTime = 0;
}

int isValidMove(position_t nextPos) {
    int halfSquare = SQUARE_SIZE / 2;
    int nextX = nextPos.x;
    int nextY = nextPos.y;
    return (nextX - halfSquare >= BOARD_MARGIN &&
            nextX + halfSquare <= BOARD_MARGIN + BOARD_WIDTH &&
            nextY - halfSquare >= BOARD_MARGIN &&
            nextY + halfSquare <= BOARD_MARGIN + BOARD_HEIGHT);
}

void autoTurn(snake_t &snake) {
    int step = SQUARE_SIZE;
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
    
    if (isValidMove(rightPos)) {
        snake.direction = rightTurn;
        return;
    }
    
    if (isValidMove(leftPos)) {
        snake.direction = leftTurn;
        return;
    }
}

void drawGrid(SDL_Surface *screen, Uint32 color) {
    for (int x = BOARD_MARGIN; x < BOARD_MARGIN + BOARD_WIDTH; x += SQUARE_SIZE) {
        DrawLine(screen, x, BOARD_MARGIN, BOARD_HEIGHT, 0, 1, color);
    }
    
    for (int y = BOARD_MARGIN; y < BOARD_MARGIN + BOARD_HEIGHT; y += SQUARE_SIZE) {
        DrawLine(screen, BOARD_MARGIN, y, BOARD_WIDTH, 1, 0, color);
    }
}

void UpdateSnake(snake_t &snake) {
    for (int i = snake.length - 1; i > 0; --i) {
        snake.body[i] = snake.body[i - 1];
    }

    if (snake.direction == 0) {
        snake.body[0].y -= SQUARE_SIZE;
    } 
    else if (snake.direction == 1) {
        snake.body[0].x += SQUARE_SIZE;
    } 
    else if (snake.direction == 2) {
        snake.body[0].y += SQUARE_SIZE;
    } 
    else if (snake.direction == 3) {
        snake.body[0].x -= SQUARE_SIZE;
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

void randomizePosition(position_t &pos, snake_t *snake) {
    int validPosition;
    int i;

    do {
        pos.x = BOARD_MARGIN + (rand() % (BOARD_WIDTH / SQUARE_SIZE)) * SQUARE_SIZE + SQUARE_SIZE / 2;
        pos.y = BOARD_MARGIN + (rand() % (BOARD_HEIGHT / SQUARE_SIZE)) * SQUARE_SIZE + SQUARE_SIZE / 2;

        validPosition = 1;
        for (i = 0; i < snake->length; i++) {
            if (pos.x == snake->body[i].x && pos.y == snake->body[i].y) {
                validPosition = 0;
                break;
            }
        }
    } while (!validPosition);
}


void checkBlueDot(snake_t* snake, int nextX, int nextY, position_t &blue_dot_pos) {
	if (nextX == blue_dot_pos.x && nextY == blue_dot_pos.y &&snake->length <=MAX_SNAKE_LENGTH) {
		snake->length++;
		randomizePosition(blue_dot_pos, snake);
	}
}

void drawGameOver(SDL_Surface* screen, SDL_Surface* charset, int score, GameState* gameState) {
    int windowWidth = SCREEN_WIDTH / 2;
    int windowHeight = SCREEN_HEIGHT / 3;
    int windowX = (SCREEN_WIDTH - windowWidth) / 2;
    int windowY = (SCREEN_HEIGHT - windowHeight) / 2;
    
    Uint32 colorBlack = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
    Uint32 colorRed = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
    Uint32 colorWhite = SDL_MapRGB(screen->format, 0xFF, 0xFF, 0xFF);
    
    DrawRectangle(screen, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 
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

void handleGameOverInput(SDL_Event* event, int* quit, GameState* gameState, snake_t* snake,  double* worldTime) {
    while(SDL_PollEvent(event)) {
        switch(event->type) {
            case SDL_KEYDOWN:
                if(event->key.keysym.sym == SDLK_ESCAPE) {
                    *quit = 1;
                }
                else if(event->key.keysym.sym == SDLK_n) {
                    ResetGame(*worldTime, *snake);
                    *gameState = GAME_RUNNING;
                }
                break;
            case SDL_QUIT:
                *quit = 1;
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

void spawnRedDot(red_dot_t* redDot, snake_t* snake, float worldTime) { // add a check for no blue and red dot collision!
    if (redDot->isActive) return;

    SDL_Point newPos;
    int validPosition = 0;
    while (!validPosition) {
        newPos.x = BOARD_MARGIN + (rand() % (BOARD_WIDTH / SQUARE_SIZE)) * SQUARE_SIZE + SQUARE_SIZE / 2;
        newPos.y = BOARD_MARGIN + (rand() % (BOARD_WIDTH / SQUARE_SIZE)) * SQUARE_SIZE + SQUARE_SIZE / 2;
        validPosition = 1;
        for (int i = 0; i < snake->length; i++) {
            if (newPos.x == snake->body[i].x && newPos.y == snake->body[i].y) {
                validPosition = 1;
                break;
            }
        }
    }

    redDot->position = newPos;
    redDot->spawnTime = worldTime;
    redDot->isActive = 1;
}

void checkRedDot(snake_t* snake, red_dot_t* redDot) {
    if (!redDot->isActive) return;

    if (redDot->position.x == snake->body[0].x && 
        snake->body[0].y == redDot->position.y) {
        if (rand() % 2 == 0) {
            int newLength = snake->length - SHORTEN_LENGTH;
            if (newLength < 1) newLength = 1;
            snake->length = newLength;
        } else {
            if(snake->speed - SLOWDOWN_FACTOR>1){
                snake->speed -= SLOWDOWN_FACTOR;
            }
            else {
                snake->speed = DEFAULT_SNAKE_SPEED;
            }
        }
        redDot->isActive = 0;
    }
}

void drawProgressBar(SDL_Surface* screen, red_dot_t* redDot, float worldTime, Uint32 color, Uint32 czarny, SDL_Surface* charset) { // bad progress bar height
    if (!redDot->isActive) return;

    float timeElapsed = worldTime - redDot->spawnTime;
    float remainingTime = redDot->displayDuration - timeElapsed;

    if (remainingTime <= 0) {
        redDot->isActive = 0;
        return;
    }

    SDL_Rect borderRect = {
        PROGRESS_BAR_WIDTH *2,
        PROGRESS_BAR_HEIGHT *2,
        PROGRESS_BAR_WIDTH,
        PROGRESS_BAR_HEIGHT
    };
    DrawRectangle(screen, borderRect.x, borderRect.y, 
                  borderRect.w, borderRect.h, color, czarny);

    float fillHeight = (remainingTime / redDot->displayDuration) * (PROGRESS_BAR_HEIGHT/3);
    SDL_Rect fillRect = {
        borderRect.x + 2,
        borderRect.y + 2 + (int)((PROGRESS_BAR_HEIGHT - 4) - fillHeight),
        borderRect.w - 4,
        (int)fillHeight
    };
    SDL_FillRect(screen, &fillRect, color);

    char timeText[32];
    snprintf(timeText, sizeof(timeText), "%.1fs", remainingTime);

    DrawString(screen, borderRect.x, borderRect.y + PROGRESS_BAR_HEIGHT + 10, timeText, charset);
}


void updateBonusSystem(snake_t* snake, red_dot_t* redDot, float worldTime) {
    if (!redDot->isActive && worldTime >= redDot->spawnTime) {
        spawnRedDot(redDot, snake, worldTime);
        float spawnInterval = MIN_SPAWN_INTERVAL + 
            (rand()%2) * 
            (MAX_SPAWN_INTERVAL- MIN_SPAWN_INTERVAL);
        redDot->spawnTime = worldTime + spawnInterval;
    }

    checkRedDot(snake, redDot);

    if (redDot->isActive && 
        (worldTime - redDot->spawnTime) >= redDot->displayDuration) {
        redDot->isActive = 0;
    }
}





#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char **argv) {
	srand(time(NULL));
	GameState gameState = GAME_RUNNING;
    int t1, t2, quit, frames, rc;
    double delta, worldTime, fpsTimer, fps;
    int stepX, stepY;
	double speedUpTime = 3;
	double speedUpAmount= 0.1;
	double nextIncrease = speedUpAmount;

    snake_t snake;
    snake.direction = 1;
    snake.length = DEFAULT_SNAKE_LENGTH;
    snake.speed = DEFAULT_SNAKE_SPEED;
    ResetGame(worldTime, snake);

	position_t blue_dot_pos;
	randomizePosition(blue_dot_pos, &snake);

    red_dot_t redDot = {
        .isActive = 0,
        .displayDuration = 5,
    };

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

    rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0,
                                     &window, &renderer);
    if(rc != 0) {
        SDL_Quit();
        printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    SDL_SetWindowTitle(window, "snek");

    screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
                                  0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING,
                               SCREEN_WIDTH, SCREEN_HEIGHT);

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

    eti = SDL_CreateRGBSurface(0, SQUARE_SIZE, SQUARE_SIZE, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	ite = SDL_CreateRGBSurface(0, SQUARE_SIZE, SQUARE_SIZE, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	blue_dot = SDL_CreateRGBSurface(0, SQUARE_SIZE, SQUARE_SIZE, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    red_dot = SDL_CreateRGBSurface(0, SQUARE_SIZE, SQUARE_SIZE, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    SDL_FillRect(eti, NULL, SDL_MapRGB(eti->format, 255, 255, 0)); 
	SDL_FillRect(ite, NULL, SDL_MapRGB(ite->format, 0, 255, 0)); 
	drawCircle(blue_dot, SQUARE_SIZE / 2, SQUARE_SIZE / 2, SQUARE_SIZE / 3, SDL_MapRGB(blue_dot->format, 0, 0, 255));
    drawCircle(red_dot, SQUARE_SIZE / 2, SQUARE_SIZE / 2, SQUARE_SIZE / 3, SDL_MapRGB(red_dot->format, 255, 0, 0));
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
    int czarny = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
    int zielony = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
    int czerwony = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
    int niebieski = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);

    t1 = SDL_GetTicks();

    frames = 0;
    fpsTimer = 0;
    fps = 0;
    quit = 0;
    ResetGame(worldTime, snake);

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

        while (timeAccumulator >= (SQUARE_SIZE / (snake.speed * SPEED_FIX))) {
            int stepX = 0;
            int stepY = 0;

            if (snake.direction == 0) {
                stepY -= SQUARE_SIZE;
            }
            else if (snake.direction == 1) {
                stepX += SQUARE_SIZE;
            }
            else if (snake.direction == 2) {
                stepY += SQUARE_SIZE;
            }
            else if (snake.direction == 3) {
                stepX -= SQUARE_SIZE;
            }
			int nextX = snake.body[0].x + stepX;
			int nextY =  snake.body[0].y + stepY;
            position_t nextPos = {
                .x = nextX,
                .y = nextY,
            };
			if (!isValidMove(nextPos)) {
                autoTurn(snake);
            }
			if(checkSnakeCollision(&snake, nextX, nextY)){
				gameState = GAME_OVER;
			}
			checkBlueDot(&snake, nextX, nextY, blue_dot_pos);
            updateBonusSystem(&snake,&redDot,worldTime);
			UpdateSnake(snake);
            timeAccumulator -= (SQUARE_SIZE / (snake.speed * SPEED_FIX));
        }

        SDL_FillRect(screen, NULL, czarny);
        DrawRectangle(screen, BOARD_MARGIN, BOARD_MARGIN, BOARD_WIDTH, BOARD_HEIGHT, zielony, czarny); 
        drawGrid(screen, SDL_MapRGB(screen->format, 0x33, 0x33, 0x33));
        DrawRectangle(screen, STATS_X, STATS_Y, STATS_WIDTH, STATS_HEIGHT, zielony, czarny);

        snprintf(text, sizeof(text), "Score: %d, speed: %f", 0, snake.speed);
        DrawString(screen, STATS_X + (STATS_WIDTH/2) - BOARD_MARGIN, STATS_Y + BOARD_MARGIN/2, text, charset);

        snprintf(text, sizeof(text), "Length: %d", snake.length);
        DrawString(screen, STATS_X + STATS_WIDTH - BOARD_MARGIN, STATS_Y + BOARD_MARGIN/2, text, charset);

        snprintf(text, sizeof(text), "ESC - quit, N - new game");
        DrawString(screen, STATS_X, STATS_Y + BOARD_MARGIN/2, text, charset);
		DrawSurface(screen, eti, snake.body[0].x, snake.body[0].y);
		for (int i = 1; i <snake.length; i++) {
        	DrawSurface(screen, ite, snake.body[i].x, snake.body[i].y);
		}
		DrawSurface(screen, blue_dot, blue_dot_pos.x, blue_dot_pos.y);\
        if(redDot.isActive == 1){
            DrawSurface(screen, red_dot, redDot.position.x, redDot.position.y);
            drawProgressBar(screen,&redDot,worldTime,zielony,czarny, charset);
        }
        fpsTimer += delta;

        if(fpsTimer > 0.5) {
            fps = frames * 2;
            frames = 0;
            fpsTimer -= 0.5;
        }

        DrawRectangle(screen, 4, 4, SCREEN_WIDTH - 8, 36, zielony, czarny);
        snprintf(text, sizeof(text), "czas trwania = %.1lf s  %.0lf klatek / s", worldTime, fps);
        DrawString(screen,10 , 20, text, charset);
		snprintf(text, sizeof(text), "wymagania: 1, 2, 3, 4, A, B");
        DrawString(screen,2*SCREEN_WIDTH/3 ,20, text, charset);
        SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
        SDL_RenderCopy(renderer, scrtex, NULL, NULL);
        SDL_RenderPresent(renderer);

        while(SDL_PollEvent(&event)) {
            switch(event.type) {
                case SDL_KEYDOWN:
                    if(event.key.keysym.sym == SDLK_ESCAPE) quit = 1;
                    else if(event.key.keysym.sym == SDLK_n) ResetGame(worldTime, snake);
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
		else if (gameState == GAME_OVER) {
			SDL_FillRect(screen, NULL, czarny);
			drawGameOver(screen,charset,1,&gameState);
			SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
			SDL_RenderCopy(renderer, scrtex, NULL, NULL);
			SDL_RenderPresent(renderer);
			handleGameOverInput(&event, &quit, &gameState, &snake, &worldTime);
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
