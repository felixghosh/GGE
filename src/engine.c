#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
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



double camera_dist = 330;

point camera_dir = {0.0, 0.0, 1.0};

struct timespec t0, t1;

double elapsed_time;

double resScale = 1;

double camera_angle_y = 0.0;
double camera_angle_x = 0.0;


triangle camera_basis = { //Currently not used
  {1.0, 0.0, 0.0},
  {0.0, 1.0, 0.0},
  {0.0, 0, 1.0},
  0
};

unsigned int texture[2][2] = 
{{0xff0000,0x00ff00}, 
 {0x0000ff,0xffff00}};

unsigned int* load_texture(unsigned int width, unsigned int height, char* filepath){
  
}


object translateObject(object obj, double x, double y, double z){
  for(int i = 0; i < obj.nFaces; i++){
    obj.tris[i] = translateTriangle(obj.tris[i], x, y, z);
  }
  obj.pos = translatePoint(obj.pos, x, y, z);
  return obj;
}

object rotateObjectX(object obj, double angle, double x, double y, double z){
  for(int i = 0; i < obj.nFaces; i++)
    obj.tris[i] = rotateTriX(obj.tris[i], angle, x, y, z);
  obj.pos = rotatePointX(obj.pos, angle, x, y, z);
  return obj;
}

object rotateObjectY(object obj, double angle, double x, double y, double z){
  for(int i = 0; i < obj.nFaces; i++)
    obj.tris[i] = rotateTriY(obj.tris[i], angle, x, y, z);
  obj.pos = rotatePointY(obj.pos, angle, x, y, z);
  return obj;
}

object rotateObjectZ(object obj, double angle, double x, double y, double z){
  for(int i = 0; i < obj.nFaces; i++)
    obj.tris[i] = rotateTriZ(obj.tris[i], angle, x, y, z);
  obj.pos = rotatePointZ(obj.pos, angle, x, y, z);
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
   tri_map* tma = (tri_map*)a;
   tri_map* tmb = (tri_map*)b;
   triangle* pa = tma->tri;
   triangle* pb = tmb->tri;
   return (vectorLength(subtractPoints(calcCenter(*pb), camera_pos)) - vectorLength(subtractPoints(calcCenter(*pa), camera_pos)));
}

double radToDeg(double rad){
  double deg = (rad * 360.0) / (2*M_PI);
  return deg;
}

double calcFOV(){
  double fov = 2 * atan((WIDTH/2)/camera_dist);
  return radToDeg(fov);
}

unsigned int interpolateColor(unsigned int colors[3], point bcc){
  unsigned int r, g, b, r0, g0, b0, r1, g1, b1, r2, g2, b2, rValue, gValue, bValue;
  r = 0x00FF0000;
  g = 0x0000FF00;
  b = 0x000000FF;
  r0 = (unsigned int)((double)(colors[1] & r)*bcc.x);
  g0 = (unsigned int)((double)(colors[1] & g)*bcc.x);
  b0 = (unsigned int)((double)(colors[1] & b)*bcc.x);
  r1 = (unsigned int)((double)(colors[2] & r)*bcc.y);
  g1 = (unsigned int)((double)(colors[2] & g)*bcc.y);
  b1 = (unsigned int)((double)(colors[2] & b)*bcc.y);
  r2 = (unsigned int)((double)(colors[0] & r)*bcc.z);
  g2 = (unsigned int)((double)(colors[0] & g)*bcc.z);
  b2 = (unsigned int)((double)(colors[0] & b)*bcc.z);
  
  rValue = ((r0 + r1 + r2) & r);
  gValue = ((g0 + g1 + g2) & g);
  bValue = ((b0 + b1 + b2) & b);

  return rValue + gValue + bValue;
}

point interpolateTexCoords(triangle tri, point bcc){
  double u = tri.texA.x * bcc.x + tri.texB.x * bcc.y + tri.texC.x * bcc.z;
  double v = tri.texA.y * bcc.x + tri.texB.y * bcc.y + tri.texC.y * bcc.z;
  // printf("Ax: %2.1lf,Bx: %2.1lf, Cx: %2.1lf\n", tri.texA.x, tri.texB.x, tri.texC.x);
  // printf("u: %2.2lf, v: %2.2lf\n", u, v);
  return (point){u, v, 0.0};
}

