#!/bin/bash

#
# A wrapper to gen-data.sh.
#

source scripts.cfg

GRAPHFILE="plotscript-table.gnu" 
MODIFIERS="-"
WORKTYPE="-"
GENDATA="yes"
GENTEX="no"
PLOTTYPE="default"
NORMFILE="-"
COMPFLV="-"

function usage {

	echo $0'-d <output-dir> [-n <sufix-name>] [-g <graph-file>] [-z <norm-file>] [-a <applications>] [-r <app modifier>] [-k <workload type>] [-f <flavors>] [-l <stm-lib>] [-m <mem-allocators>] [-t <threads>] [-b <plottype>] [-u] [-j]'

	echo
	echo "-d <output-dir>: subdirectory to store the generated files "
	
	echo
	echo "-n <sufix-name>: a name to be used as sufix for the output filenames"
	
	echo
	echo "-g <graph-file>: gnuplot script file to plot the graph "
	echo "default = $GRAPHFILE"

	echo
	echo "-a <applications>: application list"
 	echo "default = $APPS"	

	echo
	echo "-r <app modifier>: application modifier"
	echo "a list of sufixes to be appended to each application name (ex: _taff)"
	echo "use '-' in case no one is required"
	echo "default = $MODIFIERS"

	echo
	echo "-k <workload type>: a list of workload types"
	echo "each type will be appended to application name (after the modifier)"
	echo "Ex: -k \"read r20\""
	echo "use '-' in case no one is required"
	echo "default = $WORKTYPE"

	echo
	echo "-f <flavors>: library variation" 
	echo "default = $FLAVORS"

	echo
	echo "-l <stm-lib>: stm libraries -  use 'seq' for sequential version"
	echo "default = $STMLIBS"
	
	echo
	echo "-m <mem-allocators>: memory allocators"
	echo "default = $ALLOCATORS"

	echo
	echo "-t <threads>: list with number of threads to be used"
	echo "default = $THREADS"
	
	echo
	echo "-b <plottype>: type of the plot generated"
	echo " default is columns agrouped by allocators"
	echo "  c1: put each modifier (specified by -r) side by side in the same column"
	echo "  c2: same as c1, but for flavors"
        echo "  c3: put each allocator on a different graph"

	echo
	echo "-u: do not generate the data file"
	echo "default = $GENDATA"

	echo
	echo "-j: also generate tex file"
	echo "default = $GENTEX"

	echo
	echo "-z <norm-file>: normalize to file <norm-file>"
	echo "default = $NORMFILE"
}

function show_using {

#	echo $0' -d <output-dir> [-g graph-file] [-a <applications>] [-r <app modifier>] [-k <workload type>] [-f <flavors>] [-l <stm-lib>] [-m <mem-allocators>] [-t <threads>] [-b <plottype>] [-u] [-j]' $1
	echo "Using:" $1
	echo "  outd:   $_OUTDIR" $1
	echo "  graf:   $_GRAPHFILE" $1
	echo "  apps:   $_APPS" $1
	echo "  mods:   $_MODIFIERS" $1
	echo "  work:   $_WORKTYPE" $1
	echo "  flvs:   $_FLAVORS" $1
	echo "  libs:   $_STMLIBS" $1
	echo "  alls:   $_ALLOCATORS" $1
	echo "  thrs:   $_THREADS" $1
	echo "  gend:   $_GENDATA" $1
	echo "  gtex:   $_GENTEX" $1
	echo "  plot:   $_PLOTTYPE" $1
	echo "  norm:   $_NORMFILE" $1
	echo "  cflv:   $_COMPFLV" $1
}




function validate_input {

# if not set, use the default configs
	[ -z "$_APPS" ] && _APPS=$APPS
	[ -z "$_GRAPHFILE" ] && _GRAPHFILE=$GRAPHFILE
	[ -z "$_MODIFIERS" ] && _MODIFIERS=$MODIFIERS
	[ -z "$_WORKTYPE" ] && _WORKTYPE=$WORKTYPE
	[ -z "$_FLAVORS" ] && _FLAVORS=$FLAVORS
	[ -z "$_STMLIBS" ] && _STMLIBS=$STMLIBS
	[ -z "$_ALLOCATORS" ] && _ALLOCATORS=$ALLOCATORS
	[ -z "$_THREADS" ] && _THREADS=$THREADS
	[ -z "$_GENDATA" ] && _GENDATA=$GENDATA
	[ -z "$_GENTEX" ] && _GENTEX=$GENTEX
	[ -z "$_PLOTTYPE" ] && _PLOTTYPE=$PLOTTYPE
	[ -z "$_NORMFILE" ] && _NORMFILE=$NORMFILE
        [ -z "$_COMPFLV" ] && _COMPFLV=$COMPFLV

# check if number of threads is a valid numeric value: will be checked by gen-data.sh
		
	show_using >&1
				
	if [ ! -f $_GRAPHFILE ]; then
		echo "Graph file not found"
		exit 1
	fi
}

