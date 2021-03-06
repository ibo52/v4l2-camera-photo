#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include<string.h>

#include <jpeglib.h>

#include<time.h>

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

	// Variables for the output buffer, and how long it is
	unsigned long processed_size;
	unsigned char *processed_buffer;


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
	char *out_img_name="image_shot";
	char c;
	
	printf(	"------------FILTERS------------\n"
			"1. binary\n2. inverse\n3. grayscale\n"
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
		default:{
			printf("default:no filter\n");
			break;
		}
	}

	save2ppm( processed_buffer, processed_size, width, height,out_img_name );
	save2jpeg( processed_buffer, processed_size, width, height,out_img_name,90 );
	//printf("End of decompression\n");
	free( processed_buffer );
	return 0;
}
