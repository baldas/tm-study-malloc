#!/usr/bin/gnuplot

TITLE = system("echo $TITLE")
INPUT_FILE = system("echo $INPUT")
OUTPUT_FILE = system("echo $OUTPUT")
TOTAL_COL = system("echo $COLS")

#print INPUT_FILE

set terminal postscript eps enhanced font 'Helvetica,24'
set output '| ps2pdf -dEPSCrop - '.OUTPUT_FILE.'-line.pdf'


set title TITLE

set xlabel "Number of cores"
set ylabel "Speedup"

set style data yerrorlines
#set style fill pattern 1.0 border lt -1
#set boxwidth 0.9 relative

#set style line 1 lc rgb "grey20"
#set style line 2 lc rgb "grey40"
#set style line 3 lc rgb "grey60"
#set style line 4 lc rgb "grey80"
#set style increment user
   
set style line 1 lt 1 pt 3 ps 2.8 lw 2
set style line 3 lt 1 pt 5 ps 2.8 lw 2
set style line 5 lt 1 pt 7 ps 2.8 lw 2
set style line 7 lt 1 pt 9 ps 2.8 lw 2

   
#set bars fullwidth
#set bars large

#set size 1.1, 1.0
#set lmargin 0.0

#set logscale x 2 
#set mytics
#set grid ytics mytics lw 2, lw 1

set xtics (1,2,4,8)

# 
# Key options
#
set key on
#set key center bmargin horizontal Left reverse
set key left top horizontal Left reverse 
#set key center bmargin horizontal Left reverse 
set key autotitle columnhead
set key width -3 samplen 3 spacing 1 font ", 24"
#set key  samplen 2 font "Helvetica, 16"

plot for [COL=2:TOTAL_COL:12] INPUT_FILE using 1:COL:COL+1 t columnhead(COL) ls COL-1


