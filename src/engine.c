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
#include "engine.h"


point camera_pos = {0.0, 0.0, 0.0};

SDL_Window* screen = NULL;
SDL_Renderer* renderer;
SDL_Event evt;
SDL_Rect source, destination, dst;



double camera_dist = 554.0;

point camera_dir = {0.0, 0.0, 1.0};

struct timespec t0, t1;

double elapsed_time;

double resScale = 1;

double camera_angle_y = 0.0;
double camera_angle_x = 0.0;


triangle camera_basis = { //Currently not used
  {1.0, 0.0, 0.0},
  {0.0, 1.0, 0.0},
  {0.0, 0.0, 1.0},
  0
};


object translateObject(object obj, double x, double y, double z){
  for(int i = 0; i < obj.nFaces; i++){
    obj.tris[i] = translateTriangle(obj.tris[i], x, y, z);
  }
  return obj;
}

object rotateObjectX(object obj, double angle, double x, double y, double z){
  for(int i = 0; i < obj.nFaces; i++)
    obj.tris[i] = rotateTriX(obj.tris[i], angle, x, y, z);
  return obj;
}

object rotateObjectY(object obj, double angle, double x, double y, double z){
  for(int i = 0; i < obj.nFaces; i++)
    obj.tris[i] = rotateTriY(obj.tris[i], angle, x, y, z);
  return obj;
}

object rotateObjectZ(object obj, double angle, double x, double y, double z){
  for(int i = 0; i < obj.nFaces; i++)
    obj.tris[i] = rotateTriZ(obj.tris[i], angle, x, y, z);
  return obj;
}

node translateNode(node node, double x, double y, double z){
  *node.obj = translateObject(*node.obj, x, y, z);
  node.pos = translatePoint(node.pos, x, y, z);
  for(int i = 0; i < node.nChildren; i++)
    node.children[i] = translateNode(node.children[i], x, y, z);
  return node;
}

node rotateNodeX(node node, double angle, double x, double y, double z){
  *node.obj = rotateObjectX(*node.obj, angle, x, y, z);
  node.pos = rotatePointX(node.pos, angle, x, y, z);
  for(int i = 0; i < node.nChildren; i++)
    node.children[i] = rotateNodeX(node.children[i], angle, x, y, z);
  return node;
}

node rotateNodeY(node node, double angle, double x, double y, double z){
  *node.obj = rotateObjectY(*node.obj, angle, x, y, z);
  node.pos = rotatePointY(node.pos, angle, x, y, z);
  for(int i = 0; i < node.nChildren; i++)
    node.children[i] = rotateNodeY(node.children[i], angle, x, y, z);
  return node;
}

node rotateNodeZ(node node, double angle, double x, double y, double z){
  *node.obj = rotateObjectZ(*node.obj, angle, x, y, z);
  node.pos = rotatePointZ(node.pos, angle, x, y, z);
  for(int i = 0; i < node.nChildren; i++)
    node.children[i] = rotateNodeZ(node.children[i], angle, x, y, z);
  return node;
}

int cmpfunc (const void * a, const void * b) {
   //triangle* pa = (triangle*)a;
   //triangle* pb = (triangle*)b;
   tri_map* tma = (tri_map*)a;
   tri_map* tmb = (tri_map*)b;
   triangle* pa = tma->tri;
   triangle* pb = tmb->tri;
   //return calcCenter(*pb).z - calcCenter(*pa).z;
   return (vectorLength(subtractPoints(calcCenter(*pb), camera_pos)) - vectorLength(subtractPoints(calcCenter(*pa), camera_pos)));
}

double radToDeg(double rad){
  double deg = (rad * 360.0) / (2*M_PI);
  return deg;
}

double calcFOV(){
  double fov = 2 * atan((WIDTH*resScale/2)/camera_dist);
  return radToDeg(fov);
}

