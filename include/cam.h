#ifndef halocam_camera
#define halocam_camera
#include <stdint.h>
#include <linux/videodev2.h>

typedef struct camera{
	char *name;						//camera path, default:/dev/video0 on main
	int fd;							//file descriptor of camera file
	uint8_t *buffer;				//cam output buffer that concatenated to mmap
}camera; 
extern struct camera Camera;		

extern struct v4l2_format fmt;			//format specs
extern struct v4l2_fmtdesc fmt_desc;	//default format desc
extern struct v4l2_frmsizeenum frame_size;//enumerate available framesizes for camera formats
extern struct v4l2_buffer cam_buf;		//take camera mmap to here
extern struct v4l2_capability caps;		//keep device capabilities
extern struct v4l2_cropcap cropcap;		//default cropping capabilities
extern struct v4l2_crop crop;			//set crop settings
extern struct v4l2_requestbuffers req;
/*
 *  
*/
int 	 camera__activate();						//activate camera
uint8_t* camera__decode_rgb(unsigned char *buffer,int buffsize,int width,int height);//convert camera buffer to RGB and apply filters
uint8_t* camera__get_RGB_buff();			//convert camera buffer to RGB and return that RGB buffer
char*	 camera__dump_buffer_to_file(const char* filename);//dumps raw buffer data to file
#endif
