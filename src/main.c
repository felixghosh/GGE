#include "engine.h"

#define LEVEL 0
#define GUN 1

#define GRAVITY 0.1
#define SLOWDOWN 0.5
#define BASE_MAX_SPEED 5
#define MAX_VERTICAL_SPEED 15

#define MAXOBJ 100
#define MAXLIGHT 100
#define MAXENEMY 100

#define MUZZLE 0


typedef struct enemy_t{
  node enemy;
  int hp;
  bool render;
} enemy_t;

object* objects;
unsigned int nObj = 0;

light* lights;
unsigned int nLights = 0;

enemy_t* enemies;
unsigned int nEnemies = 0;

bool muzzle_flash = false;
double flash_time = 0.2;

unsigned long totalTris;

tri_map* allTris;

const Uint8* keystates;


double speed_fast = 5.0;
double speed_slow = 2.0;
double speed = 2.0;

bool wireframe = false;

bool debug = false;

node player;
int player_hp = 100;
long player_points = 0;

double player_radius = 10.0;
double enemy_radius = 10.0;

node enemy;
int enemy_hp = 10;


int running;
double game_time = 0.0;

unsigned int menu_color = 0x999999;

double max_vel = BASE_MAX_SPEED;

double x_vel, y_vel, z_vel;

double room_x_max, room_x_min, room_z_max, room_z_min;

typedef enum game_state {
  MENU, NEW_GAME, GAME_RUNNING, GAME_OVER
} game_state;

game_state current_state;

static void calc_obj_extremes(object obj, double* x_max, double* x_min, double* y_max, double* y_min, double* z_max, double* z_min) {
  double maxx = obj.tris[0].a.x;
  double maxy = obj.tris[0].a.y;
  double maxz = obj.tris[0].a.z;

  double minx = maxx;
  double miny = maxy;
  double minz = maxz;

  for(int i = 0; i < obj.nFaces; i++) {
    if (obj.tris[i].a.x > maxx) {
      maxx = obj.tris[i].a.x;
    }
    if (obj.tris[i].b.x > maxx) {
      maxx = obj.tris[i].b.x;
    }
    if (obj.tris[i].c.x > maxx) {
      maxx = obj.tris[i].c.x;
    }
    if (obj.tris[i].a.x < minx) {
      minx = obj.tris[i].a.x;
    }
    if (obj.tris[i].b.x < minx) {
      minx = obj.tris[i].b.x;
    }
    if (obj.tris[i].c.x < minx) {
      minx = obj.tris[i].c.x;
    }

    if (obj.tris[i].a.y > maxy) {
      maxy = obj.tris[i].a.y;
    }
    if (obj.tris[i].b.x > maxy) {
      maxy = obj.tris[i].b.y;
    }
    if (obj.tris[i].c.y > maxy) {
      maxy = obj.tris[i].c.y;
    }
    if (obj.tris[i].a.y < miny) {
      miny = obj.tris[i].a.y;
    }
    if (obj.tris[i].b.y < miny) {
      miny = obj.tris[i].b.y;
    }
    if (obj.tris[i].c.x < miny) {
      miny = obj.tris[i].c.y;
    }

    if (obj.tris[i].a.z > maxz) {
      maxz = obj.tris[i].a.z;
    }
    if (obj.tris[i].b.x > maxz) {
      maxz = obj.tris[i].b.z;
    }
    if (obj.tris[i].c.x > maxz) {
      maxz = obj.tris[i].c.z;
    }
    if (obj.tris[i].a.z < minz) {
      minz = obj.tris[i].a.z;
    }
    if (obj.tris[i].b.z < minz) {
      minz = obj.tris[i].b.z;
    }
    if (obj.tris[i].c.z < minz) {
      minz = obj.tris[i].c.z;
    }
  }
  *x_max = maxx;
  *x_min = minx;
  *y_max = maxy;
  *y_min = miny;
  *z_max = maxz;
  *z_min = minz;
}

