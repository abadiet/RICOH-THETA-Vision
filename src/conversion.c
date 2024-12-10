#include "conversion.h"


struct Conversion {
	AVPacket* packet;
	AVFrame* decoded_frame;
	AVCodec* codec;
	AVCodecContext* avcontext;
	int bytes_per_pixel;
};


Conversion_t Init_Conversion(enum AVCodecID from_fmt, enum AVPixelFormat to_fmt, unsigned int channels) {
	Conversion_t conv;
	const AVPixFmtDescriptor* desc;
	int res;

	conv = (Conversion_t) malloc(sizeof(struct Conversion));
	if (!conv){
		printf("Failed to allocate conversion struct\n");
		return NULL;
	}

	conv->packet = av_packet_alloc();
	if (!conv->packet) {
		printf("Failed to allocate packet\n");
		free(conv);
		return NULL;
	}

	conv->decoded_frame = av_frame_alloc();
	if (!conv->decoded_frame) {
		printf("Failed to allocate frame\n");
		av_packet_free(&conv->packet);
		free(conv);
		return NULL;
	}

	conv->codec = avcodec_find_decoder(from_fmt);
	if (!conv->codec){
		printf("Could not found codec by given id\n");
		av_packet_free(&conv->packet);
		av_frame_free(&conv->decoded_frame);
		free(conv);
		return NULL;
	}

	conv->avcontext = avcodec_alloc_context3(conv->codec);
	if (!conv->avcontext){
		printf("Could not allocate avcodec context\n");
		av_packet_free(&conv->packet);
		av_frame_free(&conv->decoded_frame);
		free(conv);
		return NULL;
	}

	res = avcodec_open2(conv->avcontext, conv->codec, NULL);
	if (res < 0){
		printf("Could not initialize avcodec context\n");
		av_packet_free(&conv->packet);
		av_frame_free(&conv->decoded_frame);
		avcodec_free_context(&conv->avcontext);
		free(conv);
		return NULL;
	}

	desc = av_pix_fmt_desc_get(to_fmt);
	if (!desc){
		printf("Can't get descriptor for pixel format\n");
		av_packet_free(&conv->packet);
		av_frame_free(&conv->decoded_frame);
		avcodec_free_context(&conv->avcontext);
		free(conv);
		return NULL;
	}
	conv->bytes_per_pixel = av_get_bits_per_pixel(desc) / 8;
	if (!(conv->bytes_per_pixel == (int) channels && !(av_get_bits_per_pixel(desc) % 8))){
		printf("Unhandled bits per pixel, bad in pix fmt\n");
		av_packet_free(&conv->packet);
		av_frame_free(&conv->decoded_frame);
		avcodec_free_context(&conv->avcontext);
		free(conv);
		return NULL;
	}

	return conv;
}


int H264_to_BGR24(Conversion_t conv, uint8_t** dst, uint8_t* src, size_t size) {
	struct SwsContext* swscontext;
	int cv_linesize[1];
	int res;

	conv->packet->data = src;
	conv->packet->size = (int) size;

	res = avcodec_send_packet(conv->avcontext, conv->packet);
	if (res < 0) {
		printf("Error sending packet\n");
		return 0;
	}

	res = avcodec_receive_frame(conv->avcontext, conv->decoded_frame);
	if (res < 0) {
		printf("Error decoding frame\n");
		return 0;
	}

	*dst = (uint8_t*) malloc((size_t) (conv->bytes_per_pixel * conv->decoded_frame->width * conv->decoded_frame->height));
	if (!*dst){
		printf("Failed to allocate frame buffer\n");
		return 0;
	}

	swscontext = sws_getContext(
		conv->decoded_frame->width, conv->decoded_frame->height, conv->decoded_frame->format,
		conv->decoded_frame->width, conv->decoded_frame->height, AV_PIX_FMT_BGR24,
		SWS_BILINEAR, NULL, NULL, NULL
	);
	if (!swscontext){
		printf("Failed to create sws context\n");
		return 0;
	}

	cv_linesize[0] = conv->bytes_per_pixel * conv->decoded_frame->width;

	res = sws_scale(
		swscontext,
		(const uint8_t**) conv->decoded_frame->data, conv->decoded_frame->linesize, 0, conv->decoded_frame->height,
		dst, cv_linesize
	);
	if (res != conv->decoded_frame->height){
		printf("Failed to scale frame\n");
		sws_freeContext(swscontext);
		return 0;
	}

	sws_freeContext(swscontext);

	return 1;
}


void Free_Conversion(Conversion_t* conv) {
	if (!conv || !*conv) return;

	if ((*conv)->decoded_frame) av_frame_free(&(*conv)->decoded_frame);
	if ((*conv)->avcontext) avcodec_free_context(&(*conv)->avcontext);
	if ((*conv)->packet) av_packet_free(&(*conv)->packet);

	free(*conv);
	*conv = NULL;
}
