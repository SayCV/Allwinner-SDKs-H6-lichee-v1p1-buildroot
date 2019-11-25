#!/bin/sh

source send_cmd_pipe.sh
source script_parser.sh

csi_activated=`script_fetch "camera" "activated"`
usb_activated=`script_fetch "camera" "usb_activated"`
tvd_activated=`script_fetch "camera" "tvd_activated"`
interval_time=`script_fetch "camera" "switch_time"`

send_result_msg=0
result=""
result1=0
result2=0
result3=0

chip=`script_fetch_chip "chip" "chip_name"`
while true
do
    #start csi camera tester
	if [ $csi_activated -eq 1 ]; then
		if [ "x$chip" = "xsun8iw6p1" ] ; then
			/dragonboard/bin/cameratester -t $interval_time
		elif [ "x$chip" = "xsun8iw11p1" ] ; then
			/dragonboard/bin/cameratester -d /dev/video0 -p [0:800] -s 400x480 -t $interval_time
		fi
		result1=$?
	fi
	
	#start csi camera tester
	if [ $usb_activated -eq 1 ]; then
		/dragonboard/bin/usbcamtester -t $interval_time
		result2=$?
	fi
	
	#start tvd camera tester
	if [ $tvd_activated -eq 1 ]; then
		/dragonboard/bin/tvdcamtester -d /dev/video4 -p [0:800] -s 400x480 -r 720*576 -t $interval_time
		result3=$?
	fi
	
	#send the result of test
	if [ $send_result_msg -eq 0 ]; then
		if [ $csi_activated -eq 1 -a ${result1} -eq 0 ]; then
			result="csi_cam:pass"
		elif [ $csi_activated -eq 1 -a ${result1} -ne 0 ]; then
			result="csi_cam:fail"
		fi
		
		if [ $usb_activated -eq 1 -a ${result2} -eq 0 ]; then
			result="${result}",usb_cam:pass""
		elif [ $usb_activated -eq 1 -a ${result2} -ne 0 ]; then
			result="${result}",usb_cam:fail""
		fi
		
		if [ $tvd_activated -eq 1 -a ${result3} -eq 0 ]; then
			result="${result}",tvd_cam:pass""
		elif [ $tvd_activated -eq 1 -a ${result3} -ne 0 ]; then
			result="${result}",tvd_cam:pass""
		fi
		send_result_msg=1
		
		if [ ${result1} -eq 0 -a ${result2} -eq 0 -a ${result3} -eq 0 ]; then
			SEND_CMD_PIPE_OK_EX $3 ${result}
		else
			SEND_CMD_PIPE_FAIL_EX $3 ${result}
		fi
	fi
done