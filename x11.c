#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <xcb/xproto.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <stdbool.h>

#define WIDTH 640.0
#define HEIGHT 480.0
#define ESC 9

double camera_dist = 554.0; //For FOV 60


typedef struct point{
  double x;
  double y;
  double z;
} point;

typedef struct triangle{
  point a;
  point b;
  point c;
} triangle;

point camera_pos = {0.0, 0.0, 0.0};
double camera_angle_y = 0.0;
double camera_angle_x = -0.1;
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

double radToDeg(double rad){
  double deg = (rad * 360.0) / (2*M_PI);
  return deg;
}

double calcFOV(){
  double fov = 2 * atan((WIDTH/2)/camera_dist);
  return radToDeg(fov);
}

point translatePoint(point p, double x, double y, double z){
  point newPoint = {p.x + x, p.y + y, p.z + z};
  return newPoint;
}

triangle translateTriangle(triangle tri, double x, double y, double z){
  triangle translated_tri = {
    {tri.a.x + x, tri.a.y + y, tri.a.z + z},
    {tri.b.x + x, tri.b.y + y, tri.b.z + z},
    {tri.c.x + x, tri.c.y + y, tri.c.z + z}
  };
  return translated_tri;
}

void yawCamera(double rad){
  point newBasisX = {sin(M_PI/2 + rad + camera_angle_y), 0.0, cos(M_PI/2 + rad + camera_angle_y)};
  point newBasisZ = {sin(rad + camera_angle_y), 0.0, cos(rad + camera_angle_y)};
  camera_basis.a = newBasisX;
  camera_basis.c = newBasisZ;
  camera_angle_y += rad;
}

void pitchCamera(double rad){
  camera_angle_x += rad;
}

void movCamera(double distZ, double distX){
  camera_pos.z += cos(camera_angle_y)*distZ;
  camera_pos.x += sin(camera_angle_y)*distZ;

  camera_pos.z += cos(camera_angle_y + M_PI/2)*distX;
  camera_pos.x += sin(camera_angle_y + M_PI/2)*distX;
}

