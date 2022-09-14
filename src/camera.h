#ifndef CAMERA_H
#define CAMERA_H

#include "lin_alg.h"

extern double camera_dist;
extern point camera_pos;
extern double camera_angle_y;
extern double camera_angle_x;
extern triangle camera_basis;
extern point camera_dir;
extern int mouseX, mouseY, lastMouseX, lastMouseY;


void yawCamera(double rad);

void pitchCamera(double rad);

void movCamera(double distX, double distY, double distZ);

point toCameraBasis(point p);

triangle projectTriangle(triangle tri);

#endif