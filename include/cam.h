#ifndef halocam_camera
#define halocam_camera
#include <stdint.h>
#include <linux/videodev2.h>	

typedef struct __buff{
	intptr_t* address;
	size_t length;
}__buff;

typedef struct __specs{
	struct v4l2_format fmt;							//format specs of camera
	struct v4l2_fmtdesc fmt_desc;					//default format desc
	struct v4l2_frmsizeenum frame_size;				//enumerate available framesizes for camera formats
	struct v4l2_buffer cam_buf; 					//camera buffer retrivede from device by VIDIOC_DQBUF
	struct v4l2_capability caps;					//keeps informations, device capabilities
	struct v4l2_cropcap cropcap;					//to get cropping capabilities
	struct v4l2_crop crop;							//set crop settings
	struct v4l2_requestbuffers req;					//to concatenate camera buffers by VIDIOC_Q_BUFF
	
	struct v4l2_queryctrl queryctrl;				//to get supported controls
	struct v4l2_querymenu querymenu;				//to get menus those of controls
	struct v4l2_frmsize_discrete USER_FRAME_SIZE;	//user defined width and height frames for camera
}__specs;


typedef struct _CameraObject{
	char *name;										//camera path, default:/dev/video0 on main
	int fd;											//file descriptor of camera file
	__buff *buffer;									//cam output buffer that concatenated to mmap
	int IO_METHOD;									//mmap, userptr or R/W
	uint8_t IS_ACTIVE;								//return if camera active to send image data
	__specs specs;
}CameraObject; 
/*
*/
CameraObject* camera__new(const char* path);		//create new object with given device path
void camera__destroy(CameraObject** self);			//destroy object(release from memory)

int 	 camera__activate(CameraObject* self);	//opens camera
int		 camera__deactivate(CameraObject* self);						//closes camera
void 	 camera__print_specs(CameraObject* self);

int camera__control__get_ctrl(CameraObject* self);					//get supported camera controls to 'struct queryctrl' one by one
void camera__control__enumerate_menu(CameraObject* self);				//(if exist)get menu of control 'struct queryctrl' to 'struct querymenu' one by one
void camera__control__set(CameraObject* self, int ctrl_id, int val);	//set control 'ctrl_id' to desired 'int val'

//uint8_t* camera__decode_rgb(unsigned char *buffer,int buffsize,int width,int height);//convert camera buffer to RGB and apply filters
intptr_t* camera__capture(CameraObject* self, int buffer_type);			//convert camera buffer to 'buffer_type' and return
char*	 camera__imsave(CameraObject* self, const char* filename);		//dumps buffer data to file

int get_format(CameraObject* self, int fmt_description_index);			//get supported camera formats to 'struct fmt_desc' one by one
int get_frameSize(CameraObject* self, int pixelformat);					//get framesizes of that supported formats to 'struct frame_size' obne by one
void set_streaming(CameraObject* self, int control_value);

#endif
