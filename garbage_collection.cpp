#include "garbage_collection.hh"
#include "pagemap.hh"

void launch_gc_for_plane(ssd_info * ssd, gc_operation * gc_node){	
	if (gc_node == NULL) return; 
	unsigned int page_move_count = 0;  
	local * location = gc_node->location; 	
	if (find_victim_block(ssd, location) != SUCCESS) {
		printf("Error: invalid block selected for gc \n");
		return; 
	}
	for(unsigned int i=0;i<ssd->parameter->page_block;i++)
	{
		if(ssd->channel_head[location->channel].lun_head[location->lun].plane_head[location->plane].blk_head[location->block].page_head[i].valid_state>0) 		
		{
			location->page=i;
			if (move_page(ssd, location, gc_node) == FAIL){
				cout << "Error: problem while moving valid pages in GC " << endl; 
				return; 
			}
			page_move_count++;
		}
	}
	sub_request * erase_subreq = create_gc_sub_request( ssd, location, ERASE, gc_node); 
	ssd->channel_head[location->channel].lun_head[location->lun].GCSubs.push_tail(erase_subreq); 	
}

STATE gc_for_lun(ssd_info *ssd, unsigned int channel, unsigned int lun){	
	bool launch_all_planes = false; 
	bool on_demand = false; 
	for (unsigned int plane = 0; plane < ssd->parameter->plane_lun; plane++){
		gc_operation * gc_node = ssd->channel_head[channel].lun_head[lun].scheduled_gc[plane]; 
		if ((gc_node != NULL) && (gc_node->priority == GC_ONDEMAND)) on_demand = true;  	
	}
	
	if ((on_demand && (ssd->parameter->plane_level_tech == GCGC)) || (!on_demand)) launch_all_planes = true; 

	for (unsigned int plane = 0; plane < ssd->parameter->plane_lun; plane++){
		gc_operation * gc_node = ssd->channel_head[channel].lun_head[lun].scheduled_gc[plane]; 
		if (gc_node == NULL) continue; 
		if (launch_all_planes || (gc_node->priority == GC_ONDEMAND)) {
			launch_gc_for_plane(ssd, gc_node); 
			ssd->channel_head[channel].lun_head[lun].plane_head[plane].GCMode = PLANE_MODE_GC; 
		}
	}

	return SUCCESS; 
}
void ProcessGC(ssd_info *ssd){ // send LUNs to GC mode, or return back to IO mode 
	gc_operation * gc_node = NULL;
	
	for (unsigned int channel = 0; channel < ssd->parameter->channel_number; channel++){
		for (unsigned int lun = 0; lun < ssd->parameter->lun_channel[channel]; lun++){
			bool on_demand = update_priority (ssd, channel, lun );  
			 
			if ( (ssd->channel_head[channel].lun_head[lun].GCMode == LUN_MODE_IO) && 
				( on_demand ||  
					(ssd->channel_head[channel].lun_head[lun].rsubs_queue.is_empty() && 
					ssd->channel_head[channel].lun_head[lun].wsubs_queue.is_empty()))) {
 
				ssd->channel_head[channel].lun_head[lun].GCMode = LUN_MODE_GC; 
				gc_for_lun(ssd, channel, lun); 
			}
		}
	}
	
	for (unsigned int channel = 0; channel < ssd->parameter->channel_number; channel++){
		for (unsigned int lun = 0; lun < ssd->channel_head[channel].lun; lun++){
			if (ssd->channel_head[channel].lun_head[lun].GCMode == LUN_MODE_GC && 
				ssd->channel_head[channel].lun_head[lun].GCSubs.is_empty()){
				ssd->channel_head[channel].lun_head[lun].GCMode = LUN_MODE_IO;
				for (unsigned int plane = 0; plane < ssd->parameter->plane_lun; plane++)
					ssd->channel_head[channel].lun_head[lun].plane_head[plane].GCMode = PLANE_MODE_IO; 
			}
		}
	}
}
STATE find_victim_block(ssd_info * ssd, local * location){
 
	switch (ssd->parameter->gc_algorithm) {
		case GREEDY: 
				greedy_algorithm(ssd, location);
				
				break; 
		case FIFO:
				fifo_algorithm(ssd, location);
				
				break; 
		case WINDOWED: 
				windowed_algorithm(ssd, location); 
				
				break; 
		case RGA:
				RGA_algorithm(ssd, location, -1); // select the best one based on valid number 
				
				break; 
		case RANDOM: 
				RANDOM_algorithm(ssd, location); 
				
				break; 
		case RANDOMP:
				RANDOM_p_algorithm(ssd, location); 
				
				break; 
		case RANDOMPP:
				RANDOM_pp_algorithm(ssd, location); 
				
				break; 
		
		default:
		{
			printf("Wrong garbage collection in parameters %d\n", ssd->parameter->gc_algorithm);
			return ERROR; 
		}
	
	}
	if (location->block == -1){
		printf("Error: Problem in finding the victim block \n");
		return FAIL;

	}

	if (ssd->channel_head[location->channel].lun_head[location->lun].plane_head[location->plane].blk_head[location->block].free_page_num >= ssd->parameter->page_block){
		printf("Error: too much free page in selected victim \n");
		return FAIL; 
	}
	
	int free_page = 0; 
	for(int i=0;i<ssd->parameter->page_block;i++)
	{		
		if ((ssd->channel_head[location->channel].lun_head[location->lun].plane_head[location->plane].blk_head[location->block].page_head[i].free_state&PG_SUB)==0x00000000000f)
			free_page++;
	}
	if(free_page!=0){
		printf("Error: too much free page.\t%d\t%d\t%d\t%d\t\n",free_page,location->channel,location->lun,location->plane);
		return FAIL; 
	}	
	
	return SUCCESS; 
}
sub_request * create_gc_sub_request( ssd_info * ssd,const local * location, int operation, gc_operation * gc_node){
	static int seq_number = 0; 
	sub_request * sub = new sub_request(ssd->current_time); 
		
	sub->gc_node = gc_node;

	if (gc_node != NULL)	{
		if (location->channel != gc_node->location->channel || location->lun != gc_node->location->lun)
			cout << "Error in location and gc_node incompatible! " << endl; 
	}

	sub->seq_num = seq_number++; 
	if (operation != ERASE){
		sub->lpn = ssd->channel_head[location->channel].lun_head[location->lun].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn;
		sub->size= ssd->parameter->subpage_page;  
		sub->state=(ssd->dram->map->map_entry[sub->lpn].state&0x7fffffffffffffff);
	}
	sub->begin_time=ssd->current_time;
	if (operation == READ)
	{	
		sub->location = new local(location->channel, location->lun, location->plane,location->block, location->page); 
		sub->begin_time = ssd->current_time;
	
		sub->ppn = find_ppn(ssd, location);  // or use the map FIXME 
		
		if (sub->ppn != ssd->dram->map->map_entry[sub->lpn].pn )
			printf("Error ppn does not match the mapping table! \n"); 
		
		sub->operation = READ;
		
	}
	else if(operation == WRITE)
	{
		sub->operation = WRITE;
		sub->location = new local(location->channel, location->lun, location->plane); 
	
		sub->ppn= get_ppn_for_gc(ssd, location); 
		find_location(ssd, sub->ppn, sub->location); // fill the rest of location ??? need this? 
	
		ssd->dram->map->map_entry[sub->lpn].pn= sub->ppn;
        ssd->dram->map->map_entry[sub->lpn].state=(ssd->dram->map->map_entry[sub->lpn].state|sub->state);
	}
	else if (operation == ERASE)
	{
		sub->operation = ERASE; 
		sub->location = new local(location->channel, location->lun, location->plane, location->block, 0); 
		
	}else {
		delete sub; 
		printf("\nERROR ! Unexpected command.\n");
		return NULL;
	}
	
		
	return sub; 
}
STATE move_page(ssd_info * ssd,  const local * location, gc_operation * gc_node){

	sub_request * rsub = create_gc_sub_request(ssd, location, READ, gc_node); 
	sub_request * wsub = create_gc_sub_request(ssd, location, WRITE, gc_node); 
	
	ssd->channel_head[rsub->location->channel].lun_head[rsub->location->lun].GCSubs.push_tail(rsub);
	ssd->channel_head[wsub->location->channel].lun_head[wsub->location->lun].GCSubs.push_tail(wsub); 
	
	return SUCCESS;
}
int get_ppn_for_gc(ssd_info *ssd,const local *old_location){
	int ppn;
	unsigned int active_block,block,page;

	if(find_active_block(ssd,old_location)!=SUCCESS)
	{
		printf("\n\n Error in get_ppn_for_gc().\n");
		return -1; 
	}
    
	active_block=ssd->channel_head[old_location->channel].lun_head[old_location->lun].plane_head[old_location->plane].active_block;
	unsigned int last_write_page = ssd->channel_head[old_location->channel].lun_head[old_location->lun].plane_head[old_location->plane].blk_head[active_block].last_write_page+1; 
	
	if(last_write_page>=ssd->parameter->page_block )
	{
		printf("error! the last write page larger than %d!!!!!\n", ssd->parameter->page_block);
		return -1; 
	}
	int lpn = ssd->channel_head[old_location->channel].lun_head[old_location->lun].plane_head[old_location->plane].blk_head[old_location->block].page_head[old_location->page].lpn; 
	int valid_state = ssd->channel_head[old_location->channel].lun_head[old_location->lun].plane_head[old_location->plane].blk_head[old_location->block].page_head[old_location->page].valid_state; 
	int free_state  = ssd->channel_head[old_location->channel].lun_head[old_location->lun].plane_head[old_location->plane].blk_head[old_location->block].page_head[old_location->page].free_state; 
	
	ssd->channel_head[old_location->channel].lun_head[old_location->lun].plane_head[old_location->plane].blk_head[old_location->block].page_head[old_location->page].lpn = -1;	
	ssd->channel_head[old_location->channel].lun_head[old_location->lun].plane_head[old_location->plane].blk_head[old_location->block].page_head[old_location->page].valid_state = 0; 	
	ssd->channel_head[old_location->channel].lun_head[old_location->lun].plane_head[old_location->plane].blk_head[old_location->block].page_head[old_location->page].free_state = PG_SUB; 
	
	local * new_location = new local(old_location->channel, old_location->lun, old_location->plane); 
	
	new_location->block=active_block;	
	new_location->page=last_write_page;	

	ssd->channel_head[new_location->channel].lun_head[new_location->lun].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].lpn = lpn; 
	ssd->channel_head[new_location->channel].lun_head[new_location->lun].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].valid_state = valid_state; 	
	ssd->channel_head[new_location->channel].lun_head[new_location->lun].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].free_state = free_state; 
	
	ppn=find_ppn(ssd,new_location);
	ssd->channel_head[new_location->channel].lun_head[new_location->lun].plane_head[new_location->plane].blk_head[new_location->block].last_write_page++; 
	ssd->channel_head[new_location->channel].lun_head[new_location->lun].plane_head[new_location->plane].blk_head[new_location->block].free_page_num--;
	ssd->channel_head[new_location->channel].lun_head[new_location->lun].plane_head[new_location->plane].free_page--;

	ssd->flash_prog_count++;
	ssd->channel_head[new_location->channel].program_count++;
	ssd->channel_head[new_location->channel].lun_head[new_location->lun].program_count++;
	ssd->channel_head[new_location->channel].lun_head[new_location->lun].plane_head[new_location->plane].program_count++; 
	ssd->channel_head[new_location->channel].lun_head[new_location->lun].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].written_count++;

	delete new_location; 
	return ppn;

}
int erase_operation(ssd_info * ssd,const local * location){
	
	unsigned int channel = location->channel; 
	unsigned int lun = location->lun; 
	unsigned int plane = location->plane; 
	unsigned int block = location->block; 
	
	unsigned int i=0;
	
	int initial_free = ssd->channel_head[channel].lun_head[lun].plane_head[plane].blk_head[block].free_page_num; 
	ssd->channel_head[channel].lun_head[lun].plane_head[plane].blk_head[block].free_page_num=ssd->parameter->page_block;
	ssd->channel_head[channel].lun_head[lun].plane_head[plane].blk_head[block].invalid_page_num=0;
	ssd->channel_head[channel].lun_head[lun].plane_head[plane].blk_head[block].last_write_page=-1;
	ssd->channel_head[channel].lun_head[lun].plane_head[plane].blk_head[block].erase_count++;
	
	for (i=0;i<ssd->parameter->page_block;i++)
	{
		ssd->channel_head[channel].lun_head[lun].plane_head[plane].blk_head[block].page_head[i].free_state=PG_SUB;
		ssd->channel_head[channel].lun_head[lun].plane_head[plane].blk_head[block].page_head[i].valid_state=0;
		ssd->channel_head[channel].lun_head[lun].plane_head[plane].blk_head[block].page_head[i].lpn=-1;
	}
	
	ssd->flash_erase_count++;
	ssd->channel_head[channel].erase_count++;			
	ssd->channel_head[channel].lun_head[lun].erase_count++;
	ssd->channel_head[channel].lun_head[lun].plane_head[plane].erase_count++; 
	ssd->channel_head[channel].lun_head[lun].plane_head[plane].free_page += (ssd->parameter->page_block - initial_free);  
	
//	cout << "erase block " << channel << " "<< lun << " "<< plane << " "<< block << endl; 
	return SUCCESS;

}

