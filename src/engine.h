#ifndef ENGINE_H
#define ENGINE_H

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




typedef struct object{
  triangle* tris;
  unsigned int nFaces;
  point pos;
} object;

typedef struct node{
  object* obj;
  point pos;
  struct node* children;
  unsigned int nChildren;
} node;

typedef struct tri_map{
  triangle* tri;
  object* obj;
  bool* render;
} tri_map;

extern SDL_Window* screen;
extern SDL_Renderer* renderer;
extern SDL_Event evt;

extern double resScale;

object translateObject(object obj, double x, double y, double z);

object rotateObjectX(object obj, double angle, double x, double y, double z);

object rotateObjectY(object obj, double angle, double x, double y, double z);

object rotateObjectZ(object obj, double angle, double x, double y, double z);

node translateNode(node node, double x, double y, double z);

node rotateNodeX(node node, double angle, double x, double y, double z);

node rotateNodeY(node node, double angle, double x, double y, double z);

node rotateNodeZ(node node, double angle, double x, double y, double z);

int cmpfunc (const void * a, const void * b);

double radToDeg(double rad);

double calcFOV();

unsigned int colorLightness(double value, unsigned int color);

void drawTriangle(SDL_Renderer* renderer, triangle tri);

void initialize_engine();

void terminate_engine();

void sortPoints(point points[], int a, int b);

void rasterizeTriangle(SDL_Renderer* renderer, triangle tri);

object loadOBJ(const char* filePath, unsigned int color, double x, double y, double z, double scale);

void drawText(SDL_Renderer* renderer, const char* message, int x, int y, int width, int height, unsigned int color, int pt);

point calcIntersect(point p0, point p1, char axis, unsigned int value);

void clipEdge(point p1, point p2, triangle** clipped_tris, unsigned int* nTris, int index, char axis);

void clipTriangle(triangle** clipped_tris, unsigned int* nTris);

#endif