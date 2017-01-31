#include "common.hh"

parameter_value::parameter_value(int argc, char ** argv){
	strcpy(filename, argv[1]); 
	load_parameters(argv[1]);
	time_scale = atoi(argv[4]); 
	lun_num = atoi(argv[3]); 
	// distribute luns between channels 
	for (int i = 0; i < channel_number; i++){
		lun_channel[i] = 0;
	}
	int channel = 0; 
	for (int i = 0; i < lun_num; i++){
		lun_channel[channel]++;
		channel = (channel + 1) % channel_number; 		
	}
	
	channel = 0; 	
	for (int i = 0; i < channel_number; i++){
		if (lun_channel[i] != 0){
			channel++;			
		}		
	}	
	channel_number = channel; 
	syn_rd_ratio = atof(argv[6]); 
	syn_req_size = atoi(argv[7]); 
	syn_interarrival_mean = atoi(argv[8]); 
	plane_level_tech = atoi(argv[9]);  
}

void parameter_value::print_all_parameters(FILE * stat_file){
	FILE * fp = fopen(filename, "r");
	char buffer[300];  
	while (fgets(buffer, 300, fp)){
		fprintf(stat_file, "%s", buffer); 
		fflush(stat_file); 
	}
	fclose(fp); 
}

ssd_info::ssd_info(parameter_value * parameters, char * statistics_filename, char * trace_filename)
{	
	parameter = parameters; 
	int over_provide = 100.0 * parameters->overprovide; 
	
	repeat_times = new int[parameters->consolidation_degree]; 
	last_times = new int64_t [parameters->consolidation_degree]; 
	
	for (int cd = 0; cd < parameters->consolidation_degree; cd++){
		repeat_times[cd]=0; // do not repeat the trace (just once) 
		last_times[cd]=0; 
	}
		
	min_lsn=0x7fffffff;
	total_page_number=parameters->lun_num*parameters->plane_lun*parameters->block_plane*parameters->page_block;

	dram = new dram_info(parameters);
	
	channel_head= new channel_info *[parameters->channel_number];
	for (int i = 0; i < parameters->channel_number; i++){
		channel_head[i] = new channel_info(i, parameters); 
	}

	statisticfile=fopen(statistics_filename,"w");
	if(statisticfile==NULL)
	{
		printf("the statistic file can't open\n");
		return; 
	}
	
	fprintf(statisticfile,"-----------------------parameter file----------------------\n");
	parameters->print_all_parameters(statisticfile); 
	fprintf(statisticfile,"\n");
	fprintf(statisticfile,"-----------------------simulation output----------------------\n");
	fflush(statisticfile);

	read_request_count = new int64_t[parameters->consolidation_degree];
	total_read_request_count = new int64_t[parameters->consolidation_degree];
	write_request_count = new int64_t[parameters->consolidation_degree];
	total_write_request_count = new int64_t[parameters->consolidation_degree];

	read_avg = new int64_t[parameters->consolidation_degree];
	write_avg = new int64_t[parameters->consolidation_degree];
	total_RT = new int64_t[parameters->consolidation_degree];
	total_read_RT = new int64_t[parameters->consolidation_degree];
	total_write_RT = new int64_t[parameters->consolidation_degree];

	read_request_size = new int64_t[parameters->consolidation_degree];
	total_read_request_size = new int64_t[parameters->consolidation_degree];
	write_request_size = new int64_t[parameters->consolidation_degree];
	total_write_request_size = new int64_t[parameters->consolidation_degree];

	for (int i = 0; i < parameters->consolidation_degree; i++){
		read_request_count[i] = 0;
		total_read_request_count[i] = 0;

		write_request_count[i] = 0;
		total_write_request_count[i] = 0;

		write_avg[i] = 0;
		read_avg[i] = 0;
		total_RT[i] = 0; 

		read_request_size[i] = 0;
		total_read_request_size[i] = 0;
		write_request_size[i] = 0;
		total_write_request_size[i] = 0;
	}

	flash_read_count = 0; 
	total_flash_read_count = 0; 
	flash_prog_count = 0; 
	total_flash_prog_count = 0; 
	flash_erase_count = 0; 
	total_flash_erase_count = 0; 

	queue_read_count = 0; 
	queue_prog_count = 0; 	
	direct_erase_count = 0; 
	total_direct_erase_count = 0; 
	
	m_plane_read_count = 0; 
	total_m_plane_read_count = 0; 
	m_plane_prog_count = 0; 
	total_m_plane_prog_count = 0; 
	m_plane_erase_count = 0; 
	total_m_plane_erase_count = 0; 
	
	interleave_read_count = 0; 
	total_interleave_read_count = 0; 
	interleave_prog_count = 0; 
	total_interleave_prog_count = 0; 
	interleave_erase_count = 0; 
	total_interleave_erase_count = 0; 
	
	gc_copy_back = 0; 
	total_gc_copy_back = 0; 
	
	waste_page_count = 0; 
	total_waste_page_count = 0; 
	
	update_read_count = 0; 
	total_update_read_count = 0; 
	
	write_worst_case_rt = 0; 
	read_worst_case_rt = 0; 
	
	gc_moved_page = 0; 
	request_sequence_number = 0;
	subrequest_sequence_number = 0;  
	gc_sequence_number = 0; 
	lun_token = 0; 	
	subreq_state_time = new int64_t[SR_MODE_NUM]; 
	for (int i = 0; i < SR_MODE_NUM; i++){
		subreq_state_time[i] = 0; 	
	}

	read_multiplane_count = 0; 
	write_multiplane_count = 0; 
	erase_multiplane_count = 0; 

}