static void find_walls() {
  double x_max, x_min, y_max, y_min, z_max, z_min;

  calc_obj_extremes(objects[0], &x_max, &x_min, &y_max, &y_min, &z_max, &z_min);

  room_x_max = x_max - camera_dist/10;
  room_x_min = x_min + camera_dist/10;

  room_z_max = z_max - camera_dist/10;
  room_z_min = z_min + camera_dist/10;
}

bool detectColision(node enemy){
  return vectorLength(subtractPoints(enemy.pos, player.pos)) < player_radius + enemy_radius;
}

bool playerHits(node enemy){
  point u = subtractPoints(enemy.pos, player.pos);
  point v = (point){-camera_dir.x, camera_dir.y, -camera_dir.z};
  point rejection = subtractPoints(u, scaleVector(v, dotProduct(v, u)));
  return vectorLength(rejection) < enemy_radius*1.3;
}

void movePlayer(double distX, double distY, double distZ){
  movCamera(distX, distY, distZ);
  player = translateNode(player, sin(camera_angle_y)*distZ + sin(camera_angle_y + M_PI/2)*distX, distY, cos(camera_angle_y)*distZ + cos(camera_angle_y + M_PI/2)*distX);
}

void movePlayerWorldSpace(double distX, double distY, double distZ){
  movCameraWorldSpace(distX, distY, distZ);
  player = translateNode(player, distX, distY, distZ);
}

static void update_player_movement() {
  double x_vel_old = x_vel;
  double y_vel_old = y_vel;
  double z_vel_old = z_vel;

  if (x_vel > 0) {
    x_vel -= SLOWDOWN;
  }
  else if (x_vel < 0) {
    x_vel += SLOWDOWN;
  }
  if (camera_pos.y < 0) {
    y_vel += GRAVITY;
  }
  if (z_vel > 0) {
    z_vel -= SLOWDOWN;
  }
  else if (z_vel < 0) {
    z_vel += SLOWDOWN;
  }

  if (x_vel * x_vel_old < 0) {
    x_vel = 0;
  }
  if (y_vel * y_vel_old < 0) {
    y_vel = 0;
  }
  if (z_vel * z_vel_old < 0) {
    z_vel = 0;
  }

  x_vel = x_vel < max_vel ? x_vel : max_vel;
  x_vel = x_vel > -1*max_vel ? x_vel : -1*max_vel;

  y_vel = y_vel < MAX_VERTICAL_SPEED ? y_vel : MAX_VERTICAL_SPEED;
  y_vel = y_vel > -1*MAX_VERTICAL_SPEED ? y_vel : -1*MAX_VERTICAL_SPEED;

  z_vel = z_vel < max_vel ? z_vel : max_vel;
  z_vel = z_vel > -1*max_vel ? z_vel : -1*max_vel;

  movePlayer(x_vel*elapsed_time*TIME_CONST, y_vel*elapsed_time*TIME_CONST, z_vel*elapsed_time*TIME_CONST);

  if (player.pos.y > 0) {
    movePlayer(0, -1*player.pos.y, 0);
    y_vel = 0;
  }

  if(player.pos.x > room_x_max) {
    movePlayerWorldSpace(room_x_max - player.pos.x, 0, 0);
  } else if(player.pos.x < room_x_min) {
    movePlayerWorldSpace(room_x_min - player.pos.x, 0, 0);
  }

  if(player.pos.z > room_z_max) {
    movePlayerWorldSpace(0, 0, room_z_max - player.pos.z);
  } else if (player.pos.z < room_z_min) {
    movePlayerWorldSpace(0, 0, room_z_min - player.pos.z);
  }
}

void yawPlayer(double rad){
  yawCamera(rad);
  player = rotateNodeY(player, rad, player.pos.x, player.pos.y, player.pos.z);
}

