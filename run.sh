#!/bin/bash 

for rd in 0.9
do 
	for size in 16 32 48 64 80 96 112 128 256 512 1024 
	do
		for inter in 10000
		do 
			for chip in 64 
			do 
				echo "chip ".$chip." inter ".$inter." size ".$size." rd ".$rd 
				./ssd page.parameters_SLC slc_repeat_$rd.$size.$inter.$chip $chip 1 trace_1 $rd $size $inter 0 > out_repeat_$chip.$size.$rd.$inter 
#				./ssd page.parameters_MT29F64G08_base gcio_$rd.$size.$inter.$chip $chip 1 trace_1 $rd $size $inter 1
#				./ssd page.parameters_MT29F64G08_base iogc_$rd.$size.$inter.$chip $chip 1 trace_1 $rd $size $inter 2
#				./ssd page.parameters_MT29F64G08_base gcgc_$rd.$size.$inter.$chip $chip 1 trace_1 $rd $size $inter 3
			done 
		done 
	done 
done 
