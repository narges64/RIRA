#!/bin/bash 


for i in {1..10}
do
	./ssd page.parameters_SLC test_$i 16 1 trace_1 0.1 4 100 0 32 
done
