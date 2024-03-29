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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

double tex_width_denom;
double tex_height_denom;


triangle camera_basis = { //Currently not used
  {1.0, 0.0, 0.0},
  {0.0, 1.0, 0.0},
  {0.0, 0, 1.0},
  0
};

// unsigned int texture[3][3] = 
// {{0xff0000,0x00ff00, 0x0000ff}, 
//  {0xffff00,0x00ffff, 0xff00ff},
//  {0xff9933,0x99ff33, 0x3399ff}};

// unsigned char* texture;

// int texture_width, texture_height, nrChannels;



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
  return (point){u, v, 0.0};
}

double interpolateZ(triangle tri, point bcc){
  double z = tri.a.z * bcc.x + tri.b.z * bcc.y + tri.c.z * bcc.z;
  return z;
}

point interpolateNormal(point normals[3], point bcc){
  double x = normals[0].x*bcc.x + normals[1].x*bcc.y + normals[2].x*bcc.z;
  double y = normals[0].y*bcc.x + normals[1].y*bcc.y + normals[2].y*bcc.z;
  double z = normals[0].z*bcc.x + normals[1].z*bcc.y + normals[2].z*bcc.z;
  return (point){x, y, z};
}

point interpolateWorldspace(triangle tri, point bcc){
  double x = tri.wsa.x*bcc.x + tri.wsb.x*bcc.y + tri.wsc.x*bcc.z;
  double y = tri.wsa.y*bcc.x + tri.wsb.y*bcc.y + tri.wsc.y*bcc.z;
  double z = tri.wsa.z*bcc.x + tri.wsb.z*bcc.y + tri.wsc.z*bcc.z;
  return (point){x, y, z};
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

unsigned int sampleTexture(double u, double v, unsigned char* texture, int texture_width, int texture_height){
  u = u >= 1.0 ? 1.0 - 1.0/texture_height : u;
  v = v >= 1.0 ? 1.0 - 1.0/texture_width : v;
  u = u < 0.0 ? 0.0 : u;
  v = v < 0.0 ? 0.0 : v;
  v = 1.0 - v;

  unsigned int r, g, b, rshift, gshift, rValue, gValue, bValue;
  r = 0x00FF0000;
  g = 0x0000FF00;
  b = 0x000000FF;
  rshift = 16;
  gshift = 8;

  // rValue = (texture[texture_width*3 + (int)(v*texture_width)*3+0]<<rshift) & r;
  // gValue = (texture[texture_width*3 + (int)(v*texture_width)*3+1]<<gshift) & g; 
  // bValue = (texture[texture_width*3 + (int)(v*texture_width)*3+2]) & b;

  rValue = (texture[(int)(u*texture_height)*texture_width*3+(int)(v*texture_width)*3+0]<<rshift) & r;
  gValue = (texture[(int)(u*texture_height)*texture_width*3+(int)(v*texture_width)*3+1]<<gshift) & g; 
  bValue = (texture[(int)(u*texture_height)*texture_width*3+(int)(v*texture_width)*3+2]) & b;
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

void calcEdgeFuncParams(triangle tri, double* arr){
  point p0 = tri.a;
  point p1 = tri.b;
  point p2 = tri.c;

  arr[0] = -(p2.y - p1.y);
  arr[1] = p2.x- p1.x;
  arr[2] = -(p0.y - p2.y);
  arr[3] = p0.x- p2.x;
  arr[4] = -(p1.y - p0.y);
  arr[5] = p1.x- p0.x;
}

void rasterizeTriangle(SDL_Renderer* renderer, triangle tri, SDL_Surface* surf, object obj){

  double edgeFuncParams[6];

  calcEdgeFuncParams(tri, edgeFuncParams);

  point normals[3] = {normalizeVector(tri.normA), normalizeVector(tri.normB), normalizeVector(tri.normC)};
  
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
          point bcc = calcPCBCC((point){(double)k, (double)(i+p[0].y), (double)0.0}, tri, edgeFuncParams);
          double frag_z = interpolateZ(tri, bcc);
          if(depth_buffer[k][(i+(int)p[0].y)] > frag_z){
            depth_buffer[k][(i+(int)p[0].y)] = frag_z;
            point texCoords = interpolateTexCoords(tri, bcc);
            unsigned int chosen_color;
            if(obj.texture != NULL){
              unsigned int texture_color = sampleTexture(texCoords.x, texCoords.y, obj.texture, obj.texture_width, obj.texture_height);
              chosen_color = texture_color;
            } else{
              chosen_color = tri.color;
            }
            
            point N = interpolateNormal(normals, bcc);
            point world_space_coord = interpolateWorldspace(tri, bcc);

            double lightness = 0.0;
            double ambient = 0.3;             
            for (int l = 0; l < nLights; l++)
            {
              point light_direction = normalizeVector(subtractPoints(lights[l].p, world_space_coord));
              double light_dist = vectorLength(subtractPoints(world_space_coord, lights[l].p));
              double partial_light = (lights[l].intensity / pow(light_dist, 1.1)) * dotProduct(N, light_direction);
              partial_light = partial_light < 0 ? 0 : partial_light;
              lightness += partial_light;
            }
            unsigned int color = colorLightness(lightness + ambient, chosen_color);
            set_pixel(surf, k, (i+(int)p[0].y), color);
          }
        }
      } else{
        for(int k  = slope_long[i]; k <= slope_short[i]; k++){
          point bcc = calcPCBCC((point){(double)k, (double)(i+p[0].y), (double)0.0}, tri, edgeFuncParams);
          double frag_z = interpolateZ(tri, bcc);
          if(depth_buffer[k][(i+(int)p[0].y)] > frag_z){
            depth_buffer[k][(i+(int)p[0].y)] = frag_z;
            point texCoords = interpolateTexCoords(tri, bcc);
            unsigned int chosen_color;
            if(obj.texture != NULL){
              unsigned int texture_color = sampleTexture(texCoords.x, texCoords.y, obj.texture, obj.texture_width, obj.texture_height);
              chosen_color = texture_color;
            } else{
              chosen_color = tri.color;
            }

            point N = interpolateNormal(normals, bcc);
            point world_space_coord = interpolateWorldspace(tri, bcc);

            double lightness = 0.0;
            double ambient = 0.3;             
            for (int l = 0; l < nLights; l++)
            {
              point light_direction = normalizeVector(subtractPoints(lights[l].p, world_space_coord));
              double light_dist = vectorLength(subtractPoints(world_space_coord, lights[l].p));
              double partial_light = (lights[l].intensity / pow(light_dist, 1.1)) * dotProduct(N, light_direction);
              partial_light = partial_light < 0 ? 0 : partial_light;
              lightness += partial_light;
            }
            unsigned int color = colorLightness(lightness + ambient, chosen_color);
            set_pixel(surf, k, (i+(int)p[0].y), color);
          }
        }
      }
    }      
  }
  if(dy_last != 0){
    int origin = i;
    for(i; i < dy_long; i++){
      if((slope_last[i - origin]) - (slope_long[i]) < 0){
        for(int k = slope_long[i]; k > slope_last[i - origin]; k--){
          point bcc = calcPCBCC((point){(double)k, (double)(i+p[0].y), (double)0.0}, tri, edgeFuncParams);
          double frag_z = interpolateZ(tri, bcc);
          if(depth_buffer[k][(i+(int)p[0].y)] > frag_z){
            depth_buffer[k][(i+(int)p[0].y)] = frag_z;
            point texCoords = interpolateTexCoords(tri, bcc);
            unsigned int chosen_color;
            if(obj.texture != NULL){
              unsigned int texture_color = sampleTexture(texCoords.x, texCoords.y, obj.texture, obj.texture_width, obj.texture_height);
              chosen_color = texture_color;
            } else{
              chosen_color = tri.color;
            }
            
            point N = interpolateNormal(normals, bcc);
            point world_space_coord = interpolateWorldspace(tri, bcc);

            double lightness = 0.0;
            double ambient = 0.3;             
            for (int l = 0; l < nLights; l++)
            {
              point light_direction = normalizeVector(subtractPoints(lights[l].p, world_space_coord));
              double light_dist = vectorLength(subtractPoints(world_space_coord, lights[l].p));
              double partial_light = (lights[l].intensity / pow(light_dist, 1.1)) * dotProduct(N, light_direction);
              partial_light = partial_light < 0 ? 0 : partial_light;
              lightness += partial_light;
            }
            unsigned int color = colorLightness(lightness + ambient, chosen_color);
            set_pixel(surf, k, (i+(int)p[0].y), color);
          }
        }
      }else{
        for(int k = slope_long[i]; k <= slope_last[i - origin]; k++){
          point bcc = calcPCBCC((point){(double)k, (double)(i+p[0].y), (double)0.0}, tri, edgeFuncParams);
          double frag_z = interpolateZ(tri, bcc);
          if(depth_buffer[k][(i+(int)p[0].y)] > frag_z){  
            depth_buffer[k][(i+(int)p[0].y)] = frag_z;
            point texCoords = interpolateTexCoords(tri, bcc);
            unsigned int chosen_color;
            if(obj.texture != NULL){
              unsigned int texture_color = sampleTexture(texCoords.x, texCoords.y, obj.texture, obj.texture_width, obj.texture_height);
              chosen_color = texture_color;
            } else{
              chosen_color = tri.color;
            }
            
            point N = interpolateNormal(normals, bcc);
            point world_space_coord = interpolateWorldspace(tri, bcc);

            double lightness = 0.0;
            double ambient = 0.3;             
            for (int l = 0; l < nLights; l++)
            {
              point light_direction = normalizeVector(subtractPoints(lights[l].p, world_space_coord));
              double light_dist = vectorLength(subtractPoints(world_space_coord, lights[l].p));
              double partial_light = (lights[l].intensity / pow(light_dist, 1.1)) * dotProduct(N, light_direction);
              partial_light = partial_light < 0 ? 0 : partial_light;
              lightness += partial_light;
            }
            unsigned int color = colorLightness(lightness + ambient, chosen_color);
            set_pixel(surf, k, (i+(int)p[0].y), color);
          }
        }
      }
    }
  }
}

