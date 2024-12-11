#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <libuvc/libuvc.h>
#include <sys/time.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "thetauvc.h"
#include "conversion.h"
#include "projection.h"

#define UNUSED(x) (void)(x)


struct Callback_Args {
	Conversion_t conv;
	Projection_t projec;
	int dev_fd;
};

uvc_error_t Setup_UVC(uvc_device_handle_t **devh, uvc_stream_ctrl_t *ctrl, uvc_context_t **ctx, unsigned int mode);

int Open_Video(const char *device, __u32 frame_width, __u32 frame_height, __u32 frame_bytes);

void callback(uvc_frame_t *frame, void *ptr);


int main(int argc, char **argv) {
	uvc_device_handle_t *devh;
	uvc_stream_ctrl_t ctrl;
	uvc_context_t *ctx;
	uvc_error_t res;
	struct Callback_Args args;

	const unsigned int thetauvc_mode = THETAUVC_MODE_UHD_2997;
	const unsigned int width_raw_frame = stream_mode[thetauvc_mode].width;
	const unsigned int height_raw_frame = stream_mode[thetauvc_mode].height;
	const unsigned int channels = 3;
	const unsigned int width = 1496;
	const unsigned int height = 1000;
	const double fov_x = 3.14 / 2.0;
	const double fov_y = fov_x * height / width;
	const double center_lat = 0.0;
	const double center_lon = 0.0;
	args.projec = Init_Projection(width_raw_frame, height_raw_frame, channels, center_lat, center_lon, width, height, fov_x, fov_y);

	args.conv = Init_Conversion(AV_CODEC_ID_H264, AV_PIX_FMT_BGR24, channels);

	res = Setup_UVC(&devh, &ctrl, &ctx, thetauvc_mode);
	if (res != UVC_SUCCESS) {
		return res;
	}

	args.dev_fd = Open_Video("/dev/video0", (__u32) width, (__u32) height, (__u32) (width * height * channels));
	if (args.dev_fd < 0) {
		return -1;
	}

	res = uvc_start_streaming(devh, &ctrl, callback, &args, 0);
	if (res == UVC_SUCCESS) {
		printf("Streaming...\n");

		usleep(30000000);

		uvc_stop_streaming(devh);
		printf("Done streaming.\n");
	} else {
		uvc_perror(res, "Failed to start streaming");
	}

	uvc_close(devh);
	uvc_exit(ctx);

	Free_Conversion(&args.conv);
	Free_Projection(&args.projec);
	close(args.dev_fd);

	return res;

	UNUSED(argc);
	UNUSED(argv);
}


uvc_error_t Setup_UVC(uvc_device_handle_t **devh, uvc_stream_ctrl_t *ctrl, uvc_context_t **ctx, unsigned int mode) {
	uvc_device_t *dev;
	uvc_error_t res;

	res = uvc_init(ctx, NULL);
	if (res != UVC_SUCCESS) {
		uvc_perror(res, "uvc_init");
		return res;
	}

	res = thetauvc_find_device(*ctx, &dev, 0);
	if (res != UVC_SUCCESS) {
		fprintf(stderr, "THETA not found\n");
		return res;
	}

	res = uvc_open(dev, devh);
	if (res != UVC_SUCCESS) {
		fprintf(stderr, "Can't open THETA\n");
		return res;
	}

	uvc_print_diag(*devh, stdout);

	res = thetauvc_get_stream_ctrl_format_size(*devh, mode, ctrl);

	res = uvc_set_ae_mode(*devh, 2);			/* auto exposure */
	if (res != UVC_SUCCESS) uvc_perror(res, "AE mode control not applied");
	/* uvc_set_focus_auto
	uvc_set_digital_roi */

	return UVC_SUCCESS;
}


int Open_Video(const char *device, __u32 frame_width, __u32 frame_height, __u32 frame_bytes) {
	struct v4l2_format v;
	int dev_fd;

	dev_fd = open(device, O_WRONLY);
	if (dev_fd == -1) {
		printf("Cannot open video device\n");
		return -1;
	}

	v.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	v.fmt.pix.width = frame_width;
	v.fmt.pix.height = frame_height;
	v.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR24;
	v.fmt.pix.sizeimage = frame_bytes;
	v.fmt.pix.field = V4L2_FIELD_NONE;
	if (ioctl(dev_fd, VIDIOC_S_FMT, &v) == -1) {
		printf("Cannot setup video device\n");
		return -1;
	}

	return dev_fd;
}


void callback(uvc_frame_t *frame, void *ptr) {
	struct timeval start, end;
	uint8_t *frame_bgr24, *frame_perspective;
	size_t frame_perspective_sz;
	int res;
	ssize_t write_sz;
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

		/* Send to video device */
		write_sz = write(args->dev_fd, frame_perspective, frame_perspective_sz);
		if (write_sz != (ssize_t) frame_perspective_sz) {
			printf("Failed to write to video device\n");
			return;
		}

		free(frame_bgr24);
		free(frame_perspective);
	
		/* frame_count++; */

	} else {
		printf("Unexpected format %d\n", frame->frame_format);
	}

	gettimeofday(&end, NULL);
	printf("Callback execution time: %lu us\n", (end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec);
}
