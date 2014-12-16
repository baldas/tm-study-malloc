#!/bin/bash

source scripts.cfg



function usage {
	
	echo $0' [-a <applications>] [-f <flavors>] [-l <stm-lib>] [-m <mem-allocators>] [-t <threads>] [-n <repeat>]'

	echo
	echo "-a <applications>: application list"
        echo "use '.workload' to specify a specific workload"
	echo "Example: intset-ll.read"
	echo "default = $APPS"	
	
	echo
	echo "-f <flavors>: library variation"
	echo "default = $FLAVORS"

	echo
	echo "-l <stm-lib>: stm libraries"
	echo "   use 'seq' for sequential version"
	echo "   use 'glock' for global lock"
	echo "   default = $STMLIBS"

	echo
	echo "-m <mem-allocators>: memory allocators"
	echo "default = $ALLOCATORS"

	echo
	echo "-t <threads>: list with number of threads to be used"
	echo "default = $THREADS"

	echo
	echo "-n <repeat>: number of times to repeat"
	echo "default = $REPEAT"
}

function show_using {

	echo "Using:" $1
	echo "  apps:   $_APPS" $1
	echo "  flvs:   $_FLAVORS" $1
	echo "  libs:   $_STMLIBS" $1
	echo "  alls:   $_ALLOCATORS" $1
	echo "  thrs:   $_THREADS" $1
	echo "  reps:   $_REPEAT" $1
}

#
# Validate the input parameters and set them accordingly.
#
function validate_input {

# if not set, use the default configs
	[ -z "$_APPS" ] && _APPS=$APPS
	[ -z "$_FLAVORS" ] && _FLAVORS=$FLAVORS
	[ -z "$_STMLIBS" ] && _STMLIBS=$STMLIBS
	[ -z "$_ALLOCATORS" ] && _ALLOCATORS=$ALLOCATORS
	[ -z "$_THREADS" ] && _THREADS=$THREADS
	[ -z "$_REPEAT" ] && _REPEAT=$REPEAT


# TODO: check if workloads exist	

	for app in $_APPS; do

		IFS="."
		tokens=( $app )
		appname="${tokens[0]}"
		workload="${tokens[1]}"
		
		IFS="-"
		tokens=( $appname )
		unset IFS
		
		appname="${tokens[0]}"
				
		IFS="_"
		tokens=( $appname )
		unset IFS
		
		appprefix="${tokens[0]}"

		eval exec_flag=\$EXEC_FLAG_${appprefix}${workload}
		[ -z "${exec_flag}" ] && echo "warning: empty workload: EXEC_FLAG_${appprefix}${workload}" && exit

	done

# check if applications exist
	for stm in $_STMLIBS; do
		if [ $stm == "seq" ]; then
			for app in $_APPS; do
				IFS="."
				tokens=( $app )
				unset IFS
				appname="${tokens[0]}"
				IFS="_"
				tokens=( $appname )
				unset IFS
				appname="${appname}_seq"
				appdirname="${tokens[0]}"
				if [ ! -f "${BINDIR}/$stm/$appdirname/$appname" ]; then
					echo "Binary file -${BINDIR}/$stm/$appdirname/$appname- not found"
					exit 1
				fi
			done
		else
			for flv in $_FLAVORS; do
				for app in $_APPS; do
					IFS="."
					tokens=( $app )
					unset IFS
					appname="${tokens[0]}"
					IFS="_"
					tokens=( $appname )
					unset IFS
					appdirname="${tokens[0]}"
					if [ ! -f "${BINDIR}/$stm/$appdirname/$appname-$stm-$flv" ]; then
						echo "Binary file -${BINDIR}/$stm/$appdirname/$appname-$stm-$flv- not found"
						exit 1
					fi
				done
			done
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
	# TODO: Check for TBB files	
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

}

