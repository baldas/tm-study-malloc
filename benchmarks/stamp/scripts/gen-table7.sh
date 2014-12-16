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
OUTPUTFILE="${OUTPUTDIR}/table7.txt"

DIR="${TABDIR}/stamp-tinySTM/"


APPS="genome intruder vacation yada"
# first generate the data table ...
for app in $APPS; do
	echo "Generating for $app"
	./gen-data.sh -n "${app}-suicidecache" -d stamp-tinySTM  -c "$app:tinySTM:suicidecache:glibc $app:tinySTM:suicidecache:hoard 
            $app:tinySTM:suicidecache:tbbmalloc $app:tinySTM:suicidecache:tcmalloc" -t 8
done

# then compile the results
echo "App   Glibc   Hoard   TBBMalloc   TCMalloc" > $OUTPUTFILE
for app in $APPS; do
	NORMGLIBC=`cat ${DIR}/${app}-suicide.table | awk '{ if ($1 == 8) printf("%2.2f", $2)}'`
	NORMHOARD=`cat ${DIR}/${app}-suicide.table | awk '{ if ($1 == 8) printf("%2.2f", $14)}'`
	NORMTBBMALLOC=`cat ${DIR}/${app}-suicide.table | awk '{ if ($1 == 8) printf("%2.2f", $26)}'`
	NORMTCMALLOC=`cat ${DIR}/${app}-suicide.table | awk '{ if ($1 == 8) printf("%2.2f", $38)}'`

	echo $app >> $OUTPUTFILE
	cat "${DIR}/${app}-suicidecache.table" | awk -v nglibc=$NORMGLIBC -v nhoard=$NORMHOARD -v ntbb=$NORMTBBMALLOC -v ntc=$NORMTCMALLOC \
		'{ if ( $1 == "8"){ gl=sprintf("%2.2f", $2); ho=sprintf("%2.2f", $14); \
		    tbb=sprintf("%2.2f", $26); tc=sprintf("%2.2f", $38); \
		    printf("%2.2f %2.2f %2.2f %2.2f\n", ((nglibc/gl)-1)*100, ((nhoard/ho)-1)*100, \
		    ((ntbb/tbb)-1)*100, ((ntc/tc)-1)*100); }}' >> $OUTPUTFILE 
done
