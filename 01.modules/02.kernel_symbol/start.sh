#!/bin/bash

make MODULE=symbol

sudo insmod symbol.ko

make MODULE=helloworld

sudo insmod helloworld.ko

sudo rmmod helloworld

sudo rmmod symbol

make clean