void pitchPlayer(double rad){
  if(camera_angle_x - rad > -M_PI/2 && camera_angle_x - rad < M_PI/2){
    pitchCamera(rad);
    player = rotateNodeX(player, -cos(camera_angle_y)*rad, player.pos.x, player.pos.y, player.pos.z);
    player = rotateNodeZ(player, sin(camera_angle_y)*rad, player.pos.x, player.pos.y, player.pos.z);
  }
}

void handle_input_menu(){
keystates = SDL_GetKeyboardState(NULL);
        
  while(SDL_PollEvent(&evt)){
    if(evt.type == SDL_KEYDOWN){
      int keypressed = evt.key.keysym.sym;
      if (keypressed == SDLK_ESCAPE){
          running = 0;
      }
    }else if(evt.type == SDL_MOUSEMOTION){
      int x = evt.motion.x;
      int y = evt.motion.y;
      if(x >= WIDTH*resScale/2-55*(WIDTH*resScale/700) && x <= WIDTH*resScale/2-55*(WIDTH*resScale/700) + 120*(WIDTH*resScale/700)
         && y >= HEIGHT*resScale/2-27*(HEIGHT*resScale/700) && y <= HEIGHT*resScale/2-27*(HEIGHT*resScale/700) + 32*(HEIGHT*resScale/700)){
          menu_color = 0xFFFFFF;
      } else menu_color = 0x999999;
    }else if(evt.type == SDL_MOUSEBUTTONDOWN){
      int x = evt.motion.x;
      int y = evt.motion.y;
      if(evt.button.button == SDL_BUTTON_LEFT){
        if(x >= WIDTH*resScale/2-55*(WIDTH*resScale/700) && x <= WIDTH*resScale/2-55*(WIDTH*resScale/700) + 120*(WIDTH*resScale/700)
          && y >= HEIGHT*resScale/2-27*(HEIGHT*resScale/700) && y <= HEIGHT*resScale/2-27*(HEIGHT*resScale/700) + 32*(HEIGHT*resScale/700)){
            current_state = NEW_GAME;
        }
      }
    }
  }
}

void handle_logic_menu(){

}

void render_menu(){
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);

  drawText(renderer, "DASK", WIDTH*resScale/2-90*(WIDTH*resScale/700), HEIGHT*resScale/2-127*(HEIGHT*resScale/700), 180*(WIDTH*resScale/700), 54*(HEIGHT*resScale/700), 0xFF1111, 36);
  drawText(renderer, "START", WIDTH*resScale/2-55*(WIDTH*resScale/700), HEIGHT*resScale/2-27*(HEIGHT*resScale/700), 120*(WIDTH*resScale/700), 32*(HEIGHT*resScale/700), menu_color, 24);


  drawText(renderer, "GGE v0.0.1", WIDTH-65, HEIGHT-20, 60, 16, 0xFFFFFF, 12);
  

  //SWITCH BUFFERS          
  SDL_RenderPresent(renderer);
}

void load_objects(){
objects = malloc(MAXOBJ*sizeof(object));
  object teapot = loadOBJ("OBJ/teapot.obj", 0xDF2332, 0, 0, 30, 10);
  object cube = loadOBJ("OBJ/cube.obj", 0xDF3F32, -20, -20, 20, 10);
  object monkey = loadOBJ("OBJ/monkey.obj", 0x2323DF, 0, -30, 40, 10);
  object tri = loadOBJ("OBJ/tri.obj", 0x23D33F, 0, 0, 40, 10);
  object dog = loadOBJ("OBJ/dog.obj", 0x23D33F, 0, 0, 40, 10);
  object get = loadOBJ("OBJ/get.obj", 0x23D33F, 0, 0, 80, 10);
  object room = loadOBJ("OBJ/room2.obj", 0xE3737F, 0, 10, 0, 100);
  object rifle = loadOBJ("OBJ/rifle.obj", 0x636393, (WIDTH)*0.004, (HEIGHT)*0.015, -7, 10);
  
  
  objects[nObj++] = room;
  //objects[nObj++] = cube;
  objects[nObj++] = rifle;
  //objects[nObj++] = monkey;
  //objects[nObj++] = tri;
  //objects[nObj++] = dog;
  //objects[nObj++] = get;
  //objects[nObj++] = teapot;
  //objects[nObj-1] = rotateObjectX(objects[nObj-1], 3.14/2, objects[nObj-1].pos.x, objects[nObj-1].pos.y, objects[nObj-1].pos.z);

  player = (node){&objects[GUN], camera_pos, NULL, 0};
 

  totalTris = 0;
  for(int i = 0; i < nObj; i++)
    totalTris += objects[i].nFaces;

}

