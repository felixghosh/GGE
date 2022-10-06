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
  unsigned int color;
} triangle;



point translatePoint(point p, double x, double y, double z);

triangle translateTriangle(triangle tri, double x, double y, double z);

triangle rotateTriX(triangle tri, double angle, double x, double y, double z);

triangle rotateTriY(triangle tri, double angle, double x, double y, double z);

triangle rotateTriZ(triangle tri, double angle, double x, double y, double z);

point rotatePointX(point p, double angle, double x, double y, double z);

point rotatePointY(point p, double angle, double x, double y, double z);

point rotatePointZ(point p, double angle, double x, double y, double z);

point calcCenter(triangle tri);

point calcNormal(triangle tri);

point normalizeVector(point p);

point roundVector(point p);

double dotProduct(point p1, point p2);

point subtractPoints(point p1, point p2);

double vectorLength(point p);

#endif