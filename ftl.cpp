#include "ftl.hh"
sub_request * create_sub_request( ssd_info * ssd, int lpn,int size,uint64_t state, request * req,unsigned int operation){
	sub_request* sub=NULL,* sub_r=NULL;
	channel_info * p_ch=NULL;
	unsigned int flag=0;
	sub = new sub_request(ssd->current_time); 	
	if(req!=NULL)
	{
		sub->next_subs = req->subs;
		req->subs = sub;
	}
	sub->seq_num = ssd->subrequest_sequence_number++; 
	if (operation == READ)
	{
		find_location(ssd,ssd->dram->map->map_entry[lpn].pn, sub->location);
		sub->begin_time = ssd->current_time;  	
		sub->lpn = lpn;
		sub->size=size;            
		if (req != NULL){
			sub->app_id = req->app_id;
			sub->io_num = req->io_num;
		}
		for (int i = 0; i < 10; i++){
			sub->state_time[i] = 0;
		}
		if (req != NULL){
			sub->state_time[SR_MODE_WAIT] = ssd->current_time - req->time;
			sub->state_current_time = ssd->current_time;
		}
		else {
			sub->state_time[SR_MODE_WAIT] = 0;
			sub->state_current_time = ssd->current_time;
		}
		sub->ppn = ssd->dram->map->map_entry[lpn].pn;
		sub->operation = READ;
		sub->state=(ssd->dram->map->map_entry[lpn].state&0x7fffffffffffffff);
		if (ssd->channel_head[sub->location->channel]->lun_head[sub->location->lun]->rsubs_queue.find_subreq(sub)){ // the request already exists 
			sub->complete_time=ssd->current_time+1000;
			change_subrequest_state(ssd, sub,SR_MODE_ST_S,ssd->current_time,SR_MODE_COMPLETE,sub->complete_time); 
			sub->state_current_time = sub->next_state_predict_time; 
			ssd->channel_head[sub->location->channel]->read_count++; 
			ssd->channel_head[sub->location->channel]->lun_head[sub->location->lun]->read_count++;		
		} else {
			ssd->channel_head[sub->location->channel]->lun_head[sub->location->lun]->rsubs_queue.push_tail(sub); 
		} 
	}
	else if(operation == WRITE)
	{
		sub->ppn=0;
		sub->operation = WRITE;
		sub->lpn=lpn;
		sub->size=size;
		sub->state=state;
		sub->begin_time=ssd->current_time;
		if (req != NULL){
			sub->app_id = req->app_id;
			sub->io_num = req->io_num;
		}
		for (int i = 0; i < 10; i++){
			sub->state_time[i] = 0;
		}
		if (req != NULL){
			sub->state_time[SR_MODE_WAIT] = ssd->current_time - req->time;
			sub->state_current_time = ssd->current_time;
		}
		else {
			sub->state_time[SR_MODE_WAIT] = 0;
			sub->state_current_time = ssd->current_time;
		}
		if (get_ppn(ssd, sub->location, sub->lpn, sub->ppn) != SUCCESS) {
			printf("Error: could not get ppn \n"); 
		}  
		if (ssd->channel_head[sub->location->channel]->lun_head[sub->location->lun]->wsubs_queue.find_subreq(sub)){ 
			sub->complete_time=ssd->current_time+1000;
			change_subrequest_state(ssd,sub,SR_MODE_ST_S,ssd->current_time,SR_MODE_COMPLETE, sub->complete_time); 
			ssd->channel_head[sub->location->channel]->program_count++; 
			ssd->channel_head[sub->location->channel]->lun_head[sub->location->lun]->program_count++; 
		}else { 
			ssd->channel_head[sub->location->channel]->lun_head[sub->location->lun]->wsubs_queue.push_tail(sub); 
		}
	}
	else
	{
		delete sub; 
		printf("\nERROR ! Unexpected command.\n");
		return NULL;
	}
	
	return sub;
}
ssd_info *process( ssd_info *ssd)   {
	ProcessGC(ssd);
	// use some random to stop always prioritizing a single channel (channel 0) 
	int random = rand(); 
	for(int chan=0;chan<ssd->parameter->channel_number;chan++)	     
	{
		int c = ( chan + random ) % ssd->parameter->channel_number; 
		unsigned int flag=0;
		if(find_channel_state(ssd, c) == CHANNEL_MODE_IDLE)
		{
			services_2_io(ssd, c, &flag); 
			if(flag == 0)
				services_2_gc(ssd, c, &flag); 
		}
	}
	return ssd;
}