unsigned int colorLightness(double value, unsigned int color){
  unsigned int r, g, b, rValue, gValue, bValue;
  r = 0x00FF0000 & color;
  g = 0x0000FF00 & color;
  b = 0x000000FF & color;

  r = (int)(r * value) > 0x00FF0000 ? 0x00FF0000 : (int)(r * value);
  g = (int)(g * value) > 0x0000FF00 ? 0x0000FF00 : (int)(g * value);
  b = (int)(b * value) > 0x000000FF ? 0x000000FF : (int)(b * value);

  r &= 0x00FF0000;
  g &= 0x0000FF00;
  b &= 0x000000FF;

  unsigned int newColor = 0x00FFFFFF & (r | g | b);
  return newColor;
}

void drawTriangle(SDL_Renderer* renderer, triangle tri){
  point center = calcCenter(tri);
  if(center.z > 0){
    SDL_RenderDrawLine(renderer, tri.a.x, tri.a.y, tri.b.x, tri.b.y);
    SDL_RenderDrawLine(renderer, tri.b.x, tri.b.y, tri.c.x, tri.c.y);
    SDL_RenderDrawLine(renderer, tri.c.x, tri.c.y, tri.a.x, tri.a.y);
  }
}

void initialize_engine(bool fullscreen){
    if(SDL_Init(SDL_INIT_EVERYTHING) < 0)
            printf("SDL could not be initialized! %s\n", SDL_GetError());
        else
            printf("SDL video system is initialized and ready to go!\n");
        
        SDL_CreateWindowAndRenderer(WIDTH*resScale, HEIGHT*resScale, SDL_WINDOW_SHOWN, &screen, &renderer);
        if(!screen)
            printf("InitSetup failed to create window\n");

        SDL_SetWindowTitle(screen, "GGE");
        if(fullscreen)
          SDL_SetWindowFullscreen(screen, SDL_WINDOW_FULLSCREEN);

        if(TTF_Init() == -1){
          printf("TTF could not be initialized!\n");
        }
}

void terminate_engine(){
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(screen);
    SDL_Quit();
    exit(0);
}

//Aux function used by rasterizeTriangle
void sortPoints(point points[], int a, int b){
  point temp;
  if(points[a].y != points[b].y){
    if(points[a].y > points[b].y){
      temp = points[a];
      points[a] = points[b];
      points[b] = temp;
    }
  } else {
    if(points[a].x > points[b].x){
      temp = points[a];
      points[a] = points[b];
      points[b] = temp;
    }
  }
}

void rasterizeTriangle(SDL_Renderer* renderer, triangle tri){
  //sort points by height
  int i ;
  point p[3];
  p[0] = tri.a;
  p[1] = tri.b;
  p[2] = tri.c;
  sortPoints(p, 0, 1);
  sortPoints(p, 1, 2);
  sortPoints(p, 0, 1);

  if(p[0].y == p[2].y)
    return;
  
  int dy_long = round(p[2].y - p[0].y);
  double denominator = 1.0 / dy_long;
  double slope_long[dy_long];
  for(i = 0; i < dy_long; i++){
    slope_long[i] = p[0].x + (p[2].x-p[0].x)*(i)*denominator;
  }
  int dy_short = round(p[1].y - p[0].y);
  denominator = 1.0 / dy_short;
  double slope_short[dy_short];
  if(dy_short != 0){
    for(i = 0; i < dy_short; i++){
      slope_short[i] = p[0].x + (p[1].x-p[0].x)*i*denominator;
    }
  }
  
  int dy_last = dy_long - dy_short;
  denominator = 1.0 / dy_last;
  double slope_last[dy_last];
  if(dy_last != 0){
    for(i = 0; i < dy_last; i++){
      slope_last[i] = p[1].x + (p[2].x-p[1].x)*i*denominator;
    }
  }
  //scanline
  i = 0;
  if(dy_short != 0){
    for(i; i < dy_short; i++)
      for(int j = 0; j < resScale; j++){
        SDL_RenderDrawLine(renderer, slope_long[i]*resScale, (i+p[0].y)*resScale+j, (int)slope_short[i]*resScale, (i+p[0].y)*resScale+j);
      }
  }
  if(dy_last != 0){
    int origin = i;
    for(i; i < dy_long; i++)
      for(int j = 0; j < resScale; j++){
        SDL_RenderDrawLine(renderer, slope_long[i]*resScale, (i+p[0].y)*resScale+j, (int)slope_last[i - origin]*resScale, (i+p[0].y)*resScale+j);
      }
  }
}

