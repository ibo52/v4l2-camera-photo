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
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define CLEAR(x) memset(&(x), 0, sizeof(x)) //write zero to the struct space

typedef struct buff_{
	intptr_t* address;
	size_t length;
}buff_;

typedef struct camera{
	char *name;				//camera path, default:/dev/video0 on main
	int fd;					//file descriptor of camera file
	buff_ *buffer;			//cam output buffer that concatenated to mmap
	int IO_METHOD;			//mmap, userptr or R/W
	uint8_t IS_ACTIVE;		//return if camera active to send image data
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
struct v4l2_frmsize_discrete USER_FRAME_SIZE;//user defined width and height frames for camera

int xioctl(int fh, int request, void *arg){
    int r;

    do {
            r = ioctl(fh, request, arg);
    } while (-1 == r && EINTR == errno);

    return r;
}

int get_frameSize(int index){
	CLEAR(frame_size);
	frame_size.type=V4L2_FRMSIZE_TYPE_DISCRETE;
	frame_size.index= index;
	frame_size.pixel_format=fmt_desc.pixelformat;		//calculate frame sizes for given pixel format
	
	if (-1 != xioctl(Camera.fd, VIDIOC_ENUM_FRAMESIZES, &frame_size)){

			//printf("\t\tFrame Size:%u %u\n", frame_size.discrete.width, frame_size.discrete.height);
			//frame_size.index++;
			return 0;
	}else{
		if (errno!=EINVAL)
			perror(ANSI_COLOR_YELLOW"VIDIOC_ENUM_FRAMESIZES"ANSI_COLOR_RESET);
		return -1;
	}
	
	return 0;	
}

static int get_format(int fmt_description_index){	//if 1: get all formats; else get first format
	CLEAR(fmt_desc);
	fmt_desc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt_desc.index=fmt_description_index;
	
	if (-1 != xioctl(Camera.fd, VIDIOC_ENUM_FMT, &fmt_desc) ){

			//printf("Format:%c%c%c%c(%s)\n", fmt_desc.pixelformat&0xff, (fmt_desc.pixelformat>>8)&0xff, (fmt_desc.pixelformat>>16)&0xff, (fmt_desc.pixelformat>>24)&0xff, fmt_desc.description);
			get_frameSize(0);

			return 0;
	}else{
		perror(ANSI_COLOR_YELLOW"VIDIOC_ENUM_FMT"ANSI_COLOR_RESET);
		return -1;
	}	
}

static int get_fps(){	//if 1: get parameters to 'struct param' (fps denominator numerator etc.)
	struct v4l2_streamparm param;
	CLEAR(param);
	param.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	
	if (-1 == xioctl(Camera.fd, VIDIOC_G_PARM, &param)) {
    	perror(ANSI_COLOR_YELLOW"VIDIOC_G_PARM"ANSI_COLOR_RESET);
   		return -1;
	}
	printf(ANSI_COLOR_YELLOW"Camera Supports %i frames per %i seconds\n"ANSI_COLOR_RESET, param.parm.capture.timeperframe.denominator , param.parm.capture.timeperframe.numerator);
	return 0;		
}

void camera__control__set(int ctrl_id, int val){
/*
*	Sets values for supported controls defined in 'struıct queryctrl'
*/
	struct v4l2_queryctrl queryctrl;
	struct v4l2_control control;
	
	CLEAR(queryctrl);CLEAR(control);
	queryctrl.id = ctrl_id;
	
	if (-1 == xioctl(Camera.fd, VIDIOC_QUERYCTRL, &queryctrl)) {
			if (errno != EINVAL) {
				perror(ANSI_COLOR_RED"VIDIOC_QUERYCTRL"ANSI_COLOR_RESET);
				exit(EXIT_FAILURE);
			} else {
				printf("Control \'%08x\' is NOT supported!\n", ctrl_id);
			}
	
	} else if ( (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) ) {
    		printf("Control \'%08x\' is"ANSI_COLOR_RED"NOT supported!\n"ANSI_COLOR_RESET, ctrl_id);
	
	}else if ( (queryctrl.flags & V4L2_CTRL_FLAG_INACTIVE) ) {
    		printf("Control \'%08x\' is"ANSI_COLOR_RED" inactive.\n"ANSI_COLOR_RESET, ctrl_id);
	
	}else if ((queryctrl.flags & V4L2_CTRL_FLAG_READ_ONLY) ) {
    		printf("Control \'%08x\' is"ANSI_COLOR_RED" read only.\n"ANSI_COLOR_RESET, ctrl_id);
	
	}else if ((queryctrl.flags & V4L2_CTRL_FLAG_GRABBED) ) {
    		printf("Control \'%08x\'is"ANSI_COLOR_YELLOW"temporarily unchangeable.\n"ANSI_COLOR_RESET, ctrl_id);
	
	} else {

			control.id = ctrl_id;
			control.value = val;//queryctrl.default_value;
			//printf("queryctrl value set to %i\n",control.value);
			
			if (-1 == xioctl(Camera.fd, VIDIOC_S_CTRL, &control)) {
				fprintf(stdout, ANSI_COLOR_YELLOW"VIDIOC_S_CTRL(Control ID: %08x)"ANSI_COLOR_RESET": %s\n", ctrl_id, strerror(errno));
				if(errno!=EBUSY)
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
    	perror(ANSI_COLOR_YELLOW"VIDIOC_QUERYMENU:menu index is more than maximum"ANSI_COLOR_RESET);
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
		perror(ANSI_COLOR_RED"VIDIOC_QUERYCTRL"ANSI_COLOR_RESET);
		exit(errno);
	}
	else if (retval==-1){
		queryctrl.id=V4L2_CID_BASE;
	}

	return retval;		
}
//static void set_streaming(int);
static void set_format(int width,int height){
	//Custom camera resolution
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	
	if(  (width | height)  ){
                fmt.fmt.pix.width       = width;
                fmt.fmt.pix.height      = height;
                fmt.fmt.pix.pixelformat = frame_size.pixel_format;//V4L2_PIX_FMT_RGB24;//V4L2_PIX_FMT_MJPEG
                //fmt.fmt.pix.field       = pixfield;//V4L2_FIELD_INTERLACED;


    }else{		//Set default best settings by querying device
    			fmt.fmt.pix.width       = frame_size.discrete.width;
                fmt.fmt.pix.height      = frame_size.discrete.height;
                fmt.fmt.pix.pixelformat = frame_size.pixel_format;//V4L2_PIX_FMT_RGB24;//V4L2_PIX_FMT_MJPEG

    }
    //get default camera formats to fmt struct
	if (-1 == xioctl(Camera.fd, VIDIOC_S_FMT, &fmt)){
		perror(ANSI_COLOR_YELLOW"Set Pixel Format(VIDIOC_S_FMT)"ANSI_COLOR_RESET);
		if( errno!=EBUSY )
			exit( errno );
		}
    /* Buggy driver paranoia. */
        unsigned int min = fmt.fmt.pix.width * 2;
        if (fmt.fmt.pix.bytesperline < min)
                fmt.fmt.pix.bytesperline = min;
        min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
        if (fmt.fmt.pix.sizeimage < min)
                fmt.fmt.pix.sizeimage = min;
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
						fprintf(stdout,ANSI_COLOR_RED"%s is not a V4L2 device: errno%d:%s\n"ANSI_COLOR_RESET ,Camera.name, errno, strerror(errno));
				    	exit(errno);
				    	
					default:
						perror(ANSI_COLOR_RED"VIDIOC_QUERYCAP"ANSI_COLOR_RESET);
				    	exit(errno);
					
			}

    }//check if device is a video camera
    if (!(caps.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
                fprintf(stderr, ANSI_COLOR_RED"%s is not a video capture device\n"ANSI_COLOR_RESET,Camera.name);
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
                perror(ANSI_COLOR_YELLOW"Cropping is not supported"ANSI_COLOR_RESET);
                break;
                
            default:
                perror(ANSI_COLOR_YELLOW"VIDIOC_S_CROP error: (errors ignored)"ANSI_COLOR_RESET);
                break;
            }
        }
        //printf("cropcap bounds: %d %d %d %d \n", cropcap.bounds.left, cropcap.bounds.top, cropcap.bounds.width, cropcap.bounds.height);
    } else {
        perror(ANSI_COLOR_YELLOW"VIDIOC_CROPCAP error(errors ignored):"ANSI_COLOR_RESET);
    }

    /*request FORMAT SETTİNGS */
    //https://www.kernel.org/doc/html/v4.11/media/uapi/v4l/vidioc-cropcap.html#c.v4l2_cropcap
    
    return 0;
}

