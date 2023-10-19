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
#include <stdint.h>		//uint8_t
#include <unistd.h>  //open, close
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

typedef struct __buff{
	intptr_t* address;
	size_t length;
}__buff;

typedef struct __specs{
	struct v4l2_format fmt;					//format specs of dev
	struct v4l2_fmtdesc fmt_desc;			//default format desc
	struct v4l2_frmsizeenum frame_size;		//enumerate available framesizes for camera formats
	struct v4l2_buffer cam_buf; 			//camera buffer took from device, we can also use *buffer to access RGB data
	struct v4l2_capability caps;			//keep device info and capabilities
	struct v4l2_cropcap cropcap;			//default cropping capabilities
	struct v4l2_crop crop;					//set crop settings
	struct v4l2_requestbuffers req;
	
	struct v4l2_queryctrl queryctrl;
	struct v4l2_querymenu querymenu;
	struct v4l2_frmsize_discrete USER_FRAME_SIZE;//user defined width and height frames for camera
}__specs;

typedef struct _CameraObject{
	char *name;				//camera path, default:/dev/video0 on main
	int fd;					//file descriptor of camera file
	__buff *buffer;			//cam output buffer that concatenated to mmap
	int IO_METHOD;			//mmap, userptr or R/W
	uint8_t IS_ACTIVE;		//return if camera active to send image data
	__specs specs;
}CameraObject; 

/*
 *
 * 
 *  
*/

static int xioctl(int fd, int request, void *arg){
    int r;

    do {
            r = ioctl(fd, request, arg);
    } while (-1 == r && EINTR == errno);

    return r;
}