# if executing profile-based apps (.prealloc) we need to make sure the .prof files exist
function check_profile {

	for stm in $_STMLIBS; do
		[ $stm == "seq" ] && continue
		for flv in $_FLAVORS; do
			IS_PREALLOC=`echo $flv | sed 's/.*\.prealloc//'`
			[ ! -z "${IS_PREALLOC}" ] && continue
			for app in $_APPS; do
				for alloc in $_ALLOCATORS; do
					for t in $_THREADS; do
						
						# TODO: split 'app' into appname and workload

						FLVREM=`echo $flv | sed 's/\.prealloc//g'`
						PROF_NAME=${app}.${stm}.${FLVREM}.${alloc}.${t}.prof
							
						if [ ! -f "${PROFDIR}/${PROF_NAME}" ]; then
							echo "Profile file -${PROFDIR}/${PROF_NAME}- not found"
							exit 1
						fi
					done
				done
			done
		done
	done

}


function exec_seq {
			

    TARGETDIR="$RESULTDIR/seq"
	  [ ! -d "${TARGETDIR}" ] && mkdir ${TARGETDIR}
  	for app in $_APPS; do

		IFS="."
		tokens=( $app )
		appname="${tokens[0]}"
		workload="${tokens[1]}"
		
		IFS="_"
		tokens=( $appname )
		appdirname="${tokens[0]}"
		
		IFS="-"
		tokens=( $appdirname )
		unset IFS
	
		appname="${appname}_seq"

		appprefix="${tokens[0]}"

		FILENAME=`date +%Y%m%d-%H%M%S`
		LOG_FILE="${TARGETDIR}/seq-${app}-${FILENAME}"
		LOG_FILE_MD5=${LOG_FILE}.md5
		HOST=`hostname`
		DATE=`date`
		echo "Execution on $HOST at $DATE" > ${LOG_FILE}
		echo "App:    $app" >> ${LOG_FILE} 
		echo "Allocs: $_ALLOCATORS" >> ${LOG_FILE}
		echo "Repeat: $_REPEAT" >> ${LOG_FILE}
		echo "=======================================" >> ${GLOBAL_LOG_FILE}
		cat ${LOG_FILE} >> ${GLOBAL_LOG_FILE}
		touch ${LOG_FILE_MD5}



		TARGETAPPDIR="$TARGETDIR/$app"
		[ ! -d "${TARGETAPPDIR}" ] && mkdir ${TARGETAPPDIR}

		md5sum ${BINDIR}/seq/$appdirname/$appname >> ${LOG_FILE_MD5}
		cd ${BINDIR}/seq/$appdirname > /dev/null
		for alloc in $_ALLOCATORS; do


			eval exec_flag=\$EXEC_FLAG_${appprefix}${workload}

			#eval exec_flag=\$EXEC_FLAG_$app
			for r in `seq 1 $_REPEAT`; do

				ALLOCATOR=""
				test "${alloc}" == "tcmalloc" && ALLOCATOR=$TCMALLOCLIB
				test "${alloc}" == "hoard" && ALLOCATOR=$HOARDLIB
				test "${alloc}" == "tbbmalloc" && ALLOCATOR=$TBBLIB
							
				echo "Executing [$alloc] $appname ${exec_flag}1 - turn $r" | tee -a ${LOG_FILE}	${GLOBAL_LOG_FILE}
				LD_PRELOAD=${ALLOCATOR} ./$appname ${exec_flag}1 > $TARGETAPPDIR/${app}-seq-${alloc}-${r} 
	
        			if [ -f "memprofile.dat" ]; then  # TODO: des-hardcode the file name
        			  # if profile file was generated, move it to the target directory
       				   mv "memprofile.dat" ${TARGETAPPDIR}
        			fi

			done
		done
		cd - > /dev/null
		DATE=`date`
		echo "Execution finished at $DATE" >> ${LOG_FILE}
		echo "Execution finished at $DATE" >> ${GLOBAL_LOG_FILE}
		echo "=======================================" >> ${GLOBAL_LOG_FILE}
	done
}


