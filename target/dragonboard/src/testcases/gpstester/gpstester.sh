#!/bin/sh

source send_cmd_pipe.sh
source script_parser.sh

device_node=`script_fetch "gps" "device_node"`
baud_rate=`script_fetch "gps" "baud_rate"`

echo "gpstester: device_node = ${device_node}"
echo "gpstester: baud_rate = ${baud_rate}"

for j in `seq 3`;do
	gpstester "${device_node}" "${baud_rate}"
	if [ $? -eq 0 ]; then
		SEND_CMD_PIPE_OK $3
		exit 0
	fi
	sleep 1
done
SEND_CMD_PIPE_FAIL $3
exit 1