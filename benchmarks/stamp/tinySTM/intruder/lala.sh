#!/bin/bash
LD_PRELOAD=/home/baldas/artifact/allocators/libhoard.so.3.10 ./intruder-tinySTM-suicide -a10 -l128 -n262144 -s1 -t8
