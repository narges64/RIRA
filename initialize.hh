
#ifndef INITIALIZE_H
#define INITIALIZE_H 10000
#include <iostream>
using namespace std; 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include "common.hh"

class local;
class plane_info;
class direct_erase; 
class lun_ifo; 
class channel_info; 
class dram_info; 
class parameter_value; 
class lun_info; 
class blk_info; 
class map_info; 
class page_info;
class sub_request; 
class request; 
class gc_operation; 
class entry;   

ssd_info *initiation(ssd_info *, char **);
parameter_value *load_parameters(char parameter_file[30]);
page_info * initialize_page(page_info * p_page);
blk_info * initialize_block(blk_info * p_block,parameter_value *parameter);
plane_info * initialize_plane(plane_info * p_plane,parameter_value *parameter );
lun_info * initialize_lun(lun_info * p_lun,parameter_value *parameter,long long current_time );
ssd_info * initialize_channels(ssd_info * ssd );
dram_info * initialize_dram(ssd_info * ssd);

#endif
