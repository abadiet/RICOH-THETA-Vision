#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <libuvc/libuvc.h>
#include <sys/time.h>
#include "thetauvc.h"
#include "conversion.h"

#include <opencv2/opencv.hpp>


#define UNUSED(x) (void)(x)


void callback (uvc_frame_t *frame, void *ptr);


int main(int argc, char **argv) {
	uvc_context_t *ctx;
	uvc_device_t *dev;
	uvc_device_handle_t *devh;
	uvc_stream_ctrl_t ctrl;
	uvc_error_t res;
    Conversion_t conv;

    conv = Init_Conversion();

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

	res = uvc_start_streaming(devh, &ctrl, callback, conv, 0);
	if (res == UVC_SUCCESS) {
		printf("Streaming...\n");
		usleep(500000);
		uvc_stop_streaming(devh);
		printf("Done streaming.\n");
	}

	uvc_close(devh);
	uvc_exit(ctx);

    Free_Conversion(&conv);

	return res;

	UNUSED(argc);
	UNUSED(argv);
}


void callback(uvc_frame_t *frame, void *ptr) {
    struct timeval start, end;
	uint8_t* frame_bgr24;
	int res;
    static int frame_count = 0;
    char filename[16];

    gettimeofday(&start, NULL);
    printf("Received a frame: frame_format = %d, width = %d, height = %d, length = %lu\n", frame->frame_format, frame->width, frame->height, frame->data_bytes);

	if (frame->frame_format == UVC_FRAME_FORMAT_H264) {

		/* Save H264 */
		/* fp = fopen("frame.h264", "w");
		fwrite(frame->data, 1, frame->data_bytes, fp);
		fclose(fp); */

		/* Convert to BGR24 */
		res = H264_to_BGR24((Conversion_t) ptr, &frame_bgr24, (uint8_t*) frame->data, (int) frame->data_bytes);
		if (!res) {
			printf("Failed to convert to BGR24\n");
			return;
		}

		/* Save BGR24 */
        cv::Mat img(frame->height, frame->width, CV_8UC3, frame_bgr24);
        sprintf(filename, ".test/%d.jpg", frame_count++);
        cv::imwrite(filename, img);

        free(frame_bgr24);

	} else {
		printf("Unexpected format %d\n", frame->frame_format);
	}

    gettimeofday(&end, NULL);
    printf("Callback execution time: %lu us\n", (end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec);
}
