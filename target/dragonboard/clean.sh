#!/bin/sh

cd src
make clean
rm -rf lib/libscript.a
rm -rf lib/libdisp.a
cd ..

rm -rf rootfs
rm -rf rootfs.ext4
rm -rf sysroot

