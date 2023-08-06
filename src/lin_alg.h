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
  point texA;
  point texB;
  point texC;
  point normA;
  point normB;
  point normC;
  point wsa;
  point wsb;
  point wsc;
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

point addPoints(point p1, point p2);

point scaleVector(point p, double value);

double vectorLength(point p);

point calcBCC(point p, triangle t);

point calcPCBCC(point p, triangle t);

double calcTriArea(triangle t);

double edgeFunc(point p, point p1, point p0);

void printTriangle(triangle tri);

#endif