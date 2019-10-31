#include <minih264_encoder.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MINIH264_IMPLEMENTATION
//#define MINIH264_ONLY_SIMD
#include <minih264e.h>

//#gcc -flto -O3 -m32 -msse2 -std=gnu11 -DH264E_MAX_THREADS=4 -DH264E_SVC_API=1 -DNDEBUG -Wall -Wextra -ffast-math -fno-stack-protector -fomit-frame-pointer -ffunction-sections -fdata-sections -Wl,--gc-sections -mpreferred-stack-boundary=4 -o h264enc_x86_sse2 minih264e_test.c system.c -lm -lpthread
//#gcc -flto -O3 -m32 -std=gnu11 -DH264E_MAX_THREADS=4 -DH264E_SVC_API=1 -DNDEBUG -Wall -Wextra -ffast-math -fno-stack-protector -fomit-frame-pointer -ffunction-sections -fdata-sections -Wl,--gc-sections -mpreferred-stack-boundary=4 -o h264enc_x86 minih264e_test.c system.c -lm -lpthread
//#gcc -flto -O3 -std=gnu11 -DH264E_MAX_THREADS=4 -DH264E_SVC_API=1 -DNDEBUG -Wall -Wextra -ffast-math -fno-stack-protector -fomit-frame-pointer -ffunction-sections -fdata-sections -Wl,--gc-sections -mpreferred-stack-boundary=4 -o h264enc_x64 minih264e_test.c system.c -lm -lpthread
#define DEFAULT_GOP 20
#define DEFAULT_QP 33
#define DEFAULT_DENOISE 0

#define ENABLE_TEMPORAL_SCALABILITY 1
#define MAX_LONG_TERM_FRAMES        8 // used only if ENABLE_TEMPORAL_SCALABILITY==1

#ifdef _WIN32
// only vs2017 have aligned_alloc
#define ALIGNED_ALLOC(n, size) malloc(size)
#else
#define ALIGNED_ALLOC(n, size) aligned_alloc(n, size)
#endif

static unsigned char get_svc_layer_id(unsigned char* pNalBuffer) {
    uint8_t tid = 0;
    if (((pNalBuffer[0] & 0x1f) == 7) ||
        ((pNalBuffer[0] & 0x1f) == 8) ||
        ((pNalBuffer[0] & 0x1f) == 6)) {
        tid = 0;
    }
    else {
        tid = (pNalBuffer[0] & 0xe0) >> 5;
        LOGE("tid: %d", tid);
        tid = tid > 0 ? 0 : 1;
    }

    return tid;
}

namespace ENC_TEST {

class Minih264EncoderImpl {
private:
    friend Minih264Encoder;

    Minih264EncoderImpl(videoCodecConfig &config, IvideoEncoderObserver *observer);
    ~Minih264EncoderImpl();

    bool Destroy();
    int encode(encodeFrameInfo &frameinfo);
    void reconfig(videoCodecConfig &config);

private:
    bool open_encoder();
    void close_encoder();
private:
    bool _initialized = false;
    H264E_persist_t *_enc = NULL;
    H264E_scratch_t *_scratch = NULL;
    H264E_create_param_t _create_param;
    int _frameIdx = 0;

