#!/bin/bash 

for dram_cap in 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 
do
	./ssd page.parameters_SLC result_$dram_cap  dram_capacity=$dram_cap syn_rd_ratio=0.01 > out_${dram_cap} 
done 
