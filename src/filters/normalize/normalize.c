#include <stdint.h>//uint8_t
#include <unistd.h>//provides access to the POSIX operating system API.
#include <stdlib.h>
#include <stdio.h>

#include "normalize.h"

uint8_t *normalize(float *buffer,int width,int height){
	//function normalizes image to 0-255 range
	float max=buffer[0];
	float min=buffer[0];
	
	int newMin=0;
	int newMax=255;
	
	uint8_t *new_buffer=(uint8_t*)malloc(width*height*3*sizeof(uint8_t));
	
	for(int i=1; i<width*height*3; i++){
		if (buffer[i]<min){
			min=buffer[i];
		}
		else if(buffer[i]>max){
			max=buffer[i];
		}
	}
	
	//printf("min:%f max:%f\n",min,max);
	
	for(int i=0; i<width*height*3; i++){
		new_buffer[i]=(buffer[i]-min)*(newMax-newMin)/(max-min)+newMin;
	}

	return new_buffer;
}