point interpolatePoint(point a, point b, point c, point bcc){
  double x0, x1, x2, y0, y1, y2, z0, z1, z2;
  x0 = a.x * bcc.x;
  x1 = b.x * bcc.y;
  x2 = c.x * bcc.z;
  y0 = a.y * bcc.x;
  y1 = b.y * bcc.y;
  y2 = c.y * bcc.z;
  z0 = a.z * bcc.x;
  z1 = b.z * bcc.y;
  z2 = c.z * bcc.z;

  return (point){(x0+x1+x2), (y0+y1+y2), (z0+z1+z2)};
}

unsigned int sampleTexture(double u, double v, int len){
  unsigned int r, g, b, rshift, gshift, rValue, gValue, bValue;
  int u_index = (int)(u*len);
  int v_index = (int)(v*len);
  u_index = u_index == len ? len-1 : u_index;
  v_index = v_index == len ? len-1 : v_index;
  r = 0x00FF0000;
  g = 0x0000FF00;
  b = 0x000000FF;
  rshift = 16;
  gshift = 8;

  rValue = (texture[v_index][u_index]) & r;
  gValue = (texture[v_index][u_index]) & g;
  bValue = (texture[v_index][u_index]) & b;

  return rValue + gValue + bValue;
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
        
        SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, SDL_WINDOW_SHOWN, &screen, &renderer);
        if(!screen)
            printf("InitSetup failed to create window\n");

        SDL_SetWindowTitle(screen, "GGE");
        if(fullscreen)
          SDL_SetWindowFullscreen(screen, SDL_WINDOW_FULLSCREEN);
        
        //Initialize SDL_ttf
        if(TTF_Init() == -1){
          printf("TTF could not be initialized! SDL_ttf Error: %s\n", TTF_GetError());
        }

        //Initialize SDL_mixer
        if( Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 512 ) < 0 )
        {
            printf( "SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
        }

        if(!load_media()){
          printf("Could not load all media!\n");
        }
}

void terminate_engine(){
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(screen);
    free_media();
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

void set_pixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
  Uint32 * const target_pixel = (Uint32 *) ((Uint8 *) surface->pixels
                                             + y * surface->pitch
                                             + x * surface->format->BytesPerPixel);
  *target_pixel = pixel;
}

void rasterizeTriangle(SDL_Renderer* renderer, triangle tri, SDL_Surface* surf, unsigned int colors[3]){
  //sort points by height
  int i ;
  point p[3];
  p[0] = tri.a;
  p[1] = tri.b;
  p[2] = tri.c;

  for(int i = 0; i < 3; i++){
    p[i].x = round(p[i].x);
    p[i].y = round(p[i].y);
    p[i].z = round(p[i].z);
  }
  sortPoints(p, 0, 1);
  sortPoints(p, 1, 2);
  sortPoints(p, 0, 1);
  // point p1 = (point){-3, -2.0, 200.0};
  // p1 = projectPoint(toCameraBasis(p1));
  // point b1 = calcBCC(p1, tri);
  // point tc1 = interpolateTexCoords(tri, b1);
  // unsigned int c1 = sampleTexture(tc1.x, tc1.y, 2);
  // point p2 = (point){-3.0, -2.0, 200.0};
  // p2 = projectPoint(toCameraBasis(p2));
  // point b2 = calcBCC(p2, tri);
  // point tc2 = interpolateTexCoords(tri, b2);
  // unsigned int c2 = sampleTexture(tc2.x, tc2.y, 2);
  // printf("bcc x: %2.2lf, y: %2.2lf, z: %2.2lf\n", b1.x, b1.y, b1.z);
  // printf("u: %2.2lf, v: %2.2lf\n", tc1.x, tc1.y);
  // printf("2u: %2.2lf, v: %2.2lf\n", tc2.x, tc2.y);
  // printf("color: %6x\n", c1);
  // printf("2color: %6x\n", c2);
  // printf("u: %lf v: %lf\n", tri.texA, tri.texB);
  
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
  unsigned int cols[3] = {0xff0000, 0x00ff00, 0x0000ff};
  i = 0;
  if(dy_short != 0){
    for(i; i < dy_short; i++){
      if((slope_short[i]) - (slope_long[i]) < 0){
        for(int k  = slope_long[i]; k > slope_short[i]; k--){
          point bcc = calcBCC((point){(double)k, (double)(i+p[0].y), (double)0.0}, tri);
          point texCoords = interpolateTexCoords(tri, bcc);
          unsigned int texture_color = sampleTexture(texCoords.x, texCoords.y, 2);
          unsigned int interpolated_color = interpolateColor(cols, bcc);
          set_pixel(surf, k, (i+p[0].y), texture_color);
        }
      } else{
        for(int k  = slope_long[i]; k <= slope_short[i]; k++){
          point bcc = calcBCC((point){(double)k, (double)(i+p[0].y), (double)0.0}, tri);
          point texCoords = interpolateTexCoords(tri, bcc);
          unsigned int texture_color = sampleTexture(texCoords.x, texCoords.y, 2);
          unsigned int interpolated_color = interpolateColor(cols, bcc);
          set_pixel(surf, k, (i+p[0].y), texture_color);
        }
      }
    }      
  }
  if(dy_last != 0){
    int origin = i;
    for(i; i < dy_long; i++){
      if((slope_last[i - origin]) - (slope_long[i]) < 0){
        for(int k = slope_long[i]; k > slope_last[i - origin]; k--){
          point bcc = calcBCC((point){(double)k, (double)(i+p[0].y), (double)0.0}, tri);
          point texCoords = interpolateTexCoords(tri, bcc);
          unsigned int texture_color = sampleTexture(texCoords.x, texCoords.y, 2);
          unsigned int interpolated_color = interpolateColor(cols, bcc);
          set_pixel(surf, k, (i+p[0].y), texture_color);
        }
      }else{
        for(int k = slope_long[i]; k <= slope_last[i - origin]; k++){
          point bcc = calcBCC((point){(double)k, (double)(i+p[0].y), (double)0.0}, tri);
          //printf("bcc: %3.1lf %3.1lf %3.1lf\n", bcc.x, bcc.y, bcc.z);   
          point texCoords = interpolateTexCoords(tri, bcc);
          unsigned int texture_color = sampleTexture(texCoords.x, texCoords.y, 2);
          unsigned int interpolated_color = interpolateColor(cols, bcc);
          set_pixel(surf, k, (i+p[0].y), texture_color);
        }
      }
    }
  }
}