static int init_mmap(int fd)
{
	Camera.IO_METHOD=V4L2_MEMORY_MMAP;
	
    CLEAR(req);
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req))
    {
    	switch(errno){
    	
    		case EINVAL:
    			fprintf(stderr, ANSI_COLOR_RED"%s does not support memory mapping I/O."ANSI_COLOR_RED" Errno:%d->%s\n"ANSI_COLOR_RESET ,Camera.name, errno, strerror(errno));
    			exit(errno);
    			
    		default:
    			perror(ANSI_COLOR_RED"Requesting memory buffer"ANSI_COLOR_RESET);
        		exit(errno);
    	}
    }
    
    if(req.count < 4){
    	fprintf(stdout, ANSI_COLOR_YELLOW"Insufficient buffer count(%i) on %s for queueing.\n", req.count, Camera.name);	
    	if(req.count>0)
    		fprintf(stdout, "Program will try to run %s with this number of buffer\n"ANSI_COLOR_RESET, Camera.name);
    	else
    		exit(-1);
    }
    
    Camera.buffer=calloc(req.count, sizeof(buff_));
    
    for (int i=0; i<req.count; i++){
		struct v4l2_buffer buf;
		
		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		
		if(-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf)){
		    perror(ANSI_COLOR_RED"VIDIOC_QUERYBUF"ANSI_COLOR_RESET);
		    exit(errno);
		}
		
		cam_buf.bytesused=buf.length;
		Camera.buffer[i].length=buf.length;
		Camera.buffer[i].address = mmap (NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
		
		if (MAP_FAILED == Camera.buffer ){
			perror(ANSI_COLOR_RED"mmap"ANSI_COLOR_RESET);
			exit(errno);
		}
	}
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
static int ready_to_capture(){
/*
*	Adjust the camera buffers and sets camera for streaming
*/
    for(int i=0; i<req.count; i++){
			//clear the struct which keep image
			CLEAR(cam_buf);
			
			cam_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			cam_buf.memory =Camera.IO_METHOD;
    		cam_buf.index = i;
			/*
			if(Camera.IO_METHOD==V4L2_MEMORY_USERPTR)//additional settings for userptr method
			{
					cam_buf.m.userptr=Camera.buffer;
					cam_buf.length=fmt.fmt.pix.sizeimage;
			}
			*/
			if(-1 == xioctl(Camera.fd, VIDIOC_QBUF, &cam_buf))
			{//queue buffers to fill camera data
				perror(ANSI_COLOR_RED"VIDIOC_QBUF"ANSI_COLOR_RESET);
				exit(errno);
			}
	}
	
    if(-1 == xioctl(Camera.fd, VIDIOC_STREAMON, &cam_buf.type))
    {//switch streaming on
        perror(ANSI_COLOR_RED"VIDIOC_STREAMON"ANSI_COLOR_RESET);
        exit(errno);
    }
	Camera.IS_ACTIVE=1;
    return 0;
}
int dequeue_buff(){

	if( !Camera.IS_ACTIVE ){
		printf("camera is not active:\n");
		errno=ENODATA;
		return -1;
	}
	/**/
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(Camera.fd, &fds);

    struct timeval tv = {0};
    tv.tv_sec = 2;

    int r = select(Camera.fd+1, &fds, NULL, NULL, &tv);
    if(-1 == r)
    {
    	if (EINTR != errno){
        	perror(ANSI_COLOR_RED"select() fail"ANSI_COLOR_RESET);
        	exit(errno);
        }
    }
    if (0 == r) {
    	fprintf(stdout, "select timeout\n");
		exit(errno);
	}

	//took the frame and retrieve
    if(-1 == xioctl(Camera.fd, VIDIOC_DQBUF, &cam_buf))
    {
        perror(ANSI_COLOR_YELLOW"VIDIOC_DQBUF"ANSI_COLOR_RESET);
        return -1;
    }

	return 0;
}
void camera__print_specs(void){

	char buffer_format[4];
	int pixfmt=fmt.fmt.pix.pixelformat;
	for(int i=0; i<4;i++){
    	buffer_format[i]= pixfmt & 0xFF;
    	pixfmt>>=8;	
    };
    
    fprintf(stdout, ANSI_COLOR_YELLOW "------------------------------\n"
    		"Device Info: %s\n"
            "------------------------------\n"ANSI_COLOR_RESET
            "  Driver: \"%s\"\n"
            "  Card:   \"%s\"\n"
            "  Bus:    \"%s\"\n"
            "  Version: %u.%u.%u\n"
            "  Capabilities: %08x\n"ANSI_COLOR_YELLOW
            ,
            Camera.name,
            caps.driver,
            caps.card,
            caps.bus_info,
            (caps.version>>16)&0xff, (caps.version>>8)&0xff, caps.version&0xff,
            caps.capabilities
            );
            
            int cap=caps.capabilities;

            if( cap & V4L2_CAP_VIDEO_CAPTURE )
				fprintf(stdout, "\t--> Video Capture(single-planar API)\n");
	
			if( cap & V4L2_CAP_VIDEO_OUTPUT )
				fprintf(stdout, "\t--> Video Output(single-planar API)\n");
			
			if( cap & V4L2_CAP_VIDEO_OVERLAY )
				fprintf(stdout, "\t--> Video Overlay\n");

			if( cap & V4L2_CAP_VBI_CAPTURE )
				fprintf(stdout, "\t--> Raw VBI\n");

			if( cap & V4L2_CAP_SLICED_VBI_CAPTURE )
				fprintf(stdout, "\t--> Sliced VBI Capture\n");
			
			if( cap & V4L2_CAP_TUNER )
				fprintf(stdout, "\t--> Has tuner(s) to demodulate a RF signal\n");
			
			if( cap & V4L2_CAP_AUDIO )
				fprintf(stdout, "\t--> Audio I/O\n");
			
			if( cap & V4L2_CAP_RADIO )
				fprintf(stdout, "\t--> This is a radio recevier\n");
			
			if( cap & V4L2_CAP_MODULATOR )
				fprintf(stdout, "\t--> Has modulator(s) to emit RF signals\n");
			
			if( cap & V4L2_CAP_SDR_CAPTURE )
				fprintf(stdout, "\t--> Software Defined Radio(SDR)\n");

			if( cap & V4L2_CAP_EXT_PIX_FORMAT )
				fprintf(stdout, "\t--> Extended Pixel Format\n");
			
			if( ((cap>>16) & 0x80)==0x80)
				fprintf(stdout, "\t--> Metadata Capture\n");

			if( cap & V4L2_CAP_READWRITE )
				fprintf(stdout, "\t--> Read/Write I/O\n");
			
			if( cap & V4L2_CAP_ASYNCIO )
				fprintf(stdout, "\t--> Asynchronous I/O\n");
			
			if( cap & V4L2_CAP_STREAMING)
				fprintf(stdout, "\t--> Streaming I/O\n");
			
			if( cap & V4L2_CAP_TOUCH )
				fprintf(stdout, "\t--> Device is a Touch Device\n");
            
            fprintf(stdout, 
           ANSI_COLOR_GREEN "------------------------------\n"
            "Format properties\n"
            "------------------------------\n" ANSI_COLOR_CYAN
            "Width: 		%i\n"
            "Height:		%i\n"
            "Buffer Format:	%s(%s)\n"
            "------------------------------\n" ANSI_COLOR_RESET
            ,
            //format properties
            fmt.fmt.pix.width,
            fmt.fmt.pix.height,
            buffer_format,fmt_desc.description);
            
            get_fps(); //get fps info
}

