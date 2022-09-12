#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <xcb/xproto.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#define WIDTH 640.0
#define HEIGHT 480.0
#define ESC 9
#define TIME_CONST 100

float camera_dist = 554.0; //For FOV 60

struct timespec t0, t1;
double elapsed_time;

typedef struct point{
  float x;
  float y;
  float z;
} point;

typedef struct triangle{
  point a;
  point b;
  point c;
} triangle;

point camera_pos = {0.0, 0.0, 0.0};
float camera_angle_y = 0.0;
float camera_angle_x = -0.1;

point light_direction = {0.0, 0.0, 1.0};

unsigned int cubeColor = 0x60E0E0;

bool wireframe = false;

triangle camera_basis = { //Currently not used
  {1.0, 0.0, 0.0},
  {0.0, 1.0, 0.0},
  {0.0, 0.0, 1.0}
};

triangle tri0 = {
  {0, 0, 100},
  {100, 0, 100},
  {0, 100, 100}
  
};
triangle tri1 = {
  {0, 100, 100},
  {100, 0, 100},
  {100, 100, 100}
};
triangle tri2 = {
  {0, 0, 100},
  {0, 100, 100},
  {0, 0, 200}
  
};
triangle tri3 = {
  {0, 0, 200},
  {0, 100, 100},
  {0, 100, 200}
};
triangle tri4 = {
  {0, 0, 200},
  {0, 100, 200},
  {100, 0, 200}
};
triangle tri5 = {
  {0, 100, 200},
  {100, 100, 200},
  {100, 0, 200}
  
};
triangle tri6 = {
  {100, 0, 100},
  {100, 0, 200},
  {100, 100, 100}
};
triangle tri7 = {
  {100, 0, 200},
  {100, 100, 200},
  {100, 100, 100}
  
};
triangle tri8 = {
  {0, 0, 100},
  {0, 0, 200},
  {100, 0, 100}
  
};
triangle tri9 = {
  {100, 0, 100},
  {0, 0, 200},
  {100, 0, 200}
};
triangle tri10 = {
  {0, 100, 100},
  {100, 100, 100},
  {0, 100, 200}
};
triangle tri11 = {
  {100, 100, 100},
  {100, 100, 200},
  {0, 100, 200}
  
};

triangle test = {
  {0, 0, 100},
  {10, 4, 100},
  {4, 11, 100}
};

float radToDeg(float rad){
  float deg = (rad * 360.0) / (2*M_PI);
  return deg;
}

float calcFOV(){
  float fov = 2 * atan((WIDTH/2)/camera_dist);
  return radToDeg(fov);
}