object loadOBJ(const char* filePath, unsigned int color, double x, double y, double z, double scale){
  unsigned long int nFaces = 0;
  unsigned long int nVertices = 0;
  unsigned long int nCoords = 0;
  FILE* fp;
  fp = fopen(filePath, "r");
  
  size_t buf_size = 50;
  char* buf = malloc(buf_size*sizeof(char));
  char* endptr;

  do{
    getline(&buf, &buf_size, fp);
    nVertices++;
  } while(buf[0] == 'v');

  getline(&buf, &buf_size, fp);

  do{
    getline(&buf, &buf_size, fp);
    nCoords++;
  } while(buf[0] == 'v');

  while(getline(&buf, &buf_size, fp)>0)
    nFaces++;
  nVertices--;

  printf("nvert:%d nface:%d\n", nVertices, nFaces);

  rewind(fp);

  point* vertices = malloc(nVertices * sizeof(point));
  for(int i = 0; i < nVertices; i++){
    getline(&buf, &buf_size, fp);
    endptr = buf;
    double values[3];
    for(int i = 0; i < 3; i++){
      values[i] = scale*strtod(endptr+1, &endptr);
      // printf("%lf\n", values[i]);
    }
    // printf("\n");
    vertices[i] = (point){-values[0] + x, -values[1] + y, -values[2] + z};
  }
  
  getline(&buf, &buf_size, fp);

  point* texCoords = malloc(nCoords * sizeof(point));

  for(int i = 0; i < nCoords; i++){
    getline(&buf, &buf_size, fp);
    endptr = buf;
    double values[2];
    values[0] = strtod(endptr+2, &endptr);
    values[1] = strtod(endptr+1, &endptr);
    // printf("tex %lf\n", values[0]);
    // printf("tex %lf\n", values[1]); 
    texCoords[i] = (point){values[0], values[1], 0.0};
  }

  getline(&buf, &buf_size, fp);

