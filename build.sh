#!/bin/bash
set -e

PRUDYNT_CROSS="ccache mipsel-linux-"

TOP=$(pwd)
echo "TOP = $TOP"

rm -rf 3rdparty/install
mkdir 3rdparty/install

echo "Build OpenSSL"
cd 3rdparty
rm -rf openssl
if [[ ! -f openssl-1.1.1t.tar.gz ]]; then
    wget 'https://www.openssl.org/source/openssl-1.1.1t.tar.gz'
fi
tar xvf openssl-1.1.1t.tar.gz
mv openssl-1.1.1t openssl
cd openssl
./Configure linux-mips32 no-async --prefix="$TOP/3rdparty/install"
make clean
make -j$(nproc) CC="${PRUDYNT_CROSS}gcc"
make -j$(nproc) install_sw
cd ../../

echo "Build freetype2"
cd 3rdparty
rm -rf freetype
if [[ ! -f freetype-2.13.0.tar.xz ]]; then
    wget 'https://download-mirror.savannah.gnu.org/releases/freetype/freetype-2.13.0.tar.xz'
fi
tar xvf freetype-2.13.0.tar.xz
mv freetype-2.13.0 freetype
cd freetype
CC="${PRUDYNT_CROSS}gcc" ./configure --host mipsel-linux-gnu --prefix="$TOP/3rdparty/install/" --without-harfbuzz --disable-largefile --disable-mmap --without-png --without-brotli --without-zlib
make -j$(nproc)
make install
cd ../../

echo "Build ffmpeg"
cd 3rdparty
rm -rf ffmpeg
if [[ ! -f ffmpeg-5.1.2.tar.xz ]]; then
    wget 'https://ffmpeg.org/releases/ffmpeg-5.1.2.tar.xz'
fi
tar xvf ffmpeg-5.1.2.tar.xz
mv ffmpeg-5.1.2 ffmpeg
cd ffmpeg
./configure --disable-zlib --target-os=linux --arch=mipsel --cpu=mips32r2 --disable-msa --cc="${PRUDYNT_CROSS}gcc" --cxx="${PRUDYNT_CROSS}g++" --strip="${PRUDYNT_CROSS}strip" --prefix="$TOP/3rdparty/install" --enable-gpl --enable-cross-compile --extra-libs=-latomic --enable-version3
make -j$(nproc)
make install
cd ../../

echo "Build libconfig"
cd 3rdparty
rm -rf libconfig
if [[ ! -f libconfig-1.7.3.tar.gz ]]; then
    wget 'https://hyperrealm.github.io/libconfig/dist/libconfig-1.7.3.tar.gz';
fi
tar xvf libconfig-1.7.3.tar.gz
mv libconfig-1.7.3 libconfig
cd libconfig
CC="${PRUDYNT_CROSS}gcc" CXX="${PRUDYNT_CROSS}g++" ./configure --host mipsel-linux-gnu --prefix="$TOP/3rdparty/install"
make -j$(nproc)
make install
cd ../../

echo "Build live555"
cd 3rdparty/live
if [[ -f Makefile ]]; then
    make distclean
fi
./genMakefiles wyze
PRUDYNT_ROOT="${TOP}" PRUDYNT_CROSS="${PRUDYNT_CROSS}" make -j$(nproc)
PRUDYNT_ROOT="${TOP}" PRUDYNT_CROSS="${PRUDYNT_CROSS}" make install
cd ../../

echo "Build libmuslshim"
cd 3rdparty
rm -rf ingenic-musl
if [[ ! -d ingenic-musl ]]; then
    git clone https://github.com/gtxaspec/ingenic-musl
    cd ingenic-musl
    if [[ "$2" == "-static" ]]; then
        make CC="${PRUDYNT_CROSS}gcc" static
    else
        make CC="${PRUDYNT_CROSS}gcc"
    fi
    cp libmuslshim.* $TOP/3rdparty/install/lib
fi
cd ../..

echo "Build prudynt"
make clean
/usr/bin/make -j13 \
ARCH= CROSS_COMPILE="${PRUDYNT_CROSS}" \
CFLAGS="-O0 -DALLOW_RTSP_SERVER_PORT_REUSE=1 \
-I./blob/include \
-I./3rdparty/install/include \
-I./3rdparty/install/include/freetype2 \
-I./3rdparty/install/include/liveMedia \
-I./3rdparty/install/include/groupsock \
-I./3rdparty/install/include/UsageEnvironment \
-I./3rdparty/install/include/BasicUsageEnvironment" \
-I./3rdparty/install/include/libavcodec" \
-I./3rdparty/install/include/libavdevice" \
-I./3rdparty/install/include/libavfilter" \
-I./3rdparty/install/include/libavformat" \
-I./3rdparty/install/include/libavutil" \
-I./3rdparty/install/include/libswresample" \
-I./3rdparty/install/include/libswscale" \
-I./3rdparty/install/include/libpostproc" \
LDFLAGS=" -L./3rdparty/install/lib -L./blob/lib" \
-C $PWD all
exit 0

