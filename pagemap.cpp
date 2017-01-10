#include "pagemap.hh"

//#define UTIL_CHAN_CURRENT  fprintf(ssd->statisticfile2, "channel %d , state %d , time %lld\n", channel, ssd->channel_head[channel].current_state, ssd->channel_head[channel].current_time); 
//#define UTIL_CHAN_NEXT fprintf(ssd->statisticfile2, "channel %d , state %d , time %lld\n", channel, ssd->channel_head[channel].next_state, ssd->channel_head[channel].next_state_predict_time);
//#define //UTIL_CHAN_STATE  ssd->channel_head[channel].state_time[ssd->channel_head[channel].current_state] += ssd->channel_head[channel].next_state_predict_time - ssd->channel_head[channel].current_time; 
//#define UTIL_LUN_CURRENT  fprintf(ssd->statisticfile2, "channel %d , lun %d , state %d , time %lld\n", channel, lun, ssd->channel_head[channel].lun_head[lun].current_state, ssd->channel_head[channel].lun_head[lun].current_time); 
//#define UTIL_LUN_NEXT fprintf(ssd->statisticfile2, "channel %d , lun %d , state %d , time %lld\n", channel, lun, ssd->channel_head[channel].lun_head[lun].next_state, ssd->channel_head[channel].lun_head[lun].next_state_predict_time); 
//#define //UTIL_LUN_STATE ssd->channel_head[channel].lun_head[lun].state_time[ssd->channel_head[channel].lun_head[lun].current_state - 100] += ssd->channel_head[channel].lun_head[lun].next_state_predict_time - ssd->channel_head[channel].lun_head[lun].current_time; 

void find_location(ssd_info *ssd,int ppn, local * location )
{	
	unsigned int i=0;
	int pn,ppn_value=ppn;
	int page_plane=0,page_lun=0,page_channel=0;

	pn = ppn;

	page_plane=ssd->parameter->page_block*ssd->parameter->block_plane;
	page_lun=page_plane*ssd->parameter->plane_lun;
	
	int page = ppn % (page_lun * ssd->parameter->lun_num); 
	location->channel = 0; 
	
	while (true){
		page_channel = page_lun * ssd->parameter->lun_channel[location->channel]; 
		page = page - page_channel; 
		if (page < 0){
			page = page + page_channel; 
			break; 
		}else {
			location->channel++; 
		}		
	}		
	
	
	location->lun = page/page_lun;
	location->plane = (page%page_lun)/page_plane;
	location->block = ((page%page_lun)%page_plane)/ssd->parameter->page_block;
	location->page = (((page%page_lun)%page_plane)%ssd->parameter->page_block)%ssd->parameter->page_block;

}
int find_ppn(ssd_info * ssd,const local * location)
{

	unsigned int channel = location->channel; 
	unsigned int lun = location->lun; 
	unsigned int plane = location->plane; 
	unsigned int block = location->block;
	unsigned int page = location->page; 

	int ppn=0;
	
	int page_plane=0,page_lun=0;
	int page_channel[100];                  
	
	page_plane=ssd->parameter->page_block*ssd->parameter->block_plane;
	page_lun=page_plane*ssd->parameter->plane_lun;
	
	unsigned int i=0;
	while(i<ssd->parameter->channel_number)
	{
		page_channel[i]=ssd->parameter->lun_channel[i]*page_lun;
		i++;
	}

   
	i=0;
	while(i<channel)
	{
		ppn=ppn+page_channel[i];
		i++;
	}
	ppn=ppn+page_lun*lun+page_plane*plane+block*ssd->parameter->page_block+page;
	
	return ppn;
}

uint64_t set_entry_state(ssd_info *ssd,unsigned int lsn,unsigned int size)
{
	uint64_t temp,state,move;

	temp=~(0xffffffffffffffff<<size);
	move=lsn%ssd->parameter->subpage_page;
	state=temp<<move;

	return state;
}


