#ifndef RIBBONMODE_H    
#define RIBBONMODE_H

void scan_ribbon(int size, int mode);
void scan_full_image(int size);

void ribbonCallback(void);
void videoCallback(void);
void raster_idle(void);

void render_ribbon_buffer(char *filename, int *scanBuffer,
                          int width, int height);

void dump_ribbon_buffer(char *filename,
                          int *scanBuffer,
                          int width, int height);

#endif