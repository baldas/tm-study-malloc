#!/bin/bash

# compile microbenchmarks with default settings
./compile.sh -a "intset-ll intset-hs intset-rb"

./compile.sh -a "intset-ll intset-hs intset-rb" -p

# compile linked-list with shift amount of 4 (default = 5)
./compile.sh -a intset-ll -f suicidee1

# compile STAMP applications
./compile.sh

# compile sequential STAMP
./compile.sh -l seq

# compile sequential STAMP with instrumentation to collect allocation stats
# (as per Table 5 in the paper)
./compile.sh -l seq -i

# compile STAMP with caching optimization
./compile.sh -f suicidecache