int get_frameSize(CameraObject* self, int index){
	CLEAR(self->specs.frame_size);
	
	self->specs.frame_size.type=V4L2_FRMSIZE_TYPE_DISCRETE;
	self->specs.frame_size.index= index;
	self->specs.frame_size.pixel_format=self->specs.fmt_desc.pixelformat;		//calculate frame sizes for given pixel format
	
	if (-1 != xioctl(self->fd, VIDIOC_ENUM_FRAMESIZES, &self->specs.frame_size)){

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

static int get_format(CameraObject* self, int fmt_description_index){	//if 1: get all formats; else get first format
	CLEAR(self->specs.fmt_desc);
	self->specs.fmt_desc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	self->specs.fmt_desc.index=fmt_description_index;
	
	if (-1 != xioctl(self->fd, VIDIOC_ENUM_FMT, &self->specs.fmt_desc) ){

			//printf("Format:%c%c%c%c(%s)\n", fmt_desc.pixelformat&0xff, (fmt_desc.pixelformat>>8)&0xff, (fmt_desc.pixelformat>>16)&0xff, (fmt_desc.pixelformat>>24)&0xff, fmt_desc.description);
			get_frameSize(self, 0);

			return 0;
	}else{
		perror(ANSI_COLOR_YELLOW"VIDIOC_ENUM_FMT"ANSI_COLOR_RESET);
		return -1;
	}	
}

static int camera__get_fps(CameraObject* self){	//if 1: get parameters to 'struct param' (fps denominator numerator etc.)
	struct v4l2_streamparm param;
	CLEAR(param);
	param.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	
	if (-1 == xioctl(self->fd, VIDIOC_G_PARM, &param)) {
    	perror(ANSI_COLOR_YELLOW"VIDIOC_G_PARM"ANSI_COLOR_RESET);
   		return -1;
	}
	printf(ANSI_COLOR_YELLOW"Camera Supports %i frames per %i seconds\n"ANSI_COLOR_RESET, param.parm.capture.timeperframe.denominator , param.parm.capture.timeperframe.numerator);
	return 0;		
}

void camera__control__set(CameraObject* self, int ctrl_id, int val){
/*
*	Sets values for supported controls defined in 'struıct queryctrl'
*/
	struct v4l2_queryctrl queryctrl;
	struct v4l2_control control;
	
	CLEAR(queryctrl);CLEAR(control);
	queryctrl.id = ctrl_id;
	
	if (-1 == xioctl(self->fd, VIDIOC_QUERYCTRL, &queryctrl)) {
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
			
			if (-1 == xioctl(self->fd, VIDIOC_S_CTRL, &control)) {
				fprintf(stdout, ANSI_COLOR_YELLOW"VIDIOC_S_CTRL(Control ID: %08x)"ANSI_COLOR_RESET": %s\n", ctrl_id, strerror(errno));
				if(errno!=EBUSY)
					exit(EXIT_FAILURE);
			}
	}
}

void camera__control__enumerate_menu(CameraObject* self){
	// Writes informations to 'struct v4l2_querymenu'.
    self->specs.querymenu.id = self->specs.queryctrl.id;

    if (self->specs.querymenu.index <= self->specs.queryctrl.maximum) {

        if ( 0 == xioctl(self->fd, VIDIOC_QUERYMENU, &self->specs.querymenu) ) {
            //printf("%8i : %25s\n",querymenu.index, querymenu.name);
        }
    }else{
    	perror(ANSI_COLOR_YELLOW"VIDIOC_QUERYMENU:menu index is more than maximum"ANSI_COLOR_RESET);
    }
}

int camera__control__get_ctrl(CameraObject* self){
	/*
	*	Get control data to queryctrl struct. queryctrl.id=V4L2_CID_BASE have to set inside caller function.
	*/
	int retval;
	
	retval=xioctl(self->fd, VIDIOC_QUERYCTRL, &self->specs.queryctrl);
	if (0 == retval ) {
        	//values can be accessed by queryctrl struct
        	//printf("Control %s  min:%i max:%i default:%i\n",queryctrl.type, queryctrl.name, queryctrl.minimum, queryctrl.maximum, queryctrl.default_value);
        	//camera__control__set(queryctrl.id ,queryctrl.default_value);//reset to default parameters on init
        	self->specs.queryctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;

	}
	else if ( errno != EINVAL ) {
		perror(ANSI_COLOR_RED"VIDIOC_QUERYCTRL"ANSI_COLOR_RESET);
		exit(errno);
	}
	else if (retval==-1){
		self->specs.queryctrl.id=V4L2_CID_BASE;
	}

	return retval;		
}
//static void set_streaming(int);
static void set_format(CameraObject* self, int width,int height){
	//Custom camera resolution
	self->specs.fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	
	if(  (width | height)  ){
                self->specs.fmt.fmt.pix.width       = width;
                self->specs.fmt.fmt.pix.height      = height;
                self->specs.fmt.fmt.pix.pixelformat = self->specs.frame_size.pixel_format;//V4L2_PIX_FMT_RGB24;//V4L2_PIX_FMT_MJPEG
                //fmt.fmt.pix.field       = pixfield;//V4L2_FIELD_INTERLACED;


    }else{		//Set default best settings by querying device
    			self->specs.fmt.fmt.pix.width       = self->specs.frame_size.discrete.width;
                self->specs.fmt.fmt.pix.height      = self->specs.frame_size.discrete.height;
                self->specs.fmt.fmt.pix.pixelformat = self->specs.frame_size.pixel_format;//V4L2_PIX_FMT_RGB24;//V4L2_PIX_FMT_MJPEG

    }
    //get default camera formats to fmt struct
	if (-1 == xioctl(self->fd, VIDIOC_S_FMT, &self->specs.fmt)){
		perror(ANSI_COLOR_YELLOW"Set Pixel Format(VIDIOC_S_FMT)"ANSI_COLOR_RESET);
		if( errno!=EBUSY )
			exit( errno );
		}
    /* Buggy driver paranoia. */
        unsigned int min = self->specs.fmt.fmt.pix.width * 2;
        if (self->specs.fmt.fmt.pix.bytesperline < min)
                self->specs.fmt.fmt.pix.bytesperline = min;
        min = self->specs.fmt.fmt.pix.bytesperline * self->specs.fmt.fmt.pix.height;
        if (self->specs.fmt.fmt.pix.sizeimage < min)
                self->specs.fmt.fmt.pix.sizeimage = min;
    //printf("Display resolution formatted to: %dx%d\n",fmt.fmt.pix.width,fmt.fmt.pix.height);
}

static int init_camera(CameraObject* self){

    CLEAR(self->specs.cropcap);	//clear data structs
    CLEAR(self->specs.fmt);
    CLEAR(self->specs.crop);
    CLEAR(self->specs.caps);

    //request capabilities of device
    if (-1 == xioctl(self->fd, VIDIOC_QUERYCAP, &self->specs.caps) ) {
			switch(errno){
					case EINVAL:
						fprintf(stdout,ANSI_COLOR_RED"%s is not a V4L2 device: errno%d:%s\n"ANSI_COLOR_RESET ,self->name, errno, strerror(errno));
				    	exit(errno);
				    	
					default:
						perror(ANSI_COLOR_RED"VIDIOC_QUERYCAP"ANSI_COLOR_RESET);
				    	exit(errno);
					
			}

    }//check if device is a video camera
    if (!(self->specs.caps.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
                fprintf(stderr, ANSI_COLOR_RED"%s is not a video capture device\n"ANSI_COLOR_RESET,self->name);
                exit(errno);
	}
	/*
	*/
    self->specs.cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    //request crop capabilities(width,heigth,frame pos etc..)
    if (0 == xioctl(self->fd, VIDIOC_CROPCAP, &self->specs.cropcap)) {
        self->specs.crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        self->specs.crop.c = self->specs.cropcap.defrect; /* reset to default */

        if (-1 == xioctl(self->fd, VIDIOC_S_CROP, &self->specs.crop)) {
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

static int init_mmap(CameraObject* self)
{
	self->IO_METHOD=V4L2_MEMORY_MMAP;
	
    CLEAR(self->specs.req);
    self->specs.req.count = 4;
    self->specs.req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    self->specs.req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(self->fd, VIDIOC_REQBUFS, &self->specs.req))
    {
    	switch(errno){
    	
    		case EINVAL:
    			fprintf(stderr, ANSI_COLOR_RED"%s does not support memory mapping I/O."ANSI_COLOR_RED" Errno:%d->%s\n"ANSI_COLOR_RESET , self->name, errno, strerror(errno));
    			exit(errno);
    			
    		default:
    			perror(ANSI_COLOR_RED"Requesting memory buffer"ANSI_COLOR_RESET);
        		exit(errno);
    	}
    }
    
    if(self->specs.req.count < 4){
    	fprintf(stdout, ANSI_COLOR_YELLOW"Insufficient buffer count(%i) on %s for queueing.\n", self->specs.req.count, self->name);	
    	if(self->specs.req.count>0)
    		fprintf(stdout, "Program will try to run %s with this number of buffer\n"ANSI_COLOR_RESET, self->name);
    	else
    		exit(-1);
    }
    
    self->buffer=calloc(self->specs.req.count, sizeof(__buff));
    
    for (int i=0; i < self->specs.req.count; i++){
		struct v4l2_buffer buf;
		
		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		
		if(-1 == xioctl(self->fd, VIDIOC_QUERYBUF, &buf)){
		    perror(ANSI_COLOR_RED"VIDIOC_QUERYBUF"ANSI_COLOR_RESET);
		    exit(errno);
		}
		
		self->specs.cam_buf.bytesused=buf.length;
		self->buffer[i].length=buf.length;
		self->buffer[i].address = mmap (NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, self->fd, buf.m.offset);
		
		if (MAP_FAILED == self->buffer ){
			perror(ANSI_COLOR_RED"Memory Mapping failed"ANSI_COLOR_RESET);
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
static int ready_to_capture(CameraObject* self){
/*
*	Adjust the camera buffers and sets camera for streaming
*/
    for(int i=0; i<self->specs.req.count; i++){
			//clear the struct which keep image
			CLEAR(self->specs.cam_buf);
			
			self->specs.cam_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			self->specs.cam_buf.memory =self->IO_METHOD;
    		self->specs.cam_buf.index = i;
			/*
			if(Camera.IO_METHOD==V4L2_MEMORY_USERPTR)//additional settings for userptr method
			{
					cam_buf.m.userptr=Camera.buffer;
					cam_buf.length=fmt.fmt.pix.sizeimage;
			}
			*/
			if(-1 == xioctl(self->fd, VIDIOC_QBUF, &self->specs.cam_buf))
			{//queue buffers to fill camera data
				perror(ANSI_COLOR_RED"VIDIOC_QBUF"ANSI_COLOR_RESET);
				exit(errno);
			}
	}
	
    if(-1 == xioctl(self->fd, VIDIOC_STREAMON, &self->specs.cam_buf.type))
    {//switch streaming on
        perror(ANSI_COLOR_RED"VIDIOC_STREAMON"ANSI_COLOR_RESET);
        exit(errno);
    }
	self->IS_ACTIVE=1;
    return 0;
}
static int dequeue_buff(CameraObject* self){

	if( !self->IS_ACTIVE ){
		printf("camera is not active:\n");
		errno=ENODATA;
		return -1;
	}
	/**/
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(self->fd, &fds);

    struct timeval tv = {0};
    tv.tv_sec = 2;

    int r = select(self->fd+1, &fds, NULL, NULL, &tv);
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
    if(-1 == xioctl(self->fd, VIDIOC_DQBUF, &self->specs.cam_buf))
    {
        perror(ANSI_COLOR_YELLOW"VIDIOC_DQBUF"ANSI_COLOR_RESET);
        return -1;
    }

	return 0;
}
void camera__print_specs(CameraObject* self){

	char buffer_format[4];
	int pixfmt=self->specs.fmt.fmt.pix.pixelformat;
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
            self->name,
            self->specs.caps.driver,
            self->specs.caps.card,
            self->specs.caps.bus_info,
            (self->specs.caps.version>>16)&0xff, (self->specs.caps.version>>8)&0xff, self->specs.caps.version&0xff,
            self->specs.caps.capabilities
            );
            
            int cap=self->specs.caps.capabilities;

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
            self->specs.fmt.fmt.pix.width,
            self->specs.fmt.fmt.pix.height,
            buffer_format, self->specs.fmt_desc.description);
            
            camera__get_fps(self); //get fps info
}

int camera__activate(CameraObject* self){
	//try to open camera as read-write mode
	//CLEAR(self);

    //open camera file
    if( ( self->fd = open(self->name, O_RDWR /* required */ | O_NONBLOCK, 0))==-1 ) {
    
        fprintf(stderr, ANSI_COLOR_RED"Could not open"ANSI_COLOR_RESET" '%s': (%s)\n",self->name, strerror(errno));
        return self->fd;
    }
    
    init_camera(self); 			//prepare camera by getting info
    get_format(self, 0);			//get default format options to frame_size(for required for set_format)
    set_format(self, self->specs.USER_FRAME_SIZE.width, self->specs.USER_FRAME_SIZE.height);		//automaticly format to default best format
    camera__print_specs(self);
    init_mmap(self);	//open memory map and concatenate to buffer
	ready_to_capture(self);		//adjust camera buffers and open streaming
	//init_userptr();
    return 0;
}

void set_streaming(CameraObject* self, int control_value){

	switch (self->IO_METHOD) {

        	case V4L2_MEMORY_USERPTR:
		    case V4L2_MEMORY_MMAP:
		    	if (-1 == xioctl(self->fd, control_value, &self->specs.cam_buf.type)){
							perror(ANSI_COLOR_RED"Streaming (VIDIOC_STREAM_X)"ANSI_COLOR_RESET);
		                    exit(errno);
				}else{

					if (control_value==VIDIOC_STREAMON ){
						ready_to_capture(self);
						self->IS_ACTIVE=1;
					}else{
						self->IS_ACTIVE=0;
						}
				}
		        break;
		}
}
static void close_device(CameraObject* self){
        if (-1 == close(self->fd)){
        		perror(ANSI_COLOR_RED"Camera could not closed"ANSI_COLOR_RESET);
                exit(errno);
		}
        self->fd = -1;
}
int camera__deactivate(CameraObject* self){
		set_streaming(self, VIDIOC_STREAMOFF);
		
        switch (self->IO_METHOD) {
        	
		    case V4L2_MEMORY_MMAP:
		    	for (int i = 0; i < self->specs.req.count; i++){
					if (-1 == munmap( self->buffer[i].address, self->buffer[i].length) ){
							perror(ANSI_COLOR_RED"Camera deactivate(munmap)"ANSI_COLOR_RESET);
							exit(errno);
					}
		    	}
		        break;
		    
		    case V4L2_MEMORY_USERPTR:
        		for (int i = 0; i < self->specs.req.count; i++){
					free( self->buffer[i].address );
		    	}
        		break;
		}
		close_device(self);
		return 0;
}

#include<setjmp.h>

//----LİBJPEG USER ERROR HANDLING OVERRİDE
typedef struct jpegErrorManager {
    
    struct jpeg_error_mgr pub;/* "public" fields */
    jmp_buf setjmp_buffer;/* for return to caller */
    char jpegLastErrorMsg[JMSG_LENGTH_MAX]; //keeps error

}customJpegErrorManager;

void jpegErrorExit (j_common_ptr cinfo){
    /* cinfo->err actually points to a jpegErrorManager struct */
    customJpegErrorManager* myerr = (customJpegErrorManager*) cinfo->err;
    /* note : *(cinfo->err) is now equivalent to myerr->pub */

    /* output_message is a method to print an error message */
    /*(* (cinfo->err->output_message) ) (cinfo);*/      
    
    /* Create the message */
    ( *(cinfo->err->format_message) ) (cinfo, myerr->jpegLastErrorMsg);

    /* Jump to the setjmp point */
    longjmp(myerr->setjmp_buffer, 1);
  
}

static uint8_t* camera__decode_rgb(unsigned char *buffer,int buffsize,int width,int height) {
	/*Decode JPEG data of camera to RAW RGB*/
	int rc;
	
	// Variables for the decompressor itself
	struct jpeg_decompress_struct cinfo;
	customJpegErrorManager jerr;//struct jpeg_error_mgr jerr;

	// Variables for the output buffer, and how long it is
	unsigned long processed_size;
	unsigned char *processed_buffer;

	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = jpegErrorExit; //connect signal handler to our custom method
	
	if (setjmp(jerr.setjmp_buffer)) {
        /* If we get here, the JPEG code has signaled an error. */
        printf(ANSI_COLOR_RED"libjpeg.h: "ANSI_COLOR_YELLOW"%s\n"ANSI_COLOR_RESET, jerr.jpegLastErrorMsg);
        jpeg_destroy_decompress(&cinfo);
        
        processed_buffer=NULL;
        errno=EAGAIN;
        return processed_buffer;
    }
	
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
intptr_t* camera__capture(CameraObject* self, int buffer_type){
	
	intptr_t* rgbBuff;
	if( dequeue_buff(self) ){//io to dequeue buffer. Queueing made here(end of this function) after image processed
		rgbBuff=NULL;
		return rgbBuff;
	}

		switch (buffer_type){
		
			case V4L2_PIX_FMT_RGB24:
				rgbBuff=(intptr_t*)camera__decode_rgb( 
				(uint8_t*)self->buffer[ self->specs.cam_buf.index ].address, 
				self->specs.cam_buf.bytesused , 
				self->specs.fmt.fmt.pix.width, 
				self->specs.fmt.fmt.pix.height);
				break;
				
			case V4L2_PIX_FMT_JPEG://if requested its native buffer(possibly jpeg)  !! returns struct __buff
				if(self->specs.fmt.fmt.pix.pixelformat&V4L2_PIX_FMT_JPEG){
					rgbBuff=(intptr_t*)self->buffer[ self->specs.cam_buf.index ].address;
				}
				break;
			
			default:
				rgbBuff=(intptr_t*)camera__decode_rgb( 
				(uint8_t*)self->buffer[ self->specs.cam_buf.index ].address, 
				self->specs.cam_buf.bytesused , 
				self->specs.fmt.fmt.pix.width, 
				self->specs.fmt.fmt.pix.height);
				break;
		}
		
		//resend buffer to queue, so camera can fill it up again
		if(-1 == xioctl(self->fd, VIDIOC_QBUF, &self->specs.cam_buf)){
				
				perror(ANSI_COLOR_RED"VIDIOC_QBUF"ANSI_COLOR_RESET);
				exit(errno);
		}
    return rgbBuff;
}

char *camera__imsave(CameraObject* self, const char* name){

	char* filename=calloc(129,sizeof(char));
	
	strcat(filename,"../images/");
	//write_time(filename, 0 );
	strcat(filename,name);
	
	switch(self->specs.fmt.fmt.pix.pixelformat){
	
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
		recv+=write(jpg_fd, self->buffer[ self->specs.cam_buf.index ].address, 
		self->buffer[ self->specs.cam_buf.index ].length);
		//printf("recv:%i\n",recv);
	}while( recv < self->specs.cam_buf.bytesused );

	close(jpg_fd);
	
	return filename;
	
}
CameraObject* camera__new(const char* path){

	CameraObject* obj=(CameraObject*)calloc(1, sizeof(CameraObject));
	
	obj->name=calloc(257, sizeof(char));
	
	strcpy(obj->name, path);
	
	return obj;
}

void camera__destroy(CameraObject** self){
	
	if(self){
		/*
		set_streaming( (*self), VIDIOC_STREAMOFF );
		camera__deactivate( (*self) );
		*/
		free( (*self)->buffer );//mmap which interacts with camera buffer
		free((*self)->name);	//malloc'd buffer for camera path
		free( (*self) );		//malloc'd main struct of object 
		
		*self=NULL;				//set to NULL to check later elsewhere
		
	}else
		printf("already null. Do not call free()\n");
		
	printf("free:CameraObject destructed\n");
}
/*
//Test
int main(int argc, char **argv){

	CameraObject* cam;
	cam=camera__new("/dev/video0");
	
	printf("new camera object constructed: %s\n",cam->name);
	
	camera__activate(cam);
	printf("camera object activated\n");
	
	//wait for camera to fill mmap
	intptr_t* temp=camera__capture(cam,-1);//have to call capture before imsave, since capture dequeues buffer of device
	free(temp);
	
	char* savepath=camera__imsave(cam,"test image");
	printf("test image took from camera and saved to:%s\n",savepath);
	free(savepath);
	
	camera__destroy(&cam);
	
	printf("Camera object destroyed\n");
	
	return 0;
}
*/
