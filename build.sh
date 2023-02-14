#!/bin/bash
set -e

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
make -j$(nproc) install
cd ../../

echo "Build freetype2"
cd 3rdparty
rm -rf freetype
if [[ ! -f freetype-2.13.0.tar.xz ]]; then
    wget 'https://download.savannah.gnu.org/releases/freetype/freetype-2.13.0.tar.xz'
fi
tar xvf freetype-2.13.0.tar.xz
mv freetype-2.13.0 freetype
cd freetype
CC="${PRUDYNT_CROSS}gcc" ./configure --host mipsel-linux-gnu --prefix="$TOP/3rdparty/install/" --with-png=no --with-brotli=no --with-harfbuzz=no --with-zlib=no
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
./configure --disable-zlib --target-os=linux --disable-loongson3 --disable-mipsfpu --arch=mipsel --cc="${PRUDYNT_CROSS}gcc" --cxx="${PRUDYNT_CROSS}g++" --strip="${PRUDYNT_CROSS}strip" --prefix="$TOP/3rdparty/install" --enable-gpl --enable-cross-compile --extra-libs=-latomic --enable-version3
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

echo "Build prudynt"
rm -rf build
mkdir build
cd build
cmake -DPRUDYNT_CROSS="${PRUDYNT_CROSS}" ..
make
cd ..