int camera__activate(const char* device_path){
	//try to open camera as read-write mode
	CLEAR(Camera);
	
	if(device_path==NULL)//if no args; select first one
    	Camera.name="/dev/video0";
    else
    	Camera.name=(char*)device_path;//select desired camera
    	
    //open camera file
    if( ( Camera.fd = open(Camera.name, O_RDWR /* required */ | O_NONBLOCK, 0))==-1 ) {
    
        char msg[64]={'0'};
        sprintf(msg,ANSI_COLOR_RED"Could not open"ANSI_COLOR_RESET" '%s'",Camera.name);
        perror(msg);
        return Camera.fd;
    }
    
    init_camera(); 			//prepare camera by getting info
    get_format(0);			//get default format options to frame_size(for required for set_format)
    set_format(USER_FRAME_SIZE.width, USER_FRAME_SIZE.height);		//automaticly format to default best format
    camera__print_specs();
    init_mmap(Camera.fd);	//open memory map and concatenate to buffer
	ready_to_capture();		//adjust camera buffers and open streaming
	//init_userptr();
    return 0;
}

void set_streaming(int control_value){

	switch (Camera.IO_METHOD) {

        	case V4L2_MEMORY_USERPTR:
		    case V4L2_MEMORY_MMAP:
		    	if (-1 == xioctl(Camera.fd, control_value, &cam_buf.type)){
							perror(ANSI_COLOR_RED"Streaming (VIDIOC_STREAM_X)"ANSI_COLOR_RESET);
		                    exit(errno);
				}else{

					if (control_value==VIDIOC_STREAMON ){
						ready_to_capture();
						Camera.IS_ACTIVE=1;
					}
				}
		        break;
		}
}
static void close_device(void){
        if (-1 == close(Camera.fd)){
        		perror(ANSI_COLOR_RED"Camera could not closed"ANSI_COLOR_RESET);
                exit(errno);
		}
        Camera.fd = -1;
}
int camera__deactivate(){
		set_streaming(VIDIOC_STREAMOFF);
		
        switch (Camera.IO_METHOD) {
        	
		    case V4L2_MEMORY_MMAP:
		    	for (int i = 0; i < req.count; i++){
					if (-1 == munmap( Camera.buffer[i].address, Camera.buffer[i].length) ){
							perror(ANSI_COLOR_RED"Camera deactivate(munmap)"ANSI_COLOR_RESET);
							exit(errno);
					}
		    	}
		        break;
		    
		    case V4L2_MEMORY_USERPTR:
        		for (int i = 0; i < req.count; i++){
					free( Camera.buffer[i].address );
		    	}
        		break;
		}
		close_device();
		return 0;
}

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
		perror(ANSI_COLOR_RED"Could not decode."ANSI_COLOR_YELLOW" Possibly broken JPEG"ANSI_COLOR_RESET);
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
intptr_t* camera__capture(int buffer_type){
	
	intptr_t* rgbBuff;
	if( dequeue_buff() ){//io to dequeue buffer. Queueing made here(end of this function) after image processed
		rgbBuff=NULL;
		return rgbBuff;
	}

		switch (buffer_type){
		
			case V4L2_PIX_FMT_RGB24:
				rgbBuff=(intptr_t*)camera__decode_rgb( (uint8_t*)Camera.buffer[cam_buf.index].address, cam_buf.bytesused , fmt.fmt.pix.width, fmt.fmt.pix.height);
				break;
				
			default:
				rgbBuff=(intptr_t*)camera__decode_rgb( (uint8_t*)Camera.buffer[cam_buf.index].address, cam_buf.bytesused , fmt.fmt.pix.width, fmt.fmt.pix.height);
				break;
		}
		
		//resend buffer to queue, so camera can fill it up again
		if(-1 == xioctl(Camera.fd, VIDIOC_QBUF, &cam_buf))
		{
					perror(ANSI_COLOR_RED"VIDIOC_QBUF"ANSI_COLOR_RESET);
					exit(errno);
		}
    return rgbBuff;
}

