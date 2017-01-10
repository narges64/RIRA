# RIRA

Started with SSDSim, the simulator modifies the simulator and implement some kind of FTL, and timing model for the flash array. 

Compile: make 
How to run: 

./ssd page.parameters name #chip timescale trace_file syn_rd_ratio syn_size syn_interarrival plane_method 

example: 
./ssd page.parameters_MT29F64G08_base test 1 1 test_fake 0.1 16 100000 0 
