#!/bin/sh

source send_cmd_pipe.sh
source script_parser.sh

chip=`script_fetch_chip "chip" "chip_name"`
module_count=`script_fetch "tv" "module_count"`
if [ $module_count -gt 0 ]; then
    for i in $(seq $module_count); do
        key_name="module"$i"_path"
        module_path=`script_fetch "tv" "$key_name"`
        if [ -n "$module_path" ]; then
            echo "insmod $module_path"
            insmod "$module_path"
            if [ $? -ne 0 ]; then
                echo "insmod $module_path failed"
            fi
        fi
    done
fi

sleep 3

if [ "x$chip" = "xsun50iw6p1" ] ; then
	for i in `seq 0 15`
	do
		audio_name=`cat /proc/asound/card$i/id`
		if [ "$audio_name" = "sndacx00codec" ]; then
			device_id=$i
			echo device_id is $device_id
			break
		fi
	done
	source config_ac200.sh $device_id
else
	source config_ac100.sh
fi
device_name=`script_fetch "tv" "device_name"`
	tvtester $* "$device_name" &
	exit 0
SEND_CMD_PIPE_FAIL $3
