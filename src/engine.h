#ifndef ENGINE_H
#define ENGINE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
// #include <unistd.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>

#include "lin_alg.h"
#include "camera.h"
#include "global.h"
#include "light.h"
#include "media.h"





typedef struct object{
  triangle* tris;
  unsigned int nFaces;
  point pos;
  unsigned char* texture;
  int texture_width;
  int texture_height; 
  int nrChannels;
} object;

typedef struct node{
  object* obj;
  point pos;
  struct node* children;
  unsigned int nChildren;
} node;

extern SDL_Window* screen;
extern SDL_Renderer* renderer;
extern SDL_Event evt;

extern double resScale;

void printTriangle(triangle tri);

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

void initialize_engine(bool fullscreen);

void terminate_engine();

void sortPoints(point points[], int a, int b);

void rasterizeTriangle(SDL_Renderer* renderer, triangle tri, SDL_Surface* surf, object obj);

object loadOBJ(const char* filePath, unsigned int color, double x, double y, double z, double scale, const char* texture_path);

void drawText(SDL_Renderer* renderer, const char* message, int x, int y, int width, int height, unsigned int color, int pt);

point calcIntersect(point p0, point p1, char axis, double value);

void clipEdge(point p1, point p2, triangle* clipped_tris, unsigned int* nTris, int* index, char axis);

void clipTriangle(triangle* clipped_tris, unsigned int* nTris);

point calcInterpolatedTexCoords(point in, point out, double ratiox, double ratioy, double in_z, double out_z, double intersect_z);

#endif