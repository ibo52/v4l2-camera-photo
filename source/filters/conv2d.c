#include <stdint.h>//uint8_t
#include <unistd.h>//provides access to the POSIX operating system API.
#include <stdlib.h>

float* conv2d(uint8_t *buffer,int width,int height,float* kernel,int ksize){
	/*
		convoles the buffer with kernel, then returns address of filtered image
	*/
	//open new image array to apply filter
	int new_w=width-ksize+1;
	int new_h=height-ksize+1;
	short color_=3;//RGB color array
	
	float *apply;//new array for filtered image
	apply=(float*)calloc(new_w*new_h*color_ ,sizeof(float));
	
	float SUM_OF_WEIGHTS_OF_KERNEL = kernel[ksize*ksize];
	int i=0,j=0,idx=0,a_idx=0;
	
	while (i<new_h){
		
		j=j%new_w;
		while (j<new_w*color_){

                        /*placing the center of the kernel
                        to the original image array*/
						idx=i*width*color_+j;
                        
                        //to access our new image array
                        a_idx=i*new_w*color_+j;

                        int ken[ksize*ksize*3];
                            
                        //here we getting buffer index offsets
                        //to process later on
                        for(int y=0; y<ksize; y++){

                            for(int x=0; x<ksize*3; x++){

                            	int idx2=y*width*color_+x;
                            	//printf(" ken[%d]= idx+ idx2:%d+ %d\n",y*fsize*3+x,idx,idx2);
                            	ken[y*ksize*3+x]=idx+idx2;//keeping indexes to be processed
                       		}
						}
                        //calcultaing the sum of indexes with kernel
                        //then placing manipulated data to new buffer
                        float sum[3];
                        for(int y=0; y< ksize*ksize*3; y+=3){
                            int c_idx=y/3;
                            
                            float coeff=kernel[c_idx];//getting coefficient from kernel
                            
                            sum[0]+=buffer[ ken[ y ] ]*coeff;//R values
                            sum[1]+=buffer[ ken[y+1] ]*coeff;//G values
                            sum[2]+=buffer[ ken[y+2] ]*coeff;//B values
                        }
                        
			//transfer manipulated data to appropriate location of new array
			apply[  a_idx  ]= sum[0]/SUM_OF_WEIGHTS_OF_KERNEL;
            apply[ a_idx+1 ]= sum[1]/SUM_OF_WEIGHTS_OF_KERNEL;
            apply[ a_idx+2 ]= sum[2]/SUM_OF_WEIGHTS_OF_KERNEL;
            
			sum[0]=0;
			sum[1]=0;
			sum[2]=0;
			j+=color_;
		}
		i++;
	}
	return apply;
	
}