  triangle* tris = malloc(nFaces * sizeof(triangle));
  for(int i = 0; i < nFaces; i++){
    getline(&buf, &buf_size, fp);
    endptr = buf;
    long int values[3];
    long int tex[3];
    for(int i = 0; i < 3; i++){
      values[i] = strtol(endptr+1, &endptr, 10) - 1;
      tex[i] = strtol(endptr+1, &endptr, 10) - 1;
      // printf("val %ld\n", values[i]);
      // printf("tex %ld\n", tex[i]); 
    }
      
    tris[i] = (triangle){
      vertices[values[0]],
      vertices[values[1]],
      vertices[values[2]],
      color,
      texCoords[tex[0]],
      texCoords[tex[1]],
      texCoords[tex[2]]
      };

      // printf("trix: %2.1lf, %2.1lf, %2.1lf\ntexu: %2.1lf, %2.1lf, %2.1lf\n", vertices[values[0]].x, vertices[values[1]].x, vertices[values[2]].x, texCoords[tex[0]].x, texCoords[tex[1]].x,texCoords[tex[2]].x);
  }
  free(buf);
  object obj = {tris, nFaces, (point){x,y,z}};
  return obj;
}

void drawText(SDL_Renderer* renderer, const char* message, int x, int y, int width, int height, unsigned int color, int pt){
  TTF_Font* font = TTF_OpenFont("fonts/TerminusTTF-4.49.2.ttf", pt);
  if(font == NULL){
    printf("FONT COULD NOT BE LOADED!\n");
    exit(1);
  }
  SDL_Color text_color = {(0xFF0000&color)>>16, (0x00FF00&color)>>8, 0x0000FF&color, 0xFF};
  SDL_Surface* surfaceMessage = TTF_RenderText_Solid(font, message, text_color);
  SDL_Texture* Message = SDL_CreateTextureFromSurface(renderer, surfaceMessage); 

  SDL_Rect Message_rect;
  Message_rect.x = x;
  Message_rect.y = y;
  Message_rect.w = width; 
  Message_rect.h = height;

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
    intersect.z = round(((p0.x - value)/(p0.x - p1.x))*(p1.z - p0.z) + p0.z);
  } else if(axis == 'y'){
    intersect.x = round(((p0.y - value)/(p0.y - p1.y))*(p1.x - p0.x) + p0.x);
    intersect.y = value;
    intersect.z = round(((p0.y - value)/(p0.y - p1.y))*(p1.z - p0.z) + p0.z);
  } else if(axis == 'z'){
    intersect.x = round(((p0.z - value)/(p0.z - p1.z))*(p1.x - p0.x) + p0.x);
    intersect.y = round(((p0.z - value)/(p0.z - p1.z))*(p1.y - p0.y) + p0.y);
    intersect.z = value;
  }
  return intersect;
}

