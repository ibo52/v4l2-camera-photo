#ifndef HALO_CAMERA_H
#define HALO_CAMERA_H
#include <stdint.h>
#include <linux/videodev2.h>	

typedef struct __buff{
	int8_t* address;
	uint32_t length;
}cameraBuffer;

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


typedef struct __Camera{
	char *name;										//camera path, default:/dev/video0 on main
	int fd;											//file descriptor of camera file
	cameraBuffer *buffer;							//cam output buffer that concatenated(points) to mmap
	int IO_METHOD;									//mmap, userptr or R/W
	uint8_t IS_ACTIVE;								//return if camera active to send image data
	__specs specs;
}Camera; 

/**
 * virtual methods table for Camera
*/
typedef struct __Camera_VTable{
	Camera* (*new)(const char* path);

	int (*activate)(Camera* self);
	int (*deactivate)(Camera* self);
	void (*printSpecs)(Camera* self);

	int (*getControl)(Camera* self);
	void (*setControl)(Camera* self, int ctrl_id, int val);

	cameraBuffer (*capture)(Camera* self, int buffer_type);
	const char* (*imsave)(Camera* self, const char* filename);

	void (*destroy)(Camera** self);

}Camera_VTable;

/**
 * Allocate mem for a new camera object
*/
Camera* camera__new(const char* path);		//create new object with given device path

/**
 * Deallocate mem for the camera object
*/
void camera__destroy(Camera** self);			//destroy object(release from memory)

/**
 * Activates camera/video device to capture video data
 * by initializing buffers and required settings
*/
int 	 camera__activate(Camera* self);

/**
 * Deactivates camera/video device by deallocating buffers
*/
int		 camera__deactivate(Camera* self);						//closes camera

/**
 * Prints the specifications of desired device
 * @param self camera device which specs to be verbosed
*/
void 	 camera__print_specs(Camera* self);

int camera__control__get_ctrl(Camera* self);					//get supported camera controls to 'struct queryctrl' one by one
void camera__control__enumerate_menu(Camera* self);				//(if exist)get menu of control 'struct queryctrl' to 'struct querymenu' one by one

/**
 * Set control 'ctrl_id' of camera/video device
 * to desired option
 * @param self the device which controls be adjusted
 * @param ctrl_id the control identity number to be changed
 * @param val the desired value of the control
*/
void camera__control__set(Camera* self, int ctrl_id, int val);

//uint8_t* camera__decode_rgb(unsigned char *buffer,int buffsize,int width,int height);//convert camera buffer to RGB and apply filters

/**
 * Capture an image from camera and return a pointer to that
 * buffer on the inner struct of cam
 * 
 * No need to be free membe address of buffer, but buffer itself
 * 
 * @param self the Camera which image to be captured 
 * @param buffer_type type of buffer, only V4L2_PIX_FMT_(M)JPEG and V4L2_PIX_FMT_RGB24 for now
*/
cameraBuffer camera__capture(Camera* self, int buffer_type);			//convert camera buffer to 'buffer_type' and return

/**
 * Save the last image returned by camera,
 * which called by camera__capture function
 * 
 * @param self Camera object that holds image in its inner structure
 * @param filename filename of image to be saved. Also can be a path
*/
const char*	 camera__imsave(Camera* self, const char* filename);		//dumps buffer data to file

/**
 * Check supported format options of video device
*/
int get_format(Camera* self, int fmt_description_index);			//get supported camera formats to 'struct fmt_desc' one by one

/**
 * get framesize info of that supported format value 'self.specs.fmt desc'
 * to 'self.specs.frame_size' one by one
*/
int get_frameSize(Camera* self, int pixelformat);					

/**
 * Set the streaming the video device On/Off
*/
void set_streaming(Camera* self, int control_value);

/**
 * virtual methods table of Camera
 * 
*/
const static Camera_VTable CameraClass={
	.new=camera__new,
	.activate=camera__activate,
	.deactivate=camera__deactivate,
	.printSpecs=camera__print_specs,

	.getControl=camera__control__get_ctrl,
	.setControl=camera__control__set,

	.capture=camera__capture,
	.imsave=camera__imsave,

	.destroy=camera__destroy
};

#endif