function plot_file {
	
	#[ "$_CATEGORIES" == " - " ] && _CATEGORIES=""

	#TITLE="${_PREFIXNAME} ${_CATEGORIES}"
	TITLE="$1"
	INPUTFILE="${TABDIR}/${_OUTDIR}/$1.table"

	[ ! -d "${TABDIR}/${_OUTDIR}/plot" ] && mkdir ${TABDIR}/${_OUTDIR}/plot

	OUTPUTFILE="${TABDIR}/${_OUTDIR}/plot/$1"
	COLS=`echo $2 | wc -w`
	let TOTAL_COL=COLS*12
#	echo "$2 $COLS $TOTAL_COL"
	export TITLE=$TITLE INPUT=$INPUTFILE OUTPUT="$OUTPUTFILE" COLS=$TOTAL_COL

	echo "   plotting ..."	
	gnuplot $_GRAPHFILE
}

function generate_tex {
	
	[ ! -d "${TABDIR}/${_OUTDIR}/tex" ] && mkdir ${TABDIR}/${_OUTDIR}/tex

	TEXFILE="${TABDIR}/${_OUTDIR}/tex/$1.tex"
  echo "\\documentclass{article}" > $TEXFILE
  echo "\\usepackage[a4paper,margin=0.5in,landscape]{geometry}" >> $TEXFILE
  echo "\\begin{document}" >> $TEXFILE
  echo "\\small" >> $TEXFILE
	
	INFILE="${TABDIR}/${_OUTDIR}/$1"
	
	_COLUMNS=$2
	COLS=`echo $_COLUMNS | wc -w`
	COLNUM=2
	echo "   generating tex file ..."	
	for col in $_COLUMNS; do
		
		IFS=":" 
		tokens=( $col );
		unset IFS

		APP=${tokens[0]}
		STM=${tokens[1]}
		FLV=${tokens[2]}
		ALLOC=${tokens[3]}
	
		echo "\\begin{tabular}{l|llllllllll} \\hline" >> $TEXFILE
		echo "& \\multicolumn{10}{c}{ $ALLOC } \\\\\hline" >> $TEXFILE
		echo "& time & ci & aborts & ci & PAPI1 & ci & PAPI2 & ci & PAPI3 & ci \\\\\hline" >> $TEXFILE
		
		awk -v fc=$COLNUM '{ if (NR>1) {time=$fc; timeci = (time == 0) ? 0 : ($(fc+1)/time)*100; \
					abort=$(fc+2); abortci = (abort == 0) ? 0 : ($(fc+3)/abort)*100; \
					papi1=$(fc+4); papi1ci = (papi1 == 0) ? 0 : ($(fc+5)/papi1)*100; \
					papi2=$(fc+6); papi2ci = (papi2 == 0) ? 0 : ($(fc+7)/papi2)*100; \
					papi3=$(fc+8); papi3ci = (papi3 == 0) ? 0 : ($(fc+9)/papi3)*100; \
				printf "%u & %\047.2f & %.2f & %\047.0f & %.2f & %\047.2f & %.2f & %\047.0f & %.2f & %\047.0f & %.2f \\\\ \n", \
				$1, time, timeci, abort, abortci, papi1, papi1ci, papi2, papi2ci, papi3, papi3ci } \
			}' ${INFILE}.table >> $TEXFILE

		echo "\\end{tabular}" >> $TEXFILE  
		echo "" >> $TEXFILE  
		let COLNUM=COLNUM+10
	done
		
	echo "\\end{document}" >> $TEXFILE


	# compile the .tex file if 'pdflatex' is available
	if type "pdflatex" &> /dev/null; then
		echo "   compiling tex file ..."	
		cd ${TABDIR}/${_OUTDIR}/tex
		pdflatex $TEXFILE > /dev/null
		rm $1.aux
		rm $1.log

		TGDIR="${TABDIR}/${_OUTDIR}"
		# and if we have 'gs' we concatenate both graph and table
		if type "gs" &> /dev/null; then
			# check if at least one plot file exists
			FLS=`ls ${TGDIR}/plot/$1*.pdf 2>/dev/null | wc -w`
			if [ "$FLS" != "0" ]; then
			
				[ ! -d "${TGDIR}/all" ] && mkdir ${TGDIR}/all
				echo "   concatenating plot+latex ..."	
				gs -dBATCH -dNOPAUSE -q -sDEVICE=pdfwrite -sOutputFile=${TGDIR}/all/$1.pdf ${TGDIR}/plot/$1*.pdf $1.pdf
			fi

		fi
		
		cd - > /dev/null
	fi

}





