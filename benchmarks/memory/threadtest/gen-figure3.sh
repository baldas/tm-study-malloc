#!/bin/bash


echo "Post-processing data"
./run.sh gen
if [ $? -ne 0 ]; then
	echo "  Post-processing failed"
	exit 1
fi

echo "Generating chart"
./run.sh plot -d tables/threadtest-8-8-table.dat 
