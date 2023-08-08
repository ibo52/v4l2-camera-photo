/*
 *halil ibrahim mut
 * 
 * 
 * 
 * this program using v4l2(linux/videodev2.h library) module,
 * which is comes as builtin on linux kernel.
 * this program simply takes photograph from
 * camera and save it as jpeg file.
 * 
 * Environ : Ubuntu 22.04
 * kernel  : 5.19.0-46-generic
 * 
 * Sources/references:
 * https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/capture.c.html
 * https://web.archive.org/web/20110520211256/http://v4l2spec.bytesex.org/spec/capture-example.html
 * https://jayrambhia.com/blog/capture-v4l2
 * https://lwn.net/Articles/203924/
 * https://linuxtv.org/docs.php
 */
#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>		//uint8_t
#include <stdlib.h>
#include <sys/mman.h>	//mmap
#include <jpeglib.h>	//to decode jpg image buffer
#include "filters.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define CLEAR(x) memset(&(x), 0, sizeof(x)) //write zero to the struct space

typedef struct camera{
	char *name;				//camera path, default:/dev/video0 on main
	int fd;					//file descriptor of camera file
	uint8_t *buffer;		//cam output buffer that concatenated to mmap
	int IO_METHOD;			//mmap, userptr or R/W
}camera; 

camera Camera;

struct v4l2_format fmt;					//format specs of dev
struct v4l2_fmtdesc fmt_desc;			//default format desc
struct v4l2_frmsizeenum frame_size;		//enumerate available framesizes for camera formats
struct v4l2_buffer cam_buf; 			//camera buffer took from device, we can also use *buffer to access RGB data
struct v4l2_capability caps;			//keep device info and capabilities
struct v4l2_cropcap cropcap;			//default cropping capabilities
struct v4l2_crop crop;					//set crop settings
struct v4l2_requestbuffers req;
/*
 *
 * 
 *  
*/
struct v4l2_queryctrl queryctrl;
struct v4l2_querymenu querymenu;
int xioctl(int fh, int request, void *arg){
    int r;

    do {
            r = ioctl(fh, request, arg);
    } while (-1 == r && EINTR == errno);

    return r;
}

static int get_frameSize(int pixelformat,int all_sizes){
	CLEAR(frame_size);
	frame_size.type=V4L2_FRMSIZE_TYPE_DISCRETE;
	frame_size.pixel_format=pixelformat;		//calculate frame sizes for given pixel format
	
	while (-1 != xioctl(Camera.fd, VIDIOC_ENUM_FRAMESIZES, &frame_size)){

			//printf("\t\tFrame Size:%u %u\n", frame_size.discrete.width, frame_size.discrete.height);
			if(all_sizes)
				frame_size.index++;
			else
				break;
	}
	
	return 0;	
}

static int get_format(int all_formats){	//if 1: get all formats; else get first format
	CLEAR(fmt_desc);
	fmt_desc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	
	while (-1 != xioctl(Camera.fd, VIDIOC_ENUM_FMT, &fmt_desc)){

			//printf("Format:%c%c%c%c(%s)\n", fmt_desc.pixelformat&0xff, (fmt_desc.pixelformat>>8)&0xff, (fmt_desc.pixelformat>>16)&0xff, (fmt_desc.pixelformat>>24)&0xff, fmt_desc.description);
			get_frameSize(fmt_desc.pixelformat,0);
			
			if(all_formats)
				fmt_desc.index++;
			else
				break;
	}
	
	return 0;		
}

static int get_parm(){	//if 1: get all formats; else get first format
	struct v4l2_streamparm param;
	CLEAR(param);
	param.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	
	if (-1 == xioctl(Camera.fd, VIDIOC_G_PARM, &param)) {
    	perror("VIDIOC_G_PARM");
   		return -1;
	}
	printf("Camera Supports %i frames per %i seconds\n", param.parm.capture.timeperframe.denominator , param.parm.capture.timeperframe.numerator);
	return 0;		
}

