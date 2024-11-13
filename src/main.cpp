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
    /* FILE *fp;
    * static int jpeg_count = 0;
    * static const char *H264_FILE = "iOSDevLog.h264";
    * static const char *MJPEG_FILE = ".jpeg";
    * char filename[16]; */

    /* We'll convert the image from YUV/JPEG to BGR, so allocate space */
    bgr = uvc_allocate_frame (frame->width * frame->height * 3);
    if (!bgr) {
        printf ("unable to allocate bgr frame!\n");
        return;
    }

    printf ("callback! frame_format = %d, width = %d, height = %d, length = %lu\n",
        frame->frame_format, frame->width, frame->height, frame->data_bytes);

    switch (frame->frame_format) {
        case UVC_FRAME_FORMAT_H264:
            /* use `ffplay H264_FILE` to play */
            /* fp = fopen(H264_FILE, "a");
            * fwrite(frame->data, 1, frame->data_bytes, fp);
            * fclose(fp); */
            break;
        case UVC_COLOR_FORMAT_MJPEG:
            /* sprintf(filename, "%d%s", jpeg_count++, MJPEG_FILE);
            * fp = fopen(filename, "w");
            * fwrite(frame->data, 1, frame->data_bytes, fp);
            * fclose(fp); */
            break;
        case UVC_COLOR_FORMAT_YUYV:
            /* Do the BGR conversion */
            ret = uvc_any2bgr (frame, bgr);
            if (ret) {
                uvc_perror (ret, "uvc_any2bgr");
                uvc_free_frame (bgr);
                return;
            }
            break;
        default:
            break;
    }

    /* Call a user function:
    *
    * my_type *my_obj = (*my_type) ptr;
    * my_user_function(ptr, bgr);
    * my_other_function(ptr, bgr->data, bgr->width, bgr->height);
    */

    /* Call a C++ method:
    *
    * my_type *my_obj = (*my_type) ptr;
    * my_obj->my_func(bgr);
    */

    /* Use opencv.highgui to display the image:
    * 
    * cvImg = cvCreateImageHeader(
    *     cvSize(bgr->width, bgr->height),
    *     IPL_DEPTH_8U,
    *     3);
    *
    * cvSetData(cvImg, bgr->data, bgr->width * 3); 
    *
    * cvNamedWindow("Test", CV_WINDOW_AUTOSIZE);
    * cvShowImage("Test", cvImg);
    * cvWaitKey(10);
    *
    * cvReleaseImageHeader(&cvImg);
    */

    uvc_free_frame (bgr);

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

            const uvc_format_desc_t *format_desc = uvc_get_format_descs (devh);
            const uvc_frame_desc_t *frame_desc = format_desc->frame_descs;
            enum uvc_frame_format frame_format;
            /* Default values */
            int width = 640;
            int height = 480;
            int fps = 30;

            switch (format_desc->bDescriptorSubtype) {
                case UVC_VS_FORMAT_MJPEG:
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

            printf("\nFirst format: (%4s) %dx%d %dfps\n", format_desc->fourccFormat, width, height, fps);

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

                    sleep (10); /* stream for 10 seconds */

                    /* End the stream. Blocks until last callback is serviced */
                    uvc_stop_streaming (devh);
                    puts ("Done streaming.");
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
