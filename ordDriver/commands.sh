#! /bin/bash

mode="0666"

sudo rm /dev/sortlist

sudo make clean

echo "COMMAND LSPCI: "

lspci | grep $1


echo "COMMAND DMESG: "

dmesg | tail -1


mod=$1
mod+=".ko"

sudo rmmod $mod

sudo make

sudo insmod $mod


echo "COMMAND DMESG: "

dmesg | tail -1


echo "COMMAND LSMOD: "

lsmod | head -1

#major=$(cat /proc/devices | awk '{ if($2 == "memory"){ print $1 } }')
major=`cat /proc/devices | grep $1 | awk '{print $1}'`

echo "VALUE OF MAJOR NUMBER: $major"

cat /proc/devices | grep $major

echo "SHOW THE LIST OF /DEV: "
ls -l /dev | grep $major | grep $1

#echo "[CREATE]: mknod /dev/$1 --mode=$modo c $major 0  \n"
#mknod /dev/$1 --mode=$mode c $major 0

# echo -n 3>/dev/sortlist

# dmesg | tail -1

# echo -n 1>/dev/sortlist

# dmesg | tail -1

# echo -n 5>/dev/sortlist

# dmesg | tail -1

# echo -n 7>/dev/sortlist

# dmesg | tail -1

# echo -n 4>/dev/sortlist

# dmesg | tail -1

# cat /dev/sortlist

# dmesg | tail -10