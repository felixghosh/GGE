#include "camera.h"
#include "lin_alg.h"
#include "global.h"
#include <math.h>

void yawCamera(double rad){
  camera_angle_y += rad;
  camera_dir.x = cos(rad)*camera_dir.x + sin(rad)*camera_dir.z;
  camera_dir.z = -sin(rad)*camera_dir.x + cos(rad)*camera_dir.z;
}

void pitchCamera(double rad){
  if(camera_angle_x - rad > -M_PI/2 && camera_angle_x - rad < M_PI/2)
    camera_angle_x -= rad;
  camera_dir.y = cos(rad)*camera_dir.y - sin(rad)*camera_dir.z;
  camera_dir.z = sin(rad)*camera_dir.y + cos(rad)*camera_dir.z;
}


void movCamera(double distX, double distY, double distZ){
  camera_pos.z += cos(camera_angle_y)*distZ + cos(camera_angle_y + M_PI/2)*distX;
  camera_pos.x += sin(camera_angle_y)*distZ + sin(camera_angle_y + M_PI/2)*distX;
  camera_pos.y += distY;
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
  return (triangle){a_cam, b_cam, c_cam};
}

triangle projectTriangle(triangle tri){
  double xp1, yp1, xp2, yp2, xp3, yp3;
  point a_cam = tri.a;//toCameraBasis(tri.a);
  point b_cam = tri.b;//toCameraBasis(tri.b);
  point c_cam = tri.c;//toCameraBasis(tri.c);

  xp1 = round(camera_dist * a_cam.x / a_cam.z + WIDTH/2);
  yp1 = round(camera_dist * a_cam.y / a_cam.z + WIDTH/2);
  xp2 = round(camera_dist * b_cam.x / b_cam.z + WIDTH/2);
  yp2 = round(camera_dist * b_cam.y / b_cam.z + WIDTH/2);
  xp3 = round(camera_dist * c_cam.x / c_cam.z + WIDTH/2);
  yp3 = round(camera_dist * c_cam.y / c_cam.z + WIDTH/2);
  
  triangle projected_tri = {
    {xp1, yp1, a_cam.z},
    {xp2, yp2, b_cam.z},
    {xp3, yp3, c_cam.z},
    tri.color
  };
  return projected_tri;
}