int get_ppn_for_pre_process(ssd_info *ssd,unsigned int lsn,int app_id)     
{
	unsigned int channel=0,lun=0,plane=0; 
	int ppn,lpn;
	unsigned int active_block;
	unsigned int channel_num=0,lun_num=0,plane_num=0;

#ifdef DEBUG
	printf("enter get_psn_for_pre_process\n");
#endif

	for (int i = 0; i < ssd->parameter->channel_number; i++){
		if (ssd->parameter->lun_channel[i] != 0)
			channel_num++;
	}
	
	//channel_num=ssd->parameter->channel_number;
	lun_num=ssd->parameter->lun_channel[0];
	plane_num=ssd->parameter->plane_lun; 
	lpn=lsn/ssd->parameter->subpage_page;

	channel=lpn%channel_num;
	lun_num=ssd->parameter->lun_channel[channel];
	lun=(lpn/channel_num)%(lun_num);
	plane=(lpn/(lun_num*channel_num))%plane_num;
	
	local * location = new local(channel, lun, plane);


	if(find_active_block(ssd,location)!= SUCCESS)
	{
		delete location; 
		printf("the read operation is expand the capacity of SSD, %d %d %d\n", channel, lun, plane);	
		return 0;
	}
	
	active_block=ssd->channel_head[channel].lun_head[lun].plane_head[plane].active_block;
	
	location->block = active_block; 

	if(write_page(ssd,location,&ppn)==ERROR)
	{
		delete location; 
		printf("error in write page \n");
		return 0;
	}
	
	delete location; 
	return ppn;
}

void add_write_to_table(ssd_info * ssd, request * request1){
	unsigned int size, add_size = 0;
	int sub_size,ppn,lpn, lsn; 	
	uint64_t map_entry_new,map_entry_old,modify;
	int flag=0;
	uint64_t full_page=~(0xffffffffffffffff<<(ssd->parameter->subpage_page));
	
	local *location = new local(0,0,0);
	
	lsn = request1->lsn; 
	size = request1->size; 
	while(add_size<size)
	{				
		
		sub_size=ssd->parameter->subpage_page-(request1->lsn%ssd->parameter->subpage_page);		
		if(add_size+sub_size>=size)                             
		{		
			sub_size=size-add_size;		
			add_size+=sub_size;		
		}
	
		if((sub_size>ssd->parameter->subpage_page)||(add_size>size))
		{		
			printf("pre_process sub_size:%d\n",sub_size);		
		}
	
		lpn=lsn/ssd->parameter->subpage_page;
		if(ssd->dram->map->map_entry[lpn].state==0)               
		{
			
			ppn=get_ppn_for_pre_process(ssd,lsn, request1->app_id); 
				
			find_location(ssd,ppn, location);
			ssd->flash_prog_count++;	
			ssd->channel_head[location->channel].program_count++;
			ssd->channel_head[location->channel].lun_head[location->lun].program_count++;		
			ssd->channel_head[location->channel].lun_head[location->lun].plane_head[location->plane].program_count++; 
			ssd->dram->map->map_entry[lpn].pn=ppn;	
			ssd->dram->map->map_entry[lpn].state=set_entry_state(ssd,request1->lsn,sub_size);  
			ssd->channel_head[location->channel].lun_head[location->lun].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn=lpn;
			ssd->channel_head[location->channel].lun_head[location->lun].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state=ssd->dram->map->map_entry[lpn].state;
			ssd->channel_head[location->channel].lun_head[location->lun].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state=((~ssd->dram->map->map_entry[lpn].state)&full_page);
			
		}
		else if(ssd->dram->map->map_entry[lpn].state>0)          
		{
			map_entry_new=set_entry_state(ssd,lsn,sub_size);      
			map_entry_old=ssd->dram->map->map_entry[lpn].state;
			modify=map_entry_new|map_entry_old;
			ppn=ssd->dram->map->map_entry[lpn].pn;
			find_location(ssd,ppn, location);
	
			ssd->flash_prog_count++;	
			ssd->channel_head[location->channel].program_count++;
			ssd->channel_head[location->channel].lun_head[location->lun].program_count++;			
			ssd->channel_head[location->channel].lun_head[location->lun].plane_head[location->plane].program_count++; 			
			ssd->dram->map->map_entry[lsn/ssd->parameter->subpage_page].state=modify; 
			ssd->channel_head[location->channel].lun_head[location->lun].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state=modify;
			ssd->channel_head[location->channel].lun_head[location->lun].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state=((~modify)&full_page);
			
	
		}
		lsn=lsn+sub_size;                                         
		add_size+=sub_size;                                       
	}
}

