#!/bin/bash 

for size in 128 4 # 32 64 128 256 512  
do
	for chip in 256 #128 64  #128 64 # 32 64  128  
	do 
		for queue in 32 # 1000 
		do 
			for rd in 0.9 0.5 0.1  
			do 
				echo "chip ".$chip." inter ".$inter." size ".$size." rd ".$rd.$queue
				./ssd page.parameters_SLC_0MB Results/slc_${rd}.${size}.${chip}.${queue}_0MB $chip 1 trace_1 $rd $size 100 0  $queue > Results/out_slc_$rd.$size.$chip.$queue_0MB
				./ssd page.parameters_SLC_8MB Results/slc_${rd}.${size}.${chip}.${queue}_8MB $chip 1 trace_1 $rd $size 100 0  $queue > Results/out_slc_$rd.$size.$chip.$queue_8MB
				./ssd page.parameters_SLC_16MB Results/slc_${rd}.${size}.${chip}.${queue}_16MB $chip 1 trace_1 $rd $size 100 0  $queue > Results/out_slc_$rd.$size.$chip.$queue_16MB
			done
		done 
	done 
done 