dram_info::dram_info(parameter_value * parameters)
{
	dram_capacity = parameters->dram_capacity;		
	map = new map_info(); 
	int page_num = parameters->page_block*parameters->block_plane*parameters->plane_lun*parameters->lun_num;
	map->map_entry = new entry[page_num];
}

page_info::page_info(){
	valid_state =0;
	free_state = PG_SUB;
	lpn = -1;
	written_count=0; 
}

blk_info::blk_info(parameter_value * parameters)
{
	free_page_num = parameters->page_block;	// all pages are free
	last_write_page = -1;	// no page has been programmed
	page_num = parameters->page_block; 
	invalid_page_num = 0; 

	page_head = new page_info*[parameters->page_block]; 
	for(int i = 0; i<parameters->page_block; i++)
	{
		page_head[i] = new page_info(); 
	}
}
plane_info::plane_info(parameter_value  * parameters)
{
	add_reg_ppn = -1;  
	free_page=parameters->block_plane*parameters->page_block;

	blk_head = new blk_info*[parameters->block_plane]; 
	for(int i = 0; i<parameters->block_plane; i++)
	{
		blk_head[i] = new blk_info(parameters); 
	}
	block_num = parameters->block_plane; 
	active_block = 0; 
	erase_count = 0; 
	program_count = 0; 
	GCMode = false; 
	current_state = PLANE_MODE_IDLE; 
	next_state = PLANE_MODE_IDLE; 
	current_time = 0; 
	next_state_predict_time = 0; 
	state_time = new int64_t[PLANE_MODE_NUM]; 
	for (int i = 0; i < PLANE_MODE_NUM; i++){
		state_time[i] = 0; 
	}
	scheduled_gc = NULL; 
}

