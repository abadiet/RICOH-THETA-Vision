#include <libuvc/libuvc.h>
#include <stdio.h>
#include <unistd.h>


#define UNUSED(x) (void)(x)


/* This callback function runs once per frame. Use it to perform any
 * quick processing you need, or have it put the frame into your application's
 * input queue. If this function takes too long, you'll start losing frames. */
void cb (uvc_frame_t *frame, void *arg) {
    uvc_frame_t *bgr;
    uvc_error_t ret;
    IplImage *img;

    /* Print info */
    printf ("callback! frame_format = %d, width = %d, height = %d, length = %lu\n",
        frame->frame_format, frame->width, frame->height, frame->data_bytes);

    /* Convert to BGR */
    if (
        frame->frame_format == UVC_FRAME_FORMAT_YUYV ||
        frame->frame_format == UVC_FRAME_FORMAT_UYVY
    ) {
        bgr = uvc_allocate_frame (frame->width * frame->height * 3);
        if (!bgr) {
            printf ("unable to allocate bgr frame!\n");
            return;
        }

        ret = uvc_any2bgr (frame, bgr);
        if (ret) {
            uvc_perror (ret, "uvc_any2bgr");
            uvc_free_frame (bgr);
            return;
        }

        ret = uvc_duplicate_frame (bgr, frame);
        if (ret) {
            uvc_perror (ret, "uvc_duplicate_frame");
            uvc_free_frame (bgr);
            return;
        }

        uvc_free_frame (bgr);
    }

    img = cvCreateImageHeader(
        cvSize(frame->width, frame->height),
        IPL_DEPTH_8U,
        3
    );
    cvSetData(img, frame->data, frame->width * 3); 
    cvNamedWindow("Test", CV_WINDOW_AUTOSIZE);
    cvShowImage("Test", img);
    cvWaitKey(10);
    cvReleaseImageHeader(&img);

    UNUSED (arg);
}


int main (int argc, char **argv) {
    uvc_context_t *ctx;
    uvc_device_t *dev;
    uvc_device_handle_t *devh;
    uvc_stream_ctrl_t ctrl;
    uvc_error_t res;

    res = uvc_init (&ctx, NULL);

    if (res < 0) {
        uvc_perror (res, "uvc_init");
        return res;
    }

    puts ("UVC initialized");

    res = uvc_find_device (ctx, &dev, 0, 0, NULL);

    if (res < 0) {
        uvc_perror(res, "uvc_find_device");
    } else {
        puts ("Device found");

        res = uvc_open (dev, &devh);

        if (res < 0) {
            uvc_perror (res, "uvc_open");
        } else {
            puts ("Device opened");

            /* Print out a message containing all the information that libuvc knows about the device */
            uvc_print_diag (devh, stderr);

            const uvc_format_desc_t * format_desc = uvc_get_format_descs (devh);
            while (format_desc && format_desc->bDescriptorSubtype == UVC_VS_FORMAT_MJPEG) {
                format_desc = format_desc->next;
            }
            if (!format_desc) {
                puts ("No non-MJPEG formats found");
            } else {

                const uvc_frame_desc_t *frame_desc = format_desc->frame_descs;
                enum uvc_frame_format frame_format;
                /* Default values */
                int width = 640;
                int height = 480;
                int fps = 30;

                switch (format_desc->bDescriptorSubtype) {
                    case UVC_VS_FORMAT_MJPEG:
                        /* Impossible */
                        frame_format = UVC_COLOR_FORMAT_MJPEG;
                        break;
                    case UVC_VS_FORMAT_FRAME_BASED:
                        frame_format = UVC_FRAME_FORMAT_H264;
                        break;
                    default:
                        frame_format = UVC_FRAME_FORMAT_YUYV;
                        break;
                }

                if (frame_desc) {
                    width = frame_desc->wWidth;
                    height = frame_desc->wHeight;
                    fps = 10000000 / frame_desc->dwDefaultFrameInterval;
                }

                printf("\nFirst non-MJPEG format: (%4s) %dx%d %dfps\n", format_desc->fourccFormat, width, height, fps);

                /* Try to negotiate first stream profile */
                res = uvc_get_stream_ctrl_format_size (
                    devh, &ctrl, /* result stored in ctrl */
                    frame_format,
                    width, height, fps
                );

                /* Print out the result */
                uvc_print_stream_ctrl (&ctrl, stderr);

                if (res < 0) {
                    uvc_perror (res, "get_mode"); /* device doesn't provide a matching stream */
                } else {
                    res = uvc_start_streaming (devh, &ctrl, cb, NULL, 0);

                    if (res < 0) {
                        uvc_perror (res, "start_streaming"); /* unable to start stream */
                    } else {
                        puts ("Streaming...");

                        uvc_set_ae_mode (devh, 1); /* e.g., turn on auto exposure */

                        sleep (3); /* stream for x seconds */

                        /* End the stream. Blocks until last callback is serviced */
                        uvc_stop_streaming (devh);
                        puts ("Done streaming.");
                    }
                }
            }

            uvc_close (devh);
            puts ("Device closed");
        }

        uvc_unref_device(dev);
    }

    uvc_exit (ctx);
    puts ("UVC exited");

    return 0;

    UNUSED (argc);
    UNUSED (argv);
}
