#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#define SQUARE_SIZE 20
#define SCREEN_WIDTH 30*SQUARE_SIZE
#define SCREEN_HEIGHT 30*SQUARE_SIZE
#define MAX_SNAKE_LENGTH 16*SQUARE_SIZE
#define BOARD_MARGIN 4*SQUARE_SIZE
#define BOARD_WIDTH (SCREEN_WIDTH - (2 * BOARD_MARGIN))
#define BOARD_HEIGHT (SCREEN_HEIGHT - (2 * BOARD_MARGIN))
#define STATS_HEIGHT 3*SQUARE_SIZE
#define STATS_Y (SCREEN_HEIGHT - STATS_HEIGHT - 4)
#define STATS_X 4
#define STATS_WIDTH (SCREEN_WIDTH - 8)
#define SPEED_FIX 10000

typedef struct {
    int x;
    int y;
} Position;

typedef struct {
    Position body[MAX_SNAKE_LENGTH];
    int length;
    int direction;
    double speed;
} Snake;

//template functions
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

//gameplay functions
void ResetGame(double &worldTime, double &distance, Snake *snake, const int initialLength) {
    worldTime = 0;
    distance = 0;
    snake->speed = 1;
    snake->direction = 1;
    snake->length = initialLength;

    snake->body[0].x = 16*SQUARE_SIZE;
    snake->body[0].y = 16*SQUARE_SIZE;
    for (int i = 1; i < initialLength; i++) {
        snake->body[i].x = snake->body[0].x - (i * SQUARE_SIZE);
        snake->body[i].y = snake->body[0].y;
    }
}

bool isValidMove(double nextX, double nextY) {
    double halfSquare = SQUARE_SIZE / 2;
    
    return (nextX - halfSquare >= BOARD_MARGIN &&
            nextX + halfSquare <= BOARD_MARGIN + BOARD_WIDTH &&
            nextY - halfSquare >= BOARD_MARGIN &&
            nextY + halfSquare <= BOARD_MARGIN + BOARD_HEIGHT);
}

void autoTurn(Snake *snake) {
    int newDirection;
    double testX, testY;

    int rightTurn = (snake->direction + 1) % 4;
    int leftTurn = (snake->direction + 3) % 4;

    testX = snake->body[0].x + (rightTurn == 1 ? SQUARE_SIZE : (rightTurn == 3 ? -SQUARE_SIZE : 0));
    testY = snake->body[0].y + (rightTurn == 0 ? -SQUARE_SIZE : (rightTurn == 2 ? SQUARE_SIZE : 0));
    
    if (isValidMove(testX, testY)) {
        snake->direction = rightTurn;
    }

    testX = snake->body[0].x + (leftTurn == 1 ? SQUARE_SIZE : (leftTurn == 3 ? -SQUARE_SIZE : 0));
    testY = snake->body[0].x + (leftTurn == 0 ? -SQUARE_SIZE : (leftTurn == 2 ? SQUARE_SIZE : 0));
    
    if (isValidMove(testX, testY)) {
        snake->direction = leftTurn;
    }
}

// main refactoring parts

void processInput(SDL_Event *event, int *quit, double *worldTime, double *distance, Snake *snake, int initialLength) {
    while(SDL_PollEvent(event)) {
        switch(event->type) {
            case SDL_KEYDOWN:
                if(event->key.keysym.sym == SDLK_ESCAPE) {
                    *quit = 1;
                } 
                else if(event->key.keysym.sym == SDLK_n) {
    				ResetGame(*worldTime, *distance, snake, initialLength);
                } 
                else if(event->key.keysym.sym == SDLK_UP) {
                    snake->direction = 0;
                } 
                else if(event->key.keysym.sym == SDLK_RIGHT) {
                    snake->direction = 1;
                } 
                else if(event->key.keysym.sym == SDLK_DOWN) {
                    snake->direction = 2;
                } 
                else if(event->key.keysym.sym == SDLK_LEFT) {
                    snake->direction = 3;
                }
                break;
            case SDL_QUIT:
                *quit = 1;
                break;
            default:
                break;
        }
    }
}

void drawStats(SDL_Surface *screen, int score, int length, SDL_Surface *charset, int colorRed, int colorBlue) {
    char text[100]; // Assuming the text will fit in 100 characters

    // Draw the stats rectangle
    DrawRectangle(screen, STATS_X, STATS_Y, STATS_WIDTH, STATS_HEIGHT, colorRed, colorBlue);

    // Draw the score
    sprintf(text, "Score: %d", score);
    DrawString(screen, STATS_X + (STATS_WIDTH / 2) - BOARD_MARGIN, STATS_Y + BOARD_MARGIN / 2, text, charset);

    // Draw the length
    sprintf(text, "Length: %d", length);
    DrawString(screen, STATS_X + STATS_WIDTH - BOARD_MARGIN, STATS_Y + BOARD_MARGIN / 2, text, charset);

    // Draw instructions
    sprintf(text, "ESC - quit, N - new game");
    DrawString(screen, STATS_X, STATS_Y + BOARD_MARGIN / 2, text, charset);
}

void UpdateSnakePosition(Snake *snake) {
    for (int i = snake->length - 1; i > 0; i--) {
        snake->body[i] = snake->body[i - 1];
    }

    switch (snake->direction) {
        case 0: snake->body[0].y -= SQUARE_SIZE; break; // Up
        case 1: snake->body[0].x += SQUARE_SIZE; break; // Right
        case 2: snake->body[0].y += SQUARE_SIZE; break; // Down
        case 3: snake->body[0].x -= SQUARE_SIZE; break; // Left
    }
}

