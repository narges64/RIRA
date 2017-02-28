#!/bin/bash 

for rd in 1.0 0.9 0.8 0.7 0.6 0.5 0.4 0.3 0.2 0.1 0.0  
do 
	for size in 128 16 # 32 64 128 256 512  
	do
		for chip in 256 128 64  #128 64 # 32 64  128  
		do 
			for queue in 32  1000 
			do 
				echo "chip ".$chip." inter ".$inter." size ".$size." rd ".$rd.$queue
				./ssd page.parameters_SLC Results/slc_$rd.$size.$chip.$queue $chip 1 trace_1 $rd $size 100 0  $queue > Results/out_slc_$rd.$size.$chip.$queue
				./ssd page.parameters_MLC Results/mlc_$rd.$size.$chip.$queue $chip 1 trace_1 $rd $size 100 0  $queue > Results/out_mlc_$rd.$size.$chip.$queue 
				./ssd page.parameters_TLC Results/tlc_$rd.$size.$chip.$queue $chip 1 trace_1 $rd $size 100 0  $queue > Results/out_tlc_$rd.$size.$chip.$queue 
	#			./ssd page.parameters_MLC mlc_$rd.$size.$chip $chip 1 trace_1 $rd $size 100 0 > out_mlc_$chip.$size.$rd
	#			./ssd page.parameters_TLC tlc_$rd.$size.$chip $chip 1 trace_1 $rd $size 100 0 > out_tlc_$chip.$size.$rd
			done
		done 
	done 
done 
