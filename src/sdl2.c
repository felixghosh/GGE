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

SDL_Window* screen = NULL;
SDL_Renderer* renderer;
SDL_Event evt;
SDL_Rect source, destination, dst;

double camera_dist = 554.0; //For FOV 60

struct timespec t0, t1;
double elapsed_time;

double speed = 3.0;
double resScale = 4;

point camera_pos = {0.0, 0.0, 0.0};
double camera_angle_y = 0.0;
double camera_angle_x = -0.1;

int lastMouseX = WIDTH/2;
int lastMouseY = HEIGHT/2;
int mouseX, mouseY;

point light_direction = {0.0, 0.0, 1.0};

unsigned int cubeColor = 0xFF0000;

bool wireframe = false;

triangle camera_basis = { //Currently not used
  {1.0, 0.0, 0.0},
  {0.0, 1.0, 0.0},
  {0.0, 0.0, 1.0},
  0
};

triangle tri0 = {
  {0, 0, 100},
  {100, 0, 100},
  {0, 100, 100},
  0xFF0000
  
};
triangle tri1 = {
  {0, 100, 100},
  {100, 0, 100},
  {100, 100, 100},
  0xFF0000
};
triangle tri2 = {
  {0, 0, 100},
  {0, 100, 100},
  {0, 0, 200},
  0xFF0000
};
triangle tri3 = {
  {0, 0, 200},
  {0, 100, 100},
  {0, 100, 200},
  0xFF0000
};
triangle tri4 = {
  {0, 0, 200},
  {0, 100, 200},
  {100, 0, 200},
  0xFF0000
};
triangle tri5 = {
  {0, 100, 200},
  {100, 100, 200},
  {100, 0, 200},
  0xFF0000

};
triangle tri6 = {
  {100, 0, 100},
  {100, 0, 200},
  {100, 100, 100},
  0xFF0000
};
triangle tri7 = {
  {100, 0, 200},
  {100, 100, 200},
  {100, 100, 100},
  0xFF0000

};
triangle tri8 = {
  {0, 0, 100},
  {0, 0, 200},
  {100, 0, 100},
  0xFF0000

};
triangle tri9 = {
  {100, 0, 100},
  {0, 0, 200},
  {100, 0, 200},
  0xFF0000
};
triangle tri10 = {
  {0, 100, 100},
  {100, 100, 100},
  {0, 100, 200},
  0xFF0000
};
triangle tri11 = {
  {100, 100, 100},
  {100, 100, 200},
  {0, 100, 200},
  0xFF0000
};

int cmpfunc (const void * a, const void * b) {
   triangle* pa = (triangle*)a;
   triangle* pb = (triangle*)b;
   return (calcCenter(*pb).z - calcCenter(*pa).z);
}

double radToDeg(double rad){
  double deg = (rad * 360.0) / (2*M_PI);
  return deg;
}

double calcFOV(){
  double fov = 2 * atan((WIDTH/2)/camera_dist);
  return radToDeg(fov);
}