function classic_plot {

	for stm in $_STMLIBS; do
		for flv in $_FLAVORS; do
			for app in $_APPS; do
				for mod in $_MODIFIERS; do
					for wt in $_WORKTYPE; do
						COL=""
						[ "$mod" == "-" ] && mod="" 
						if [ "$wt" == "-" ]; then
							wt=""
							PREFX="${COL}$app${mod}:$stm:$flv:"	
							FILENAME="${app}${mod}-$flv${_SUFIXNAME}" 
						else
							PREFX="${COL}$app${mod}.$wt:$stm:$flv:"	
							FILENAME="${app}${mod}-$wt-$flv${_SUFIXNAME}" 
						fi
						
						for alloc in $_ALLOCATORS; do
							COL="${COL}${PREFX}${alloc} "	
						done

						if [ "$_NORMFILE" != "-" ]; then
							FILENAME="${FILENAME}-norm"
						fi
									
						if [ "$_COMPFLV" != "-" ]; then
							FILENAME="${FILENAME}-rel-${_COMPFLV}"
						fi

						echo "=== Generating ${FILENAME} ==="
						#echo "-d ${_OUTDIR} -n ${FILENAME} -c \"$COL\" -t \"$THREADS\""
					
						if [ "$_GENDATA" == "yes" ]; then
							if [ "$_NORMFILE" == "-" ]; then
								if [ "$_COMPFLV" == "-" ]; then
									./gen-data.sh -d ${_OUTDIR} -n "${FILENAME}" -c "$COL" -t "$_THREADS"
								else
									./gen-data.sh -d ${_OUTDIR} -y "$_COMPFLV" -n "${FILENAME}" -c "$COL" -t "$_THREADS"
								fi
							else
								./gen-data.sh -d ${_OUTDIR} -n "${FILENAME}" -z "$_NORMFILE" -c "$COL" -t "$_THREADS"
							fi
							if [[ $? -ne 0 ]]; then
								echo "Error while processing ${app}${mod}${wt}"
								exit 1
							fi
						else
							# check if file exists
							if [ ! -f "${TABDIR}/${_OUTDIR}/${FILENAME}.table" ]; then
								echo "File - ${TABDIR}/${_OUTDIR}/${FILENAME}.table - not found"
								exit 1
							fi
						fi

						plot_file "${FILENAME}" "$COL" "${FILENAME}"
					
						if [ "$_GENTEX" == "yes" ]; then
							generate_tex "${FILENAME}" "$COL"
						fi
					done
				done
			done
		done
	done
}	


function category_plot {

	for stm in $_STMLIBS; do
		for flv in $_FLAVORS; do
			for app in $_APPS; do
				for wt in $_WORKTYPE; do
					PREFX=""
					COL=""
					if [ "$wt" == "-" ]; then
						FILENAME="${app}-$flv[" 
					else
						FILENAME="${app}-$wt-$flv["	
					fi
					SUF=`echo $_MODIFIERS | sed 's/ /#/g'`
					#SUF="$_MODIFIERS"
					FILENAME="${FILENAME}$SUF]${_SUFIXNAME}"
					for alloc in $_ALLOCATORS; do
					
						for mod in $_MODIFIERS; do
							[ "$mod" == "-" ] && mod="" 
					
							if [ "$wt" == "-" ]; then
							#	wt=""
								PREFX="$app${mod}:$stm:$flv:"	
							else
								PREFX="$app${mod}.$wt:$stm:$flv:"	
							fi
					
							COL="${COL}${PREFX}${alloc} "	
						done	
					done
				
					#echo $COL
					echo "=== Generating ${FILENAME} ==="

					if [ "$_GENDATA" == "yes" ]; then
						./gen-data.sh -d ${_OUTDIR} -n "${FILENAME}" -c "$COL" -t "$_THREADS"
						if [[ $? -ne 0 ]]; then
							echo "Error while processing ${app}${mod}${wt}"
							exit 1
						fi
					else
						# check if file exists
						if [ ! -f "${TABDIR}/${_OUTDIR}/${FILENAME}.table" ]; then
							echo "File - ${TABDIR}/${_OUTDIR}/${FILENAME}.table - not found"
							exit 1
						fi
					fi

					plot_file "${FILENAME}" "$COL" "${FILENAME}"
					
					if [ "$_GENTEX" == "yes" ]; then
						generate_tex "${FILENAME}" "$COL"
					fi
				done
			done
		done
	done
}	





