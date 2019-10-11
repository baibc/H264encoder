/***********************************************************************
    > File Name:    testENC.c
    > Author:       BaiBingCheng
    > Email:        916975723@qq.com 
    > Created Time: 2016年04月11日 星期一 17时22分30秒
 *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include "vencoder.h"
#include "libve_enc.h"

long long time1=0;
long long time2=0;
long long time3=0;
static long long GetNowUs()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec * 1000000 + now.tv_usec;
}

picWidth =1600;
picHeight =900;
VencBaseConfig baseConfig;
VencAllocateBufferParam bufferParam;
VencHeaderData sps_pps_data;
VencH264Param h264Param;
VideoEncoder* pVideoEnc = NULL;

int main()
{
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
		h264Param.nFramerate = 30; /* fps */
		h264Param.nCodingMode = VENC_FRAME_CODING;
		h264Param.nMaxKeyInterval = 30;
		h264Param.sProfileLevel.nProfile = VENC_H264ProfileMain;
		h264Param.sProfileLevel.nLevel = VENC_H264Level31;
		h264Param.sQPRange.nMinqp = 10;
		h264Param.sQPRange.nMaxqp = 40;

		int testNumber = 0;
		unsigned char* pOutBuf = NULL;
		unsigned char* pInBuf  = NULL;
		pOutBuf = (unsigned char*)malloc(picWidth*picHeight*3/2);
		pInBuf  = (unsigned char*)malloc(picWidth*picHeight*3/2);
		if(libveENC_init(&pVideoEnc, &baseConfig, &h264Param) != -1)
		{
				printf("1.>>>>> libveENC_init done!\n");
				handleBeforeJob();
				while(1)
				{
						//time1 = GetNowUs();
						libveENC_handle(&pVideoEnc, pInBuf, pOutBuf, &sps_pps_data, &bufferParam);
						//time2 = GetNowUs();
						//time3 += time2-time1;
						//printf("encode frame %d use time is %lldus...\n",testNumber, (time2-time1));
						testNumber++;
						if(testNumber >350)
								break;
				}
				//printf("the average encode time is %lldus...\n",time3/testNumber);
				libveENC_exit(&pVideoEnc);
				handleAfterJob();
				free(pOutBuf);
				free(pInBuf);
		}
		return 0;
}
