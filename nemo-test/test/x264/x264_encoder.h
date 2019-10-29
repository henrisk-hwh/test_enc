#ifndef _X264_ENCODER_H_
#define _X264_ENCODER_H_

#include <video_codec_interface.h>

namespace ENC_TEST {

class X264EncoderImpl;
class X264Encoder : public IvideoEncoder
{
public:
    X264Encoder(videoCodecConfig &config, IvideoEncoderObserver *observer);
    virtual ~X264Encoder();

    virtual bool Destroy();
    virtual int encode(encodeFrameInfo &frameinfo);
    virtual void reconfig(videoCodecConfig &config);

private:
    X264EncoderImpl *_impl;
};

}
#endif