function flavor_plot {

	for stm in $_STMLIBS; do
		for app in $_APPS; do
			for mod in $_MODIFIERS; do
				for wt in $_WORKTYPE; do
					COLUMNS=""
					COL="$app"
					if [ "$mod" != "-" ]; then
						COL="${COL}${mod}"
					else
						mod=""
					fi

					if [ "$wt" == "-" ]; then
						wt=""
						COL="${COL}:$stm" 
					else
						COL="${COL}.$wt:$stm"	
					fi
					
					for flv in $_FLAVORS; do

						COLUMNST="${COL}:$flv"

						for alloc in $_ALLOCATORS; do
				
							COLUMNS="${COLUMNS}${COLUMNST}:$alloc "
						done	
					done
				
					#echo $COLUMNS
					#exit
					FILENAME="${app}${wt}${mod}${_SUFIXNAME}"
					echo "=== Generating ${FILENAME} ==="

					if [ "$_GENDATA" == "yes" ]; then
						#./gen-data.sh -d ${_OUTDIR} -s "${_FLAVORS}" -n "${FILENAME}" -c "$COLUMNS" -t "$_THREADS"
						./gen-data.sh -d ${_OUTDIR} -n "${FILENAME}" -c "$COLUMNS" -t "$_THREADS"
						if [[ $? -ne 0 ]]; then
							echo "Error while processing ${app}${mod}${wt}"
							exit 1
						fi
					else
						# check if file exists
						if [ ! -f "${TABDIR}/${_OUTDIR}/${FILENAME}.table" ]; then
							echo "File - ${TABDIR}/${_OUTDIR}/${FILENAME}.table - not found"
							exit 1
						fi
					fi

					plot_file "${FILENAME}" "$COLUMNS"
					
					if [ "$_GENTEX" == "yes" ]; then
						generate_tex "${FILENAME}" "$COLUMNS"
					fi
				done
			done
		done
	done
}	


function allocator_plot {

	for stm in $_STMLIBS; do
		for flv in $_FLAVORS; do
			for app in $_APPS; do
				for mod in $_MODIFIERS; do
					for wt in $_WORKTYPE; do
						for alloc in $_ALLOCATORS; do

							[ "$mod" == "-" ] && mod="" 
							if [ "$wt" == "-" ]; then
								wt=""
								PREFX="$app${mod}:$stm:$flv:$alloc"	
								FILENAME="${app}${mod}-$flv-$alloc" 
							else
								PREFX="$app${mod}.$wt:$stm:$flv:$alloc"	
								FILENAME="${app}${mod}-$wt-$flv-$alloc" 
							fi
					
							FILENAME="${FILENAME}${_SUFIXNAME}"	
							echo "=== Generating ${FILENAME} ==="
							#echo $PREFX
							#exit 0
					
							if [ "$_GENDATA" == "yes" ]; then
								./gen-data.sh -d ${_OUTDIR} -n "${FILENAME}" -c "$PREFX" -t "$_THREADS"
								if [[ $? -ne 0 ]]; then
									echo "Error while processing ${app}${mod}${wt}"
									exit 1
								fi
							else
								# check if file exists
								if [ ! -f "${TABDIR}/${_OUTDIR}/${FILENAME}.table" ]; then
									echo "File - ${TABDIR}/${_OUTDIR}/${FILENAME}.table - not found"
									exit 1
								fi
							fi

							plot_file "${FILENAME}" "$PREFX" "${FILENAME}"
					
							if [ "$_GENTEX" == "yes" ]; then
								generate_tex "${FILENAME}" "$PREFX"
							fi
						done
					done
				done
			done
		done
	done
}	



while getopts "d:n:g:a:r:k:f:l:m:t:b:z:y:huj" opt;
do
	case $opt in
		d) _OUTDIR=$OPTARG ;;
		n) _SUFIXNAME=$OPTARG ;;	
		g) _GRAPHFILE=$OPTARG ;;
		a) _APPS=$OPTARG ;;
		r) _MODIFIERS=$OPTARG ;;
		k) _WORKTYPE=$OPTARG ;;
		f) _FLAVORS=$OPTARG ;;
		l) _STMLIBS=$OPTARG ;;
		m) _ALLOCATORS=$OPTARG ;;
		t) _THREADS=$OPTARG ;;
		b) _PLOTTYPE=$OPTARG ;;
		u) _GENDATA="no" ;;
		j) _GENTEX="yes" ;;
		z) _NORMFILE="$OPTARG" ;;
		y) _COMPFLV="$OPTARG" ;;
		h) usage; exit 0 ;;
		\?) usage
			exit 0
	esac
done

if [ -z "$_OUTDIR" ]; then
	echo "output directory name required"
	usage
	exit 0
fi

validate_input

if [ "$_PLOTTYPE" == "c1" ]; then
	category_plot
elif [ "$_PLOTTYPE" == "c2" ]; then
	flavor_plot
elif [ "$_PLOTTYPE" == "c3" ]; then
	allocator_plot
elif [ "$_PLOTTYPE" == "default" ]; then
	classic_plot
else
	echo "invalid plot type: $_PLOTTYPE"
fi
