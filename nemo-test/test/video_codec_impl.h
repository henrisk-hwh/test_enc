#ifndef _INTERFACE_H_
#define _INTERFACE_H_

#include <video_codec_interface.h>
#include <list>

long long current_us(void);

class videoEncoderImpl : public IvideoEncoder
{
public:
    videoEncoderImpl(videoCodecConfig config);
    virtual bool Destroy();
    virtual ~videoEncoderImpl();
    virtual videoCodecInfo encode(char *in, char *out);
    virtual void reconfig(videoCodecConfig &config);

private:
    //MP::SyncVideoEncoder *syncVideoEncoder_;
    int duration_;
    int frameEncodeSeq_;
    unsigned int encodeTs_;
    videoCodecConfig config_;
};

#endif
