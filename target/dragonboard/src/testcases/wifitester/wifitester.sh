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
#			1.wifi hotpoint ssid and single strongth san
#			2.sort the hotpoint by single strongth quickly
##############################################################################
source send_cmd_pipe.sh
source script_parser.sh


mkdir -p /etc/firmware
mkdir -p /system/etc/firmware
ln -s /dragonboard/bin/*.bin /etc/firmware/
ln -s /dragonboard/bin/*.txt /etc/firmware/
ln -s /dragonboard/bin/*.cfg /etc/firmware/
ln -s /dragonboard/bin/esp_init_data.bin /system/vendor/modules/
ln -s /dragonboard/bin/init_data.conf /system/vendor/modules/
ln -s /etc/firmware/* /system/etc/firmware/

WIFI_PIPE=/tmp/wifi_pipe
WIFI_HARDWARE_INFO=/data/wifi_hardware_info
wlan_try=0
module_path=""
module_args=""
module_arg_num=0

wifitester
if [ $? -ne 0 ]; then
	module_path=`script_fetch "wifi" "module_path"`
	module_args=`script_fetch "wifi" "module_args"`
	echo "insmod ${module_path} ${module_args}"

	if [ -z "$module_path" ]; then
		SEND_CMD_PIPE_FAIL $3
		exit 1
	fi
	insmod "${module_path}" $module_args
	if [ $? -ne 0 ]; then
		SEND_CMD_PIPE_FAIL $3
		exit 1
	fi
else
	wifi_driver_name=`sed -n '/driver_name=/'p ${WIFI_HARDWARE_INFO} | sed 's/driver_name=//'`
	module_path="/system/vendor/modules/"${wifi_driver_name}".ko"
	module_arg0=`sed -n '/module_arg0=/'p ${WIFI_HARDWARE_INFO} | sed 's/module_arg0=//'`

	if [ -z "$module_path" ]; then
		SEND_CMD_PIPE_FAIL $3
		exit 1
	fi
	echo "insmod ${module_path} ${module_arg0}"
	insmod "${module_path}" "${module_arg0}"
	if [ $? -ne 0 ]; then
		SEND_CMD_PIPE_FAIL $3
		exit 1
	fi
fi

flag="######"

ifconfig_count=0
while true ; do
	if ifconfig -a | grep wlan0; then
		# enable wlan0
		module_arg_num=`sed -n '/module_arg_num=/'p ${WIFI_HARDWARE_INFO} | sed 's/module_arg_num=//'`
		if [ "x${module_arg_num}" = "x" ] ; then
			echo "module_arg_num="${module_arg_num}
		else
			arg_count=1
			while [ ${arg_count} -le ${module_arg_num} ]
			do
				temp="module_arg"${arg_count}
				arg_temp=`sed -n '/'${temp}'=/'p ${WIFI_HARDWARE_INFO} | sed 's/'${temp}'=//'`
				arg_value=${arg_temp##*=}
				arg_name=${arg_temp%=*}
				echo $arg_value > /sys/module/$wifi_driver_name/parameters/$arg_name
				arg_count=$((arg_count + 1 ))
			done
			if [ $((arg_count - 1 )) -ne  ${module_arg_num} ] ; then
				echo "args num not match"
				SEND_CMD_PIPE_FAIL $3
				exit 1
			fi
		fi

		for i in `seq 3`; do
			ifconfig wlan0 up > /dev/null
			if [ $? -ne 0 -a $i -eq 3 ]; then
				echo "ifconfig wlan0 up failed, no more try"
				SEND_CMD_PIPE_FAIL $3
				exit 1
			fi
			if [ $? -ne 0 ]; then
				echo "ifconfig wlan0 up failed, try again 1s later"
				sleep 1
			else
				echo "ifconfig wlan0 up done"
				break
			fi
		done
	else
		ifconfig_count=`expr $ifconfig_count + 1`;
		if [ ${ifconfig_count} -gt 3 ] ; then
			SEND_CMD_PIPE_FAIL $3
			exit 1
		fi
		sleep 2
		continue
	fi

	wifi=`iw dev wlan0 scan | awk -F"[:|=]" '(NF&&$1~/^[[:space:]]*SSID/) \
									{printf "%s:",substr($2,2,length($2)-1)}\
									(NF&&/[[:space:]]*signal/){printf "%d\n",$2 }'\
									| sort -n -r -k 2 -t :`
	for item in $wifi ; do
		echo $item >> $WIFI_PIPE
		done

	echo $flag >> $WIFI_PIPE
	# update in 3s
	sleep 3

	# disable wlan0
	ifconfig wlan0 down
	if [ $? -ne 0 ]; then
		echo $flag >> $WIFI_PIPE
		SEND_CMD_PIPE_FAIL $3
		exit 1
	fi

	# test done
	SEND_CMD_PIPE_OK $3
	sleep 5
done

# test failed
echo "wlan0 not found, no more try"
SEND_CMD_PIPE_FAIL $3
exit 1
