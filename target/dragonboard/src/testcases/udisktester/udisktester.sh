#!/bin/sh

source send_cmd_pipe.sh

while true; do
    for nr in a b c d e f g h i j k l m n o p q r s t u v w x y z; do
        udisk="/dev/sd$nr"
        part=$udisk

        while true; do
            while true; do
                if [ -b "$udisk" ]; then
                    sleep 1
                    if [ -b "$udisk" ]; then
						echo "udisk="$udisk
                        echo "udisk insert"
                        break;
                    fi
                else
                    sleep 1
                fi
            done
            
            if [ ! -d "/tmp/udisk" ]; then
                mkdir /tmp/udisk
            fi
            
            mount $udisk /tmp/udisk
            if [ $? -ne 0 ]; then
				for i in `seq 10`; do
					udiskp=$udisk$i
					mount $udiskp /tmp/udisk
					if [ $? -ne 0 ]; then
						if [ $i = 10 ] ; then
							sleep 3
							SEND_CMD_PIPE_FAIL $3
							continue 3
						else
							echo "check "$udiskp" failed"
						fi
					else
						part=$udiskp
						break
					fi
				done
            fi
            break
        done
    
        capacity=`df -h | grep $part | awk '{printf $2}'`
        echo "$part: $capacity"
        
        SEND_CMD_PIPE_OK_EX $3 $capacity
    
        while true; do
            if [ -b "$udisk" ]; then
                sleep 1
            else
            	umount /tmp/udisk/$udisk
                echo "udisk removed"
                break
            fi
        done
    done
done
