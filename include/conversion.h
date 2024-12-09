#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

#if defined(__cplusplus)
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>


typedef struct Conversion* Conversion_t;


Conversion_t Init_Conversion();

int H264_to_BGR24(Conversion_t conv, uint8_t** dst, uint8_t* src, size_t size);

void Free_Conversion(Conversion_t* conv);


#if defined(__cplusplus)
}
#endif

#endif
