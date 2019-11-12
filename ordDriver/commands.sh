#! /bin/bash

lspci | grep $1

echo "------------------------"

dmesg | grep $1 > file.txt 
tail -n 10 file.txt

echo "------------------------"

/sbin/lsmod

#If a driver is recognized by those commands 
#but not by lscpi or dmesg, it means the driver 
#is on the disk but not in the kernel. 
#In this case, load the module with 
#the modprobe command:
#$1 = module name
sudo modprobe $1

sudo make

mod=$1
mod+=".ko"

sudo insmod $mod

echo "------------------------"

dmesg | grep $1 > file.txt 
tail -n 10 file.txt

echo "------------------------"