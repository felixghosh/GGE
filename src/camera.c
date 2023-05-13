#include "camera.h"
#include "lin_alg.h"
#include "global.h"
#include <math.h>
#include <stdio.h>

void yawCamera(double rad){
  camera_angle_y += rad;
  camera_dir.x = sin(camera_angle_y);
  camera_dir.z = cos(camera_angle_y);
}

void pitchCamera(double rad){
  if(camera_angle_x - rad > -M_PI/2 && camera_angle_x - rad < M_PI/2){
    camera_angle_x -= rad;
    camera_dir.y = sin(camera_angle_x);
  }
}


void movCamera(double distX, double distY, double distZ){
  camera_pos.z += cos(camera_angle_y)*distZ + cos(camera_angle_y + M_PI/2)*distX;
  camera_pos.x += sin(camera_angle_y)*distZ + sin(camera_angle_y + M_PI/2)*distX;
  camera_pos.y += distY;
}

void movCameraWorldSpace(double distX, double distY, double distZ){
  camera_pos.z += distZ;
  camera_pos.y += distY;
  camera_pos.x += distX;
}

point toCameraBasis(point p){
  point camToPoint = {
    p.x - camera_pos.x, p.y - camera_pos.y, p.z - camera_pos.z
  };
  double xr = cos(-camera_angle_y)*camToPoint.x + sin(-camera_angle_y)*camToPoint.z;
  double yr = camToPoint.y;
  double zr = -sin(-camera_angle_y)*camToPoint.x + cos(-camera_angle_y)*camToPoint.z;
  point rotated_py = {xr, yr, zr};
  xr = rotated_py.x;
  yr = cos(-camera_angle_x)*rotated_py.y - sin(-camera_angle_x)*rotated_py.z;
  zr = sin(-camera_angle_x)*rotated_py.y + cos(-camera_angle_x)*rotated_py.z;
  point(rotated_pyx) = {xr, yr, zr};
  return rotated_pyx;
}

triangle toCameraBasisTriangle(triangle tri){
  point a_cam = toCameraBasis(tri.a);
  point b_cam = toCameraBasis(tri.b);
  point c_cam = toCameraBasis(tri.c);
  return (triangle){a_cam, b_cam, c_cam, tri.color, tri.texA, tri.texB, tri.texC};
}

point projectPoint(point p){
  double xp1, yp1;
  
  xp1 = camera_dist * p.x / p.z + WIDTH/2;
  yp1 = camera_dist * p.y / p.z + WIDTH/2;

  return (point){xp1, yp1, p.z};
}

triangle projectTriangle(triangle tri){
  double xp1, yp1, xp2, yp2, xp3, yp3;
  point a_cam = tri.a;//toCameraBasis(tri.a);
  point b_cam = tri.b;//toCameraBasis(tri.b);
  point c_cam = tri.c;//toCameraBasis(tri.c);

  xp1 = camera_dist * a_cam.x / a_cam.z + WIDTH/2;
  yp1 = camera_dist * a_cam.y / a_cam.z + WIDTH/2;
  xp2 = camera_dist * b_cam.x / b_cam.z + WIDTH/2;
  yp2 = camera_dist * b_cam.y / b_cam.z + WIDTH/2;
  xp3 = camera_dist * c_cam.x / c_cam.z + WIDTH/2;
  yp3 = camera_dist * c_cam.y / c_cam.z + WIDTH/2;
  
  triangle projected_tri = {
    {xp1, yp1, a_cam.z},
    {xp2, yp2, b_cam.z},
    {xp3, yp3, c_cam.z},
    tri.color,
    tri.texA,
    tri.texB,
    tri.texC
  };
  return projected_tri;
}

triangle screenToEyeSpace(triangle tri){
  double xp1, yp1, xp2, yp2, xp3, yp3;

  double denominator = 1/camera_dist;
  double c = (WIDTH/2);
  xp1 =  (tri.a.x - c)*(tri.a.z) * denominator;
  yp1 =  (tri.a.y - c)*(tri.a.z) * denominator;
  xp2 =  (tri.b.x - c)*(tri.b.z) * denominator;
  yp2 =  (tri.b.y - c)*(tri.b.z) * denominator;
  xp3 =  (tri.c.x - c)*(tri.c.z) * denominator;
  yp3 =  (tri.c.y - c)*(tri.c.z) * denominator;

  triangle eyeTri = {
    {xp1, yp1, tri.a.z},
    {xp2, yp2, tri.b.z},
    {xp3, yp3, tri.c.z},
    tri.color
  };

  return eyeTri;
}