/*
	pad the image and return the address of padded buffer
*/
#include <stdint.h>//uint8_t
#include <unistd.h>//provides access to the POSIX operating system API.
#include <stdlib.h>//calloc
#include<pthread.h>		//timestamp for imagenames
#include "utils.h"

typedef struct padThreadArguments{
	uint8_t *buffer;
	uint8_t *padded;
	int width;
	int height;
	int top_pad;
	int right_pad;
	int bottom_pad;
	int left_pad;
}padThreadArguments;
padThreadArguments GLOBAL_PAD_ARGS;

//private interface
void* padThread(void *offset){
	
	//---GET THE ARGUMENTS------
	padThreadArguments *arguments=&GLOBAL_PAD_ARGS;
	uint8_t *buffer=arguments->buffer;
	uint8_t *padded=arguments->padded;	//get new array pointer for convoled image
	int width=arguments->width;
	int height=arguments->height;
	int top_pad=arguments->top_pad;
	int right_pad=arguments->right_pad;
	//int bottom_pad=arguments->bottom_pad;
	int left_pad=arguments->left_pad;
	//---GET THE ARGUMENTS------

	int new_w=width+left_pad+right_pad;
	short color_=3;//RGB
	
	long i=(long)offset; int j=0,idx=0;

	int left_right_pad_skip=(left_pad + (right_pad+ left_pad)*i)*color_;//pad skipping for left and right sides
	
	int top_pad_skip=top_pad*new_w*color_;//pad skipping for top side
	
	while (i<height){
		
		j=j%width;
		while (j<width*color_){
		
			idx=i*width*color_+j;
			//transfer rgb data to appropriate location of new array
			padded[ idx+ top_pad_skip+ left_right_pad_skip ]= buffer[idx];
			
			j++;
		}
		left_right_pad_skip+=color_*(right_pad+ left_pad)*NUM_PROCESSORS;
		i+=NUM_PROCESSORS;
	}
	
	//Done
	pthread_exit( NULL ); //return from thread
}

//public interface
uint8_t* pad(uint8_t *buffer,int width,int height,int top_pad, int right_pad, int bottom_pad, int left_pad){

	uintptr_t i, threadFailed;
	pthread_t thread_id[NUM_PROCESSORS]; //open thread list
	
	int new_w=width+left_pad+right_pad;
	int new_h=height+top_pad+bottom_pad;
	
	uint8_t *padded=calloc(new_w*new_h*3 ,sizeof(float));
	
	padThreadArguments argument={buffer, padded, width, height, top_pad, right_pad, bottom_pad, left_pad};
	GLOBAL_PAD_ARGS=argument;
	
	for(i=0; i<NUM_PROCESSORS; i++){

		    threadFailed=pthread_create(&thread_id[i], NULL, padThread, (void*)i);
		    
		    if(threadFailed){
		        exit(threadFailed);
		    }
	}
	for(i=0; i<NUM_PROCESSORS; i++){
			pthread_join(thread_id[i],NULL);	//with joining threads, main waits for others to terminate.
	}
	
	return padded;
}
