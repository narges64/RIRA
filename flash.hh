/*****************************************************************************************************************************
This project was supported by the National Basic Research 973 Program of China under Grant No.2011CB302301
Huazhong University of Science and Technology (HUST)   Wuhan National Laboratory for Optoelectronics

FileName£º flash.h
Author: Hu Yang		Version: 2.1	Date:2011/12/02
Description: 

History:
<contributor>     <time>        <version>       <desc>                   <e-mail>
Yang Hu	        2009/09/25	      1.0		    Creat SSDsim       yanghu@foxmail.com
                2010/05/01        2.x           Change 
Zhiming Zhu     2011/07/01        2.0           Change               812839842@qq.com
Shuangwu Zhang  2011/11/01        2.1           Change               820876427@qq.com
Chao Ren        2011/07/01        2.0           Change               529517386@qq.com
Hao Luo         2011/01/01        2.0           Change               luohao135680@gmail.com
*****************************************************************************************************************************/

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

