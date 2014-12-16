#!/usr/bin/gnuplot

#TITLE = "teste"
TITLE = system("echo $TITLE")
#INPUT_FILE = "teste.table"
INPUT_FILE = system("echo $INPUT")
#OUTPUT_FILE = "teste"
OUTPUT_FILE = system("echo $OUTPUT")
#TOTAL_COL = 8
TOTAL_COL = system("echo $COLS")


set terminal postscript eps enhanced color solid font 'Helvetica,24'
set output '| ps2pdf -dEPSCrop - '.OUTPUT_FILE.'-bar.pdf'


set title TITLE

set xlabel "Number of cores"
set ylabel "Time (s)"

set style data histograms
set style histogram errorbars gap 2 lw 2
set style fill solid 1.0 border lt -1
#set style fill pattern 1.0 border lt -1
set boxwidth 0.9 relative

set style line 1 lc rgb "grey80"
set style line 3 lc rgb "grey40"
set style line 5 lc rgb "grey60"
set style line 7 lc rgb "grey20"
#set style increment user
      
set bars fullwidth
#set bars large

#set size 1.1, 1.0
#set lmargin 0.0

set mytics
#set grid ytics mytics lw 2, lw 1

#set format y "%+-12.3f"
set format y "%.0s*10^%T"
set format y "%.0s"

# 
# Key options
#
set key on
set key right top vertical Left reverse
#set key center bmargin horizontal Left reverse
set key autotitle columnhead

#plot for [COL=2:TOTAL_COL:2] INPUT_FILE using COL:COL+1:xticlabels(1) t columnhead(COL) ls COL-1

GAPSIZE=2
NCOL=TOTAL_COL/2-2+1
BOXWIDTH=1/(GAPSIZE+4) #Width of each box.
STARTCOL=2
plot for [COL=2:TOTAL_COL:12] INPUT_FILE using COL:COL+1:xticlabels(1) t columnhead(COL) ls COL-1
