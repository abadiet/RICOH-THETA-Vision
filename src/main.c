#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <libuvc/libuvc.h>
#include "thetauvc.h"


#define UNUSED(x) (void)(x)

void callback(uvc_frame_t *frame, void *ptr){

    printf("Frame %u: %lu bytes\n", frame->sequence, frame->data_bytes);

    UNUSED (ptr);
	return;
}

int main(int argc, char **argv){
    uvc_context_t *ctx;
	uvc_device_t *dev;
	uvc_device_handle_t *devh;
    uvc_device_descriptor_t *desc;
	uvc_stream_ctrl_t ctrl;
	uvc_error_t res;

	res = uvc_init(&ctx, NULL);
	if (res != UVC_SUCCESS) {
		uvc_perror(res, "uvc_init");
		return 1;
	}

    if (argc > 1 && strcmp("-l", argv[1]) == 0) {
		thetauvc_print_devices(ctx, stdout);
		uvc_exit(ctx);
	    return 0;
	}

	res = thetauvc_find_device(ctx, &dev, 0);
	if (res != UVC_SUCCESS) {
		uvc_perror(res, "Device not found");
		uvc_exit(ctx);
	    return 1;
	}

    if (uvc_get_device_descriptor(dev, &desc) == UVC_SUCCESS) {
        printf("Using %s (SN:%s)\n", desc->product, desc->serialNumber);
    } else {
        printf ("Failed to get device descriptor\n");
    }
    uvc_free_device_descriptor(desc);

	res = uvc_open(dev, &devh);
	if (res != UVC_SUCCESS) {
		uvc_perror(res, "Can't open the device");
		uvc_exit(ctx);
	    return 1;
	}
	
	res = thetauvc_get_stream_ctrl_format_size(devh, THETAUVC_MODE_UHD_2997, &ctrl);

	res = uvc_start_streaming(devh, &ctrl, callback, NULL, 0);
	if (res == UVC_SUCCESS) {
		printf("Stream has begun, hit any key to stop it...\n");

		printf("Stream interrupted\n");

		uvc_stop_streaming(devh);
	}

	uvc_close(devh);
	uvc_exit(ctx);

    return 0;
}
