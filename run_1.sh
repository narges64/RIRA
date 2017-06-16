#!/bin/bash 

name=gcb_results
for dram_cap in 32
do
	for trace in `cat ../../Traces/tlist` 
	do 
		./ssd page.parameters_MLC Results/${name}/cachedGC_result_${dram_cap}_gcb1_$trace  dram_capacity=${dram_cap} gcb_capacity=1 synthetic=0 trace_file=../../Traces/${trace} > Results/${name}/cachedGC_out_${dram_cap}_gcb1_$trace 
		./ssd page.parameters_MLC Results/${name}/cachedGC_result_${dram_cap}_gcb0_$trace  dram_capacity=${dram_cap} gcb_capacity=0 synthetic=0 trace_file=../../Traces/${trace} > Results/${name}/cachedGC_out_${dram_cap}_gcb0_$trace 
	done 
done

