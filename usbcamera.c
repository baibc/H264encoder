/*
 * Copyright 2011 - Churn Labs, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * This is mostly based off of the FFMPEG tutorial:
 * http://dranger.com/ffmpeg/
 * With a few updates to support Android output mechanisms and to update
 * places where the APIs have shifted.
 */
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>	
#include <sys/stat.h>	
#include <linux/fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>    
#include <stdint.h>
#include <stdlib.h>
#include <asm/types.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <string.h>
#include <malloc.h>
#include<sys/poll.h> //poll()
#include <linux/fb.h>
#include <time.h>

/* step1 */
/* ####################################### */
#include "libve_enc.h"
VencBaseConfig baseConfig;
VencAllocateBufferParam bufferParam;
VencHeaderData sps_pps_data;
VencH264Param h264Param;
VideoEncoder* pVideoEnc = NULL;
int bcount=1;
int fps_value=0;
int picWidth =0;
int picHeight =0;
/* ####################################### */

struct fimc_buffer {
    unsigned char *start;
    size_t  length;
};

static int fd = -1;
struct fimc_buffer *buffers=NULL;
struct v4l2_buffer v4l2_buf;
static int bufnum = 1;
static int mwidth,mheight;

static int fd2 = -1;
struct fimc_buffer *buffers2=NULL;
struct v4l2_buffer v4l2_buf2;
static int bufnum2 = 1;
static int mwidth2,mheight2;

fd_set fds;
struct timeval tv;

int fdsret;
int maxfd = 0;
int cnt = 0;

FILE *yuv30file = NULL;

/*
 *open usb camera device
 */
int open_init(int devid)
{
	char *devname;
	struct v4l2_input inp;
	switch(devid){
		case 0:
			devname = "/dev/video0";
			break;
		case 1:
			devname = "/dev/video1";
			break;
		case 2:
			devname = "/dev/video2";
			break;
		case 3:
			devname = "/dev/video3";
			break;
		default:
			devname = "/dev/video1";
			break;
	}
	fd = open(devname, O_RDWR, 0);
	if (fd < 0)
		printf("%s ++++ open error\n",devname);

	inp.index = devid;
    if (-1 == ioctl (fd, VIDIOC_S_INPUT, &inp)){
        printf("VIDIOC_S_INPUT %d error!\n",devid);
        return -1;
    }

	return fd;
}

/*
 * init device
 */
int init(int width, int height, int numbuf)
{
	int ret;
	int i;
	bufnum = numbuf;
	mwidth = width;
	mheight = height;
	struct v4l2_format fmt;	
	struct v4l2_capability cap;
	printf("%d :numbuf = %d\n",__LINE__, numbuf);

	/*memset(&cap, 0, sizeof(struct v4l2_capability));
    ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);//查询设备属性
    if (ret < 0) {
        printf("%d :VIDIOC_QUERYCAP failed\n",__LINE__);
        return -1;
    }
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        printf("%d : no capture devices\n",__LINE__);
        return -1;
    }*/

    struct v4l2_fmtdesc tfmt;
    memset(&tfmt, 0, sizeof(tfmt));
    tfmt.index = 0;
    tfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    /*while ((ret = ioctl(fd, VIDIOC_ENUM_FMT, &tfmt)) == 0)
    {  //查询并显示所有支持的格式
       tfmt.index++;
       printf("{ pixelformat = ''%c%c%c%c'', description = ''%s'' }\n",
       tfmt.pixelformat & 0xFF, (tfmt.pixelformat >> 8) & 0xFF, (tfmt.pixelformat >> 16) & 0xFF,
       (tfmt.pixelformat >> 24) & 0xFF, tfmt.description);
    }*/

	/* set */
	memset( &fmt, 0, sizeof(struct v4l2_format));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
	fmt.fmt.pix.width = width;
	fmt.fmt.pix.height = height;					
	if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0)
	{
		printf("++++%d : set format failed\n",__LINE__);
		return -1;
	}
/* ############################################################################## */
	printf("2..... get the following resolution from sensor after VIDIOC_S_FMT!\n");
	printf("2..... fmt.fmt.pix.width=%d, fmt.fmt.pix.height=%d\n\n",fmt.fmt.pix.width,fmt.fmt.pix.height);
