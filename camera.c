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
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>

//write zero to the struct space
#define CLEAR(x) memset(&(x), 0, sizeof(x))

char *dev_name;//camera path, default:/dev/video0 on main
int fd = -1;//file descriptor of camera
uint8_t *buffer;	//cam output buffer

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
static int xioctl(int fh, int request, void *arg){
    int r;

    do {
            r = ioctl(fh, request, arg);
    } while (-1 == r && EINTR == errno);

    return r;
}

static void set_format(int width,int height){
    fmt.fmt.pix.width=width;
    fmt.fmt.pix.height=height;
    if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt)){
        perror("Setting Pixel Format");
    }
    
    printf("display formatted to: %dx%d\n",fmt.fmt.pix.width,fmt.fmt.pix.height);
}

static int init_camera(int fd){

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
    //get deafult camera formats to fmt struct
    if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt)){
        perror("get default dev format");
        return errno;
    }
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
    int outfd = open("out.jpg", O_RDWR|O_CREAT);
    write(outfd, buffer, cam_buf.bytesused);
    close(outfd);
    return 0;
}
void print_specs(){
    printf( "Device Informations:\n"
            "--------------------\n"
            "  Driver: \"%s\"\n"
            "  Card: \"%s\"\n"
            "  Bus: \"%s\"\n"
            "  Version: %d.%d\n"
            "  Capabilities: %08x\n"
            "--------------------\n",
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
           return -1;//let interpret as exit silently
        }
	for(i=1;i<argc;i++){
	
		if(argv[i][0]=='-'){
			
			switch(argv[i][1]){
				case 'o': dumpBuffer();break;
				case 'i': print_specs();break;
                                case 'h': printf("%s [options]\n"
                "-h :print this message\n"
                "-i :device information\n"
                "-o :dump img to stdout\n",argv[0]);break;
			}
		}
	}
        return 0;
}
int main(int argc, char **argv){
	//try to open camera as read-write mode
    printf("halilibo cam dump struggles\n");
    dev_name="/dev/video0";
    
    if( (fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0))==-1 ) {
        char *msg;
        sprintf(msg,"Could not open '%s'\n",dev_name);
        perror(msg);
        return fd;
    }
    init_camera(fd);
    
    init_mmap(fd);
    
    char c='n';
    do{
    	ready_to_capture(fd);
        /*if(-1==get_arg_opts(argc,argv) ){
            continue;
        }*/
    	printf("do you want to re-take?(y-n):");
    	scanf(" %c",&c);
    }while(c!='n');
    
    return 0;
}
