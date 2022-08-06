#!/bin/bash
set -e

TOP=$(pwd)
echo "TOP = $TOP"

rm -rf 3rdparty/install
mkdir 3rdparty/install

echo "Build OpenSSL"
cd 3rdparty/openssl
./Configure linux-mips32 no-async --prefix="$TOP/3rdparty/install"
make clean
make -j$(nproc) CC="${PRUDYNT_CROSS}gcc"
make -j$(nproc) install
cd ../../

echo "Build freetype2"
cd 3rdparty
rm -rf freetype
if [[ ! -f freetype-2.10.4.tar.xz ]]; then
    wget 'https://download.savannah.gnu.org/releases/freetype/freetype-2.10.4.tar.xz'
fi
tar xvf freetype-2.10.4.tar.xz
mv freetype-2.10.4 freetype
cd freetype
CC="${PRUDYNT_CROSS}gcc" ./configure --host mipsel-linux-gnu --prefix="$TOP/3rdparty/install/" --with-png=no --with-brotli=no --with-harfbuzz=no --with-zlib=no
make -j$(nproc)
make install
cd ../../

echo "Build ffmpeg"
cd 3rdparty
rm -rf ffmpeg
if [[ ! -f ffmpeg-4.4.1.tar.xz ]]; then
    wget 'https://web.archive.org/web/20220429132854if_/https://ffmpeg.org/releases/ffmpeg-4.4.1.tar.xz'
fi
tar xvf ffmpeg-4.4.1.tar.xz
mv ffmpeg-4.4.1 ffmpeg
cd ffmpeg
./configure --disable-zlib --target-os=linux --disable-loongson3 --disable-mipsfpu --arch=mipsel --cc="${PRUDYNT_CROSS}gcc" --cxx="${PRUDYNT_CROSS}g++" --strip="${PRUDYNT_CROSS}strip" --prefix="$TOP/3rdparty/install" --enable-gpl --enable-cross-compile --extra-libs=-latomic --enable-version3
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

echo "Build prudynt"
rm -rf build
mkdir build
cd build
cmake -DPRUDYNT_CROSS="${PRUDYNT_CROSS}" ..
make
cd ..
