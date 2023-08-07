#include "engine.h"
#include <float.h>
#include <stdio.h>

#define MAXOBJ 100
#define MAXLIGHT 100

typedef enum game_state
{
  INIT,
  BASE
} game_state;

object *objects;
light *lights;
game_state current_state;
SDL_Surface *surf;

const Uint8 *keystates;


int running;
unsigned int nObj = 0;
unsigned int nLights = 0;
unsigned long totalTris;
double game_time = 0.0;
double max_vel = 5;
double x_vel, y_vel, z_vel;
bool wireframe = false;
bool debug = false;

double depth_buffer[(int)WIDTH][(int)HEIGHT];


void movePlayer(double distX, double distY, double distZ)
{
  movCamera(distX, distY, distZ);
}

static void update_player_movement()
{
  movePlayer(x_vel * elapsed_time * TIME_CONST, y_vel * elapsed_time * TIME_CONST, z_vel * elapsed_time * TIME_CONST);

  x_vel = 0;
  y_vel = 0;
  z_vel = 0;
}

void yawPlayer(double rad)
{
  yawCamera(rad);
}

void pitchPlayer(double rad)
{
  if (camera_angle_x - rad > -M_PI / 2 && camera_angle_x - rad < M_PI / 2)
  {
    pitchCamera(rad);
  }
}

void free_objects()
{
  for (int j = 0; j < nObj; j++){
    free(objects[j].tris);
  }
    
  free(objects);
  free(lights);

  nObj = 0;
  nLights = 0;
  totalTris = 0;
}

void load_objects()
{
  objects = malloc(MAXOBJ * sizeof(object));
  // object teapot = loadOBJ("OBJ/teapot.obj", 0xDF2332, 0, 0, 30, 10, NULL);
  object cube = loadOBJ("OBJ/cube_normals.obj", 0xDF3F32, 0, 0, 400, 30, "textures/test.png");
  object sphere = loadOBJ("OBJ/sphere.obj", 0xDF3F32, 400, -200, 500, 300, NULL);
  object monkey = loadOBJ("OBJ/monkey.obj", 0x2323DF, 0, -30, 30, 200, NULL);
  object quad = loadOBJ("OBJ/quad.obj", 0x23D33F, 0, 0, 40, 100, "textures/test3.jpg");
  // object dog = loadOBJ("OBJ/dog.obj", 0x23D33F, 0, 0, 40, 10, NULL);
  // object get = loadOBJ("OBJ/get.obj", 0x23D33F, 0, 0, 80, 10, NULL);
  object room = loadOBJ("OBJ/room.obj", 0x32F48D, 0, 200, 200, 600, "textures/test2.jpg");
  // object rifle = loadOBJ("OBJ/rifle.obj", 0x636393, (WIDTH)*0.004, (HEIGHT)*0.015, -7, 10, NULL);
  // object quad = loadOBJ("OBJ/texTest.obj", 0xFF0000, 0, 0, 40, 100, NULL);
  object tri = loadOBJ("OBJ/tri.obj", 0xFF0000, 0, 0, 120, 100, NULL);

  objects[nObj++] = room;
  objects[nObj++] = quad;
  // objects[nObj++] = tri;
  // objects[nObj++] = cube;
  objects[nObj++] = sphere;
  // objects[nObj++] = monkey;
  // objects[nObj++] = dog;
  // objects[nObj++] = get;
  // objects[nObj++] = teapot;
  // objects[nObj-1] = rotateObjectX(objects[nObj-1], 3.14/2, objects[nObj-1].pos.x, objects[nObj-1].pos.y, objects[nObj-1].pos.z);

  totalTris = 0;
  for (int i = 0; i < nObj; i++)
    totalTris += objects[i].nFaces;
}

void load_lights()
{
  lights = malloc(sizeof(light) * MAXLIGHT);
  // lights[nLights++] = (light){(point){0, 0, 0,}, 0};  //MUZZLE FLASH
  // lights[nLights++] = (light){(point){100.0, -100.0, -100.0}, 100};
  lights[nLights++] = (light){(point){0.0, 0.0, -1000.0}, 1500};
  // lights[nLights++] = (light){(point){-500.0, 10.0, 500.0}, 300.0};
  // lights[nLights++] = (light){(point){500.0, 10.0, 500.0}, 300.0};
  // lights[nLights++] = (light){(point){500.0, 10.0, -500.0}, 300.0};
  // lights[nLights++] = (light){(point){-500.0, 10.0, -500.0}, 300.0};

  // lights[nLights++] = (light){(point){0.0, -1000.0, 0.0}, 1500};
}

