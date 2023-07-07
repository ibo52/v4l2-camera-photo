#include <stdint.h>//uint8_t
#include <unistd.h>//provides access to the POSIX operating system API.
#include <stdlib.h>

#include <math.h>		//pow() func; M_PI, M_E constants
#include "filters.h"
#include "conv2d.h"
#include "normalize.h"

float* gaussfilt2d(int size, float sigma){

		float SQUARE_SIGMA=sigma*sigma;

        //float COEFF_ROOT=1/(2*M_PI*SQUARE_SIGMA);

        int range_=(size-1)/2;
        
        float *kernel;		//+1 alloc is for sum of weights of kernel
        kernel=(float*)malloc(pow(size,2)*sizeof(float)+1);
        
        float SUM_WEIGHTS=0;
        //center value is 1
        //as kernel normalized
        int idx=0;
		//calculate 2d gauss kernel
		//ref:https://en.wikipedia.org/wiki/Gaussian_blur
        for(int y=-range_; y< range_+1; y++){

            for(int x=-range_; x< range_+1; x++){

                float calculated= pow(M_E,-(x*x + y*y)/(2*SQUARE_SIGMA));

                kernel[idx++]=calculated;
             	SUM_WEIGHTS+=calculated;
            }
        }
        kernel[size*size]=SUM_WEIGHTS;//last index keeps sum of weights
        return kernel;
        
}

uint8_t* gaussBlur(uint8_t *buffer,int width,int height,int fsize,float fsigma){
	
	uint8_t *apply;//new array for filtered image
    float   *kernel;//our kernel to calculate distribution ratio
    int sizeReducted=(fsize-1);
	
    kernel=gaussfilt2d(fsize,fsigma);//(float*)malloc(fsize*fsize*sizeof(float));
    
    //here we normalizing result of float* to uint8*
    apply=normalize(conv2d(buffer,width, height, kernel, fsize)  ,width-sizeReducted, height-sizeReducted);
	
	return apply;//return processed image
}
