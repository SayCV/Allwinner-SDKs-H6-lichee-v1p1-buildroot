#!/bin/sh

cp ${LICHEE_BR_OUT}/external-toolchain/arm-linux-gnueabi/libc/lib/arm-linux-gnueabi/* ${LICHEE_BR_OUT}/target/lib/ -pfr 
chmod +x ${LICHEE_BR_OUT}/target/lib/*
rm -rf ${LICHEE_BR_OUT}/target/init
(cd ${LICHEE_BR_OUT}/target && ln -s bin/busybox init)

cat > ${LICHEE_BR_OUT}/target/etc/init.d/rcS << EOF
#!/bin/sh

ROOT_DEVICE=/dev/nandd


mount -t devtmpfs none /dev
mkdir /dev/pts
mount -t devpts none /dev/pts
mount -t sysfs sysfs /sys
mknod /dev/mali c 230 0
hostname sun8i

MODULES_DIR=/lib/modules/\`uname -r\`

for parm in \$(cat /proc/cmdline); do
    case \$parm in
        root=*)
        ROOT_DEVICE=\`echo \$parm | awk -F\= '{print \$2}'\`
        ;;
    esac
done

case \$ROOT_DEVICE in
    /dev/nand*)
        echo "nand boot"
        mkdir -p /boot
        mount /dev/nanda /boot
        ;;
    /dev/mtd*)
        # insmod tp driver
        echo "insmod device driver ko module files"
        insmod	/lib/modules/\`uname -r\`/gt82x.ko
        echo "nor flash boot"
        mkdir -p /tmp/data
        mount -t jffs2 /dev/mtdblock5  /tmp/data
        ;;
    /dev/mmc*)
        #mkdir -p /boot
        echo "mmc boot"
        #mount /dev/mmcblk0p2 /boot
        ;;
    *)
        mkdir -p /boot
        echo "default boot nand type"
        mount /dev/nanda /boot
        ;;
esac


for initscript in /etc/init.d/S[0-9][0-9]* /etc/init.d/auto_config_network
do
    if [ -x \$initscript ];then
        \$initscript start
    fi
done

EOF

sed -i '/TSLIB/d' ${LICHEE_BR_OUT}/target/etc/profile

echo "export TSLIB_TSEVENTTYPE=H3600" >> ${LICHEE_BR_OUT}/target/etc/profile
echo "export TSLIB_CONSOLEDEVICE=none" >> ${LICHEE_BR_OUT}/target/etc/profile
echo "export TSLIB_FBDEVICE=/dev/fb0" >> ${LICHEE_BR_OUT}/target/etc/profile
echo "export TSLIB_TSDEVICE=/dev/input/event2" >> ${LICHEE_BR_OUT}/target/etc/profile
echo "export TSLIB_CALIBFILE=/etc/pointercal" >> ${LICHEE_BR_OUT}/target/etc/profile
echo "export TSLIB_CONFFILE=/etc/ts.conf" >> ${LICHEE_BR_OUT}/target/etc/profile
echo "export TSLIB_PLUGINDIR=/usr/lib/ts" >> ${LICHEE_BR_OUT}/target/etc/profile
echo "" >> ${LICHEE_BR_OUT}/target/etc/profile

touch ${LICHEE_BR_OUT}/target/etc/init.d/auto_config_network

cat > ${LICHEE_BR_OUT}/target/etc/init.d/auto_config_network << EOF
#!/bin/sh

MAC_ADDR="\`cat /proc/cpuinfo | awk '\$1=="Serial" {print \$3}' | sed 's/../&:/g' | cut -c16-29\`"

ifconfig eth0 hw ether "00:\$MAC_ADDR"
ifconfig lo 127.0.0.1
udhcpc -b -R

EOF

chmod +x ${LICHEE_BR_OUT}/target/etc/init.d/auto_config_network

if [ ${LICHEE_PLATFORM} = "eyeseelinux" ] ; then
echo "not use skel"
else
(cd target/skel/ && tar -c *) |tar -C ${LICHEE_BR_OUT}/target/ -xv
fi


