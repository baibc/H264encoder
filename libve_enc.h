#ifndef __LIBVE_ENC_H
#define __LIBVE_ENC_H

#include "vencoder.h"

int handleBeforeJob();
int handleAfterJob();
unsigned int libveENC_handle(VideoEncoder** pEncoder, unsigned char* pInBuf, unsigned char* pOutBuf, 
				    VencHeaderData* sps_pps_data, VencAllocateBufferParam* bufferParam);
int libveENC_init(VideoEncoder** pEncoder, VencBaseConfig* sBaseEncConfig, VencH264Param* h264Param);
int libveENC_exit(VideoEncoder** pEncoder);

#endif  /* __LIBVE_ENC_H */



