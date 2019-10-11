#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vencoder.h"
#include <time.h>
#include <fcntl.h>
#include <sys/types.h> 
#include <unistd.h>

#define ENCODE_MODE 0		//1:jpeg encode,	0:h264 encode

#if ENCODE_MODE
#define	JPEG_ENCODE
#endif

//#define SOURCE_YUV420SP
#define _ENCODER_TIME_
#ifdef _ENCODER_TIME_
static long long GetNowUs()
{
	struct timeval now;
	gettimeofday(&now, NULL);
	return now.tv_sec * 1000000 + now.tv_usec;
}

long long time1=0;
long long time2=0;
long long time3=0;
#endif


//#define ENC_INPUT_STREAM_PATH "/mnt/extsd/yuv/1080p.yuv"
//#define ENC_OUTPUT_STREAM_PATH "/mnt/extsd/yuv/test_1080p.264"

#if CONFIG_OS == OPTION_OS_LINUX
   #define ENC_INPUT_STREAM_PATH "/mnt/yuv/vga.yuv"
   
   #ifdef JPEG_ENCODE
   #define ENC_OUTPUT_STREAM_PATH "/mnt/yuv/auto_test_result/test_vga.jpg"
   #else
   #define ENC_OUTPUT_STREAM_PATH "/mnt/yuv/auto_test_result/test_vga.264"
   #endif
#elif CONFIG_OS == OPTION_OS_ANDROID
	#define ENC_INPUT_STREAM_PATH "/data/camera/vga.yuv"
   
   #ifdef JPEG_ENCODE
   #define ENC_OUTPUT_STREAM_PATH "/data/camera/test_vga.jpg"
   #else
   #define ENC_OUTPUT_STREAM_PATH "/data/camera/test_vga.264"
   #endif
#endif


static FILE* enc_input_stream = NULL;
static FILE* enc_output_stream = NULL;


