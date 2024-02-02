#ifndef halocam_filters_conv2d_h
#define halocam_filters_conv2d_h

#include<stdint.h>

float* conv2d(uint8_t *buffer,int width,int height,float* kernel,int ksize);

#endif
