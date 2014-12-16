#!/bin/bash

source scripts.cfg

# General format of the data files (generate through gen-data.sh)
#
# 10 sec
# column 12*k+2 - tx/sec
# column 12*k+4 - #aborts
# column 12*k+6 - PAPI1 - (total of L1d misses/total of L1d accesses)
# column 12*k+8 - PAPI2 - 
# column 12*k+10 - PAPI3 - (total insn)
# column 12*k+12 - PAPI4 - (total cycles)
# where k=0 for glibc, k=1 for hoard, k=2 for tbbmalloc, and k=3 for tcmalloc


_OUTDIR="paper"
	
OUTPUTDIR="${TABDIR}/${_OUTDIR}"
OUTPUTFILE="${OUTPUTDIR}/table4.txt"

#
# In the paper the abort rate comes from the 'normal' execution file,
# while  the miss rate from the PAPI one
#
FILECM="${TABDIR}/intset-tinySTM/intset-ll_papi-rw-suicide.table"
FILEAB="${TABDIR}/intset-tinySTM/intset-ll-rw-suicide.table"

# generate table with PAPI instrumentation
app="intset-ll_papi.rw"
./gen-data.sh -n "intset-ll_papi-rw-suicide" -d intset-tinySTM -c "$app:tinySTM:suicide:glibc $app:tinySTM:suicide:hoard 
            $app:tinySTM:suicide:tbbmalloc $app:tinySTM:suicide:tcmalloc" -t "1 2 4 6 8"

# get abort rate
echo "Abort rate" > $OUTPUTFILE
echo "Cores   Glibc  |  Hoard  |  TBBMalloc  |  TCMalloc" >> $OUTPUTFILE
cat $FILEAB | awk '{ if ( $1 != "t") 
            printf("%s | %2.2f | %2.2f | %2.2f | %2.2f\n",
	    $1, ($4/($4+$2*10))*100, \
   	    ($16/($16+$14*10))*100, \
	    ($28/($28+$26*10))*100, \
	    ($40/($40+$38*10))*100)}' >> $OUTPUTFILE

# get cache misses
echo "Cache misse ratio" >> $OUTPUTFILE
echo "Cores   Glibc  |  Hoard  |  TBBMalloc  |  TCMalloc" >> $OUTPUTFILE
cat $FILECM | awk '{ if ( $1 != "t") 
            printf("%s | %2.2f | %2.2f | %2.2f | %2.2f\n",
	    $1, ($8/$6)*100, ($20/$18)*100,  \
		($32/$30)*100, ($44/$42)*100)}' >> $OUTPUTFILE

# The script below gets both abort and cache miss ratio from the PAPI file 
# and should be preferred for future measurements.
#
# echo "Cores   Glibc  |  Hoard  |  TBBMalloc  |  TCMalloc" > $OUTPUTFILE
#cat $FILE | awk '{ if ( $1 != "t") 
#           printf("%s %2.1f %2.1f | %2.1f %2.1f | %2.1f %2.1f | %2.1f %2.1f\n",
#	    $1, ($4/($4+$2*10))*100, ($8/$6)*100,  \
#  	    ($16/($16+$14*10))*100, ($20/$18)*100, \
#	    ($28/($28+$26*10))*100, ($32/$30)*100, \
#	    ($40/($40+$38*10))*100, ($44/$42)*100)}' >> $OUTPUTFILE