void services_2_io(ssd_info * ssd, unsigned int channel, unsigned int * channel_busy_flag){
	int64_t read_transfer_time = 7 * ssd->parameter->time_characteristics.tWC + 
			(ssd->parameter->subpage_page * ssd->parameter->subpage_capacity) * 
			ssd->parameter->time_characteristics.tRC; 
	int64_t read_time = ssd->parameter->time_characteristics.tR; 
	int64_t write_transfer_time = 7 * ssd->parameter->time_characteristics.tWC + 
			(ssd->parameter->subpage_page * ssd->parameter->subpage_capacity) * 
			ssd->parameter->time_characteristics.tWC; 
	int64_t write_time = ssd->parameter->time_characteristics.tPROG;
	int64_t channel_busy_time = 0; 
	int64_t lun_busy_time = 0; 	
	sub_request ** subs; 
	unsigned int subs_count = 0; 
	unsigned int max_subs_count = ssd->parameter->plane_lun; 
	subs = new sub_request *[max_subs_count]; 
	unsigned int lun; 
	int random = rand(); 
	for (unsigned int c = 0; c < ssd->channel_head[channel]->lun_num; c++){
		for (int i = 0; i < max_subs_count; i++) subs[i] = NULL; 
		subs_count = 0; 
		lun = (c + random ) % ssd->channel_head[channel]->lun_num; 
		if (find_lun_state(ssd , channel, lun) == LUN_MODE_IDLE && 
			ssd->channel_head[channel]->lun_head[lun]->GCMode == false){
			int operation = -1; 
			subs_count = find_lun_io_requests(ssd, channel, lun, subs, &operation); 
			if ( subs_count  == 0 ) continue; 	
			if ((subs_count < max_subs_count) && (ssd->parameter->plane_level_tech == IOGC)) 
				subs_count = find_lun_gc_requests(ssd, channel, lun, subs, &operation); 
			switch (operation){
				case READ: 
					if (subs_count > 1) ssd->read_multiplane_count++; 
					channel_busy_time = subs_count * read_transfer_time; 
					lun_busy_time = channel_busy_time + read_time; 
					break; 
				case WRITE: 
					if (subs_count > 1) ssd->write_multiplane_count++; 
					channel_busy_time = subs_count * write_transfer_time; 
					lun_busy_time = channel_busy_time + write_time; 
					break; 
				default: 
					cout << "Error: wrong operation (cannot be erase) " << endl; 
			}
	
			for (int i = 0; i < max_subs_count; i++){
				if (subs[i] == NULL) continue; 
				subs[i]->complete_time = ssd->current_time + lun_busy_time; 
				change_subrequest_state (ssd, subs[i], 
						SR_MODE_ST_S, ssd->current_time, 
						SR_MODE_COMPLETE , subs[i]->complete_time); 
				change_plane_state(ssd, subs[i]->location->channel, 
						subs[i]->location->lun, subs[i]->location->plane, 
						PLANE_MODE_IO, ssd->current_time, 
						PLANE_MODE_IDLE, subs[i]->complete_time); 
			}
			
			if (subs_count > 0){
				change_lun_state (ssd, channel, lun, LUN_MODE_IO, 
						ssd->current_time, LUN_MODE_IDLE, 
						ssd->current_time + lun_busy_time); 
				change_channel_state(ssd, channel, CHANNEL_MODE_IO, 
						ssd->current_time, CHANNEL_MODE_IDLE, 
						ssd->current_time + channel_busy_time);
				*channel_busy_flag = 1;
				break; 
			}
		}
	}
}