lun_info::lun_info(parameter_value * parameter)
{		
	current_state = LUN_MODE_IDLE;
	next_state = LUN_MODE_IDLE;
	current_time = 0;
	next_state_predict_time = 0;		
	plane_num = parameter->plane_lun; 	
	read_count = 0;
	program_count = 0;
	erase_count = 0;
	read_avg = 0; 
	program_avg = 0; 
	gc_avg = 0; 
	plane_token = 0; 	

	GCMode = false; 
	plane_head = new plane_info*[parameter->plane_lun];
	for (int i = 0; i<parameter->plane_lun; i++)
	{
		plane_head[i] = new plane_info(parameter); 
	}
	
	state_time = new int64_t [LUN_MODE_NUM]; 
	for (int i = 0; i < LUN_MODE_NUM; i++)
		state_time[i] = 0; 
	
}
channel_info::channel_info(int channel_number, parameter_value * parameters) {
	// set the parameter of each channel
	lun_num = parameters->lun_channel[channel_number];
	current_state = CHANNEL_MODE_IDLE;
	next_state = CHANNEL_MODE_IDLE;
	current_time = 0; 
	next_state_predict_time = 0; 	
	lun_head = new lun_info*[lun_num]; 
		
	for (int j = 0; j< lun_num; j++)
	{
		lun_head[j] = new lun_info(parameters); 
	}
		
	state_time = new int64_t[CHANNEL_MODE_NUM]; 
	for (int i = 0; i < CHANNEL_MODE_NUM; i++){
		state_time[i] = 0; 
	}
	
}
void parameter_value::load_parameters(char *parameter_file)
{
	if (parameter_file == NULL) return; 
	FILE * fp;
	FILE * fp1;
	FILE * fp2;
	//errno_t ferr;
	parameter_value *p;
	char buf[BUFSIZE];
	int i;
	int pre_eql,next_eql;
	int res_eql;
	char *ptr;

	queue_length=QUEUE_LENGTH; // NARGES why 5, it's better to be more than 5, like 32 or 20  // FIXME 
	memset(buf,0,BUFSIZE);
	
	fp=fopen(parameter_file,"r");
	if(fp == NULL)
	{	
		printf("the file parameter_file error!\n");
		return; 	
	}
	
	while(fgets(buf,200,fp)){
		if(buf[0] =='#' || buf[0] == ' ') continue;
		ptr=strchr(buf,'=');
		if(!ptr) continue; 
		
		pre_eql = ptr - buf;
		next_eql = pre_eql + 1;

		while(buf[pre_eql-1] == ' ') pre_eql--;
		buf[pre_eql] = 0;
		if((res_eql=strcmp(buf,"lun number")) ==0){			
			sscanf(buf + next_eql,"%d",&lun_num);
		}else if((res_eql=strcmp(buf,"dram capacity")) ==0){
			sscanf(buf + next_eql,"%d",&dram_capacity);
		}else if((res_eql=strcmp(buf,"cpu sdram")) ==0){
			sscanf(buf + next_eql,"%d",&cpu_sdram);
		}else if((res_eql=strcmp(buf,"channel number")) ==0){
			sscanf(buf + next_eql,"%d",&channel_number); 
		}else if((res_eql=strcmp(buf,"plane number")) ==0){
			sscanf(buf + next_eql,"%d",&plane_lun); 
		}else if((res_eql=strcmp(buf,"block number")) ==0){
			sscanf(buf + next_eql,"%d",&block_plane); 
		}else if((res_eql=strcmp(buf,"page number")) ==0){
			sscanf(buf + next_eql,"%d",&page_block); 
		}else if((res_eql=strcmp(buf,"subpage page")) ==0){
			sscanf(buf + next_eql,"%d",&subpage_page); 
		}else if((res_eql=strcmp(buf,"page capacity")) ==0){
			sscanf(buf + next_eql,"%d",&page_capacity); 
		}else if((res_eql=strcmp(buf,"subpage capacity")) ==0){
			sscanf(buf + next_eql,"%d",&subpage_capacity); 
		}else if((res_eql=strcmp(buf,"t_PROG")) ==0){
			sscanf(buf + next_eql,"%d",&time_characteristics.tPROG); 
		}else if((res_eql=strcmp(buf,"t_DBSY")) ==0){
			sscanf(buf + next_eql,"%d",&time_characteristics.tDBSY); 
		}else if((res_eql=strcmp(buf,"t_BERS")) ==0){
			sscanf(buf + next_eql,"%d",&time_characteristics.tBERS); 
		}else if((res_eql=strcmp(buf,"t_WC")) ==0){
			sscanf(buf + next_eql,"%d",&time_characteristics.tWC); 
		}else if((res_eql=strcmp(buf,"t_R")) ==0){
			sscanf(buf + next_eql,"%d",&time_characteristics.tR); 
		}else if((res_eql=strcmp(buf,"t_RC")) ==0){
			sscanf(buf + next_eql,"%d",&time_characteristics.tRC); 
		}else if((res_eql=strcmp(buf,"erase limit")) ==0){
			sscanf(buf + next_eql,"%d",&ers_limit); 
		}else if((res_eql=strcmp(buf,"address mapping")) ==0){
			sscanf(buf + next_eql,"%d",&address_mapping); 
		}else if((res_eql=strcmp(buf,"wear leveling")) ==0){
			sscanf(buf + next_eql,"%d",&wear_leveling); 
		}else if((res_eql=strcmp(buf,"gc")) ==0){
			sscanf(buf + next_eql,"%d",&gc); 
		}else if((res_eql=strcmp(buf,"clean in background")) ==0){
			sscanf(buf + next_eql,"%d",&clean_in_background); 
		}else if((res_eql=strcmp(buf,"overprovide")) ==0){
			sscanf(buf + next_eql,"%f",&overprovide); 
		}else if((res_eql=strcmp(buf,"gc threshold")) ==0){
			sscanf(buf + next_eql,"%f",&gc_threshold); 
		}else if((res_eql=strcmp(buf,"scheduling algorithm")) ==0){
			sscanf(buf + next_eql,"%d",&scheduling_algorithm); 
		}else if((res_eql=strcmp(buf,"quick table radio")) ==0){
			sscanf(buf + next_eql,"%f",&quick_radio); 
		}else if((res_eql=strcmp(buf,"related mapping")) ==0){
			sscanf(buf + next_eql,"%d",&related_mapping); 
		}else if((res_eql=strcmp(buf,"striping")) ==0){
			sscanf(buf + next_eql,"%d",&striping); 
		}else if((res_eql=strcmp(buf,"interleaving")) ==0){
			sscanf(buf + next_eql,"%d",&interleaving); 
		}else if((res_eql=strcmp(buf,"pipelining")) ==0){
			sscanf(buf + next_eql,"%d",&pipelining); 
		}else if((res_eql=strcmp(buf,"time_step")) ==0){
			sscanf(buf + next_eql,"%d",&time_step); 
		}else if((res_eql=strcmp(buf,"small large write")) ==0){
			sscanf(buf + next_eql,"%d",&small_large_write); 
		}
		else if((res_eql=strcmp(buf,"active write")) ==0){
			sscanf(buf + next_eql,"%d",&active_write); 
		}else if((res_eql=strcmp(buf,"gc down threshold")) ==0){
			sscanf(buf + next_eql,"%f",&gc_down_threshold); 
		}else if ((res_eql = strcmp(buf, "gc up threshold")) == 0){
			sscanf(buf + next_eql, "%f", &gc_up_threshold);
		}else if ((res_eql = strcmp(buf, "gc mplane threshold")) == 0){
			sscanf(buf + next_eql, "%f", &gc_mplane_threshold);
		}else if((res_eql=strcmp(buf,"advanced command")) ==0){
			sscanf(buf + next_eql,"%d",&advanced_commands); 
		}else if((res_eql=strcmp(buf,"queue_length")) ==0){
			sscanf(buf + next_eql,"%d",&queue_length); 
		}else if((res_eql=strcmp(buf,"consolidation degree")) == 0) {
			sscanf(buf + next_eql,"%d",&consolidation_degree); 
		}else if((res_eql=strcmp(buf,"MP address check")) == 0){
			sscanf(buf + next_eql,"%d",&MP_address_check); 
		}else if((res_eql=strcmp(buf,"repeat trace")) == 0) {
			sscanf(buf + next_eql,"%d",&repeat_trace); 
		}else if((res_eql=strcmp(buf,"mplane gc")) == 0) {
			sscanf(buf + next_eql,"%d",&mplane_gc); 
		}else if ((res_eql = strcmp(buf, "pargc approach")) == 0) {
			sscanf(buf + next_eql, "%d", &pargc_approach);
		}else if ((res_eql = strcmp(buf, "gc algorithm")) == 0) {
			sscanf(buf + next_eql,"%d",&gc_algorithm); 
		}else if((res_eql=strncmp(buf,"lun number",11)) ==0)
		{
			sscanf(buf+12,"%d",&i);
			sscanf(buf + next_eql,"%d",&lun_channel[i]); 
		}else if((res_eql=strcmp(buf,"syn rd ratio")) == 0){
			sscanf(buf + next_eql, "%f",&syn_rd_ratio); 
		}else if((res_eql=strcmp(buf,"syn interarrival mean")) == 0){
			sscanf(buf + next_eql, "%d",&syn_interarrival_mean); 
		}else if((res_eql=strcmp(buf,"syn req size")) == 0){
			sscanf(buf + next_eql, "%d",&syn_req_size); 
		}else if((res_eql=strcmp(buf,"syn req count")) == 0){
			sscanf(buf + next_eql, "%d",&syn_req_count); 	
		}else{
			printf("don't match\t %s\n",buf);
		}
		
		memset(buf,0,BUFSIZE);
		
	}
	fclose(fp);
	
}

