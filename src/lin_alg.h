#ifndef LIN_ALG_H
#define LIN_ALG_H

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

point translatePoint(point p, double x, double y, double z);

triangle translateTriangle(triangle tri, double x, double y, double z);

triangle rotateX(triangle tri, double angle, double x, double y, double z);

triangle rotateY(triangle tri, double angle, double x, double y, double z);

triangle rotateZ(triangle tri, double angle, double x, double y, double z);

point calcCenter(triangle tri);

#endif