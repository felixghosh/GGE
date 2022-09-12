#ifndef LIN_ALG_H
#define LIN_ALG_H

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

point translatePoint(point p, float x, float y, float z);

triangle translateTriangle(triangle tri, float x, float y, float z);

triangle rotateX(triangle tri, float angle, float x, float y, float z);

triangle rotateY(triangle tri, float angle, float x, float y, float z);

triangle rotateZ(triangle tri, float angle, float x, float y, float z);

point calcCenter(triangle tri);

#endif