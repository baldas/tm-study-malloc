#!/bin/bash

source scripts.cfg

SUFFIX="_mc"

function usage {

	echo $0' -d <output-dir> [-a <apps>]'

	echo
	echo "-d <output-dir>: subdirectory to store the generated files"
	
  	echo
	echo "-a <apps>: application list"
	echo "default = $APPS"	

}

function show_using {

  	echo "Using:" $1
  	echo " dir : $_OUTDIR" $1
  	echo " apps: $_APPS" $1
}


function validate_input {

  	_THREADS="1"
  	_GRAPHFILE="memprofile.gnu"
	[ -z "$_APPS" ] && _APPS=$APPS


  	if [ -z "$_OUTDIR" ]; then
    		echo "Output directory required"
    		usage
    		exit 1
  	fi

	for app in $_APPS; do

		dirname="${app}${SUFFIX}"
		appname="${app}${SUFFIX}"
			
    		FILENAME="${RESULTDIR}/seq/$dirname/${appname}-seq-glibc-1"

		if [ ! -f "${FILENAME}" ]; then
			echo "File ${FILENAME} not found"
			exit 1
		fi
    
 #   GNUFILE="${RESULTDIR}/seq/$appname/memprofile.dat"
 # 		if [ ! -f "${GNUFILE}" ]; then
 #					echo "File ${GNUFILE} not found"
 #					exit 1
 #		fi

	done
	
	#if [ ! -f $_GRAPHFILE ]; then
	#	echo "Graph file not found"
	#	exit 1
	#fi
	
  show_using >&1

}


function generate_graph {
	
  	[ ! -d "${TABDIR}" ] && mkdir ${TABDIR}
	OUTPUTDIR="${TABDIR}/${_OUTDIR}"
	[ ! -d "${OUTPUTDIR}" ] && mkdir ${OUTPUTDIR}

	for app in $_APPS; do
		
   		dirname="${app}${SUFFIX}"
		appname="${app}${SUFFIX}"

    		INPUTFILE="${RESULTDIR}/seq/$dirname/${appname}-seq-glibc-1"
    		OUTPUTFILE="${OUTPUTDIR}/${app}"

    		BEGINPAR=`cat $INPUTFILE | awk '{ if (($1 == "BeginRegion") && $2 == "1:") print $3}'`
    		ENDPAR=`cat $INPUTFILE | awk '{ if (($1 == "BeginRegion") && $2 == "2:") print $3}'`

    #echo "$app: $BEGINPAR -- $ENDPAR"

    		TITLE="$app"
    		GNUFILE="${RESULTDIR}/seq/$appname/memprofile.dat"

    		echo "plotting $app ..."	
    		export TITLE=$TITLE INPUT=$GNUFILE OUTPUT="${OUTPUTDIR}/${app}" BEGINPAR="$BEGINPAR" ENDPAR="$ENDPAR"
	  
    		gnuplot $_GRAPHFILE

  	done
}


