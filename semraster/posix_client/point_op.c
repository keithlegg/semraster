#include <stdlib.h>
#include <stdio.h>
#include <cmath>


#include "point_op.h"


double deg_to_rad ( double deg){
   return deg * DEG_TO_RAD;
}

double rad_to_deg ( double rad){
   return rad * RAD_TO_DEG;
}

/*****************************/
float dotProduct ( vector2d v1, vector2d v2){
    return (v1.x*v2.x + v1.y*v2.y ); 
}

/*****************************/
float fcalc_distance(float pt1[2], float pt2[2]){
    return sqrt( ( (pt1[0]-pt2[0])*(pt1[0]-pt2[0])) + ((pt1[1]-pt2[1])*(pt1[1]-pt2[1])) ) ;
}

float fcalc_distance(int pt1[2], int pt2[2]){
    return sqrt( ( (pt1[0]-pt2[0])*(pt1[0]-pt2[0])) + ((pt1[1]-pt2[1])*(pt1[1]-pt2[1])) ) ;
}

float fcalc_distance( vector2d input){
    return sqrt( (input.x * input.x) + (input.y * input.y) );
}

/*****************************/
vector2d newvec( int x, int y ){
    vector2d output;
    output.x = (float)x;
    output.y = (float)y;
    return output; 
}

vector2d newvec( float x, float y ){
    vector2d output;
    output.x = x;
    output.y = y;
    return output; 
}

/*****************************/
vector2d mult_vec_scalar( vector2d input, float amount ){
    vector2d output;
    output.x = input.x*amount;
    output.y = input.y*amount;
    return output; 
}

/*****************************/
vector2d scale_vec( vector2d input, float amount ){
    vector2d output;
    output.x = input.x/amount;
    output.y = input.y/amount;
    return output; 
}
/*****************************/
vector2d normalize( vector2d input )
{
     /* normalize a vector */

     float length  = sqrt( (input.x * input.x) + (input.y * input.y) );

     vector2d output;

     if(length != 0)
     {
        output.x = input.x/length;
        output.y = input.y/length;
     }

     return output; 
}

/*****************************/

vector2d vmul_2d ( vector2d v1, vector2d v2 )
{
     /* multiply two vectors */

     vector2d output;
     
     output.x = v1.x * v2.x;
     output.y = v1.y * v2.y;

     return output; 
}

/*****************************/

vector2d line2vect(float start_x, float start_y, float end_x, float end_y)
/*
  convert an arbitrary line segment to a true vector from origin.
*/

{
    vector2d out;
    out.x = end_x-start_x;
    out.y = end_y-start_y;
    return out;
}

vector2d line2vect(int start_x, int start_y, int end_x, int end_y)
{
    vector2d out;
    out.x = (float)(end_x-start_x);
    out.y = (float)(end_y-start_y);
    return out;
}


/*****************************/

float angle_between( vector2d v_one, vector2d v_two )
{
    vector2d v1 ;
    vector2d v2 ;
    v1 = normalize(v_one);
    v2 = normalize(v_two);
    float dot = dotProduct(v1,v2);
    return rad_to_deg( acos(dot) );
}


/*****************************/

float calc_theta_vert ( float start_x, float start_y, float end_x, float end_y)
{
    //#get corner to build a right triangle
    float a_x = end_x-start_x;  
    float a_y = end_y-start_y;

    float r = 0;
    
    //#relative offset (depending on order of start-end)
    if (a_x != 0 && a_y !=0){
         r = ( rad_to_deg( atan(a_x/a_y) ) );
    }
  
    //printf(" angle is %f\n" , r);
    return r;
}


/*****************************/


int get_line_intersection(float p0_x, float p0_y, float p1_x, float p1_y, 
    float p2_x, float p2_y, float p3_x, float p3_y, float *i_x, float *i_y)
{
    float s02_x, s02_y, s10_x, s10_y, s32_x, s32_y, s_numer, t_numer, denom, t;
    s10_x = p1_x - p0_x;
    s10_y = p1_y - p0_y;
    s32_x = p3_x - p2_x;
    s32_y = p3_y - p2_y;

    denom = s10_x * s32_y - s32_x * s10_y;
    if (denom == 0)
        return 0; // Collinear
    bool denomPositive = denom > 0;

    s02_x = p0_x - p2_x;
    s02_y = p0_y - p2_y;
    s_numer = s10_x * s02_y - s10_y * s02_x;
    if ((s_numer < 0) == denomPositive)
        return 0; // No collision

    t_numer = s32_x * s02_y - s32_y * s02_x;
    if ((t_numer < 0) == denomPositive)
        return 0; // No collision

    if (((s_numer > denom) == denomPositive) || ((t_numer > denom) == denomPositive))
        return 0; // No collision
    // Collision detected
    t = t_numer / denom;
    if (i_x != NULL)
        *i_x = p0_x + (t * s10_x);
    if (i_y != NULL)
        *i_y = p0_y + (t * s10_y);

    return 1;
}

/*****************************/
//not convinced this totally works due to roundoff error, but its close enough
void calc_circle ( pix_coord *out_coords, int numdiv, int x_orig, int y_orig, float dia, int *num)
{
    int divamt = (int)(360/numdiv);
    int rotation_offset = 45;

    for (int i = 0; i <360; i=i+divamt)
    {  
        out_coords[*num].x = x_orig + (sin(deg_to_rad(i-rotation_offset))*dia);
        out_coords[*num].y = y_orig + (cos(deg_to_rad(i-rotation_offset))*dia);
        *num = *num+1;
    }

}

/*****************************/

void calc_line( pix_coord *out_coords, int *pt1, int *pt2, int *num)
{

    int x1 = pt1[0];
    int y1 = pt1[1];
    int const x2 = pt2[0];
    int const y2 = pt2[1];

    int const foobar = 100;

    short xy_idx     = 0;
    int delta_x(x2 - x1);

    // if x1 == x2, then it does not matter what we set here
    signed char const ix((delta_x > 0) - (delta_x < 0));
    delta_x = abs(delta_x) << 1;
    int delta_y(y2 - y1);
    // if y1 == y2, then it does not matter what we set here
    signed char const iy((delta_y > 0) - (delta_y < 0));
    delta_y = abs(delta_y) << 1;
    
    //dump a point
    
    out_coords[xy_idx].x = x1;
    out_coords[xy_idx].y = y1;
    xy_idx++;


    if (delta_x >= delta_y)
    {
      // error may go below zero
      int error(delta_y - (delta_x >> 1));
      while (x1 != x2)
      {
          if ((error >= 0) && (error || (ix > 0)))
          {
              error -= delta_x;
              y1 += iy;
          }
          // else do nothing
          error += delta_y;
          x1 += ix;
            //dump  a point
            out_coords[xy_idx].x = x1;
            out_coords[xy_idx].y = y1;
            xy_idx++;

      }
    }
    else
    {
      // error may go below zero
      int error(delta_x - (delta_y >> 1));
      while (y1 != y2)
      {
          if ((error >= 0) && (error || (iy > 0)))
          {
              error -= delta_y;
              x1 += ix;
          }
          // else do nothing
          error += delta_x;
          y1 += iy;

            //dump  a point
            out_coords[xy_idx].x = x1;
            out_coords[xy_idx].y = y1;
            xy_idx++;
      }
    }

   *num = xy_idx;
}

/*****************************/


