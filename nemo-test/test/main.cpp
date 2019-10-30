/* 
Creat by ivan tsui
编码器命令行参数
encoder -s <WxH> -fps <fps> -bitrate <bitrate> -i <input file> -o <output_file>
demo -c svc -r 512x288 -f 30 -b 128000 -g 30 -i classroom_288_30.yuv
编码器输出格式
每帧编码完成后，输出如下信息：
1 帧序号
2 帧类型
3 帧大小
4 编码耗时
5 其他质量信息（QP，宏块个数，各种类型宏块的个数）（Optional） 
*/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <list>
#include <video_codec_interface.h>

#include <signal.h>	    /* for signal */
#include <execinfo.h> 	/* for backtrace() */
 
#define BACKTRACE_SIZE   16
 
void dump(void)
{
	int j, nptrs;
	void *buffer[BACKTRACE_SIZE];
	char **strings;
	
	nptrs = backtrace(buffer, BACKTRACE_SIZE);
	
	printf("backtrace() returned %d addresses\n", nptrs);
 
	strings = backtrace_symbols(buffer, nptrs);
	if (strings == NULL) {
		perror("backtrace_symbols");
		exit(EXIT_FAILURE);
	}
 
	for (j = 0; j < nptrs; j++)
		printf("  [%02d] %s\n", j, strings[j]);
 
	free(strings);
}
 
void signal_handler(int signo)
{
	
#if 0	
	char buff[64] = {0x00};
		
	sprintf(buff,"cat /proc/%d/maps", getpid());
		
	system((const char*) buff);
#endif	
 
	printf("\n=========>>>catch signal %d <<<=========\n", signo);
	
	printf("Dump stack start...\n");
	dump();
	printf("Dump stack end...\n");
 
	signal(signo, SIG_DFL); /* 恢复信号默认处理 */
	raise(signo);           /* 重新发送信号 */
}

struct CodecConfig {
    const char *key;
    EncoderType type;
    bool svc;
};

struct CodecConfig CodecConfigs[] = {
    {"openh264-avc", TYPE_ENC_OPENH264, false},
    {"openh264-svc", TYPE_ENC_OPENH264, true},
    {"r58-avc", TYPE_ENC_R58_OMX, false},
    {"r58-svc", TYPE_ENC_R58_OMX, true},
    {"mt8765-avc", TYPE_ENC_MT8765_OMX, false},
    {"mt8765-svc", TYPE_ENC_MT8765_OMX, true},
    {"mt8167-avc", TYPE_ENC_MT8167_OMX, false},
    {"mt8167-svc", TYPE_ENC_MT8167_OMX, true},
    {"rk3326-avc", TYPE_ENC_RK3326_OMX, false},
    {"rk3326-svc", TYPE_ENC_RK3326_OMX, true},
    {"x264-avc", TYPE_ENC_X264, false},
    {"minih264-svc", TYPE_ENC_MINIH264, true},
    {"minih264-avc", TYPE_ENC_MINIH264, false},
    
};

void printHelp() {
    printf("Video Encoder Unittest.\n");
    printf("Instructions(Required):\n");
    printf("'-codec': Video codec type(soft), the value can be as:\n");
    for (auto config : CodecConfigs) {
        printf("    %s\n", config.key);
    }
    printf("'-s': Input raw video resolution, the value can be as: 1280x720\n");
    printf("'-f','fps': Input raw video frame rate, the value can be as:\n");
    printf("    1, 7.5\n");
    printf("    2, 10\n");
    printf("    3, 15\n");
    printf("    4, 30\n");
    printf("'-b','-bitrate': Input raw video bit rate.\n");
    printf("'-g','-gop': Key Frame Duration.\n");
    printf("'-R','-roi': Input video ROI info (include path).\n");
    printf("'-i','-input': Input raw video name (include path).\n");
    printf("'-o','-output': Ouput encoded video name (include path)(Optional).\n");
    printf("'-h','-help': Display help.\n");
    printf("Example:\n");
    printf("demo.exe -c svc -r 288P -f 30 -b 128000 -g 90 -i classroom_288_30.yuv\n");
    printf("\n\n\n");
}

