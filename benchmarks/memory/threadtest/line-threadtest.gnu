#!/usr/bin/gnuplot

TITLE = system("echo $TITLE")
INPUT_FILE = system("echo $INPUT")
OUTPUT_FILE = system("echo $OUTPUT")
TOTAL_COL = system("echo $COLS")

#print INPUT_FILE

#set terminal postscript eps enhanced solid color font 'Helvetica,14'
set terminal postscript eps enhanced font 'Helvetica,24'
set output '| ps2pdf -dEPSCrop - figure3.pdf'


set title TITLE

set xlabel "Block size (bytes)"
set ylabel "Throughput (x 10^6 op/s)"

set style data yerrorlines
#set style fill pattern 1.0 border lt -1
#set boxwidth 0.9 relative

#set style line 1 lc rgb "grey20"
#set style line 2 lc rgb "grey40"
#set style line 3 lc rgb "grey60"
#set style line 4 lc rgb "grey80"
#set style increment user
   
set style line 1 lt 1 pt 3 ps 2.5 lw 2
set style line 3 lt 1 pt 5 ps 2.5 lw 2
set style line 5 lt 1 pt 7 ps 2.5 lw 2
set style line 7 lt 1 pt 9 ps 2.5 lw 2

   
#set bars fullwidth
#set bars large

#set size 1.1, 1.0
#set lmargin 0.0

#set logscale x 2 
#set mytics
#set grid ytics mytics lw 2, lw 1

set format y "%.0s"

set xtics ("16" 1, "64" 2, "128" 3, "256" 4, "512" 5, "2048" 6, "8192" 7)

# 
# Key options
#
set key on
#set key center bmargin horizontal Left reverse
set key center tmargin horizontal Left reverse
set key autotitle columnhead


plot for [COL=2:TOTAL_COL:2] INPUT_FILE using 1:COL:COL+1 ls COL-1


