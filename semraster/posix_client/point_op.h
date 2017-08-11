#ifndef POINTOP_H    
#define POINTOP_H

#define DEG_TO_RAD 0.0174532925
#define RAD_TO_DEG 57.29577951

//coordinate to a pixel in the scancache 
struct pix_coord {
    int x;
    int y;
};

//2 dimensional vector 
struct vector2d {
    float x;
    float y;
};

//object instance generators
vector2d newvec( float x, float y );
vector2d newvec( int   x, int   y );

//utilities
double deg_to_rad ( double deg);
double rad_to_deg ( double rad);
float dotProduct ( vector2d v1, vector2d v2);

void calc_circle ( pix_coord *out_coords, int numdiv, int x_orig, int y_orig, float dia, int *num);
void calc_line(  pix_coord *out_coords, int *pt1, int *pt2, int *num);

int get_line_intersection(float p0_x, float p0_y, float p1_x, float p1_y, 
    float p2_x, float p2_y, float p3_x, float p3_y, float *i_x, float *i_y);

float fcalc_distance(int pt1[2], int pt2[2]);
float fcalc_distance(float pt1[2], float pt2[2]);
float fcalc_distance( vector2d input);

float calc_theta_vert ( float start_x, float start_y, float end_x, float end_y);
float angle_between( vector2d v_one, vector2d v_two );

vector2d normalize( vector2d input );
vector2d scale_vec( vector2d input, float amount );
vector2d mult_vec_scalar( vector2d input, float amount );

vector2d line2vect(int start_x, int start_y, int end_x, int end_y);
vector2d line2vect(float start_x, float start_y, float end_x, float end_y);
vector2d vmul_2d ( vector2d v1, vector2d v2 );


#endif
