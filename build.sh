#!/bin/sh
set -e

TOP=$(pwd)
echo "TOP = $TOP"

rm -r 3rdparty/install
mkdir 3rdparty/install

echo "Build OpenSSL"
cd 3rdparty/openssl
./Configure linux-mips32 no-async --prefix="$TOP/3rdparty/install"
make clean
make -j$(nproc) CC="$TOP/toolchain/bin/mips-linux-uclibc-gnu-gcc"
make -j$(nproc) install
cd ../../

echo "Build live555"
cd 3rdparty/live
make distclean
./genMakefiles wyze
PRUDYNT_ROOT="$TOP" make -j$(nproc)
PRUDYNT_ROOT="$TOP" make install
cd ../../

echo "Build prudynt"
rm -rf build
mkdir build
cd build
cmake ..
make
cd ..
