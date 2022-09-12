#ifndef CAMERA_H
#define CAMERA_H

#include "lin_alg.h"

extern float camera_dist;
extern point camera_pos;
extern float camera_angle_y;
extern float camera_angle_x;
extern triangle camera_basis;


void yawCamera(float rad);

void pitchCamera(float rad);

void movCamera(float distX, float distY, float distZ);

point toCameraBasis(point p);

triangle projectTriangle(triangle tri);

#endif