unsigned int colorLightness(float value, unsigned int color){
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

point translatePoint(point p, float x, float y, float z){
  point newPoint = {p.x + x, p.y + y, p.z + z};
  return newPoint;
}

triangle translateTriangle(triangle tri, float x, float y, float z){
  triangle translated_tri = {
    {tri.a.x + x, tri.a.y + y, tri.a.z + z},
    {tri.b.x + x, tri.b.y + y, tri.b.z + z},
    {tri.c.x + x, tri.c.y + y, tri.c.z + z}
  };
  return translated_tri;
}

void yawCamera(float rad){
  rad*=elapsed_time*TIME_CONST;
  point newBasisX = {sin(M_PI/2 + rad + camera_angle_y), 0.0, cos(M_PI/2 + rad + camera_angle_y)};
  point newBasisZ = {sin(rad + camera_angle_y), 0.0, cos(rad + camera_angle_y)};
  camera_basis.a = newBasisX;
  camera_basis.c = newBasisZ;
  camera_angle_y += rad;
}

void pitchCamera(float rad){
  rad*=elapsed_time*TIME_CONST;
  camera_angle_x += rad;
}

void movCamera(float distZ, float distX, float distY){
  distZ*=elapsed_time*TIME_CONST; distX*=elapsed_time*TIME_CONST; distY*=elapsed_time*TIME_CONST;
  camera_pos.z += cos(camera_angle_y)*distZ;
  camera_pos.x += sin(camera_angle_y)*distZ;

  camera_pos.z += cos(camera_angle_y + M_PI/2)*distX;
  camera_pos.x += sin(camera_angle_y + M_PI/2)*distX;

  camera_pos.y += distY;
}

point calcCenter(triangle tri){
  float x = tri.a.x + tri.b.x + tri.c.x / 3;
  float y = tri.a.y + tri.b.y + tri.c.y / 3;
  float z = tri.a.z + tri.b.z + tri.c.z / 3;
  point p = {x, y, z};
  return p;
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
  float denominator = 1 / dy_long;
  float slope_long[dy_long];
  for(i = 0; i < dy_long; i++){
    slope_long[i] = p[0].x + (p[2].x-p[0].x)*(i)/dy_long;
    //XDrawPoint(dsp, win, gc, slope_long[i], (int)i+p[0].y);
  }
  int dy_short = (p[1].y - p[0].y);
  denominator = 1 / dy_short;
  float slope_short[dy_short];
  if(dy_short != 0){
    for(i = 0; i < dy_short; i++){
      slope_short[i] = p[0].x + (p[1].x-p[0].x)*i/dy_short;
      //XDrawPoint(dsp, win, gc, slope_short[i], (int)i+p[0].y);
    }
  }
  int dy_last = (p[2].y - p[1].y);
  denominator = 1 / dy_last;
  float slope_last[dy_last];
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
      XDrawLine(dsp, win, gc, slope_long[i], i+p[0].y, (int)slope_short[i], i+p[0].y);
    }
    
  }
  if(dy_last != 0){
    int origin = i;
    for(i; i < dy_long; i++){
      XDrawLine(dsp, win, gc, slope_long[i], i+p[0].y, (int)slope_last[i - origin], i+p[0].y);
    }
    
  }
  //XFlush(dsp);
}

triangle rotateX(triangle tri, float angle, float x, float y, float z){
  angle *= elapsed_time*TIME_CONST;
  tri = translateTriangle(tri, -x, -y, -z);
  float x1, y1, x2, y2, x3, y3, z1, z2, z3;
  x1 = tri.a.x;
  x2 = tri.b.x;
  x3 = tri.c.x;
  y1 = cos(angle)*tri.a.y - sin(angle)*tri.a.z;
  y2 = cos(angle)*tri.b.y - sin(angle)*tri.b.z;
  y3 = cos(angle)*tri.c.y - sin(angle)*tri.c.z;
  z1 = sin(angle)*tri.a.y + cos(angle)*tri.a.z;
  z2 = sin(angle)*tri.b.y + cos(angle)*tri.b.z;
  z3 = sin(angle)*tri.c.y + cos(angle)*tri.c.z;
  triangle rotated_tri = {
    {x1, y1, z1},
    {x2, y2, z2},
    {x3, y3, z3}
  };
  rotated_tri = translateTriangle(rotated_tri, x, y, z);
  return rotated_tri;
}

triangle rotateY(triangle tri, float angle, float x, float y, float z){
  angle *= elapsed_time*TIME_CONST;
  tri = translateTriangle(tri, -x, -y, -z);
  float x1, y1, x2, y2, x3, y3, z1, z2, z3;
  x1 = cos(angle)*tri.a.x + sin(angle)*tri.a.z;
  x2 = cos(angle)*tri.b.x + sin(angle)*tri.b.z;
  x3 = cos(angle)*tri.c.x + sin(angle)*tri.c.z;
  y1 = tri.a.y;
  y2 = tri.b.y;
  y3 = tri.c.y;
  z1 = -sin(angle)*tri.a.x + cos(angle)*tri.a.z;
  z2 = -sin(angle)*tri.b.x + cos(angle)*tri.b.z;
  z3 = -sin(angle)*tri.c.x + cos(angle)*tri.c.z;
  triangle rotated_tri = {
    {x1, y1, z1},
    {x2, y2, z2},
    {x3, y3, z3}
  };
  rotated_tri = translateTriangle(rotated_tri, x, y, z);
  return rotated_tri;
}

