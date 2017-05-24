#!/bin/bash 

echo > reads 
echo > writes 

for i in 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 
do
	echo >> writes 
	echo $i >> writes
	echo >> reads 
	echo $i >> reads  
	grep "response" cachedGC_result_*_${i} | grep write | awk 'BEGIN{FS="_"}{print $3"\t"$4}' | awk '{print $1"\t"$8}' | sort -n -k 1 >> writes 
	grep "response" cachedGC_result_*_${i} | grep read | awk 'BEGIN{FS="_"}{print $3"\t"$4}' | awk '{print $1"\t"$8}' | sort -n -k 1 >> reads 

#	grep IOPS cachedGC_result_*_${i} | grep write | awk 'BEGIN{FS="_"}{print $3"\t"$4}' | awk '{print $1"\t"$7"\t"$9}' | sort -n -k 1 >> writes 
#	grep IOPS cachedGC_result_*_${i} | grep read | awk 'BEGIN{FS="_"}{print $3"\t"$4}' | awk '{print $1"\t"$7"\t"$9}' | sort -n -k 1 >> reads 

	echo >> writes 
	echo >> reads 
done 


