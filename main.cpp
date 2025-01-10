#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#define SCREEN_WIDTH	640
#define SCREEN_HEIGHT	480

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

void DrawRectangle(SDL_Surface *screen, int x, int y, int l, int k,
                   Uint32 outlineColor, Uint32 fillColor) {
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for(i = y + 1; i < y + k - 1; i++)
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
	};

void ResetGame(double &posX, double &posY, double &worldTime, double &distance, double &etiSpeed, int &direction) {
    posX = SCREEN_WIDTH / 2;
    posY = SCREEN_HEIGHT / 2;
    worldTime = 0;
    distance = 0;
    etiSpeed = 1;
    direction = 1;
}

bool isValidMove(double nextX, double nextY, int BOARD_MARGIN, int BOARD_WIDTH, int BOARD_HEIGHT, int squareSize) {
    double halfSquare = squareSize / 2;
    
    return (nextX - halfSquare >= BOARD_MARGIN &&
            nextX + halfSquare <= BOARD_MARGIN + BOARD_WIDTH &&
            nextY - halfSquare >= BOARD_MARGIN &&
            nextY + halfSquare <= BOARD_MARGIN + BOARD_HEIGHT);
}

int autoTurn(int currentDirection, double posX, double posY, int BOARD_MARGIN, int BOARD_WIDTH, int BOARD_HEIGHT, int squareSize) {
    int newDirection;
    double testX, testY;
    double step = squareSize;

    int rightTurn = (currentDirection + 1) % 4;
    int leftTurn = (currentDirection + 3) % 4;

    testX = posX + (rightTurn == 1 ? step : (rightTurn == 3 ? -step : 0));
    testY = posY + (rightTurn == 0 ? -step : (rightTurn == 2 ? step : 0));
    
    if (isValidMove(testX, testY, BOARD_MARGIN, BOARD_WIDTH, BOARD_HEIGHT, squareSize)) {
        return rightTurn;
    }

    testX = posX + (leftTurn == 1 ? step : (leftTurn == 3 ? -step : 0));
    testY = posY + (leftTurn == 0 ? -step : (leftTurn == 2 ? step : 0));
    
    if (isValidMove(testX, testY, BOARD_MARGIN, BOARD_WIDTH, BOARD_HEIGHT, squareSize)) {
        return leftTurn;
    }
	return currentDirection;
}

#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char **argv) {
	int t1, t2, quit, frames, rc;
	double delta, worldTime, fpsTimer, fps, distance, etiSpeed, posX, posY, stepX, stepY, speed_fix;
	SDL_Event event;
	SDL_Surface *screen, *charset;
	SDL_Surface *eti;
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
		};
	
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
	if(charset == NULL) {
		printf("SDL_LoadBMP(cs8x8.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
		};
	SDL_SetColorKey(charset, true, 0x000000);

	int squareSize = 20;

	eti = SDL_CreateRGBSurface(0, squareSize, squareSize, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

	SDL_FillRect(eti, NULL, SDL_MapRGB(eti->format, 0, 255, 0)); 
	if(eti == NULL) {
		printf("SDL_LoadBMP(eti.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
		};

	char text[128];
	int czarny = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	int zielony = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
	int czerwony = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
	int niebieski = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);


	const int BOARD_MARGIN = 80;
    const int BOARD_WIDTH = SCREEN_WIDTH - (2 * BOARD_MARGIN);
    const int BOARD_HEIGHT = SCREEN_HEIGHT - (2 * BOARD_MARGIN);
    const int STATS_HEIGHT = 60;
    const int STATS_Y = SCREEN_HEIGHT - STATS_HEIGHT - 4;
    const int STATS_X = 4;
    const int STATS_WIDTH = SCREEN_WIDTH - 8;

	int direction = 1;

	t1 = SDL_GetTicks();

	frames = 0;
	fpsTimer = 0;
	fps = 0;
	quit = 0;
	speed_fix = 10;
	ResetGame(posX, posY, worldTime, distance, etiSpeed, direction);

	while(!quit) {
		t2 = SDL_GetTicks();
		delta = (t2 - t1) * 0.001;
		t1 = t2;

		worldTime += delta;

		stepX = etiSpeed * delta * SCREEN_WIDTH / speed_fix;
		stepY = etiSpeed * delta * SCREEN_HEIGHT / speed_fix;

		double nextX = posX;
        double nextY = posY;
        
        if(direction == 0) nextY -= stepY;
        else if(direction == 1) nextX += stepX;
        else if(direction == 2) nextY += stepY;
        else nextX -= stepX;

		if (!isValidMove(nextX, nextY, BOARD_MARGIN, BOARD_WIDTH, BOARD_HEIGHT, squareSize)) {
            direction = autoTurn(direction, posX, posY, BOARD_MARGIN, BOARD_WIDTH, BOARD_HEIGHT, squareSize);
            
            nextX = posX;
            nextY = posY;
            if(direction == 0) nextY -= stepY;
            else if(direction == 1) nextX += stepX;
            else if(direction == 2) nextY += stepY;
            else nextX -= stepX;
        }
		posX = nextX;
        posY = nextY;


		SDL_FillRect(screen, NULL, czarny);
		DrawRectangle(screen, BOARD_MARGIN, BOARD_MARGIN, BOARD_WIDTH, BOARD_HEIGHT, zielony, czarny); 

		DrawRectangle(screen, STATS_X, STATS_Y, STATS_WIDTH, STATS_HEIGHT, czerwony, niebieski);

		sprintf(text, "Score: %d", 0);
		DrawString(screen, STATS_X + (STATS_WIDTH/2) - BOARD_MARGIN, STATS_Y + BOARD_MARGIN/2, text, charset);

		sprintf(text, "Length: %d", 1);
		DrawString(screen, STATS_X + STATS_WIDTH - BOARD_MARGIN, STATS_Y + BOARD_MARGIN/2, text, charset);

		sprintf(text, "ESC - quit, N - new game");
		DrawString(screen, STATS_X, STATS_Y + BOARD_MARGIN/2, text, charset);
		DrawSurface(screen, eti, posX, posY);
		fpsTimer += delta;
		if(fpsTimer > 0.5) {
			fps = frames * 2;
			frames = 0;
			fpsTimer -= 0.5;
			};

		DrawRectangle(screen, 4, 4, SCREEN_WIDTH - 8, 36, czerwony, niebieski);
		sprintf(text, "czas trwania = %.1lf s  %.0lf klatek / s", worldTime, fps);
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 10, text, charset);

		SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
//		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, scrtex, NULL, NULL);
		SDL_RenderPresent(renderer);

		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_KEYDOWN:
					if(event.key.keysym.sym == SDLK_ESCAPE) quit = 1;
					else if(event.key.keysym.sym == SDLK_n) ResetGame(posX, posY, worldTime, distance, etiSpeed, direction);
					else if(event.key.keysym.sym == SDLK_UP) direction = 0;
					else if(event.key.keysym.sym == SDLK_RIGHT) direction = 1;
					else if(event.key.keysym.sym == SDLK_DOWN) direction = 2;
					else if(event.key.keysym.sym == SDLK_LEFT) direction = 3;
					break;
				case SDL_QUIT:
					quit = 1;
					break;
				};
			};
		frames++;
	};

	SDL_FreeSurface(charset);
	SDL_FreeSurface(screen);
	SDL_DestroyTexture(scrtex);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();
	return 0;
	};
