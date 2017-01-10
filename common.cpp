#include "common.hh"


void file_assert(int error,const char *s){
	if(error == 0) return;
	printf("open %s error\n",s);
	getchar();
	exit(-1);
}


void trace_assert(int64_t time_t,int device,unsigned int lsn,int size,int ope)
{
	if(time_t <0 || device < 0  || size < 0 || ope < 0)
	{
		printf("trace error:%lld %d %d %d %d\n",time_t,device,lsn,size,ope);
		getchar();
		exit(-1);
	}
	if(time_t == 0 && device == 0 && lsn == 0 && size == 0 && ope == 0)
	{
		printf("probable read a blank line\n");
		getchar();
	}
}


unsigned int size(uint64_t stored){
	unsigned int i,total=0; 
	uint64_t mask=0x800000000000;

	for(i=1;i<=64;i++)
	{
		if(stored & mask) total++;
		stored<<=1;
	}

	return total;
}


void my_assert_1(ssd_info * ssd, unsigned int channel, unsigned int lun, unsigned int plane) {
	int free_page_sum = 0;
	for (int i = 0; i < ssd->parameter->block_plane; i++){
		free_page_sum += ssd->channel_head[channel].lun_head[lun].plane_head[plane].blk_head[i].free_page_num;
	}

	if (free_page_sum != ssd->channel_head[channel].lun_head[lun].plane_head[plane].free_page)
		printf("ASSSERRRT \n");
}

void my_assert_2(ssd_info * ssd, unsigned int channel, unsigned int lun, unsigned int plane){

	unsigned int twin_plane = (plane + 1) % ssd->parameter->plane_lun;
	plane_info * plane_0 = &ssd->channel_head[channel].lun_head[lun].plane_head[plane];
	plane_info * plane_1 = &ssd->channel_head[channel].lun_head[lun].plane_head[twin_plane];

	if (plane_0->blk_head[plane_0->second_active_block].last_write_page != plane_1->blk_head[plane_1->second_active_block].last_write_page){
		printf("Assert, second active block does'n work well for %d , %d  \n", channel, lun);
	}

}

