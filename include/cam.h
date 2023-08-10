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
extern struct v4l2_queryctrl queryctrl;
extern struct v4l2_querymenu querymenu;
/*
*/
int 	 camera__activate();						//activate camera
//int		 camera__deactivate();

int camera__control__get_ctrl();					//get supported camera controls to 'struct queryctrl' one by one
void camera__control__enumerate_menu();				//(if exist)get menu of control 'struct queryctrl' to 'struct querymenu' one by one
void camera__control__set(int ctrl_id, int val);	//set control 'ctrl_id' to desired 'int val'

//uint8_t* camera__decode_rgb(unsigned char *buffer,int buffsize,int width,int height);//convert camera buffer to RGB and apply filters
intptr_t* camera__capture(int buffer_type);			//convert camera buffer to 'buffer_type' and return
char*	 camera__imsave(const char* filename);//dumps raw buffer data to file

#endif
