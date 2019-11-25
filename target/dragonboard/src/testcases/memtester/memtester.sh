#!/bin/sh

source send_cmd_pipe.sh
source script_parser.sh

debug_node=/sys/class/sunxi_dump/dump
#sdram config register
chip=`script_fetch_chip "chip" "chip_name"`
if [ "x$chip" = "xsun50iw1p1" -o "x$chip" = "xsun50iw2p1" ] ; then
	SDCR="0x01c62000"
elif [ "x$chip" = "xsun8iw11p1" -o "x$chip" = "xsun8iw7p1" \
	 -o "x$chip" = "xsun8iw6p1" -o "x$chip" = "xsun9iw1p1" ] ; then
	SDCR="0xf1c62000"
	FLAG="0x01c65004"
else
	SDCR="0x04002000"
fi


reg_read()
{
 echo $1 > $debug_node 
 cat $debug_node 
}

get_dram_size()
{
if [ "x$chip" = "xsun50iw1p1" -o "x$chip" = "xsun50iw2p1" \
	-o "x$chip" = "xsun8iw6p1" -o "x$chip" = "xsun8iw7p1" \
	-o "x$chip" = "xsun8iw11p1" -o "x$chip" = "xsun9iw1p1" ] ; then
	sdcr_value=`reg_read $SDCR`
    let "page_size=($sdcr_value>>8)&0xf"
    if [ $page_size -eq 7 ]; then
        dram_size=1
    elif [ $page_size -eq 8 ]; then
        dram_size=2
    elif [ $page_size -eq 9 ]; then
        dram_size=4
    elif [ $page_size -eq 10 ]; then
        dram_size=8
    else 
        dram_size=0
    fi
    
    let "row_addr_width=($sdcr_value>>4)&0xf"
    let "dram_size *=(1<<($row_addr_width-9))"
    let "bank_addr_width=($sdcr_value>>2)&0x3"
    let "dram_size *=(4<<$bank_addr_width)"
    let "dual_channel_enable=($sdcr_value>>19)&0x1"
    let "dram_size *=($dual_channel_enable+1)"
    let "rank_addr_width=($sdcr_value>>0)&0x3"
    let "dram_size *=($rank_addr_width+1)"
    echo $dram_size
else
	sdcr_value=`reg_read $SDCR`
	let "page_size=($sdcr_value>>4)&0xf"
	let "dram_size=($page_size<<8)"
	echo $dram_size
fi
}


dram_size=`script_fetch "dram" "dram_size"`
test_size=`script_fetch "dram" "test_size"`

actual_size=`get_dram_size`

path="../d/" 
if [ ! -d "$path" ]; then 
	mkdir "$path" 
fi
mount -t debugfs none "$path"

if [ "x$chip" = "xsun8iw6p1" -o "x$chip" = "xsun8iw7p1" ] ; then
	pll6_freq=`cat /d/clk/hosc/pll_ddr/clk_rate`
	let "pll6_freq=$pll6_freq/1000000"
	echo "pll6_freq:$pll6_freq"M""
	let "dram_freq=$pll6_freq>>1"

elif [ "x$chip" = "xsun9iw1p1" ] ; then
	pll6_freq=`cat /d/clk/hosc/pll6/clk_rate`
	let "pll6_freq=$pll6_freq/1000000"
	echo "pll6_freq:$pll6_freq"M""
	flag_value=`reg_read $FLAG`
	let "flag_value_bit17=($flag_value>>17)&0x1"
	echo "flag_value_bit17:$flag_value_bit17"
	if [ $flag_value_bit17 -eq 0 ]; then
	    let "dram_freq=$pll6_freq<<1"
	else
	    let "dram_freq=$pll6_freq>>1"
	fi
elif [ "x$chip" = "xsun50iw1p1" -o "x$chip" = "xsun50iw2p1" -o "x$chip" = "xsun50iw6p1" ] ; then
	dram_freq=`cat /sys/class/devfreq/dramfreq/cur_freq | awk -F: '{print $1}'`
	let "dram_freq=$dram_freq/1000"
else
	factor_N=`cat /proc/ccmu | awk -F: '(NF&&$1~/ *Pll5Ctl.FactorN$/) {print $2}' `
	factor_K=`cat /proc/ccmu | awk -F: '(NF&&$1~/ *Pll5Ctl.FactorK$/) {print $2}' `
	factor_M=`cat /proc/ccmu | awk -F: '(NF&&$1~/ *Pll5Ctl.FactorM$/) {print $2}' `
	let "dram_freq=(24*$factor_N*$factor_K)/$factor_M"
fi

echo "dram_freq=$dram_freq"M""
echo "config dram_size=$dram_size"M""
echo "actual_size=$actual_size"M""
echo "test_size=$test_size"M""

if [ $actual_size -lt $dram_size ]; then
   SEND_CMD_PIPE_FAIL_EX $3 "size "$actual_size"M"" error"
   exit 0
fi
SEND_CMD_PIPE_MSG $3 "size:$actual_size"M" freq:$dram_freq"MHz""
memtester $test_size"M" 1 > /dev/null 
if [ $? -ne 0 ]; then
    echo "memtest fail"
    SEND_CMD_PIPE_FAIL_EX $3 "size:$actual_size"M" freq:$dram_freq"MHz""
else
    echo "memtest success!"
    SEND_CMD_PIPE_OK_EX $3 "size:$actual_size"M" freq:$dram_freq"MHz""
fi