char *camera__imsave(const char* name){

	char *filename=(char*)calloc(128,sizeof(char));
	
	strcat(filename,"../images/");
	write_time(filename, 0 );
	strcat(filename,name);
	
	switch(fmt.fmt.pix.pixelformat){
	
		case V4L2_PIX_FMT_MJPEG:
		case V4L2_PIX_FMT_JPEG:
			strcat(filename,".jpg");
			break;
		
		case V4L2_PIX_FMT_RGB24:
			strcat(filename,".rgb");
			break;
		
		default:
			strcat(filename,".jpg");
			break;
	}
	
	
	
	int jpg_fd;
	if(  ( jpg_fd= open(filename, O_WRONLY |O_TRUNC| O_CREAT, 0664) ) ==-1  ){
		printf("%s->",filename);
		perror(ANSI_COLOR_RED"JPEG file open"ANSI_COLOR_RESET);
		exit(jpg_fd);
	}
	
	int recv=0;
	//printf("expecting %i bytes to write\n",cam_buf.bytesused);
	do{
		recv+=write(jpg_fd, Camera.buffer[cam_buf.index].address, Camera.buffer[cam_buf.index].length);
		//printf("recv:%i\n",recv);
	}while( recv<cam_buf.bytesused) ;

	close(jpg_fd);
	//printf("write file: %s\n",filename);
	
	return filename;
	
}

