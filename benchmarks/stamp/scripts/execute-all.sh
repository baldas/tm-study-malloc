#!/bin/bash

# execute microbenchmarks with papi
./execute.sh -a "intset-ll_papi.rw intset-hs_papi.rw intset-rb_papi.rw" -t "1 2 4 6 8" -n 5

# execute microbenchmarks with default settings
./execute.sh -a "intset-ll.rw intset-hs.rw intset-rb.rw" -t "1 2 4 6 8" -n 5

# execute intset-ll with shift amount of 4 (default = 5)
./execute.sh -a "intset-ll.rw" -f suicidee1 -t "1 2 4 6 8" -n 5

# execute STAMP applications
./execute.sh -a "bayes genome intruder labyrinth vacation yada" -n 5

# execute sequential STAMP
./execute.sh -a "genome yada" -l seq -n 5

# execute sequential once to get allocation stats (doesn't matter the allocater used)
./execute.sh -m glibc -a "bayes_mc genome_mc intruder_mc kmeans_mc labyrinth_mc ssca2_mc vacation_mc yada_mc" -l seq -n 1

# execute STAMP with caching optimization
./execute.sh -a "genome intruder vacation yada" -f suicidecache -t 8 -n 5
