#include <stdint.h>//uint8_t
#include <unistd.h>//provides access to the POSIX operating system API.
#include <stdlib.h>//calloc

//pad the image and return the address of padded buffer
uint8_t* pad(uint8_t *buffer,int width,int height,int top_pad, int right_pad, int bottom_pad, int left_pad){
	
	//open new image array and set to zero
	//uint8_t *padded;//dynamicly allocate to return
	int new_w=width+left_pad+right_pad;
	int new_h=height+top_pad+bottom_pad;
	short color_=3;//RGB
	
	uint8_t *padded;//new array for ğadded image
	padded=(uint8_t*)calloc(new_w*new_h*color_ ,sizeof(uint8_t));
	
	int i=0,j=0,idx=0;
	
	int left_right_pad_skip=left_pad*color_;//pad skipping for left and right sides
	
	int top_pad_skip=top_pad*new_w*color_;//pad skipping for top side
	
	while (i<height){
		
		j=j%width;
		while (j<width*color_){
		
			idx=i*width*color_+j;
			//transfer rgb data to appropriate location of new array
			padded[ idx+ top_pad_skip+ left_right_pad_skip ]= buffer[idx];
			
			j++;
		}
		left_right_pad_skip+=color_*(right_pad+ left_pad);
		i++;
	}
	
	return padded;
}
