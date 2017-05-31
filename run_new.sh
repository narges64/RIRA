#!/bin/bash 

for dram_cap in 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 32 48 64  
do
	for rd in 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9  
	do 
		s=1
		temp=$dram_cap-1
		#gcb_cap=$s 	
		for (( gcb_cap=$s; gcb_cap<=$temp; gcb_cap++ )) 
		do 
			./ssd page.parameters_MLC Results/cachedGC_result_${dram_cap}_${gcb_cap}_${rd}  dram_capacity=$dram_cap gcb_capacity=$gcb_cap syn_rd_ratio=$rd > Results/cachedGC_out_${dram_cap}_${gcb_cap}_${rd} 
		done
	done 
done 
