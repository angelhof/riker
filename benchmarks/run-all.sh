#!/bin/sh

# this is just a convenient script to run all of the benchmarks;
# the benchmark runner can already handle running multiple
# programs, but the syntax is a tad fiddly

BENCHMARKS=`find . -iname "benchmark.json" -print`

./run.py --cleanup-before --cleanup-after --incr-none-dodo --incr-none-make --dont-ask $1 $BENCHMARKS