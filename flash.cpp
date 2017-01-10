#include "flash.hh"
unsigned int find_subrequest_state(ssd_info * ssd, sub_request * sub){
	
	if (sub->next_state_predict_time <= ssd->current_time)
		return sub->next_state; 
	return sub->current_state; 
	
}
unsigned int find_lun_state (ssd_info * ssd, unsigned int channel, unsigned int lun){
	lun_info * the_lun = &ssd->channel_head[channel].lun_head[lun]; 
	if (the_lun->next_state_predict_time <= ssd->current_time) 
		return the_lun->next_state; 
	return the_lun->current_state; 
}
void change_lun_state (ssd_info * ssd, unsigned int channel, unsigned int lun, unsigned int current_state, int64_t current_time, unsigned int next_state, int64_t next_time){

	// have been in the next state for some time 
	int state1 = ssd->channel_head[channel].lun_head[lun].current_state; 
	int state2 = ssd->channel_head[channel].lun_head[lun].next_state; 
	int64_t state1_time = ssd->channel_head[channel].lun_head[lun].current_time; 
	int64_t state2_time = ssd->channel_head[channel].lun_head[lun].next_state_predict_time; 

	ssd->channel_head[channel].lun_head[lun].state_time[state1] += ((state2_time - state1_time) > 0)? state2_time - state1_time: 0; 
	ssd->channel_head[channel].lun_head[lun].state_time[state2] += ((current_time - state2_time) > 0)? (current_time - state2_time): 0; 
	
	
	ssd->channel_head[channel].lun_head[lun].current_state=current_state;	
	ssd->channel_head[channel].lun_head[lun].current_time=current_time;
	
	ssd->channel_head[channel].lun_head[lun].next_state=next_state; 	
	ssd->channel_head[channel].lun_head[lun].next_state_predict_time=next_time;
	 
	
	
}
void change_lun_state(ssd_info * ssd, local * location, unsigned int current_state, int64_t current_time, unsigned int next_state, int64_t next_time){
	change_lun_state (ssd, location->channel, location->lun, current_state, current_time, next_state, next_time); 
}
unsigned int find_plane_state(ssd_info * ssd , unsigned int channel, unsigned int lun, unsigned int plane){
	plane_info * the_plane = &ssd->channel_head[channel].lun_head[lun].plane_head[plane]; 
	if (the_plane->next_state_predict_time <= ssd->current_time) 
		return the_plane->next_state; 
	return the_plane->current_state; 	
}
unsigned int find_plane_state(ssd_info * ssd, local * location){
	return find_plane_state(ssd, location->channel, location->lun, location->plane); 
	
}
void change_plane_state (ssd_info * ssd, unsigned int channel, unsigned int lun, unsigned int plane, unsigned int current_state, int64_t current_time, unsigned int next_state, int64_t next_time){

	int state1 = ssd->channel_head[channel].lun_head[lun].plane_head[plane].current_state; 
	int state2 = ssd->channel_head[channel].lun_head[lun].plane_head[plane].next_state; 
	int64_t state1_time = ssd->channel_head[channel].lun_head[lun].plane_head[plane].current_time; 
	int64_t state2_time = ssd->channel_head[channel].lun_head[lun].plane_head[plane].next_state_predict_time; 
	
	ssd->channel_head[channel].lun_head[lun].plane_head[plane].state_time[state1] += (state2_time - state1_time)>0? state2_time - state1_time : 0; 
	ssd->channel_head[channel].lun_head[lun].plane_head[plane].state_time[state2] += (current_time - state2_time)>0? current_time - state2_time : 0;  
	
	ssd->channel_head[channel].lun_head[lun].plane_head[plane].current_state=current_state;	
	ssd->channel_head[channel].lun_head[lun].plane_head[plane].current_time=current_time;
	                          
	ssd->channel_head[channel].lun_head[lun].plane_head[plane].next_state=next_state; 	
	ssd->channel_head[channel].lun_head[lun].plane_head[plane].next_state_predict_time=next_time;
 
	sub_request * rhead = ssd->channel_head[channel].lun_head[lun].rsubs_queue.queue_head; 
	while(rhead != NULL){
		if (rhead->location->plane == plane){
			if (current_state == PLANE_MODE_GC)
				change_subrequest_state (ssd, rhead, SR_MODE_GCC_S, current_time, SR_MODE_WAIT, next_time);
			else 
				change_subrequest_state (ssd, rhead, SR_MODE_IOC_S, current_time, SR_MODE_WAIT, next_time);
				 	
		}
		else {
			if (current_state == PLANE_MODE_GC) 
				change_subrequest_state(ssd, rhead, SR_MODE_GCC_O, current_time, SR_MODE_WAIT, next_time); 
			else
				change_subrequest_state(ssd, rhead, SR_MODE_IOC_O, current_time, SR_MODE_WAIT, next_time); 
		}
		rhead = rhead->next_node; 
	}	

	sub_request * whead = ssd->channel_head[channel].lun_head[lun].wsubs_queue.queue_head; 
	while(whead != NULL){
		if (whead->location->plane == plane){
			if (current_state == PLANE_MODE_GC)
				change_subrequest_state (ssd, whead, SR_MODE_GCC_S, current_time, SR_MODE_WAIT, next_time);
			else 
				change_subrequest_state (ssd, whead, SR_MODE_IOC_S, current_time, SR_MODE_WAIT, next_time);
				 	
		}
		else {
			if (current_state == PLANE_MODE_GC) 
				change_subrequest_state(ssd, whead, SR_MODE_GCC_O, current_time, SR_MODE_WAIT, next_time); 
			else
				change_subrequest_state(ssd, whead, SR_MODE_IOC_O, current_time, SR_MODE_WAIT, next_time); 
		}
		whead = whead->next_node; 
	}	
	sub_request * gchead = ssd->channel_head[channel].lun_head[lun].GCSubs.queue_head; 
	while(rhead != NULL){
		if (gchead->location->plane == plane){
			if (current_state == PLANE_MODE_GC)
				change_subrequest_state (ssd, gchead, SR_MODE_GCC_S, current_time, SR_MODE_WAIT, next_time);
			else 
				change_subrequest_state (ssd, gchead, SR_MODE_IOC_S, current_time, SR_MODE_WAIT, next_time);
				 	
		}
		else {
			if (current_state == PLANE_MODE_GC) 
				change_subrequest_state(ssd, gchead, SR_MODE_GCC_O, current_time, SR_MODE_WAIT, next_time); 
			else
				change_subrequest_state(ssd, gchead, SR_MODE_IOC_O, current_time, SR_MODE_WAIT, next_time); 
		}
		gchead = gchead->next_node; 
	}	

}
void change_plane_state (ssd_info * ssd, local * location, unsigned int current_state, int64_t current_time, unsigned int next_state, int64_t next_time){
	change_plane_state (ssd, location->channel, location->lun, location->plane, current_state, current_time, next_state, next_time); 
}
unsigned int find_channel_state(ssd_info * ssd, unsigned int channel){
	channel_info * the_channel = &ssd->channel_head[channel]; 
	if (the_channel->next_state_predict_time <= ssd->current_time) 
		return the_channel->next_state; 
	return the_channel->current_state; 	
}
void change_channel_state(ssd_info * ssd, unsigned int channel, unsigned int current_state, int64_t current_time, unsigned int next_state, int64_t next_time){
	
	int state1 = ssd->channel_head[channel].current_state; 
	int state2 = ssd->channel_head[channel].next_state; 
	int64_t state1_time = ssd->channel_head[channel].current_time; 
	int64_t state2_time = ssd->channel_head[channel].next_state_predict_time; 
	
	ssd->channel_head[channel].state_time[state1] += (state2_time - state1_time)>0? state2_time - state1_time : 0; 
	ssd->channel_head[channel].state_time[state2] += (current_time - state2_time)>0? current_time - state2_time : 0;  
	

	int prev_state = ssd->channel_head[channel].current_state;  
	ssd->channel_head[channel].state_time[prev_state] += current_time - ssd->channel_head[channel].current_time; 
	
	
	ssd->channel_head[channel].current_state=current_state;	
	ssd->channel_head[channel].current_time=current_time;
	
	ssd->channel_head[channel].next_state=next_state; 	
	ssd->channel_head[channel].next_state_predict_time=next_time; 
	
}
void change_channel_state (ssd_info * ssd, local * location, unsigned int current_state, int64_t current_time, unsigned int next_state, int64_t next_time){
	change_channel_state (ssd, location->channel, current_state, current_time, next_state, next_time); 
}
void change_subrequest_state(ssd_info * ssd, sub_request * sub, unsigned int current_state, int64_t current_time, unsigned int next_state , int64_t next_time){
	int state1 = sub->current_state; 
	int state2 = sub->next_state; 
	int64_t state1_time = sub->current_time; 
	int64_t state2_time = sub->next_state_predict_time; 
	
	sub->state_time[state1] += (state2_time - state1_time)>0? state2_time - state1_time : 0; 
	sub->state_time[state2] += (current_time - state2_time)>0? current_time - state2_time : 0;  

	
	sub->current_state = current_state; 
	sub->current_time = current_time; 
	sub->next_state = next_state; 
	sub->next_state_predict_time = next_time; 
}

