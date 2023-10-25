#include <stdint.h>//uint8_t
#include <unistd.h>//provides access to the POSIX operating system API.
#include <stdlib.h>

#include "conv2d.h"

float* laplacian(uint8_t *buffer,int width,int height){
	//this function detects edges on image buffer
	int fsize=3;
	//---SPECIFY THE KERNEL----------
	float *kernel=(float*)malloc(fsize*fsize*sizeof(float)+1);//3x3 laplace mask
	for(int i=0;i<fsize ; i++){//memset is not appropriate for float types
		for(int j=0;j<fsize ; j++){
			kernel[i*fsize+j]=1;
		}
	}kernel[4]=-8;
	kernel[fsize*fsize]=fsize*fsize;
	//---SPECIFY THE KERNEL----------
	
	float *apply=conv2d(buffer,width, height, kernel, fsize);//processed image
	free(kernel);
	return apply;//return processed image
	
}