unsigned int colorLightness(double value, unsigned int color){
  unsigned int r, g, b, rValue, gValue, bValue;
  r = 0x00FF0000 & color;
  g = 0x0000FF00 & color;
  b = 0x000000FF & color;

  r = (int)(r * value);
  g = (int)(g * value);
  b = (int)(b * value);

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
  double denominator = 1 / dy_long;
  double slope_long[dy_long];
  for(i = 0; i < dy_long; i++){
    slope_long[i] = p[0].x + (p[2].x-p[0].x)*(i)/dy_long;
  }
  int dy_short = (p[1].y - p[0].y);
  denominator = 1 / dy_short;
  double slope_short[dy_short];
  if(dy_short != 0){
    for(i = 0; i < dy_short; i++){
      slope_short[i] = p[0].x + (p[1].x-p[0].x)*i/dy_short;
    }
  }
  int dy_last = (p[2].y - p[1].y);
  denominator = 1 / dy_last;
  double slope_last[dy_last];
  if(dy_last != 0){
    for(i = 0; i < dy_last; i++){
      slope_last[i] = p[1].x + (p[2].x-p[1].x)*i/dy_last;
    }
  }
  //scanline
  i = 0;
  if(dy_short != 0){
    for(i; i < dy_short; i++)
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

triangle roundTriangle(triangle tri){
  tri.a.x = round(tri.a.x);
  tri.a.y = round(tri.a.y);
  tri.a.z = round(tri.a.z);
  tri.b.x = round(tri.b.x);
  tri.b.x = round(tri.b.x);
  tri.b.x = round(tri.b.x);
  tri.c.x = round(tri.c.x);
  tri.c.x = round(tri.c.x);
  tri.c.x = round(tri.c.x);
  return tri;
}



int main(int argc, char* argv[]){
    initialize();
    int running = 1;
    int i = 0;

    clock_gettime(CLOCK_REALTIME, &t0);
  triangle* tris = malloc(24*sizeof(triangle));
  tris[0] = tri0;
  tris[1] = tri1;
  tris[2] = tri2;
  tris[3] = tri3;
  tris[4] = tri4;
  tris[5] = tri5;
  tris[6] = tri6;
  tris[7] = tri7;
  tris[8] = tri8;
  tris[9] = tri9;
  tris[10] = tri10;
  tris[11] = tri11;
  tris[12] = tri0;
  tris[13] = tri1;
  tris[14] = tri2;
  tris[15] = tri3;
  tris[16] = tri4;
  tris[17] = tri5;
  tris[18] = tri6;
  tris[19] = tri7;
  tris[20] = tri8;
  tris[21] = tri9;
  tris[22] = tri10;
  tris[23] = tri11;
  tris[12].color = 0x00FF00;
  tris[13].color = 0x00FF00;
  tris[14].color = 0x00FF00;
  tris[15].color = 0x00FF00;
  tris[16].color = 0x00FF00;
  tris[17].color = 0x00FF00;
  tris[18].color = 0x00FF00;
  tris[19].color = 0x00FF00;
  tris[20].color = 0x00FF00;
  tris[21].color = 0x00FF00;
  tris[22].color = 0x00FF00;
  tris[23].color = 0x00FF00;

  for(int i = 0; i < 12; i++)
    tris[i] = translateTriangle(tris[i], 0, -100, 700);
  for(int i = 12; i < 24; i++)
    tris[i] = translateTriangle(tris[i], 0, 150, 700);

    while(running){
        clock_gettime(CLOCK_REALTIME, &t1);
        elapsed_time = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec)/1000000000.0;
        clock_gettime(CLOCK_REALTIME, &t0);
        printf("fps: %5u\n", (int)(1/elapsed_time));

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        

        qsort(tris, 24, sizeof(triangle), cmpfunc);


        for(int i = 0; i < 24; i++){
        //for(int i = 0; i < nFaces; i++){
            //Check normal
            triangle projected_tri = projectTriangle(tris[i]);
            point normal, line1, line2;
            line1.x = projected_tri.b.x - projected_tri.a.x;
            line1.y = projected_tri.b.y - projected_tri.a.y;
            line1.z = projected_tri.b.z - projected_tri.a.z;

            line2.x = projected_tri.c.x - projected_tri.a.x;
            line2.y = projected_tri.c.y - projected_tri.a.y;
            line2.z = projected_tri.c.z - projected_tri.a.z;

            normal.x = line1.y*line2.z - line1.z*line2.y;
            normal.y = line1.z*line2.x - line1.x*line2.z;
            normal.z = line1.x*line2.y - line1.y*line2.x;
            
            double normalLength = sqrt(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
            normal.x /= normalLength; normal.y /= normalLength; normal.z /= normalLength;
            normal.x = round(normal.x*100)/100; normal.y = round(normal.y*100)/100; normal.z = round(normal.z*100)/100;
            if(normal.z > 0){
                
                double lightLength = sqrt(light_direction.x*light_direction.x + light_direction.y*light_direction.y + light_direction.z*light_direction.z);
                light_direction.x /= lightLength; light_direction.y /= lightLength; light_direction.z /= lightLength;
                light_direction.x = round(light_direction.x*100)/100; light_direction.y = round(light_direction.y*100)/100; light_direction.z = round(light_direction.z*100)/100;
                double dp = normal.x * light_direction.x + normal.y*light_direction.y + normal.z*light_direction.z;
                
                unsigned int color = colorLightness(dp, tris[i].color);
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
            
            tris[i] = rotateX(tris[i], 0.007, 0, -100, 800);
            tris[i] = rotateY(tris[i], 0.013, 0, -100, 800);
            tris[i] = rotateZ(tris[i], 0.003, 0, -100, 800);
        }
        SDL_RenderPresent(renderer);

        
        const Uint8* keystates = SDL_GetKeyboardState(NULL);
        
        while(SDL_PollEvent(&evt)){
            if(evt.type == SDL_KEYDOWN){
                int keypressed = evt.key.keysym.sym;
                //printf("%d\n", keypressed);
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
                    pitchCamera((double)-dy/100);
                }
            }
        }

        if(keystates[SDL_SCANCODE_W]){ //w
            movCamera(1.0, 1.0, speed);
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
        }if(keystates[SDL_SCANCODE_Q]){//g
            pitchCamera(-0.01);
        }if(keystates[SDL_SCANCODE_T]){//t
            pitchCamera(0.01);
        }if(keystates[SDL_SCANCODE_J]){//j
            camera_dist+= 2*elapsed_time*TIME_CONST;
        }if(keystates[SDL_SCANCODE_K]){//k
            camera_dist-= 2*elapsed_time*TIME_CONST;
        }

    }

    terminate();
}