bool update_priority(ssd_info * ssd, unsigned int channel, unsigned int lun){
	bool on_demand = false; 
	unsigned int all_page = ssd->parameter->page_block*ssd->parameter->block_plane;
	unsigned int free_page = 0;  
	gc_operation * gc_node = NULL; 
	for (int plane = 0; plane < ssd->parameter->plane_lun; plane++){ 
		free_page = ssd->channel_head[channel].lun_head[lun].plane_head[plane].free_page;
		gc_node = ssd->channel_head[channel].lun_head[lun].scheduled_gc[plane]; 
		if (gc_node == NULL) continue; 
		if (free_page > (all_page * ssd->parameter->gc_up_threshold)) {
			if (gc_node != NULL) 
				cout << "GC is no longer needed, FIXME  " << endl; 
			continue; 
		}	
		if (free_page < (all_page * ssd->parameter->gc_down_threshold)) {
			gc_node->priority = GC_ONDEMAND; 
			on_demand = true; 
		}else if (free_page < (all_page * ssd->parameter->gc_up_threshold)) gc_node->priority = GC_EARLY; 
	} 

	return on_demand; 
}

bool Schedule_GC(ssd_info * ssd, local * location){	
	unsigned int free_page = ssd->channel_head[location->channel].lun_head[location->lun].plane_head[location->plane].free_page; 
	unsigned int all_page = ssd->parameter->page_block*ssd->parameter->block_plane; 
	gc_operation * gc_node = NULL; 
	
	if (free_page > (all_page * ssd->parameter->gc_up_threshold)){
		return false; 
	}
	gc_node = new gc_operation(location); 
	
	if (free_page  < (all_page *ssd->parameter->gc_down_threshold)) gc_node->priority=GC_ONDEMAND;
	else if (free_page < (all_page * ssd->parameter->gc_up_threshold)) gc_node->priority=GC_EARLY;

	// Schedule GC 
	if (add_gc_node(ssd, gc_node) != SUCCESS){
		delete gc_node; 
		return false; 
	}
	
	return true; 
}
STATE add_gc_node(ssd_info * ssd, gc_operation * gc_node){
	if (gc_node == NULL) return FAIL; 
	gc_operation * gc = ssd->channel_head[gc_node->location->channel].lun_head[gc_node->location->lun].scheduled_gc[gc_node->location->plane]; 
	
	if (gc == NULL){
		ssd->channel_head[gc_node->location->channel].lun_head[gc_node->location->lun].scheduled_gc[gc_node->location->plane] = gc_node;
		return SUCCESS;  
	}
/*	
	for (unsigned int channel = 0; channel < ssd->parameter->channel_number; channel++){
		for (unsigned int lun = 0; lun < ssd->channel_head[channel].lun; lun++){
			if (ssd->channel_head[channel].lun_head[lun].GCMode == LUN_MODE_GC && ssd->channel_head[channel].lun_head[lun].GCSubs.is_empty()){
				ssd->channel_head[channel].lun_head[lun].GCMode = LUN_MODE_IO;
				for (unsigned int plane = 0; plane < ssd->parameter->plane_lun; plane++)
					ssd->channel_head[channel].lun_head[lun].plane_head[plane].GCMode = PLANE_MODE_IO; 
			}
		}
	}
*/	
	return FAIL; 
}
STATE delete_gc_node(ssd_info * ssd, gc_operation * gc_node){ 
	if (gc_node == NULL) return FAIL; 
	if (ssd->channel_head[gc_node->location->channel].lun_head[gc_node->location->lun].scheduled_gc[gc_node->location->plane] == NULL) return FAIL; 
	ssd->channel_head[gc_node->location->channel].lun_head[gc_node->location->lun].scheduled_gc[gc_node->location->plane] = NULL; 
	delete gc_node; 
	return SUCCESS; 
}
// =============== GC ALGORITHMS ================================ 
unsigned int best_cost(ssd_info * ssd, plane_info * the_plane, int * block_numbers, int number, int order /*which best, first best, second best, etc. */, int active_block /*not this one*/, int second_active_block){
	int i, j; 
	if (block_numbers == NULL) // means all block in the plane 
	{
		block_numbers = new int[number]; // fixme no delete 
		for (i = 0; i < number; i++){
			block_numbers[i] = i; 
		}
	}
	
	/* FASTER CODE */
	
	for (i = 0; i < 3; i++){
		for (j = i + 1; j < number; j++){
			int first_block = block_numbers[i];
			int second_block = block_numbers[j];
			if ( the_plane->blk_head[first_block].invalid_page_num  < the_plane->blk_head[second_block].invalid_page_num) {
				// swap 
				block_numbers[i] = second_block;
				block_numbers[j] = first_block;
			}
		}
	}

	while (block_numbers[order] == active_block || block_numbers[order] == second_active_block) // don't return the active block as the victim
		order++;
	if (order < 3){
		return block_numbers[order];
	}
	return -1; 
}
STATE greedy_algorithm(ssd_info * ssd,  local * location){

	unsigned int block = -1; 
	unsigned int active_block = 0, second_active_block = 0; 
	if(find_active_block(ssd,location)!=SUCCESS)                                 
	{
		printf("\n\n Error in greedy algorithm, plane active block\n");
		return ERROR;
	}
	active_block=ssd->channel_head[location->channel].lun_head[location->lun].plane_head[location->plane].active_block;
	second_active_block=ssd->channel_head[location->channel].lun_head[location->lun].plane_head[location->plane].second_active_block;
	location->block = best_cost(ssd, &ssd->channel_head[location->channel].lun_head[location->lun].plane_head[location->plane], NULL , ssd->parameter->block_plane, 0/* first best option*/, active_block, second_active_block); 
	return SUCCESS; 
}
STATE fifo_algorithm(ssd_info * ssd,  local * location){
	int block = -1; 
	int64_t min_time = 0; 
	int active_block = 0; 

	if(find_active_block(ssd,location)!=SUCCESS)                                 
	{
		printf("\n\n Error in greedy algorithm, plane active block\n");
		return ERROR;
	}
	active_block=ssd->channel_head[location->channel].lun_head[location->lun].plane_head[location->plane].active_block;

	for (int i = 0; i < ssd->parameter->block_plane; i++){
		if (i == active_block) continue; 
	
		if (ssd->channel_head[location->channel].lun_head[location->lun].plane_head[location->plane].blk_head[i].last_write_time <= min_time){
			min_time = ssd->channel_head[location->channel].lun_head[location->lun].plane_head[location->plane].blk_head[i].last_write_time; 
			block = i; 			
		}		
	}
	printf("block %d , %lld \n", block, ssd->channel_head[location->channel].lun_head[location->lun].plane_head[location->plane].blk_head[block].last_write_time );
	location->block = block; 
	return SUCCESS; 
}
STATE windowed_algorithm(ssd_info * ssd, local * location){
	unsigned int window_size = 100; // FIXME make it parameter 
	
	int * blocks = new int[window_size];
	//cout << "new blocks" << endl; 
	for (int i = 0; i < window_size; i++)
		blocks[i] = -1; 
	
	
	unsigned int active_block = 0, second_active_block = 0; 

	plane_info *the_plane = &ssd->channel_head[location->channel].lun_head[location->lun].plane_head[location->plane]; 
	
	if(find_active_block(ssd,location)!=SUCCESS)                                 
	{
		printf("\n\n Error in greedy algorithm, plane active block\n");
		return ERROR;
	}
	active_block=the_plane->active_block;
	second_active_block = the_plane->second_active_block; 

	for (int i = 0; i < ssd->parameter->block_plane; i++){
		if (i == active_block) continue; 
		int64_t time_i = the_plane->blk_head[i].last_write_time; 
		for (int j = 0; j < window_size; j++){
			if (blocks[j] == -1) {blocks[j] = i; break;}
			
			int64_t time_j = the_plane->blk_head[blocks[j]].last_write_time; 
			
			if (time_j > time_i){
				
				for (int k = window_size - 2; k >= j; k--){
					blocks[k+1] = blocks[k];				
				}
				blocks[j] = i; 
				break; 
			}
			
		}		
	}
	
	int temp = window_size - 1;
	while (blocks[temp] == -1) temp--; 
	
	unsigned int block = -1; 
	
	block= best_cost(ssd, the_plane, blocks, temp+1, 0 /*which best, first best, second best, etc. */, active_block /*not this one*/, second_active_block); 
	
	location->block = block;

	delete blocks;  
	return SUCCESS; 
}
STATE RGA_algorithm(ssd_info * ssd, local * location, int valid_number){
	unsigned int window_size = 200; // FIXME 
	unsigned int total_size = ssd->parameter->block_plane; 

	int * blocks = new int[total_size];
	for (int i = 0; i < total_size; i++){
		blocks[i] = i; 
	}

	unsigned int active_block = 0, second_active_block = 0;
	if (find_active_block(ssd, location) != SUCCESS)
	{
		printf("\n\n Error in rga algorithm, plane active block\n");
		return ERROR;
	}
	plane_info * p = &(ssd->channel_head[location->channel].lun_head[location->lun].plane_head[location->plane]);
	active_block = p->active_block;
	second_active_block = p->second_active_block;

	blocks[active_block] = blocks[total_size - 1]; 
	total_size--;
	blocks[second_active_block] = blocks[total_size - 1];
	total_size--; 

	for (int i = 0; i < window_size; i++){
		int j = rand() % (total_size - i);
		if (j != 0){
			int temp = blocks[i]; 
			blocks[i] = blocks[i+j];
			blocks[i + j] = temp; 				
		}
	}
	
	unsigned int selected = 0;

	if (valid_number != -1){
		int most_similar_diff = ssd->parameter->page_block;
		
		for (int i = 0; i < window_size; i++){
			unsigned int temp = blocks[i];
			int temp_valid = ssd->parameter->page_block - p->blk_head[temp].invalid_page_num ; 
			if (((temp_valid - valid_number) <= most_similar_diff)  && ((valid_number - temp_valid) <= most_similar_diff) ){
				most_similar_diff = temp_valid - valid_number;
				if (most_similar_diff < 0) most_similar_diff = -1 * most_similar_diff; 
				selected = i;
			}
		}
		location->block = blocks[selected];
	} else {
		unsigned int block = -1;
		block = best_cost(ssd, p, blocks, window_size, 0, active_block, second_active_block); // which best, first best, second best, etc. 
		location->block = block;
	}
	delete blocks; 

	return SUCCESS; 
}
STATE RANDOM_algorithm(ssd_info * ssd, local * location){
	unsigned int block = -1; 
	unsigned int active_block = 0; 
	if(find_active_block(ssd,location)!=SUCCESS)                                 
	{
		printf("\n\n Error in greedy algorithm, plane active block\n");
		return ERROR;
	}
	active_block=ssd->channel_head[location->channel].lun_head[location->lun].plane_head[location->plane].active_block;
	
	unsigned int rand_block = rand() % ssd->parameter->block_plane; 
	while (rand_block == active_block)
		rand_block = rand() % ssd->parameter->block_plane; 
	
	location->block = rand_block;
	return SUCCESS; 
}
STATE RANDOM_p_algorithm(ssd_info * ssd, local * location){
	unsigned int block = -1; 
	unsigned int active_block = 0; 
	if(find_active_block(ssd,location)!=SUCCESS)                                 
	{
		printf("\n\n Error in greedy algorithm, plane active block\n");
		return ERROR;
	}
	active_block=ssd->channel_head[location->channel].lun_head[location->lun].plane_head[location->plane].active_block;
	
	unsigned int rand_block = rand() % ssd->parameter->block_plane; 
	while (rand_block == active_block || ssd->channel_head[location->channel].lun_head[location->lun].plane_head[location->plane].blk_head[rand_block].invalid_page_num == 0)
		rand_block = rand() % ssd->parameter->block_plane; 
	
	location->block = rand_block; 

	return SUCCESS; 
}
STATE RANDOM_pp_algorithm(ssd_info * ssd, local * location){
	unsigned int least_invalid = ssd->parameter->overprovide * ssd->parameter->block_plane; 
	
	unsigned int block = -1; 
	unsigned int active_block = 0; 
	if(find_active_block(ssd,location)!=SUCCESS)                                 
	{
		printf("\n\n Error in greedy algorithm, plane active block\n");
		return ERROR;
	}
	active_block=ssd->channel_head[location->channel].lun_head[location->lun].plane_head[location->plane].active_block;
	
	unsigned int rand_block = rand() % ssd->parameter->block_plane; 
	int counter = 0; 
	while (rand_block == active_block || (ssd->channel_head[location->channel].lun_head[location->lun].plane_head[location->plane].blk_head[rand_block].invalid_page_num < least_invalid && counter <= ssd->parameter->block_plane)){
		rand_block = rand() % ssd->parameter->block_plane; 
		counter++; 
	}
	
	location->block = rand_block; 
	return SUCCESS; 
}
