#ifndef halocam_filters_gaussblur_h
#define halocam_filters_gaussblur_h

#include<stdint.h>

float* gaussfilt2d(int size, float sigma);
uint8_t* gaussBlur(uint8_t *buffer,int width,int height,int fsize,float fsigma);

#endif
