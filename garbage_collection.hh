#ifndef GARBAGE_COLLECTION_H
#define GARBAGE_COLLECTION_H 10000

#include <sys/types.h>
#include "ssd.hh"
#include "flash.hh"
#include "common.hh"


//#define UTIL_CHAN_CURRENT  //fprintf(ssd->statisticfile2, "channel %d , state %d , time %lld\n", channel, ssd->channel_head[channel].current_state, ssd->channel_head[channel].current_time); 
//#define UTIL_CHAN_NEXT //fprintf(ssd->statisticfile2, "channel %d , state %d , time %lld\n", channel, ssd->channel_head[channel].next_state, ssd->channel_head[channel].next_state_predict_time);
//#define UTIL_CHAN_STATE  ssd->channel_head[channel].state_time[ssd->channel_head[channel].current_state] += ssd->channel_head[channel].next_state_predict_time - ssd->channel_head[channel].current_time; 

//#define UTIL_LUN_CURRENT  //fprintf(ssd->statisticfile2, "channel %d , lun %d , state %d , time %lld\n", channel, lun, ssd->channel_head[channel].lun_head[lun].current_state, ssd->channel_head[channel].lun_head[lun].current_time); 
//#define UTIL_LUN_NEXT fprintf(ssd->statisticfile2, "channel %d , lun %d , state %d , time %lld\n", channel, lun, ssd->channel_head[channel].lun_head[lun].next_state, ssd->channel_head[channel].lun_head[lun].next_state_predict_time); 
// #define UTIL_LUN_STATE ssd->channel_head[channel].lun_head[lun].state_time[ssd->channel_head[channel].lun_head[lun].current_state - 100] += ssd->channel_head[channel].lun_head[lun].next_state_predict_time - ssd->channel_head[channel].lun_head[lun].current_time; 
 
#define UTIL_PLANE_STATE ssd->channel_head[channel].lun_head[lun].plane_head[plane].state_time[plane_current_state] += ssd->channel_head[channel].lun_head[lun].next_state_predict_time - ssd->channel_head[channel].lun_head[lun].current_time; 


void plane_erase_observation(ssd_info *ssd, const local * location);
STATE find_victim_block( ssd_info *ssd,local * location); 
STATE greedy_algorithm(ssd_info * ssd, local * location);
STATE fifo_algorithm(ssd_info * ssd, local * location);
STATE windowed_algorithm(ssd_info * ssd, local * location);
STATE RGA_algorithm(ssd_info * ssd, local * location, int order);
STATE RANDOM_algorithm(ssd_info * ssd, local * location);
STATE RANDOM_p_algorithm(ssd_info * ssd, local * location);
STATE RANDOM_pp_algorithm(ssd_info * ssd, local * location);

unsigned int best_cost( ssd_info * ssd,  plane_info * the_plane, int * block_numbers, int number, int order /*which best, first best, second best, etc. */, int active_block /*not this one*/, int second_active_block);
STATE uninterrupt_gc( ssd_info *ssd, gc_operation * gc_node);

int64_t compute_moving_cost(ssd_info * ssd, const local * location, const local * twin_location, int approach); 
int erase_operation(ssd_info * ssd,const local * location);
bool update_priority(ssd_info * ssd, unsigned int channel, unsigned int lun); 
STATE move_page(ssd_info * ssd, const local * location, gc_operation * gc_node); 
void ProcessGC(ssd_info *ssd);
STATE add_gc_node(ssd_info * ssd, gc_operation * gc_node); 
int plane_emergency_state(ssd_info * ssd, const local * location); 
int get_ppn_for_gc(ssd_info *ssd, const local * location);
void update_subreq_state(ssd_info * ssd, const local * location, int64_t next_state_time); 
sub_request * create_gc_sub_request( ssd_info * ssd, const local * location, int operation, gc_operation * gc_node); 
bool Schedule_GC(ssd_info * ssd, local * location); 
STATE delete_gc_node(ssd_info * ssd, gc_operation * gc_node); 
#endif