void cmd(int argc, char *argv[], videoCodecConfig &config) {
    if (argc == 1) {
        printf("Error.No command line arguments.\n");
        printHelp();
        exit(0);
    }
    
    for (int cnt = 1; cnt < argc; cnt++) {
        if(strcmp(argv[cnt], "-s")==0) {
            int cnt_ = cnt + 1;
            if(cnt_ >= argc) {
                printf("Error. No bitrate arguments.\n");
                printHelp();
                exit(0);
            }

            char value[64] = {0};
            char* start = argv[cnt_];
            char* end = strchr(start,'x');
            memcpy(value, start, end-start);
            config.width = atoi(value);
            start = end + 1;
            config.height = atoi(start);
        } else if(strcmp(argv[cnt], "-fps") == 0) {
            int cnt_ = cnt + 1;
            if(cnt_ < argc) {
                sscanf(argv[cnt_], "%f", &config.fps); 
            } else {
                printf("Error. No fps arguments.\n");
                printHelp();
                exit(0);
            }
        } else if(strcmp(argv[cnt], "-b") == 0
               || strcmp(argv[cnt], "-bitrate") == 0) {
            int cnt_ = cnt + 1;
            if(cnt_ < argc) {
                sscanf(argv[cnt_], "%d", &config.bitrate); 
            } else {
                printf("Error. No bitrate arguments.\n");
                printHelp();
                exit(0);
            }
        } else if(strcmp(argv[cnt], "-g")==0
               || strcmp(argv[cnt], "-gop")==0) {
            int cnt_ = cnt + 1;
            if(cnt_ < argc) {
                sscanf(argv[cnt_], "%d", &config.gop); 
            } else {
                printf("Error. No gop arguments.\n");
                printHelp();
                exit(0);
            }
            //if (config.gop == 0) config.gop = -1;   //fake
        } else if(strcmp(argv[cnt], "-i") == 0) {
            int cnt_ = cnt + 1;
            if(cnt_ < argc) {
                config.inFileName = argv[cnt_];
            } else {
                printf("Error. No input arguments.\n");
                printHelp();
                exit(0);
            }
        } else if(strcmp(argv[cnt], "-o") == 0) {
            int cnt_ = cnt + 1;
            if(cnt_ < argc) {
                config.outFileName = argv[cnt_];
            } else {
                printf("Error. No output arguments.\n");
                printHelp();
                exit(0);
            }
        } else if(strcmp(argv[cnt], "-codec") == 0) {
            int cnt_ = cnt + 1;
            if(cnt_ < argc) {
                bool incompatible = false;
                for (auto codecconfig : CodecConfigs) {
                    LOGI("matching    %s", codecconfig.key);
                    if(strcmp(argv[cnt_], codecconfig.key) == 0) {
                        incompatible = true;
                        config.type = codecconfig.type;
                        config.svc = codecconfig.svc;
                        config.encInfo = codecconfig.key;
                        break;
                    }
                }
                if(!incompatible) {
                    printf("Error. codec arguments not incompatible.\n");
                    printHelp();
                    exit(0);
                }
            } else {
                printf("Error. No codec arguments.\n");
                printHelp();
                exit(0);
            }
        } else if(strcmp(argv[cnt], "-h") == 0
            || strcmp(argv[cnt], "-help")==0) {
            printHelp();
        } else if(strcmp(argv[cnt], "-R") == 0
               || strcmp(argv[cnt], "-roi")==0) {
            int cnt_ = cnt + 1;
            if(cnt_<argc) {
                config.roiFileName = argv[cnt_];
            } else {
                printf("Error. No Roi arguments.\n");
                printHelp();
                exit(0);
            }
        } else if(strcmp(argv[cnt], "-mbqp")==0) {
            int cnt_=cnt+1;
            if(cnt_<argc) {
                sscanf(argv[cnt_], "%d", &config.roiMbQpDelta); 
            } else {
                printf("Error. No Roi MB QP arguments.\n");
                printHelp();
                exit(0);
            }
        } else if(strcmp(argv[cnt], "-frameqp")==0) {
            int cnt_=cnt+1;
            if(cnt_<argc) {
                sscanf(argv[cnt_], "%d", &config.roiFrameQpDelta); 
            } else {
                printf("Error. No Roi Frame QP arguments.\n");
                printHelp();
                exit(0);
            }
        } else if(strcmp(argv[cnt], "-speed")==0) {
            int cnt_=cnt+1;
            if(cnt_<argc) {
                sscanf(argv[cnt_], "%d", &config.speed);
                LOGE("11run_param.encode_speed: %d %p", config.speed, &config.speed);
            } else {
                printf("Error. No speed arguments.\n");
                printHelp();
                exit(0);
            }
        }
    }
}