int find_lun_io_requests(ssd_info * ssd, unsigned int channel, unsigned int lun, sub_request ** subs, int * operation){
	int max_subs_count = ssd->parameter->plane_lun; 
	unsigned int page_offset = -1; 
	int subs_count = 0; 
	(*operation) = -1; 
	for (int i = 0; i < max_subs_count; i++){
		if (subs[i] == NULL) continue; 
		subs_count++; 
		(*operation) = subs[i]->operation; 
		page_offset = subs[i]->location->page; 
	}
	if ((*operation) == ERASE ) return subs_count; 
	
	for (unsigned plane = 0; plane < ssd->parameter->plane_lun; plane++){
		if (subs[plane] != NULL ) continue; 
		if ((*operation) == -1 || (*operation) == READ ) {
			subs[plane] = ssd->channel_head[channel]->lun_head[lun]->rsubs_queue.target_request(plane, -1, page_offset); 
			if (subs[plane] != NULL) {
				ssd->channel_head[channel]->lun_head[lun]->rsubs_queue.remove_node(subs[plane]);
				page_offset = subs[plane]->location->page; 
				subs_count++; 
				(*operation) = subs[plane]->operation; 
			}
		}
	}

	if ((*operation) == READ) return subs_count;
	
	for (unsigned plane = 0; plane < ssd->parameter->plane_lun; plane++){
		if (subs[plane] != NULL ) continue; 
		
		if ((*operation) == -1 || (*operation) == WRITE ){
			subs[plane] = ssd->channel_head[channel]->lun_head[lun]->wsubs_queue.target_request(plane, -1, page_offset); 
			if (subs[plane] != NULL) {
				ssd->channel_head[channel]->lun_head[lun]->wsubs_queue.remove_node(subs[plane]);
				page_offset = subs[plane]->location->page;
				subs_count++; 
				(*operation) = subs[plane]->operation;  
			}	
		}	
	}
	return subs_count; 		
}

