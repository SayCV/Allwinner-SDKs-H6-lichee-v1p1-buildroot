#!/bin/sh

show_help()
{
    printf "
    ./build.sh [-c] [-h]
    OPTIONS
        -c      Copy rootfs.ext4 to ../../../out/dragonboard
        -h      Display help message
    "
}

build_clean()
{
	make -C src clean
	rm -rf src/lib/libscript.a
	rm -rf src/lib/libdisp.a

	#rm -rf rootfs
	#rm -rf rootfs.ext4
	rm -rf sysroot
}

OUT_PATH=""
BR_ROOT=`(cd ../..; pwd)`
export PATH=$PATH:$BR_ROOT/target/tools/host/usr/bin
export PATH=$PATH:$BR_ROOT/output/external-toolchain/bin

while getopts hc OPTION
do
    case $OPTION in
        h)
            show_help
            exit 0
            ;;
        c)
            OUT_PATH="../../../out/dragonboard"
            ;;
    esac
done

build_clean

# sysroot exist?
if [ ! -d "./sysroot" ]; then
    echo "extract sysroot.tar.gz"
    tar zxf sysroot.tar.gz
fi

if [ ! -d "./output/bin" ]; then
    mkdir -p ./output/bin
fi

#export cpu env
export CPU=${LICHEE_CHIP%p*}
echo $CPU

#support the multiple
cd src
rm -rf ./include/asm/arch
ln -s `pwd`/include/asm/arch-${LICHEE_CHIP} `pwd`/include/asm/arch

make
if [ $? -ne 0 ]; then
    exit 1
fi
cd ..

if [ ! -d "rootfs/dragonboard" ]; then
    mkdir -p rootfs/dragonboard
fi

cp -rf extra/* rootfs/
rm -rf rootfs/dragonboard/*
cp -rf output/* rootfs/dragonboard/

#add file to check what chip used
echo "[chip]" > rootfs/dragonboard/platform.fex
echo "chip_name=" >> rootfs/dragonboard/platform.fex
sed -i 's/\(chip_name=\).*/\1'${LICHEE_CHIP}'/' rootfs/dragonboard/platform.fex


echo "generating rootfs..."

NR_SIZE=`du -sm rootfs | awk '{print $1}'`
NEW_NR_SIZE=$(((($NR_SIZE+32)/16)*16))
#NEW_NR_SIZE=360
TARGET_IMAGE=rootfs.ext4

echo "blocks: $NR_SIZE"M" -> $NEW_NR_SIZE"M""
make_ext4fs -l $NEW_NR_SIZE"M" $TARGET_IMAGE rootfs/
fsck.ext4 -y $TARGET_IMAGE > /dev/null
echo "success in generating rootfs"

if [ -n "$OUT_PATH" ]; then
    cp -v rootfs.ext4 $OUT_PATH/
	if [ $? -ne 0 ] ; then
		exit 1
	fi
fi

echo "Build at: `date`"
