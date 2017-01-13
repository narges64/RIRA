#ifndef FLASH_H
#define FLASH_H 100000

#include <stdlib.h>
#include "common.hh"
#include "garbage_collection.hh"

#define MAX(A,B) (A>B)?A:B;  

unsigned int find_plane_state(ssd_info * ssd , unsigned int channel, unsigned int lun, unsigned int plane); 
unsigned int find_lun_state (ssd_info * ssd, unsigned int channel, unsigned int lun);
unsigned int find_channel_state(ssd_info * ssd, unsigned int channel);  
unsigned int find_subrequest_state(ssd_info * ssd, sub_request * sub); 
void change_plane_state (ssd_info * ssd, unsigned int channel, unsigned int lun, unsigned int plane, unsigned int current_state, int64_t current_time, unsigned int next_state, int64_t next_time); 
void change_lun_state (ssd_info * ssd, unsigned int channel, unsigned int lun, unsigned int current_state, int64_t current_time, unsigned int next_state, int64_t next_time);
void change_channel_state(ssd_info * ssd, unsigned int channel, unsigned int current_state, int64_t current_time, unsigned int next_state, int64_t next_time); 
void change_subrequest_state(ssd_info * ssd, sub_request * sub, unsigned int current_state, int64_t current_time, unsigned int next_state , int64_t next_time); 
ssd_info *process( ssd_info *);
ssd_info *insert2buffer( ssd_info *,unsigned int lpn,uint64_t state, sub_request *, request *);
sub_request * create_sub_request( ssd_info * ssd,unsigned int lpn,int size,uint64_t state, request * req,unsigned int operation);
void services_2_gc(ssd_info * ssd, unsigned int channel, unsigned int * channel_busy_flag); 
void services_2_io(ssd_info * ssd, unsigned int channel, unsigned int * channel_busy_flag); 
int find_lun_io_requests(ssd_info * ssd, unsigned int channel, unsigned int lun, sub_request ** subs, int * operation); 
int find_lun_gc_requests(ssd_info * ssd, unsigned int channel, unsigned int lun, sub_request ** subs, int * operation); 
void find_location(ssd_info *ssd,int ppn, local * location);
int find_ppn(ssd_info * ssd, const local * location); 
int get_ppn_for_pre_process(ssd_info *ssd,unsigned int lsn, int app_id);
void add_write_to_table(ssd_info * ssd, request * request1); 
int write_page( ssd_info *ssd,local * location, int *ppn);
void full_sequential_write(ssd_info * ssd);
STATE get_ppn(ssd_info *ssd,const local * location,sub_request *sub);
STATE allocate_location( ssd_info * ssd , sub_request *sub_req);
uint64_t set_entry_state(ssd_info *ssd,unsigned int lsn,unsigned int size);
STATE  find_active_block( ssd_info *ssd,const local * location);
unsigned int get_target_lun(ssd_info * ssd); 
unsigned int get_target_plane(ssd_info * ssd, unsigned int channel, unsigned int lun); 

#endif

