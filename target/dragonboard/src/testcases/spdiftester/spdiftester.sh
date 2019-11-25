#!/bin/sh

source send_cmd_pipe.sh
source script_parser.sh

#insmod spdif driver

module_count=`script_fetch "spdif" "module_count"`
if [ $module_count -gt 0 ]; then
	for i in $(seq $module_count); do
		key_name="module"$i"_path"
		module_path=`script_fetch "spdif" "$key_name"`
		if [ -n "$module_path" ]; then
			insmod "$module_path"
			if [ $? -ne 0 ]; then
				echo "insmod $module_path failed"
			fi
		fi
	done
fi


sleep 1

device_name=`script_fetch "spdif" "device_name"`
	spdiftester $* "$device_name" &
	exit 0
SEND_CMD_PIPE_FAIL $3
