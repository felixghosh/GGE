#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <xcb/xproto.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#define WIDTH 640
#define HEIGHT 480
#define ESC 9
#define CAM_DIST 640


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
double camera_angle = 0.0;
triangle camera_basis = {
  {1.0, 0.0, 0.0},
  {0.0, 1.0, 0.0},
  {0.0, 0.0, 1.0}
};

triangle tri0 = {
  {0, 0, 100},
  {0, 100, 100},
  {100, 0, 100}
};
triangle tri1 = {
  {0, 100, 100},
  {100, 0, 100},
  {100, 100, 100}
};
triangle tri2 = {
  {0, 0, 100},
  {0, 0, 200},
  {0, 100, 100}
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
  {100, 0, 200},
  {100, 100, 200}
};
triangle tri6 = {
  {100, 0, 100},
  {100, 0, 200},
  {100, 100, 100}
};
triangle tri7 = {
  {100, 0, 200},
  {100, 100, 100},
  {100, 100, 200}
};
triangle tri8 = {
  {0, 0, 100},
  {100, 0, 100},
  {0, 0, 200}
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
  {0, 100, 200},
  {100, 100, 200}
};

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

void rotCamera(double rad){
  point newBasisX = {sin(M_PI/2 + rad + camera_angle), 0.0, cos(M_PI/2 + rad + camera_angle)};
  point newBasisZ = {sin(rad + camera_angle), 0.0, cos(rad + camera_angle)};
  camera_basis.a = newBasisX;
  camera_basis.c = newBasisZ;
  camera_angle += rad;
  
}

void movCamera(double distZ, double distX){
  camera_pos.z += cos(camera_angle)*distZ;
  camera_pos.x += sin(camera_angle)*distZ;

  camera_pos.z += cos(camera_angle + M_PI/2)*distX;
  camera_pos.x += sin(camera_angle + M_PI/2)*distX;
}

point calcCenter(triangle tri){
  double x = tri.a.x + tri.b.x + tri.c.x / 3;
  double y = tri.a.y + tri.b.y + tri.c.y / 3;
  double z = tri.a.z + tri.b.z + tri.c.z / 3;
  point p = {x, y, z};
  return p;
}

