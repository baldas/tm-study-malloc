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
OUTPUTFILE="${OUTPUTDIR}/table3.txt"

LLFILE="${TABDIR}/intset-tinySTM/intset-ll-rw-suicide.table"
HSFILE="${TABDIR}/intset-tinySTM/intset-hs-rw-suicide.table"
RBFILE="${TABDIR}/intset-tinySTM/intset-rb-rw-suicide.table"


function bestworst {

echo "=== $1 ===" >> $OUTPUTFILE
cat $2 | awk 'BEGIN { max = -1; newmax=0; minlocal=0; colmax=-1; colmin=-1; colminlocal=-1; core="-"; \
	                 aloc[2] = "glibc"; aloc[14] = "hoard"; aloc[26] = "tbbmalloc"; aloc[38] = "tcmalloc"} \
	{ if ($1 != "t") { \
		min = 100000000;
		for (i=2; i<50; i+=12) { \
			if ($i > max) {max=$i; colmax=i; core=$1; newmax=1} \
			if ($i < min) {min=$i; colmin=i} \
		} \
		if (newmax == 1) { minlocal=min; colminlocal=colmin; newmax=0; }
	  } \
	}
 	END { \
	  ml = sprintf("%2.2f", minlocal); m = sprintf("%2.2f", max); # force rounding \
	  printf("core %d max = %s (%2.2f) min = %s (%2.2f) [%2.3f%]\n", \
	  core, aloc[colmax], m, aloc[colminlocal], ml, ((m/ml)-1)*100); \
  	}' >> $OUTPUTFILE

}

[ ! -d "${TABDIR}" ] && mkdir ${TABDIR}
[ ! -d "${OUTPUTDIR}" ] && mkdir ${OUTPUTDIR}

echo "" > $OUTPUTFILE
bestworst "LinkedList" $LLFILE
bestworst "HashSet" $HSFILE
bestworst "RBTree" $RBFILE
