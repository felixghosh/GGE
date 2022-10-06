#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>

#include "lin_alg.h"
#include "camera.h"
#include "global.h"
#include "light.h"

#define MAXOBJ 100
#define MAXLIGHT 100


typedef struct object{
  triangle* tris;
  unsigned int nFaces;
} object;

typedef struct tri_map{
  triangle* tri;
  object* obj;
} tri_map;

extern SDL_Renderer* renderer;
extern SDL_Event evt;

extern double resScale;

int cmpfunc (const void * a, const void * b);

double radToDeg(double rad);

double calcFOV();

unsigned int colorLightness(double value, unsigned int color);

void drawTriangle(SDL_Renderer* renderer, triangle tri);

void initialize();

void terminate();

void sortPoints(point points[], int a, int b);

void rasterizeTriangle(SDL_Renderer* renderer, triangle tri);

object loadOBJ(const char* filePath, unsigned int color, double x, double y, double z, double scale);

void drawText(SDL_Renderer* renderer, const char* message, int x, int y, int width, int height, unsigned int color, int pt);

point calcIntersect(point p0, point p1, char axis, unsigned int value);

void clipEdge(point p1, point p2, triangle** clipped_tris, unsigned int* nTris, int index, char axis);

void clipTriangle(triangle** clipped_tris, unsigned int* nTris);

int run_game();

#endif