#include "lin_alg.h"
#include "global.h"
#include <stdio.h>
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
    tri.color,
    tri.texA, tri.texB, tri.texC,
    tri.normA, tri.normB, tri.normC,
    {tri.a.x + x, tri.a.y + y, tri.a.z + z},
    {tri.b.x + x, tri.b.y + y, tri.b.z + z},
    {tri.c.x + x, tri.c.y + y, tri.c.z + z}
  };
  return translated_tri;
}
point rotatePointX(point p, double angle, double x, double y, double z){
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
  point normA = rotatePointX(tri.normA, angle, 0.0, 0.0, 0.0);
  point normB = rotatePointX(tri.normB, angle, 0.0, 0.0, 0.0);
  point normC = rotatePointX(tri.normC, angle, 0.0, 0.0, 0.0);
  triangle rotated_tri = {
    {x1, y1, z1},
    {x2, y2, z2},
    {x3, y3, z3},
    tri.color,
    tri.texA, tri.texB, tri.texC,
    normA, normB, normC,
    {x1, y1, z1},
    {x2, y2, z2},
    {x3, y3, z3}
  };
  rotated_tri = translateTriangle(rotated_tri, x, y, z);
  return rotated_tri;
}

triangle rotateTriY(triangle tri, double angle, double x, double y, double z){
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
  point normA = rotatePointY(tri.normA, angle, 0.0, 0.0, 0.0);
  point normB = rotatePointY(tri.normB, angle, 0.0, 0.0, 0.0);
  point normC = rotatePointY(tri.normC, angle, 0.0, 0.0, 0.0);
  triangle rotated_tri = {
    {x1, y1, z1},
    {x2, y2, z2},
    {x3, y3, z3},
    tri.color,
    tri.texA, tri.texB, tri.texC,
    normA, normB, normC,
    {x1, y1, z1},
    {x2, y2, z2},
    {x3, y3, z3}
  };
  rotated_tri = translateTriangle(rotated_tri, x, y, z);
  return rotated_tri;
}

triangle rotateTriZ(triangle tri, double angle, double x, double y, double z){
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
  point normA = rotatePointZ(tri.normA, angle, 0.0, 0.0, 0.0);
  point normB = rotatePointZ(tri.normB, angle, 0.0, 0.0, 0.0);
  point normC = rotatePointZ(tri.normC, angle, 0.0, 0.0, 0.0);
  triangle rotated_tri = {
    {x1, y1, z1},
    {x2, y2, z2},
    {x3, y3, z3},
    tri.color,
    tri.texA, tri.texB, tri.texC,
    normA, normB, normC,
    {x1, y1, z1},
    {x2, y2, z2},
    {x3, y3, z3}
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
  double denominator = 1.0/Length;
  p.x *= denominator;
  p.y *= denominator;
  p.z *= denominator;
  return p;
}

point roundVector(point p){
  return (point){
    round(p.x),
    round(p.y),
    round(p.z)
  };
}

point scaleVector(point p, double value){
  return (point){p.x*value, p.y*value, p.z*value};
}

double dotProduct(point p1, point p2){
  return p1.x*p2.x + p1.y*p2.y + p1.z*p2.z;
}

point subtractPoints(point p1, point p2){
  return (point) {p1.x - p2.x, p1.y - p2.y, p1.z - p2.z};
}

point addPoints(point p1, point p2){
  return (point) {p1.x + p2.x, p1.y + p2.y, p1.z + p2.z};
}

double vectorLength(point p){
  return sqrt(p.x*p.x + p.y*p.y + p.z*p.z);
}

point calcBCC(point p, triangle t){
  point p0 = t.a;
  point p1 = t.b;
  point p2 = t.c;
  
  double A1 = 0.5*((p.x - p2.x)*(p1.y - p2.y)-(p.y - p2.y)*(p1.x -  p2.x));
  double A2 = 0.5*((p.x - p0.x)*(p2.y - p0.y)-(p.y - p0.y)*(p2.x -  p0.x));
  double A3 = 0.5*((p.x - p1.x)*(p0.y - p1.y)-(p.y - p1.y)*(p0.x -  p1.x));
  
  double area = A1 + A2 + A3;
  
  double divisor = 1 / area;
  double u = A1 * divisor;
  double v = A2 * divisor;
  u = u < 0.0 ? 0.0 : u;
  v = v < 0.0 ? 0.0 : v;
  
  u = u > 1.0 ? 1.0 : u;
  v = v > 1.0 ? 1.0 : v;
  
  u = isnan(u) ? 0.0 : u;
  v = isnan(v) ? 0.0 : v;

  double w = 1.0 - u - v;
  w = w < 0.0 ? 0.0 : w;
  w = w > 1.0 ? 1.0 : w;
  w = isnan(w) ? 0.0 : w;

  return (point){u, v, w};
}

point calcPCBCC(point p, triangle t, double* params){
  double h_w0 = -t.a.z;
  double h_w1 = -t.b.z;
  double h_w2 = -t.c.z;
  
  double e0 = edgeFunc(p, t.b, params[0], params[1]);
  double e1 = edgeFunc(p, t.c, params[2], params[3]);
  double e2 = edgeFunc(p, t.a, params[4], params[5]);

  double f0 = e0 / h_w0;
  double f1 = e1 / h_w1;
  double f2 = e2 / h_w2;
  
  double divisor = 1 / (f0+f1+f2);

  double u = f0 * divisor;
  double v = f1 * divisor;
  u = u < 0.0 ? 0.0 : u;
  v = v < 0.0 ? 0.0 : v;
  
  u = u > 1.0 ? 1.0 : u;
  v = v > 1.0 ? 1.0 : v;
  
  u = isnan(u) ? 0.0 : u;
  v = isnan(v) ? 0.0 : v;

  double w = 1.0 - u - v;
  w = w < 0.0 ? 0.0 : w;
  w = w > 1.0 ? 1.0 : w;
  w = isnan(w) ? 0.0 : w;
  return (point){u, v, w};
}

double interpolate_attrib(point bcc, double s0, double s1, double s2){

}

double calcTriArea(triangle t){
  point p0 = t.a;
  point p1 = t.b;
  point p2 = t.c;
  double a = sqrt(pow((p0.x - p1.x), 2) + pow((p0.x - p1.y), 2));
  double b = sqrt(pow((p1.x - p2.x), 2) + pow((p1.x - p2.y), 2));
  double c = sqrt(pow((p2.x - p0.x), 2) + pow((p2.x - p0.y), 2));

  double area = ((a+b+c)*(-a+b+c)*(a-b+c)*(a+b-c));
  area = area < 0 ? -area : area;
  area = 0.25*sqrt(area);
  return area;
}

double edgeFunc(point p, point p0, double a, double b){
  return a * (p.x - p0.x) + b * (p.y - p0.y);
}

void printTriangle(triangle tri){
  printf("a{%2.2lf, %2.2lf, %2.2lf} b{%2.2lf, %2.2lf, %2.2lf} c{%2.2lf, %2.2lf, %2.2lf}\n", tri.a.x, tri.a.y, tri.a.z, tri.b.x, tri.b.y, tri.b.z, tri.c.x, tri.c.y, tri.c.z);
}