void load_lights(){
  lights = malloc(sizeof(light)*MAXLIGHT);
  lights[nLights++] = (light){(point){0, 0, 0,}, 0};  //MUZZLE FLASH
  lights[nLights++] = (light){(point){20.0, 20.0, -70.0}, 100.0};
  lights[nLights++] = (light){(point){-500.0, 10.0, 500.0}, 300.0};
  //lights[nLights++] = (light){(point){0.0, -1000.0, 0.0}, 100};
}

void readTris(node node){
  totalTris += node.obj->nFaces;
  for(int i = 0; i < node.nChildren; i++)
    readTris(node.children[i]);
}

void mapTris(node node, unsigned long int* index, bool* render){
  for(int i = 0; i < node.obj->nFaces; i++)
    allTris[(*index)++] = (tri_map){&(node.obj->tris[i]), node.obj, render};
  for(int i = 0; i < node.nChildren; i++)
    mapTris(node.children[i], index, render);
}

void load_enemies(){
  enemies = malloc(sizeof(enemy_t)*MAXENEMY);
  object* sphere1 = malloc(sizeof(object));
  object* sphere2 = malloc(sizeof(object));
  *sphere1 = loadOBJ("OBJ/sphere.obj", 0xD3b3bF, 200, 10, 0, 10);
  *sphere2 = loadOBJ("OBJ/sphere.obj", 0x444477, 200, 10, -8, 4);

  node pupil = {sphere1, sphere1->pos, NULL, 0};
  node* children = malloc(1*sizeof(node));
  children[0] = pupil;
  node enemy1 = {sphere2, sphere2->pos, children, 1};
  enemies[nEnemies++] = (enemy_t){enemy1, 10, true};

  for(int i = 0; i < nEnemies; i++)
    readTris(enemies[i].enemy);
}

void load_tri_map(){
  allTris = malloc(totalTris*sizeof(tri_map));
  bool* render = malloc(sizeof(bool));
  *render = true;
  unsigned long index = 0;
  for(int i = 0; i < nObj; i++){
    for(int j = 0; j < objects[i].nFaces; j++){
      allTris[index++] = (tri_map){&(objects[i].tris[j]), &(objects[i]), render};
    }
  }
  for(int i = 0; i < nEnemies; i++){
    mapTris(enemies[i].enemy, &index, &(enemies[i].render));
  }
  if(index != totalTris){
    printf("ERROR INDEX DOESNT MATCH TOTALTRIS!\nindex %lu, totalTris %lu\n", index, totalTris);
    exit(1);
  }
  printf("All objects loaded. Total triangles: %lu \n", totalTris);
}

void update_time(){
  clock_gettime(CLOCK_REALTIME, &t1);
  elapsed_time = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec)/1000000000.0;
  game_time += elapsed_time;
  clock_gettime(CLOCK_REALTIME, &t0);
}

