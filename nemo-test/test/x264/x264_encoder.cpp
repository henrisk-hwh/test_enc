#include <string.h>
#include <x264_encoder.h>

#include <x264.h>

// depending in definition of x264_profile_names[] in x264.h
#define X264_PROFILE_BASELINE 0
#define X264_PROFILE_MAIN     1
#define X264_PROFILE_HIGH     2
#define X264_PROFILE_HIGH10   3
#define X264_PROFILE_HIGH422  4
#define X264_PROFILE_HIGH444  5

// depending in definition of x264_preset_names[] in x264.h
#define X264_PRESET_ULTRAFAST  0
#define X264_PRESET_SUPERFAST  1
#define X264_PRESET_VERYFAST   2
#define X264_PRESET_FASTER     3
#define X264_PRESET_FAST       4
#define X264_PRESET_MEDIUM     5
#define X264_PRESET_SLOW       6
#define X264_PRESET_SLOWER     7
#define X264_PRESET_VERYSLOW   8
#define X264_PRESET_PLACEBO    9

// depending in definition of x264_tune_names[] in x264.h
#define X264_TUNE_FILM        0
#define X264_TUNE_ANIMATION   1
#define X264_TUNE_GRAIN       2
#define X264_TUNE_STILLIMAGE  3
#define X264_TUNE_PSNR        4
#define X264_TUNE_SSIM        5
#define X264_TUNE_FASTDECODE  6
#define X264_TUNE_ZEROLATENCY 7

typedef enum
{
    Y_PLANE = 0,
    U_PLANE,
    V_PLANE,
    PLANE_COUNT
} IMAGE_PLANES;

namespace ENC_TEST {

class X264EncoderImpl {
private:
    friend X264Encoder;

    X264EncoderImpl(videoCodecConfig &config, IvideoEncoderObserver *observer);
    ~X264EncoderImpl();

    bool Destroy();
    int encode(encodeFrameInfo &frameinfo);
    void reconfig(videoCodecConfig &config);

private:
    bool open_encoder();
    void close_encoder();
private:
    bool _initialized;

