#!/bin/bash 

for dram_cap in 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 
do
	for rd_ratio in 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 
	do  
		./ssd page.parameters_SLC result_${rd_ratio}.$dram_cap  dram_capacity=$dram_cap syn_rd_ratio=$rd_ratio > out${rd_ratio}.${dram_cap} 
	done 
done 