object loadOBJ(const char* filePath, unsigned int color, double x, double y, double z, double scale, const char* texture_path){
  unsigned long int nVertices = 0;
  unsigned long int nCoords = 0;
  unsigned long int nNormals = 0;
  unsigned long int nFaces = 0;

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

  getline(&buf, &buf_size, fp);

  do{
    getline(&buf, &buf_size, fp);
    nNormals++;
  } while(buf[0] == 'v');

  while(getline(&buf, &buf_size, fp)>0)
    nFaces++;
  nVertices--;

  rewind(fp);

  point* vertices = malloc(nVertices * sizeof(point));
  for(int i = 0; i < nVertices; i++){
    getline(&buf, &buf_size, fp);
    endptr = buf;
    double values[3];
    for(int i = 0; i < 3; i++){
      values[i] = scale*strtod(endptr+1, &endptr);
    }
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
    texCoords[i] = (point){values[0], values[1], 0.0};
  }

  getline(&buf, &buf_size, fp);

  point* normals = malloc(nNormals * sizeof(point));

  for(int i = 0; i < nNormals; i++){
    getline(&buf, &buf_size, fp);
    endptr = buf;
    double values[3];
    values[0] = strtod(endptr+2, &endptr)*-1.0;
    values[1] = strtod(endptr+1, &endptr)*-1.0;
    values[2] = strtod(endptr+1, &endptr)*-1.0;
    
    normals[i] = normalizeVector((point){values[0], values[1], values[2]});
  }