void handle_input(){
  keystates = SDL_GetKeyboardState(NULL);
        
  while(SDL_PollEvent(&evt)){
    if(evt.type == SDL_KEYDOWN){
      int keypressed = evt.key.keysym.sym;
      if (keypressed == SDLK_ESCAPE){
          running = false;
      } else if(keypressed == SDLK_l){//l
          wireframe = !wireframe;
      } else if(keypressed == SDLK_i){//i
          debug = !debug;
      }
    }else if(evt.type == SDL_MOUSEMOTION){
      if(evt.motion.x != WIDTH*resScale/2 && evt.motion.y != HEIGHT*resScale/2){
        int dx = evt.motion.xrel;
        int dy = evt.motion.yrel;
        pitchPlayer((double)dy/300);
        yawPlayer((double)dx/300);
      }
    }else if(evt.type == SDL_MOUSEBUTTONDOWN){
      int x = evt.motion.x;
      int y = evt.motion.y;
      if(evt.button.button == SDL_BUTTON_LEFT){
        muzzle_flash = true;
        Mix_PlayChannel(-1, gun_sound, 0);
        for(int i = 0; i < nEnemies; i++){
          if(playerHits(enemies[i].enemy)){
            Mix_PlayChannel(-1, hit_sound, 0);
            enemies[i].hp -= 3;
            if(enemies[i].hp <= 0){
              enemies[i].render = false;
              player_points += 50;
            }
              
            printf("HIT\n");
            printf("hp:%d render:%d\n", enemies[i].hp, enemies[i].render);
          }
        }
        
      }
    }
  }

  if(keystates[SDL_SCANCODE_W]){ //w
    z_vel = max_vel;
  }if(keystates[SDL_SCANCODE_A]){//a
    x_vel -= max_vel;
  }if(keystates[SDL_SCANCODE_S]){//s
    z_vel -= max_vel;
  }if(keystates[SDL_SCANCODE_D]){//d
    x_vel += max_vel;
  }if(keystates[SDL_SCANCODE_SPACE]){
    if(camera_pos.y == 0) {
      y_vel -= 10;
    } else if (y_vel < 0) {
      y_vel -= 0.01;
  }
  }if(keystates[SDL_SCANCODE_R]){//r
      //movePlayer(0.0, -speed*elapsed_time*TIME_CONST, 0.0);
  }if(keystates[SDL_SCANCODE_F]){//f
      //movePlayer(0.0, speed*elapsed_time*TIME_CONST, 0.0);
  }if(keystates[SDL_SCANCODE_Q]){//q
      yawPlayer(-0.01*elapsed_time*TIME_CONST);
  }if(keystates[SDL_SCANCODE_E]){//e
      yawPlayer(0.01*elapsed_time*TIME_CONST);
  }if(keystates[SDL_SCANCODE_G]){//g
      pitchPlayer(0.01*elapsed_time*TIME_CONST);
  }if(keystates[SDL_SCANCODE_T]){//t
      pitchPlayer(-0.01*elapsed_time*TIME_CONST);
  }if(keystates[SDL_SCANCODE_J]){//j
      camera_dist+= 2*elapsed_time*TIME_CONST;
  }if(keystates[SDL_SCANCODE_K]){//k
      camera_dist-= 2*elapsed_time*TIME_CONST;
  }if(keystates[SDL_SCANCODE_O]){//o
    for(int j = 2; j < nObj; j++){
      objects[j] = rotateObjectX(objects[j], 0.007*elapsed_time*TIME_CONST, 0, 0, 40);
      objects[j] = rotateObjectY(objects[j], 0.013*elapsed_time*TIME_CONST, 0, 0, 40);
      objects[j] = rotateObjectZ(objects[j], 0.003*elapsed_time*TIME_CONST, 0, 0, 40);
    }
  }if(keystates[SDL_SCANCODE_U]){//u
    lights[0].p.z += speed_fast*elapsed_time*TIME_CONST;
  }if(keystates[SDL_SCANCODE_Y]){//y
    lights[0].p.z -= speed_fast*elapsed_time*TIME_CONST;
  }if(keystates[SDL_SCANCODE_LSHIFT]){
    max_vel = 2 * BASE_MAX_SPEED;
  } else {
    max_vel = BASE_MAX_SPEED;
  }if(keystates[SDL_SCANCODE_M]){//m
    printf("Camera direction : (%f, %f, %f)\n", camera_dir.x, camera_dir.y, camera_dir.z);
  }
}

