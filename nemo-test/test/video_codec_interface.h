#ifndef _VIDEO_CODEC_INTERFACE_H_
#define _VIDEO_CODEC_INTERFACE_H_
//#include "media_processor/include/mp_payloadtypes.h"

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>

#include <string>


#ifdef WIN32
#define DLL_API __declspec(dllexport)
#else
#define DLL_API
#endif

#define MAX_NUM_OF_FACE_NUMBERS 4

#if 0
#define LOGE(...) //
#define LOGI(...) //
#define LOGD(...) //
#else
#define LOGI(fmt,arg...) printf("[%d-%ld] - " fmt "\n", getpid(), syscall(224), ##arg)
#define LOGE(fmt,arg...) printf("[%d-%ld] - " fmt "\n", getpid(), syscall(224), ##arg)
#define LOGD(fmt,arg...) printf("[%d-%ld] - " fmt "\n", getpid(), syscall(224), ##arg)
#endif

enum EncoderType
{
    TYPE_ENC_NONE,
    TYPE_ENC_MINIH264,
    TYPE_ENC_OPENH264,
    TYPE_ENC_X264,
    TYPE_ENC_HW_OMX,
    TYPE_ENC_R58_OMX,
    TYPE_ENC_MT8765_OMX,
    TYPE_ENC_MT8167_OMX,
    TYPE_ENC_RK3326_OMX,
};

typedef struct videoRoiRect {
    int nLeft;
    int nTop;
    int nWidth;
    int nHeight;
} videoRoiRect;

typedef struct videoRoiInfo {
    int nFaces = 0;
    int nMbqp;
    int nFrameqp;
    videoRoiRect sRect[MAX_NUM_OF_FACE_NUMBERS];
} videoRoiInfo;

typedef struct videoCodecConfig {
    unsigned short width = 0;
    unsigned short height = 0;
    float          fps = 0.0;
    unsigned int   bitrate = 0;
    signed int   gop = 0;
    EncoderType type = TYPE_ENC_NONE;
    bool svc = false;
    std::string encInfo = "";
    std::string inFileName = "";
    std::string outFileName = "";
    std::string infoFileName = "";

    FILE *fin = NULL;
    FILE *fout = NULL;
    FILE *finfo = NULL;
    FILE *froi = NULL;

    //ROI config
    std::string roiFileName = "";
    int roiMbQpDelta = 2;
    int roiFrameQpDelta = 0;

    int speed = 10;
} videoCodecConfig;

typedef struct videoCodecInfo {
    unsigned short QP;
    unsigned short blockNum;
    unsigned short frameType;
    unsigned int   size;
    unsigned long long consume;
    unsigned int priority = 0;
    // ...
} videoCodecInfo;

typedef struct encodeFrameInfo {
    char *in;
    char *out;
    bool keyframe;
    long long ts;
    long long tick;
    videoRoiInfo roiInfo;
} encodeFrameInfo;

class DLL_API IvideoEncoderObserver {
public:
    virtual void onFrameEncoded(videoCodecInfo &encodedInfo, encodeFrameInfo &frameinfo) = 0;
};

class DLL_API IvideoEncoder
{
public:
    virtual ~IvideoEncoder() {};
    virtual bool Destroy() = 0;
    virtual void reconfig(videoCodecConfig &config) = 0;
    virtual int encode(encodeFrameInfo &frameinfo) = 0;
};

extern "C" DLL_API IvideoEncoder *CreateVideoEncoder(videoCodecConfig &config, IvideoEncoderObserver *observer);
extern "C" DLL_API long long current_us(void);

class Lock {
    private:
        pthread_mutex_t mMutex;
    public:
        Lock() {
            pthread_mutex_init(&mMutex, NULL);
        }
        ~Lock() {
            pthread_mutex_destroy(&mMutex);
        }

        void lock() {
            pthread_mutex_lock(&mMutex);
        }

        void unlock() {
            pthread_mutex_unlock(&mMutex);
        }
};

class Condition {
    private:
        pthread_mutex_t mMutex;
        pthread_cond_t mCond;
    public:
        Condition() {
            pthread_mutex_init(&mMutex, NULL);
            pthread_cond_init(&mCond, NULL);
        }
        ~Condition() {
            pthread_mutex_destroy(&mMutex);
            pthread_cond_destroy(&mCond);
        }

        void notify() {
            pthread_mutex_lock(&mMutex);
            pthread_cond_signal(&mCond);
            pthread_mutex_unlock(&mMutex);
        }

        int wait(int timeout = 0) {
            int ret = 0;
            pthread_mutex_lock(&mMutex);
            if (timeout == 0) {
                ret = pthread_cond_wait(&mCond, &mMutex);
            } else {
                struct timespec abs_timeout;
                struct timeval now;
                gettimeofday(&now, NULL);
                abs_timeout.tv_sec = now.tv_sec + timeout;
                abs_timeout.tv_nsec = now.tv_usec * 1000;
                ret = pthread_cond_timedwait(&mCond, &mMutex, &abs_timeout);
            }
            pthread_mutex_unlock(&mMutex);
            return ret;
        }
};

class Semaphore {
    private:
        sem_t mSem;
    public:
        Semaphore() {
            sem_init(&mSem, 0, 0);
        }
        ~Semaphore() {
            sem_destroy(&mSem);
        }

        void notify() {
            //long long tick = current_us();
            int count = 0;
            sem_getvalue(&mSem, &count);
            //LOGI(" >>>>>>>> before nofity : %d", count);
            sem_post(&mSem);
            sem_getvalue(&mSem, &count);
            //LOGI(" >>>>>>>> after nofity : %d, cost: %lld", count, current_us() - tick);
        }

        void wait(int timeout) {
            //long long tick = current_us();
            int count = 0;
            sem_getvalue(&mSem, &count);
            //LOGI(" >>>>>>>> before wait : %d", count);
            struct timespec abs_timeout;
            abs_timeout.tv_sec = timeout;
            sem_timedwait(&mSem, &abs_timeout);
            sem_getvalue(&mSem, &count);
            //LOGI(" >>>>>>>> after wait : %d, cost: %lld", count, current_us() - tick);
        }

        void wait() {
            //long long tick = current_us();
            int count = 0;
            sem_getvalue(&mSem, &count);
            //LOGI(" <<<<<<<<< before wait : %d", count);
            sem_wait(&mSem);
            sem_getvalue(&mSem, &count);
            //LOGI(" <<<<<<<<< after wait : %d, cost: %lld", count, current_us() - tick);
        }
};

#endif
