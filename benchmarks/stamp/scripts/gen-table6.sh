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
OUTPUTFILE="${OUTPUTDIR}/table6.txt"

DIR="${TABDIR}/stamp-tinySTM"

function bestworst {

echo "=== $1 ===" >> $OUTPUTFILE
cat $2 | awk 'BEGIN { best = 100000; newbest=0; worstlocal=0; colbest=-1; colworst=-1; colworstlocal=-1; core="-"; \
	                 aloc[2] = "glibc"; aloc[14] = "hoard"; aloc[26] = "tbbmalloc"; aloc[38] = "tcmalloc"} \
	{ if ($1 != "t") { \
		worst = -1;
		for (i=2; i<50; i+=12) { \
			if ($i < best) {best=$i; colbest=i; core=$1; newbest=1} \
			if ($i > worst) {worst=$i; colworst=i} \
		} \
		if (newbest == 1) { worstlocal=worst; colworstlocal=colworst; newbest=0; }
	  } \
	}
 	END { \
	  wl = sprintf("%2.2f", worstlocal); b = sprintf("%2.2f", best); \
	  printf("core %d max = %s (%2.2f) min = %s (%2.2f) [%2.3f%]\n", \
	  core, aloc[colbest], b, aloc[colworstlocal], wl, ((wl/b)-1)*100); \
  	}' >> $OUTPUTFILE

}

APPS="bayes genome intruder labyrinth vacation yada"

echo "" > $OUTPUTFILE
for app in $APPS; do
	bestworst $app "${DIR}/${app}-suicide.table"
done