void services_2_gc(ssd_info * ssd, unsigned int channel, unsigned int * channel_busy_flag){	
	int64_t read_transfer_time = 7 * ssd->parameter->time_characteristics.tWC + (ssd->parameter->subpage_page * ssd->parameter->subpage_capacity) * ssd->parameter->time_characteristics.tRC; 
	int64_t read_time = ssd->parameter->time_characteristics.tR; 
	int64_t write_transfer_time = 7 * ssd->parameter->time_characteristics.tWC + (ssd->parameter->subpage_page * ssd->parameter->subpage_capacity) * ssd->parameter->time_characteristics.tWC; 
	int64_t write_time = ssd->parameter->time_characteristics.tPROG;
	int64_t erase_transfer_time = 5 * ssd->parameter->time_characteristics.tWC;
	int64_t erase_time = ssd->parameter->time_characteristics.tBERS; 
	
	sub_request ** subs = new sub_request * [ssd->parameter->plane_lun];

	int subs_count = 0;  
	int random = rand() % ssd->channel_head[channel]->lun_num; 
	unsigned int lun = 0; 
	for (unsigned int c = 0; c < ssd->channel_head[channel]->lun_num; c++){
		for (int i = 0; i < ssd->parameter->plane_lun; i++) subs[i] = NULL; 
		subs_count = 0; 	
		lun = (c + random) % ssd->channel_head[channel]->lun_num;

		int64_t lun_busy_time = 0; 
		int64_t channel_busy_time = 0; 
		if (find_lun_state(ssd , channel, lun) != LUN_MODE_IDLE ||  
			ssd->channel_head[channel]->lun_head[lun]->GCMode != true) continue; 
		
		int operation = -1; 
		subs_count = find_lun_gc_requests(ssd, channel, lun, subs, &operation); 
		if (subs_count == 0) continue; 
		if ((subs_count < 2) && (ssd->parameter->plane_level_tech == GCIO)) 
			subs_count = find_lun_io_requests(ssd, channel, lun, subs, &operation); 

		switch(operation){
			case READ: 
               			if (subs_count > 1) ssd->read_multiplane_count++; 
				channel_busy_time = read_transfer_time * subs_count;
				lun_busy_time = channel_busy_time + read_time;
				break; 
			case WRITE: 
                    		if (subs_count > 1) ssd->write_multiplane_count++; 
				channel_busy_time = write_transfer_time * subs_count; 
				lun_busy_time = channel_busy_time + write_time; 
				break; 
			case ERASE:
                    		if (subs_count > 1) ssd->erase_multiplane_count++; 
				channel_busy_time = erase_transfer_time * subs_count; 
				lun_busy_time = channel_busy_time + erase_time; 
				break;
			default: 
				cout << "Error in the operation: " << operation << endl; 	
		}

		*channel_busy_flag = 1;

		for (int i = 0; i < ssd->parameter->plane_lun; i++){ 
			if (subs[i] == NULL) continue; 
			subs[i]->complete_time = ssd->current_time + lun_busy_time; 
			change_subrequest_state(ssd, subs[i], 
				SR_MODE_ST_S, ssd->current_time, SR_MODE_COMPLETE , 
				ssd->current_time + lun_busy_time); 
			change_plane_state (ssd, subs[i]->location->channel, 
				subs[i]->location->lun, subs[i]->location->plane, 
				PLANE_MODE_GC, ssd->current_time, PLANE_MODE_IDLE, 
				ssd->current_time + lun_busy_time); 
			if (operation == ERASE){
				delete_gc_node(ssd, subs[i]->gc_node);
				erase_operation(ssd,subs[i]->location); 
			}
		}
		change_lun_state (ssd, channel, lun, LUN_MODE_GC, ssd->current_time , 
					LUN_MODE_IDLE, ssd->current_time + lun_busy_time); 
		change_channel_state(ssd, channel, CHANNEL_MODE_GC, ssd->current_time , 
					CHANNEL_MODE_IDLE, ssd->current_time + channel_busy_time); 	
	}
}

int find_lun_gc_requests(ssd_info * ssd, unsigned int channel, unsigned int lun, sub_request ** subs, int * operation){	
	int max_subs_count = ssd->parameter->plane_lun; 
	unsigned int page_offset = -1; 
	int subs_count = 0;  
	(*operation) = -1;  
	
	for (int i = 0; i < max_subs_count; i++){
		if (subs[i] != NULL) {
			subs_count++; 
			(*operation) = subs[i]->operation; 
			page_offset = subs[i]->location->page; 
		}
	}

	if (ssd->channel_head[channel]->lun_head[lun]->GCSubs.is_empty()) return subs_count;  
	for (int i = 0; i < max_subs_count; i++){ 
		if (subs[i] != NULL) continue; 
		sub_request * temp = ssd->channel_head[channel]->lun_head[lun]->GCSubs.target_request(i, -1, page_offset); 
		if (temp == NULL || ((*operation) != -1 && temp->operation != (*operation))) continue;  // can be improved 
		subs[i] = temp; 
		ssd->channel_head[channel]->lun_head[lun]->GCSubs.remove_node(temp);
		(*operation) = subs[i]->operation;
		subs_count++;
		if (ssd->parameter->plane_level_tech != GCGC) 
			return subs_count; // NOT FOR GCGC 
	}

	if ((subs_count < max_subs_count) && (ssd->channel_head[channel]->lun_head[lun]->GCMode == false)) {
		cout << "here is the problem " << endl;  
	} 
		
	return subs_count; 	
}

