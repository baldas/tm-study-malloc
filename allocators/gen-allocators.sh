#!/bin/bash
#
# Simple script to generate the binary files of the allocators
#

echo "Building allocators..."

# Hoard 3.10
echo "###########"
echo "## Hoard ##"
echo "###########"
cd Hoard-3.10/src
make linux-gcc-x86-64
if [ $? -eq 0 ]; then
	mv libhoard.so ../../libhoard.so.3.10
	echo "  Hoard (successful)"
else
	echo "  Hoard (failed)"
fi
cd -


# TBBMalloc 4.1
echo "###############"
echo "## TBBMalloc ##"
echo "###############"
cd tbb41_20130314oss
make tbbmalloc
if [ $? -eq 0 ]; then
	mv build/linux_intel64_gcc_cc*_release/libtbbmalloc*.so.2 ../
	echo "  TBBMalloc (successful)"
else
	echo "  TBBMalloc (failed)"
fi
cd -


# TCMalloc minimal 3.1
echo "##############"
echo "## TCMalloc ##"
echo "##############"
cd gperftools-2.1
./configure --prefix=$(pwd)/bin --enable-minimal CXXFLAGS=-O3
make
if [ $? -eq 0 ]; then
	make install
	mv bin/lib/libtcmalloc_minimal.so.4.1.2 ../
	echo "  TCMalloc (successful)"
else
	echo "  TCMalloc (failed)"
fi
cd -

echo "Done."
