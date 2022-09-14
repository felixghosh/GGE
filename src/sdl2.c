#include <SDL2/SDL.h>
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

#define MAXOBJ 100

SDL_Window* screen = NULL;
SDL_Renderer* renderer;
SDL_Event evt;
SDL_Rect source, destination, dst;

double camera_dist = 554.0; //For FOV 60

point camera_dir = {0.0, 0.0, 1.0};

struct timespec t0, t1;
double elapsed_time;

double speed = 3.0;
double resScale = 1;

point camera_pos = {0.0, 0.0, 0.0};
double camera_angle_y = 0.0;
double camera_angle_x = 0.0;

point light_direction = {0.0, 0.0, 1.0};

bool wireframe = false;




object* objects;
unsigned int nObj = 0;

triangle camera_basis = { //Currently not used
  {1.0, 0.0, 0.0},
  {0.0, 1.0, 0.0},
  {0.0, 0.0, 1.0},
  0
};

int cmpfunc (const void * a, const void * b) {
   triangle* pa = (triangle*)a;
   triangle* pb = (triangle*)b;
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

void initialize(){
    if(SDL_Init(SDL_INIT_EVERYTHING) < 0)
            printf("SDL could not be initialized! %s\n", SDL_GetError());
        else
            printf("SDL video system is initialized and ready to go!\n");
        
        SDL_CreateWindowAndRenderer(WIDTH*resScale, HEIGHT*resScale, SDL_WINDOW_SHOWN, &screen, &renderer);
        if(!screen)
            printf("InitSetup failed to create window\n");

        SDL_SetWindowTitle(screen, "GGE");
        SDL_SetWindowMouseGrab(screen, SDL_TRUE);
        SDL_SetRelativeMouseMode(SDL_TRUE);
        SDL_ShowCursor(SDL_DISABLE);
}

void terminate(){
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

  bool shortSide = (p[1].y - p[0].y) * (p[2].x - p[0].x) < (p[1].x - p[0].x) * (p[2].y - p[0].y); //false = left, true = right
  
  int dy_long = (p[2].y - p[0].y);
  double denominator = 1.0 / dy_long;
  double slope_long[dy_long];
  for(i = 0; i < dy_long; i++){
    slope_long[i] = p[0].x + (p[2].x-p[0].x)*(i)*denominator;
  }
  int dy_short = (p[1].y - p[0].y);
  denominator = 1.0 / dy_short;
  double slope_short[dy_short];
  if(dy_short != 0){
    for(i = 0; i < dy_short; i++){
      slope_short[i] = p[0].x + (p[1].x-p[0].x)*i*denominator;
    }
  }
  int dy_last = (p[2].y - p[1].y);
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
      //SDL_RenderFillRect(renderer, &((SDL_Rect){slope_long[i]*resScale, (i+p[0].y)*resScale, (int)slope_short[i]*resScale - slope_long[i]*resScale, (i+p[0].y)*resScale+resScale-1 - (i+p[0].y)*resScale}));
      for(int j = 0; j < resScale; j++)
        SDL_RenderDrawLine(renderer, slope_long[i]*resScale, (i+p[0].y)*resScale+j, (int)slope_short[i]*resScale, (i+p[0].y)*resScale+j);
  }
  if(dy_last != 0){
    int origin = i;
    for(i; i < dy_long; i++)
      for(int j = 0; j < resScale; j++)
        SDL_RenderDrawLine(renderer, slope_long[i]*resScale, (i+p[0].y)*resScale+j, (int)slope_last[i - origin]*resScale, (i+p[0].y)*resScale+j);
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
  //free(buf);
  object obj = {tris, nFaces};
  printf("returning tris at %p\n", tris);
  return obj;
}

bool checkIfOutside(point p){
  return p.x < -50 || p.y < -50 || p.x > WIDTH*resScale + 50 || p.y > HEIGHT*resScale + 50;
}


int main(int argc, char* argv[]){
  initialize();
  int running = 1;
  int i = 0;  

  objects = malloc(MAXOBJ*sizeof(object));
  object teapot = loadOBJ("/home/felixghosh/prog/c/GGE/OBJ/teapot.obj", 0xDF2332, 300, 0, 400, 100);
  object cube = loadOBJ("/home/felixghosh/prog/c/GGE/OBJ/cube.obj", 0x23DF32, 0, 0, 400, 100);
  object monkey = loadOBJ("/home/felixghosh/prog/c/GGE/OBJ/monkey.obj", 0x2323DF, 0, -300, 400, 100);
  objects[nObj++] = teapot;
  objects[nObj++] = cube;
  objects[nObj++] = monkey;

  clock_gettime(CLOCK_REALTIME, &t0);

  //for(int i = 0; i < nFaces; i++)
  //  tris[i] = translateTriangle(tris[i], 0, 0, 200);

    while(running){
        clock_gettime(CLOCK_REALTIME, &t1);
        elapsed_time = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec)/1000000000.0;
        clock_gettime(CLOCK_REALTIME, &t0);
        //printf("fps: %5u\n", (int)(1/elapsed_time));
        //printf("fov: %5u\n", (int)calcFOV());

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

        for(int j = 0; j < nObj; j++){
          
          triangle* tris = objects[j].tris;
          unsigned int nFaces = objects[j].nFaces;

          //sort triangles for painters algorithm
          qsort(tris, nFaces, sizeof(triangle), cmpfunc);

          for(int i = 0; i < nFaces; i++){
              
              triangle projected_tri = projectTriangle(tris[i]);
              bool outside = checkIfOutside(projected_tri.a) || checkIfOutside(projected_tri.b) || checkIfOutside(projected_tri.c);
              if(outside)
                continue;
              point projected_normal = calcNormal(projected_tri);
              projected_normal = normalizeVector(projected_normal);

              //check if behind camera.
              point center = calcCenter(projected_tri);
              if(center.z < 5)
                continue;

              //Check normal (backface culling)
              if(projected_normal.z > 0){
                
                point world_normal = calcNormal(tris[i]);
                world_normal = normalizeVector(world_normal);

                double lightness = pow(dotProduct(world_normal, light_direction), 1);
                lightness = lightness < 0 ? 0.0 : lightness;
                unsigned int color = colorLightness(lightness, tris[i].color);
                SDL_SetRenderDrawColor(renderer, 0x0000FF&color>>16, (0x00FF00&color)>>8, 0x0000FF&color, 255);
                
                rasterizeTriangle(renderer, projected_tri);
                
                if(wireframe){
                SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
                triangle scaledTri = (triangle){
                    {projected_tri.a.x*resScale, projected_tri.a.y*resScale, projected_tri.a.z},
                    {projected_tri.b.x*resScale, projected_tri.b.y*resScale, projected_tri.b.z},
                    {projected_tri.c.x*resScale, projected_tri.c.y*resScale, projected_tri.c.z},
                    projected_tri.color
                };
                drawTriangle(renderer, scaledTri);
                }
              }
              //tris[i] = rotateTriX(tris[i], 0.007, 0, -100, 800);
              //tris[i] = rotateTriY(tris[i], 0.013, 0, -100, 800);
              //tris[i] = rotateTriZ(tris[i], 0.003, 0, -100, 800);
          }
        }          
        SDL_RenderPresent(renderer);

        
        const Uint8* keystates = SDL_GetKeyboardState(NULL);
        
        while(SDL_PollEvent(&evt)){
            if(evt.type == SDL_KEYDOWN){
                int keypressed = evt.key.keysym.sym;
                if (keypressed == SDLK_ESCAPE){
                    running = 0;
                } else if(keypressed == SDLK_l){//l
                    wireframe = !wireframe;
                } 
            } else if(evt.type == SDL_MOUSEMOTION){
                if(evt.motion.x != WIDTH*resScale/2 && evt.motion.y != HEIGHT*resScale/2){
                    int dx = evt.motion.xrel;
                    int dy = evt.motion.yrel;
                    yawCamera((double)dx/100);
                    pitchCamera((double)dy/100);
                }
            }
        }

        if(keystates[SDL_SCANCODE_W]){ //w
            movCamera(0.0, 0.0, speed);
        }if(keystates[SDL_SCANCODE_A]){//a
            movCamera(-speed, 0.0, 0.0);
        }if(keystates[SDL_SCANCODE_S]){//s
            movCamera(0.0, 0.0, -speed);
        }if(keystates[SDL_SCANCODE_D]){//d
            movCamera(speed, 0.0, 0.0);
        }if(keystates[SDL_SCANCODE_R]){//r
            movCamera(0.0, -speed, 0.0);
        }if(keystates[SDL_SCANCODE_F]){//f
            movCamera(0.0, speed, 0.0);
        }if(keystates[SDL_SCANCODE_Q]){//q
            yawCamera(-0.01);
        }if(keystates[SDL_SCANCODE_E]){//e
            yawCamera(0.01);
        }if(keystates[SDL_SCANCODE_G]){//g
            pitchCamera(0.01);
        }if(keystates[SDL_SCANCODE_T]){//t
            pitchCamera(-0.01);
        }if(keystates[SDL_SCANCODE_J]){//j
            camera_dist+= 2*elapsed_time*TIME_CONST;
        }if(keystates[SDL_SCANCODE_K]){//k
            camera_dist-= 2*elapsed_time*TIME_CONST;
        }if(keystates[SDL_SCANCODE_O]){//o
          for(int j = 0; j < nObj; j++){
            triangle* tris = objects[j].tris;
            for(int i = 0; i < objects[j].nFaces; i++){
              tris[i] = rotateTriX(tris[i], 0.007, 0, 0, 300);
              tris[i] = rotateTriY(tris[i], 0.013, 0, 0, 300);
              tris[i] = rotateTriZ(tris[i], 0.003, 0, 0, 300);
            }
          }
        }

    }
    //for(int j = 0; j < nObj; j++)
    //  free(objects[j].tris);
    //free(objects);

    terminate();
}