object loadOBJ(const char* filePath, unsigned int color, double x, double y, double z, double scale){
  unsigned long int nFaces = 0;
  unsigned long int nVertices = 0;
  FILE* fp;
  fp = fopen(filePath, "r");
  
  size_t buf_size = 50;
  char* buf = malloc(buf_size*sizeof(char));
  char* endptr;

  do{
    getline(&buf, &buf_size, fp);
    nVertices++;
  } while(buf[0] == 'v');

  while(getline(&buf, &buf_size, fp)>0)
    nFaces++;
  nVertices--;

  printf("nvertices %lu nface %lu\n", nVertices, nFaces);
  rewind(fp);

  point* vertices = malloc(nVertices * sizeof(point));
  for(int i = 0; i < nVertices; i++){
    getline(&buf, &buf_size, fp);
    endptr = buf;
    double values[3];
    for(int i = 0; i < 3; i++)
      values[i] = scale*strtod(endptr+1, &endptr);
    vertices[i] = (point){-values[0] + x, -values[1] + y, -values[2] + z};
  }
  
  getline(&buf, &buf_size, fp);

  triangle* tris = malloc(nFaces * sizeof(triangle));
  for(int i = 0; i < nFaces; i++){
    getline(&buf, &buf_size, fp);
    endptr = buf;
    long int values[3];
    for(int i = 0; i < 3; i++)
      values[i] = strtol(endptr+1, &endptr, 10) - 1;
    tris[i] = (triangle){
      vertices[values[0]],
      vertices[values[1]],
      vertices[values[2]],
      color
      };
  }
  free(buf);
  object obj = {tris, nFaces, (point){x,y,z}};
  return obj;
}

void drawText(SDL_Renderer* renderer, const char* message, int x, int y, int width, int height, unsigned int color, int pt){
  TTF_Font* font = TTF_OpenFont("fonts/TerminusTTF-4.49.2.ttf", pt*resScale);
  if(font == NULL){
    printf("FONT COULD NOT BE LOADED!\n");
    exit(1);
  }
  SDL_Color text_color = {(0xFF0000&color)>>16, (0x00FF00&color)>>8, 0x0000FF&color, 0xFF};
  SDL_Surface* surfaceMessage = TTF_RenderText_Solid(font, message, text_color);
  SDL_Texture* Message = SDL_CreateTextureFromSurface(renderer, surfaceMessage); 

  SDL_Rect Message_rect;
  Message_rect.x = x*resScale;
  Message_rect.y = y*resScale;
  Message_rect.w = width*resScale; 
  Message_rect.h = height*resScale;

  SDL_RenderCopy(renderer, Message, NULL, &Message_rect);

  SDL_FreeSurface(surfaceMessage);
  SDL_DestroyTexture(Message);
  TTF_CloseFont(font);
}

point calcIntersect(point p0, point p1, char axis, unsigned int value){
  point intersect;
  if(axis == 'x'){
    intersect.x = value;
    intersect.y = round(((p0.x - value)/(p0.x - p1.x))*(p1.y - p0.y) + p0.y);
    //intersect.y = intersect.y < 0.0 ? 0.0 : intersect.y;
    //intersect.y = intersect.y > HEIGHT ? HEIGHT : intersect.y;
    
    intersect.z = 1.0;
  } else if(axis == 'y'){
    intersect.x = round(((p0.y - value)/(p0.y - p1.y))*(p1.x - p0.x) + p0.x);
    //intersect.x = intersect.x < 0.0 ? 0.0 : intersect.x;
    //intersect.x = intersect.x > WIDTH ? WIDTH : intersect.x;
    intersect.y = value;
    intersect.z = 1.0;
  } else if(axis == 'z'){
    intersect.x = round(((p0.z - value)/(p0.z - p1.z))*(p1.x - p0.x) + p0.x);
    intersect.y = round(((p0.z - value)/(p0.z - p1.z))*(p1.y - p0.y) + p0.y);
    intersect.z = value;
  }
  return intersect;
}