    IvideoEncoderObserver *_observer = NULL;
    videoCodecConfig &_config;
};

void Minih264EncoderImpl::close_encoder()
{
    if (_enc)
        free(_enc);
    if (_scratch)
        free(_scratch);
}

bool Minih264EncoderImpl::open_encoder()
{
    _create_param.enableNEON = 1;
#if H264E_SVC_API
    _create_param.num_layers = 2;
    _create_param.inter_layer_pred_flag = 1;
    _create_param.inter_layer_pred_flag = 0;
#endif
    _create_param.gop = _config.gop == -1 ? 0 : _config.gop;
    _create_param.height = _config.height;
    _create_param.width  = _config.width;
    _create_param.max_long_term_reference_frames = 0;

#if ENABLE_TEMPORAL_SCALABILITY
    if (_config.svc) {
        _create_param.max_long_term_reference_frames = MAX_LONG_TERM_FRAMES;
    }
#endif
    _create_param.fine_rate_control_flag = 0;
    _create_param.const_input_flag = 0;
    //_create_param.vbv_overflow_empty_frame_flag = 1;
    //_create_param.vbv_underflow_stuffing_flag = 1;
    _create_param.vbv_size_bytes = 100000/8;
    _create_param.temporal_denoise_flag = DEFAULT_DENOISE;
    //_create_param.vbv_size_bytes = 1500000/8;

    int sizeof_persist = 0, sizeof_scratch = 0, error;

    error = H264E_sizeof(&_create_param, &sizeof_persist, &sizeof_scratch);
    if (error)
    {
        LOGE("H264E_sizeof error = %d\n", error);
        return _initialized;
    }

    LOGE("sizeof_persist = %d sizeof_scratch = %d", sizeof_persist, sizeof_scratch);
    _enc     = (H264E_persist_t *)ALIGNED_ALLOC(64, sizeof_persist);
    _scratch = (H264E_scratch_t *)ALIGNED_ALLOC(64, sizeof_scratch);
    error = H264E_init(_enc, &_create_param);
    if (error)
    {
        LOGE("H264E_init error = %d\n", error);
        return _initialized;
    }

    LOGI("Minih264EncoderImpl: width %d,height %d,bitrate %d,tempoLayers %d, speed: %d",
        _create_param.width,
        _create_param.height,
        _config.bitrate,
        _create_param.num_layers,
        _config.speed);

    _initialized = true;
    return _initialized;
}

Minih264EncoderImpl::Minih264EncoderImpl(videoCodecConfig &config, IvideoEncoderObserver *observer) :
    _observer(observer),
    _config(config)
{
    open_encoder();
}

bool Minih264EncoderImpl::Destroy()
{
    close_encoder();
    return true;
}


Minih264EncoderImpl::~Minih264EncoderImpl()
{
    close_encoder();
}

int Minih264EncoderImpl::encode(encodeFrameInfo &frameinfo)
{
    int error;
    H264E_run_param_t run_param;
    H264E_io_yuv_t yuv;

    uint8_t *coded_data;
    int sizeof_coded_data;
    run_param.nalu_callback = NULL;
    run_param.frame_type = H264E_FRAME_TYPE_DEFAULT;
    run_param.encode_speed = _config.speed; //speed [0..10], 0 means best quality

    //run_param.desired_nalu_bytes = 100;


    run_param.desired_frame_bytes = _config.bitrate/8/_config.fps;
    run_param.qp_min = 10;
    run_param.qp_max = 50;
#if ENABLE_TEMPORAL_SCALABILITY
    if (_config.svc) {
        int i = _frameIdx;
        int level, logmod = 1;
        int j, mod = 1 << logmod;
        static int fresh[200] = {-1,-1,-1,-1};

        run_param.frame_type = H264E_FRAME_TYPE_CUSTOM;

        for (level = logmod; level && (~i & (mod >> level)); level--){}

        run_param.long_term_idx_update = level + 1;
        if (level == logmod && logmod > 0)
            run_param.long_term_idx_update = -1;
        if (level == logmod - 1 && logmod > 1)
            run_param.long_term_idx_update = 0;

        //if (run_param.long_term_idx_update > logmod) run_param.long_term_idx_update -= logmod+1;
        //run_param.long_term_idx_update = logmod - 0 - level;
        //if (run_param.long_term_idx_update > 0)
        //{
        //    run_param.long_term_idx_update = logmod - run_param.long_term_idx_update;
        //}
        run_param.long_term_idx_use    = fresh[level];
        LOGE("level: %d, long_term_idx_use: %d", level, fresh[level]);
        for (j = level; j <= logmod; j++)
        {
            fresh[j] = run_param.long_term_idx_update;
        }
        if (!i)
        {
            run_param.long_term_idx_use = -1;
        }
    }
#endif

    //run_param.qp_min = run_param.qp_max = cmdline->qp;
    uint8_t *buf_in = (uint8_t*)(frameinfo.in);
    yuv.yuv[0] = buf_in; yuv.stride[0] = _config.width;
    yuv.yuv[1] = buf_in + _config.width * _config.height;yuv.stride[1] = _config.width / 2;
    yuv.yuv[2] = buf_in + _config.width * _config.height * 5 / 4; yuv.stride[2] = _config.width / 2;

    long long tick = current_us();
    LOGE("desired_frame_bytes: %d", run_param.desired_frame_bytes);
    error = H264E_encode(_enc, _scratch, &run_param, &yuv, &coded_data, &sizeof_coded_data);
    assert(!error);

    _frameIdx ++;

    long long tock = current_us();

    memcpy((unsigned char *)(frameinfo.out), coded_data, sizeof_coded_data);

    videoCodecInfo info;

    info.blockNum = 0;  //unused
    info.QP = 0;  //unused
    info.frameType = 0; //TODO
    info.size = sizeof_coded_data;
    info.consume = tock - tick;

    int naltype = coded_data[4] & 0x1f;
    if (naltype == 5 || naltype == 6 || naltype == 7 || naltype == 8)
        info.frameType = 1;

    info.priority = get_svc_layer_id(coded_data + 4);
    _observer->onFrameEncoded(info, frameinfo);
    return 0;

}

void Minih264EncoderImpl::reconfig(videoCodecConfig &)
{

}


//=================================================================================//
Minih264Encoder::Minih264Encoder(videoCodecConfig &config, IvideoEncoderObserver *observer)
{
    _impl = new Minih264EncoderImpl(config, observer);
}

bool Minih264Encoder::Destroy()
{
    return _impl->Destroy();
}


Minih264Encoder::~Minih264Encoder()
{
    delete _impl;
}

int Minih264Encoder::encode(encodeFrameInfo &frameinfo)
{
    return _impl->encode(frameinfo);
}

void Minih264Encoder::reconfig(videoCodecConfig &config)
{
    _impl->reconfig(config);
}

}
