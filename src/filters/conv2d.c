/*
		convoles the buffer with kernel
	*/
#include <stdint.h>		//uint8_t
#include <unistd.h>		//provides access to the POSIX operating system API.
#include <stdlib.h>
#include <pthread.h>
#include "utils.h"

typedef struct conv2dThreadArguments{
	uint8_t *buffer;	//pointer to read original buffer
	float *processed;	//pointer to write new buffer
	int width;			//original buffer width
	int height;			//original buffer height
	float* kernel;		//pointer to kernel for convole ops
	int ksize;			//size of kernel
}conv2dThreadArguments;
conv2dThreadArguments GLOBAL_CONV2D_ARGS;


//private interface for threading conv2d
void* conv2dThread(void *offset){

	//---GET THE ARGUMENTS------
	conv2dThreadArguments *arguments=&GLOBAL_CONV2D_ARGS;
	uint8_t *buffer=arguments->buffer;
	float *apply=arguments->processed;	//get new array pointer for convoled image
	int width=arguments->width;
	int height=arguments->height;
	float *kernel=arguments->kernel;
	int ksize=arguments->ksize;
	//---GET THE ARGUMENTS------
	
	//open new image array to apply filter
	int new_w=width-ksize+1;
	int new_h=height-ksize+1;
	short color_=3;//RGB color array
	
	
	
	float SUM_OF_WEIGHTS_OF_KERNEL = kernel[ksize*ksize];
	long i=(long)offset; int j=0, idx=0, a_idx=0;

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
		i+=NUM_PROCESSORS;
	}
	//Done
	pthread_exit( NULL ); //return from thread
	
}
//public interface
float* conv2d(uint8_t *buffer,int width,int height,float* kernel,int ksize){

	uintptr_t i, threadFailed;
	pthread_t thread_id[NUM_PROCESSORS]; //open thread list
	
	int new_w=width-ksize+1;
	int new_h=height-ksize+1;
	float *apply=calloc(new_w*new_h*3 ,sizeof(float));
	//write info to global variable
	conv2dThreadArguments argument={buffer, apply, width, height, kernel, ksize};
	GLOBAL_CONV2D_ARGS=argument;
	
	for(i=0; i<NUM_PROCESSORS; i++){

		    threadFailed=pthread_create(&thread_id[i], NULL, conv2dThread, (void*)i);
		    
		    if(threadFailed){
		        exit(threadFailed);
		    }
	}
	for(i=0; i<NUM_PROCESSORS; i++){
			pthread_join(thread_id[i],NULL);	//with joining threads, main waits for others to terminate.
	}

    return apply;
}
