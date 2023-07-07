#ifndef halocam_filters_pad_h
#define halocam_filters_pad_h

#include<stdint.h>

uint8_t* pad(uint8_t *buffer,int width,int height,int top_pad, int right_pad, int bottom_pad, int left_pad);
#endif
