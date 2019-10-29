#include <stdio.h>
#include <string>
#include <video_codec_interface.h>
#include <openh264_encoder.h>
#include <x264_encoder.h>

long long current_us(void) {
    struct timeval tv_date;
    gettimeofday(&tv_date, NULL);
    return ((long long)tv_date.tv_sec * 1000000 + (long long)tv_date.tv_usec);
}

IvideoEncoder *CreateVideoEncoder(videoCodecConfig config, IvideoEncoderObserver *observer) {
    IvideoEncoder *encoder = NULL;
    switch(config.type) {
        case TYPE_ENC_OPENH264:
            encoder = new ENC_TEST::Openh264Encoder(config, observer);
            break;
        case TYPE_ENC_X264:
            encoder = new ENC_TEST::X264Encoder(config, observer);
        case TYPE_ENC_HW_OMX:
        case TYPE_ENC_R58_OMX:
        case TYPE_ENC_MT8765_OMX:
        case TYPE_ENC_MT8167_OMX:
        case TYPE_ENC_RK3326_OMX:
            break;
        default:
            break;
    }
    return encoder;
}