    x264_t *_x264;
    int _index = 0;
    videoCodecConfig &_config;
    IvideoEncoderObserver *_observer = NULL;
};

void X264EncoderImpl::close_encoder()
{
    if (_x264 != NULL) {
        x264_encoder_close(_x264);
        _x264 = NULL;
        _initialized = false;
    }

}

bool X264EncoderImpl::open_encoder()
{
    LOGI("X264Encoder, version: %s, pointver: %s\n", X264_VERSION, X264_POINTVER);

    x264_param_t param;
    x264_param_default_preset( &param,
                               x264_preset_names[X264_PRESET_VERYFAST],
                               x264_tune_names[X264_TUNE_ZEROLATENCY]); // yangfan
    //PRESET_VERYFAST
    //param.i_threads = X264_THREADS_AUTO;
    //param.analyse.i_me_method = X264_ME_DIA;
    //param.analyse.i_subpel_refine = 2;
    // param.rc.i_lookahead = 10;
    //param.i_bframe = 3;
    param.i_level_idc = 50;
    param.i_threads = 3;
    param.b_sliced_threads = 1;
    param.i_lookahead_threads = 0;
    // yangfan
    // param.b_vfr_input = 1;

    // if (_param.isHighProfile)
    //     x264_param_apply_profile(&param, x264_profile_names[X264_PROFILE_HIGH]);
    // else
        x264_param_apply_profile(&param, x264_profile_names[X264_PROFILE_BASELINE]);


    param.i_bframe = 0;
    LOGI("x264encoder b frames is %d\n", param.i_bframe);

    //useBitrateControl = true
    param.rc.i_rc_method = X264_RC_ABR;

    // yangfan
    // param.i_keyint_max = 3 * _param.inputFrameRate;
    // param.i_keyint_min = 3 * _param.inputFrameRate;
    param.i_keyint_max = 0x7fffffff;
    param.i_keyint_min = 0x7fffffff;
    param.rc.i_lookahead = 0;
    param.rc.b_mb_tree = 0;
    param.rc.i_aq_mode = X264_AQ_AUTOVARIANCE;
    param.b_repeat_headers = 1;
    param.b_annexb = 1;

    param.i_width = _config.width;
    param.i_height = _config.height;

    if (_config.fps == 7.5) {
        param.i_fps_den = 2;
        param.i_fps_num = 15;
    } else {
        param.i_fps_den = 1;
        param.i_fps_num = _config.fps;
    }

    // param.i_fps_den = 1;
    // param.i_fps_num = 25;

    // svc-t did not supported by x264
    param.rc.i_bitrate = _config.bitrate/1000;


    LOGI("X264EncoderImpl, config bitrate %d\r\n", param.rc.i_bitrate);

    param.rc.i_qp_min = 10;
    param.rc.i_qp_max = 40;
    param.rc.i_vbv_max_bitrate = 2 * param.rc.i_bitrate;
    param.rc.i_vbv_buffer_size = 2 * param.rc.i_vbv_max_bitrate;

    _x264 = x264_encoder_open(&param);

    if (_x264 != NULL) {
        _initialized = true;
    }

    return _initialized;
}

X264EncoderImpl::X264EncoderImpl(videoCodecConfig &config, IvideoEncoderObserver *observer) :
    _observer(observer),
    _config(config),
    _initialized(false)
{
    open_encoder();
}

bool X264EncoderImpl::Destroy()
{
    close_encoder();
    return true;
}


X264EncoderImpl::~X264EncoderImpl()
{
    close_encoder();
}

int X264EncoderImpl::encode(encodeFrameInfo &frameinfo)
{
#ifdef OPEN_ROI
    videoRoiInfo &roiInfo = frameinfo.roiInfo;
    if (roiInfo.nFaces > 0) {
        LOGE("X264Encoder is no support ROI now!");
        exit(-1);
    }
#endif

    videoCodecInfo info;
    info.size = 0; // -1 for Error
    info.blockNum = 0;  //unused
    info.frameType = 0;
    info.QP = 0;  //unused
    info.consume = 0;


    if (!_initialized) {
        LOGE("X264EncoderImpl: encoder is not initialized");
        return -1;
    }

    // prepare input frame
    x264_picture_t pic_in;
    x264_picture_init(&pic_in);
    pic_in.i_type                = frameinfo.keyframe ? X264_TYPE_IDR : X264_TYPE_P;
    pic_in.i_pts                 = _index++;//inBufParam->timestamp;
    if (_config.fps == 7.5) {
        _index++;
    }
    // pic_in.b_keyframe            = keyFrame; //force I frame
    pic_in.img.i_csp             = X264_CSP_I420;
    pic_in.img.i_plane           = 3;
    pic_in.img.i_stride[Y_PLANE] = _config.width;
    pic_in.img.i_stride[U_PLANE] = _config.width >> 1;
    pic_in.img.i_stride[V_PLANE] = _config.width >> 1;
    pic_in.img.plane[Y_PLANE]    = (unsigned char*)(frameinfo.in);
    pic_in.img.plane[U_PLANE]    = pic_in.img.plane[Y_PLANE] + _config.width * _config.height;
    pic_in.img.plane[V_PLANE]    = pic_in.img.plane[U_PLANE] + _config.width * _config.height / 4;

    int i_nal;
    int encBytes;
    x264_nal_t *nal;
    x264_picture_t pic_out;
    long long tick = current_us();
    encBytes = x264_encoder_encode( _x264, &nal, &i_nal, &pic_in, &pic_out );
    long long tock = current_us();
    // printf("x264 encode return %d, i_nal = %d\n", encBytes, i_nal);

    // flush delayed frames
    //i_frame_size = x264_encoder_encode( _x264, &nal, int &i_nal, NULL, &pic_out );

    if (encBytes < 0)  {
        // Fix build error on Windows.
        LOGE("X264 s-encoder encode frame failed,  result=%d", encBytes);
        return -1;
    }

    if (encBytes == 0) {
        LOGE("X264 s-encoder encode frame failed, frame dropped");
        return -1;
    }

    info.frameType = pic_out.b_keyframe;//nal->i_type;
    info.priority = 0;
    info.consume = tock - tick;
    info.size = encBytes;

    memcpy(frameinfo.out, nal->p_payload, encBytes);
    _observer->onFrameEncoded(info, frameinfo);
    return 0;

}

void X264EncoderImpl::reconfig(videoCodecConfig &config)
{
}


//=================================================================================//
X264Encoder::X264Encoder(videoCodecConfig &config, IvideoEncoderObserver *observer)
{
    _impl = new X264EncoderImpl(config, observer);
}

bool X264Encoder::Destroy()
{
    return _impl->Destroy();
}


X264Encoder::~X264Encoder()
{
    delete _impl;
}

int X264Encoder::encode(encodeFrameInfo &frameinfo)
{
    return _impl->encode(frameinfo);
}

void X264Encoder::reconfig(videoCodecConfig &config)
{
    _impl->reconfig(config);
}

}

