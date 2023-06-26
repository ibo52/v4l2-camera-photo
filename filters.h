//filters, libjpeg definitions, image save options
#ifndef halocam_filters_h
#define halocam_filters_h

#include<stdint.h>

void im2bw(uint8_t*,int);
void im2gray(uint8_t*,int);
void im2inverse(uint8_t*,int);
int decode_rgb(uint8_t*,int ,int ,int );
int save2ppm(uint8_t*,int ,int ,int ,char*);
int save2jpeg(unsigned char *,int ,int ,int ,char *,int );
uint8_t* pad(uint8_t*,int width, int height, int , int ,int ,int);
#endif
