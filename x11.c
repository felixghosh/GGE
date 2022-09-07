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


void drawTriangle(Display* dsp, Window win, GC gc, triangle tri){
  XDrawLine(dsp, win, gc, tri.a.x, tri.a.y, tri.b.x, tri.b.y);
  XDrawLine(dsp, win, gc, tri.b.x, tri.b.y, tri.c.x, tri.c.y);
  XDrawLine(dsp, win, gc, tri.c.x, tri.c.y, tri.a.x, tri.a.y);
}

triangle rotateY(triangle tri, double angle){
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
  return rotated_tri;
}

triangle translateTriangle(triangle tri, int x, int y, int z){
  triangle translated_tri = {
    {tri.a.x + x, tri.a.y + y, tri.a.z + z},
    {tri.b.x + x, tri.b.y + y, tri.b.z + z},
    {tri.c.x + x, tri.c.y + y, tri.c.z + z}
  };
  return translated_tri;
}

triangle rotateZ(triangle tri, double angle){
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
  return rotated_tri;
}

triangle projectTriangle(triangle tri, int camera_distance){
  int xp1, yp1, xp2, yp2, xp3, yp3;
  xp1 = camera_distance * tri.a.x / tri.a.z + WIDTH/2;
  xp2 = camera_distance * tri.b.x / tri.b.z + WIDTH/2;
  xp3 = camera_distance * tri.c.x / tri.c.z + WIDTH/2;
  yp1 = camera_distance * tri.a.y / tri.a.z + HEIGHT/2;
  yp2 = camera_distance * tri.b.y / tri.b.z + HEIGHT/2;
  yp3 = camera_distance * tri.c.y / tri.c.z + HEIGHT/2;
  triangle projected_tri = {
    {xp1, yp1, tri.a.z},
    {xp2, yp2, tri.b.z},
    {xp3, yp3, tri.c.z}
  };
  return projected_tri;
}

int cam_dist = 100;

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

  for(int i = 0; i < 12; i++)
    tris[i] = translateTriangle(tris[i], 100, 100, 0);

  Display *dsp = XOpenDisplay( NULL );
  if( !dsp ){ return 1; }


  int screenNumber = DefaultScreen(dsp);
  unsigned long white = WhitePixel(dsp,screenNumber);
  unsigned long black = BlackPixel(dsp,screenNumber);


  Window win = XCreateSimpleWindow(dsp,
                               DefaultRootWindow(dsp),
                               50, 50,   // origin
                                WIDTH, HEIGHT, // size
                               0, black, // border
                               white );  // backgd

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
  XSetForeground( dsp, gc, black );


  

  eventMask = ButtonPressMask|ButtonReleaseMask|NoEventMask;
  XSelectInput(dsp,win,eventMask); // override prev
   
  int pos = 0; 
  double x = 1.0;
  double y = 1.0;
  do{
    XClearWindow(dsp, win);
    for(int i = 0; i < 12; i++){
      //triangle projected_tri = projectTriangle(tris[i], cam_dist);
      triangle projected_tri = tris[i];
      drawTriangle(dsp, win, gc, projected_tri);
      tris[i] = translateTriangle(tris[i], -100, -100, -100);
      tris[i] = rotateY(tris[i], 0.005);
      tris[i] = rotateZ(tris[i], 0.002);
      tris[i] = translateTriangle(tris[i], 100, 100, 100);
      
    }
    XSync(dsp, 0);
    usleep(10000);
    int e = XCheckWindowEvent( dsp, win, eventMask, &evt);
  }while( evt.type != ButtonRelease );
  

  XDestroyWindow( dsp, win );
  XCloseDisplay( dsp );

  return 0;
}