void readTris(node node)
{
  //Read new triangles of a node to update number of totalTris
  totalTris += node.obj->nFaces;
  for (int i = 0; i < node.nChildren; i++)
    readTris(node.children[i]);
}


void update_time()
{
  clock_gettime(CLOCK_REALTIME, &t1);
  elapsed_time = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1000000000.0;
  game_time += elapsed_time;
  printf("fps: %5u\n", (int)(1 / elapsed_time));
  clock_gettime(CLOCK_REALTIME, &t0);
}

void handle_input()
{
  keystates = SDL_GetKeyboardState(NULL);

  while (SDL_PollEvent(&evt))
  {
    // Individual keypresses
    if (evt.type == SDL_KEYDOWN)
    {
      int keypressed = evt.key.keysym.sym;
      if (keypressed == SDLK_ESCAPE)
      {
        running = false;
      }
      else if (keypressed == SDLK_l)
      { // l
        wireframe = !wireframe;
      }
      else if (keypressed == SDLK_i)
      { // i
        debug = !debug;
      }
    }

    // Mouse movement
    else if (evt.type == SDL_MOUSEMOTION)
    {
      if (evt.motion.x != WIDTH * resScale / 2 && evt.motion.y != HEIGHT * resScale / 2)
      {
        int dx = evt.motion.xrel;
        int dy = evt.motion.yrel;
        pitchPlayer((double)dy / 300);
        yawPlayer((double)dx / 300);
      }
    }

    // Click
    else if (evt.type == SDL_MOUSEBUTTONDOWN)
    {
      int x = evt.motion.x;
      int y = evt.motion.y;
      if (evt.button.button == SDL_BUTTON_LEFT)
      {

      }
    }
  }

  // Multiple keypresses
  if (keystates[SDL_SCANCODE_W])
  { // w
    z_vel = max_vel;
  }
  if (keystates[SDL_SCANCODE_A])
  { // a
    x_vel = -max_vel;
  }
  if (keystates[SDL_SCANCODE_S])
  { // s
    z_vel = -max_vel;
  }
  if (keystates[SDL_SCANCODE_D])
  { // d
    x_vel = max_vel;
  }
  if (keystates[SDL_SCANCODE_R])
  { // r
    y_vel = -max_vel;
  }
  if (keystates[SDL_SCANCODE_F])
  { // f
    y_vel = max_vel;
  }
  if (keystates[SDL_SCANCODE_Q])
  { // q
    yawPlayer(-0.01 * elapsed_time * TIME_CONST);
  }
  if (keystates[SDL_SCANCODE_E])
  { // e
    yawPlayer(0.01 * elapsed_time * TIME_CONST);
  }
  if (keystates[SDL_SCANCODE_G])
  { // g
    pitchPlayer(0.01 * elapsed_time * TIME_CONST);
  }
  if (keystates[SDL_SCANCODE_T])
  { // t
    pitchPlayer(-0.01 * elapsed_time * TIME_CONST);
  }
  if (keystates[SDL_SCANCODE_J])
  { // j
    camera_dist += 2 * elapsed_time * TIME_CONST;
  }
  if (keystates[SDL_SCANCODE_K])
  { // k
    camera_dist -= 2 * elapsed_time * TIME_CONST;
  }
  if (keystates[SDL_SCANCODE_O])
  { // o
    for (int j = 1; j < nObj; j++)
    {
      objects[j] = rotateObjectX(objects[j], 0.007 * elapsed_time * TIME_CONST, 0, 0, 40);
      objects[j] = rotateObjectY(objects[j], 0.013 * elapsed_time * TIME_CONST, 0, 0, 40);
      objects[j] = rotateObjectZ(objects[j], 0.003 * elapsed_time * TIME_CONST, 0, 0, 40);
    }
  }
  if (keystates[SDL_SCANCODE_U])
  { // u
    lights[0].p.z += max_vel * elapsed_time * TIME_CONST;
  }
  if (keystates[SDL_SCANCODE_Y])
  { // y
    lights[0].p.z -= max_vel * elapsed_time * TIME_CONST;
  }
  if (keystates[SDL_SCANCODE_LSHIFT])
  {
    x_vel *= 4;
    y_vel *= 4;
    z_vel *= 4;
  }
  if (keystates[SDL_SCANCODE_M])
  { // m
    printf("Camera direction : (%f, %f, %f)\n", camera_dir.x, camera_dir.y, camera_dir.z);
  }
}

void update_game_logic()
{
  update_player_movement();
}