void update_game_logic(){
  static double enemy_speed = 2.0;
  static double last_point_time = 0.0;

  //Update muzzle light
  lights[MUZZLE].p = (point){camera_dir.x*100+(player.obj->pos.x), (player.obj->pos.y), camera_dir.z*100+(player.obj->pos.z)};

  if(muzzle_flash){
    lights[MUZZLE].intensity = 500;
    muzzle_flash = false;
  }
  if(lights[MUZZLE].intensity > 0){
    lights[MUZZLE].intensity -= (500/flash_time)*elapsed_time;
    if(lights[MUZZLE].intensity < 0)
      lights[MUZZLE].intensity = 0.0;
  }
  
  //Update points
  if(game_time - last_point_time > 5){
    last_point_time = game_time;
    player_points += 10;
  }

  //Update enemies
  for(int i = 0; i < nEnemies; i++){
    if(!enemies[i].render)
      continue;
    node enemy = enemies[i].enemy;
    point dir = normalizeVector(subtractPoints(player.pos, enemy.pos));
    //enemy = translateNode(enemy, dir.x*enemy_speed*elapsed_time*TIME_CONST, dir.y*enemy_speed*elapsed_time*TIME_CONST, dir.z*enemy_speed*elapsed_time*TIME_CONST);
    enemy = rotateNodeX(enemy, ((rand()%100 - 50) * 0.001)*elapsed_time*TIME_CONST, enemy.pos.x, enemy.pos.y, enemy.pos.z);
    enemy = rotateNodeY(enemy, ((rand()%100 - 50) * 0.001)*elapsed_time*TIME_CONST, enemy.pos.x, enemy.pos.y, enemy.pos.z);
    enemy = rotateNodeZ(enemy, (rand()%10 * 0.01)*elapsed_time*TIME_CONST, enemy.pos.x, enemy.pos.y, enemy.pos.z);
    //enemy = translateNode(enemy, 0.5*sin(game_time + (rand()%20 * 0.01)), 0.6*cos(game_time + (rand()%20 * 0.01)), cos(game_time + (rand()%20 * 0.01)));

    

    if(detectColision(enemy))
      player_hp--;
    enemies[i].enemy = enemy;
  }

  //Update player
  if(player_hp <= 0)
    current_state = GAME_OVER;

  update_player_movement();
  
  //printf("%2.1lf, %2.1lf, %2.1lf\n", camera_dir.x, camera_dir.y, camera_dir.z);
  //printf("%2.1lf %2.1lf\n", camera_angle_x, camera_angle_y);
}

