#!/bin/bash

cd out && cmake ../

if [ $# -gt 0 ] && [ "$1" = "clean" ] ; then
    echo "cleaning..."
    make clean
else
    echo "make..."
    make
fi