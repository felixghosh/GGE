void rasterizeTriangle(SDL_Renderer* renderer, triangle tri){
  //sort points by height
  int i ;
  point p[3];
  p[0] = tri.a;
  p[1] = tri.b;
  p[2] = tri.c;
  sortPoints(p, 0, 1);
  sortPoints(p, 1, 2);
  sortPoints(p, 0, 1);

  if(p[0].y == p[2].y)
    return;

  bool shortSide = (p[1].y - p[0].y) * (p[2].x - p[0].x) < (p[1].x - p[0].x) * (p[2].y - p[0].y); //false = left, true = right
  
  int dy_long = round(p[2].y - p[0].y);
  double denominator = 1.0 / dy_long;
  double slope_long[dy_long];
  for(i = 0; i < dy_long; i++){
    slope_long[i] = p[0].x + (p[2].x-p[0].x)*(i)*denominator;
  }
  int dy_short = round(p[1].y - p[0].y);
  denominator = 1.0 / dy_short;
  double slope_short[dy_short];
  if(dy_short != 0){
    for(i = 0; i < dy_short; i++){
      slope_short[i] = p[0].x + (p[1].x-p[0].x)*i*denominator;
    }
  }
  
  int dy_last = dy_long - dy_short;//round((p[2].y - p[1].y));
  denominator = 1.0 / dy_last;
  double slope_last[dy_last];
  if(dy_last != 0){
    for(i = 0; i < dy_last; i++){
      slope_last[i] = p[1].x + (p[2].x-p[1].x)*i*denominator;
    }
  }
  //scanline
  i = 0;
  if(dy_short != 0){
    for(i; i < dy_short; i++){
      if((slope_short[i]*resScale) - (slope_long[i]*resScale) < 0){
        for(int j = 0; j < resScale; j++){
          for(int k  = slope_long[i]*resScale; k > slope_short[i]*resScale; k--){
            SDL_RenderDrawPoint(renderer, k, (i+p[0].y)*resScale+j);
          }
        }
      } else{
        for(int j = 0; j < resScale; j++){
          for(int k  = slope_long[i]*resScale; k <= slope_short[i]*resScale; k++){
            SDL_RenderDrawPoint(renderer, k, (i+p[0].y)*resScale+j);
          }
        }
        //SDL_RenderDrawLine(renderer, slope_long[i]*resScale, (i+p[0].y)*resScale+j, (int)slope_short[i]*resScale, (i+p[0].y)*resScale+j);
        //printf("drawing from (%lf, %lf) to (%lf, %lf)\n", slope_long[i], (i+p[0].y)*resScale+j, slope_short[i]*resScale, ((i+p[0].y)*resScale+j));
      }
    }      
  }
  if(dy_last != 0){
    int origin = i;
    for(i; i < dy_long; i++){
      if((slope_last[i - origin]*resScale) - (slope_long[i]*resScale) < 0){
        for(int j = 0; j < resScale; j++){
          for(int k = slope_long[i]*resScale; k > slope_last[i - origin]*resScale; k--){
            SDL_RenderDrawPoint(renderer, k, (i+p[0].y)*resScale+j);
          }
        }
      }else{
        for(int j = 0; j < resScale; j++){
          for(int k = slope_long[i]*resScale; k <= slope_last[i - origin]*resScale; k++){
            SDL_RenderDrawPoint(renderer, k, (i+p[0].y)*resScale+j);
          }
          //SDL_RenderDrawLine(renderer, slope_long[i]*resScale, (i+p[0].y)*resScale+j, (int)slope_last[i - origin]*resScale, (i+p[0].y)*resScale+j);
          //printf("drawing from (%lf, %lf) to (%lf, %lf)\n", slope_long[i], (i+p[0].y)*resScale+j, slope_last[i - origin]*resScale, ((i+p[0].y)*resScale+j));
        }
      }
    }
  }
}