void camera__control__set(int ctrl_id, int val){
	struct v4l2_queryctrl queryctrl;
	struct v4l2_control control;
	
	CLEAR(queryctrl);CLEAR(control);
	queryctrl.id = ctrl_id;
	
	if (-1 == xioctl(Camera.fd, VIDIOC_QUERYCTRL, &queryctrl)) {
			if (errno != EINVAL) {
				perror("VIDIOC_QUERYCTRL");
				exit(EXIT_FAILURE);
			} else {
				printf("Control \'%08x\' is NOT supported!\n", ctrl_id);
			}
	
	} else if ( (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) ) {
    		printf("Control \'%08x\' is NOT supported!\n", ctrl_id);
	
	}else if ( (queryctrl.flags & V4L2_CTRL_FLAG_INACTIVE) ) {
    		printf("Control \'%08x\' is inactive.\n", ctrl_id);
	
	}else if ((queryctrl.flags & V4L2_CTRL_FLAG_READ_ONLY) ) {
    		printf("Control \'%08x\' is read only.\n", ctrl_id);
	
	}else if ((queryctrl.flags & V4L2_CTRL_FLAG_GRABBED) ) {
    		printf("Control \'%08x\'temporarily unchangeable.\n", ctrl_id);
	
	} else {

			control.id = ctrl_id;
			control.value = val;//queryctrl.default_value;
			//printf("queryctrl value set to %i\n",control.value);
			
			if (-1 == xioctl(Camera.fd, VIDIOC_S_CTRL, &control)) {
				fprintf(stdout, "VIDIOC_S_CTRL error for Ctrl id:%08x: %s\n", ctrl_id, strerror(errno));
				exit(EXIT_FAILURE);
			}
	}
}

void camera__control__enumerate_menu(){
	// Writes informations to 'struct v4l2_querymenu'.
    querymenu.id = queryctrl.id;

    if (querymenu.index <= queryctrl.maximum) {

        if ( 0 == xioctl(Camera.fd, VIDIOC_QUERYMENU, &querymenu) ) {
            //printf("%8i : %25s\n",querymenu.index, querymenu.name);
        }
    }else{
    	perror("VIDIOC_QUERYMENU:menu index is more than maximum");
    }
}

int camera__control__get_ctrl(){
	/*
	*	Get control data to queryctrl struct. queryctrl.id=V4L2_CID_BASE have to set inside caller function.
	*/
	int retval;
	
	retval=xioctl(Camera.fd, VIDIOC_QUERYCTRL, &queryctrl);
	if (0 == retval ) {
        	//values can be accessed by queryctrl struct
        	//printf("Control %s  min:%i max:%i default:%i\n",queryctrl.type, queryctrl.name, queryctrl.minimum, queryctrl.maximum, queryctrl.default_value);
        	//camera__control__set(queryctrl.id ,queryctrl.default_value);//reset to default parameters on init
        	queryctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;

	}
	else if ( errno != EINVAL ) {
		perror("VIDIOC_QUERYCTRL");
		exit(errno);
	}
	else if (retval==-1){
		queryctrl.id=V4L2_CID_BASE;
	}

	return retval;		
}

void set_format(int width,int height,int pixfmt,int pixfield){
	//Custom camera resolution
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	get_format(0);	//dump default format options to frame_size
	
	if(  (width | height| pixfmt | pixfield)!=0  ){
                fmt.fmt.pix.width       = width;
                fmt.fmt.pix.height      = height;
                fmt.fmt.pix.pixelformat = pixfmt;//V4L2_PIX_FMT_RGB24;//V4L2_PIX_FMT_MJPEG
                fmt.fmt.pix.field       = pixfield;//V4L2_FIELD_INTERLACED;
    

				if (-1 == xioctl(Camera.fd, VIDIOC_S_FMT, &fmt)){
					perror("Setting Pixel Format");
					exit(errno);
				}
    }else{		//Set default best settings by querying device
    			fmt.fmt.pix.width       = frame_size.discrete.width;
                fmt.fmt.pix.height      = frame_size.discrete.height;
                fmt.fmt.pix.pixelformat = frame_size.pixel_format;//V4L2_PIX_FMT_RGB24;//V4L2_PIX_FMT_MJPEG

				//get default camera formats to fmt struct
				if (-1 == xioctl(Camera.fd, VIDIOC_S_FMT, &fmt)){
					perror("Get Default Pixel Format");
					if(errno==EBUSY){
						printf("retry fmt set\n");
					}else{
					exit( errno );}
				}
    }get_parm();
    //printf("Display resolution formatted to: %dx%d\n",fmt.fmt.pix.width,fmt.fmt.pix.height);
}