static int ReadLine (FILE *fp,char pVal[][64]) {
    if (fp == NULL || pVal == NULL)
        return 0;

    int n = 0;
    int nTag = 0;
    do {
        const char kCh = (char)fgetc(fp);

        if (kCh == '\n' || feof (fp)) {
            break;
        }
        if(kCh == ';')
        {
            nTag++;
            n = 0;
        }
        else
        {
            pVal[nTag][n] = kCh;
            n++;
        }
    } while (1);

    return nTag;
}

static int parseFrameFace(FILE *fp,videoRoiRect *reg, int size)
{
    char buf[4][64] = {0};
    int nTag = 0;
    int faceNum = -1;
    int i;
    
    if((nTag = ReadLine(fp,buf)) == 0)
        return 0;
    
    faceNum = atoi(buf[0]);
    printf("faceNum:%d\r\n",faceNum);
    if(faceNum == 0)
        return 0;

    for(i = 0; i < faceNum && i < size; i++) {
        memset(buf,0,sizeof(buf));
        if((nTag = ReadLine(fp,buf)) != 4)  {
            printf("readline error:%d\r\n", nTag);
            return 0;
        }

        int left,top,right,bottom;
        left = atoi(buf[0]);
        top = atoi(buf[1]);
        right = atoi(buf[2]);
        bottom = atoi(buf[3]);

        reg[i].nLeft = left;
        reg[i].nTop = top;
        reg[i].nWidth = right - reg[i].nLeft;
        reg[i].nHeight = bottom - reg[i].nTop;

        printf("l:%d,t:%d,r:%d,b:%d\r\n", left, right, top, bottom);
    }

    return faceNum;
}

class EncoderCtx : public IvideoEncoderObserver
{
public:
    EncoderCtx(videoCodecConfig &conf) : _config(conf){
        _encoder = CreateVideoEncoder(_config, this);
    };
    ~EncoderCtx(){
        delete _encoder;
    };
    void onFrameEncoded(videoCodecInfo &encodedInfo, encodeFrameInfo &frameinfo){
        fwrite(frameinfo.out, sizeof(char), encodedInfo.size, _config.fout);
        _encodedFrame++;
        char buf[256] = {0};
        sprintf(buf,"[debug]: frame=%d Type:%s%d Size=%d Bytes Time=%0.2f ms\n",
            _encodedFrame,encodedInfo.frameType==1?"I":"P",encodedInfo.priority,encodedInfo.size,(float)encodedInfo.consume/1000.0f);
        fwrite(buf, 1, strlen(buf), _config.finfo);
        LOGI("%s", buf);
        
        free(frameinfo.out);
        free(frameinfo.in);

        _cond.notify();
    };

