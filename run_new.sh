#!/bin/bash 

for rd in 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 
do
	for dram_cap in 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 32 48 64 72 80 88 96 
	do 	
		./ssd page.parameters_SLC cachedGC_result_${dram_cap}_${rd}  dram_capacity=$dram_cap syn_rd_ratio=$rd > cachedGC_out_${dram_cap}_${rd} 
	done 
done 
