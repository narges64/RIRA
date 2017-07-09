#!/bin/bash 

name=Synthetic

for gc_cap in 0 32 64 96  
do 
	for rd in 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 
	do 	 
		./ssd page.parameters_MLC Results/$name/cachedGC_result_128_${gc_cap}_${rd}_alg0 dram_capacity=128 synthetic=1 syn_rd_ratio=${rd} gcb_capacity=${gc_cap}  gc_algorithm=0 > Results/$name/cachedGC_out_128_${gc_cap}_${rd}_alg0  
		./ssd page.parameters_MLC Results/$name/cachedGC_result_128_${gc_cap}_${rd}_alg3 dram_capacity=128 synthetic=1 syn_rd_ratio=${rd} gcb_capacity=${gc_cap}  gc_algorithm=3 > Results/$name/cachedGC_out_128_${gc_cap}_${rd}_alg3  
	done
done


for gc_cap in 0 8 16 24
do 
	for rd in 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 
	do 	 
		./ssd page.parameters_MLC Results/$name/cachedGC_result_32_${gc_cap}_${rd}_alg0 dram_capacity=32 synthetic=1 syn_rd_ratio=${rd} gcb_capacity=${gc_cap}  gc_algorithm=0 > Results/$name/cachedGC_out_32_${gc_cap}_${rd}_alg0  
		./ssd page.parameters_MLC Results/$name/cachedGC_result_32_${gc_cap}_${rd}_alg3 dram_capacity=32 synthetic=1 syn_rd_ratio=${rd} gcb_capacity=${gc_cap}  gc_algorithm=3 > Results/$name/cachedGC_out_32_${gc_cap}_${rd}_alg3  
	done
done 