function execute {

	
	[ ! -d "${RESULTDIR}" ] && mkdir ${RESULTDIR}


	FILENAME=`date +%Y%m%d-%H%M%S`
	GLOBAL_LOG_FILE="${RESULTDIR}/log-${FILENAME}"
	HOST=`hostname`
	DATE=`date`
	show_using >> ${GLOBAL_LOG_FILE}

	for stm in $_STMLIBS; do
		[ $stm == "seq" ] && exec_seq && continue
		for flv in $_FLAVORS; do

        		TARGETDIR=$RESULTDIR/$stm-$flv
			[ ! -d "${TARGETDIR}" ] && mkdir ${TARGETDIR}
			for app in $_APPS; do

				IFS="."
				tokens=( $app )
	 			appname="${tokens[0]}"
				workload="${tokens[1]}"
		
				IFS="_"
				tokens=( $appname )
				appdirname="${tokens[0]}"
				
				IFS="-"
				tokens=( $appdirname )
				unset IFS
				
				appprefix="${tokens[0]}"


				FILENAME=`date +%Y%m%d-%H%M%S`
				LOG_FILE="${TARGETDIR}/$app-$stm-$flv-${FILENAME}"
				LOG_FILE_MD5=${LOG_FILE}.md5
				HOST=`hostname`
				DATE=`date`
				echo "Execution on $HOST at $DATE" > ${LOG_FILE}
				echo "App:    $app" >> ${LOG_FILE} 
				echo "Allocs: $_ALLOCATORS" >> ${LOG_FILE}
				echo "Thread: $_THREADS" >> ${LOG_FILE}
				echo "Repeat: $_REPEAT" >> ${LOG_FILE}
				echo "=======================================" >> ${GLOBAL_LOG_FILE}
				cat ${LOG_FILE} >> ${GLOBAL_LOG_FILE}
				touch ${LOG_FILE_MD5}



				TARGETAPPDIR="$TARGETDIR/$app"
				[ ! -d "${TARGETAPPDIR}" ] && mkdir ${TARGETAPPDIR}


				md5sum ${BINDIR}/$stm/$appdirname/$appname-$stm-$flv >> ${LOG_FILE_MD5}
				for alloc in $_ALLOCATORS; do
					cd ${BINDIR}/$stm/$appdirname > /dev/null
					#eval exec_flag=\$EXEC_FLAG_$app
					eval exec_flag=\$EXEC_FLAG_${appprefix}${workload}
				
					for t in $_THREADS; do
						
						FLVREM=`echo $flv | sed 's/\.prealloc//g'`
						PROF_NAME=${app}.${stm}.${FLVREM}.${alloc}.${t}.prof
		
						for r in `seq 1 $_REPEAT`; do
		
							ALLOCATOR=""
							test "${alloc}" == "tcmalloc" && ALLOCATOR=$TCMALLOCLIB
							test "${alloc}" == "hoard" && ALLOCATOR=$HOARDLIB
							test "${alloc}" == "tbbmalloc" && ALLOCATOR=$TBBLIB
							
							echo "Executing [$alloc] $appname-$stm-$flv ${exec_flag}$t - turn $r" | tee -a ${LOG_FILE} ${GLOBAL_LOG_FILE}
						 		
							PROF_FILE=${PROFDIR}/${PROF_NAME} LD_PRELOAD=${ALLOCATOR} ./$appname-$stm-$flv ${exec_flag}$t > ${TARGETAPPDIR}/${app}-${stm}-${flv}-${alloc}-${t}-${r} 2>&1 || echo "last command failed" >> ${LOG_FILE}
					
						done
					done

					cd - > /dev/null
				done

				DATE=`date`
				echo "Execution finished at $DATE" >> ${LOG_FILE}
				echo "Execution finished at $DATE" >> ${GLOBAL_LOG_FILE}
				echo "=======================================" >> ${GLOBAL_LOG_FILE}
			done
		done
	done
				
	echo "=======================================" >> ${GLOBAL_LOG_FILE}
	echo "Execution finished at $DATE" >> ${GLOBAL_LOG_FILE}
}



while getopts "a:f:l:m:t:n:h" opt;
do
	case $opt in
		a) _APPS=$OPTARG ;;
		f) _FLAVORS=$OPTARG ;;
		l) _STMLIBS=$OPTARG ;;
		m) _ALLOCATORS=$OPTARG ;;
		t) _THREADS=$OPTARG ;;
		n) _REPEAT=$OPTARG ;;
		h) usage; exit 1 ;;
		\?) usage
			exit 1
	esac
done

validate_input
show_using >&1
check_profile
execute

