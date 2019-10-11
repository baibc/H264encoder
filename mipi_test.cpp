#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>            
#include <fcntl.h>             
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>         
#include "videodev2.h"
#include <time.h>

/* ####################################### */
#include "libve_enc.h"
int picWidth =1360;
int picHeight =768;
VencBaseConfig baseConfig;
VencAllocateBufferParam bufferParam;
VencHeaderData sps_pps_data;
VencH264Param h264Param;
VideoEncoder* pVideoEnc = NULL;
/* ####################################### */

#define CLEAR(x) memset (&(x), 0, sizeof (x))
#define ALIGN_4K(x) (((x) + (4095)) & ~(4095))
#define ALIGN_16B(x) (((x) + (15)) & ~(15))

struct size{
	int width;
	int height;
};

struct buffer {
    void * start;
    size_t length;
};

int cnt = 0;
static int  fd  = -1;
struct buffer * buffers  = NULL;
static unsigned int   n_buffers	= 0;

struct size input_size;
struct size subch_size;
unsigned int  req_frame_num = 10;

static int read_frame (void)
{
	struct v4l2_buffer buf;
	char fdstr[1360];
	void * bfstart = NULL;
	FILE *file_fd = NULL;
	int i,num;
	
	CLEAR (buf);
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;

	if (-1 ==ioctl (fd, VIDIOC_DQBUF, &buf) )   
		return -1;
	    
	assert (buf.index < n_buffers);

	//num = (mode > 2) ? 2 : mode;
	bfstart = buffers[buf.index].start;

    /* ###################################################################################### */
        libveENC_handle(&pVideoEnc, (unsigned char*)buffers[buf.index].start, (unsigned char*)fdstr, &sps_pps_data, &bufferParam);
    /* ###################################################################################### */  	

	//for (i = 0; i <= num; i++)
	{	/*
		printf("file %d start = %p\n", i, bfstart); 

		sprintf(fdstr,"%s/fb%d_y%d.bin",path_name,i+1,mode);
		file_fd = fopen(fdstr,"w");
		fwrite(bfstart, buf_size[i]*2/3, 1, file_fd); 
		fclose(file_fd);

		sprintf(fdstr,"%s/fb%d_u%d.bin",path_name,i+1,mode);
		file_fd = fopen(fdstr,"w");
		fwrite(bfstart + buf_size[i]*2/3, buf_size[i]/6, 1, file_fd); 
		fclose(file_fd);

		sprintf(fdstr,"%s/fb%d_v%d.bin",path_name,i+1,mode);
		file_fd = fopen(fdstr,"w");
		fwrite(bfstart + buf_size[i]*2/3 + buf_size[i]/6, buf_size[i]/6, 1, file_fd); 
		fclose(file_fd); */

		//bfstart += ALIGN_4K( buf_size[i] );	
	}

	cnt++;
	if (-1 == ioctl (fd, VIDIOC_QBUF, &buf))
		return -1;
	return 0;
}

static int req_frame_buffers(void)
{
	unsigned int i;
	struct v4l2_requestbuffers req;
	
	CLEAR (req);
	req.count		= req_frame_num;
	req.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory		= V4L2_MEMORY_MMAP;	
	
	ioctl (fd, VIDIOC_REQBUFS, &req); 

	buffers = (buffer*)calloc (req.count, sizeof (*buffers));

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) 
	{
		struct v4l2_buffer buf;  
		CLEAR (buf);
		buf.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory		= V4L2_MEMORY_MMAP;
		buf.index		= n_buffers;

		if (-1 == ioctl (fd, VIDIOC_QUERYBUF, &buf)) 
			printf ("VIDIOC_QUERYBUF error\n");

		buffers[n_buffers].length = buf.length;
		printf("XXXXXXXXXXXXXXXXX buf.length=%d\n", buf.length);
		buffers[n_buffers].start  = mmap (NULL /* start anywhere */, 
								         buf.length,
								         PROT_READ | PROT_WRITE /* required */,
								         MAP_SHARED /* recommended */,
								         fd, buf.m.offset);
	
		if (MAP_FAILED == buffers[n_buffers].start)
		{
			printf ("mmap failed\n");
			return -1;
		}

	}

	for (i = 0; i < n_buffers; ++i) 
	{
		struct v4l2_buffer buf;
		CLEAR (buf);

		buf.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory	= V4L2_MEMORY_MMAP;
		buf.index	= i;

		if (-1 == ioctl (fd, VIDIOC_QBUF, &buf))
		{
			printf ("VIDIOC_QBUF failed\n");
			return -1;
		}
	}
	return 0;

}