triangle rotateZ(triangle tri, float angle, float x, float y, float z){
  angle *= elapsed_time*TIME_CONST;
  tri = translateTriangle(tri, -x, -y, -z);
  float x1, y1, x2, y2, x3, y3, z1, z2, z3;
  x1 = cos(angle)*tri.a.x - sin(angle)*tri.a.y;
  x2 = cos(angle)*tri.b.x - sin(angle)*tri.b.y;
  x3 = cos(angle)*tri.c.x - sin(angle)*tri.c.y;
  y1 = sin(angle)*tri.a.x + cos(angle)*tri.a.y;
  y2 = sin(angle)*tri.b.x + cos(angle)*tri.b.y;
  y3 = sin(angle)*tri.c.x + cos(angle)*tri.c.y;
  z1 = tri.a.z;
  z2 = tri.b.z;
  z3 = tri.c.z;
  triangle rotated_tri = {
    {x1, y1, z1},
    {x2, y2, z2},
    {x3, y3, z3}
  };
  rotated_tri = translateTriangle(rotated_tri, x, y, z);
  return rotated_tri;
}

point toCameraBasis(point p){
  point camToPoint = {
    p.x - camera_pos.x, p.y - camera_pos.y, p.z - camera_pos.z
  };
  float xr = cos(-camera_angle_y)*camToPoint.x + sin(-camera_angle_y)*camToPoint.z;
  float yr = camToPoint.y;
  float zr = -sin(-camera_angle_y)*camToPoint.x + cos(-camera_angle_y)*camToPoint.z;
  point rotated_py = {xr, yr, zr};
  xr = rotated_py.x;
  yr = cos(-camera_angle_x)*rotated_py.y - sin(-camera_angle_x)*rotated_py.z;
  zr = sin(-camera_angle_x)*rotated_py.y + cos(-camera_angle_x)*rotated_py.z;
  point(rotated_pyx) = {xr, yr, zr};
  return rotated_pyx;
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

triangle projectTriangle(triangle tri){
  int xp1, yp1, xp2, yp2, xp3, yp3;
  point a_cam = toCameraBasis(tri.a);
  point b_cam = toCameraBasis(tri.b);
  point c_cam = toCameraBasis(tri.c);
  xp1 = round(camera_dist * a_cam.x / a_cam.z + WIDTH/2);
  xp2 = round(camera_dist * b_cam.x / b_cam.z + WIDTH/2);
  xp3 = round(camera_dist * c_cam.x / c_cam.z + WIDTH/2);
  yp1 = round(camera_dist * a_cam.y / a_cam.z + WIDTH/2);
  yp2 = round(camera_dist * b_cam.y / b_cam.z + WIDTH/2);
  yp3 = round(camera_dist * c_cam.y / c_cam.z + WIDTH/2);
  triangle projected_tri = {
    {xp1, yp1, a_cam.z},
    {xp2, yp2, b_cam.z},
    {xp3, yp3, c_cam.z}
  };
  return projected_tri;
}


int main(){
  clock_gettime(CLOCK_REALTIME, &t0);
  triangle* tris = malloc(12*sizeof(triangle));
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

  point* centers = malloc(12*sizeof(point));
  centers[0] = calcCenter(tris[0]);
  centers[1] = calcCenter(tris[1]);
  centers[2] = calcCenter(tris[2]);
  centers[3] = calcCenter(tris[3]);
  centers[4] = calcCenter(tris[4]);
  centers[5] = calcCenter(tris[5]);
  centers[6] = calcCenter(tris[6]);
  centers[7] = calcCenter(tris[7]);
  centers[8] = calcCenter(tris[8]);
  centers[9] = calcCenter(tris[9]);
  centers[10] = calcCenter(tris[10]);
  centers[11] = calcCenter(tris[11]);

  for(int i = 0; i < 12; i++)
    tris[i] = translateTriangle(tris[i], 0, -100, 700);

  Display *dsp = XOpenDisplay( NULL );
  if( !dsp ){ return 1; }

  int screenNumber = DefaultScreen(dsp);
  unsigned long white = WhitePixel(dsp,screenNumber);
  unsigned long black = BlackPixel(dsp,screenNumber);

  Window win = XCreateSimpleWindow(dsp,
                               DefaultRootWindow(dsp),
                               50, 50,   // origin
                                WIDTH, HEIGHT, // size
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

  Pixmap double_buffer = XCreatePixmap(dsp, win, WIDTH, HEIGHT, 24);


  eventMask = KeyPressMask|KeyReleaseMask;
  XSelectInput(dsp,win,eventMask); // override prev
   
  do{
    clock_gettime(CLOCK_REALTIME, &t1);
    elapsed_time = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec)/1000000000.0;
    clock_gettime(CLOCK_REALTIME, &t0);
    //printf("fps: %u\n", (int)(1/elapsed_time));
    XClearWindow(dsp, win);
    XSetForeground( dsp, gc, black);
    XFillRectangle(dsp, double_buffer, gc, 0, 0, WIDTH, HEIGHT);
    XSetForeground( dsp, gc, white);
    XDrawString(dsp, double_buffer, gc, WIDTH-70, HEIGHT-10, "GGE v0.0.1", 10);
    char fov[11];
    float currentFOV = calcFOV();
    if(currentFOV < 100.0){
      snprintf(fov, sizeof fov-1, "FOV: %3.1lf", currentFOV);
      XDrawString(dsp, double_buffer, gc, 10, HEIGHT-10, fov, 9);
    }
    else{
      snprintf(fov, sizeof fov, "FOV: %3.1lf", currentFOV);
      XDrawString(dsp, double_buffer, gc, 10, HEIGHT-10, fov, 10);
    }
    for(int i = 0; i < 12; i++){
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
      
      float normalLength = sqrt(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
      normal.x /= normalLength; normal.y /= normalLength; normal.z /= normalLength;
      normal.x = roundf(normal.x*100)/100; normal.y = roundf(normal.y*100)/100; normal.z = roundf(normal.z*100)/100;
      if(normal.z > 0){
        
        float lightLength = sqrt(light_direction.x*light_direction.x + light_direction.y*light_direction.y + light_direction.z*light_direction.z);
        light_direction.x /= lightLength; light_direction.y /= lightLength; light_direction.z /= lightLength;
        light_direction.x = roundf(light_direction.x*100)/100; light_direction.y = roundf(light_direction.y*100)/100; light_direction.z = roundf(light_direction.z*100)/100;
        float dp = normal.x * light_direction.x + normal.y*light_direction.y + normal.z*light_direction.z;

        unsigned int color = colorLightness(dp, cubeColor);
        XSetForeground( dsp, gc, color);

        if(!wireframe)
          rasterizeTriangle(dsp, double_buffer, gc, projected_tri);
        else
          drawTriangle(dsp, double_buffer, gc, projected_tri);
      }
      
      //tris[i] = rotateX(tris[i], 0.012, 0, -100, 800);
      tris[i] = rotateY(tris[i], 0.013, 0, -100, 800);
      tris[i] = rotateZ(tris[i], 0.003, 0, -100, 800);
    }
    //XSync(dsp, 0);
    XCopyArea(dsp, double_buffer, win, gc, 0, 0, WIDTH, HEIGHT, 0, 0);
    usleep(1000);
    XCheckWindowEvent( dsp, win, eventMask, &evt);

    if(evt.type == KeyPress){  //Handle Input
      //printf("%u\n", evt.xkey.keycode);
      if(evt.xkey.keycode == 25){       //w
        movCamera(2.0, 0.0, 0.0);
      } else if(evt.xkey.keycode == 38){//a
        movCamera(0.0, -2.0, 0.0);
      } else if(evt.xkey.keycode == 39){//s
        movCamera(-2.0, 0.0, 0.0);
      } else if(evt.xkey.keycode == 40){//d
        movCamera(0.0, 2.0, 0.0);
      } else if(evt.xkey.keycode == 27){//r
        movCamera(0.0, 0.0, -2.0);
      } else if(evt.xkey.keycode == 41){//f
        movCamera(0.0, 0.0, 2.0);
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
        usleep(100000);
      }
    }
  }while( evt.xkey.keycode != ESC );
  

  XDestroyWindow( dsp, win );
  XCloseDisplay( dsp );

  return 0;
}