void drawTriangle(Display* dsp, Window win, GC gc, triangle tri){
  XDrawLine(dsp, win, gc, tri.a.x, tri.a.y, tri.b.x, tri.b.y);
  XDrawLine(dsp, win, gc, tri.b.x, tri.b.y, tri.c.x, tri.c.y);
  XDrawLine(dsp, win, gc, tri.c.x, tri.c.y, tri.a.x, tri.a.y);
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

/*int projectX(point p){
  point o = {0.0, 0.0, 1.0}; //original cam pos vector
  point v = {.x = o.x*cos(camera_angle)-o.z*sin(camera_angle), .y = 0.0, .z = o.x*sin(camera_angle)+o.z*cos(camera_angle)};
  point u = {camera_pos.x - p.x, 0.0, camera_pos.z - p.z};
  
  double proj_const = (v.x*u.x + v.z*u.z) / sqrt(pow(v.x, 2) + pow(v.z, 2));
  point u_proj = {v.x*proj_const ,0.0 , v.z*proj_const};
  point w = {u.x - u_proj.x, 0.0, u.z - u_proj.z};
  signed int sign = w.x >= 0 ? -1 : 1;

  int x_screen = sign * CAM_DIST * sqrt(pow(w.x, 2) + pow(w.z, 2)) / sqrt(pow(u_proj.x, 2) + pow(u_proj.z, 2)) + WIDTH/2;

  return x_screen;
}

int projectY(point p){
  point o = {0.0, 0.0, 1.0}; //original cam pos vector
  point v = o;//{.x = 0.0, .y = o.y*cos(camera_angle)-o.z*sin(camera_angle), .z = o.y*sin(camera_angle)+o.z*cos(camera_angle)};
  point u = {0.0, camera_pos.y - p.y, camera_pos.z - p.z};
  double proj_const = (v.y*u.y + v.z*u.z) / sqrt(pow(v.y, 2) + pow(v.z, 2));
  point u_proj = {0.0, v.y*proj_const, v.z*proj_const};
  point w = {0.0, u.y - u_proj.y, u.z - u_proj.z};
  signed int sign = w.y >= 0 ? -1 : 1;

  int y_screen = sign * CAM_DIST * sqrt(pow(w.y, 2) + pow(w.z, 2)) / sqrt(pow(u_proj.y, 2) + pow(u_proj.z, 2)) + HEIGHT/2;

  return y_screen;
}*/

point toCameraBasis(point p){
  point new_p = {
    p.x - camera_pos.x, p.y - camera_pos.y, p.z - camera_pos.z
  };
  double xr = cos(-camera_angle)*new_p.x + sin(-camera_angle)*new_p.z;
  double yr = new_p.y;
  double zr = -sin(-camera_angle)*new_p.x + cos(-camera_angle)*new_p.z;
  point rotated_p = {xr, yr, zr};
  return rotated_p;
}

triangle projectTriangle(triangle tri){
  int xp1, yp1, xp2, yp2, xp3, yp3;
  point a_cam = toCameraBasis(tri.a);
  point b_cam = toCameraBasis(tri.b);
  point c_cam = toCameraBasis(tri.c);
  xp1 = CAM_DIST * a_cam.x / a_cam.z + WIDTH/2;//projectX(tri.a);//
  xp2 = CAM_DIST * b_cam.x / b_cam.z + WIDTH/2;//projectX(tri.b);//
  xp3 = CAM_DIST * c_cam.x / c_cam.z + WIDTH/2;//projectX(tri.c);//
  yp1 = CAM_DIST * a_cam.y / a_cam.z + WIDTH/2;//projectY(tri.a);//
  yp2 = CAM_DIST * b_cam.y / b_cam.z + WIDTH/2;//projectY(tri.b);//
  yp3 = CAM_DIST * c_cam.y / c_cam.z + WIDTH/2;//projectY(tri.c);//
  triangle projected_tri = {
    {xp1, yp1, tri.a.z},
    {xp2, yp2, tri.b.z},
    {xp3, yp3, tri.c.z}
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
  XSetForeground( dsp, gc, white );


  eventMask = KeyPressMask|KeyReleaseMask;
  XSelectInput(dsp,win,eventMask); // override prev
   
  do{
    XClearWindow(dsp, win);
    XDrawString(dsp, win, gc, WIDTH-70, HEIGHT-10, "GGE v0.0.1", 10);
    for(int i = 0; i < 12; i++){
      triangle projected_tri = projectTriangle(tris[i]);
      drawTriangle(dsp, win, gc, projected_tri);
      //tris[i] = rotateX(tris[i], 0.012, 0, 0, 700);
      //tris[i] = rotateY(tris[i], 0.013, 0, -100, 800);
      //tris[i] = rotateZ(tris[i], 0.003, 0, -100, 800);
    }
    XSync(dsp, 0);
    usleep(10000);
    XCheckWindowEvent( dsp, win, eventMask, &evt);

    if(evt.type == KeyPress){  //Handle Input
      //printf("%u\n", evt.xkey.keycode);
      if(evt.xkey.keycode == 25){       //w
        //for(int i = 0; i < 12; i++)
        //  tris[i] = translateTriangle(tris[i], 0, 0, -1);
        movCamera(2.0, 0.0);
      } else if(evt.xkey.keycode == 38){//a
        //for(int i = 0; i < 12; i++)
        //  tris[i] = translateTriangle(tris[i], -1, 0, 0);
        movCamera(0.0, -2.0);
      } else if(evt.xkey.keycode == 39){//s
        //for(int i = 0; i < 12; i++)
        //  tris[i] = translateTriangle(tris[i], 0, 0, 1);
        movCamera(-2.0, 0.0);
      } else if(evt.xkey.keycode == 40){//d
        //for(int i = 0; i < 12; i++)
        //  tris[i] = translateTriangle(tris[i], 1, 0, 0);
        movCamera(0.0, 2.0);
      } else if(evt.xkey.keycode == 27){//r
        //for(int i = 0; i < 12; i++)
        //  tris[i] = translateTriangle(tris[i], 0, 1, 0);
        camera_pos.y -= 2.0;
      } else if(evt.xkey.keycode == 41){//f
        //for(int i = 0; i < 12; i++)
        //  tris[i] = translateTriangle(tris[i], 0, -1, 0);
        camera_pos.y += 2.0;
      } else if(evt.xkey.keycode == 24){//q
        //for(int i = 0; i < 12; i++)
        //  tris[i] = rotateY(tris[i], 0.01, 0, 0, 700);
        rotCamera(-0.01);
      } else if(evt.xkey.keycode == 26){//e
        //for(int i = 0; i < 12; i++)
        //  tris[i] = rotateY(tris[i], -0.01, 0, 0, 700);
        rotCamera(0.01);
      }
    }
    printf("cam:(%lf,%lf,%lf)\n", camera_pos.x, camera_pos.y, camera_pos.z);
  }while( evt.xkey.keycode != ESC );
  

  XDestroyWindow( dsp, win );
  XCloseDisplay( dsp );

  return 0;
}
