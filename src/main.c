#include "engine.h"

object* objects;
unsigned int nObj = 0;

light* lights;
unsigned int nLights = 0;

unsigned long totalTris;

tri_map* allTris;

const Uint8* keystates;


double speed_fast = 5.0;
double speed_slow = 2.0;
double speed = 2.0;

bool wireframe = false;

bool debug = false;

node player;

int running;

typedef enum game_state {
  MENU, NEW_GAME, GAME_RUNNING, GAME_OVER
} game_state;

game_state current_state;

void movePlayer(double distX, double distY, double distZ){
  movCamera(distX, distY, distZ);
  player = translateNode(player, sin(camera_angle_y)*distZ + sin(camera_angle_y + M_PI/2)*distX, distY, cos(camera_angle_y)*distZ + cos(camera_angle_y + M_PI/2)*distX);
}

void yawPlayer(double rad){
  yawCamera(rad);
  player = rotateNodeY(player, rad, player.pos.x, player.pos.y, player.pos.z);
}

void pitchPlayer(double rad){
  pitchCamera(rad);
  player = rotateNodeX(player, -cos(camera_angle_y)*rad, player.pos.x, player.pos.y, player.pos.z);
  player = rotateNodeZ(player, sin(camera_angle_y)*rad, player.pos.x, player.pos.y, player.pos.z);
}

void handle_input_menu(){
keystates = SDL_GetKeyboardState(NULL);
        
  while(SDL_PollEvent(&evt)){
    if(evt.type == SDL_KEYDOWN){
      int keypressed = evt.key.keysym.sym;
      if (keypressed == SDLK_ESCAPE){
          running = 0;
      } else if(keypressed == SDLK_s){//s
          current_state = NEW_GAME;
      }
    }else if(evt.type == SDL_MOUSEMOTION){
      
    }
  }
}

void handle_logic_menu(){

}

void render_menu(){
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);

  drawText(renderer, "DASK", WIDTH/2-90, HEIGHT/2-27, 180, 54, 0xFFFFFF, 32);

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
  object get = loadOBJ("OBJ/get.obj", 0x23D33F, 0, 0, 40, 10);
  object room = loadOBJ("OBJ/room3.obj", 0xE3737F, 0, 10, 0, 100);
  object sphere = loadOBJ("OBJ/sphere.obj", 0xD3b3bF, 0, 10, 0, 10);
  object rifle = loadOBJ("OBJ/rifle.obj", 0x636393, (WIDTH)*0.004, (HEIGHT)*0.01, 0, 10);
  
  
  objects[nObj++] = room;
  //objects[nObj++] = cube;
  objects[nObj++] = rifle;
  //objects[nObj++] = monkey;
  //objects[nObj++] = tri;
  //objects[nObj++] = dog;
  objects[nObj++] = get;
  //objects[nObj++] = teapot;
  //objects[nObj++] = sphere;

  
  /*node child;
  child.nChildren = 0;
  child.children = NULL;
  child.obj = &objects[nObj-2];
  child.pos = cube.pos;
  node* children = malloc(1*sizeof(node));
  children[0] = child;*/
  player = (node){&objects[nObj-2], camera_pos, NULL, 0};


  totalTris = 0;
  for(int i = 0; i < nObj; i++)
    totalTris += objects[i].nFaces;

  allTris = malloc(totalTris*sizeof(tri_map));
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
  printf("All objects loaded. Total triangles: %lu \n", totalTris);
}

void load_lights(){
  lights = malloc(sizeof(light)*MAXLIGHT);
  lights[nLights++] = (light){(point){20.0, 20.0, -70.0}, 100.0};
  lights[nLights++] = (light){(point){-500.0, 10.0, 500.0}, 300.0};
  //lights[nLights++] = (light){(point){0.0, -10000.0, 0.0}, 20000};
}

void update_time(){
  clock_gettime(CLOCK_REALTIME, &t1);
  elapsed_time = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec)/1000000000.0;
  clock_gettime(CLOCK_REALTIME, &t0);
}

void handle_input(){
  keystates = SDL_GetKeyboardState(NULL);
        
  while(SDL_PollEvent(&evt)){
    if(evt.type == SDL_KEYDOWN){
      int keypressed = evt.key.keysym.sym;
      if (keypressed == SDLK_ESCAPE){
          current_state = GAME_OVER;
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
    }
  }

  if(keystates[SDL_SCANCODE_W]){ //w
      movePlayer(0.0, 0.0, speed*elapsed_time*TIME_CONST);
  }if(keystates[SDL_SCANCODE_A]){//a
      movePlayer(-speed*elapsed_time*TIME_CONST, 0.0, 0.0);
  }if(keystates[SDL_SCANCODE_S]){//s
      movePlayer(0.0, 0.0, -speed*elapsed_time*TIME_CONST);
  }if(keystates[SDL_SCANCODE_D]){//d
      movePlayer(speed*elapsed_time*TIME_CONST, 0.0, 0.0);
  }if(keystates[SDL_SCANCODE_R]){//r
      movePlayer(0.0, -speed*elapsed_time*TIME_CONST, 0.0);
  }if(keystates[SDL_SCANCODE_F]){//f
      movePlayer(0.0, speed*elapsed_time*TIME_CONST, 0.0);
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
    for(int j = 1; j < nObj; j++){
      objects[j] = rotateObjectX(objects[j], 0.007*elapsed_time*TIME_CONST, 0, 0, 300);
      objects[j] = rotateObjectY(objects[j], 0.013*elapsed_time*TIME_CONST, 0, 0, 300);
      objects[j] = rotateObjectZ(objects[j], 0.003*elapsed_time*TIME_CONST, 0, 0, 300);
    }
  }if(keystates[SDL_SCANCODE_U]){//u
    lights[0].p.z += speed_fast*elapsed_time*TIME_CONST;
  }if(keystates[SDL_SCANCODE_Y]){//y
    lights[0].p.z -= speed_fast*elapsed_time*TIME_CONST;
  }if(keystates[SDL_SCANCODE_LSHIFT]){
    speed = speed_fast;
  }else{
    speed = speed_slow;
  }
}

void update_game_logic(){

}

void render_scene(){
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
    clipEdge((point){0,0,5} , (point){WIDTH,HEIGHT,5}, &clipped_tris_z, &nTrisZ, 0, 'z');

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

  drawText(renderer, "GGE v0.0.1", WIDTH-65, HEIGHT-20, 60, 16, 0xFFFFFF, 12);

  //SWITCH BUFFERS          
  SDL_RenderPresent(renderer);
}

void free_objects(){
  for(int j = 0; j < nObj; j++)
      free(objects[j].tris);
    free(objects);
    free(lights);
    free(allTris);
}


int main(int argc, char* argv[]){
    initialize_engine();
    running = 1;
    current_state = MENU;

    while(running){
      switch(current_state){
        case MENU:
          
          handle_input_menu();
          handle_logic_menu();
          render_menu();
          break;

        case NEW_GAME:

          load_objects();
          load_lights();
           
          clock_gettime(CLOCK_REALTIME, &t0); //set time t0
          current_state = GAME_RUNNING;
          break;

        case GAME_RUNNING:

          update_time();
          //printf("fps: %5u\n", (int)(1/elapsed_time));
          //printf("fov: %5u\n", (int)calcFOV());
          handle_input();

          update_game_logic();

          render_scene();
          break;

        case GAME_OVER:
          free_objects();
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