/* ############################################################################## */


/* step2 */
/* ###################################### */
	/* 20160418 add */
	if(width ==1280 && height ==720){
		picWidth  = 1280;
		picHeight = 720;
		fps_value = 60;
	}else if(width ==1280 && height ==721){
		picWidth  = 1280;
		picHeight = 720;
		fps_value = 50;
	}else if(width ==1920 && height ==1080){
		picWidth  = 1920;
		picHeight = 1080;
		fps_value = 25;
	}else if(width ==1360 && height ==768){	
		picWidth  = 1360;
		picHeight = 768;
		fps_value = 60;
	}
/* ###################################### */

/* testCode */
/* ###################################### */
	if (-1 == ioctl (fd, VIDIOC_G_FMT, &fmt)){
			printf("1..... VIDIOC_G_FMT error!\n");
	}else{
			printf("1..... get the following resolution from sensor by VIDIOC_G_FMT!\n");
			printf("1..... fmt.fmt.pix.width=%d, fmt.fmt.pix.height=%d\n\n",fmt.fmt.pix.width,fmt.fmt.pix.height);
	}
	/*if((fmt.fmt.pix.height == 720) || (fmt.fmt.pix.height == 768))
		fps_value =60;
	else if(fmt.fmt.pix.height == 721)
		fps_value =50;
	else if(fmt.fmt.pix.height == 1080)
		fps_value =25; */
	/*if(fmt.fmt.raw_data[0] == 60)
		fps_value =60;
	else if(fmt.fmt.raw_data[0] == 50)
		fps_value =50;
	else if(fmt.fmt.raw_data[0] == 25)
		fps_value =25; */
/* ###################################### */

	//request buffers
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(struct v4l2_requestbuffers));
    req.count = numbuf;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(fd, VIDIOC_REQBUFS, &req);
    if (ret < 0) {
        printf("++++%d : VIDIOC_REQBUFS failed\n",__LINE__);
        return -1;
    }

    printf("%d :numbuf = %d\n",__LINE__, req.count);
    buffers = calloc(req.count, sizeof(*buffers));
    if (!buffers) {
        printf ("++++%d Out of memory\n",__LINE__);
		return -1;
    }

	for(i = 0; i < bufnum; ++i) {
		memset(&v4l2_buf, 0, sizeof(struct v4l2_buffer));
		v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		v4l2_buf.memory = V4L2_MEMORY_MMAP;
		v4l2_buf.index = i;
		ret = ioctl(fd , VIDIOC_QUERYBUF, &v4l2_buf);
		if(ret < 0) {
		   printf("+++%d : VIDIOC_QUERYBUF failed\n",__LINE__);
		   return -1;
		}
		buffers[i].length = v4l2_buf.length;
		if ((buffers[i].start = (char *)mmap(0, v4l2_buf.length,
		                                     PROT_READ | PROT_WRITE, MAP_SHARED,
		                                     fd, v4l2_buf.m.offset)) < 0) {
		     printf("%d : mmap() failed",__LINE__);
		     return -1;
		}
	}

	for (i = 0; i < bufnum; ++i) {
		memset(&v4l2_buf, 0, sizeof(struct v4l2_buffer));
		v4l2_buf.index = i;
		v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		v4l2_buf.memory = V4L2_MEMORY_MMAP;
		ret = ioctl(fd, VIDIOC_QBUF, &v4l2_buf);
		if (ret < 0) {
			printf("%d : 111VIDIOC_QBUF failed\n",__LINE__);
			return ret;
		}
	}
	return 0;
}

/*
 *stream on
 */
int streamon(void)
{
	int i;
	int ret;
	int iType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(fd, VIDIOC_STREAMON, &iType);
    if (ret < 0) {
        printf("%d : VIDIOC_STREAMON failed\n",__LINE__);
        return ret;
    }
	return 0;
}

/*
 *get one frame data
 */
