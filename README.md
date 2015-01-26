Transactional Memory and Memory Allocators Interaction Study
===============

This repository contains the tools used in our PPoPP'15 paper *Performance 
Implications of Dynamic Memory Allocators on Transactional Memory Systems*. 
It can be used to reproduce our results and, with the necessary changes, 
investigate the behavior of new allocators and/or (S)TMs.

The repository contains the following open-source software:

* Allocators
  * Hoard (version 3.10) - See http://www.hoard.org
  * GPerftools (version 2.1) - See https://code.google.com/p/gperftools
  * Intel TBB (version 4.1) - See https://www.threadingbuildingblocks.org
* Benchmarks
  * Synthetic programs (`threadtest` and `intset`)
  * STAMP (Community version) - See https://code.google.com/p/tm-benchmarks
* TM
  * TinySTM (version (1.0.4) - See http://tmware.org/tinystm
* Tools
  * malloc_count - Adpated from http://panthema.net/2013/malloc_count/
  * PAPI 5.2 - See http://icl.cs.utk.edu/papi/


Dependences
-----------

The scripts used to compile, run, and post-process the data rely on some
linux-based tools. In general they should be available in a typical distro, but
we are listing here the main ones. In case some of the scripts finish with a
weird error message, this could be of help.

* Gnuplot
* R Project - See http://www.r-project.org

You should also check the dependences of each of the tools (allocators, STM,
etc.) before compiling hem.


Code Organization
-----------------

The code is organized in the following directory structure:

.
├── allocators
├── benchmarks
│   ├── memory
│   │   └── threadtest
│   │       ├── results-paper
│   │       └── results-smallrun
│   └── stamp
│       ├── data
│       ├── results-paper
│       ├── results-smallrun
│       ├── scripts
│       ├── seq
│       ├── tinySTM
│       └── trunk
├── tm
│   └── tinySTM-1.0.4
└── tools
    ├── malloc_profile
    └── papi-5.2.0

The top directories contain the allocators, benchmarks, tm library and tools
needed to run the experiments. Only the source code are provided.  Binaries are
provided as part of our
[artifact](http://lampiao.lsc.ic.unicamp.br/~baldas/artifact/ppopp15-artifact.html)
only.



Quick Start
-----------

The first thing you should do is compile the packages needed to execute the
experiments.  This includes the allocators, tools, TM libraries and, finally,
benchmarks. We provide a simple bash script to compile each of them (please
enter their respective directories before executing the commands):

* Allocators
	`./gen-allocators.sh`

* Tools (needed if cache miss data is required)
	`./gen-tools.sh`

* Threadtest
	`make`

* STAMP and Intset
	`./compile-all.sh`


If all succeeds you are ready to run the experiments. Most of the benchmarks 
have a script to do it. You might want to look in the `benchmarks/stamp/scripts/`. 
The main script is `execute.sh`, followed by `gen-data.sh` and `plot-graph.sh`. 
They currently have a very limited but somewhat useful command-line help system.  
For instance, to run the `yada` application 5 times you should run:

`./execute.sh -a "yada" -n 5`

This will run the application with the default STM library (tinySTM), with four
allocators (glibc, hoard, tbbmalloc, and tcmalloc), 1, 2, 4, and 8 threads, 
five times each. Try using `./execute.sh -h` for some help.

To post-process the data and generate the chart for the previous executions you 
could try the following:

`./plot-graph.sh -d results-dir -g bar-stamp.gnu -a "yada"`

It will store the tables and chart inside directory named `results-dir` (flag
`-d`).  The root directory for the results is stored in the `scripts.cfg` file,
variable `RESULTDIR`. This file also holds general configuration options used
by the scripts.  The plot-graph relies on the `gnuplot` script (flag `-g`) to
plot the graph. Currently, the ones provided (extension `.gnu`) only support 8
threads, but it should not be hard to change them to your needs.


Please keep in mind that this is working-in-progress and any help is really
welcome.  Also, take a look at our [artifact
page](http://lampiao.lsc.ic.unicamp.br/~baldas/artifact/ppopp15-artifact.html)
for further information.


