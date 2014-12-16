#!/bin/bash

source scripts.cfg


#
# if an application has the symbol '-', a specific Makefile will be used.
# Example: -a intset-ll     -> make -f Makefile.ll (directory intset)
#

function usage {
	
	echo $0' [-a <applications>] [-f <flavors>] [-l <stm-lib>] [-p] [-w] [-i]'

	echo
	echo "-a <applications>: application list"
 	echo " if the symbol '-' is present, a specific Makefile will be used."
	echo " Example: -a intset-ll    ->  make -f Makefile.ll (directory intset)"
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
	echo "-p:  turn on PAPI to instrument the code"

	echo
	echo "-w:  turn on thread affinity"
	
  echo
  echo "-i:  instrument the code to measure malloc count (seq only)"
  echo

}

function show_using {

	echo "Using:" $1
	echo "  apps:   $_APPS" $1
	echo "  flvs:   $_FLAVORS" $1
	echo "  libs:   $_STMLIBS" $1
  echo "  papi:   $_USEPAPI" $1
  echo "  taff:   $_USETAFF" $1
  echo "  malc:   $_USEMC" $1
}


function validate_input {

# if not set, use the default configs
	[ -z "$_APPS" ] && _APPS=$APPS
	[ -z "$_FLAVORS" ] && _FLAVORS=$FLAVORS
	[ -z "$_STMLIBS" ] && _STMLIBS=$STMLIBS
	[ -z "$_USEPAPI" ] && _USEPAPI="no"
	[ -z "$_USETAFF" ] && _USETAFF="no"
	[ -z "$_USEMC" ] && _USEMC="no"

# check if applications exist
	for app in $_APPS; do

		IFS="-"
		tokens=( $app )
		unset IFS
	 	app="${tokens[0]}"
		
		# TODO: check if the Makefiles exist (?)

		if [ ! -d "$STAMP_DIR/$app" ]; then
			echo "Application -$app- does not exist"
			exit 1
		fi
	done

#check if required STM libs exist
# by default, we assume there is a directory with the same name specified in _STMLIBS
	for stm in $_STMLIBS; do
		if [ ! $stm == "seq" ]; then
		     	if [ ! $stm == "glock" ]; then
				if [ ! -d "${TMDIR}/$stm" ]; then
					echo "Library -${TMDIR}/$stm- does not exist"
					exit 1
				fi
			fi
		fi
	done

# check if required files to build the STM flavors exist
	for stm in $_STMLIBS; do
		if [ ! $stm == "seq" ]; then
		     	if [ ! $stm == "glock" ]; then
				for flavor in $_FLAVORS; do
					if [ ! -f "${TMDIR}/$stm/Makefile.$flavor" ]; then
						echo "Configuration file -${TMDIR}/$stm/Makefile.$flavor- not found"
						exit 1
					fi
				done
			fi
		fi
	done

}


function compile_seq_apps {
	
	# build sequential versions
	echo "Compiling sequential version of the applications ..."
	for app in $_APPS; do
		IFS="-"
		tokens=( $app )
		unset IFS
		# if there is no '-', then the directory equals the name
		appdir="${tokens[0]}"
		extension="${tokens[1]}"

		cd $STAMP_DIR/$appdir > /dev/null

		[ ! -z "$extension" ] && extension=".$extension"
					
		echo "   Compiling $app"
		make -s -f Makefile${extension} clean 
		make -s -f Makefile${extension} PAPI=${_USEPAPI} AFFINITY=${_USETAFF} MALLOCPROFILE=${_USEMC} 
		if [[ $? -ne 0 ]]; then
			cd -
			echo "Error while generating sequential $app"
			exit 1
		fi
		cd - > /dev/null

    		SUFFIX="_seq"
		if [ "$_USETAFF" == "yes" ]; then
      			SUFFIX="_taff${SUFFIX}"
    		fi
		if [ "$_USEPAPI" == "yes" ]; then
      			SUFFIX="_papi${SUFFIX}"
    		fi
    		if [ "$_USEMC" == "yes" ]; then
      			SUFFIX="_mc${SUFFIX}"
    		fi

		mv ${BINDIR}/seq/$app/$app ${BINDIR}/seq/$app/${app}${SUFFIX}
	done
}