point calcCenter(triangle tri){
  double x = tri.a.x + tri.b.x + tri.c.x / 3;
  double y = tri.a.y + tri.b.y + tri.c.y / 3;
  double z = tri.a.z + tri.b.z + tri.c.z / 3;
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

triangle rotateX(triangle tri, double angle, double x, double y, double z){
  tri = translateTriangle(tri, -x, -y, -z);
  double x1, y1, x2, y2, x3, y3, z1, z2, z3;
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

triangle rotateY(triangle tri, double angle, double x, double y, double z){
  tri = translateTriangle(tri, -x, -y, -z);
  double x1, y1, x2, y2, x3, y3, z1, z2, z3;
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

triangle rotateZ(triangle tri, double angle, double x, double y, double z){
  tri = translateTriangle(tri, -x, -y, -z);
  double x1, y1, x2, y2, x3, y3, z1, z2, z3;
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
  double xr = cos(-camera_angle_y)*camToPoint.x + sin(-camera_angle_y)*camToPoint.z;
  double yr = camToPoint.y;
  double zr = -sin(-camera_angle_y)*camToPoint.x + cos(-camera_angle_y)*camToPoint.z;
  point rotated_py = {xr, yr, zr};
  xr = rotated_py.x;
  yr = cos(-camera_angle_x)*rotated_py.y - sin(-camera_angle_x)*rotated_py.z;
  zr = sin(-camera_angle_x)*rotated_py.y + cos(-camera_angle_x)*rotated_py.z;
  point(rotated_pyx) = {xr, yr, zr};
  return rotated_pyx;
}

triangle projectTriangle(triangle tri){
  int xp1, yp1, xp2, yp2, xp3, yp3;
  point a_cam = toCameraBasis(tri.a);
  point b_cam = toCameraBasis(tri.b);
  point c_cam = toCameraBasis(tri.c);
  xp1 = camera_dist * a_cam.x / a_cam.z + WIDTH/2;//projectX(tri.a);//
  xp2 = camera_dist * b_cam.x / b_cam.z + WIDTH/2;//projectX(tri.b);//
  xp3 = camera_dist * c_cam.x / c_cam.z + WIDTH/2;//projectX(tri.c);//
  yp1 = camera_dist * a_cam.y / a_cam.z + WIDTH/2;//projectY(tri.a);//
  yp2 = camera_dist * b_cam.y / b_cam.z + WIDTH/2;//projectY(tri.b);//
  yp3 = camera_dist * c_cam.y / c_cam.z + WIDTH/2;//projectY(tri.c);//
  triangle projected_tri = {
    {xp1, yp1, a_cam.z},
    {xp2, yp2, b_cam.z},
    {xp3, yp3, c_cam.z}
  };
  return projected_tri;
}


int main(){

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
  XSetForeground( dsp, gc, 0xA4FFB8 );


  eventMask = KeyPressMask|KeyReleaseMask;
  XSelectInput(dsp,win,eventMask); // override prev
   
  do{
    XClearWindow(dsp, win);

    XDrawString(dsp, win, gc, WIDTH-70, HEIGHT-10, "GGE v0.0.1", 10);
    char fov[11];
    double currentFOV = calcFOV();
    if(currentFOV < 100.0){
      snprintf(fov, sizeof fov-1, "FOV: %3.1lf", currentFOV);
      XDrawString(dsp, win, gc, 10, HEIGHT-10, fov, 9);
    }
    else{
      snprintf(fov, sizeof fov, "FOV: %3.1lf", currentFOV);
      XDrawString(dsp, win, gc, 10, HEIGHT-10, fov, 10);
    }
    //triangle projected_tri = projectTriangle(test);
    //rasterizeTriangle(dsp, win, gc, projected_tri);
    //drawTriangle(dsp, win, gc, projected_tri);
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
      double l = sqrt(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
      normal.x /= l; normal.y /= l; normal.z /= l;
      if(normal.z > 0){
        rasterizeTriangle(dsp, win, gc, projected_tri);
        //drawTriangle(dsp, win, gc, projected_tri);
      }
      
      
      //tris[i] = rotateX(tris[i], 0.012, 0, 0, 700);
      //tris[i] = rotateY(tris[i], 0.013, 0, -100, 800);
      //tris[i] = rotateZ(tris[i], 0.003, 0, -100, 800);
    }
    //XSync(dsp, 0);
    usleep(10000);
    XCheckWindowEvent( dsp, win, eventMask, &evt);

    if(evt.type == KeyPress){  //Handle Input
      //printf("%u\n", evt.xkey.keycode);
      if(evt.xkey.keycode == 25){       //w
        movCamera(2.0, 0.0);
      } else if(evt.xkey.keycode == 38){//a
        movCamera(0.0, -2.0);
      } else if(evt.xkey.keycode == 39){//s
        movCamera(-2.0, 0.0);
      } else if(evt.xkey.keycode == 40){//d
        movCamera(0.0, 2.0);
      } else if(evt.xkey.keycode == 27){//r
        camera_pos.y -= 2.0;
      } else if(evt.xkey.keycode == 41){//f
        camera_pos.y += 2.0;
      } else if(evt.xkey.keycode == 24){//q
        yawCamera(-0.01);
      } else if(evt.xkey.keycode == 26){//e
        yawCamera(0.01);
      } else if(evt.xkey.keycode == 42){//g
        pitchCamera(-0.01);
      } else if(evt.xkey.keycode == 28){//t
        pitchCamera(0.01);
      } else if(evt.xkey.keycode == 44){//j
        camera_dist++;
      } else if(evt.xkey.keycode == 45){//k
        camera_dist--;
      }
    }
  }while( evt.xkey.keycode != ESC );
  

  XDestroyWindow( dsp, win );
  XCloseDisplay( dsp );

  return 0;
}
