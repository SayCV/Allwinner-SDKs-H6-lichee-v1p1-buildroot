#!/bin/sh
##############################################################################
# \version     1.0.0
# \date        2012年05月31日
# \author      James Deng <csjamesdeng@allwinnertech.com>
# \Descriptions:
#			create the inital version

# \version     1.1.0
# \date        2012年09月26日
# \author      Martin <zhengjiewen@allwinnertech.com>
# \Descriptions:
#			add some new features:
#			1.bt hotpoint ssid and single strongth san
#			2.sort the hotpoint by single strongth quickly
##############################################################################
source send_cmd_pipe.sh
source script_parser.sh

WIFI_HARDWARE_INFO=/data/wifi_hardware_info

module_path=`script_fetch "bluetooth" "module_path"`
loop_time=`script_fetch "bluetooth" "test_time"`
destination_bt=`script_fetch "bluetooth" "dst_bt"`
device_node=`script_fetch "bluetooth" "device_node"`
baud_rate=`script_fetch "bluetooth" "baud_rate"`
module_vendor=`script_fetch "bluetooth" "module_vendor"`

echo $module_path
echo $loop_time
echo "${destination_bt}"
echo $device_node
echo $baud_rate

if [ ! -x /etc/firmware ] ; then
	mkdir -p /etc/firmware
fi

ln -s /dragonboard/bin/*.hcd /etc/firmware/

echo 0 > /sys/class/rfkill/rfkill0/state
sleep 2
echo 1 > /sys/class/rfkill/rfkill0/state
sleep 2

for i in $(seq 10); do
	if [ -f ${WIFI_HARDWARE_INFO} ] ; then
		break;
	fi
	sleep 1
done

if [ -f ${WIFI_HARDWARE_INFO} ] ; then
	bt_firmware=`sed -n '/bt_firmware=/'p ${WIFI_HARDWARE_INFO} | sed 's/bt_firmware=//'`
	if [ "x${bt_firmware}" != "x" ]; then
		echo "bt_firmware="$bt_firmware
		bt_type=${bt_firmware:0:3}
		echo "bt_type="$bt_type
		if [ "x${bt_type}" = "xbcm" ] ; then
			module_path_temp="/etc/firmware/"${bt_firmware}
			device_node_temp="/dev/"${device_node}
			bluetooltester  --tosleep=50000 --no2bytes --bd_addr 11:22:33:44:55:66 --enable_hci --scopcm=0,2,0,0,0,0,0,0,0,0  --baudrate ${baud_rate} --use_baudrate_for_download --patchram ${module_path_temp}  $device_node_temp &
		else
			rtk_hciattach -n -s ${baud_rate} ${device_node} rtk_h5 &
		fi
	else
		SEND_CMD_PIPE_OK_EX $3 "no bt"
		exit 1
	fi
else
	if [ "x$module_vendor" = "xrtk" ]; then
		rtk_hciattach -n -s ${baud_rate} ${device_node} rtk_h5 &
	else
		if [ -z ${module_path} ] ; then
			echo "module_patch is null, module_path="${module_path}
			SEND_CMD_PIPE_FAIL $3
			exit 1
		fi
		bluetooltester  --tosleep=50000 --no2bytes --enable_hci --scopcm=0,2,0,0,0,0,0,0,0,0  --baudrate ${baud_rate} --use_baudrate_for_download --patchram ${module_path}  $device_node &
	fi
fi

sleep 5

for i in $(seq ${loop_time}); do
	cciconfig hci0 up
	if cciconfig hci0 | grep "hci0" ; then
		SEND_CMD_PIPE_OK_EX $3
		exit 0
	fi
	sleep 2
done

cciconfig hci0 down
SEND_CMD_PIPE_FAIL $3 "test timeout"
