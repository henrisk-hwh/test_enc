#1.cmake verson，指定cmake版本
cmake_minimum_required(VERSION 3.2)

#2.project name，指定项目的名称，一般和项目的文件夹名称对应
PROJECT(testEncAllInOne)

add_compile_options(-std=c++11
                    -Wall
                    -Wextra
                    -ffast-math
                    -fno-stack-protector
                    -fomit-frame-pointer
                    -ffunction-sections
                    -fdata-sections
                    -Wl,--gc-sections
                    -fpermissive
                    -mpreferred-stack-boundary=4
                    -g)
ADD_DEFINITIONS(-DH264E_SVC_API=1 -DNDEBUG -DH264E_MAX_THREADS=1)

INCLUDE_DIRECTORIES(
    ../include
    ../../minih264
    ./openh264
    ./x264
    ./minih264
    .
)

LINK_DIRECTORIES(../lib/)
LINK_LIBRARIES(libopenh264.a libx264.a pthread dl m)

#6.add executable file，添加要编译的可执行文件
ADD_EXECUTABLE(${PROJECT_NAME} main.cpp video_codec_impl.cpp openh264/openh264_encoder.cpp x264/x264_encoder.cpp minih264/minih264_encoder.cpp) # 生成动态库或共享库

INSTALL(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