request * get_next_write_request(ssd_info * ssd, int cd){
	unsigned int device,lsn,size,ope,lpn; 
	
	unsigned int largest_lsn=(unsigned int )((ssd->parameter->lun_num*ssd->parameter->plane_lun*ssd->parameter->block_plane*ssd->parameter->page_block*ssd->parameter->subpage_page)*(1-ssd->parameter->overprovide));
	request * request1; 
	int64_t time;
	char buffer_request[200];
	
	while (fgets(buffer_request,200,ssd->tracefile[cd])){
	
		// UMASS format: IO # , Arrival Time (ns) , Device # , File Descriptor # , Access Type , Offset , Length , Application ID 
		int io_num, file_desc, app_id; 
		char * type = new char[5]; 
		sscanf(buffer_request,"%d %lu %d %d %s %d %d %d",&io_num,&time,&device,&file_desc,type,&lsn,&size,&app_id); 
		if (strcasecmp(type, "Read") == 0)
			ope = 1; 
		else 
			ope = 0; 
		
		time = time / ssd->parameter->time_scale; 
			
		//lsn = (lsn % ADDRESS_CHUNK) + (cd * ADDRESS_CHUNK); 	
		lsn=lsn%largest_lsn;      
		
		trace_assert(time,device,lsn,size,ope);                        
	                                                
	
		if(ope==1)   // Write request 
		{
			request1 = new request(); 
			request1->time = time; 
			request1->lsn = lsn;
			request1->size = size;
			request1->app_id = cd; 
			request1->io_num = io_num; 
			request1->operation = ope;	
			request1->begin_time = time; 
			request1->response_time = 0;	
			request1->energy_consumption = 0;	
			request1->next_node = NULL;
			request1->distri_flag = 0;              // indicate whether this request has been distributed already
			request1->subs = NULL;
			request1->need_distr_flag = NULL;
			request1->complete_lsn_count=0;         //record the count of lsn served by buffer
			
			return request1;
		}
	}

	return NULL; 	
}

int write_page( ssd_info *ssd, local * location, int *ppn)
{
	unsigned int channel = location->channel; 
	unsigned int lun = location->lun; 
	unsigned int plane = location->plane; 
	unsigned int active_block = location->block; 

	int last_write_page=0;
	last_write_page=++(ssd->channel_head[channel].lun_head[lun].plane_head[plane].blk_head[active_block].last_write_page);	
	if(last_write_page>=(int)(ssd->parameter->page_block))
	{
		ssd->channel_head[channel].lun_head[lun].plane_head[plane].blk_head[active_block].last_write_page=0;
		printf("error! the last write page larger than %d!\n", (int)(ssd->parameter->page_block));
		return ERROR;
	}
		
	ssd->channel_head[channel].lun_head[lun].plane_head[plane].blk_head[active_block].free_page_num--; 
	ssd->channel_head[channel].lun_head[lun].plane_head[plane].free_page--;
	ssd->channel_head[channel].lun_head[lun].plane_head[plane].blk_head[active_block].page_head[last_write_page].written_count++;
	ssd->flash_prog_count++; 

	location->page = last_write_page; 
	*ppn=find_ppn(ssd,location);

	return SUCCESS;
}

void full_sequential_write(ssd_info * ssd){

	unsigned int total_size = ssd->parameter->lun_num * ssd->parameter->plane_lun * ssd->parameter->block_plane * ssd->parameter->page_block * ssd->parameter->subpage_page; 
	total_size = total_size * (1-ssd->parameter->overprovide); 
	printf("full sequential write for total size %d \n", total_size );
	
	// create a write request 
	request * request1 = new request();
	//cout << "new request" << endl; 
	request1->time = 0; 
	request1->lsn = 0; 
	request1->size = total_size; 
	request1->app_id = 1000; 
	request1->io_num = 0;
	request1->operation = 1; 
	request1->begin_time = 0; 
	request1->response_time = 0;
	request1->energy_consumption = 0;
	request1->next_node = NULL;
	request1->distri_flag = 0;              // indicate whether this request has been distributed already
	request1->subs = NULL;
	request1->need_distr_flag = NULL;
	request1->complete_lsn_count = 0;         //record the count of lsn served by buffer


	// add write to table 

	add_write_to_table(ssd, request1); 


	printf("\n");
	printf("pre_process is complete!\n");

	for (int i = 0; i<ssd->parameter->lun_num; i++)
		for (int j = 0; j < ssd->parameter->plane_lun; j++)
		{
			fprintf(ssd->statisticfile, "lun:%d,plane:%d have free page: %d\n", i, j, ssd->channel_head[i%ssd->parameter->channel_number].lun_head[i / ssd->parameter->channel_number].plane_head[j].free_page);
			printf("lun:%d,plane:%d have free page: %d\n", i, j, ssd->channel_head[i%ssd->parameter->channel_number].lun_head[i / ssd->parameter->channel_number].plane_head[j].free_page);
			fflush(ssd->statisticfile);
		}
}
