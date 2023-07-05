#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include<string.h>
#include <jpeglib.h>	//to decode jpg image buffer
#include<time.h>		//timestamp for imagenames
#include <math.h>		//pow() func; M_PI, M_E constants
#include "../include/filters.h"//function definitions of this file

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
	
	printf("min:%f max:%f\n",min,max);
	
	for(int i=0; i<width*height*3; i++){
		new_buffer[i]=(buffer[i]-min)*(newMax-newMin)/(max-min)+newMin;
	}

	return new_buffer;
}

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
	return apply;//return processed image
	
}

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

float* conv2d(uint8_t *buffer,int width,int height,float* kernel,int ksize){
	/*convoles the buffer with kernel, then returns address of filtered image
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
//pad the image and return the address of padded buffer
uint8_t* gaussBlur(uint8_t *buffer,int width,int height,int fsize,float fsigma){
	
	uint8_t *apply;//new array for filtered image
    float   *kernel;//our kernel to calculate distribution ratio
    int sizeReducted=(fsize-1);
	
    kernel=gaussfilt2d(fsize,fsigma);//(float*)malloc(fsize*fsize*sizeof(float));
    
    //here we normalizing result of float* to uint8*
    apply=normalize(conv2d(buffer,width, height, kernel, fsize)  ,width-sizeReducted, height-sizeReducted);
	
	return apply;//return processed image
}
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
			out_img_name="gauss_blur";
			
			uint8_t *buffer_;
			
			int fsize=11;
			buffer_=gaussBlur(processed_buffer,width,height, fsize, 5.0);//sample zero padding

			free(processed_buffer);
			processed_buffer=buffer_;
			
			width=width-fsize+1;
			height=height-fsize+1;
			processed_size=width*height*3;
			break;
		}
		case '6':{
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

