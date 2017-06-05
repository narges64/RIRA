#!/bin/bash 

for dram_cap in 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20  
do
	for gc in 0 0.05 0.1 0.15 0.2 0.25 0.3 0.35 0.4 0.45 0.5 0.55 0.6 0.65 0.7 0.75 0.8 0.85 0.9 0.95 1.0  
	do
		rd=0.1
		./ssd page.parameters_MLC Results/cachedGC_result_${dram_cap}_${gc}_${rd}  dram_capacity=$dram_cap gc_time_ratio=${gc} syn_rd_ratio=$rd > Results/cachedGC_out_${dram_cap}_${gc}_$rd
	done 
done 