static int free_frame_buffers(void)
{
	unsigned int i;

	for (i = 0; i < n_buffers; ++i) {
		if (-1 == munmap (buffers[i].start, buffers[i].length)) {
			printf ("munmap error");
			return -1;
		}
	}
	return 0;
}

static int camera_init(char *dev_name, int sel)
{
	struct v4l2_input inp;
	struct v4l2_streamparm parms;
	
	fd = open (dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);	
	if(!fd) {
		printf("open falied\n");
		return -1;	
	}
	
	inp.index = sel;	
	if (-1 == ioctl (fd, VIDIOC_S_INPUT, &inp))
	{
		printf("VIDIOC_S_INPUT %d error!\n",sel);
		return -1;
	}
	
	//VIDIOC_S_PARM			
	parms.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parms.parm.capture.timeperframe.numerator = 1;
	parms.parm.capture.timeperframe.denominator = 30;
	parms.parm.capture.capturemode = V4L2_MODE_VIDEO; //V4L2_MODE_IMAGE
					
	if (-1 == ioctl (fd, VIDIOC_S_PARM, &parms)) 
	{
		printf ("VIDIOC_S_PARM error\n");	
		return -1;
	}
			
	return 0;
			
}

static int camera_fmt_set(int subch, int angle)
{
	struct v4l2_format fmt;
	struct v4l2_pix_format subch_fmt;
	
	//VIDIOC_S_FMT
	CLEAR (fmt);
	fmt.type                	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       	= input_size.width; 	//640; 
	fmt.fmt.pix.height      	= input_size.height; 	//480;
	fmt.fmt.pix.pixelformat 	= V4L2_PIX_FMT_YUV420;		//V4L2_PIX_FMT_YUV422P;//V4L2_PIX_FMT_NV12;//V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.field       	= V4L2_FIELD_NONE;			//V4L2_FIELD_INTERLACED;//V4L2_FIELD_NONE;
	fmt.fmt.pix.rot_angle	    = 0;

	if (0 == subch) {
		fmt.fmt.pix.subchannel	= NULL;	
	} else {
		fmt.fmt.pix.subchannel	= &subch_fmt;
		subch_fmt.width 	= subch_size.width;
		subch_fmt.height	= subch_size.height;
		subch_fmt.pixelformat	= V4L2_PIX_FMT_YUV420; 		//V4L2_PIX_FMT_YUV422P;//V4L2_PIX_FMT_NV12;//V4L2_PIX_FMT_YUYV;
		subch_fmt.field 	= V4L2_FIELD_NONE;			//V4L2_FIELD_INTERLACED;//V4L2_FIELD_NONE;
		subch_fmt.rot_angle 	= angle;
	}
			
	if (-1 == ioctl (fd, VIDIOC_S_FMT, &fmt))
	{
		printf("VIDIOC_S_FMT error!\n");
		return -1;
	}
	
	//Test VIDIOC_G_FMT	
	if (-1 == ioctl (fd, VIDIOC_G_FMT, &fmt)) 
	{
		printf("VIDIOC_G_FMT error!\n");
		return -1;
	}
	else
	{
		printf("resolution got from sensor = %d*%d\n",fmt.fmt.pix.width,fmt.fmt.pix.height);
	}
	return 0;

}

/*
 *stream on
 */
int streamon(void)
{
	int ret;
	enum v4l2_buf_type iType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	//int iType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(fd, VIDIOC_STREAMON, &iType);
    if (ret < 0) {
        printf("%d : VIDIOC_STREAMON failed\n",__LINE__);
        return ret;
    }else 
	printf ("VIDIOC_STREAMON ok\n");

	return 0;
}


/*
 *streamoff
 */
