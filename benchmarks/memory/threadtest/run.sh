#!/bin/bash


APPS="threadtest"
ALLOCATORS="glibc hoard tbbmalloc tcmalloc"

# Default parameters
# number of iterations
ITERATIONS="50000000"
# number of objects alllocated by all threads per iteration 
# should be the same number of threads for now (== 1 obj/thread)
ALLOCNUM="8"
# allocation sizes/8 (i.e., 2 == 16B, 8 = 64B, ..., 1024 == 8KiB 
ALLOCSIZE="2 8 16 32 64 256 1024"
THREADS="8"
REPEAT=30


#EXEC_FLAG_default=' 10000 100000 0 8'

TCMALLOCLIB="/home/baldas/artifact/allocators/libtcmalloc_minimal.so.4.1.2"
HOARDLIB="/home/baldas/artifact/allocators/libhoard.so.3.10"
TBBLIB="/home/baldas/artifact/allocators/libtbbmalloc_proxy.so.2:/home/baldas/artifact/allocators/libtbbmalloc.so.2"

LOGDIR="results"
TABDIR="tables"


function usage {
	
	echo $0' <command> [-a <applications>] [-m <mem-allocators>] [-l <num_allocs>] [-s <size>] [-t <threads>] [-n <repeat>] [-d <dfile_plot>]'

	echo
	echo "<command>:"
	echo "          exec - execute"
	echo "          gen  - generate data files (post-processing)"
	echo "          plot - plot graph (support is very limited)"

	echo
	echo "-a <applications>: application list"
        echo "default = $APPS"	
	
	echo
	echo "-m <mem-allocators>: memory allocators"
	echo "default = $ALLOCATORS"

	echo
	echo "-l <num_allocs>: number of (de)allocations per iteration (total)"
	echo "default = $ALLOCNUM"
	
	echo
	echo "-s <size>: size of each allocation (divided by 8)"
	echo "default = $ALLOCSIZE"

	echo
	echo "-t <threads>: list with number of threads to be used"
	echo "default = $THREADS"

	echo
	echo "-n <repeat>: number of times to repeat"
	echo "default = $REPEAT"

	echo
	echo "-d <dfile_plot>: name of the data file to plot"
}


function validate_input {

# if not set, use the default configs
	[ -z "$_APPS" ] && _APPS=$APPS
	[ -z "$_ALLOCATORS" ] && _ALLOCATORS=$ALLOCATORS
	[ -z "$_ALLOCNUM" ] && _ALLOCNUM=$ALLOCNUM
	[ -z "$_ALLOCSIZE" ] && _ALLOCSIZE=$ALLOCSIZE
	[ -z "$_THREADS" ] && _THREADS=$THREADS
	[ -z "$_REPEAT" ] && _REPEAT=$REPEAT

# check if applications exist
	for app in $_APPS; do
		if [ ! -f "$app" ]; then
			echo "Application -$app- does not exist"
			exit 1
		fi
	done

# check if the dynamic libraries for the allocators are valid
	for alloc in $_ALLOCATORS; do
	# valid pre-defined allocators
		if [ "$alloc" != "glibc" ] && [ "$alloc" != "tcmalloc" ] && [ "$alloc" != "hoard" ] && [ "$alloc" != "tbbmalloc" ]; then
			echo "Invalid allocator: $alloc"
			exit 1
		fi
	# check if library exists
		if [ $alloc == "tcmalloc" ] && [ ! -f $TCMALLOCLIB ]; then
			echo "File does not exist: $TCMALLOCLIB"
			exit 1
		fi	
		if [ $alloc == "hoard" ] && [ ! -f $HOARDLIB ]; then
			echo "File does not exist: $HOARDLIB"
			exit 1
		fi	
#		if [ $alloc == "tbbmalloc" ] && [ ! -f $TBBLIB ]; then
#			echo "File does not exist: $TBBLIB"
#			exit 1
#		fi	

	done

# check if number of threads is a valid numeric value
	for t in $_THREADS; do
		if ! [[ "$t" =~ ^[0-9]+$ ]] ; then
			echo "error: Not a number in -$_THREADS-" 
			exit 1
		fi
	done

# check if repeatition is a valid numeric value	
	if ! [[ "$_REPEAT" =~ ^[0-9]+$ ]] ; then
		echo "error: -$_REPEAT- is not a number" 
		exit 1
	fi

	echo "Using:"
	echo "  Apps      :   $_APPS"
	echo "  Allocators:   $_ALLOCATORS"
	echo "  Num allocs:   $_ALLOCNUM"
	echo "  Alloc size:   $_ALLOCSIZE"
	echo "  Threads   :   $_THREADS"
	echo "  Repeat    :   $_REPEAT"
}