unsigned int get_target_lun(ssd_info * ssd){
	unsigned int target_lun = ssd->lun_token; 
	ssd->lun_token = (ssd->lun_token + 1) % ssd->parameter->lun_num; 
	return target_lun; 
}
unsigned int get_target_plane(ssd_info * ssd, unsigned int channel, unsigned int lun) {
	unsigned int target_plane = ssd->channel_head[channel]->lun_head[lun]->plane_token; 
	ssd->channel_head[channel]->lun_head[lun]->plane_token = 
			(ssd->channel_head[channel]->lun_head[lun]->plane_token + 1) % ssd->parameter->plane_lun; 
	return target_plane; 
}  

STATE allocate_location( ssd_info * ssd , local * location){
	sub_request * update=NULL;
	unsigned int channel_num=0,lun_num=0,plane_num=0;
	if (location == NULL) location = new local(0,0,0); 
	channel_num = ssd->parameter->channel_number; 
	lun_num=ssd->parameter->lun_channel[0];
	plane_num=ssd->parameter->plane_lun;

	unsigned int target_lun = get_target_lun(ssd); 
	location->channel=target_lun / channel_num;
	location->lun= target_lun % channel_num; 
	location->plane= get_target_plane(ssd, location->channel, location->lun); 
	return SUCCESS; 
}	

STATE find_active_block( ssd_info *ssd,const local * location){
	STATE s = SUCCESS; 
	unsigned int count=0;
	unsigned int channel = location->channel; 
	unsigned int lun = location->lun; 
	unsigned int plane = location->plane; 
	unsigned int active_block=ssd->channel_head[channel]->lun_head[lun]->plane_head[plane]->active_block;
	unsigned int free_page_num=ssd->channel_head[channel]->lun_head[lun]->plane_head[plane]->blk_head[active_block]->free_page_num;
	while(((free_page_num==0))&&(count<ssd->parameter->block_plane))
	{
		active_block=(active_block+1)%ssd->parameter->block_plane;	
		free_page_num=ssd->channel_head[channel]->lun_head[lun]->plane_head[plane]->blk_head[active_block]->free_page_num;
		count++;
	}
	ssd->channel_head[channel]->lun_head[lun]->plane_head[plane]->active_block=active_block;
	if(count<ssd->parameter->block_plane) 
		return SUCCESS;
	else 	
		return FAIL; 
}


