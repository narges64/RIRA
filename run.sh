#!/bin/bash 

for rd in 0.01 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 0.99
do 
	for size in 16 32 64 
	do
		for inter in 1000000
		do 
			for chip in 64 
			do 
				echo "chip ".$chip." inter ".$inter." size ".$size." rd ".$rd 
				./ssd page.parameters_MT29F64G08_base base_$rd.$size.$inter.$chip $chip 1 trace_1 $rd $size $inter 0 > out_$chip_$size_$rd_$inter 
#				./ssd page.parameters_MT29F64G08_base gcio_$rd.$size.$inter.$chip $chip 1 trace_1 $rd $size $inter 1
#				./ssd page.parameters_MT29F64G08_base iogc_$rd.$size.$inter.$chip $chip 1 trace_1 $rd $size $inter 2
#				./ssd page.parameters_MT29F64G08_base gcgc_$rd.$size.$inter.$chip $chip 1 trace_1 $rd $size $inter 3
			done 
		done 
	done 
done 