int streamoff(void)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret;

    ret = ioctl(fd, VIDIOC_STREAMOFF, &type);
    if (ret < 0) {
        printf("%s : VIDIOC_STREAMOFF failed\n",__func__);
        return ret;
    } else {
	printf ("VIDIOC_STREAMOFF ok\n");
	}	

    return 0;
}


/*
 *release
 */
int release(void)
{
	if ( -1 == free_frame_buffers())
		return -1;

	close (fd);	
	return 0;
}


/*
 *open usb camera device
 */
int mipi_init()
{

	int sel = 0;
	int width = 1360;
	int height = 768;
	int subch = 1;
	int angle = 270;

	input_size.width = width;
	input_size.height = height;
	
	subch_size.width = input_size.width >> 1;
	subch_size.height = input_size.height >> 1;


	char *dev_name = "/dev/video0";

	if (-1== camera_init(dev_name, 0))	//camera select
		return -1;
	if (-1 == camera_fmt_set(subch, angle)) 
		return -1;
	if (-1 ==req_frame_buffers())
		return -1;		
	
	return 0;
}


int main()
{
/* ####################################################################### */
        //the basic configration for init encoder
        memset(&baseConfig, 0 ,sizeof(VencBaseConfig));
        baseConfig.nInputWidth= picWidth;
        baseConfig.nInputHeight = picHeight;
        baseConfig.nStride = picWidth;  //(picWidth + 15) & 0xfffffff0;
        baseConfig.nDstWidth = picWidth;
        baseConfig.nDstHeight = picHeight;
        baseConfig.eInputFormat = VENC_PIXEL_YUV420P;

        //alloc frame memory's configration
        memset(&bufferParam, 0 ,sizeof(VencAllocateBufferParam));
        bufferParam.nSizeY = baseConfig.nInputWidth*baseConfig.nInputHeight;
        bufferParam.nSizeC = baseConfig.nInputWidth*baseConfig.nInputHeight/2;
        bufferParam.nBufferNum = 4;

        //h264 param
        h264Param.bEntropyCodingCABAC = 1;
        h264Param.nBitrate = 4*1024*1024; /* bps */
        h264Param.nFramerate = 30; /* fps */
        h264Param.nCodingMode = VENC_FRAME_CODING;
        h264Param.nMaxKeyInterval = 30;
        h264Param.sProfileLevel.nProfile = VENC_H264ProfileMain;
        h264Param.sProfileLevel.nLevel = VENC_H264Level31;
        h264Param.sQPRange.nMinqp = 10;
        h264Param.sQPRange.nMaxqp = 40;
        if(libveENC_init(&pVideoEnc, &baseConfig, &h264Param) != -1){
                printf("1.>>>>> libveENC_init done!\n");
                handleBeforeJob();
        }
/* ########################################################################## */

	int i, ret;
	clock_t start;

	mipi_init();
	streamon();

	start = clock();	        			
	for (;;) 
	{
		fd_set fds;
		struct timeval tv;
		int r;
		
		FD_ZERO (&fds);
		FD_SET (fd, &fds);

		tv.tv_sec = 2;			/* Timeout. */
		tv.tv_usec = 0;
	
		r = select (fd + 1, &fds, NULL, NULL, &tv);			
		if (-1 == r) {
			if (EINTR == errno)
				continue;
			printf ("select err\n");
		}
		if (0 == r) {
			fprintf (stderr, "select timeout\n");
			return -1;
		}
		
		ret = read_frame ();
		if (ret < 0) {
			printf("read frame error!\n");		
		}

		if (cnt == 60)break;		
	}

	//clock_t stop = clock();
	//printf(">>>>>>>>>>>>>>>>>>>>>>>>end =%d\n", stop);
    //float end = (float)(stop - start)/CLOCKS_PER_SEC;
	//printf(">>>>>>>>>>>>>>>>:%d\n", (stop-start));
    //printf("time comsumption is %f\n", end);		
	//printf("cnt is %d\n", cnt);		
	streamoff();
	release();

    /* ################################### */
        libveENC_exit(&pVideoEnc);
        handleAfterJob();
    /* ################################### */

	return 0;
}





