#include "lin_alg.h"
#include "global.h"
#include <math.h>

point translatePoint(point p, double x, double y, double z){
  point newPoint = {p.x + x, p.y + y, p.z + z};
  return newPoint;
}

triangle translateTriangle(triangle tri, double x, double y, double z){
  triangle translated_tri = {
    {tri.a.x + x, tri.a.y + y, tri.a.z + z},
    {tri.b.x + x, tri.b.y + y, tri.b.z + z},
    {tri.c.x + x, tri.c.y + y, tri.c.z + z},
    tri.color
  };
  return translated_tri;
}
point rotatePointX(point p, double angle, double x, double y, double z){
  angle *= elapsed_time*TIME_CONST;
  p = (point){p.x - x, p.y - y, p.z - z};
  double rx, ry, rz;
  rx = p.x;
  ry = cos(angle)*p.y - sin(angle)*p.z;
  rz = sin(angle)*p.y + cos(angle)*p.z;
  p = (point){rx, ry, rz};
  p = (point){p.x + x, p.y + y, p.z + z};
  return p;
}

point rotatePointY(point p, double angle, double x, double y, double z){
  angle *= elapsed_time*TIME_CONST;
  p = (point){p.x - x, p.y - y, p.z - z};
  double rx, ry, rz;
  rx = cos(angle)*p.x + sin(angle)*p.z;
  ry = p.y;
  rz = -sin(angle)*p.x + cos(angle)*p.z;
  p = (point){rx, ry, rz};
  p = (point){p.x + x, p.y + y, p.z + z};
  return p;
}

point rotatePointZ(point p, double angle, double x, double y, double z){
  angle *= elapsed_time*TIME_CONST;
  p = (point){p.x - x, p.y - y, p.z - z};
  double rx, ry, rz;
  rx = cos(angle)*p.x - sin(angle)*p.y;
  ry = sin(angle)*p.x + cos(angle)*p.y;
  rz = p.z;
  p = (point){rx, ry, rz};
  p = (point){p.x + x, p.y + y, p.z + z};
  return p;
}

triangle rotateTriX(triangle tri, double angle, double x, double y, double z){
  angle *= elapsed_time*TIME_CONST;
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
    {x3, y3, z3},
    tri.color
  };
  rotated_tri = translateTriangle(rotated_tri, x, y, z);
  return rotated_tri;
}

triangle rotateTriY(triangle tri, double angle, double x, double y, double z){
  angle *= elapsed_time*TIME_CONST;
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
    {x3, y3, z3},
    tri.color
  };
  rotated_tri = translateTriangle(rotated_tri, x, y, z);
  return rotated_tri;
}

triangle rotateTriZ(triangle tri, double angle, double x, double y, double z){
  angle *= elapsed_time*TIME_CONST;
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
    {x3, y3, z3},
    tri.color
  };
  rotated_tri = translateTriangle(rotated_tri, x, y, z);
  return rotated_tri;
}

point calcCenter(triangle tri){
  double x = (tri.a.x + tri.b.x + tri.c.x) / 3;
  double y = (tri.a.y + tri.b.y + tri.c.y) / 3;
  double z = (tri.a.z + tri.b.z + tri.c.z) / 3;
  point p = {x, y, z};
  return p;
}

point calcNormal(triangle tri){
  point normal, line1, line2;
  line1.x = tri.b.x - tri.a.x;
  line1.y = tri.b.y - tri.a.y;
  line1.z = tri.b.z - tri.a.z;

  line2.x = tri.c.x - tri.a.x;
  line2.y = tri.c.y - tri.a.y;
  line2.z = tri.c.z - tri.a.z;

  normal.x = line1.y*line2.z - line1.z*line2.y;
  normal.y = line1.z*line2.x - line1.x*line2.z;
  normal.z = line1.x*line2.y - line1.y*line2.x;

  return normal;
}

point normalizeVector(point p){
  double Length = sqrt(p.x*p.x + p.y*p.y + p.z*p.z);
  p.x /= Length; p.y /= Length; p.z /= Length;
  return p;
}

point roundVector(point p){
  return (point){
    round(p.x),
    round(p.y),
    round(p.z)
  };
}

double dotProduct(point p1, point p2){
  return p1.x*p2.x + p1.y*p2.y + p1.z*p2.z;
}

point subtractPoints(point p1, point p2){
  return (point) {p1.x - p2.x, p1.y - p2.y, p1.z - p2.z};
}

double vectorLength(point p){
  return sqrt(p.x*p.x + p.y*p.y + p.z*p.z);
}