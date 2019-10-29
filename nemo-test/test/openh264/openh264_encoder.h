#ifndef _OPENH264_ENCODER_H_
#define _OPENH264_ENCODER_H_

#include <video_codec_interface.h>

namespace ENC_TEST {

class Openh264EncoderImpl;
class Openh264Encoder : public IvideoEncoder
{
public:
    Openh264Encoder(videoCodecConfig &config, IvideoEncoderObserver *observer);
    virtual ~Openh264Encoder();

    virtual bool Destroy();
    virtual int encode(encodeFrameInfo &frameinfo);
    virtual void reconfig(videoCodecConfig &config);

private:
    Openh264EncoderImpl *_impl;
};

}
#endif