sub_request * create_sub_request( ssd_info * ssd,unsigned int lpn,int size,uint64_t state, request * req,unsigned int operation){

	static int seq_number = 0; 

	sub_request* sub=NULL,* sub_r=NULL;
	channel_info * p_ch=NULL;
	unsigned int flag=0;


	sub = new sub_request(ssd->current_time); 	

	if(req!=NULL)
	{
		sub->next_subs = req->subs;
		req->subs = sub;
	}
	
	// NRAGES
	sub->seq_num = seq_number++; 
	
	if (operation == READ)
	{
		find_location(ssd,ssd->dram->map->map_entry[lpn].pn, sub->location);
		
		sub->begin_time = ssd->current_time;  
		
		sub->lpn = lpn;
		sub->size=size;            
		// NARGES 
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
		
		if (ssd->channel_head[sub->location->channel].lun_head[sub->location->lun].rsubs_queue.find_subreq(sub)){ // the request already exists 
			sub->complete_time=ssd->current_time+1000;

			change_subrequest_state(ssd, sub, SR_MODE_ST_S, ssd->current_time, SR_MODE_COMPLETE, sub->complete_time); 
			
			sub->state_current_time = sub->next_state_predict_time; 
			
			ssd->channel_head[sub->location->channel].read_count++; 
			ssd->channel_head[sub->location->channel].lun_head[sub->location->lun].read_count++;		
		} else {
			ssd->channel_head[sub->location->channel].lun_head[sub->location->lun].rsubs_queue.push_tail(sub); 
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

		if (allocate_location(ssd ,sub)==ERROR)
		{
			delete sub; 
			return NULL;
		}
	
		/*NEW*/	get_ppn(ssd, sub->location, sub); 
		if (ssd->channel_head[sub->location->channel].lun_head[sub->location->lun].wsubs_queue.find_subreq(sub)){ // the request already exists 
			sub->complete_time=ssd->current_time+1000;
			change_subrequest_state(ssd, sub, SR_MODE_ST_S, ssd->current_time, SR_MODE_COMPLETE, sub->complete_time); 

			ssd->channel_head[sub->location->channel].program_count++; 
			ssd->channel_head[sub->location->channel].lun_head[sub->location->lun].program_count++;		
			 
		}else { 
		
			ssd->channel_head[sub->location->channel].lun_head[sub->location->lun].wsubs_queue.push_tail(sub); 
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

#ifdef DEBUG
	printf("TAP enter process,  current time:%lld\n",ssd->current_time);
#endif
	
	ProcessGC(ssd);
	
	// use some random to stop always prioritizing a single channel (channel 0) 
	for(int chan=0;chan<ssd->parameter->channel_number;chan++)	     
	{
		unsigned int flag=0;

		if(find_channel_state(ssd, chan) == CHANNEL_MODE_IDLE)
		{                             
			services_2_io(ssd, chan, &flag); 
			if(flag == 0)
			{
				services_2_gc(ssd, chan, &flag); 		
			}
		}	
	}

	return ssd;
}

void services_2_io(ssd_info * ssd, unsigned int channel, unsigned int * channel_busy_flag){
	int64_t read_transfer_time = 7 * ssd->parameter->time_characteristics.tWC + (ssd->parameter->subpage_page * ssd->parameter->subpage_capacity) * ssd->parameter->time_characteristics.tRC; 
	int64_t read_time = ssd->parameter->time_characteristics.tR; 
	int64_t write_transfer_time = 7 * ssd->parameter->time_characteristics.tWC + (ssd->parameter->subpage_page * ssd->parameter->subpage_capacity) * ssd->parameter->time_characteristics.tWC; 
	int64_t write_time = ssd->parameter->time_characteristics.tPROG;
	int64_t channel_busy_time = 0; 
	int64_t lun_busy_time = 0; 
	
	sub_request ** subs; 
	unsigned int subs_count = 0; 
	unsigned int max_subs_count = ssd->parameter->plane_lun; 
	subs = new sub_request *[max_subs_count]; 
	
 	
	unsigned int lun; 
	int random = rand() % ssd->channel_head[channel].lun; 
	for (unsigned int c = 0; c < ssd->channel_head[channel].lun; c++){
		for (int i = 0; i < max_subs_count; i++) subs[i] = NULL; 
		subs_count = 0; 
		lun = (c + random ) % ssd->channel_head[channel].lun; 
		
		if (find_lun_state(ssd , channel, lun) == LUN_MODE_IDLE && ssd->channel_head[channel].lun_head[lun].GCMode == LUN_MODE_IO){
			
			int operation = -1; 
			subs_count = find_lun_io_requests(ssd, channel, lun, subs, &operation); 
			if ( subs_count  == 0 ) continue; // find those requests to the lun which can go in parallel (two-plane and interleave )	
			if ((subs_count < 2) && (ssd->parameter->plane_level_tech == IOGC)) 
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
				change_subrequest_state (ssd, subs[i], SR_MODE_ST_S, ssd->current_time, SR_MODE_COMPLETE , subs[i]->complete_time); 
				change_plane_state(ssd, subs[i]->location, PLANE_MODE_IO, ssd->current_time, PLANE_MODE_IDLE, subs[i]->complete_time); 
			}
			
			if (subs_count > 0){
				change_lun_state (ssd, channel, lun, LUN_MODE_IO, ssd->current_time, LUN_MODE_IDLE, ssd->current_time + lun_busy_time); 
				change_channel_state(ssd, channel, CHANNEL_MODE_IO, ssd->current_time, CHANNEL_MODE_IDLE, ssd->current_time + channel_busy_time);
					
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
			subs[plane] = ssd->channel_head[channel].lun_head[lun].rsubs_queue.target_request(plane, -1, page_offset); 
				
			if (subs[plane] != NULL) {
				ssd->channel_head[channel].lun_head[lun].rsubs_queue.remove_node(subs[plane]);
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
			subs[plane] = ssd->channel_head[channel].lun_head[lun].wsubs_queue.target_request(plane, -1, page_offset); 
			if (subs[plane] != NULL) {
				ssd->channel_head[channel].lun_head[lun].wsubs_queue.remove_node(subs[plane]);
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
	int random = rand() % ssd->channel_head[channel].lun; 
	unsigned int lun = 0; 
	for (unsigned int c = 0; c < ssd->channel_head[channel].lun; c++){
		for (int i = 0; i < ssd->parameter->plane_lun; i++) subs[i] = NULL; 
		subs_count = 0; 	
		lun = (c + random) % ssd->channel_head[channel].lun;

		int64_t lun_busy_time = 0; 
		int64_t channel_busy_time = 0; 
		if (find_lun_state(ssd , channel, lun) != LUN_MODE_IDLE ||  ssd->channel_head[channel].lun_head[lun].GCMode != LUN_MODE_GC) continue; 
		
		int operation = -1; 
		subs_count = find_lun_gc_requests(ssd, channel, lun, subs, &operation); //ssd->channel_head[channel].lun_head[lun].GCSubs.queue_head;
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
				// ssd->channel_head[channel].lun_head[lun].GCMode = LUN_MODE_IO; 
				break;
			default: 
				cout << "Error in the operation: " << operation << endl; 	
		}

		*channel_busy_flag = 1;

		for (int i = 0; i < ssd->parameter->plane_lun; i++){ 
			if (subs[i] == NULL) continue; 
			subs[i]->complete_time = ssd->current_time + lun_busy_time; 
			change_subrequest_state(ssd, subs[i], SR_MODE_ST_S, ssd->current_time, SR_MODE_COMPLETE , ssd->current_time + lun_busy_time); 
			change_plane_state (ssd, subs[i]->location, PLANE_MODE_GC, ssd->current_time, PLANE_MODE_IDLE, ssd->current_time + lun_busy_time); 
			if (operation == ERASE){
				delete_gc_node(ssd, subs[i]->gc_node);
				erase_operation(ssd,subs[i]->location); 
			}
		}
				
		change_lun_state (ssd, channel, lun, LUN_MODE_GC, ssd->current_time , LUN_MODE_IDLE, ssd->current_time + lun_busy_time); 
		change_channel_state(ssd, channel, CHANNEL_MODE_GC, ssd->current_time , CHANNEL_MODE_IDLE, ssd->current_time + channel_busy_time); 	
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

	if (ssd->channel_head[channel].lun_head[lun].GCSubs.is_empty()) return subs_count;  
	for (int i = 0; i < max_subs_count; i++){ 
		if (subs[i] != NULL) continue; 
		sub_request * temp = ssd->channel_head[channel].lun_head[lun].GCSubs.target_request(i, -1, page_offset); 
		if (temp == NULL || ((*operation) != -1 && temp->operation != (*operation))) continue;  // can be improved 
		subs[i] = temp; 
		ssd->channel_head[channel].lun_head[lun].GCSubs.remove_node(temp);
		(*operation) = subs[i]->operation;
		subs_count++;
		if (ssd->parameter->plane_level_tech != GCGC) 
			return subs_count; // NOT FOR GCGC 
	}

	if ((subs_count < max_subs_count) && (ssd->channel_head[channel].lun_head[lun].GCMode == LUN_MODE_IO)) {
		cout << "here is the problem " << endl;  
	} 
		
	return subs_count; 	
}




STATE allocate_location( ssd_info * ssd , sub_request *sub_req){
	 sub_request * update=NULL;
	 unsigned int channel_num=0,lun_num=0,plane_num=0;
	 local *location= new local(0,0,0); 
	
	channel_num = ssd->parameter->channel_number; 
	lun_num=ssd->parameter->lun_channel[0];
	plane_num=ssd->parameter->plane_lun;
 
	sub_req->location->channel=sub_req->lpn%channel_num;
	lun_num=ssd->parameter->lun_channel[sub_req->location->channel];
	sub_req->location->lun=(sub_req->lpn/channel_num)%(lun_num);
	sub_req->location->plane=(sub_req->lpn/(lun_num*channel_num))%plane_num;
		
	
	if (ssd->dram->map->map_entry[sub_req->lpn].state!=0)
	{            

		if ((sub_req->state&ssd->dram->map->map_entry[sub_req->lpn].state)!=ssd->dram->map->map_entry[sub_req->lpn].state)  
		{
		
			ssd->flash_read_count++;
			ssd->update_read_count++;
			update = new sub_request(ssd->current_time); 
								
			find_location(ssd,ssd->dram->map->map_entry[sub_req->lpn].pn, location);
			update->location=location;
			update->begin_time = ssd->current_time;
			
			
			update->state_current_time = ssd->current_time; 
			update->lpn = sub_req->lpn;
		
			update->state=((ssd->dram->map->map_entry[sub_req->lpn].state^sub_req->state)&0x7fffffffffffffff);
			update->size=size(update->state);
			update->ppn = ssd->dram->map->map_entry[sub_req->lpn].pn;
			update->operation = READ;
			// NARGES 
			update->app_id = sub_req->app_id; 
			update->io_num = sub_req->io_num;
				
			ssd->channel_head[location->channel].lun_head[location->lun].rsubs_queue.push_tail(update); 
			
			printf("ERROR: FIXME update needs to be fixed \n"); 
		}

		if (update!=NULL)
		{
		
			sub_req->update=update;

			sub_req->state=(sub_req->state|update->state);
			sub_req->size=size(sub_req->state);
		}

	}


	return SUCCESS; 
}	

STATE find_active_block( ssd_info *ssd,const local * location){

	STATE s = SUCCESS; 
	unsigned int active_block;
	unsigned int free_page_num=0;
	unsigned int count=0;
	
	unsigned int channel = location->channel; 
	unsigned int lun = location->lun; 
	unsigned int plane = location->plane; 

	active_block=ssd->channel_head[channel].lun_head[lun].plane_head[plane].active_block;
	free_page_num=ssd->channel_head[channel].lun_head[lun].plane_head[plane].blk_head[active_block].free_page_num;
	unsigned int second_active_block;
	second_active_block = ssd->channel_head[channel].lun_head[lun].plane_head[plane].second_active_block;

	while(((free_page_num==0) || active_block == second_active_block)&&(count<ssd->parameter->block_plane))
	{
		active_block=(active_block+1)%ssd->parameter->block_plane;	
		free_page_num=ssd->channel_head[channel].lun_head[lun].plane_head[plane].blk_head[active_block].free_page_num;
		count++;
	}
	ssd->channel_head[channel].lun_head[lun].plane_head[plane].active_block=active_block;
	if(count<ssd->parameter->block_plane)
	{
		s = SUCCESS;
	}
	else
	{
		s = FAIL; 
	}

	free_page_num = ssd->channel_head[channel].lun_head[lun].plane_head[plane].blk_head[second_active_block].free_page_num;

	while ((free_page_num == 0 || second_active_block == active_block) && (count<ssd->parameter->block_plane))
	{
		second_active_block = (second_active_block + 1) % ssd->parameter->block_plane;
		free_page_num = ssd->channel_head[channel].lun_head[lun].plane_head[plane].blk_head[second_active_block].free_page_num;
		count++;
	}
	ssd->channel_head[channel].lun_head[lun].plane_head[plane].second_active_block = second_active_block;
	if (count<ssd->parameter->block_plane)
	{
		return s; 
	}
	else
	{
		return FAIL; 
	}


}


STATE get_ppn(ssd_info *ssd, const local * location, sub_request *sub){
	if (find_active_block(ssd, location) != SUCCESS){
		printf("ERROR :there is no free page in channel:%d, lun:%d, plane:%d\n", location->channel, location->lun, location->plane);
		return FAIL;
	}

	unsigned int active_block = ssd->channel_head[location->channel].lun_head[location->lun].plane_head[location->plane].active_block; 
	unsigned int last_write_page = ssd->channel_head[location->channel].lun_head[location->lun].plane_head[location->plane].blk_head[active_block].last_write_page+1; 
	
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
	
	int lpn=sub->lpn;

	if(ssd->dram->map->map_entry[lpn].state==0)                                       /*this is the first logical page*/
	{
		if(ssd->dram->map->map_entry[lpn].pn!=0)
		{
			printf("1. Error in get_ppn() %d \n ", ssd->dram->map->map_entry[lpn].pn);
			delete new_location;  
			return FAIL; 
		}
		ssd->dram->map->map_entry[lpn].pn=find_ppn(ssd,new_location);
		ssd->dram->map->map_entry[lpn].state=sub->state;
	}
	else                                                                         
	{
		 
		int ppn=ssd->dram->map->map_entry[lpn].pn;
		
		find_location(ssd,ppn, old_location); // fill the old location 

		int old_stored_lpn = ssd->channel_head[old_location->channel].lun_head[old_location->lun].plane_head[old_location->plane].blk_head[old_location->block].page_head[old_location->page].lpn; 

		if(	old_stored_lpn!=lpn)
		{
			printf("\n2. Error in get_ppn() ppn: %d  lpn: %d (%d, %d, %d, %d, %d) page stored lpn: %d, io_num %d \n", ppn, lpn, old_location->channel, old_location->lun, old_location->plane, old_location->block, old_location->page,old_stored_lpn , sub->io_num);
			delete new_location; 
			delete old_location; 
	
			return FAIL; 
		}
		ssd->channel_head[old_location->channel].lun_head[old_location->lun].plane_head[old_location->plane].blk_head[old_location->block].page_head[old_location->page].valid_state=0;       
		ssd->channel_head[old_location->channel].lun_head[old_location->lun].plane_head[old_location->plane].blk_head[old_location->block].page_head[old_location->page].free_state=0; 
		ssd->channel_head[old_location->channel].lun_head[old_location->lun].plane_head[old_location->plane].blk_head[old_location->block].page_head[old_location->page].lpn=-1;
		ssd->channel_head[old_location->channel].lun_head[old_location->lun].plane_head[old_location->plane].blk_head[old_location->block].invalid_page_num++;
		
		ssd->dram->map->map_entry[lpn].pn=find_ppn(ssd,new_location);
		ssd->dram->map->map_entry[lpn].state=(ssd->dram->map->map_entry[lpn].state|sub->state);
	}

	
	sub->ppn=ssd->dram->map->map_entry[lpn].pn;                                     
	sub->location->channel=new_location->channel;
	sub->location->lun=new_location->lun;
	sub->location->plane=new_location->plane;
	sub->location->block=new_location->block;
	sub->location->page=new_location->page;

	 
	ssd->channel_head[new_location->channel].lun_head[new_location->lun].plane_head[new_location->plane].blk_head[new_location->block].last_write_page++;
	ssd->channel_head[new_location->channel].lun_head[new_location->lun].plane_head[new_location->plane].blk_head[new_location->block].last_write_time = ssd->current_time;
	ssd->channel_head[new_location->channel].lun_head[new_location->lun].plane_head[new_location->plane].blk_head[new_location->block].free_page_num--;
	ssd->channel_head[new_location->channel].lun_head[new_location->lun].plane_head[new_location->plane].free_page--;

	ssd->flash_prog_count++;                                                           
	ssd->channel_head[new_location->channel].program_count++;
	ssd->channel_head[new_location->channel].lun_head[new_location->lun].program_count++;
	ssd->channel_head[new_location->channel].lun_head[new_location->lun].plane_head[new_location->plane].program_count++; 
	
																											
	ssd->channel_head[new_location->channel].lun_head[new_location->lun].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].lpn=lpn;	
	ssd->channel_head[new_location->channel].lun_head[new_location->lun].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].valid_state=sub->state;
	ssd->channel_head[new_location->channel].lun_head[new_location->lun].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].free_state=((~(sub->state))&full_page);
	ssd->channel_head[new_location->channel].lun_head[new_location->lun].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].written_count++;
	ssd->flash_prog_count++;

	Schedule_GC(ssd, new_location);  // will check if required 
	
	delete old_location; 
	delete new_location; 
	
	return SUCCESS; 
}