void file_assert(int error,const char *s){
	if(error == 0) return;
	printf("open %s error\n",s);
	getchar();
	exit(-1);
}


void trace_assert(int64_t time,int device,unsigned int lsn,int size,int ope)
{
	if(time <0 || device < 0  || size < 0 || ope < 0)
	{
		printf("trace error:%lld %d %d %d %d\n",time,device,lsn,size,ope);
		getchar();
		exit(-1);
	}
	if(time == 0 && device == 0 && lsn == 0 && size == 0 && ope == 0)
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
sub_request * SubQueue::get_subreq(int index){

	if (index < 0) return NULL; 
	int temp = 0; 
	sub_request * qsub = queue_head; 

	while (qsub != NULL) {
		if (temp == index) return qsub; 
		qsub = qsub->next_node;  
		temp++; 
	} 

	return NULL; 

}

bool SubQueue::find_subreq(sub_request * sub){
	// is there any request for the same location and same operation (read and write FIXME??? )
	sub_request * qsub = queue_head; 
	
	while (qsub != NULL){
		
		if (qsub == sub) return true; 
		if (qsub->lpn == sub->lpn) return true; 
		if (qsub->location->channel == sub->location->channel && qsub->location->lun == sub->location->lun && qsub->location->plane == sub->location->plane){
			
			if (qsub->location->block == sub->location->block && qsub->location->page == sub->location->page) {
				// FIXME do we need to compare state?? 
				return true; 
				
			}
			
		}
		
		qsub = qsub->next_node; 
	}
	return false; 
}
	
void SubQueue::push_tail(sub_request * sub){
	if (sub == NULL) return; 
	
	size++; 
	
	if (queue_tail == NULL){
		queue_head = sub; 
		queue_tail = sub; 
		return; 
	}
	
	queue_tail->next_node = sub; 
	queue_tail = sub; 
	sub->next_node = NULL; 
 
}

void SubQueue::push_head(sub_request * sub){
	if (sub == NULL) return; 
	
	size++; 
	if (queue_tail == NULL){
		queue_head = sub; 
		queue_tail = sub; 
		return; 
	}
	
	sub->next_node = queue_head; 
	queue_head = sub; 
	
}

void SubQueue::remove_node(sub_request * sub){
	if (sub == NULL) return; 
	
	size--; 
	if (sub == queue_head) {
		queue_head = queue_head->next_node; 
		
		if (queue_head == NULL) 
			queue_tail = NULL; 
		
		return; 
	}
	sub_request * temp = queue_head; 
	while (temp!= NULL && temp->next_node != sub){
		temp = temp->next_node;			
	}
	
	if (temp == NULL){
		printf("ERROR couldn't find the sub request \n"); 
		return; 
	}
	
	if (temp->next_node == sub){
		temp->next_node = sub->next_node; 
		if (temp->next_node == NULL){
			queue_tail = temp; 
		}
	}
	
	//delete sub; 
}

sub_request * SubQueue::target_request(int plane, int block, int page ){
	
	sub_request * sub = queue_head; 
	
	while (sub != NULL){
		if (sub->location->plane == plane)
			if (block == -1 || sub->location->block == block )
				if (page == -1 || sub->location->page == page)
					return sub; 
		
		
		sub = sub->next_node; 
		
	}
	
	return sub; 
}
bool SubQueue::is_empty(){
	if (queue_head == NULL) return true; 
	return false; 
}


