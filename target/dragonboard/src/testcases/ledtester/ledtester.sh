#!/bin/sh
##############################################################################
# \version     1.0.0
# \date        2014Äê04ÔÂ11ÈÕ
# \author      MINI_NGUI<liaoyongming@allwinnertech.com>
# \Descriptions:
#			add test LED for homlet 
##############################################################################
source send_cmd_pipe.sh
source script_parser.sh

module_path=`script_fetch "gpio_led" "module_path"`
led_count=`script_fetch "gpio_led" "led_count"`

if [ -z "$module_path" ]; then
    echo "no gpio-sunxi.ko to install"
    SEND_CMD_PIPE_FAIL $3
    exit 1
else
	echo "begin install gpio-sunxi.ko"
    insmod "$module_path"
    if [ $? -ne 0 ]; then
        echo "inStall gpio-sunxi.ko failed"
    fi
fi

if [ $led_count -gt 0 ] ; then
	while true ; do
		for i in $(seq $led_count); do
			temp_name="led"$i"_name"
			led_name=`script_fetch "gpio_led" "$temp_name"`
			if [ -n led_name ] ; then
				if [ ! -d "/sys/class/gpio_sw/$led_name" ]; then
					echo "has no ${led_name} node,mabey cant intall gpio-sunxi.ko"
					SEND_CMD_PIPE_FAIL $3
					exit 1
				else
					echo 1 > "/sys/class/gpio_sw/"$led_name"/data"
					if [ $? -ne 0 ]; then
						echo "set ${led_name} data to 1 err"
						SEND_CMD_PIPE_FAIL $3
						exit 1
					fi
					usleep 200000
					echo 0 > "/sys/class/gpio_sw/"$led_name"/data"
					if [ $? -ne 0 ]; then
						echo "set ${led_name} data to 0 err"
						SEND_CMD_PIPE_FAIL $3
						exit 1
					fi
				fi
			fi
		done
		SEND_CMD_PIPE_OK_EX $3
		usleep 200000
	done
fi
echo "no led to test"
SEND_CMD_PIPE_FAIL $3