function compile_glock_apps {
	
	# build global lock versions
	echo "Compiling global lock version of the applications ..."
	for app in $_APPS; do
		IFS="-"
		tokens=( $app )
		unset IFS
		# if there is no '-', then the directory equals the name
		appdir="${tokens[0]}"
		extension="${tokens[1]}"

		cd $STAMP_DIR/$appdir > /dev/null

		[ ! -z "$extension" ] && extension=".$extension"
					
		echo "   Compiling $app"
		make -s -f Makefile${extension} TMBUILD=glock clean 
		make -s -f Makefile${extension} TMBUILD=glock PAPI=${_USEPAPI} AFFINITY=${_USETAFF} 
		if [[ $? -ne 0 ]]; then
			cd -
			echo "Error while generating glock $app"
			exit 1
		fi
		cd - > /dev/null

    		SUFIX="_glock"
		if [ "$_USETAFF" == "yes" ]; then
      			SUFIX="${SUFIX}_taff"
    		fi
		if [ "$_USEPAPI" == "yes" ]; then
      			SUFIX="${SUFIX}_papi"
    		fi
    		if [ "$_USEMC" == "yes" ]; then
      			SUFIX="${SUFIX}_mc"
    		fi

		mv ${BINDIR}/glock/$app/$app ${BINDIR}/glock/$app/${app}${SUFIX}
	done
}


function compile_stm_apps {


	for flv in $_FLAVORS; do
		stm=$1
		echo "Generating $stm with $flv"
		cd ${TMDIR}/$stm > /dev/null
		make -s -f Makefile.${flv} clean 
   		make -s -f Makefile.${flv}
		if [[ $? -ne 0 ]]; then
			cd -
			echo "Error while generating $stm with $flv"
			exit 1
		fi
   		cd - > /dev/null
		
		for app in $_APPS; do
			IFS="-"
			tokens=( $app )
			unset IFS
			appdir="${tokens[0]}"
			extension="${tokens[1]}"

			[ ! -z "$extension" ] && extension=".$extension"

			echo "   Compiling $app"
			cd $STAMP_DIR/$appdir > /dev/null
			make -s -f Makefile${extension} TMBUILD=$stm clean 
			make -s -f Makefile${extension} TMBUILD=$stm TMLIBDIR=${TMDIR}/$stm PAPI=${_USEPAPI} AFFINITY=${_USETAFF}  
			if [[ $? -ne 0 ]]; then
				cd -
				echo "Error while generating $stm with $flv"
				exit 1
			fi
			cd - > /dev/null
			# Note that the target dir has to use the complete name for the directory
			if [ "$_USEPAPI" == "yes" ]; then
				if [ "$_USETAFF" == "yes" ]; then
					mv ${BINDIR}/$stm/$app/$app ${BINDIR}/$stm/$app/${app}_taff_papi-$stm-$flv
				else
					mv ${BINDIR}/$stm/$app/$app ${BINDIR}/$stm/$app/${app}_papi-$stm-$flv
				fi
			elif [ "$_USETAFF" == "yes" ]; then
				mv ${BINDIR}/$stm/$app/$app ${BINDIR}/$stm/$app/${app}_taff-$stm-$flv
			else
				mv ${BINDIR}/$stm/$app/$app ${BINDIR}/$stm/$app/$app-$stm-$flv
			fi
		done
	done

}


function compile {

	echo "Compiling"

	for stm in $_STMLIBS; do
		if [ $stm == "seq" ]; then
			compile_seq_apps
		elif [ $stm == "glock" ]; then
			compile_glock_apps
		else
			compile_stm_apps $stm
		fi
	done
}



while getopts "a:f:l:m:t:n:h:pwi" opt;
do
	case $opt in
		a) _APPS=$OPTARG ;;
		f) _FLAVORS=$OPTARG ;;
		l) _STMLIBS=$OPTARG ;;
		p) _USEPAPI="yes" ;;
	  w) _USETAFF="yes" ;;
    i) _USEMC="yes" ;;
		h) usage; exit 1 ;;
		\?) usage
			exit 1
	esac
done


validate_input
show_using >&1
compile
