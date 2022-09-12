#include "lin_alg.h"
#include "global.h"
#include <math.h>

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

point calcCenter(triangle tri){
  float x = tri.a.x + tri.b.x + tri.c.x / 3;
  float y = tri.a.y + tri.b.y + tri.c.y / 3;
  float z = tri.a.z + tri.b.z + tri.c.z / 3;
  point p = {x, y, z};
  return p;
}