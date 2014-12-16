#!/bin/bash

# plot INTSET (Figure 4)
./plot-graph.sh -d intset-tinySTM -g bar-intset.gnu -a "intset-ll intset-rb intset-hs" -k "rw" -f "suicide" -t "1 2 4 6 8"


# plot STAMP (Figure 7)
./plot-graph.sh -d stamp-tinySTM -g bar-stamp.gnu -a "bayes genome intruder labyrinth vacation yada" -f "suicide" -l "tinySTM"


# STAMP speedup (Figure 8)
./plot-graph.sh -d stamp-tinySTM -z "all" -g line-stamp.gnu -f "suicide" -l "tinySTM" -a "genome"
./plot-graph.sh -d stamp-tinySTM -z "all" -g line-stamp.gnu -f "suicide" -l "tinySTM" -a "yada"


# Comparatives (Figure 6)
./plot-graph.sh -d shift-intset-1 -g bar-intset-compare.gnu -y "suicide" -a "intset-ll" -f "suicidee1" -k "rw" -t "1 2 4 6 8" 

echo "Generating table 3"
./gen-table3.sh
echo "Generating table 4"
./gen-table4.sh
echo "Generating table 5"
./gen-table5.sh -d paper
echo "Generating table 6"
./gen-table6.sh
echo "Generating table 7"
./gen-table7.sh
