#ifndef IMAGEUTIL_H    
#define IMAGEUTIL_H



void saveBMP_24bit ( RGBType *data, const char *filename, int w, int h);
void loadBMP_24bit( RGBType *data, const char *filename, int *w, int *h);

inline void setRGB(RGBAType *ptr, float val);

//static void *
void createTestImage(RGBAType *buffer, int width, int height, float xS, float yS, float rad, int maxIteration);


#endif