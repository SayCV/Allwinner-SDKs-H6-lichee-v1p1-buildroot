# fetch data from script
# \param $1 section name
# \param $2 key name
# 
# example:
#   var=`script_fetch mmc display_name`
script_fetch()
{
    filp="/boot/test_config.fex"
    section=$1
    key=$2

    item=`awk -F '=' '/\['"$section"'\]/{a=1}a==1&&$1~/'"$key"'/{gsub(/[[:blank:]]*/,"",$0); print $0; exit}' $filp`
    value=${item#*=}
    start=${value:0:7}
    if [ "$start" = "string:" ]; then
        retval=${value#*string:}
    else
        start=${value:0:1}
        if [ "$start" = "\"" ]; then
            retval=${value#*\"}
            retval=${retval%\"*}
        else
            retval=$value
        fi
    fi
    echo $retval
}

# fetch chip from script
# \param $1 section name
# \param $2 key name
# 
# example:
#   var=`script_fetch chip chip_name`
script_fetch_chip()
{
    filp="/dragonboard/platform.fex"
    main_key=$1
    sub_key=$2

    item=`awk -F '=' '/\['"$main_key"'\]/{a=1}a==1&&$1~/'"$sub_key"'/{gsub(/[[:blank:]]*/,"",$0); print $0; exit}' $filp`
    value=${item#*=}
    start=${value:0:7}
    if [ "$start" = "string:" ]; then
        retval=${value#*string:}
    else
        start=${value:0:1}
        if [ "$start" = "\"" ]; then
            retval=${value#*\"}
            retval=${retval%\"*}
        else
            retval=$value
        fi
    fi
    echo $retval
}