int dqbuf(int fps)
{
    int ret;	
   	char data[1280*720*3/2];
    v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_buf.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(fd, VIDIOC_DQBUF, &v4l2_buf);
    if (ret < 0) {
        printf("%s : VIDIOC_DQBUF failed, dropped frame 3\n",__func__);
        return ret;
    }
	cnt++;

	//libveENC_handle(&pVideoEnc, buffers[v4l2_buf.index].start, (unsigned char*)data, &sps_pps_data, &bufferParam);

/* step5 */
/* ################################################################################################################## */
	//ioctl(fd, FPS_FLAG, &fps);
	if(fps ==60){
		switch(bcount)
		{
			case 1: bcount =2;
					libveENC_handle(&pVideoEnc, buffers[v4l2_buf.index].start, (unsigned char*)data, &sps_pps_data, &bufferParam);
					break;
			case 2: bcount =3; break;
			case 3: bcount =4; 
					libveENC_handle(&pVideoEnc, buffers[v4l2_buf.index].start, (unsigned char*)data, &sps_pps_data, &bufferParam);
					break;
			case 4: bcount =5; break;
			case 5: bcount =1; break;
		}
	}else if(fps ==50){
		if((cnt%2)) 
			libveENC_handle(&pVideoEnc, buffers[v4l2_buf.index].start, (unsigned char*)data, &sps_pps_data, &bufferParam);
	}else if(fps ==25){
		libveENC_handle(&pVideoEnc, buffers[v4l2_buf.index].start, (unsigned char*)data, &sps_pps_data, &bufferParam);
	}
/* ################################################################################################################## */

	//printf("^^ ^^ ^^ ^^ ^^ v4l2_buf.index =%d\n", v4l2_buf.index);
	return v4l2_buf.index;
}


/*
 *put in frame buffer to queue
 */
int qbuf(int index)
{
    int ret;

    v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_buf.memory = V4L2_MEMORY_MMAP;
    v4l2_buf.index = index;

    ret = ioctl(fd, VIDIOC_QBUF, &v4l2_buf);
    if (ret < 0) {
        printf("%s : VIDIOC_QBUF failed\n",__func__);
        return ret;
    }
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
    }
    return 0;
}

/*
 *release
 */
int release(void)
{
	int iType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret;
	int i;

    ret = ioctl(fd, VIDIOC_STREAMOFF, &iType);
    if (ret < 0) {
        printf("%s : VIDIOC_STREAMOFF failed\n",__func__);
        return ret;
    }

    for (i = 0; i < bufnum; i++) {
       ret = munmap(buffers[i].start, buffers[i].length);
		if (ret < 0) {
		    printf("%s : munmap failed\n",__func__);
		    return ret;
    	}
	}
	free (buffers);
	close(fd);
	return 0;
}


int main() {
	int ret;
	char *p = NULL;
	clock_t start;
	clock_t stop;
	//yuv30file = fopen("/mnt/sdcard/yuv30file.yuv","a+");
	//if (yuv30file == NULL)
	//	printf("++++ open file error\n");

	ret = open_init(0);
	if (ret < 0) {
		printf("no this devices\n");
	}

	printf("-------------------- 1\n");
	ret = init(1280, 720, 10);
	if (ret < 0) {
		printf("Init device failed\n");
	}

/* step3 */
/* ######################################################################## */
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
    h264Param.nBitrate = 2*1024*1024; /* bps */
    h264Param.nFramerate = 25; /* fps */
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

	printf("-------------------- 2\n");
	ret = streamon();
	if (ret < 0) {
		printf("streamon failed\n");
	}

	int i;
	printf("-------------------- 3\n");
	for(i = 0; i < 2000; i++) {

		if(cnt ==0)
			start = clock();

/* step4 */
/* ############################################ */
		ret = dqbuf(fps_value);
/* ############################################ */

		//ret = dqbuf(50); 
		if (ret < 0) {
			printf("dqbuf failed\n");
		}
		qbuf(ret);
		if(cnt ==60){
			cnt =0;
			stop = clock();
    		float end = (float)(stop - start)/CLOCKS_PER_SEC;
    		printf("\n>>> 60fps'time comsumption is %f\n\n", end);	
		}
	}

	printf("cnt = %d\n", cnt);
	ret = streamoff();
	if (ret < 0) {
		printf("streamoff failed\n");
	}

/* ################################### */
	libveENC_exit(&pVideoEnc);
    handleAfterJob();
/* ################################### */
	//fclose(yuv30file);
	return 0;
}

