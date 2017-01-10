/*****************************************************************************************************************************
This project was supported by the National Basic Research 973 Program of China under Grant No.2011CB302301
Huazhong University of Science and Technology (HUST)   Wuhan National Laboratory for Optoelectronics

FileName£º pagemap.h
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
#ifndef PAGEMAP_H
#define PAGEMAP_H 10000

#include <sys/types.h>
#include "initialize.hh"
#include "flash.hh"

#define MAX_INT64  0x7fffffffffffffffll


void find_location(ssd_info *ssd,int ppn, local * location);
int find_ppn(ssd_info * ssd, const local * location); 
int get_ppn_for_pre_process(ssd_info *ssd,unsigned int lsn, int app_id);
uint64_t set_entry_state(ssd_info *ssd,unsigned int lsn,unsigned int size);
void add_write_to_table(ssd_info * ssd, request * request1); 
request * get_next_write_request(ssd_info * ssd, int cd); 
int write_page( ssd_info *ssd,local * location, int *ppn);
void full_sequential_write(ssd_info * ssd);
#endif


