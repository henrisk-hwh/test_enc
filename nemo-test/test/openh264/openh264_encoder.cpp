#include <openh264_encoder.h>

#include <string.h>
#include <wels/codec_def.h>
#include <wels/codec_api.h>


static void openh264TraceCallback(void *ctx, int level, const char *string)
{
    LOGI("[Openh264EncoderImpl] %s", string);
}
static int g_LevelSetting = WELS_LOG_INFO;

namespace ENC_TEST {

class Openh264EncoderImpl {
private:
    friend Openh264Encoder;

    Openh264EncoderImpl(videoCodecConfig &config, IvideoEncoderObserver *observer);
    ~Openh264EncoderImpl();

    bool Destroy();
    int encode(encodeFrameInfo &frameinfo);
    void reconfig(videoCodecConfig &config);

private:
    bool open_encoder();
    void close_encoder();
private:
    bool _initialized;

    ISVCEncoder *_pSVCEncoder;
    SFrameBSInfo *_sFbi;
    WelsTraceCallback _pFunc;

    videoCodecConfig &_config;

    IvideoEncoderObserver *_observer = NULL;
};

void Openh264EncoderImpl::close_encoder()
{
    if (_pSVCEncoder != NULL) {
        WelsDestroySVCEncoder(_pSVCEncoder);
        _pSVCEncoder = NULL;
        _initialized = false;
    }
    if (_sFbi != NULL) {
        delete _sFbi;
        _sFbi = NULL;
    }

}

bool Openh264EncoderImpl::open_encoder()
{
    OpenH264Version ver = WelsGetCodecVersion();
    LOGI("OpenH264Version: v%d.%d.%d.%d", ver.uMajor, ver.uMinor, ver.uRevision, ver.uReserved);

    SEncParamExt sSvcParam;

    int iRet = 0;

    iRet = WelsCreateSVCEncoder(&_pSVCEncoder);

    if (iRet) {
        LOGE("Openh264EncoderImpl: WelsCreateSVCEncoder() failed!!");
        return false;
    }

    _pSVCEncoder->GetDefaultParams(&sSvcParam);

    sSvcParam.iUsageType = CAMERA_VIDEO_REAL_TIME;
    sSvcParam.fMaxFrameRate  = _config.fps; // input frame rate
    sSvcParam.iPicWidth = _config.width;
    sSvcParam.iPicHeight = _config.height;
    sSvcParam.iTargetBitrate = _config.bitrate;
    sSvcParam.iTemporalLayerNum = _config.svc ? 2 : 1;

    LOGI("Openh264EncoderImpl: width %d,height %d,bitrate %d,tempoLayers %d, idc: %d",
            sSvcParam.iPicWidth,
            sSvcParam.iPicHeight,
            sSvcParam.iTargetBitrate,
            sSvcParam.iTemporalLayerNum,
            sSvcParam.iLoopFilterDisableIdc);

    sSvcParam.iMaxBitrate    = 2 * sSvcParam.iTargetBitrate;
    sSvcParam.iRCMode        = RC_BITRATE_MODE;      //  rc mode control
    sSvcParam.bEnableFrameSkip           = 0; // frame skipping // yangfan

    sSvcParam.iMaxQp = 40;
    sSvcParam.iMinQp = 10;
    sSvcParam.bEnableDenoise    = 0;    // denoise control
    sSvcParam.bEnableBackgroundDetection = 1; // background detection control
    sSvcParam.bEnableAdaptiveQuant       = 1; // adaptive quantization control

    sSvcParam.bEnableLongTermReference   = 0; // long term reference control
    sSvcParam.iLtrMarkPeriod = -1;
    sSvcParam.uiIntraPeriod  = -1;           // period of Intra frame
    sSvcParam.eSpsPpsIdStrategy = CONSTANT_ID;
    sSvcParam.bPrefixNalAddingCtrl = 0;
    sSvcParam.iComplexityMode = LOW_COMPLEXITY;
    sSvcParam.bSimulcastAVC         = false;

    sSvcParam.bEnableFrameCroppingFlag = true;
    sSvcParam.iEntropyCodingModeFlag = 0;
    sSvcParam.iMultipleThreadIdc = 1;
    sSvcParam.bUseLoadBalancing = true;
    sSvcParam.bEnableSceneChangeDetect = false;

    sSvcParam.iSpatialLayerNum  = 1;    // layer number at spatial level
    int iIndexLayer = 0;
    //  if(_param.isHighProfile)
    //      sSvcParam.sSpatialLayers[iIndexLayer].uiProfileIdc       = PRO_HIGH;
    //  else
    sSvcParam.sSpatialLayers[iIndexLayer].uiProfileIdc       = PRO_BASELINE;

    sSvcParam.sSpatialLayers[iIndexLayer].uiLevelIdc         = LEVEL_5_0;
    sSvcParam.sSpatialLayers[iIndexLayer].iVideoWidth        = sSvcParam.iPicWidth;
    sSvcParam.sSpatialLayers[iIndexLayer].iVideoHeight       = sSvcParam.iPicHeight;
    sSvcParam.sSpatialLayers[iIndexLayer].fFrameRate         = sSvcParam.fMaxFrameRate;
    sSvcParam.sSpatialLayers[iIndexLayer].iSpatialBitrate    = sSvcParam.iTargetBitrate;
    sSvcParam.sSpatialLayers[iIndexLayer].iMaxSpatialBitrate = sSvcParam.iMaxBitrate;
    sSvcParam.sSpatialLayers[iIndexLayer].sSliceArgument.uiSliceMode = SM_SINGLE_SLICE;
    sSvcParam.sSpatialLayers[iIndexLayer].sSliceArgument.uiSliceNum = 1;

    _pFunc = openh264TraceCallback;
    _pSVCEncoder->SetOption(ENCODER_OPTION_TRACE_CALLBACK, &_pFunc);
    _pSVCEncoder->SetOption(ENCODER_OPTION_TRACE_LEVEL, &g_LevelSetting);

    if (cmResultSuccess != _pSVCEncoder->InitializeExt(&sSvcParam))   // SVC encoder initialization
    {
        LOGE("SoftwareOpen264Encoder: SVC encoder Initialize failed");
        return false;
    }

    //_sFbi = (SFrameBSInfo *)malloc(sizeof(SFrameBSInfo));

    if (_pSVCEncoder != NULL)
    {
        _initialized = true;
    }

    return _initialized;
}

Openh264EncoderImpl::Openh264EncoderImpl(videoCodecConfig &config, IvideoEncoderObserver *observer) :
    _observer(observer),
    _config(config),
    _initialized(false),
    _pSVCEncoder(NULL),
    _sFbi(NULL)
{
    open_encoder();
}

bool Openh264EncoderImpl::Destroy()
{
    close_encoder();
    return true;
}


Openh264EncoderImpl::~Openh264EncoderImpl()
{
    close_encoder();
}

int Openh264EncoderImpl::encode(encodeFrameInfo &frameinfo)
{
    videoCodecInfo info;
    info.size = 0; // -1 for Error
    info.blockNum = 0;  //unused
    info.frameType = 0;
    info.QP = 0;  //unused
    info.consume = 0;

    SSourcePicture *pSrcPic = NULL;
    SFrameBSInfo sFbi;
    memset (&sFbi, 0, sizeof (SFrameBSInfo));

    if (!_initialized) {
        LOGE("Openh264EncoderImpl: encoder is not initialized");
        return -1;
    }

    if(frameinfo.keyframe) {
        _pSVCEncoder->ForceIntraFrame(true);
    }

#ifdef OPEN_ROI
    videoRoiInfo &roiInfo = frameinfo.roiInfo;
    if (roiInfo.nFaces > 0) {
        sFbi.frameQpdelta = roiInfo.nFrameqp;
        for (int i = 0; i < roiInfo.nFaces; i++) {
            sFbi.roi_param[i].bEnable = true;
            sFbi.roi_param[i].nQPoffset = roiInfo.nMbqp;
            sFbi.roi_param[i].left = roiInfo.sRect[i].nLeft;
            sFbi.roi_param[i].right = roiInfo.sRect[i].nLeft + roiInfo.sRect[i].nWidth;
            sFbi.roi_param[i].top = roiInfo.sRect[i].nTop;
            sFbi.roi_param[i].bottom = roiInfo.sRect[i].nTop + roiInfo.sRect[i].nHeight;
            LOGI("roi area: %d %d %d %d, mbqp: %d, frameqp: %d",
                sFbi.roi_param[i].left, sFbi.roi_param[i].right,
                sFbi.roi_param[i].top, sFbi.roi_param[i].bottom,
                sFbi.roi_param[i].nQPoffset, sFbi.frameQpdelta);
        }
    }
#endif

    SSourcePicture srcPic;
    memset(&srcPic, 0, sizeof(SSourcePicture));
    pSrcPic = &srcPic;

    if (pSrcPic == NULL) {
        LOGE("Openh264EncoderImpl: new SSourcePicture fail");
        return -1;
    }

    pSrcPic->iPicWidth = _config.width;
    pSrcPic->iPicHeight = _config.height;
    pSrcPic->iColorFormat = videoFormatI420;
    pSrcPic->iStride[0] = _config.width;
    pSrcPic->iStride[1] = pSrcPic->iStride[2] = pSrcPic->iStride[0] >> 1;
    pSrcPic->pData[0] = (unsigned char*)(frameinfo.in);
    pSrcPic->pData[1] = pSrcPic->pData[0] + pSrcPic->iPicWidth * pSrcPic->iPicHeight;
    pSrcPic->pData[2] = pSrcPic->pData[1] + (pSrcPic->iPicWidth * pSrcPic->iPicHeight >> 2);
    pSrcPic->uiTimeStamp = frameinfo.ts;

    long long tick = current_us();
    int iEncFrames = _pSVCEncoder->EncodeFrame (pSrcPic, &sFbi);
    long long tock = current_us();

    if (videoFrameTypeSkip == sFbi.eFrameType) {
        LOGI("SoftwareOpen264Encoder: input fame is dropped");
        return -1;
    }

    if (iEncFrames == cmResultSuccess) {
        int iLayer = 0;
        int iFrameSize = 0;

        while (iLayer < sFbi.iLayerNum) {
            SLayerBSInfo *pLayerBsInfo = &sFbi.sLayerInfo[iLayer];

            if (pLayerBsInfo != NULL) {
                int iLayerSize = 0;
                int iNalIdx = pLayerBsInfo->iNalCount - 1;

                do {
                    iLayerSize += pLayerBsInfo->pNalLengthInByte[iNalIdx];
                    -- iNalIdx;
                }
                while (iNalIdx >= 0);

                memcpy((unsigned char *)(frameinfo.out) + iFrameSize, pLayerBsInfo->pBsBuf, iLayerSize);
                iFrameSize += iLayerSize;
            }

            ++ iLayer;
        }

        info.frameType = sFbi.eFrameType == 1 ? 1 : 0;
        info.size = iFrameSize;
        info.consume = tock - tick;
        info.priority = sFbi.sLayerInfo[0].uiTemporalId;
        
        _observer->onFrameEncoded(info, frameinfo);
        //VD_WARN("SoftwareOpen264Encoder: FrameType:%d,priority:%d,timestamp:%lld,%lld",sFbi.eFrameType,outBufParam->priority,outBufParam->timestamp,inBufParam->timestamp);
    }
    else
    {
        LOGE("SoftwareOpen264Encoder: encode frame failed, ret: %d", iEncFrames);
    }

    return 0;

}

void Openh264EncoderImpl::reconfig(videoCodecConfig &config)
{

}


//=================================================================================//
Openh264Encoder::Openh264Encoder(videoCodecConfig &config, IvideoEncoderObserver *observer)
{
    _impl = new Openh264EncoderImpl(config, observer);
}

bool Openh264Encoder::Destroy()
{
    return _impl->Destroy();
}


Openh264Encoder::~Openh264Encoder()
{
    delete _impl;
}

int Openh264Encoder::encode(encodeFrameInfo &frameinfo)
{
    return _impl->encode(frameinfo);
}

void Openh264Encoder::reconfig(videoCodecConfig &config)
{
    _impl->reconfig(config);
}

}

