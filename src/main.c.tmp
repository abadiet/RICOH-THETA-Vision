#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <libuvc/libuvc.h>
#include "thetauvc.h"


#define UNUSED(x) (void)(x)


void callback (uvc_frame_t *frame, void *ptr) {
	FILE* fp = NULL;
	static int frame_count = 0;
	char filename[16];

	printf("received: frame_format = %d, width = %d, height = %d, length = %lu,\n", frame->frame_format, frame->width, frame->height, frame->data_bytes);
	
	if (frame->frame_format == UVC_FRAME_FORMAT_H264) {
		sprintf(filename, ".test/%d.h264", frame_count);
		fp = fopen(filename, "w");
		fwrite(frame->data, 1, frame->data_bytes, fp);
		fclose(fp);
		frame_count++;
	} else {
		printf("[ERROR] Unexpected format %d", frame->frame_format);
	}

	UNUSED(ptr);
}


int main(int argc, char **argv) {
	uvc_context_t *ctx;
	uvc_device_t *dev;
	uvc_device_handle_t *devh;
	uvc_stream_ctrl_t ctrl;
	uvc_error_t res;
	uint16_t roi_top, roi_left, roi_bottom, roi_right, roi_autocontrols;

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