STATE get_ppn(ssd_info *ssd, local * location, int lpn, int & ppn){ 
	if (allocate_location(ssd, location) != SUCCESS) {
		printf("ERROR: could not allocate channel, lun, plane\n");
		return FAIL;  
	}  
	if (find_active_block(ssd, location) != SUCCESS){
		printf("ERROR:no free page in (%d,%d,%d)\n", location->channel, location->lun, location->plane);
		return FAIL;
	}
	unsigned int active_block = ssd->channel_head[location->channel]->lun_head[location->lun]->plane_head[location->plane]->active_block; 
	unsigned int last_write_page = ssd->channel_head[location->channel]->lun_head[location->lun]->plane_head[location->plane]->blk_head[active_block]->last_write_page+1; 
	if (last_write_page> ((int)(ssd->parameter->page_block) - 1))
	{
		printf("error! the last write page is %d larger than %d!!!!\n", last_write_page, ssd->parameter->page_block);
		return FAIL;
	}
	local * old_location = new local(0,0,0); 
	local * new_location = new local(location->channel, location->lun, location->plane);
	new_location->block = active_block;
	new_location->page = last_write_page; 
	uint64_t full_page=~(0xffffffffffffffff<<(ssd->parameter->subpage_page));
	if(ssd->dram->map->map_entry[lpn].state==0)    /*this is the first logical page*/
	{
		if(ssd->dram->map->map_entry[lpn].pn!=0)
		{
			printf("1. Error in get_ppn() %d \n ", ssd->dram->map->map_entry[lpn].pn);
			delete new_location;  
			return FAIL; 
		}
		ssd->dram->map->map_entry[lpn].pn=find_ppn(ssd,new_location);
		ssd->dram->map->map_entry[lpn].state=full_page; 
	}
	else                                                                         
	{
		int ppn=ssd->dram->map->map_entry[lpn].pn;
		find_location(ssd,ppn, old_location); // fill the old location 
		int old_stored_lpn = ssd->channel_head[old_location->channel]->lun_head[old_location->lun]->plane_head[old_location->plane]->blk_head[old_location->block]->page_head[old_location->page]->lpn; 
		if(old_stored_lpn!=lpn)
		{
			printf("\n2. Error in get_ppn() ppn: %d  lpn: %d (%d, %d, %d, %d, %d)\n", ppn, lpn, old_location->channel, old_location->lun, old_location->plane, old_location->block, old_location->page);
			delete new_location; 
			delete old_location; 
			return FAIL; 
		}
		ssd->channel_head[old_location->channel]->lun_head[old_location->lun]->plane_head[old_location->plane]->blk_head[old_location->block]->page_head[old_location->page]->valid_state=0;       
		ssd->channel_head[old_location->channel]->lun_head[old_location->lun]->plane_head[old_location->plane]->blk_head[old_location->block]->page_head[old_location->page]->free_state=0; 
		ssd->channel_head[old_location->channel]->lun_head[old_location->lun]->plane_head[old_location->plane]->blk_head[old_location->block]->page_head[old_location->page]->lpn=-1;
		ssd->channel_head[old_location->channel]->lun_head[old_location->lun]->plane_head[old_location->plane]->blk_head[old_location->block]->invalid_page_num++;
		ssd->dram->map->map_entry[lpn].pn=find_ppn(ssd,new_location);
		ssd->dram->map->map_entry[lpn].state=full_page;
	}
	ppn = ssd->dram->map->map_entry[lpn].pn; 
	ssd->channel_head[new_location->channel]->lun_head[new_location->lun]->plane_head[new_location->plane]->blk_head[new_location->block]->last_write_page++;
	ssd->channel_head[new_location->channel]->lun_head[new_location->lun]->plane_head[new_location->plane]->blk_head[new_location->block]->last_write_time = ssd->current_time;
	ssd->channel_head[new_location->channel]->lun_head[new_location->lun]->plane_head[new_location->plane]->blk_head[new_location->block]->free_page_num--;
	ssd->channel_head[new_location->channel]->lun_head[new_location->lun]->plane_head[new_location->plane]->free_page--;
	ssd->flash_prog_count++;                                                           
	ssd->channel_head[new_location->channel]->lun_head[new_location->lun]->plane_head[new_location->plane]->blk_head[new_location->block]->page_head[new_location->page]->written_count++;
	ssd->channel_head[new_location->channel]->lun_head[new_location->lun]->plane_head[new_location->plane]->blk_head[new_location->block]->page_head[new_location->page]->lpn=lpn;	
	ssd->channel_head[new_location->channel]->lun_head[new_location->lun]->plane_head[new_location->plane]->blk_head[new_location->block]->page_head[new_location->page]->valid_state=full_page;
	ssd->channel_head[new_location->channel]->lun_head[new_location->lun]->plane_head[new_location->plane]->blk_head[new_location->block]->page_head[new_location->page]->free_state=0;

	
	location->block = new_location->block; 
	location->page = new_location->page; 

	Schedule_GC(ssd, new_location);  // will check if required 
	
	delete old_location; 
	delete new_location; 
	
	return SUCCESS; 
}

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
		if (get_ppn(ssd,location, lpn, ppn) != SUCCESS){
			printf("ERROR: could not get ppn \n"); 
		} 
			
		ssd->dram->map->map_entry[lpn].pn=ppn;	
		ssd->dram->map->map_entry[lpn].state=set_entry_state(ssd,request1->lsn,sub_size);  
		ssd->channel_head[location->channel]->lun_head[location->lun]->plane_head[location->plane]->blk_head[location->block]->page_head[location->page]->lpn=lpn;
		ssd->channel_head[location->channel]->lun_head[location->lun]->plane_head[location->plane]->blk_head[location->block]->page_head[location->page]->valid_state=ssd->dram->map->map_entry[lpn].state;
		ssd->channel_head[location->channel]->lun_head[location->lun]->plane_head[location->plane]->blk_head[location->block]->page_head[location->page]->free_state=((~ssd->dram->map->map_entry[lpn].state)&full_page);	
		
		lsn=lsn+sub_size;                                         
		add_size+=sub_size;                                       
	}
}

