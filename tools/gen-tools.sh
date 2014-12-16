#!/bin/bash
#
# Simple script to generate the binary files of the allocators
#

echo "Building tools..."

echo "#####################"
echo "## Memory profiler ##"
echo "#####################"
cd malloc_profile
make
if [ $? -eq 0 ]; then
	echo "  Memory profiler (successful)"
else
	echo "  Memory profiler (failed)"
fi
cd -


echo "##########"
echo "## PAPI ##"
echo "##########"
cd papi-5.2.0/src
./configure --prefix=$(pwd)/../install
make
if [ $? -eq 0 ]; then
	make install
	echo "  PAPI (successful)"
else
	echo "  PAPI (failed)"
fi
cd -

echo "Done."