int main(int argc, char **argv) 
	{

		if (argc != 4)
		{
			loge("usage: decoder-test file://[path]");
			return 1;
		}

		if (strncmp(argv[1], "file://", 7) != 0)
		{
			loge("usage: decoder-test file://[path]");
			return 1;
		}
		int picWidth = atoi(argv[2]);
		int picHeight = atoi(argv[3]);

		logd("resolution '%d x %d'", picWidth, picHeight); //1280x720
		char *inputStream = argv[1] + 7;

		VencBaseConfig baseConfig;
		VencAllocateBufferParam bufferParam;
		VideoEncoder* pVideoEnc = NULL;
		VencInputBuffer inputBuffer;
		VencOutputBuffer outputBuffer;
		VencHeaderData sps_pps_data;
		VencH264Param h264Param;

		VencH264FixQP fixQP; //&
		EXIFInfo exifinfo;   //&
		VencCyclicIntraRefresh sIntraRefresh;//&	
		VencROIConfig sRoiConfig[4];//&

		

	#ifdef SOURCE_YUV420SP
		unsigned char *p1, *p2, *p3, *p4;
		unsigned int j;
	#endif
	
		// roi
		sRoiConfig[0].bEnable = 1;
		sRoiConfig[0].index = 0;
		sRoiConfig[0].nQPoffset = 10;
		sRoiConfig[0].sRect.nLeft = 320;
		sRoiConfig[0].sRect.nTop = 180;
		sRoiConfig[0].sRect.nWidth = 320;
		sRoiConfig[0].sRect.nHeight = 180;
	
	
		sRoiConfig[1].bEnable = 1;
		sRoiConfig[1].index = 1;
		sRoiConfig[1].nQPoffset = 10;
		sRoiConfig[1].sRect.nLeft = 320;
		sRoiConfig[1].sRect.nTop = 180;
		sRoiConfig[1].sRect.nWidth = 320;
		sRoiConfig[1].sRect.nHeight = 180;
	
	
		sRoiConfig[2].bEnable = 1;
		sRoiConfig[2].index = 2;
		sRoiConfig[2].nQPoffset = 10;
		sRoiConfig[2].sRect.nLeft = 320;
		sRoiConfig[2].sRect.nTop = 180;
		sRoiConfig[2].sRect.nWidth = 320;
		sRoiConfig[2].sRect.nHeight = 180;
	
	
		sRoiConfig[3].bEnable = 1;
		sRoiConfig[3].index = 3;
		sRoiConfig[3].nQPoffset = 10;
		sRoiConfig[3].sRect.nLeft = 320;
		sRoiConfig[3].sRect.nTop = 180;
		sRoiConfig[3].sRect.nWidth = 320;
		sRoiConfig[3].sRect.nHeight = 180;
	
	
		//intraRefresh
		
		sIntraRefresh.bEnable = 1;
		sIntraRefresh.nBlockNumber = 10;
	
		//fix qp mode
		fixQP.bEnable = 1;
		fixQP.nIQp = 20;
		fixQP.nPQp = 30;
		
		exifinfo.ThumbWidth = 176;
		exifinfo.ThumbHeight = 144;
	
		//* h264 param
		h264Param.bEntropyCodingCABAC = 1;
		h264Param.nBitrate = 4*1024*1024; /* bps */
		h264Param.nFramerate = 30; /* fps */
		h264Param.nCodingMode = VENC_FRAME_CODING;
		//h264Param.nCodingMode = VENC_FIELD_CODING;
		
		h264Param.nMaxKeyInterval = 30;
		h264Param.sProfileLevel.nProfile = VENC_H264ProfileMain;
		h264Param.sProfileLevel.nLevel = VENC_H264Level31;
		h264Param.sQPRange.nMinqp = 10;
		h264Param.sQPRange.nMaxqp = 40;
	
	
	#ifdef JPEG_ENCODE
		int codecType = VENC_CODEC_JPEG;
	#else
		int codecType = VENC_CODEC_H264;
	#endif
	
		int testNumber = 0;
	
		strcpy((char*)exifinfo.CameraMake,		"allwinner make test");
		strcpy((char*)exifinfo.CameraModel, 	"allwinner model test");
		strcpy((char*)exifinfo.DateTime,		"2014:02:21 10:54:05");
		strcpy((char*)exifinfo.gpsProcessingMethod,  "allwinner gps");
	
		exifinfo.Orientation = 0;
		
		exifinfo.ExposureTime.num = 2;
		exifinfo.ExposureTime.den = 1000;
	
		exifinfo.FNumber.num = 20;
		exifinfo.FNumber.den = 10;
		exifinfo.ISOSpeed = 50;
	
	
		exifinfo.ExposureBiasValue.num= -4;
		exifinfo.ExposureBiasValue.den= 1;
	
		exifinfo.MeteringMode = 1;
		exifinfo.FlashUsed = 0;
	
		exifinfo.FocalLength.num = 1400;
		exifinfo.FocalLength.den = 100;
	
		exifinfo.DigitalZoomRatio.num = 4;
		exifinfo.DigitalZoomRatio.den = 1;
	
		exifinfo.WhiteBalance = 1;
		exifinfo.ExposureMode = 1;
	
		exifinfo.enableGpsInfo = 1;
	
		exifinfo.gps_latitude = 23.2368;
		exifinfo.gps_longitude = 24.3244;
		exifinfo.gps_altitude = 1234.5;
		
		exifinfo.gps_timestamp = (long)time(NULL);
	
		
		enc_input_stream = fopen(inputStream, "r");
		//enc_input_stream = open(ENC_INPUT_STREAM_PATH, O_RDONLY, 0666);
		if(enc_input_stream == NULL)
		{
			loge("open enc_input_stream error");
			return -1;
		}
		enc_output_stream = fopen(ENC_OUTPUT_STREAM_PATH, "wb");
		//enc_output_stream = open(ENC_OUTPUT_STREAM_PATH, O_WRONLY | O_CREAT, 0666);
		if(enc_output_stream == NULL)
		{
			loge("open enc_output_stream error");
			return -1;
		}
	
		memset(&baseConfig, 0 ,sizeof(VencBaseConfig));
		memset(&bufferParam, 0 ,sizeof(VencAllocateBufferParam));
	
		baseConfig.nInputWidth= picWidth;
		baseConfig.nInputHeight = picHeight;
		baseConfig.nStride = picWidth; //(picWidth + 15) & 0xfffffff0;
		
		baseConfig.nDstWidth = picWidth;
		baseConfig.nDstHeight = picHeight;
		
	
		bufferParam.nSizeY = baseConfig.nInputWidth*baseConfig.nInputHeight;
		bufferParam.nSizeC = baseConfig.nInputWidth*baseConfig.nInputHeight/2;
		bufferParam.nBufferNum = 4;

		baseConfig.eInputFormat = VENC_PIXEL_YUV420P;
	#ifdef SOURCE_YUV420SP
		p1 = NULL;
		p1 = malloc(bufferParam.nSizeC);
		if(p1 == NULL)
		{
			loge("malloc yuv420sp uv temp buffer error!!!\n");
			goto out;
		}
		baseConfig.eInputFormat = VENC_PIXEL_YUV420SP;
	#endif
		
		//1.starting >>>>>>>>>>>>>>>>>>>>>>>
		pVideoEnc = VideoEncCreate(codecType);
	
	
		if(codecType == VENC_CODEC_JPEG)
		{
			VideoEncSetParameter(pVideoEnc, VENC_IndexParamJpegExifInfo, &exifinfo);
		}
		else if(codecType == VENC_CODEC_H264)
		{
			int value;
			
			//2. >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
			VideoEncSetParameter(pVideoEnc, VENC_IndexParamH264Param, &h264Param);
	
			value = 1;
			VideoEncSetParameter(pVideoEnc, VENC_IndexParamIfilter, &value);
	
			value = 0; //degree
			VideoEncSetParameter(pVideoEnc, VENC_IndexParamRotation, &value);
	
			//VideoEncSetParameter(pVideoEnc, VENC_IndexParamH264FixQP, &fixQP);
	
			//VideoEncSetParameter(pVideoEnc, VENC_IndexParamH264CyclicIntraRefresh, &sIntraRefresh);
	
			value = 720/4;
			//VideoEncSetParameter(pVideoEnc, VENC_IndexParamSliceHeight, &value);
	
			//VideoEncSetParameter(pVideoEnc, VENC_IndexParamROIConfig, &sRoiConfig[0]);
			//VideoEncSetParameter(pVideoEnc, VENC_IndexParamROIConfig, &sRoiConfig[1]);
			//VideoEncSetParameter(pVideoEnc, VENC_IndexParamROIConfig, &sRoiConfig[2]);
			//VideoEncSetParameter(pVideoEnc, VENC_IndexParamROIConfig, &sRoiConfig[3]);
		}
	
		//3. >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
		VideoEncInit(pVideoEnc, &baseConfig);
	
		if(codecType == VENC_CODEC_H264)
		{
			unsigned int sps_num=0;
			//4. <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
			VideoEncGetParameter(pVideoEnc, VENC_IndexParamH264SPSPPS, &sps_pps_data);
			fwrite(sps_pps_data.pBuffer, 1, sps_pps_data.nLength, enc_output_stream);
			//write(enc_output_stream, sps_pps_data.pBuffer, sps_pps_data.nLength);
			logd("sps_pps_data.nLength: %d", sps_pps_data.nLength);
			for(sps_num; sps_num<sps_pps_data.nLength; sps_num++)
				logd("%02x\n",*(sps_pps_data.pBuffer+sps_num));
		}
	
		//5. <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		AllocInputBuffer(pVideoEnc, &bufferParam);
	
		while(1)
		{
			//6. <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
			GetOneAllocInputBuffer(pVideoEnc, &inputBuffer);
			{
				unsigned int size1, size2;
				
				//logd("frame num is %d, bufferParam.nSizeY is %d,\n",testNumber, bufferParam.nSizeY);
				size1 = fread(inputBuffer.pAddrVirY, 1, bufferParam.nSizeY, enc_input_stream);
				//size1 = read(enc_input_stream, inputBuffer.pAddrVirY, bufferParam.nSizeY);
				if((size1!= bufferParam.nSizeY) )
				{
					logd("file end need jump back file header size1=%d",size1);
					fseek(enc_input_stream, 0, SEEK_SET);
					//lseek(enc_input_stream, 0, SEEK_SET);
					size1 = fread(inputBuffer.pAddrVirY, 1, bufferParam.nSizeY, enc_input_stream);
					//size1 = read(enc_input_stream, inputBuffer.pAddrVirY, bufferParam.nSizeY);
				}
				size2 = fread(inputBuffer.pAddrVirC, 1, bufferParam.nSizeC, enc_input_stream);
				//size2 = read(enc_input_stream, inputBuffer.pAddrVirC, bufferParam.nSizeY/2);
			#ifdef SOURCE_YUV420SP
				p2 = p1;
				p3 = inputBuffer.pAddrVirC;
				p4 = p3 + bufferParam.nSizeY/4;
				
				for(j=0; j<bufferParam.nSizeY/4; j++)
				{
					*p2++ = *p3++;
					*p2++ = *p4++;
				}
				memcpy(inputBuffer.pAddrVirC, p1,  bufferParam.nSizeC);
			#endif
			}
	
			inputBuffer.bEnableCorp = 0;
			inputBuffer.sCropInfo.nLeft =  240;
			inputBuffer.sCropInfo.nTop	=  240;
			inputBuffer.sCropInfo.nWidth  =  240;
			inputBuffer.sCropInfo.nHeight =  240;
	
			//7. <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
			FlushCacheAllocInputBuffer(pVideoEnc, &inputBuffer);
	
			//8. <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
			AddOneInputBuffer(pVideoEnc, &inputBuffer);
			
			time1 = GetNowUs();
			VideoEncodeOneFrame(pVideoEnc);
			time2 = GetNowUs();
			static int totalFrame = 0;
			time3 += time2-time1;
			logd("encode frame %d use time is %lldus, avg(%.3f)..\n",testNumber, (time2-time1), (++totalFrame) / (time3 / 1000000.0));
	
			AlreadyUsedInputBuffer(pVideoEnc,&inputBuffer);
			ReturnOneAllocInputBuffer(pVideoEnc, &inputBuffer);
	
			GetOneBitstreamFrame(pVideoEnc, &outputBuffer);
			logi("size: %d,%d", outputBuffer.nSize0,outputBuffer.nSize1);
	
			fwrite(outputBuffer.pData0, 1, outputBuffer.nSize0, enc_output_stream);
			//write(enc_output_stream, outputBuffer.pData0, outputBuffer.nSize0);
	
			if(outputBuffer.nSize1)
			{
				fwrite(outputBuffer.pData1, 1, outputBuffer.nSize1, enc_output_stream);
				//write(enc_output_stream, outputBuffer.pData1, outputBuffer.nSize1);
			}
				
			FreeOneBitStreamFrame(pVideoEnc, &outputBuffer);
			//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>//

			testNumber++;

		#ifdef JPEG_ENCODE
			if(testNumber>0)
				break;
		#else
			if(testNumber>50)
				break;
		#endif
		
		}
		logd("the average encode time is %lldus...\n",time3/testNumber);
		logd("finish, output: %s", ENC_OUTPUT_STREAM_PATH);
		
	out:
		fclose(enc_output_stream);
		fclose(enc_input_stream);

		//close(enc_output_stream);
		//close(enc_input_stream);
	
		int result = VideoEncUnInit(pVideoEnc);
		if(result)
			loge("VideoEncUnInit error result=%d...\n",result);
			
		VideoEncDestroy(pVideoEnc);
		pVideoEnc = NULL;	
	
		return 0;
	}

