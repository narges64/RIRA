#!/bin/bash 

for rd in 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 
do 
	for size in 64
	do
		for inter in 10000000
		do 
			for chip in 4 
			do 
				echo "chip ".$chip." inter ".$inter." size ".$size." rd ".$rd 
				./ssd page.parameters_MT29F64G08_base base_$rd.$size.$inter.$chip $chip 1 trace_1 $rd $size $inter 0 
				./ssd page.parameters_MT29F64G08_base gcio_$rd.$size.$inter.$chip $chip 1 trace_1 $rd $size $inter 1
				./ssd page.parameters_MT29F64G08_base iogc_$rd.$size.$inter.$chip $chip 1 trace_1 $rd $size $inter 2
				./ssd page.parameters_MT29F64G08_base gcgc_$rd.$size.$inter.$chip $chip 1 trace_1 $rd $size $inter 3
			done 
		done 
	done 
done 