void clipEdge(point p1, point p2, triangle** clipped_tris, unsigned int* nTris, int index, char axis){
  triangle tri = (*clipped_tris)[index];
  point points[3] = {tri.a, tri.b, tri.c};
  unsigned int value;

  if(axis == 'x'){
    value = p1.x;
  } else if(axis == 'y'){
    value = p1.y;
  } else if(axis == 'z'){
    value = p1.z;
  }

  int nOutside = 0;
  int inside[3] = {1,1,1};
  if(axis == 'z'){
    if((p2.z - p1.z)*(tri.a.y - p1.y)-(p2.y - p1.y)*(tri.a.z - p1.z) > 0){
      //printf("OUTSIDE!\n");
      nOutside++;
      inside[0] = 0;
    }
    if((p2.z - p1.z)*(tri.b.y - p1.y)-(p2.y - p1.y)*(tri.b.z - p1.z) > 0){
      //printf("OUTSIDE!\n");
      nOutside++;
      inside[1] = 0;
    }
    if((p2.z - p1.z)*(tri.c.y - p1.y)-(p2.y - p1.y)*(tri.c.z - p1.z) > 0){
      //printf("OUTSIDE!\n");
      nOutside++;
      inside[2] = 0;
    }
  } else{
    if((p2.x - p1.x)*(tri.a.y - p1.y)-(p2.y - p1.y)*(tri.a.x - p1.x) > 0){
      nOutside++;
      inside[0] = 0;
    }
    if((p2.x - p1.x)*(tri.b.y - p1.y)-(p2.y - p1.y)*(tri.b.x - p1.x) > 0){
      nOutside++;
      inside[1] = 0;
    }
    if((p2.x - p1.x)*(tri.c.y - p1.y)-(p2.y - p1.y)*(tri.c.x - p1.x) > 0){
      nOutside++;
      inside[2] = 0;
    }
  }

  if(nOutside == 0){
    //do nothing, leave triangle in
  } else if(nOutside == 1){
    //create two new triangles
    int firstOut = 0;
    for(int i = 0; i < 3; i++){
      if(inside[i] == 0){
        firstOut = i;
        break;
      }
    }
    point intersect1 = calcIntersect(points[firstOut], points[(firstOut+1)%3], axis, value);
    point intersect2 = calcIntersect(points[firstOut], points[(firstOut+2)%3], axis, value);
    (*clipped_tris)[index] = (triangle){points[(firstOut+1)%3], points[(firstOut+2)%3], intersect1, tri.color};
    (*clipped_tris)[*nTris] = (triangle){points[(firstOut+2)%3], intersect2, intersect1, tri.color};
    (*nTris)++;
  } else if(nOutside == 2){
    //create one new triangle
    int firstIn = 0;
    for(int i = 0; i < 3; i++){
      if(inside[i] == 1){
        firstIn = i;
        break;
      }
    }
    point intersect1 = calcIntersect(points[firstIn], points[(firstIn+1)%3], axis, value);
    point intersect2 = calcIntersect(points[firstIn], points[(firstIn+2)%3], axis, value);
    (*clipped_tris)[index] = (triangle){points[firstIn], intersect1, intersect2, tri.color};
  } else if(nOutside == 3){
    //Don't render this triangle at all
    for(int i = index; i < *nTris-1; i++)
      (*clipped_tris)[i] = (*clipped_tris)[i+1];
    (*nTris)--;
  }
}

void clipTriangle(triangle** clipped_tris, unsigned int* nTris){
  //left
  clipEdge((point){0,0,0}, (point){0,HEIGHT,0}, clipped_tris, nTris, 0, 'x');
  //top
  for(int i = 0; i < *nTris; i++){
    clipEdge((point){0,0,0}, (point){-WIDTH,0,0}, clipped_tris, nTris, i, 'y');
  }
  //right
  for(int i = 0; i < *nTris; i++){
    clipEdge((point){WIDTH,0,0}, (point){WIDTH,-HEIGHT,0}, clipped_tris, nTris, i, 'x');
  }
  //bottom
  for(int i = 0; i < *nTris; i++){
    clipEdge((point){0,HEIGHT,0}, (point){WIDTH,HEIGHT,0}, clipped_tris, nTris, i, 'y');
  }
}

