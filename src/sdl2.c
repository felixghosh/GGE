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
#include "light.h"

#define MAXOBJ 100
#define MAXLIGHT 100

SDL_Window* screen = NULL;
SDL_Renderer* renderer;
SDL_Event evt;
SDL_Rect source, destination, dst;

bool debug = false;

double camera_dist = 554.0; //For FOV 60

point camera_dir = {0.0, 0.0, 1.0};

struct timespec t0, t1;
double elapsed_time;

double speed = 10.0;
double speed_fast = 30.0;
double speed_slow = 10.0;
double resScale = 1;

point camera_pos = {0.0, 0.0, 0.0};
double camera_angle_y = 0.0;
double camera_angle_x = 0.0;

//point light_direction = {0.3, -0.2, 0.5};

bool wireframe = false;




object* objects;
unsigned int nObj = 0;

light* lights;
unsigned int nLights = 0;

triangle camera_basis = { //Currently not used
  {1.0, 0.0, 0.0},
  {0.0, 1.0, 0.0},
  {0.0, 0.0, 1.0},
  0
};

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
  
  int dy_last = dy_long - dy_short;//round((p[2].y - p[1].y));
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
        //printf("drawing from (%lf, %lf) to (%lf, %lf)\n", slope_long[i], (i+p[0].y)*resScale+j, slope_short[i]*resScale, ((i+p[0].y)*resScale+j));
      }
  }
  if(dy_last != 0){
    int origin = i;
    for(i; i < dy_long; i++)
      for(int j = 0; j < resScale; j++){
        SDL_RenderDrawLine(renderer, slope_long[i]*resScale, (i+p[0].y)*resScale+j, (int)slope_last[i - origin]*resScale, (i+p[0].y)*resScale+j);
        //printf("drawing from (%lf, %lf) to (%lf, %lf)\n", slope_long[i], (i+p[0].y)*resScale+j, slope_last[i - origin]*resScale, ((i+p[0].y)*resScale+j));
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
  object obj = {tris, nFaces};
  return obj;
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


bool triBehind(triangle tri){
  return tri.a.z < 0 && tri.b.z < 0 && tri.c.z < 0;
}



int main(int argc, char* argv[]){
  initialize();
  int running = 1;
  int i = 0;  

  objects = malloc(MAXOBJ*sizeof(object));
  object teapot = loadOBJ("/home/felixghosh/prog/c/GGE/OBJ/teapot.obj", 0xDF2332, 0, 0, 300, 100);
  object cube = loadOBJ("/home/felixghosh/prog/c/GGE/OBJ/cube.obj", 0x23DF32, 0, 0, 400, 100);
  object monkey = loadOBJ("/home/felixghosh/prog/c/GGE/OBJ/monkey.obj", 0x2323DF, 0, -300, 400, 100);
  object tri = loadOBJ("/home/felixghosh/prog/c/GGE/OBJ/tri.obj", 0x23D33F, 0, 0, 400, 100);
  object dog = loadOBJ("/home/felixghosh/prog/c/GGE/OBJ/dog.obj", 0x23D33F, 0, 0, 400, 100);
  object get = loadOBJ("/home/felixghosh/prog/c/GGE/OBJ/get.obj", 0x23D33F, 0, 0, 400, 100);
  object room = loadOBJ("/home/felixghosh/prog/c/GGE/OBJ/room3.obj", 0xE3737F, 0, 100, 0, 1000);
  object sphere = loadOBJ("/home/felixghosh/prog/c/GGE/OBJ/sphere.obj", 0xD3b3bF, 0, 100, 0, 100);
  
  objects[nObj++] = room;
  objects[nObj++] = cube;
  objects[nObj++] = monkey;
  //objects[nObj++] = tri;
  //objects[nObj++] = dog;
  //bjects[nObj++] = get;
  //objects[nObj++] = teapot;
  //objects[nObj++] = sphere;

  lights = malloc(sizeof(light)*MAXLIGHT);
  lights[nLights++] = (light){(point){200.0, 200.0, 200.0}, 1000.0};
  lights[nLights++] = (light){(point){-5000.0, 100.0, 5000.0}, 3000.0};
  //lights[nLights++] = (light){(point){0.0, -10000.0, 0.0}, 20000};

  unsigned long totalTris = 0;
  for(int i = 0; i < nObj; i++)
    totalTris += objects[i].nFaces;

  tri_map* allTris = malloc(totalTris*sizeof(tri_map));
  unsigned long index = 0;
  for(int i = 0; i < nObj; i++){
    for(int j = 0; j < objects[i].nFaces; j++){
      allTris[index++] = (tri_map){&(objects[i].tris[j]), &(objects[i])};
    }
  }
  if(index != totalTris){
    printf("ERROR INDEX DOESNT MATCH TOTALTRIS!\nindex %lu, totalTris %lu\n", index, totalTris);
    exit(1);
  }

  clock_gettime(CLOCK_REALTIME, &t0);


    while(running){
        clock_gettime(CLOCK_REALTIME, &t1);
        elapsed_time = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec)/1000000000.0;
        clock_gettime(CLOCK_REALTIME, &t0);
        printf("fps: %5u\n", (int)(1/elapsed_time));
        //printf("fov: %5u\n", (int)calcFOV());

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

        //sort triangles for painters algorithm
        qsort(allTris, totalTris, sizeof(tri_map), cmpfunc);

        for(int i = 0; i < totalTris; i++){
          triangle tri = *(allTris[i].tri);
          triangle cam_tri = toCameraBasisTriangle(tri);

          //CLIPPING AGAINST CAMERA Z-PLANE
          triangle* clipped_tris_z = malloc(2*sizeof(triangle));
          clipped_tris_z[0] = cam_tri;
          unsigned int nTrisZ = 1;
          clipEdge((point){0,0,camera_dist} , (point){WIDTH,HEIGHT,camera_dist}, &clipped_tris_z, &nTrisZ, 0, 'z');

          for(int j = 0; j < nTrisZ; j++){
            triangle projected_tri = projectTriangle(clipped_tris_z[j]);
            point projected_normal = calcNormal(projected_tri);
            projected_normal = normalizeVector(projected_normal);

            //check if behind camera.
            //point center = calcCenter(projected_tri);
            //if(center.z < camera_dist/10)
            //  continue;

            //Check normal (backface culling)
            if(projected_normal.z > 0){
              
              point world_normal = normalizeVector(calcNormal(tri));
              
              double lightness = 0.0;
              for(int i = 0; i < nLights; i++){
                point light_direction = normalizeVector(subtractPoints(calcCenter(tri), lights[i].p));
                double light_dist = vectorLength(subtractPoints(calcCenter(tri), lights[i].p));
                double partial_light = (lights[i].intensity/pow(light_dist, 1.1))*dotProduct(world_normal, light_direction);
                partial_light = partial_light < 0 ? 0 : partial_light;
                lightness += partial_light;
              }
              unsigned int color = colorLightness(lightness, tri.color);
              
              
              //CLIPPING AGAINST SCREEN BORDERS
              unsigned int nTris = 1;
              triangle* clipped_tris = malloc(16*sizeof(triangle));
              clipped_tris[0] = projected_tri;
              clipTriangle(&clipped_tris, &nTris);

              //RENDERING
              for(int i = 0; i < nTris; i++){
                if(!wireframe){
                  SDL_SetRenderDrawColor(renderer, 0x0000FF&color>>16, (0x00FF00&color)>>8, 0x0000FF&color, 255);
                  rasterizeTriangle(renderer, clipped_tris[i]);
                } else{
                  SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
                  triangle scaledTri = (triangle){
                      {clipped_tris[i].a.x*resScale, clipped_tris[i].a.y*resScale, clipped_tris[i].a.z},
                      {clipped_tris[i].b.x*resScale, clipped_tris[i].b.y*resScale, clipped_tris[i].b.z},
                      {clipped_tris[i].c.x*resScale, clipped_tris[i].c.y*resScale, clipped_tris[i].c.z},
                      clipped_tris[i].color
                  };
                  drawTriangle(renderer, scaledTri);
                }
              }
              free(clipped_tris);
            }
          }
          free(clipped_tris_z);
        }
        //}
        //SWITCH BUFFERS          
        SDL_RenderPresent(renderer);

        
        //HANDLE INPUT
        const Uint8* keystates = SDL_GetKeyboardState(NULL);
        
        while(SDL_PollEvent(&evt)){
            if(evt.type == SDL_KEYDOWN){
                int keypressed = evt.key.keysym.sym;
                if (keypressed == SDLK_ESCAPE){
                    running = 0;
                } else if(keypressed == SDLK_l){//l
                    wireframe = !wireframe;
                } else if(keypressed == SDLK_i){//i
                    debug = !debug;
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
        }if(keystates[SDL_SCANCODE_U]){//u
          lights[0].p.z += 30;
        }if(keystates[SDL_SCANCODE_Y]){//y
          lights[0].p.z -= 30;
        }if(keystates[SDL_SCANCODE_LSHIFT]){
          speed = speed_fast;
        }else{
          speed = speed_slow;
        }

    }

    //PROGRAM EXIT
    for(int j = 0; j < nObj; j++)
      free(objects[j].tris);
    free(objects);
    free(lights);
    free(allTris);

    terminate();
}