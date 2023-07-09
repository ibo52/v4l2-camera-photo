/*
 *halil ibrahim mut
 * 
 * struggles of access to camera.
 * 
 * this program using v4l2(linux/videodev2.h library) module,
 * which is comes as builtin on linux kernel.
 * this program simply takes photograph from
 * camera and save it as jpeg file.
 * 
 * coded on: Ubuntu 20.04
 * kernel  : 5.13.0-41-generic
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
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>		//uint8_t
#include <stdlib.h>

#include <sys/mman.h>	//mmap
#include <jpeglib.h>	//to decode jpg image buffer
//write zero to the struct space

#define CLEAR(x) memset(&(x), 0, sizeof(x))

char *dev_name;//camera path, default:/dev/video0 on main
int fd=-1;//file descriptor of camera
uint8_t *buffer;	//cam output buffer as one bit each

//int outfd=-1;//descriptor of output image

struct v4l2_format fmt;//format specs
struct v4l2_buffer cam_buf;
struct v4l2_capability caps;//keep device capabilities
struct v4l2_cropcap cropcap;//default cropping capabilities
struct v4l2_crop crop;//set crop settings
struct v4l2_requestbuffers req;
/*
 *
 * 
 *  
*/
int xioctl(int fh, int request, void *arg){
    int r;

    do {
            r = ioctl(fh, request, arg);
    } while (-1 == r && EINTR == errno);

    return r;
}

void set_format(int width,int height){
    fmt.fmt.pix.width=width;
    fmt.fmt.pix.height=height;
    if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt)){
        fprintf(stdout,"Setting Pixel Format: err %d:%s\n",errno,strerror(errno));
        exit(errno);
    }
    
    printf("display formatted to: %dx%d\n",fmt.fmt.pix.width,fmt.fmt.pix.height);
}

int init_camera(int fd){

    CLEAR(cropcap);//clear data struct
    CLEAR(fmt);
    CLEAR(crop);
    CLEAR(caps);

    //request capabilities of device
    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &caps) ) {
        if (EINVAL == errno) {
            fprintf(stdout,"%s is not a V4L2 device: err%d:%s\n",dev_name,errno,strerror(errno));
            return errno;
        } else {
            fprintf(stdout,"query device capabilities err %d:%s",errno,strerror(errno) );
            perror("query device capabilities");
            return errno;
        }
    }

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    //request crop capabilities(width,heigth,frame pos etc..)
    if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */

        if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
            switch (errno) {
            case EINVAL:
                fprintf(stdout,"cropping not supported (ignored)\n");
                break;
            default:
                fprintf(stdout,"VIDIOC_S_CROP error:%d(%s) (errors ignored)\n",errno,strerror(errno));
                break;
            }
        }
    } else {
        fprintf(stdout,"VIDIOC_CROPCAP error:%d(%s) (errors ignored)\n",errno,strerror(errno));
    }

    /*request FORMAT SETTİNGS */
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.pixelformat=V4L2_PIX_FMT_RGB24;
    //get deafult camera formats to fmt struct
    /*if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt)){
        perror("get default dev format");
        return errno;
    }*/
    //set to default with queried capabilities instead above
    //https://www.kernel.org/doc/html/v4.11/media/uapi/v4l/vidioc-cropcap.html#c.v4l2_cropcap
    set_format(cropcap.bounds.width,cropcap.bounds.height);
    
    return 0;
}

int init_mmap(int fd)
{
    CLEAR(req);
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req))
    {
        perror("Requesting Buffer");
        return 1;
    }
    
    struct v4l2_buffer buf;
    CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;
    if(-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
    {
        perror("Querying Buffer");
        return 1;
    }
    
    buffer = mmap (NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

    return 0;
}

int ready_to_capture(int fd)
{
    //görüntünün alınacağı arabellek
    CLEAR(cam_buf);
    
    cam_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    cam_buf.memory = V4L2_MEMORY_MMAP;
    cam_buf.index = 0;
    
    if(-1 == xioctl(fd, VIDIOC_QBUF, &cam_buf))
    {
        perror("Query Buffer");
        return 1;
    }

    if(-1 == xioctl(fd, VIDIOC_STREAMON, &cam_buf.type))
    {
        perror("vidioc Start Capture");
        return 1;
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
        perror("vidioc Wait for Frame");
        return 1;
    }

	//took the frame and retrieve
    if(-1 == xioctl(fd, VIDIOC_DQBUF, &cam_buf))
    {
        perror("vidioc Retrieve Frame");
        return 1;
    }

	//O_CREATE satırını ekledim
    /*outfd = open("out.jpg", O_RDWR|O_CREAT);
    write(outfd, buffer, cam_buf.bytesused);
    close(outfd);*/
    return 0;
}
void print_specs(){
    printf( "------------------------------\n"
    		"Device Info: %s\n"
            "------------------------------\n"
            "  Driver: \"%s\"\n"
            "  Card:   \"%s\"\n"
            "  Bus:    \"%s\"\n"
            "  Version: %d.%d\n"
            "  Capabilities: %08x\n"
            "------------------------------\n"
            "------------------------------\n",
            dev_name,
            caps.driver,
            caps.card,
            caps.bus_info,
            (caps.version>>16)&&0xff,
            (caps.version>>24)&&0xff,
            caps.capabilities);
}
//stdouta göndermek için(ipc ile gui yazarık)
//ama şu an hexadeciamal olarak dökii
void dumpBuffer(){
	//bufferı yazdırıyor									
    int i=0;
    printf("img data size:%d bytes\n",cam_buf.bytesused);
    
    for(i=0;i<cam_buf.bytesused;i++){
    	//%02x prints integer as hex
    	printf("%02x ", buffer[i]);
    	if ((i+1)%1280 == 0) printf("\n");
    	i++;
    }
    printf("\n");
}

int get_arg_opts(int argc,char **argv){
	int i;
	if(argc==1){
           return 1;//let interpret as exit silently
        }
	for(i=1;i<argc;i++){
	
		if(argv[i][0]=='-'){
			
			switch(argv[i][1]){
				case 'o': dumpBuffer();break;
				case 'i': print_specs();break;
				case 'h': printf("%s [options]\n"
                "-h :print this and exit\n"
                "-i :device information\n"
                "-o :dump img to stdout\n",argv[0]);exit(0);
			}
		}
	}
        return 0;
}

int activate(){
	//try to open camera as read-write mode
    printf("-*- halocam -*-\n");
    dev_name="/dev/video0";
    
    if( (fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0))==-1 ) {
    
        char *msg=(char*)calloc(32,sizeof(char));
        sprintf(msg,"Could not open '%s'",dev_name);
        perror(msg);
        return fd;
    }

    init_camera(fd);
    
    init_mmap(fd);
    
    return 0;
}

uint8_t* decode_rgb(unsigned char *buffer,int buffsize,int width,int height) {
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
uint8_t* get_RGB_buff(){
	ready_to_capture(fd);
    uint8_t *rgbBuff=decode_rgb(buffer, cam_buf.bytesused , fmt.fmt.pix.width, fmt.fmt.pix.height);
    return rgbBuff;
}


