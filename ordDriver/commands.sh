#! /bin/bash
#$1 is the name of your module

mode="0666"
sudo rm /dev/sortlist
sudo make clean

echo "COMMAND LSPCI: "
lspci | grep $1

echo "COMMAND DMESG: "
dmesg | tail -1

mod=$1
mod+=".ko"

echo "RUM MAKE: "
sudo rmmod $mod
sudo make
sudo insmod $mod

echo "COMMAND DMESG: "
dmesg | tail -1

echo "COMMAND LSMOD: "
lsmod | head -2

#major=$(cat /proc/devices | awk '{ if($2 == "memory"){ print $1 } }')
major=`cat /proc/devices | grep $1 | awk '{print $1}'`

echo "VALUE OF MAJOR NUMBER: $major"
cat /proc/devices | grep $major

echo "SHOW THE LIST OF /DEV: "
ls -l /dev | grep $major | grep $1

echo -n 3 >/dev/sortlist
echo -n 1 >/dev/sortlist
echo -n 5 >/dev/sortlist
echo -n 7 >/dev/sortlist
echo -n 100 >/dev/sortlist
echo -n 44 >/dev/sortlist
echo -n 4 >/dev/sortlist

dmesg | tail -10

cat /dev/sortlist

dmesg | tail -10