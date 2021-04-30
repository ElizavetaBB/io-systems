#!/bin/bash

sudo umount /mnt/mydisk1
sudo umount /mnt/mydisk5
sudo umount /mnt/mydisk6
sudo umount /mnt/mydisk7

function format_and_mount(){
    echo "Format vfat:${1}"
    sudo mkfs.vfat /dev/$1
    echo "Mount:/mnt/${1}"
    sudo mount /dev/$1 /mnt/$1
}

function fill_random(){
    sudo dd if=/dev/urandom of=/mnt/mydisk1/tfile bs=1M count=7
    sudo dd if=/dev/urandom of=/mnt/mydisk5/tfile bs=1M count=7
    sudo dd if=/dev/urandom of=/mnt/mydisk6/tfile bs=1M count=7
    sudo dd if=/dev/urandom of=/mnt/mydisk7/tfile bs=1M count=7
}

function copy_between_virt(){
    echo "Copy files between ${1} and ${2}:"
    sudo dd if=/mnt/$1/tfile of=/mnt/$2/tfile bs=1M count=7
}

function copy_virt_real(){
    echo "Copy files between virtual ${1} and real /dev/testfile"  
    sudo dd if=/mnt/$1/tfile of=/mnt/testfile bs=1M count=7
}

function delete_files(){
    rm /mnt/mydisk1/file
    rm /mnt/mydisk5/file
    rm /mnt/mydisk6/file
    rm /mnt/mydisk7/file
}

format_and_mount mydisk1
format_and_mount mydisk5
format_and_mount mydisk6
format_and_mount mydisk7

fill_random

copy_between_virt mydisk1 mydisk5
copy_between_virt mydisk5 mydisk6
copy_between_virt mydisk6 mydisk7

copy_virt_real mydisk1
copy_virt_real mydisk5
copy_virt_real mydisk6
copy_virt_real mydisk7




