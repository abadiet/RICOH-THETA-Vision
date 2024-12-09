#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <libuvc/libuvc.h>
#include <sys/time.h>
#include "thetauvc.h"
#include "conversion.h"
#include "projection.h"

#define UNUSED(x) (void)(x)


struct Callback_Args {
	Conversion_t conv;
	Projection_t projec;
};


void callback (uvc_frame_t *frame, void *ptr);


int main(int argc, char **argv) {
	uvc_context_t *ctx;
	uvc_device_t *dev;
	uvc_device_handle_t *devh;
	uvc_stream_ctrl_t ctrl;
	uvc_error_t res;
	struct Callback_Args args;

	const int width_raw_frame = 3840;
	const int height_raw_frame = 1920;
	const int channels = 3;
	const int width = 1500;
	const int height = 1000;
	const double fov_x = 3.14 / 2.0;
	const double fov_y = fov_x * height / width;
    const double center_lat = 0.0;
    const double center_lon = 0.0;
    args.projec = Init_Projection(width_raw_frame, height_raw_frame, channels, center_lat, center_lon, width, height, fov_x, fov_y);

    args.conv = Init_Conversion();

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

	res = uvc_start_streaming(devh, &ctrl, callback, &args, 0);
	if (res == UVC_SUCCESS) {
		printf("Streaming...\n");
		usleep(500000);
		uvc_stop_streaming(devh);
		printf("Done streaming.\n");
	}

	uvc_close(devh);
	uvc_exit(ctx);

    Free_Conversion(&args.conv);
	Free_Projection(&args.projec);

	return res;

	UNUSED(argc);
	UNUSED(argv);
}


void callback(uvc_frame_t *frame, void *ptr) {
    struct timeval start, end;
	uint8_t *frame_bgr24, *frame_perspective;
	size_t frame_perspective_sz;
	int res;
	/* FILE *fp;
    static int frame_count = 0;
    char filename[32]; */
	struct Callback_Args *args = (struct Callback_Args *) ptr;

    gettimeofday(&start, NULL);
    printf("Received a frame: frame_format = %d, width = %d, height = %d, length = %lu\n", frame->frame_format, frame->width, frame->height, frame->data_bytes);

	if (frame->frame_format == UVC_FRAME_FORMAT_H264) {

		/* Save H264 */
        /* sprintf(filename, ".test123/%d.h264", frame_count);
		fp = fopen(filename, "w");
		fwrite(frame->data, 1, frame->data_bytes, fp);
		fclose(fp); */

		/* Convert to BGR24 */
		res = H264_to_BGR24(args->conv, &frame_bgr24, (uint8_t*) frame->data, frame->data_bytes);
		if (!res) {
			printf("Failed to convert to BGR24\n");
			return;
		}

		/* Save BGR24 */
        /* sprintf(filename, ".test123/%d.bgr24", frame_count);
		fp = fopen(filename, "w");
		fwrite(frame_bgr24, sizeof(uint8_t), frame->width * frame->height * 3 * sizeof(uint8_t), fp);
		fclose(fp); */

        /* Project the equirectangular image to a perspective one */
		frame_perspective_sz = Equirectangular_to_Perspective(args->projec, &frame_perspective, frame_bgr24, false);
		if (!frame_perspective_sz) {
			printf("Failed to project to perspective\n");
			return;
		}

        /* Save result */
		/* sprintf(filename, ".test123/%d.jpg", frame_count);
		fp = fopen(filename, "w");
		fwrite(frame_perspective, 1, frame_perspective_sz, fp);
		fclose(fp); */

        free(frame_bgr24);
		free(frame_perspective);
    
        /* frame_count++; */

	} else {
		printf("Unexpected format %d\n", frame->frame_format);
	}

    gettimeofday(&end, NULL);
    printf("Callback execution time: %lu us\n", (end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec);
}
