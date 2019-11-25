#!/bin/sh
##flashtester
##support emmc nand nand test
##substitute emmctester and nandtester with it

source send_cmd_pipe.sh
source script_parser.sh

BOOT_TYPE=-1                
STORAGE_TYPE=-1
STORAGE_NAME="NULL"
flashdev="/dev/null"
erase_flag=0
result="null"
capacity=0
have_realnum=
insmod_nand=0
#######step 0:get boot type from cmdline#######
for parm in $(cat /proc/cmdline); do
    case $parm in
        boot_type=*)
            BOOT_TYPE=`echo $parm | awk -F\= '{print $2}'`
            ;;
    esac
done

#######step 1 insmod nand driver#############
flash_activated=`script_fetch "flash" "activated"`
if [ $flash_activated -eq 1 ]; then
	if [ "$BOOT_TYPE" -eq "1"  -a ! -b "/dev/block/mmcblk1" -a ! -b "/dev/mmcblk1" ];then
		insmod_nand=1
    elif [ "$BOOT_TYPE" -eq "0" -a ! -b "/dev/nanda" -a ! -b "/dev/block/nanda" ];then
		insmod_nand=1
	fi
	nand_module_path=`script_fetch "flash" "module_path"`
	if [ -n "$nand_module_path" -a "${insmod_nand}" -eq "1" ]; then
       insmod "$nand_module_path"
	   echo "insmod nand ok"
	fi
fi


#######step 2:get flashdev and STORAGE_TYPE#######
#chip=`script_fetch_chip "chip" "chip_name"`

#BOOT_TYPE
# 0:nand
# 1:sdcard
# 2:emmc
#STORAGE_TYPE
# 0:NAND
# 1:EMMC
if [ "$BOOT_TYPE" -eq "1" ];then #card boot
	if [ -b "/dev/mmcblk1" ];then
		flashdev="/dev/mmcblk1"
		STORAGE_TYPE=1
	elif [ -b "/dev/block/mmcblk1" ];then
		flashdev="/dev/block/mmcblk1"
		STORAGE_TYPE=1
	elif [ -b "/dev/nanda" ];then
		flashdev="/dev/nanda"
		STORAGE_TYPE=0
	elif [ -b "/dev/block/nanda" ];then
		flashdev="/dev/block/nanda"
		STORAGE_TYPE=0
	else
		SEND_CMD_PIPE_FAIL $3
		echo "card boot but can not find mmcblk1 and nanda"
		exit 1
	fi

elif [ "$BOOT_TYPE" -eq "0" ];then #nand boot
	if [ -b "/dev/nanda" ];then
		flashdev="/dev/nanda"
	elif [ -b "/dev/block/nanda" ];then
		flashdev="/dev/block/nanda"
	else
		SEND_CMD_PIPE_FAIL $3
		echo "can not find nanda"
		exit 1
	fi
		STORAGE_TYPE=0

elif [ "$BOOT_TYPE" -eq "2" ];then #emmc boot
	if [ -b "/dev/mmcblk0p1" ];then
		flashdev="/dev/mmcblk0p1"
	elif [ -b "/dev/block/mmcblk0p1" ];then
		flashdev="/dev/block/mmcblk0p1"
	elif [ -b "/dev/mmcblk0" ];then
		flashdev="/dev/mmcblk0"
	elif [ -b "/dev/block/mmcblk0" ];then
		flashdev="/dev/block/mmcblk0"
	else
	    SEND_CMD_PIPE_FAIL $3
		echo "can not find mmcblk0"
		exit 1
	fi
		STORAGE_TYPE=1
else
	SEND_CMD_PIPE_FAIL $3
	echo "no define boot_type"
	exit 1
fi

if [ "$STORAGE_TYPE" -eq "1" ];then
	STORAGE_NAME="emmc"
elif [ "$STORAGE_TYPE" -eq "0" ];then
	STORAGE_NAME="nand"
else
	SEND_CMD_PIPE_FAIL $3
	exit 1
fi
echo "flashdev=$flashdev"



#######step 3  get the capacity info#######
if [ "$STORAGE_TYPE" -eq "1" -a "$BOOT_TYPE" -eq "1" ];then #emmc stoarge and card boot
	capacity=`cat /sys/block/mmcblk1/size|grep -o '\<[0-9]\{1,\}.[0-9]\{1,\}\>'|awk '{sum+=$0}END{printf "%.2f\n",sum/2097152}'`
elif [ "$STORAGE_TYPE" -eq "1" -a "$BOOT_TYPE" -eq "2" ];then #emmc stoarge and emmc boot
	capacity=`cat /sys/block/mmcblk0/size|grep -o '\<[0-9]\{1,\}.[0-9]\{1,\}\>'|awk '{sum+=$0}END{printf "%.2f\n",sum/2097152}'`
elif [ "$STORAGE_TYPE" -eq "0" -a "$BOOT_TYPE" -eq "0" ];then #nand stoarge and nand boot
	capacity=`cat /sys/block/nand*/size|grep -o '\<[0-9]\{1,\}.[0-9]\{1,\}\>'|awk '{sum+=$0}END{printf "%.2f\n",sum/2097152}'`
elif [ "$STORAGE_TYPE" -eq "0" -a "$BOOT_TYPE" -eq "1" ];then #nand stoarge and card boot
	echo "we will support the function in future"
else
	SEND_CMD_PIPE_FAIL $3
	echo "get device capacity fail"
	exit 1
fi

have_realnum=`echo $capacity | grep -E [1-9]`
echo "have_realnum=${have_realnum}"
if [ -n "$have_realnum" ];then
	capacity="":"${capacity}"G""
	result=${STORAGE_NAME}${capacity}
else
	result=${STORAGE_NAME}
fi
echo "$flashdev result: $result"


#######step 4  format partition#######
erase_flag=`script_fetch "flash" "erase"`
if [ "$BOOT_TYPE" -eq "1" ]; then #only card boot and user set erase_flag,go to write and read test truely
	if [ $erase_flag -eq 1 -a ${STORAGE_TYPE} -eq "1" ]; then 
		mkfs.vfat $flashdev
		if [ $? -ne 0 ];then
			SEND_CMD_PIPE_FAIL $3
			echo "make filesystem fail"
			exit 1
		fi
		echo "create vfat file system for $flashdev done"
	elif [ $erase_flag -eq 0 ]; then #if card boot,but erase_flag=0,skip write and read test
		SEND_CMD_PIPE_OK_EX $3 ${result}
		exit 0
	fi
elif [ "$BOOT_TYPE" -eq "0" -o "$BOOT_TYPE" -eq "2" ];then #if boot from flash,write and read ok default
	SEND_CMD_PIPE_OK_EX $3 ${result}
	exit 0
else
	SEND_CMD_PIPE_FAIL $3
	exit 1
fi



#######step 5: start read and write test#######
test_size=64
test_size=`script_fetch "flash" "test_size"`
if [ -z "$test_size" -o $test_size -le 0 ];then
    test_size=64
fi

echo "test_size=$test_size"
echo "flash test read and write"

if [ "$STORAGE_TYPE" -eq "1" ];then
	emmcrw "$flashdev" "$test_size"
elif [ "$STORAGE_TYPE" -eq "0" ];then
	nandrw_new "$flashdev"
else
	SEND_CMD_PIPE_FAIL $3
	exit 1
fi

if [ $? -ne 0 ];then
    SEND_CMD_PIPE_FAIL $3
    echo "flash read and write test fail"
	exit 1
else
	SEND_CMD_PIPE_OK_EX $3 ${result}
    echo "flash test ok!!"
	exit 0
fi