// Function to calculate movement steps
void calculateSteps(double &stepX, double &stepY, double speed, double delta) {
    stepX = speed * delta * SCREEN_WIDTH / SPEED_FIX;
    stepY = speed * delta * SCREEN_HEIGHT / SPEED_FIX;
}

// Function to determine the next position based on direction
void determineNextPosition(double &nextX, double &nextY, double stepX, double stepY, int direction) {
    if (direction == 0) nextY -= stepY;
    else if (direction == 1) nextX += stepX;
    else if (direction == 2) nextY += stepY;
    else nextX -= stepX;
}

// Function to handle snake movement
void moveSnake(Snake &snake, double delta) {
	double stepX, stepY;
    calculateSteps(stepX, stepY, snake.speed, delta);

    double prevX = snake.body[0].x;
    double prevY = snake.body[0].y;

    // Calculate next position based on direction
    double nextX = snake.body[0].x;
    double nextY = snake.body[0].y;
    determineNextPosition(nextX, nextY, stepX, stepY, snake.direction);

    // Ensure next position is aligned to the grid
    nextX = (int)(nextX / SQUARE_SIZE) * SQUARE_SIZE;
    nextY = (int)(nextY / SQUARE_SIZE) * SQUARE_SIZE;

    // Update head position
    if (isValidMove(nextX, nextY)) {
        snake.body[0].x = nextX;
        snake.body[0].y = nextY;
    } else {
        autoTurn(&snake);
    }

    // Update the rest of the body
    for (int i = 1; i < snake.length; i++) {
        double tempX = snake.body[i].x;
        double tempY = snake.body[i].y;
        snake.body[i].x = prevX;
        snake.body[i].y = prevY;
        prevX = tempX;
        prevY = tempY;
    }
}

#ifdef __cplusplus
extern "C"
#endif

int main(int argc, char **argv) {
    int t1, t2, quit, frames, rc;
    double delta, worldTime, fpsTimer, fps, distance, etiSpeed, stepX, stepY;

    // Create the Snake struct
    Snake snake;
    const int initialLength = 4;
    snake.length = initialLength;
    int currentLength = initialLength;

    SDL_Event event;
    SDL_Surface *screen, *charset;
    SDL_Surface *eti;
    SDL_Surface *ite;
    SDL_Texture *scrtex;
    SDL_Window *window;
    SDL_Renderer *renderer;

    printf("wyjscie printfa trafia do tego okienka\n");
    printf("printf output goes here\n");

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }

    rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer);
    if (rc != 0) {
        SDL_Quit();
        printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    SDL_SetWindowTitle(window, "Szablon do zdania drugiego 2017");

    screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
                                  0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING,
                               SCREEN_WIDTH, SCREEN_HEIGHT);

    SDL_ShowCursor(SDL_DISABLE);

    charset = SDL_LoadBMP("./cs8x8.bmp");
    if (charset == NULL) {
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
    SDL_FillRect(eti, NULL, SDL_MapRGB(eti->format, 0, 255, 0));
    SDL_FillRect(eti, NULL, SDL_MapRGB(eti->format, 255, 255, 0));

    if (eti == NULL) {
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
    int colorBlack = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
    int colorGreen = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
    int colorRed = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
    int colorBlue = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);

    int direction = 1;

    t1 = SDL_GetTicks();

    frames = 0;
    fpsTimer = 0;
    fps = 0;
    quit = 0;
    ResetGame(worldTime, distance, &snake, initialLength);

    while (!quit) {
        t2 = SDL_GetTicks();
        delta = (t2 - t1) * 0.001;
        t1 = t2;

		// if (delta > 0.1) {
		// 	UpdateSnakePosition(&snake);
		// 	delta = 0;
		// }

        worldTime += delta;
		
		moveSnake(snake, delta);

        SDL_FillRect(screen, NULL, colorBlack);
        DrawRectangle(screen, BOARD_MARGIN, BOARD_MARGIN, BOARD_WIDTH, BOARD_HEIGHT, colorGreen, colorBlack);

        drawStats(screen, 0, snake.length, charset, colorRed, colorBlue);

        DrawSurface(screen, eti, snake.body->x, snake.body->y);
        for (int i = 0; i < snake.length; i++) {
            DrawSurface(screen, ite, snake.body[i].x, snake.body[i].y);
        }

        fpsTimer += delta;
        if (fpsTimer > 0.5) {
            fps = frames * 2;
            frames = 0;
            fpsTimer -= 0.5;
        }

        DrawRectangle(screen, 4, 4, SCREEN_WIDTH - 8, 36, colorRed, colorBlue);
        sprintf(text, "czas trwania = %.1lf s  %.0lf klatek / s", worldTime, fps);
		for (int i = 0; i < snake.length; i++) {
			printf("Segment %d: x = %d, y = %d\n", i, snake.body[i].x, snake.body[i].y);
		}
        DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 10, text, charset);

        SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
        SDL_RenderCopy(renderer, scrtex, NULL, NULL);
        SDL_RenderPresent(renderer);

        processInput(&event, &quit, &worldTime, &distance, &snake, initialLength);
        frames++;
    }

    SDL_FreeSurface(charset);
    SDL_FreeSurface(screen);
    SDL_DestroyTexture(scrtex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
    return 0;
}
