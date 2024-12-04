#include <stdio.h>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}
#include <opencv2/opencv.hpp>


int main() {
    FILE* fp = fopen(".test/2.h264", "rb");
    size_t size;
    void* data;
    fseek(fp, 0, SEEK_END);
    size = (size_t) ftell(fp);
    fseek(fp, 0, SEEK_SET);
    data = malloc(size);
    if (!data) {
        printf("Failed to allocate memory\n");
        fclose(fp);
        return 1;
    }
    if (fread(data, 1, size, fp) != size) {
        printf("Failed to read file\n");
        free(data);
        fclose(fp);
        return 1;
    }
    fclose(fp);


    AVPacket* packet = av_packet_alloc();
    packet->data = (uint8_t*) data;
    packet->size = (int) size;

    AVFrame *decoded_frame = av_frame_alloc();
    if (!decoded_frame) {
        printf("Failed to allocate frame\n");
        return 1;
    }

    AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec){
        printf("Could not found codec by given id");
        return 1;
    }

    AVCodecContext* context = avcodec_alloc_context3(codec);
    if (!context){
        printf("Could not allocate avcodec context");
        return 1;
    }

    if (avcodec_open2(context, codec, NULL) < 0){
        printf("Could not initialize avcodec context");
        return 1;
    }

    int res = avcodec_send_packet(context, packet);
    if (res < 0) {
        printf("Error sending packet\n");
        av_frame_free(&decoded_frame);
        avcodec_free_context(&context);
        return 1;
    }

    res = avcodec_receive_frame(context, decoded_frame);
    if (res < 0) {
        printf("Error decoding frame\n");
        av_frame_free(&decoded_frame);
        avcodec_free_context(&context);
        return 1;
    }

    cv::Mat frame(decoded_frame->height, decoded_frame->width, CV_8UC3);
    int cvLinesize[1];
    cvLinesize[0] = (int) frame.step1();

    sws_scale(sws_getContext(decoded_frame->width, decoded_frame->height, (AVPixelFormat)decoded_frame->format,
                             decoded_frame->width, decoded_frame->height, AV_PIX_FMT_BGR24,
                             SWS_BILINEAR, NULL, NULL, NULL),
              decoded_frame->data, decoded_frame->linesize, 0, decoded_frame->height,
              &frame.data, cvLinesize);

    cv::imshow("Decoded Frame", frame);
    cv::waitKey(0);

    av_frame_free(&decoded_frame);
    avcodec_free_context(&context);
    free(data);

    return 0;
}