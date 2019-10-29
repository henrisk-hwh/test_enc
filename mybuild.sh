#!/bin/bash

cd openh264
#make clean
#make OS=android NDKROOT=~/usr/bin/android-ndk-r10e TARGET=android-21 -j32
#cp libopenh264.a ~/workspace/RTCSDK-new/nemolib/thirdparty/openh264/dist/android/lib/libopenh264.a

make -j32
make install DESTDIR=../nemo-test/ PREFIX=.

cd ../x264
./configure --disable-asm --prefix=../nemo-test/
make -j32
make install
make install install-lib-static


cd ../
mkdir -p build
cd build
cmake ../nemo-test/test/
make
