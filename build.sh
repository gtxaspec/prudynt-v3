#!/bin/sh
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
