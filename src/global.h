#define WIDTH 800.0
#define HEIGHT 600.0
#define ESC 9
#define TIME_CONST 100
#include "light.h"
#include <stdbool.h>

extern double elapsed_time;
extern struct timespec t0, t1;
extern light *lights;
extern unsigned int nLights;
extern bool debug;
extern double depth_buffer[(int)WIDTH][(int)HEIGHT];