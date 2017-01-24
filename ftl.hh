#ifndef FTL_H
#define FTL_H 100000

#include "common.hh"
#include "garbage_collection.hh"
#include "flash.hh"
 
ssd_info *process( ssd_info *);
sub_request * create_sub_request( ssd_info * ssd, int lpn,int size,uint64_t state, request * req,unsigned int operation);
void services_2_gc(ssd_info * ssd, unsigned int channel, unsigned int * channel_busy_flag); 
void services_2_io(ssd_info * ssd, unsigned int channel, unsigned int * channel_busy_flag); 
int find_lun_io_requests(ssd_info * ssd, unsigned int channel, unsigned int lun, sub_request ** subs, int * operation); 
int find_lun_gc_requests(ssd_info * ssd, unsigned int channel, unsigned int lun, sub_request ** subs, int * operation); 
void find_location(ssd_info *ssd,int ppn, local * location);
int find_ppn(ssd_info * ssd, const local * location); 
void add_write_to_table(ssd_info * ssd, request * request1); 
void full_sequential_write(ssd_info * ssd);
STATE get_ppn(ssd_info *ssd, local * location, int lpn, int & ppn);
STATE allocate_location( ssd_info * ssd , local * location);
uint64_t set_entry_state(ssd_info *ssd, int lsn,unsigned int size);
STATE  find_active_block( ssd_info *ssd,const local * location);
unsigned int get_target_lun(ssd_info * ssd); 
unsigned int get_target_plane(ssd_info * ssd, unsigned int channel, unsigned int lun); 
int write_page( ssd_info *ssd,local * location, int *ppn);
void full_sequential_write(ssd_info * ssd);
void full_random_write(ssd_info * ssd);

#endif