static int init_camera(){

    CLEAR(cropcap);	//clear data structs
    CLEAR(fmt);
    CLEAR(crop);
    CLEAR(caps);

    //request capabilities of device
    if (-1 == xioctl(Camera.fd, VIDIOC_QUERYCAP, &caps) ) {
			switch(errno){
					case EINVAL:
						fprintf(stdout,"%s is not a V4L2 device: errno%d:%s\n",Camera.name, errno, strerror(errno));
				    	exit(errno);
				    	
					default:
						perror("VIDIOC_QUERYCAP");
				    	exit(errno);
					
			}

    }//check if device is a video camera
    if (!(caps.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
                fprintf(stderr, "%s is not a video capture device\n",Camera.name);
                exit(errno);
	}
	/*
	*/
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    //request crop capabilities(width,heigth,frame pos etc..)
    if (0 == xioctl(Camera.fd, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */

        if (-1 == xioctl(Camera.fd, VIDIOC_S_CROP, &crop)) {
            switch (errno) {
            case EINVAL:
                perror("Cropping is not supported");
                break;
                
            default:
                perror("VIDIOC_S_CROP error: (errors ignored)");
                break;
            }
        }
        //printf("cropcap bounds: %d %d %d %d \n", cropcap.bounds.left, cropcap.bounds.top, cropcap.bounds.width, cropcap.bounds.height);
    } else {
        perror("VIDIOC_CROPCAP error(errors ignored):");
    }

    /*request FORMAT SETTİNGS */
    //https://www.kernel.org/doc/html/v4.11/media/uapi/v4l/vidioc-cropcap.html#c.v4l2_cropcap
    set_format(0,0,0,0);//automaticly format to default best format
    return 0;
}

static int init_mmap(int fd)
{
    CLEAR(req);
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req))
    {
    	switch(errno){
    	
    		case EINVAL:
    			fprintf(stderr, "%s does not support memory mapping I/O. Errno:%d->%s\n",Camera.name, errno, strerror(errno));
    			exit(errno);
    			
    		default:
    			perror("Requesting memory buffer");
        		exit(errno);
    	}
    }
    
    struct v4l2_buffer buf;
    CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;
    if(-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
    {
        perror("VIDIOC_QUERYBUF");
        exit(errno);
    }
    Camera.IO_METHOD=V4L2_MEMORY_MMAP;
    cam_buf.bytesused=buf.length;
    Camera.buffer = mmap (NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

    return 0;
}
/*
static int init_userptr()
{
    CLEAR(req);
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_USERPTR;

    if (-1 == xioctl(Camera.fd, VIDIOC_REQBUFS, &req))
    {
    	switch(errno){
    	
    		case EINVAL:
    			fprintf(stderr, "%s does not support user pointer I/O. Errno:%d->%s\n",Camera.name, errno, strerror(errno));
    			exit(errno);
    			
    		default:
    			perror("VIDIOC_REQBUFS");
        		exit(errno);
    	}
    }

    Camera.IO_METHOD=V4L2_MEMORY_USERPTR;
    Camera.buffer = malloc (fmt.fmt.pix.sizeimage);

    return 0;
}*/
static int ready_to_capture(int fd)
{
    //clear the struct which keep image
    CLEAR(cam_buf);
    
    cam_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    cam_buf.memory = Camera.IO_METHOD;
    cam_buf.index = 0;
	/*
    if(Camera.IO_METHOD==V4L2_MEMORY_USERPTR)//additional settings for userptr method
    {
    		cam_buf.m.userptr=Camera.buffer;
    		cam_buf.length=fmt.fmt.pix.sizeimage;
    }
    */
    if(-1 == xioctl(fd, VIDIOC_QBUF, &cam_buf))
    {
        perror("VIDIOC_QBUF");
        exit(errno);
    }

    if(-1 == xioctl(fd, VIDIOC_STREAMON, &cam_buf.type))
    {
        perror("VIDIOC_STREAMON");
        exit(errno);
    }
	/**/
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    
    struct timeval tv = {0};
    tv.tv_sec = 2;
    
    int r = select(fd+1, &fds, NULL, NULL, &tv);
    if(-1 == r)
    {
    	if (EINTR == errno)
    		perror("select");
        perror("select() fail");
        exit(errno);
    }
    if (0 == r) {
    	perror("select timeout");
		exit(errno);
	}

	//took the frame and retrieve
    if(-1 == xioctl(fd, VIDIOC_DQBUF, &cam_buf))
    {
        perror("vidioc Retrieve Frame");
        return 1;
    }

    return 0;
}
void print_specs(){

	char buffer_format[4];
	int pixfmt=fmt.fmt.pix.pixelformat;
	for(int i=0; i<4;i++){
    	buffer_format[i]= pixfmt & 0xFF;
    	pixfmt>>=8;	
    };
    
    printf(ANSI_COLOR_YELLOW "------------------------------\n"
    		"Device Info: %s\n"
            "------------------------------\n"ANSI_COLOR_RESET
            "  Driver: \"%s\"\n"
            "  Card:   \"%s\"\n"
            "  Bus:    \"%s\"\n"
            "  Version: %u.%u.%u\n"
            "  Capabilities: %08x\n"
           ANSI_COLOR_GREEN "------------------------------\n"
            "Format properties\n"
            "------------------------------\n" ANSI_COLOR_CYAN
            "Width: 		%i\n"
            "Height:		%i\n"
            "Buffer Format:	%s(%s)\n"
            "------------------------------\n" ANSI_COLOR_RESET
            ,
            Camera.name,
            caps.driver,
            caps.card,
            caps.bus_info,
            (caps.version>>16)&0xff, (caps.version>>8)&0xff, caps.version&0xff,
            caps.capabilities,
            //format properties
            fmt.fmt.pix.width,
            fmt.fmt.pix.height,
            buffer_format,fmt_desc.description);
}

int get_arg_opts(int argc,char **argv){
	int i;
	if(argc==1){
           return 1;//let interpret as exit silently
        }
	for(i=1;i<argc;i++){
	
		if(argv[i][0]=='-'){
			
			switch(argv[i][1]){
				case 'i': print_specs();break;
				case 'h': printf("%s [options]\n"
                "-h :print this and exit\n"
                "-i :device information\n"
                ,argv[0]);exit(0);
			}
		}
	}
        return 0;
}

int camera__activate(){
	//try to open camera as read-write mode
    Camera.name="/dev/video0";
    
    if( ( Camera.fd = open(Camera.name, O_RDWR /* required */ | O_NONBLOCK, 0))==-1 ) {
    
        char msg[64]={'0'};
        sprintf(msg,"Could not open '%s'",Camera.name);
        perror(msg);
        return Camera.fd;
    }

    init_camera(); //prepare camera by getting info
    print_specs();
    init_mmap(Camera.fd);	//open memory map and concatenate to buffer
    ready_to_capture(Camera.fd);
	//init_userptr();
    return 0;
}

/*
int camera__deactivate(){

        switch (Camera.IO_METHOD) {
		    case V4L2_MEMORY_MMAP:
		    	if (-1 == xioctl(Camera.fd, VIDIOC_STREAMOFF, &cam_buf.type)){
		    			perror("stop streaming(VIDIOC_STREAMOFF)");
                        exit(errno);
                }
		    	if (-1 == munmap(Camera.buffer, cam_buf.bytesused)){
						perror("Camera deactivate munmap");
						exit(errno);
		    	}
		        break;
		}
		return 0;
}
static void close_device(void)
{
        if (-1 == close(Camera.fd)){
        		perror("Camera could not closed");
                exit(errno);
		}
        Camera.fd = -1;
}
*/
uint8_t* camera__decode_rgb(unsigned char *buffer,int buffsize,int width,int height) {
	/*Decode JPEG data of camera to RAW RGB*/
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
		perror("Could not decode. Possibly broken JPEG");
		exit(-1);
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

	return processed_buffer;
	
}
uint8_t* camera__get_RGB_buff(){
	ready_to_capture(Camera.fd);
    uint8_t *rgbBuff=camera__decode_rgb(Camera.buffer, cam_buf.bytesused , fmt.fmt.pix.width, fmt.fmt.pix.height);
    
    return rgbBuff;
}

char *camera__dump_buffer_to_file(const char* name){

	char *filename=(char*)calloc(128,sizeof(char));
	
	strcat(filename,"../images/");
	write_time(filename, 0 );
	strcat(filename,name);
	strcat(filename,".jpg");
	
	int jpg_fd;
	if(  ( jpg_fd= open(filename, O_WRONLY |O_TRUNC| O_CREAT, 0664) ) ==-1  ){
		printf("%s->",filename);
		perror("file open");
		exit(jpg_fd);
	}
	
	int recv=0;
	//printf("expecting %i bytes to write\n",cam_buf.bytesused);
	do{
		recv+=write(jpg_fd, Camera.buffer, cam_buf.bytesused);
		//printf("recv:%i\n",recv);
	}while( recv<cam_buf.bytesused) ;

	close(jpg_fd);
	//printf("write file: %s\n",filename);
	
	return filename;
	
}

