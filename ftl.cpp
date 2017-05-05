#include "ftl.hh"
#include <chrono>

void full_write_preconditioning(ssd_info * ssd, bool seq){
	unsigned int total_size = ssd->parameter->lun_num * ssd->parameter->plane_lun *
						ssd->parameter->block_plane * ssd->parameter->page_block;
	total_size = total_size * (1-ssd->parameter->overprovide);
	cerr << "full write for total size " << total_size << " page ";
	// add write to table
	int lpn = 0;
	for (int i = 0; i <  total_size; i++){

		if ( (invalid_old_page(ssd, lpn) != SUCCESS)  && !seq) 
			cout << "fail in invalid old page" << endl; 
		int ppn = get_new_ppn (ssd, lpn);
		write_page(ssd, lpn, ppn);
		bool gc = check_need_gc(ssd, ppn);
		if (gc) {
			if (seq) cerr << "should not have GC in sequential preconditioning " << endl;
			local * location = new local(0,0,0);
			find_location(ssd, ppn, location);
			pre_process_gc(ssd, location);
			delete location;
 		}
		if(seq) {
			lpn++;
			lpn = lpn % total_size; 
		}else
			lpn = rand() % total_size;
	}
	cerr << "is complete. erase count: " <<  ssd->stats->flash_erase_count
				<< ". move count: " << ssd->stats->gc_moved_page << endl;
	ssd->stats->flash_erase_count  = 0;
	ssd->stats->gc_moved_page = 0;
}

