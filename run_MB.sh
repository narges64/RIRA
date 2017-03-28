#!/bin/bash 


for i in {0..16}
do
	./ssd page.parameters_SLC_${i}MB test_${i}MB 16 1 trace_1 0.1 4 100 0 32 
done
