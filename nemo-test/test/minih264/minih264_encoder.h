#ifndef _MINIH264_ENCODER_H_
#define _MINIH264_ENCODER_H_

#include <video_codec_interface.h>

namespace ENC_TEST {

class Minih264EncoderImpl;
class Minih264Encoder : public IvideoEncoder
{
public:
    Minih264Encoder(videoCodecConfig &config, IvideoEncoderObserver *observer);
    virtual ~Minih264Encoder();

    virtual bool Destroy();
    virtual int encode(encodeFrameInfo &frameinfo);
    virtual void reconfig(videoCodecConfig &config);

private:
    Minih264EncoderImpl *_impl;
};

}
#endif

