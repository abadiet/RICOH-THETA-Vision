#include <thetauvc.h>
#include <stdio.h>
#include <unistd.h>


#define UNUSED(x) (void)(x)

int main (int argc, char **argv) {
    uvc_context_t *ctx;
	uvc_device_t **devlist;
	uvc_error_t res;
    int idx = 0;

	res = uvc_init(&ctx, NULL);
	if (res != UVC_SUCCESS) {
		uvc_perror(res, "uvc_init");
		return res;
	}

    res = thetauvc_find_devices(ctx, &devlist);
    if (res != UVC_SUCCESS) {
        uvc_perror(res,"");
        uvc_exit(ctx);
        return res;
    }

	printf("No : %-18s : %-10s\n", "Product", "Serial");
    while (devlist[idx] != NULL) {
        uvc_device_descriptor_t *desc;

        if (uvc_get_device_descriptor(devlist[idx], &desc) != UVC_SUCCESS)
            continue;

        printf("%2d : %-18s : %-10s\n", idx, desc->product,
            desc->serialNumber);

        uvc_free_device_descriptor(desc);
        idx++;
    }

    uvc_free_device_list(devlist, 1);
    uvc_exit(ctx);

    UNUSED (argc);
    UNUSED (argv);
}
