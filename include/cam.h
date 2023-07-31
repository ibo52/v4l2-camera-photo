#ifndef halocam_cam_
#define halocam_cam_
#include <stdint.h>
#include <linux/videodev2.h>

extern char *dev_name;				//camera path, default:/dev/video0 on main
extern int fd;						//file descriptor of camera file
extern uint8_t *buffer;				//cam output buffer as one bit each

extern struct halocam_device_specs device_specs;
extern struct v4l2_format fmt;		//format specs
extern struct v4l2_buffer cam_buf;	//take camera mmap to here
extern struct v4l2_capability caps;	//keep device capabilities
extern struct v4l2_cropcap cropcap;	//default cropping capabilities
extern struct v4l2_crop crop;		//set crop settings
extern struct v4l2_requestbuffers req;
/*
 *  
*/
int activate();						//activate camera
uint8_t* decode_rgb(unsigned char *buffer,int buffsize,int width,int height);//convert camera buffer to RGB and apply filters
uint8_t* get_RGB_buff();			//convert camera buffer to RGB and return that RGB buffer
char* dump_buffer_to_file(const char* filename);//dumps raw buffer data to file
#endif