  getline(&buf, &buf_size, fp);

  triangle* tris = malloc(nFaces * sizeof(triangle));
  for(int i = 0; i < nFaces; i++){
    getline(&buf, &buf_size, fp);
    endptr = buf;
    long int values[3];
    long int tex[3];
    long int norm[3];
    for(int i = 0; i < 3; i++){
      values[i] = strtol(endptr+1, &endptr, 10) - 1;
      tex[i] = strtol(endptr+1, &endptr, 10) - 1;
      norm[i] = strtol(endptr+1, &endptr, 10) - 1;
    }
      
    tris[i] = (triangle){
      vertices[values[0]],
      vertices[values[1]],
      vertices[values[2]],
      color,
      texCoords[tex[0]],
      texCoords[tex[1]],
      texCoords[tex[2]],
      normals[norm[0]],
      normals[norm[1]],
      normals[norm[2]],
      vertices[values[0]],
      vertices[values[1]],
      vertices[values[2]]
    };
    triangle tri = tris[i];
  }
  int texture_width, texture_height, nrChannels;
  unsigned char* texture = NULL;
  if(texture_path != NULL){
    printf("Loading object with texture at %s\n", texture_path);
    texture = stbi_load(texture_path, &texture_width, &texture_height, &nrChannels, STBI_rgb);
  }
  free(buf);
  object obj = {tris, nFaces, (point){x,y,z}, texture, texture_width, texture_height, nrChannels};
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

point calcIntersect(point p0, point p1, char axis, double value){
  point intersect;
  if(axis == 'x'){
    intersect.x = value;
    intersect.y = ((p0.x - value)/(p0.x - p1.x))*(p1.y - p0.y) + p0.y;
  } else if(axis == 'y'){
    intersect.x = ((p0.y - value)/(p0.y - p1.y))*(p1.x - p0.x) + p0.x;
    intersect.y = value;
  } else if(axis == 'z'){
    intersect.x = ((p0.z - value)/(p0.z - p1.z))*(p1.x - p0.x) + p0.x;
    intersect.y = ((p0.z - value)/(p0.z - p1.z))*(p1.y - p0.y) + p0.y;
    intersect.z = value;
  }
  return intersect;
}

void clipEdge(point p1, point p2, triangle* clipped_tris, unsigned int* nTris, int* index, char axis){
  triangle tri = clipped_tris[*index];
  point points[3] = {tri.a, tri.b, tri.c};
  point normals[3] = {tri.normA, tri.normB, tri.normC};
  double value;

  double edgeFuncParams[6];
  calcEdgeFuncParams(tri, edgeFuncParams);

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
      nOutside++;
      inside[0] = 0;
    }
    if((p2.z - p1.z)*(tri.b.y - p1.y)-(p2.y - p1.y)*(tri.b.z - p1.z) > 0){
      nOutside++;
      inside[1] = 0;
    }
    if((p2.z - p1.z)*(tri.c.y - p1.y)-(p2.y - p1.y)*(tri.c.z - p1.z) > 0){
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

    point bcc1, bcc2;

  
    if(axis != 'z'){
      bcc1 = calcPCBCC(intersect1, tri, edgeFuncParams);
      bcc2 = calcPCBCC(intersect2, tri, edgeFuncParams);
      intersect1.z = interpolateZ(tri, bcc1);
      intersect2.z = interpolateZ(tri, bcc2);
    } else{
      bcc1 = calcBCC(intersect1, tri);
      bcc2 = calcBCC(intersect2, tri);
    }

    point tex1, tex2;
    tex1 = interpolateTexCoords(tri, bcc1);
    tex2 = interpolateTexCoords(tri, bcc2);
    point norm1, norm2;
    norm1 = interpolateNormal(normals, bcc1);
    norm2 = interpolateNormal(normals, bcc2);
    point ws1, ws2;
    ws1 = interpolateWorldspace(tri, bcc1);
    ws2 = interpolateWorldspace(tri, bcc2);

    if(firstOut == 0){
      clipped_tris[*index] = (triangle){intersect1, points[(firstOut+1)%3], points[(firstOut+2)%3], tri.color, tex1, tri.texB, tri.texC, norm1, tri.normB, tri.normC, ws1, tri.wsb, tri.wsc};
      clipped_tris[(*nTris)++] = (triangle){intersect1, points[(firstOut+2)%3], intersect2, tri.color, tex1, tri.texC, tex2, norm1, tri.normC, norm2, ws1, tri.wsc, ws2};
    }
    else if(firstOut == 1){
      clipped_tris[*index] = (triangle){intersect1, points[(firstOut+1)%3], points[(firstOut+2)%3], tri.color, tex1, tri.texC, tri.texA, norm1, tri.normC, tri.normA, ws1, tri.wsc, tri.wsa};
      clipped_tris[(*nTris)++] = (triangle){intersect1, points[(firstOut+2)%3], intersect2, tri.color, tex1, tri.texA, tex2, norm1, tri.normA, norm2, ws1, tri.wsa, ws2};
    }
    else{
      clipped_tris[*index] = (triangle){intersect1, points[(firstOut+1)%3], points[(firstOut+2)%3], tri.color, tex1, tri.texA, tri.texB, norm1, tri.normA, tri.normB, ws1, tri.wsa, tri.wsb};
      clipped_tris[(*nTris)++] = (triangle){intersect1, points[(firstOut+2)%3], intersect2, tri.color, tex1, tri.texB, tex2, norm1, tri.normB, norm2, ws1, tri.wsb, ws2};
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

    point bcc1, bcc2;

  
    if(axis != 'z'){
      bcc1 = calcPCBCC(intersect1, tri, edgeFuncParams);
      bcc2 = calcPCBCC(intersect2, tri, edgeFuncParams);
      intersect1.z = tri.a.z * bcc1.x + tri.b.z * bcc1.y + tri.c.z * bcc1.z;
      intersect2.z = tri.a.z * bcc2.x + tri.b.z * bcc2.y + tri.c.z * bcc2.z;
    } else{
      bcc1 = calcBCC(intersect1, tri);
      bcc2 = calcBCC(intersect2, tri);
    }
    

    point tex1, tex2;
    tex1 = interpolateTexCoords(tri, bcc1);
    tex2 = interpolateTexCoords(tri, bcc2);

    point norm1, norm2;
    norm1 = interpolateNormal(normals, bcc1);
    norm2 = interpolateNormal(normals, bcc2);
    point ws1, ws2;
    ws1 = interpolateWorldspace(tri, bcc1);
    ws2 = interpolateWorldspace(tri, bcc2);

    point texA, texB, texC;

    if(firstIn == 0){
      texA = tri.texA;
      texB = tex1;
      texC = tex2;
      clipped_tris[*index] = (triangle){points[firstIn], intersect1, intersect2, tri.color, texA, texB, texC, tri.normA, norm1, norm2, tri.wsa, ws1, ws2};
    }
    else if(firstIn == 1){
      texA = tex2;
      texB = tri.texB;
      texC = tex1;
      clipped_tris[*index] = (triangle){intersect2, points[firstIn], intersect1, tri.color, texA, texB, texC, norm2, tri.normB, norm1, ws2, tri.wsb, ws1};
    }
    else{
      texA = tex1;
      texB = tex2;
      texC = tri.texC;
      clipped_tris[*index] = (triangle){intersect1, intersect2, points[firstIn], tri.color, texA, texB, texC, norm1, norm2, tri.normC, ws1, ws2, tri.wsc};
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
  clipEdge((point){0.0,0.0,0.0}, (point){0.0,HEIGHT-1.0,0.0}, clipped_tris, nTris, &i, 'x');
  //top
  for(i = 0; i < *nTris; i++){
    clipEdge((point){WIDTH-1.0,0.0,0.0}, (point){0.0,0.0,0.0}, clipped_tris, nTris, &i, 'y');
  }
  //right
  for(i = 0; i < *nTris; i++){
    clipEdge((point){WIDTH-1.0,HEIGHT-1.0,0.0}, (point){WIDTH-1.0,0.0,0.0}, clipped_tris, nTris, &i, 'x');
  }
  //bottom
  for(i = 0; i < *nTris; i++){
    clipEdge((point){0.0,HEIGHT-1.0,0.0}, (point){WIDTH-1.0,HEIGHT-1.0,0.0}, clipped_tris, nTris, &i, 'y');
  }
}

point calcInterpolatedTexCoords(point in, point out, double ratiox, double ratioy, double in_z, double out_z, double intersect_z){
  double zx_t = 1 / ((1/out_z + ratiox*(1/in_z - 1/out_z)));
  double zy_t = 1 / ((1/out_z + ratioy*(1/in_z - 1/out_z)));

  double newU = (out.x/out_z + (in.x/in_z - out.x/out_z)*ratiox)*zx_t;
  double newV = (out.y/out_z + (in.y/in_z - out.y/out_z)*ratioy)*zy_t;

  return (point){newU, newV, out.z};
}

