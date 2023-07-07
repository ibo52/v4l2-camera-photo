#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include<string.h>
#include <jpeglib.h>	//to decode jpg image buffer
#include<time.h>		//timestamp for imagenames
#include <math.h>		//pow() func; M_PI, M_E constants

//---USER DEFINED HEADER FILES---
#include "filters.h"//function definitions of this file
#include "laplace.h"//laplace definitions
#include "gauss-blur.h"//gaussian blurring definitions
#include "conv2d.h"//convolve definitions
#include "normalize.h"//convolve definitions
#include "pad.h"//convolve definitions

//--------------------------------------------------------
int write_time(char *buff,int buffsize){
	time_t rawtime;
	struct tm *local_curr;
	time(&rawtime);
	
	local_curr=localtime(&rawtime);
	
	char timecalc[32];
	strftime(timecalc, 32 ,"%d %b %Y %H:%M:%S-",local_curr);
	
	strcat(buff,timecalc);
	return 0;
	
}

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

//save with no compression.Useful to check outputs
int save2ppm(uint8_t *rgb_buffer,int buffer_size,int width,int height,char *img_name){

	//---setting filename--
	char filename[128];
	memset(&filename,0,sizeof(filename));
	
	strcat(filename,"../images/ppm/");
	write_time(filename, sizeof(filename) );
	strcat(filename,img_name);
	strcat(filename,".ppm");
	//--------------------
	
	int _savefile_ = open( filename , O_CREAT | O_WRONLY, 0666);
	char buf[1024];

	//rc is written size of buf
	int rc = sprintf(buf, "P6 %d %d 255\n", width, height);
	
	write( _savefile_ , buf, rc); // Write the PPM image header before data
	write( _savefile_ , rgb_buffer, buffer_size); // Write out all RGB pixel data
	
	close( _savefile_ );
	
	//free( rgb_buffer );

	//printf("End of decompression\n");
	return 0;
}

int save2jpeg(unsigned char *rgb_buffer,int buffsize,int width,int height,char *img_name,int quality) {
	
	//---setting filename--
	char filename[ 128 ];
	memset(&filename,0,sizeof(filename));
	
	strcat(filename,"../images/");
	write_time(filename, sizeof(filename) );
	strcat(filename,img_name);
	strcat(filename,".jpg");
	//--------------------
	
	// Variables for the decompressor itself
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr);	
	jpeg_create_compress(&cinfo);

	FILE *out=fopen(filename,"wb");
	
	if (out==NULL) {
		perror("JPEG:file create errror");
		return -1;
	}
	
	jpeg_stdio_dest(&cinfo, out);
	
	cinfo.image_width = width; 	/* image width and height, in pixels */
	cinfo.image_height = height;
	cinfo.input_components = 3;		/* # of color components per pixel */
	cinfo.in_color_space = JCS_RGB; 	
	
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);
	
	jpeg_start_compress(&cinfo, TRUE);


	int row_stride = width * 3;//3==cinfo.output_components

	while (cinfo.next_scanline < cinfo.image_height) {
	
		unsigned char *temp_array[1];
		temp_array[0] = &rgb_buffer[ cinfo.next_scanline * row_stride ];

		jpeg_write_scanlines(&cinfo, temp_array, 1);

	}

	jpeg_finish_compress(&cinfo);
	
	fclose(out);
	
	jpeg_destroy_compress(&cinfo);

	//printf("End of compression\n");
	return 0;
}

//decode jpg to rgb raw values
//libjpeg-turbo8-dev library used
int decode_rgb(unsigned char *buffer,int buffsize,int width,int height) {
	int rc;

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

	int pixel_size = cinfo.output_components;//pixelsize=3 (r,g,b)

	processed_size = width * height * pixel_size;
	processed_buffer = malloc(processed_size*sizeof(uint8_t));
 
	int row_stride = width * pixel_size;


	while (cinfo.output_scanline < cinfo.output_height) {
		unsigned char *temp_array[1];
		temp_array[0] = processed_buffer + (cinfo.output_scanline) * row_stride;

		jpeg_read_scanlines(&cinfo, temp_array, 1);

	}

	jpeg_finish_decompress(&cinfo);

	jpeg_destroy_decompress(&cinfo);

	//free(jpg_buffer);
	char *out_img_name="image_shot";
	char c;

	printf(	"------------FILTERS------------\n"
			"1. binary\n2. inverse\n3. grayscale\n"
			"4. zero padding\n5. gauss blur\n"
			"6. laplacian edge detection\n"
			"------------FILTERS------------\n");
			
	printf("enter the filter num to apply ('any' for no filter): ");
	scanf(" %c",&c);
	
	switch( c ){
		case '1':{
			out_img_name="binary_filtered";
			im2bw(processed_buffer,processed_size);
			break;
		}
		
		case '2':{
			out_img_name="inverse_filtered";
			im2inverse(processed_buffer,processed_size);
			break;
		}
		
		case '3':{
			out_img_name="grayscale_filtered";
			im2gray(processed_buffer,processed_size);
			break;
		}
		case '4':{
			out_img_name="zero_padded";
			
			uint8_t *buffer_;
			int pad_w=width>>3;
			int pad_h=height>>3;
			buffer_=pad(processed_buffer,width,height, pad_h, pad_w, pad_h, pad_w);//sample zero padding
			
			free(processed_buffer);
			processed_buffer=buffer_;
			
			width+=2*pad_w;
			height+=2*pad_h;
			processed_size=width*height*3;
			break;
		}
		case '5':{
		clock_t t=clock();
			out_img_name="gauss_blur";
			
			uint8_t *buffer_;
			
			int fsize=11;
			buffer_=gaussBlur(processed_buffer,width,height, fsize, 5.0);//sample zero padding

			free(processed_buffer);
			processed_buffer=buffer_;
			
			width=width-fsize+1;
			height=height-fsize+1;
			processed_size=width*height*3;
			t=clock()-t;
			printf ("gauss blurring process took %ld clicks (%f seconds).\n",t,((float)t)/CLOCKS_PER_SEC);
			break;
		}
		case '6':{
		clock_t t=clock();
			out_img_name="laplace_edge";
			
			//1. grayscale
			im2gray(processed_buffer,processed_size);
			
			//2. blur to decrease noise
			uint8_t *buffer_; int fsize=11;
			buffer_=gaussBlur(processed_buffer,width,height, fsize, 1.0);//sample zero padding
			
			width=width-fsize+1;
			height=height-fsize+1;
			processed_size=width*height*3;
			
			float *f_buffer_;
			//3. detect edges
			f_buffer_=laplacian(buffer_,width,height);
			free(buffer_);
			
			//4. normalize laplace filtered image
			width=width-2;
			height=height-2;
			processed_size=width*height*3;
			
			buffer_=normalize(f_buffer_,width,height);
			
			free(processed_buffer);
			processed_buffer=buffer_;
			
			t=clock()-t;
			printf ("laplace process took %ld clicks (%f seconds).\n",t,((float)t)/CLOCKS_PER_SEC);
			break;
		}
		default:{
			printf("default:no filter\n");
			break;
		}
	}

	//save2ppm( processed_buffer, processed_size, width, height,out_img_name );
	save2jpeg( processed_buffer, processed_size, width, height,out_img_name,90 );
	//printf("End of decompression\n");
	free( processed_buffer );
	return 0;
}

