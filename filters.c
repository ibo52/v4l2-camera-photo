#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <jpeglib.h>

void im2bw(uint8_t *buffer,int size){
	//for rgb data
	int i;
        
	for(i=0;i<size;i+=3){
		
		if( buffer[i]<128 ){
			buffer[i]=(uint8_t)0;
			buffer[i+1]=(uint8_t)0;
			buffer[i+2]=(uint8_t)0;
		}
		else{
			buffer[i]=(uint8_t)255;
			buffer[i+1]=(uint8_t)255;
			buffer[i+2]=(uint8_t)255;
			}
	}
}

void im2inverse(uint8_t *buffer,int size){
	//for rgb data
	int i;
        
	for(i=0;i<size;i++){
		uint8_t avg=255-buffer[i];
		buffer[i]=avg;
	}
}

void im2gray(uint8_t *buffer,int size){
	//for rgb data
	int i;
        
	for(i=0;i<size;i+=3){
		int avg=(buffer[i]*299 + buffer[i+1]*587 + buffer[i+2]*114)/1000;
		buffer[i]=avg;
		buffer[i+1]=avg;
		buffer[i+2]=avg;
	}
}

int save2ppm(uint8_t *rgb_buffer,int buffer_size,int width,int height,char *img_name){

	int _savefile_ = open( img_name , O_CREAT | O_WRONLY, 0666);
	char buf[1024];

	//rc is written size of buf
	int rc = sprintf(buf, "P6 %d %d 255\n", width, height);
	
	write( _savefile_ , buf, rc); // Write the PPM image header before data
	write( _savefile_ , rgb_buffer, buffer_size); // Write out all RGB pixel data
	
	close( _savefile_ );
	
	free( rgb_buffer );

	//printf("End of decompression\n");
	return 0;
}

//decode jpg to rgb raw values
//libjpeg-turbo8-dev library used
int decode_rgb(unsigned char *buffer,int buffsize,int width,int height) {
	int rc, i, j;

	// Variables for the decompressor itself
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	// Variables for the output buffer, and how long it is
	unsigned long processed_size;
	unsigned char *processed_buffer;


	cinfo.err = jpeg_std_error(&jerr);	
	jpeg_create_decompress(&cinfo);

	jpeg_mem_src(&cinfo, buffer, buffsize);

	rc = jpeg_read_header(&cinfo, TRUE);

	if (rc != 1) {
		perror("Broken JPEG");
		return -1;
	}

	jpeg_start_decompress(&cinfo);

	int pixel_size = cinfo.output_components;

	processed_size = width * height * pixel_size;
	processed_buffer = (unsigned char*) malloc(processed_size);
 
	int row_stride = width * pixel_size;


	while (cinfo.output_scanline < cinfo.output_height) {
		unsigned char *temp_array[1];
		temp_array[0] = processed_buffer + (cinfo.output_scanline) * row_stride;

		jpeg_read_scanlines(&cinfo, temp_array, 1);

	}

	jpeg_finish_decompress(&cinfo);

	jpeg_destroy_decompress(&cinfo);

	//free(jpg_buffer);
	char *out_img_name="decode.ppm";
	char c;
	
	printf(	"------------FILTERS------------\n"
			"1. binary\n2. inverse\n3. grayscale\n"
			"------------FILTERS------------\n");
			
	printf("enter the filter num to apply: ");
	scanf(" %c",&c);
	
	switch( c ){
		case '1':{
			out_img_name="binary_filtered.ppm";
			im2bw(processed_buffer,processed_size);
			break;
		}
		
		case '2':{
			out_img_name="inverse_filtered.ppm";
			im2inverse(processed_buffer,processed_size);
			break;
		}
		
		case '3':{
			out_img_name="grayscale_filtered.ppm";
			im2gray(processed_buffer,processed_size);
			break;
		}
		default:{
			printf("default:do nothing\n");
			return 0;
		}
	}

	save2ppm(processed_buffer, processed_size, width, height,out_img_name);
	//printf("End of decompression\n");
	return 0;
}
