#ifndef _COMMON_H_
#define _COMMON_H_ 10000

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include "initialize.hh"

class ssd_info; 

void file_assert(int error,const char *s);
void alloc_assert(void *p,const char *s);
void trace_assert(int64_t time_t,int device,unsigned int lsn,int size,int ope);
unsigned int size(uint64_t stored);
void my_assert_1(ssd_info * ssd, unsigned int channel, unsigned int lun, unsigned int plane);
void my_assert_2(ssd_info * ssd, unsigned int channel, unsigned int lun, unsigned int plane);


#endif