void render_scene()
{
  for(int x = 0; x < WIDTH; x++){
    for(int y = 0; y < HEIGHT; y++)
      depth_buffer[x][y] = DBL_MAX;
  }
  SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
  SDL_Rect clear = {0, 0, WIDTH, HEIGHT};
  SDL_FillRect(surf, &clear, 0);

  for (int i = 0; i < nObj; i++){
    object obj = objects[i];
    for(int t = 0; t < obj.nFaces; t++){
      triangle tri = obj.tris[t];
      triangle cam_tri = toCameraBasisTriangle(tri);

      // CLIPPING AGAINST CAMERA Z-PLANE
      triangle *clipped_tris_z = malloc(2 * sizeof(triangle));
      clipped_tris_z[0] = cam_tri;
      unsigned int nTrisZ = 1;
      int index = 0;
      const int NEAR_PLANE_Z = 10;
      clipEdge((point){0, 0, NEAR_PLANE_Z}, (point){WIDTH, HEIGHT, NEAR_PLANE_Z}, clipped_tris_z, &nTrisZ, &index, 'z');

      for (int j = 0; j < nTrisZ; j++)
      {
        triangle projected_tri = projectTriangle(clipped_tris_z[j]);
        point projected_normal = calcNormal(projected_tri);
        projected_normal = normalizeVector(projected_normal);

        // Check normal (backface culling)
        if (projected_normal.z > 0)
        {
          // CLIPPING AGAINST SCREEN BORDERS
          unsigned int nTris = 1;
          triangle *clipped_tris = malloc(16 * sizeof(triangle));
          clipped_tris[0] = projected_tri;
          clipTriangle(clipped_tris, &nTris); 
            
          // RENDERING
          for (int c = 0; c < nTris; c++)
          {
            if (!wireframe)
            {

              rasterizeTriangle(renderer, clipped_tris[c], surf, obj);
              
            }
            else
            {
              SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
              triangle tri_draw = clipped_tris[c];
              drawTriangle(renderer, tri_draw);
              SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
              point norm1 = addPoints(tri_draw.wsa, scaleVector((normalizeVector(tri_draw.normA)), 50.0));
              point norm2 = addPoints(tri_draw.wsb, scaleVector((normalizeVector(tri_draw.normB)), 50.0));
              point norm3 = addPoints(tri_draw.wsc, scaleVector((normalizeVector(tri_draw.normC)), 50.0));
              norm1 = toCameraBasis(norm1);
              norm2 = toCameraBasis(norm2);
              norm3 = toCameraBasis(norm3);
              norm1 = projectPoint(norm1);
              norm2 = projectPoint(norm2);
              norm3 = projectPoint(norm3);
              point p = (point){20.0, 20.0, 0.0};
              SDL_RenderDrawLine(renderer, (int)norm1.x, (int)norm1.y, (int)tri_draw.a.x, (int)tri_draw.a.y);
              SDL_RenderDrawLine(renderer, (int)norm2.x, (int)norm2.y, (int)tri_draw.b.x, (int)tri_draw.b.y);
              SDL_RenderDrawLine(renderer, (int)norm3.x, (int)norm3.y, (int)tri_draw.c.x, (int)tri_draw.c.y);
            }
          }
          free(clipped_tris);
        }
      }
      free(clipped_tris_z);
    }
    
  }
  if (!wireframe)
  {
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_RenderCopy(renderer, tex, NULL, NULL);
    SDL_DestroyTexture(tex);
  }
}

void render_ui()
{
  drawText(renderer, "GGE v0.0.1", WIDTH - 65, HEIGHT - 20, 60, 16, 0xFFFFFF, 12);
}

void clear_screen()
{
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);
}

int main(int argc, char *argv[])
{
  if (argc == 2)
  {
    if (strcmp(argv[1], "-f") == 0)
      // Start in fullscreen mode
      initialize_engine(true);
  }
  else
  {
    // Start in windowed mode
    initialize_engine(false);
  }
  running = 1;
  current_state = INIT;
  surf = SDL_CreateRGBSurface(0, WIDTH, HEIGHT, 32, 0, 0, 0, 0);
  // Mix_PlayMusic(music, -1);
  clock_gettime(CLOCK_REALTIME, &t0);
  SDL_SetWindowMouseGrab(screen, SDL_TRUE);
  SDL_SetRelativeMouseMode(SDL_TRUE);

  while (running)
  {
    switch (current_state)
    {
    case INIT:
      load_objects();
      load_lights();
      current_state = BASE;
      break;

    case BASE:
      update_time();
      handle_input();
      update_game_logic();
      clear_screen();
      render_scene();
      render_ui();
      SDL_RenderPresent(renderer);
      break;
    }
  }

  terminate_engine();
  return 0;
}