ssd_info * distribute(ssd_info *ssd){

	if (ssd->dram != NULL)
		ssd->dram->current_time=ssd->current_time;

	request * req=ssd->request_tail; // The request we want to distribute
	unsigned lsn=req->lsn;
	unsigned last_lpn=(req->lsn+req->size-1)/ssd->parameter->subpage_page;
	unsigned first_lpn=req->lsn/ssd->parameter->subpage_page;
	unsigned lpn = first_lpn;

	if(req->operation==READ)
	{
		while(lpn<=last_lpn)
		{
			uint64_t sub_state=(ssd->dram->map->map_entry[lpn].state&0x7fffffffffffffff);
			unsigned int sub_size=size(sub_state);
			sub_request * sub=create_sub_request(ssd,lpn,sub_size,sub_state,req,req->operation);
			if (service_in_buffer(ssd, sub) != SUCCESS)
				printf("Error in servicing a request in buffer \n");

			lpn++;
		}
	}
	else if(req->operation==WRITE)
	{
		while(lpn<=last_lpn)
		{
			uint64_t mask=~(0xffffffffffffffff<<(ssd->parameter->subpage_page));
			uint64_t state=mask;
			if(lpn==first_lpn)
			{
				uint64_t offset1=ssd->parameter->subpage_page-((lpn+1)*ssd->parameter->subpage_page-req->lsn);
				state=state&(0xffffffffffffffff<<offset1);

			}
			if(lpn==last_lpn)
			{
				uint64_t offset2=ssd->parameter->subpage_page-((lpn+1)*ssd->parameter->subpage_page-(req->lsn+req->size));
				state=state&(~(0xffffffffffffffff<<offset2));
			}

			unsigned int sub_size=size(state);
			if (sub_size > 32){
				cout << state << "***************** " << endl;
			}
			sub_request * sub=create_sub_request(ssd,lpn,sub_size,state,req,req->operation);
			if (service_in_buffer(ssd, sub) != SUCCESS)
				printf("Error in servicing write requset in buffer! \n");
			lpn++;
		}
	}else {
		// FIXME for TRIM
	}
	return ssd;
}
STATE service_in_buffer(ssd_info * ssd, sub_request * sub){
	// check mapping table and find the request
	if (sub->operation == READ){
		buffer_entry * buf_ent = NULL;
		if (ssd->parameter->dram_capacity == 0) {
			sub->buf_entry = NULL;
			service_in_flash(ssd, sub); 
			return SUCCESS; 
		}
			
		if (ssd->dram->map->map_entry[sub->lpn].buf_ent != NULL){
			buf_ent = ssd->dram->map->map_entry[sub->lpn].buf_ent;
			ssd->dram->buffer->hit_read(buf_ent);
			sub->complete_time = ssd->current_time + 1000;
			change_subrequest_state(ssd, sub,SR_MODE_ST_S,
				ssd->current_time,SR_MODE_COMPLETE,sub->complete_time);
			return SUCCESS;
		}else {
			service_in_flash(ssd, sub);
			return SUCCESS;
		}
	}
	else if (sub->operation == WRITE){
		if (ssd->parameter->dram_capacity == 0) {
			sub->buf_entry = NULL;
			service_in_flash(ssd, sub); 
			return SUCCESS; 
		}
		buffer_entry * buf_ent = NULL;
		if (ssd->dram->map->map_entry[sub->lpn].buf_ent == NULL) {
			if (ssd->dram->buffer->is_full()) {
				buf_ent = ssd->dram->buffer->add_head(sub->lpn);
				ssd->dram->map->map_entry[sub->lpn].buf_ent = buf_ent; 

				sub->buf_entry = buf_ent;
				buf_ent->sub = sub;
				buf_ent->outlier = true;   
				unsigned int full_page = ~(0xffffffff<<(ssd->parameter->subpage_page)); 
				sub_request * evict_sub = create_sub_request(ssd, sub->lpn, 
									ssd->parameter->subpage_page, full_page, 
									NULL, WRITE); 
				evict_sub->buf_entry = buf_ent; 
				service_in_flash(ssd, evict_sub); 

				return SUCCESS;
			}

			// Add a new entry to buffer
			buf_ent = ssd->dram->buffer->add_head(sub->lpn);
			ssd->dram->map->map_entry[sub->lpn].buf_ent = buf_ent;

			// Mark the request as complete
			sub->complete_time = ssd->current_time + 1000;
			change_subrequest_state(ssd, sub,SR_MODE_ST_S,ssd->current_time,
							SR_MODE_COMPLETE,sub->complete_time);

			sub->buf_entry = NULL;
			// create and issue evict buffer subrequest
			unsigned int full_page=~(0xffffffff<<(ssd->parameter->subpage_page));
			sub_request * evict_sub = create_sub_request(ssd, sub->lpn,
							ssd->parameter->subpage_page, full_page,
							NULL, WRITE);
			evict_sub->buf_entry = buf_ent;
		
			service_in_flash(ssd, evict_sub); 

	
			// invalid_old_page(ssd, sub->lpn); 
			// evict_sub->ppn = get_new_ppn(ssd, sub->lpn);
			// find_location(ssd,evict_sub->ppn, evict_sub->location);
			// write_page(ssd, evict_sub->lpn, evict_sub->ppn);

			//ssd->channel_head[evict_sub->location->channel]->
			//			lun_head[evict_sub->location->lun]->
			//			wsubs_queue.push_tail(evict_sub);

			//Schedule_GC(ssd, evict_sub->location);

			return SUCCESS;

		}else {	 // write to an entry in the buffer, just need to change the entry and make it dirty
			buf_ent = ssd->dram->map->map_entry[sub->lpn].buf_ent;
			ssd->dram->buffer->hit_write(buf_ent);
			sub->complete_time = ssd->current_time + 1000;
			change_subrequest_state(ssd, sub,SR_MODE_ST_S,
					ssd->current_time,SR_MODE_COMPLETE,sub->complete_time);
			sub->buf_entry = NULL;
			return SUCCESS;
		}

	}
	return FAIL;
}
void service_in_flash(ssd_info * ssd, sub_request * sub){
	if (sub->operation == READ) {
		if (ssd->channel_head[sub->location->channel]->
			lun_head[sub->location->lun]->
			rsubs_queue.find_subreq(sub)){
			// the request already exists
			sub->complete_time=ssd->current_time+1000;
			change_subrequest_state(ssd, sub,SR_MODE_ST_S,
					ssd->current_time,SR_MODE_COMPLETE,
					sub->complete_time);
			sub->state_current_time = sub->next_state_predict_time;
			ssd->stats->queue_read_count++;
		} else {
			ssd->channel_head[sub->location->channel]->
					lun_head[sub->location->lun]->
					rsubs_queue.push_tail(sub);
		}
	} else if (sub->operation == WRITE){
		invalid_old_page(ssd, sub->lpn);
		sub->ppn = get_new_ppn(ssd, sub->lpn);
		write_page(ssd, sub->lpn, sub->ppn);
		find_location(ssd, sub->ppn, sub->location);
		ssd->channel_head[sub->location->channel]->lun_head[sub->location->lun]->
			wsubs_queue.push_tail(sub);
		Schedule_GC(ssd, sub->location);
	}
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

void services_2_io(ssd_info * ssd, unsigned int channel,
																		unsigned int * channel_busy_flag){
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
	//		if ((subs_count < max_subs_count) && (ssd->parameter->plane_level_tech == IOGC))
		//		subs_count = find_lun_gc_requests(ssd, channel, lun, subs, &operation);
			switch (operation){
				case READ:
					if (subs_count > 1) ssd->stats->read_multiplane_count++;
					channel_busy_time = subs_count * read_transfer_time;
					lun_busy_time = channel_busy_time + read_time;

		//			ssd->channel_head[channel]->lun_head[lun]->
		//			update_stat(ssd->current_time , ssd->current_time +
		// 			lun_busy_time, READ, subs_count, ssd->parameter->subpage_page );
					break;
				case WRITE:
					if (subs_count > 1) ssd->stats->write_multiplane_count++;
					channel_busy_time = subs_count * write_transfer_time;
					lun_busy_time = channel_busy_time + write_time;
		//			ssd->channel_head[channel]->lun_head[lun]->
		//			update_stat(ssd->current_time, ssd->current_time +
		//			lun_busy_time, WRITE, subs_count, ssd->parameter->subpage_page);
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
				if (subs[i]->buf_entry != NULL){
					ssd->dram->map->map_entry[subs[i]->buf_entry->lpn].buf_ent = NULL;
					buffer_entry * buf_ent = subs[i]->buf_entry; 
					if (buf_ent->outlier && buf_ent->sub != NULL ){
						buf_ent->sub->complete_time = subs[i]->complete_time + 1000; 
						change_subrequest_state (ssd, buf_ent->sub, 
								SR_MODE_ST_S, ssd->current_time, 
								SR_MODE_COMPLETE, buf_ent->sub->complete_time);
						buf_ent->sub->buf_entry = NULL;  
					}
						 
					ssd->dram->buffer->remove_entry(buf_ent);
					sub_request * outlier_sub = ssd->dram->buffer->find_and_fix_outlier(); 
					if (outlier_sub != NULL) {
						outlier_sub->complete_time = subs[i]->complete_time; 
						change_subrequest_state(ssd, outlier_sub, 
								SR_MODE_ST_S, ssd->current_time, 
								SR_MODE_COMPLETE, outlier_sub->complete_time);
					} 
					subs[i]->buf_entry = NULL;
					delete subs[i];
					if (buf_ent != NULL)
						delete buf_ent;
				}
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
	delete [] subs;

}

int find_lun_io_requests(ssd_info * ssd, unsigned int channel,
				unsigned int lun, sub_request ** subs, int * operation){
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
			subs[plane] = ssd->channel_head[channel]->lun_head[lun]->
						rsubs_queue.target_request(plane, -1, page_offset);
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
			subs[plane] = ssd->channel_head[channel]->lun_head[lun]->
						wsubs_queue.target_request(plane, -1, page_offset);
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

void services_2_gc(ssd_info * ssd, unsigned int channel,unsigned int * channel_busy_flag) {

	int64_t read_transfer_time = 7 * ssd->parameter->time_characteristics.tWC +
					(ssd->parameter->subpage_page * ssd->parameter->subpage_capacity) *
					ssd->parameter->time_characteristics.tRC;
	int64_t read_time = ssd->parameter->time_characteristics.tR;
	int64_t write_transfer_time = 7 * ssd->parameter->time_characteristics.tWC +
					(ssd->parameter->subpage_page * ssd->parameter->subpage_capacity) *
					ssd->parameter->time_characteristics.tWC;
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
		//		if ((subs_count < 2) && (ssd->parameter->plane_level_tech == GCIO))
		//			subs_count = find_lun_io_requests(ssd, channel, lun, subs, &operation);

		switch(operation){
			case READ:
               			if (subs_count > 1) ssd->stats->read_multiplane_count++;
				channel_busy_time = read_transfer_time * subs_count;
				lun_busy_time = channel_busy_time + read_time;
		//		ssd->channel_head[channel]->lun_head[lun]->
		//		update_stat(ssd->current_time , ssd->current_time +
		//		lun_busy_time, NOOP_READ, subs_count, ssd->parameter->subpage_page);
				break;
			case WRITE:
                    		if (subs_count > 1) ssd->stats->write_multiplane_count++;
				channel_busy_time = write_transfer_time * subs_count;
				lun_busy_time = channel_busy_time + write_time;
		//		ssd->channel_head[channel]->lun_head[lun]->
		//		update_stat(ssd->current_time , ssd->current_time +
		//		lun_busy_time, NOOP_WRITE, subs_count, ssd->parameter->subpage_page);
				break;
			case ERASE:
                    		if (subs_count > 1) ssd->stats->erase_multiplane_count++;
				channel_busy_time = erase_transfer_time * subs_count;
				lun_busy_time = channel_busy_time + erase_time;
		//		ssd->channel_head[channel]->lun_head[lun]->
		//		update_stat(ssd->current_time , ssd->current_time  +
		//		lun_busy_time, NOOP, subs_count, ssd->parameter->subpage_page);
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
			if (subs[i]->buf_entry != NULL){
				buffer_entry * buf_ent  = subs[i]->buf_entry;
				buf_ent = ssd->dram->buffer->remove_entry(buf_ent);
				subs[i]->buf_entry = NULL;
				if (buf_ent != NULL)
					delete buf_ent;

				sub_request * outlier_sub = ssd->dram->buffer->find_and_fix_outlier(); 
				if (outlier_sub != NULL) {
					outlier_sub->complete_time = subs[i]->complete_time+1000; 
					change_subrequest_state(ssd, outlier_sub, 
							SR_MODE_ST_S, ssd->current_time, SR_MODE_COMPLETE,
							outlier_sub->complete_time);  
				}
			}
			if (operation == ERASE){
				delete_gc_node(ssd, subs[i]->gc_node);
				erase_block(ssd,subs[i]->location);
			}
			delete subs[i];
		}
		change_lun_state (ssd, channel, lun, LUN_MODE_GC, ssd->current_time ,
					LUN_MODE_IDLE, ssd->current_time + lun_busy_time);
		change_channel_state(ssd, channel, CHANNEL_MODE_GC, ssd->current_time ,
					CHANNEL_MODE_IDLE, ssd->current_time + channel_busy_time);
	}
	delete [] subs;
}

int find_lun_gc_requests(ssd_info * ssd, unsigned int channel, unsigned int lun,
						sub_request ** subs, int * operation) {
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
		sub_request * temp = ssd->channel_head[channel]->lun_head[lun]->
					GCSubs.target_request(i, -1, page_offset);
		if (temp == NULL ||
			((*operation) != -1 && temp->operation != (*operation)))
			continue;  // can be improved
		subs[i] = temp;
		ssd->channel_head[channel]->lun_head[lun]->GCSubs.remove_node(temp);
		(*operation) = subs[i]->operation;
		subs_count++;
		if (ssd->parameter->plane_level_tech != GCGC)
			return subs_count; // NOT FOR GCGC
	}

	if ((subs_count < max_subs_count) &&
			(ssd->channel_head[channel]->lun_head[lun]->GCMode == false))
	{
		cout << "here is the problem " << endl;
	}

	return subs_count;
}


// =============================================================================

sub_request * create_sub_request( ssd_info * ssd, int lpn,int size,
				uint64_t state, request * req,unsigned int operation){
	sub_request* sub=NULL,* sub_r=NULL;
	channel_info * p_ch=NULL;
	unsigned int flag=0;
	sub = new sub_request(ssd->current_time, lpn, size,
				ssd->subrequest_sequence_number++, operation);

	if(req!=NULL)
	{
		sub->next_subs = req->subs;
		req->subs = sub;

		sub->app_id = req->app_id;
		sub->io_num = req->io_num;

		sub->state_time[SR_MODE_WAIT] = ssd->current_time - req->time;
		sub->state_current_time = ssd->current_time;
	}
	else
	{
		sub->state_time[SR_MODE_WAIT] = 0;
		sub->state_current_time = ssd->current_time;
	}


	if (operation == READ)
	{
		sub->ppn  = ssd->dram->map->map_entry[lpn].pn;
		if (sub->ppn >= 0) {
			find_location(ssd,sub->ppn, sub->location);
			if (ssd->dram->map->map_entry[lpn].state)
				sub->state = 0x7fffffffffffffff;
			else
				sub->state = 0;
		}else {
			sub->state = state;
		}
	}
	else if(operation == WRITE)
	{
		sub->ppn = -1; // ssd->dram->map->map_entry[lpn].pn;
		sub->state=state;
	}
	else
	{
		delete sub;
		printf("\n FIXME TRIM command \n");
		return NULL;
	}

	return sub;
}

STATE write_page(ssd_info * ssd, const int lpn, const int  ppn){
	update_map_entry(ssd, lpn, ppn);
	update_physical_page(ssd, ppn, lpn);
	return SUCCESS;
}
void update_map_entry(ssd_info * ssd, int lpn, int ppn){
	bool full_page= true;
	if (ppn == -1) full_page = false;
	ssd->dram->map->map_entry[lpn].pn=ppn;
	ssd->dram->map->map_entry[lpn].state = full_page;
}
void update_physical_page(ssd_info * ssd, const int ppn, const int lpn){
	local * location = new local(0,0,0);
	find_location(ssd, ppn, location);

	int c = location->channel;
	int l = location->lun;
	int p = location->plane;
	int b = location->block;
	int pg = location->page;

	if (lpn != -1) {
		ssd->channel_head[c]->lun_head[l]->plane_head[p]->blk_head[b]->page_head[pg]->lpn=lpn;
		ssd->channel_head[c]->lun_head[l]->plane_head[p]->blk_head[b]->page_head[pg]->valid_state=true;

		ssd->channel_head[c]->lun_head[l]->plane_head[p]->free_page--;
		ssd->stats->flash_prog_count++;
		ssd->channel_head[c]->program_count++;
		ssd->channel_head[c]->lun_head[l]->program_count++;
		ssd->channel_head[c]->lun_head[l]->plane_head[p]->program_count++;
	}else {
		blk_info * block = ssd->channel_head[c]->lun_head[l]->plane_head[p]->blk_head[b];

		block->page_head[pg]->lpn=-1;
		if (block->page_head[pg]->valid_state)
			ssd->channel_head[c]->lun_head[l]->plane_head[p]->blk_head[b]->invalid_page_num++;
		block->page_head[pg]->valid_state=false;
	}
	delete location;

}
uint64_t set_entry_state(ssd_info *ssd, int lsn,unsigned int size){
	uint64_t temp,state,move;

	temp=~(0xffffffffffffffff<<size);
	move=lsn%ssd->parameter->subpage_page;
	state=temp<<move;

	return state;
}

STATE invalid_old_page(ssd_info * ssd, const int lpn){
	if (ssd->dram->map->map_entry[lpn].state == false) return FAIL;
	int old_ppn = ssd->dram->map->map_entry[lpn].pn;
	update_map_entry(ssd, lpn, -1);
	update_physical_page(ssd, old_ppn, -1);
	return SUCCESS;
}

int get_new_ppn(ssd_info *ssd, int lpn, const local * location){
	local * new_location;
	if (location != NULL) {
		new_location = new local(location->channel, location->lun, location->plane);
	}else {
		new_location = new local(0,0,0);
		allocate_plane(ssd, new_location);
	}
	if (allocate_page_in_plane(ssd, new_location) != SUCCESS){
		delete new_location;
		return -1;
	}
	int ppn = find_ppn(ssd, new_location);
	delete new_location;
	return ppn;
}
int get_target_lun(ssd_info * ssd){
	int target_lun = ssd->lun_token;
	ssd->lun_token = (ssd->lun_token + 1) % ssd->parameter->lun_num;
	return target_lun;
}
int get_target_plane(ssd_info * ssd, int channel, int lun) {
	int target_plane = ssd->channel_head[channel]->lun_head[lun]->plane_token;
	ssd->channel_head[channel]->lun_head[lun]->plane_token =
			(ssd->channel_head[channel]->lun_head[lun]->plane_token + 1) % ssd->parameter->plane_lun;
	return target_plane;
}
STATE allocate_plane( ssd_info * ssd , local * location){
	sub_request * update=NULL;
	unsigned int channel_num=0,lun_num=0,plane_num=0;
	if (location == NULL) location = new local(0,0,0);
	channel_num = ssd->parameter->channel_number;
	lun_num=ssd->parameter->lun_channel[0];
	plane_num=ssd->parameter->plane_lun;

	unsigned int target_lun = get_target_lun(ssd);
	location->channel=target_lun % channel_num;
	location->lun= (target_lun / channel_num) % ssd->parameter->lun_channel[location->channel];
	location->plane= get_target_plane(ssd, location->channel, location->lun);
	return SUCCESS;
}
int get_active_block(ssd_info *ssd, local * location){
	int channel = location->channel;
	int lun = location->lun;
	int plane = location->plane;

	int active_block = ssd->channel_head[channel]->lun_head[lun]->plane_head[plane]->active_block;
	int free_page_num = ssd->channel_head[channel]->lun_head[lun]->
					plane_head[plane]->blk_head[active_block]->free_page_num;
	int count=0;
	while(((free_page_num==0))&&(count<ssd->parameter->block_plane))
	{
		active_block=(active_block+1)%ssd->parameter->block_plane;
		free_page_num=ssd->channel_head[channel]->lun_head[lun]->
					plane_head[plane]->blk_head[active_block]->free_page_num;
		count++;
	}
	ssd->channel_head[channel]->lun_head[lun]->plane_head[plane]->active_block=active_block;

	return active_block;
}
STATE allocate_page_in_plane( ssd_info *ssd, local * location){
	int channel = location->channel;
	int lun = location->lun;
	int plane = location->plane;
	int active_block=get_active_block(ssd, location);
	int active_page = ssd->channel_head[channel]->lun_head[lun]->plane_head[plane]->
					blk_head[active_block]->last_write_page + 1;

	ssd->channel_head[channel]->lun_head[lun]->plane_head[plane]->blk_head[active_block]->last_write_page++;
	ssd->channel_head[channel]->lun_head[lun]->plane_head[plane]->blk_head[active_block]->
					last_write_time = ssd->current_time;
	ssd->channel_head[channel]->lun_head[lun]->plane_head[plane]->blk_head[active_block]->free_page_num--;
	location->block = active_block;
	location->page = active_page;

	return SUCCESS;
}
void find_location(ssd_info * ssd, int ppn, local * location){
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
int find_ppn(ssd_info * ssd,const local * location){
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

bool check_need_gc(ssd_info * ssd, int ppn){
	local * location = new local(0,0,0);
	find_location(ssd, ppn, location);

	int free_page = ssd->channel_head[location->channel]->
				lun_head[location->lun]->plane_head[location->plane]->free_page;
	int all_page = ssd->parameter->page_block*ssd->parameter->block_plane;

	if (free_page > (all_page * ssd->parameter->gc_down_threshold)){
		delete location;
		return false;
	}
	delete location;
	return true;
}