function execute {

	[ ! -d "$LOGDIR" ] && mkdir $LOGDIR

	echo "Executing"
	FILENAME=`date +%R-%d%m%y`
	LOG_FILE="exec_log-$FILENAME"
	HOST=`hostname`
	DATE=`date`
	echo "Execution on $HOST at $DATE" > $LOG_FILE


	for app in $_APPS; do
		
		for alloc in $_ALLOCATORS; do

			#eval exec_flag=\$EXEC_FLAG_$work
			exec_flag=$EXEC_FLAG_default

			for numalloc in $_ALLOCNUM; do
				for size in $_ALLOCSIZE; do

					for t in $_THREADS; do
		
						for r in `seq 1 $_REPEAT`; do
		
							ALLOCATOR=""
							test "${alloc}" == "tcmalloc" && ALLOCATOR=$TCMALLOCLIB
							test "${alloc}" == "hoard" && ALLOCATOR=$HOARDLIB
							test "${alloc}" == "tbbmalloc" && ALLOCATOR=$TBBLIB
							_ITERATIONS=$ITERATIONS						
#  Parameters: <number-of-threads> <iterations> <num-objects> <work-interval> <object-size>
							echo "Executing [$alloc] $app $t $_ITERATIONS $numalloc 0 $size - turn $r"	
							echo "Executing [$alloc] $app $t $_ITERATIONS $numalloc 0 $size - turn $r" >> $LOG_FILE	
							LD_PRELOAD=${ALLOCATOR} ./$app $t $_ITERATIONS $numalloc 0 $size > $LOGDIR/${app}-${alloc}-${numalloc}-${size}-${t}-${r} || echo "last command failed" >> ${LOG_FILE}
						done
					done
				done
			done
		done
	done
}


function generate_data {

	[ ! -d "${TABDIR}" ] && mkdir ${TABDIR}

	rm -f ${TABDIR}/threadtest.dat

	# generate the header
	HEADER="t "
	for alloc in $_ALLOCATORS; do
		HEADER="$HEADER $alloc ci" 
	done

	for alloc in $_ALLOCATORS; do
		for numalloc in $_ALLOCNUM; do
			for t in $_THREADS; do
				rm -f ${TABDIR}/${app}-${alloc}-${numalloc}-${t}.dat
				COUNTER=1
				for size in $_ALLOCSIZE; do
					# note: this assumes 8 cores
					awk -v it=$ITERATIONS -v _size=$COUNTER -v _alloc=$alloc 'BEGIN{ mean=0.0; dp=0; NUM_VAL=0; interval=0;} 
					{ if ($2 == "elapsed") { mean += $4; val[NUM_VAL++]=$4 } } 
					  END { 
					     	mean=mean/NUM_VAL; 
					     	for(i=0;i<NUM_VAL;i++) 
					        	dp+=(mean-val[i])*(mean-val[i]); 
					     	dp=dp/(NUM_VAL-1);
					     	dp=sqrt(dp);
					     	interval=(1.96*(dp/sqrt(NUM_VAL)));
						if (_alloc == "glibc")
							printf("%d %.5f %.5f\n", _size, it*8.0/mean, interval/mean*(it*8.0/mean));
	                         	      	else
	                                     		printf("%.5f %.5f\n", it*8.0/mean, interval/mean*(it*8.0/mean));
					}' ${LOGDIR}/${app}-${alloc}-${numalloc}-${size}-${t}-* >> ${TABDIR}/${app}-${alloc}-${numalloc}-${t}.dat
					let COUNTER=$COUNTER+1
				done
			done
		done	
		for numalloc in $_ALLOCNUM; do
			for t in $_THREADS; do
				paste -d' ' ${TABDIR}/${app}-*-${numalloc}-${t}.dat > "${TABDIR}/temp.dat"
				echo "$HEADER" > "${TABDIR}/${app}-${numalloc}-${t}-table.dat"
				cat "${TABDIR}/temp.dat" >> "${TABDIR}/${app}-${numalloc}-${t}-table.dat"
				rm -rf "${TABDIR}/temp.dat"
			done
		done
	done
}



function plot_graph {
	
	if [ ! -f "$1" ]; then
		echo "FIle '$1' not found - unable to plot"
		exit 1
	fi

	TITLE="Threadtest"
	_GRAPHFILE="line-threadtest.gnu"

	OUTPUTFILE="threadtest"
	TOTAL_COL="8"
	export TITLE=$TITLE INPUT=$1 OUTPUT="$OUTPUTFILE" COLS=$TOTAL_COL

	echo "   plotting ..."	
	gnuplot $_GRAPHFILE

}




run_opt=$1
shift
while getopts "a:m:l:s:t:n:d:" opt;
do
	case $opt in
		a) _APPS=$OPTARG ;;
		m) _ALLOCATORS=$OPTARG ;;
		l) _ALLOCNUM=$OPTARG ;;
		s) _ALLOCSIZE=$OPTARG ;;
		t) _THREADS=$OPTARG ;;
		n) _REPEAT=$OPTARG ;;
		d) _DATAFILE=$OPTARG ;;
		\?) usage
			exit 1
	esac
done

case $run_opt in
	exec)
		validate_input
		execute
		;;
	#seq)
		#validate_input
		#comp_exec_seq
		#;;
	gen)
		validate_input
		#validate_data
		generate_data
		;;
	plot)
		validate_input
		plot_graph ${_DATAFILE}
		;;
	*)
		echo $0" : error - invalid parameter - $run_opt"
		usage
		;;
esac

