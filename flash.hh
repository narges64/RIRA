#ifndef FLASH_H
#define FLASH_H 100000

#include <stdlib.h>
#include "garbage_collection.hh"
#include "pagemap.hh"
#include "initialize.hh"

#define MAX(A,B) (A>B)?A:B;  

unsigned int find_plane_state(ssd_info * ssd , unsigned int channel, unsigned int lun, unsigned int plane); 
unsigned int find_lun_state (ssd_info * ssd, unsigned int channel, unsigned int lun);
unsigned int find_channel_state(ssd_info * ssd, unsigned int channel);  
unsigned int find_subrequest_state(ssd_info * ssd, sub_request * sub); 
unsigned int find_plane_state(ssd_info * ssd, local * location); 
void change_lun_state (ssd_info * ssd, unsigned int channel, unsigned int lun, unsigned int current_state, int64_t current_time, unsigned int next_state, int64_t next_time); 
void change_lun_state(ssd_info * ssd, local * location, unsigned int current_state, int64_t current_time, unsigned int next_state, int64_t next_time); 
void change_plane_state (ssd_info * ssd, unsigned int channel, unsigned int lun, unsigned int plane, unsigned int current_state, int64_t current_time, unsigned int next_state, int64_t next_time); 
void change_plane_state (ssd_info * ssd, local * location, unsigned int current_state, int64_t current_time, unsigned int next_state, int64_t next_time); 
void change_channel_state(ssd_info * ssd, unsigned int channel, unsigned int current_state, int64_t current_time, unsigned int next_state, int64_t next_time); 
void change_channel_state (ssd_info * ssd, local * location, unsigned int current_state, int64_t current_time, unsigned int next_state, int64_t next_time); 
void change_subrequest_state(ssd_info * ssd, sub_request * sub, unsigned int current_state, int64_t current_time, unsigned int next_state , int64_t next_time); 

ssd_info *process( ssd_info *);
ssd_info *insert2buffer( ssd_info *,unsigned int lpn,uint64_t state, sub_request *, request *);
sub_request * create_sub_request( ssd_info * ssd,unsigned int lpn,int size,uint64_t state, request * req,unsigned int operation);
STATE  find_active_block( ssd_info *ssd,const local * location);

STATE allocate_location( ssd_info * ssd , sub_request *sub_req);
STATE get_ppn(ssd_info *ssd,const local * location,sub_request *sub);

void services_2_gc(ssd_info * ssd, unsigned int channel, unsigned int * channel_busy_flag); 
void services_2_io(ssd_info * ssd, unsigned int channel, unsigned int * channel_busy_flag); 
int find_lun_io_requests(ssd_info * ssd, unsigned int channel, unsigned int lun, sub_request ** subs, int * operation); 
int find_lun_gc_requests(ssd_info * ssd, unsigned int channel, unsigned int lun, sub_request ** subs, int * operation); 

#endif