function generate_tex {

	[ ! -d "${TABDIR}" ] && mkdir ${TABDIR}
	OUTPUTDIR="${TABDIR}/${_OUTDIR}"
	[ ! -d "${OUTPUTDIR}" ] && mkdir ${OUTPUTDIR}
    
	OUTPUTFILE="${OUTPUTDIR}/table5"
	TEXFILE="$OUTPUTFILE.tex"

	echo "\\documentclass{article}" > $TEXFILE
 	echo "\\usepackage[a4paper,margin=0.5in,landscape]{geometry}" >> $TEXFILE
 	echo "\\begin{document}" >> $TEXFILE
	echo "\\small" >> $TEXFILE


  	# summary
#	echo "" >> $TEXFILE  
#  	echo "\\begin{tabular}{l|llllll} \hline" >> $TEXFILE
#  	echo "Application & Memory (KiB) & Peak (KiB) & Total allocs & Peak allocs & Frees & Left \\\\\\hline" >> $TEXFILE
#  	for app in $_APPS; do
#		dirname="${app}${SUFFIX}"
#		appname="${app}${SUFFIX}"
#    
#    		INPUTFILE="${RESULTDIR}/seq/$dirname/${appname}-seq-glibc-1"
#  
#    		VAL=`cat ${INPUTFILE} | awk '{ \
#         	  if ($1 == "malloc_count") \
#           		print $9 " & " $11 " & " $4 " & " $5 " & " $7 " & " $13
#         	}
#         	'`
#  
#   		echo "$app & $VAL \\\\" >> $TEXFILE
# 	done
#  	echo "\hline" >> $TEXFILE
#  	echo "\\end{tabular}" >> $TEXFILE  
#	
#  	echo "\\\\" >> $TEXFILE  
#	echo "\\\\" >> $TEXFILE  

  # open the first file and count the number of columns (we assume all files
  # have the same number
  	for app in $_APPS; do
		dirname="${app}${SUFFIX}"
		appname="${app}${SUFFIX}"
    
    		INPUTFILE="${RESULTDIR}/seq/$dirname/${appname}-seq-glibc-1"
    
    		COL=`cat ${INPUTFILE} | awk '{ \
         	if ($1 == "bins:") \
           	  for (i=2; i <= NF; i++) printf "l"; \
         	}
         	'`
    		HEADER=`cat ${INPUTFILE} | awk '{ \
        	if ($1 == "bins:") \
           	  for (i=2; i <= NF; i++) printf "& %s ", $i; \
         	}
         	'`
    		break
  	done
		
	echo "" >> $TEXFILE  
  	echo "\\begin{tabular}{l|l|${COL}} \hline" >> $TEXFILE
  	echo "Application & Region $HEADER \\\\\\hline" >> $TEXFILE

	for app in $_APPS; do

    		dirname="${app}${SUFFIX}"
		appname="${app}${SUFFIX}"

    		INPUTFILE="${RESULTDIR}/seq/$dirname/${appname}-seq-glibc-1"

  
    # I do not like the ideia of multiple passes through the input file, but
    # since it is quite small this should not be a performance issue


    		SEQ=`cat ${INPUTFILE} | awk '{ \
         	if ($1 == "sequential:") \
           		for (i=2; i <= NF; i++) printf "& %s ", $i; \
         	}
         	'`
    
    		PAR=`cat ${INPUTFILE} | awk '{ \
         	if ($1 == "parallel:") \
           		for (i=2; i <= NF; i++) printf "& %s ", $i; \
         	}
         	'`
    
    		TX=`cat ${INPUTFILE} | awk '{ \
         	if ($1 == "transactional:") \
           		for (i=2; i <= NF; i++) printf "& %s ", $i; \
         	}
         	'`
		
 	   	echo "       & sequential $SEQ \\\\" >> $TEXFILE
    		echo "$app & parallel $PAR \\\\" >> $TEXFILE
    		echo "       & tx $TX \\\\\\hline" >> $TEXFILE

  	done
    
  	echo "\\end{tabular}" >> $TEXFILE  
	echo "" >> $TEXFILE  
  	echo "\\end{document}" >> $TEXFILE


	# compile the .tex file if 'pdflatex' is available
	if type "pdflatex" &> /dev/null; then
		echo "   compiling tex file ..."	
		cd ${TABDIR}/${_OUTDIR}
		pdflatex $TEXFILE > /dev/null
		rm ${OUTPUTFILE}.aux
		rm ${OUTPUTFILE}.log

	#	TGDIR="${TABDIR}/${_OUTDIR}"
		# and if we have 'gs' we concatenate both graph and table
	#	if type "gs" &> /dev/null; then
			# check if at least one plot file exists
	#		FLS=`ls ${TGDIR}/*spectrum.pdf 2>/dev/null | wc -w`
	#		if [ "$FLS" != "0" ]; then
			
		#		[ ! -d "${TGDIR}/all" ] && mkdir ${TGDIR}/all
	#			echo "   concatenating plot+latex ..."	
	#			gs -dBATCH -dNOPAUSE -q -sDEVICE=pdfwrite -sOutputFile=${TGDIR}/all.pdf ${TGDIR}/*spectrum.pdf table.pdf
	#		fi

	#	fi
		cd - > /dev/null
	fi


}




while getopts "d:a:h" opt;
do
	case $opt in
		a) _APPS=$OPTARG ;;
		d) _OUTDIR=$OPTARG ;;
		h) usage; exit 0 ;;
		\?) usage
			exit 0
	esac
done


validate_input
#generate_graph
generate_tex
