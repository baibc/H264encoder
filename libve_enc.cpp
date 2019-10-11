/***********************************************************************
    > File Name:    libve_enc.c
    > Author:       BaiBingCheng
    > Email:        916975723@qq.com 
    > Created Time: 2016年04月11日 星期一 17时00分18秒
 *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include "vencoder.h"

long long t1;
long long t2;
long long t3;
long long t4;
static long long GetNowUs()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec * 1000000 + now.tv_usec;
}
static FILE* enc_input_stream = NULL;
static FILE* enc_output_stream = NULL;
#define SAVE_BITSTREAM  1
#define ENC_INPUT_STREAM_PATH  "/data/Farsight/yuv30file.yuv"
//#define ENC_OUTPUT_STREAM_PATH "/data/Farsight/output.h264"
#define ENC_OUTPUT_STREAM_PATH "/storage/extsd/output.h264"

int tmp =0;
int tmpCount =0;
int handleBeforeJob()
{
		enc_output_stream = fopen(ENC_OUTPUT_STREAM_PATH, "wb");
		if(enc_output_stream == NULL){
            printf(">>> open enc_output_stream error");
            return -1;
        }
		return 0;
}
int handleAfterJob()
{
        fclose(enc_output_stream);
		return 0;
}

VencInputBuffer inputBuffer;
VencOutputBuffer outputBuffer;
static int sps_pps_flag =0;//overall situation
unsigned int libveENC_handle(VideoEncoder** pEncoder, unsigned char* pInBuf, unsigned char* pOutBuf, VencHeaderData* sps_pps_data,
					VencAllocateBufferParam* bufferParam)
{	
		unsigned int tmpValue=0;
		if(sps_pps_flag ==1)
		{
				VideoEncGetParameter(*pEncoder, VENC_IndexParamH264SPSPPS, sps_pps_data);
		#if SAVE_BITSTREAM
				fwrite(sps_pps_data->pBuffer, 1, sps_pps_data->nLength, enc_output_stream);//FILE*
				sps_pps_flag =0;
		#else
				memcpy(pOutBuf, sps_pps_data->pBuffer, sps_pps_data->nLength);
				tmpValue += sps_pps_data->nLength;
		#endif
				AllocInputBuffer(*pEncoder, bufferParam);
		}

		memset(&inputBuffer, 0, sizeof(VencInputBuffer));
		GetOneAllocInputBuffer(*pEncoder, &inputBuffer);		

		//t3 = GetNowUs();
		memcpy(inputBuffer.pAddrVirY, (unsigned char*)(pInBuf), bufferParam->nSizeY);
		memcpy(inputBuffer.pAddrVirC, (unsigned char*)(pInBuf + bufferParam->nSizeY), bufferParam->nSizeC);
		//t4 = GetNowUs();
		//printf("memcpy %d frame use time is %lldus...\n", ++tmp, (t4-t3));

		inputBuffer.bEnableCorp = 0;
        inputBuffer.sCropInfo.nLeft =  240;
        inputBuffer.sCropInfo.nTop  =  240;
        inputBuffer.sCropInfo.nWidth  =  240;
        inputBuffer.sCropInfo.nHeight =  240;

        FlushCacheAllocInputBuffer(*pEncoder, &inputBuffer);
   
        AddOneInputBuffer(*pEncoder, &inputBuffer);
		
		//t1 = GetNowUs();
		VideoEncodeOneFrame(*pEncoder);
		//t2 = GetNowUs();
		//printf("encode %d frame use time is %lldus...\n", ++tmpCount, (t2-t1));
		printf("encode %d frame finished!!!!!!\n", ++tmpCount);

        AlreadyUsedInputBuffer(*pEncoder, &inputBuffer);
        ReturnOneAllocInputBuffer(*pEncoder, &inputBuffer);

		memset(&outputBuffer, 0, sizeof(VencOutputBuffer));
        GetOneBitstreamFrame(*pEncoder, &outputBuffer);

#if SAVE_BITSTREAM
		{
			fwrite(outputBuffer.pData0, 1, outputBuffer.nSize0, enc_output_stream);
	        if(outputBuffer.nSize1)
    	            fwrite(outputBuffer.pData1, 1, outputBuffer.nSize1, enc_output_stream);
		}
#else
		if(sps_pps_flag ==1)
		{
				memcpy(pOutBuf + sps_pps_data->nLength, outputBuffer.pData0, outputBuffer.nSize0);//offset
				tmpValue += outputBuffer.nSize0;
				if(outputBuffer.pData1 != NULL && outputBuffer.nSize1 > 0){
						memcpy(pOutBuf + sps_pps_data->nLength + outputBuffer.nSize0, outputBuffer.pData1, outputBuffer.nSize1);
						tmpValue += outputBuffer.nSize1;
				}
				sps_pps_flag =0;
		}else{
				tmpValue += outputBuffer.nSize0;
				memcpy(pOutBuf, outputBuffer.pData0, outputBuffer.nSize0);
				if(outputBuffer.pData1 != NULL && outputBuffer.nSize1 > 0){
						memcpy(pOutBuf + outputBuffer.nSize0, outputBuffer.pData1, outputBuffer.nSize1);
						tmpValue += outputBuffer.nSize1;
				}
		}
#endif

        FreeOneBitStreamFrame(*pEncoder, &outputBuffer);
	    return tmpValue; //return 0;
}

int libveENC_init(VideoEncoder** pEncoder, VencBaseConfig* sBaseEncConfig, VencH264Param* h264Param)
{
		//* judge the encoder is NULL or not
		if(*pEncoder != NULL){
				printf("<Init> FUNC: %s, LINE: %d, pEncoder is not NULL!\n",__FUNCTION__, __LINE__);
				return -1;
		}

		//1. create a video encoder
		*pEncoder = VideoEncCreate(VENC_CODEC_H264);
		if(pEncoder == NULL){
				printf("<Init> FUNC: %s, LINE: %d, pEncoder is NULL!\n",__FUNCTION__, __LINE__);
				return -1;
		}

		//2. initialize the encoder.
		if(VideoEncInit(*pEncoder, sBaseEncConfig) != 0){
				printf("<Init> initialize video encoder fail!\n");
				VideoEncUnInit(*pEncoder);
				VideoEncDestroy(*pEncoder);
				*pEncoder = NULL;
				return -1;
		}

		//3. set videoEncoder parameter
		int value;
		VideoEncSetParameter(*pEncoder, VENC_IndexParamH264Param, h264Param);
		value = 1;
		VideoEncSetParameter(*pEncoder, VENC_IndexParamIfilter, &value);
		value = 0;
		VideoEncSetParameter(*pEncoder, VENC_IndexParamRotation, &value);

		sps_pps_flag =1;
		return 0;
}

int libveENC_exit(VideoEncoder** pEncoder)
{
		if(*pEncoder == NULL){
				printf("<exit> FUNC: %s, LINE: %d, pEncoder is NULL!\n",__FUNCTION__, __LINE__);
				return -1;
		}else{
				VideoEncUnInit(*pEncoder);
				VideoEncDestroy(*pEncoder);
				*pEncoder = NULL;
		}
		return 0;
}

