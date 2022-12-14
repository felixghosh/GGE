#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <xcb/xproto.h>
#include <X11/extensions/Xfixes.h>
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


double camera_dist = 554.0; //For FOV 60

struct timespec t0, t1;
double elapsed_time;

double speed = 3.0;
double resScale = 1;

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

triangle test = {
  {0, 0, 100},
  {10, 4, 100},
  {4, 11, 100},
  0x00FF00
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

void drawTriangle(Display* dsp, Window win, GC gc, triangle tri){
  point center = calcCenter(tri);
  if(center.z > 0){
    XDrawLine(dsp, win, gc, tri.a.x, tri.a.y, tri.b.x, tri.b.y);
    XDrawLine(dsp, win, gc, tri.b.x, tri.b.y, tri.c.x, tri.c.y);
    XDrawLine(dsp, win, gc, tri.c.x, tri.c.y, tri.a.x, tri.a.y);
  }
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

void rasterizeTriangle(Display* dsp, Window win, GC gc, triangle tri){
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
    //XDrawPoint(dsp, win, gc, slope_long[i], (int)i+p[0].y);
  }
  int dy_short = (p[1].y - p[0].y);
  denominator = 1 / dy_short;
  double slope_short[dy_short];
  if(dy_short != 0){
    for(i = 0; i < dy_short; i++){
      slope_short[i] = p[0].x + (p[1].x-p[0].x)*i/dy_short;
      //XDrawPoint(dsp, win, gc, slope_short[i], (int)i+p[0].y);
    }
  }
  int dy_last = (p[2].y - p[1].y);
  denominator = 1 / dy_last;
  double slope_last[dy_last];
  if(dy_last != 0){
    for(i = 0; i < dy_last; i++){
      slope_last[i] = p[1].x + (p[2].x-p[1].x)*i/dy_last;
      //XDrawPoint(dsp, win, gc, slope_last[i], i+p[1].y);
    }
  }
  //scanline
  i = 0;
  if(dy_short != 0){
    for(i; i < dy_short; i++){
      for(int j = 0; j < resScale; j++)
        XDrawLine(dsp, win, gc, slope_long[i]*resScale, (i+p[0].y)*resScale+j, (int)slope_short[i]*resScale, (i+p[0].y)*resScale+j);
    }
    
  }
  if(dy_last != 0){
    int origin = i;
    for(i; i < dy_long; i++){
      for(int j = 0; j < resScale; j++)
        XDrawLine(dsp, win, gc, slope_long[i]*resScale, (i+p[0].y)*resScale+j, (int)slope_last[i - origin]*resScale, (i+p[0].y)*resScale+j);
    }
    
  }
  //XFlush(dsp);
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


int main(){
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

  /*FILE* fp;
  fp = fopen("/home/felixghosh/prog/c/GGE/sphere.obj", "r");
  
  size_t buf_size = 50;
  char* buf = malloc(buf_size*sizeof(char));
  char* endptr;
  unsigned long int nVertices = 0;
  unsigned long int nFaces = 0;

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
      values[i] = 10*strtod(endptr+1, &endptr);
    vertices[i] = (point){values[0], values[1], values[2]};
  }
  
  getline(&buf, &buf_size, fp);

  triangle* tris = malloc(nFaces * sizeof(triangle));
  for(int i = 0; i < nFaces; i++){
    getline(&buf, &buf_size, fp);
    endptr = buf;
    long int values[3];
    for(int i = 0; i < 3; i++)
      values[i] = strtol(endptr+1, &endptr, 10);
    tris[i] = (triangle){
      vertices[values[0]],
      vertices[values[1]],
      vertices[values[2]],
      0xFF0000
      };

  //free(buf);
  }*/
  /*for(int i = 0; i < nVertices; i++)
    printf("(%lf, %lf, %lf)\n", vertices[i].x, vertices[i].y, vertices[i].z);
  for(int i = 0; i < nFaces; i++)
    printf("tri color: %u point1.x %lf\n", tris[i].color, tris[i].a.x);*/
  


  for(int i = 0; i < 12; i++)
    tris[i] = translateTriangle(tris[i], 0, -100, 700);
  for(int i = 12; i < 24; i++)
    tris[i] = translateTriangle(tris[i], 0, 150, 700);

  Display *dsp = XOpenDisplay( NULL );
  if( !dsp ){ return 1; }

  int screenNumber = DefaultScreen(dsp);
  unsigned long white = WhitePixel(dsp,screenNumber);
  unsigned long black = BlackPixel(dsp,screenNumber);

  Window win = XCreateSimpleWindow(dsp,
                               DefaultRootWindow(dsp),
                               50, 50,   // origin
                                WIDTH*resScale, HEIGHT*resScale, // size
                               0, white, // border
                               black );  // backgd

  XMapWindow( dsp, win );

  long eventMask = StructureNotifyMask;
  XSelectInput( dsp, win, eventMask );


  XEvent evt;
  do{
    XNextEvent( dsp, &evt );   // calls XFlush
  }while( evt.type != MapNotify );

 
  GC gc = XCreateGC( dsp, win,
                     0,        // mask of values
                     NULL );   // array of values
  XSetForeground( dsp, gc, white);

  Pixmap double_buffer = XCreatePixmap(dsp, win, WIDTH*resScale, HEIGHT*resScale, 24);


  eventMask = KeyPressMask|KeyReleaseMask|PointerMotionMask;
  XSelectInput(dsp,win,eventMask); // override prev
   
  do{
    clock_gettime(CLOCK_REALTIME, &t1);
    elapsed_time = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec)/1000000000.0;
    clock_gettime(CLOCK_REALTIME, &t0);
    printf("fps: %5u\n", (int)(1/elapsed_time));
    
    XClearWindow(dsp, win);
    XSetForeground( dsp, gc, black);
    XFillRectangle(dsp, double_buffer, gc, 0, 0, WIDTH*resScale, HEIGHT*resScale);
    XSetForeground( dsp, gc, white);
    XDrawString(dsp, double_buffer, gc, WIDTH*resScale-70, HEIGHT*resScale-10, "GGE v0.0.1", 10);
    char fov[11];
    double currentFOV = calcFOV();
    if(currentFOV < 100.0){
      snprintf(fov, sizeof fov-1, "FOV: %3.1lf", currentFOV);
      XDrawString(dsp, double_buffer, gc, 10, HEIGHT*resScale-10, fov, 9);
    }
    else{
      snprintf(fov, sizeof fov, "FOV: %3.1lf", currentFOV);
      XDrawString(dsp, double_buffer, gc, 10, HEIGHT*resScale-10, fov, 10);
    }
    //tris = quicksort(tris, 24);
    qsort(tris, 24, sizeof(triangle), cmpfunc);
    //sleep(1);
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
        XSetForeground( dsp, gc, color);
        rasterizeTriangle(dsp, double_buffer, gc, projected_tri);
        
        if(wireframe){
          XSetForeground(dsp, gc, 0x00FF);
          triangle scaledTri = (triangle){
            {projected_tri.a.x*resScale, projected_tri.a.y*resScale, projected_tri.a.z},
            {projected_tri.b.x*resScale, projected_tri.b.y*resScale, projected_tri.b.z},
            {projected_tri.c.x*resScale, projected_tri.c.y*resScale, projected_tri.c.z},
            projected_tri.color
          };
          drawTriangle(dsp, double_buffer, gc, scaledTri);
        }
      }
      
      //tris[i] = rotateX(tris[i], 0.007, 0, -100, 800);
      //tris[i] = rotateY(tris[i], 0.013, 0, -100, 800);
      //tris[i] = rotateZ(tris[i], 0.003, 0, -100, 800);
    }
    //XSync(dsp, 0);
    XCopyArea(dsp, double_buffer, win, gc, 0, 0, WIDTH*resScale, HEIGHT*resScale, 0, 0);
    //usleep(100);
    XCheckWindowEvent( dsp, win, eventMask, &evt);
    //if(evt.xkey.state != 0)
    //  printf("%lu\n", evt.xkey.state);

    if(evt.type == KeyPress){  //Handle Input
      //printf("%u\n", evt.xkey.keycode);
      if(evt.xkey.keycode == 25){       //w
        movCamera(0.0, 0.0, speed);
      } else if(evt.xkey.keycode == 38){//a
        movCamera(-speed, 0.0, 0.0);
      } else if(evt.xkey.keycode == 39){//s
        movCamera(0.0, 0.0, -speed);
      } else if(evt.xkey.keycode == 40){//d
        movCamera(speed, 0.0, 0.0);
      } else if(evt.xkey.keycode == 27){//r
        movCamera(0.0, -speed, 0.0);
      } else if(evt.xkey.keycode == 41){//f
        movCamera(0.0, speed, 0.0);
      } else if(evt.xkey.keycode == 24){//q
        yawCamera(-0.01);
      } else if(evt.xkey.keycode == 26){//e
        yawCamera(0.01);
      } else if(evt.xkey.keycode == 42){//g
        pitchCamera(-0.01);
      } else if(evt.xkey.keycode == 28){//t
        pitchCamera(0.01);
      } else if(evt.xkey.keycode == 44){//j
        camera_dist+= 2*elapsed_time*TIME_CONST;
      } else if(evt.xkey.keycode == 45){//k
        camera_dist-= 2*elapsed_time*TIME_CONST;
      } else if(evt.xkey.keycode == 46){//l
        wireframe = !wireframe;
        XWarpPointer(dsp, win, win, 0, 0, 0, 0, WIDTH*resScale/2, HEIGHT*resScale/2);
        usleep(100000);
      }
    } else if(evt.type == MotionNotify && evt.xmotion.type == 6){
      XFixesHideCursor(dsp, win);
      if(evt.xmotion.x != WIDTH*resScale/2 && evt.xmotion.y != HEIGHT*resScale/2){
        int dx = lastMouseX - evt.xmotion.x;
        int dy = lastMouseY - evt.xmotion.y;
        XWarpPointer(dsp, win, win, 0, 0, 0, 0, WIDTH*resScale/2, HEIGHT*resScale/2);
        XFlush(dsp);
        yawCamera((double)-dx/200);
        pitchCamera((double)dy/200);
      }
      lastMouseX = evt.xmotion.x;
      lastMouseY = evt.xmotion.y;
    }
  }while( evt.xkey.keycode != ESC );
  

  XDestroyWindow( dsp, win );
  XCloseDisplay( dsp );

  return 0;
}