    void run(){
        while(true) {
            bool keyframe = false;
            if (_firstEncode) {
                _encodeTs = 0;
                _firstEncode = false;
            } else {
                _encodeTs += 1000 / _config.fps;
            }

            if (_config.gop == 0) {
                keyframe = true;
            } else if (_config.gop != -1 &&
                _encodedFrame % _config.gop == 0) {
                keyframe = true;
            } else {
                keyframe = false;
            }
            
            encodeFrameInfo frameinfo;
            frameinfo.in = new char[_config.width * _config.height * 3 / 2];
            frameinfo.out = new char[_config.width * _config.height * 3 / 2];;
            frameinfo.ts = _encodeTs;
            frameinfo.keyframe = keyframe;
            
            if (!fread(frameinfo.in, sizeof(char), _config.width * _config.height * 3 / 2, _config.fin)) {
                //eof
                break;
            }
            
            if (_config.froi != NULL) {
                videoRoiInfo &roiInfo = frameinfo.roiInfo;
                roiInfo.nFaces = parseFrameFace(_config.froi, roiInfo.sRect, MAX_NUM_OF_FACE_NUMBERS);
                roiInfo.nMbqp = _config.roiMbQpDelta;
                roiInfo.nFrameqp = _config.roiFrameQpDelta;
            }
            _oriFrame++;

            LOGI("encode one, in %p, out: %p, _oriFrame %d _encodedFrame %d", frameinfo.in, frameinfo.out, _oriFrame, _encodedFrame);
            LOGE("22run_param.encode_speed: %d %p", _config.speed, &_config.speed);
            while( _encoder->encode(frameinfo) == -1) {
                printf("encode one failed, retry\n");
                usleep(50000);
            }
            //usleep(50000);
        }
        int retry = 10;
        while(_oriFrame != _encodedFrame && retry) {
            printf("_oriFrame %d _encodedFrame %d, wait\n", _oriFrame, _encodedFrame);
            int ret = _cond.wait(1);
            if(ret == ETIMEDOUT)
                retry--;
            /*
            if (_oriFrame != _encodedFrame) {
                encodeFrameInfo frameinfo;
                frameinfo.in = new char[_config.width * _config.height * 3 / 2];
                frameinfo.out = new char[_config.width * _config.height * 3 / 2];;
                frameinfo.ts = _encodeTs;
                frameinfo.keyframe = 0;
                _encoder->encode(frameinfo);
            }
            */
        }
    };
private:
    videoCodecConfig &_config;

    IvideoEncoder *_encoder = NULL;
    int _encodedFrame = 0;
    int _oriFrame = 0;
    long long _encodeTs = 0;
    bool _firstEncode = true;

    Condition _cond;
};

int main(int argc, char *argv[]) {
    
    signal(SIGSEGV, signal_handler);  /* 为SIGSEGV信号安装新的处理函数 */

    std::string infoFileName;
    videoCodecConfig config;

    //memset(&config, 0, sizeof(videoCodecConfig));
    cmd(argc, argv, config);
    if(config.width == 0 || config.height == 0 
        || config.fps == 0 || config.bitrate == 0
    /*|| config.gop == 0 */|| config.inFileName==""
    || config.type == TYPE_ENC_NONE) {
        printf("Input param error %d %d %f %d %s %d.\n", config.width, config.height, config.fps, config.bitrate, config.inFileName.c_str(), config.type);
        printHelp();
        exit(0);
    }

    if(config.outFileName == "") {
        char info[60];
        snprintf(info, sizeof(info), "_%d_%d_%d_%u_", config.height, (int)config.fps, config.gop, config.bitrate);
        config.outFileName = config.inFileName.substr(0, config.inFileName.size() - 4);
        config.outFileName = config.outFileName + info + config.encInfo;
    }
    config.outFileName = config.outFileName + ".264";

    config.infoFileName = config.outFileName.substr(0, config.outFileName.size() - 4);
    config.infoFileName = config.infoFileName + ".txt";

    if (!config.roiFileName.empty()) {
        config.froi = fopen(config.roiFileName.c_str(), "rb");
        if(config.froi == NULL) {
            printf("Can not open ROI file.\n");
            printHelp();
            exit(0);
        }
    }

    config.fin = fopen(config.inFileName.c_str(), "rb");
    if(config.fin == NULL) {
        printf("Can not open input file.\n");
        printHelp();
        exit(0);
    }

    config.fout = fopen(config.outFileName.c_str(), "wb");
    if(config.fout == NULL) {
        printf("Can not open ouput file.\n");
        printHelp();
        exit(0);
    }

    config.finfo = fopen(config.infoFileName.c_str(), "wb");
    if(config.finfo == NULL) {
        printf("Can not open info file.\n");
        printHelp();
        exit(0);
    }

    EncoderCtx ctx(config);
    ctx.run();

    fclose(config.fin);
    fclose(config.fout);
    fclose(config.finfo);

    printf("Done.\n");
    printf("data file: %s\n", config.outFileName.c_str());
    printf("info file: %s\n",config.infoFileName.c_str());
    return 0;
}