void render_scene(){
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);
  SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

  //sort triangles for painters algorithm
  qsort(allTris, totalTris, sizeof(tri_map), cmpfunc);

  for(int i = 0; i < totalTris; i++){
    if(!*allTris[i].render)
      continue;
    triangle tri = *(allTris[i].tri);
    triangle cam_tri = toCameraBasisTriangle(tri);

    //CLIPPING AGAINST CAMERA Z-PLANE
    triangle* clipped_tris_z = malloc(2*sizeof(triangle));
    clipped_tris_z[0] = cam_tri;
    unsigned int nTrisZ = 1;
    clipEdge((point){0,0,3} , (point){WIDTH,HEIGHT,3}, &clipped_tris_z, &nTrisZ, 0, 'z');

    for(int j = 0; j < nTrisZ; j++){
      triangle projected_tri = projectTriangle(clipped_tris_z[j]);
      point projected_normal = calcNormal(projected_tri);
      projected_normal = normalizeVector(projected_normal);

      //Check normal (backface culling)
      if(projected_normal.z > 0){
        
        point world_normal = normalizeVector(calcNormal(tri));
        
        double lightness = 0.0;
        double ambient = 0.1;
        for(int i = 0; i < nLights; i++){
          point light_direction = normalizeVector(subtractPoints(calcCenter(tri), lights[i].p));
          double light_dist = vectorLength(subtractPoints(calcCenter(tri), lights[i].p));
          double partial_light = (lights[i].intensity/pow(light_dist, 1.1))*dotProduct(world_normal, light_direction);
          partial_light = partial_light < 0 ? 0 : partial_light;
          lightness += partial_light;
        }
        unsigned int color = colorLightness(lightness + ambient, tri.color);
        
        
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

  drawText(renderer, "GGE v0.0.1", WIDTH-65, HEIGHT-20, 60, 16, 0xFFFFFF, 12);
  char hp[9] = {0};
  snprintf(hp, 8, "HP: %3d", player_hp);
  drawText(renderer, hp, 10, HEIGHT-20, 60, 16, 0xFFFFFF, 12);
  char points[20] = {0};
  snprintf(points, 19, "Points: %ld", player_points);
  drawText(renderer, points, 110, HEIGHT-20, 80, 16, 0xFFFFFF, 12);

  SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
  SDL_RenderDrawLine(renderer, WIDTH/2-10, HEIGHT/1.5, WIDTH/2+10, HEIGHT/1.5);
  SDL_RenderDrawLine(renderer, WIDTH/2, HEIGHT/1.5-10, WIDTH/2, HEIGHT/1.5+10);

  //SWITCH BUFFERS          
  SDL_RenderPresent(renderer);
}

void handle_input_game_over(){
  keystates = SDL_GetKeyboardState(NULL);
        
  while(SDL_PollEvent(&evt)){
    if(evt.type == SDL_KEYDOWN){
      int keypressed = evt.key.keysym.sym;
      if (keypressed == SDLK_ESCAPE){
          running = 0;
      } 
    }else if(evt.type == SDL_MOUSEMOTION){
      
    }
  }
}

void handle_logic_game_over(){

}

void render_game_over(){
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);

  drawText(renderer, "GAME OVER", WIDTH/2-90, HEIGHT/2-27, 180, 54, 0xFFFFFF, 32);

  char points[20] = {0};
  snprintf(points, 19, "Points: %ld", player_points);
  drawText(renderer, points, WIDTH/2-90, HEIGHT/2+27, 180, 54, 0xFFFFFF, 32);

  drawText(renderer, "GGE v0.0.1", WIDTH-65, HEIGHT-20, 60, 16, 0xFFFFFF, 12);

  //SWITCH BUFFERS          
  SDL_RenderPresent(renderer);
}

void free_objects(){
  for(int j = 0; j < nObj; j++)
    free(objects[j].tris);
  free(objects);
  free(lights);
  free(enemies);
  free(allTris);
}


int main(int argc, char* argv[]){
    if(argc == 2){
      if(strcmp(argv[1], "-f") == 0)
        initialize_engine(true);
    } else{
      initialize_engine(false);
    }
    running = 1;
    current_state = MENU;
    Mix_PlayMusic(music, -1);

    while(running){
      switch(current_state){
        case MENU:
          
          handle_input_menu();
          handle_logic_menu();
          render_menu();
          break;

        case NEW_GAME:

          load_objects();
          load_enemies();
          load_tri_map();
          load_lights();
          find_walls();

          SDL_SetWindowMouseGrab(screen, SDL_TRUE);
          SDL_SetRelativeMouseMode(SDL_TRUE);
           
          clock_gettime(CLOCK_REALTIME, &t0); //set time t0
          current_state = GAME_RUNNING;
          break;

        case GAME_RUNNING:

          update_time();
          //printf("fps: %5u\n", (int)(1/elapsed_time));
          //printf("fov:%u\n", (int)calcFOV());
          handle_input();

          update_game_logic();

          render_scene();
          break;

        case GAME_OVER:
          free_objects();
          SDL_SetWindowMouseGrab(screen, SDL_FALSE);
          SDL_SetRelativeMouseMode(SDL_FALSE);
          while(running){
            handle_input_game_over();
            handle_logic_game_over();
            render_game_over();
          }
          break;
      }
    }

    terminate_engine();
    return 0;
}