int write_page( ssd_info *ssd, local * location, int *ppn)
{
	unsigned int channel = location->channel; 
	unsigned int lun = location->lun; 
	unsigned int plane = location->plane; 
	unsigned int active_block = location->block; 
	int last_write_page=0;
	last_write_page=++(ssd->channel_head[channel]->lun_head[lun]->plane_head[plane]->blk_head[active_block]->last_write_page);	
	if(last_write_page>=(int)(ssd->parameter->page_block))
	{
		ssd->channel_head[channel]->lun_head[lun]->plane_head[plane]->blk_head[active_block]->last_write_page=0;
		printf("error! the last write page larger than %d!\n", (int)(ssd->parameter->page_block));
		return ERROR;
	}
	ssd->channel_head[channel]->lun_head[lun]->plane_head[plane]->blk_head[active_block]->free_page_num--; 
	ssd->channel_head[channel]->lun_head[lun]->plane_head[plane]->free_page--;
	ssd->channel_head[channel]->lun_head[lun]->plane_head[plane]->blk_head[active_block]->page_head[last_write_page]->written_count++;
	ssd->flash_prog_count++; 
	location->page = last_write_page; 
	*ppn=find_ppn(ssd,location);
	cout << "channel, lun, plane " << channel << "  " << lun << " " << plane << " free: " << ssd->channel_head[channel]->lun_head[lun]->plane_head[plane]->free_page << endl; 
	return SUCCESS;
}

void full_sequential_write(ssd_info * ssd){
	unsigned int total_size = ssd->parameter->lun_num * ssd->parameter->plane_lun * ssd->parameter->block_plane * ssd->parameter->page_block * ssd->parameter->subpage_page; 
	total_size = total_size * (1-ssd->parameter->overprovide); 
	printf("full sequential write for total size %d \n", total_size );
	// create a write request 
	request * request1 = new request();
	request1->time = 0; 
	request1->lsn = 0; 
	request1->size = total_size; 
	request1->app_id = 1000; 
	request1->io_num = 0;
	request1->operation = WRITE; 
	request1->begin_time = 0; 
	request1->response_time = 0;
	request1->next_node = NULL;
	request1->subs = NULL;
	request1->need_distr_flag = NULL;
	request1->complete_lsn_count = 0;         //record the count of lsn served by buffer
	// add write to table 
	add_write_to_table(ssd, request1); 
	printf("\nfull sequential write is complete!\n");
}

void full_random_write(ssd_info * ssd) {
	unsigned int total_size = ssd->parameter->lun_num * ssd->parameter->plane_lun * ssd->parameter->block_plane * ssd->parameter->page_block * ssd->parameter->subpage_page; 
	total_size = total_size * (1-ssd->parameter->overprovide); 	
	printf("full random write for total size %d \n", total_size );
	for (int i = 0; i < total_size / ssd->parameter->subpage_page; i++){
		request * request1 = new request(); 
		request1->time = 0; 
		request1->lsn = rand() % total_size; 
		request1->size = 16; 
		request1->app_id = 1000; 
		request1->io_num = 0; 
		request1->operation = WRITE; 
		request1->begin_time = 0; 
		request1->response_time = 0; 
		request1->next_node = NULL; 
		request1->subs = NULL; 
		request1->need_distr_flag = NULL; 
		request1->complete_lsn_count = 0; 
		// add write to table 
		add_write_to_table(ssd, request1); 
	}	
	printf("\nfull random write is complete \n"); 
}
