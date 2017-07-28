#!/bin/bash 

name=Synthetic

for dram in 16
do 
	for rd in 0.05 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 0.95
	do 	 
		./ssd page.parameters_MLC Results/$name/cachedGC_result_${dram}_${rd}_alg0 dram_capacity=${dram} synthetic=1 syn_rd_ratio=${rd} gc_algorithm=0 > Results/$name/cachedGC_out_${dram}_${rd}_alg0  
	#	./ssd page.parameters_MLC Results/$name/cachedGC_result_${dram}_${rd}_alg3 dram_capacity=${dram} synthetic=1 syn_rd_ratio=${rd}  gc_algorithm=3 > Results/$name/cachedGC_out_${dram}_${rd}_alg3  
	done
done

