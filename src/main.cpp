#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <libuvc/libuvc.h>
#include "thetauvc.h"

#if defined(__cplusplus)
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>

#if defined(__cplusplus)
}
#endif

#include <opencv2/opencv.hpp>


#define UNUSED(x) (void)(x)


int h264_to_bgr24(uint8_t** dst, uint8_t* src, int size);


void callback (uvc_frame_t *frame, void *ptr);


int main(int argc, char **argv) {
	uvc_context_t *ctx;
	uvc_device_t *dev;
	uvc_device_handle_t *devh;
	uvc_stream_ctrl_t ctrl;
	uvc_error_t res;

	res = uvc_init(&ctx, NULL);
	if (res != UVC_SUCCESS) {
		uvc_perror(res, "uvc_init");
		return res;
	}

	res = thetauvc_find_device(ctx, &dev, 0);
	if (res != UVC_SUCCESS) {
		fprintf(stderr, "THETA not found\n");
		uvc_exit(ctx);
		return res;
	}

	res = uvc_open(dev, &devh);
	if (res != UVC_SUCCESS) {
		fprintf(stderr, "Can't open THETA\n");
		uvc_exit(ctx);
		return res;
	}

	uvc_print_diag(devh, stdout);

	res = thetauvc_get_stream_ctrl_format_size(devh, THETAUVC_MODE_UHD_2997, &ctrl);

	res = uvc_set_ae_mode(devh, 2);			/* auto exposure */
	if (res != UVC_SUCCESS) uvc_perror(res, "AE mode control not applied");
	/* uvc_set_focus_auto
	uvc_set_digital_roi */

	res = uvc_start_streaming(devh, &ctrl, callback, NULL, 0);
	if (res == UVC_SUCCESS) {
		printf("Streaming...\n");
		usleep(100000);
		uvc_stop_streaming(devh);
		printf("Done streaming.\n");
	}

	uvc_close(devh);
	uvc_exit(ctx);
	return res;

	UNUSED(argc);
	UNUSED(argv);
}


int h264_to_bgr24(uint8_t** dst, uint8_t* src, int size) {
	AVPacket* packet;
    AVFrame* decoded_frame;
    AVCodec* codec;
    AVCodecContext* avcontext;
    const AVPixFmtDescriptor* desc;
    int bytes_per_pixel;
    int cv_linesize[1];
    SwsContext* swscontext;
    int res;

    packet = av_packet_alloc();
    if (!packet) {
        printf("Failed to allocate packet\n");
        return 0;
    }
    packet->data = src;
    packet->size = size;

    decoded_frame = av_frame_alloc();
    if (!decoded_frame) {
        printf("Failed to allocate frame\n");
        av_packet_free(&packet);
        return 0;
    }

    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec){
        printf("Could not found codec by given id\n");
        av_packet_free(&packet);
        av_frame_free(&decoded_frame);
        return 0;
    }

    avcontext = avcodec_alloc_context3(codec);
    if (!avcontext){
        printf("Could not allocate avcodec context\n");
        av_packet_free(&packet);
        av_frame_free(&decoded_frame);
        return 0;
    }

    res = avcodec_open2(avcontext, codec, NULL);
    if (res < 0){
        printf("Could not initialize avcodec context\n");
        av_packet_free(&packet);
        av_frame_free(&decoded_frame);
        avcodec_free_context(&avcontext);
        return 0;
    }

    res = avcodec_send_packet(avcontext, packet);
    if (res < 0) {
        printf("Error sending packet\n");
        av_packet_free(&packet);
        av_frame_free(&decoded_frame);
        avcodec_free_context(&avcontext);
        return 0;
    }

    res = avcodec_receive_frame(avcontext, decoded_frame);
    if (res < 0) {
        printf("Error decoding frame\n");
        av_packet_free(&packet);
        av_frame_free(&decoded_frame);
        avcodec_free_context(&avcontext);
        return 0;
    }

    desc = av_pix_fmt_desc_get(AV_PIX_FMT_RGB24);
    if (!desc){
        printf("Can't get descriptor for pixel format\n");
        av_packet_free(&packet);
        av_frame_free(&decoded_frame);
        avcodec_free_context(&avcontext);
        return 0;
    }
    bytes_per_pixel = av_get_bits_per_pixel(desc) / 8;
    if (!(bytes_per_pixel==3 && !(av_get_bits_per_pixel(desc) % 8))){
        printf("Unhandled bits per pixel, bad in pix fmt\n");
        av_packet_free(&packet);
        av_frame_free(&decoded_frame);
        avcodec_free_context(&avcontext);
        return 0;
    }
    cv_linesize[0] = bytes_per_pixel * decoded_frame->width;

    *dst = (uint8_t*) malloc(bytes_per_pixel * decoded_frame->width * decoded_frame->height);
    if (!*dst){
        printf("Failed to allocate frame buffer\n");
        av_packet_free(&packet);
        av_frame_free(&decoded_frame);
        avcodec_free_context(&avcontext);
        return 0;
    }

    swscontext = sws_getContext(
        decoded_frame->width, decoded_frame->height, (AVPixelFormat)decoded_frame->format,
        decoded_frame->width, decoded_frame->height, AV_PIX_FMT_BGR24,
        SWS_BILINEAR, NULL, NULL, NULL
    );
    if (!swscontext){
        printf("Failed to create sws context\n");
        av_packet_free(&packet);
        av_frame_free(&decoded_frame);
        avcodec_free_context(&avcontext);
        return 0;
    }

    res = sws_scale(
        swscontext,
        decoded_frame->data, decoded_frame->linesize, 0, decoded_frame->height,
        dst, cv_linesize
    );
    if (res != decoded_frame->height){
        printf("Failed to scale frame\n");
        av_packet_free(&packet);
        av_frame_free(&decoded_frame);
        avcodec_free_context(&avcontext);
        sws_freeContext(swscontext);
        return 0;
    }

    av_packet_free(&packet);
    av_frame_free(&decoded_frame);
    avcodec_free_context(&avcontext);
    sws_freeContext(swscontext);

    return 1;
}


void callback (uvc_frame_t *frame, void *ptr) {
	uint8_t* frame_bgr24;
	int res;

	printf("received: frame_format = %d, width = %d, height = %d, length = %lu\n", frame->frame_format, frame->width, frame->height, frame->data_bytes);
	
	if (frame->frame_format == UVC_FRAME_FORMAT_H264) {

		/* Save H264 */
		/* fp = fopen("frame.h264", "w");
		fwrite(frame->data, 1, frame->data_bytes, fp);
		fclose(fp); */

		/* Convert to BGR24 */
		res = h264_to_bgr24(&frame_bgr24, (uint8_t*) frame->data, (int) frame->data_bytes);
		if (!res) {
			printf("Failed to convert to BGR24\n");
			return;
		}

		cv::Mat test(frame->height, frame->width, CV_8UC3, frame_bgr24);
		cv::imshow("Decoded Frame", test);
		cv::waitKey(0);

        free(frame_bgr24);

	} else {
		printf("Unexpected format %d\n", frame->frame_format);
	}

	UNUSED(ptr);
}
