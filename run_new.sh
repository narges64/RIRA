#!/bin/bash 

for rd in 0.1 0.2 0.3 0.4 0.5 
do
	for gcb_cap in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 
	do 	
		./ssd page.parameters_SLC cachedGC_result_32_${gcb_cap}_${rd}  dram_capacity=32 gcb_capacity=$gcb_cap syn_rd_ratio=$rd > cachedGC_out_32_${gcb_cap}_${rd} 
	done 
done 