void clipEdge(point p1, point p2, triangle* clipped_tris, unsigned int* nTris, int* index, char axis){
  triangle tri = clipped_tris[*index];
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

    double ratio1x = points[(firstOut+1)%3].x == points[firstOut].x ? 1.0 : (intersect1.x-points[firstOut].x)/(points[(firstOut+1)%3].x - points[firstOut].x);
    double ratio2x = points[(firstOut+2)%3].x == points[firstOut].x ? 1.0 : (intersect2.x-points[firstOut].x)/(points[(firstOut+2)%3].x - points[firstOut].x);
    double ratio1y = points[(firstOut+1)%3].y == points[firstOut].y ? 1.0 : (intersect1.y-points[firstOut].y)/(points[(firstOut+1)%3].y - points[firstOut].y);
    double ratio2y = points[(firstOut+2)%3].y == points[firstOut].y ? 1.0 : (intersect2.y-points[firstOut].y)/(points[(firstOut+2)%3].y - points[firstOut].y);


    point tex1, tex2;

    if(firstOut == 0){
      tex1 = calcInterpolatedTexCoords(tri.texB, tri.texA, ratio1x, ratio1y);
      tex2 = calcInterpolatedTexCoords(tri.texC, tri.texA, ratio2x, ratio2y);
      clipped_tris[*index] = (triangle){intersect1, points[(firstOut+1)%3], points[(firstOut+2)%3], tri.color, tex1, tri.texB, tri.texC};
      clipped_tris[*nTris] = (triangle){intersect1, points[(firstOut+2)%3], intersect2, tri.color, tex1, tri.texC, tex2};
      (*nTris)++;
    }
    else if(firstOut == 1){
      tex1 = calcInterpolatedTexCoords(tri.texC, tri.texB, ratio1x, ratio1y);
      tex2 = calcInterpolatedTexCoords(tri.texA, tri.texB, ratio2x, ratio2y);
      clipped_tris[*index] = (triangle){intersect1, points[(firstOut+1)%3], points[(firstOut+2)%3], tri.color, tex1, tri.texC, tri.texA};
      clipped_tris[*nTris] = (triangle){intersect1, points[(firstOut+2)%3], intersect2, tri.color, tex1, tri.texA, tex2};
      (*nTris)++;
    }
    else{
      tex1 = calcInterpolatedTexCoords(tri.texA, tri.texC, ratio1x, ratio1y);
      tex2 = calcInterpolatedTexCoords(tri.texB, tri.texC, ratio2x, ratio2y);
      clipped_tris[*index] = (triangle){intersect1, points[(firstOut+1)%3], points[(firstOut+2)%3], tri.color, tex1, tri.texA, tri.texB};
      clipped_tris[*nTris] = (triangle){intersect1, points[(firstOut+2)%3], intersect2, tri.color, tex1, tri.texB, tex2};
      (*nTris)++;
    }

    
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

    double ratio1x = points[firstIn].x == points[(firstIn+1)%3].x ? 1.0 : (intersect1.x-points[(firstIn+1)%3].x)/(points[firstIn].x - points[(firstIn+1)%3].x);
    double ratio2x = points[firstIn].x == points[(firstIn+2)%3].x ? 1.0 : (intersect2.x-points[(firstIn+2)%3].x)/(points[firstIn].x - points[(firstIn+2)%3].x);
    double ratio1y = points[firstIn].y == points[(firstIn+1)%3].y ? 1.0 : (intersect1.y-points[(firstIn+1)%3].y)/(points[firstIn].y - points[(firstIn+1)%3].y);
    double ratio2y = points[firstIn].y == points[(firstIn+2)%3].y ? 1.0 : (intersect2.y-points[(firstIn+2)%3].y)/(points[firstIn].y - points[(firstIn+2)%3].y);

    point texA, texB, texC;

    if(firstIn == 0){
      texA = tri.texA;
      texB = calcInterpolatedTexCoords(tri.texA, tri.texB, ratio1x, ratio1y);
      texC = calcInterpolatedTexCoords(tri.texA, tri.texC, ratio2x, ratio2y);
      clipped_tris[*index] = (triangle){points[firstIn], intersect1, intersect2, tri.color, texA, texB, texC};
    }
    else if(firstIn == 1){
      texA = calcInterpolatedTexCoords(tri.texB, tri.texA, ratio2x, ratio2y);
      texB = tri.texB;
      texC = calcInterpolatedTexCoords(tri.texB, tri.texC, ratio1x, ratio1y);
      clipped_tris[*index] = (triangle){intersect2, points[firstIn], intersect1, tri.color, texA, texB, texC};
    }
    else{
      texA = calcInterpolatedTexCoords(tri.texC, tri.texA, ratio1x, ratio1y);
      texB = calcInterpolatedTexCoords(tri.texC, tri.texB, ratio2x, ratio2y);
      texC = tri.texC;
      clipped_tris[*index] = (triangle){intersect1, intersect2, points[firstIn], tri.color, texA, texB, texC};
    }
    
  } else if(nOutside == 3){
    //Don't render this triangle at all
    for(int i = *index; i < *nTris-1; i++){
      clipped_tris[i] = clipped_tris[i+1];
    }
    (*index)--;  
    (*nTris)--;
  }
}

void clipTriangle(triangle* clipped_tris, unsigned int* nTris){
  int i = 0;
  //left
  clipEdge((point){0,0,0}, (point){0,HEIGHT,0}, clipped_tris, nTris, &i, 'x');
  //top
  for(i = 0; i < *nTris; i++){
    clipEdge((point){WIDTH,0,0}, (point){0,0,0}, clipped_tris, nTris, &i, 'y');
  }
  //right
  for(i = 0; i < *nTris; i++){
    clipEdge((point){WIDTH,HEIGHT,0}, (point){WIDTH,0,0}, clipped_tris, nTris, &i, 'x');
  }
  //bottom
  for(i = 0; i < *nTris; i++){
    clipEdge((point){0,HEIGHT,0}, (point){WIDTH,HEIGHT,0}, clipped_tris, nTris, &i, 'y');
  }
}

point calcInterpolatedTexCoords(point in, point out, double ratiox, double ratioy){
  double newU = out.x + (in.x - out.x)*ratiox;
  double newV = out.y + (in.y - out.y)*ratioy;
  if(isnan(newU))
    printf("U IS NAN!\n");
  if(isnan(newV))
    printf("V IS NAN!\n");
  return (point){newU, newV, out.z};
}

