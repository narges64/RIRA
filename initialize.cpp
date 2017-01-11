#include <stdlib.h>
#include <iostream>
#include <string> 
 
#include "initialize.hh"

ssd_info *initiation(ssd_info *ssd, char ** argv)
{

	unsigned int x=0,y=0,i=0,j=0,k=0,l=0,m=0,n=0;
	
	char * add_name = argv[2];  // uniquness of each run   

	ssd->parameterfilename = new char[40];
	for (int i = 0; i < 10; i++)
		ssd->tracefilename[i] = new char[40];  
	ssd->statisticfilename = new char[40]; 	
	
	parameter_value *parameters;
	sprintf(ssd->parameterfilename, "%s", argv[1]);
	parameters=load_parameters(ssd->parameterfilename);
	parameters->time_scale = atof(argv[4]);
	parameters->lun_num = atoi(argv[3]);
	for (int i = 0; i < parameters->channel_number; i++){
		parameters->lun_channel[i] = 0;
	}
	int channel = 0; 
	for (int i = 0; i < parameters->lun_num; i++){
		parameters->lun_channel[channel]++;
		channel = (channel + 1) % parameters->channel_number; 		
	}
	
	int channel_number = 0; 
	for (int i = 0; i < parameters->channel_number; i++){
		if (parameters->lun_channel[i] != 0){
			channel_number++;			
		}		
	}
	parameters->channel_number = channel_number; 
	
	char buffer[300];
	FILE *fp=NULL;
	
	char * name = new char[40]; 
	
	int cd; 
	for (cd = 0; cd < parameters->consolidation_degree; cd++){
		strncpy(ssd->tracefilename[cd], argv[5+ cd ], 30); 
	}

	// FIXME  
	parameters->syn_rd_ratio = atof(argv[6]);
	parameters->syn_req_size = atoi(argv[7]); 
	parameters->syn_interarrival_mean = atoi(argv[8]);  
	parameters->plane_level_tech = atoi(argv[9]); 
	int over_provide = 100.0 * parameters->overprovide; 
	
	sprintf(name, "stat1_%s_%d_%d", add_name, parameters->lun_num, over_provide); 
	strncpy(ssd->statisticfilename,name,40); 
	

	ssd->repeat_times = new int[parameters->consolidation_degree]; 
	ssd->last_times = new int64_t [parameters->consolidation_degree]; 
	
	for (cd = 0; cd < parameters->consolidation_degree; cd++){
		ssd->repeat_times[cd]=0; 
		ssd->last_times[cd]=0; 
	}
	
	ssd->parameter=parameters;
		
	ssd->min_lsn=0x7fffffff;
	ssd->page=ssd->parameter->lun_num*ssd->parameter->plane_lun*ssd->parameter->block_plane*ssd->parameter->page_block;

	ssd->dram = new dram_info();
	initialize_dram(ssd);
	
	ssd->channel_head= new channel_info[ssd->parameter->channel_number];
	initialize_channels(ssd );

	printf("\n");
	ssd->statisticfile=fopen(ssd->statisticfilename,"w");
	if(ssd->statisticfile==NULL)
	{
		printf("the statistic file can't open\n");
		return NULL;
	}
	
	printf("\n");

	
	fprintf(ssd->statisticfile,"parameter file: %s\n",ssd->parameterfilename); 
	fprintf(ssd->statisticfile,"trace file: %s\n",ssd->tracefilename[0]);
	

	fflush(ssd->statisticfile);

	fp=fopen(ssd->parameterfilename,"r");
	if(fp==NULL)
	{
		printf("\nthe parameter file can't open!\n");
		return NULL;
	}

	fprintf(ssd->statisticfile,"-----------------------parameter file----------------------\n");
	while(fgets(buffer,300,fp))
	{
		fprintf(ssd->statisticfile,"%s",buffer);
		fflush(ssd->statisticfile);
	}

	fprintf(ssd->statisticfile,"\n");
	fprintf(ssd->statisticfile,"-----------------------simulation output----------------------\n");
	fflush(ssd->statisticfile);

	fclose(fp);

	ssd->read_request_count = new unsigned int[ssd->parameter->consolidation_degree];
	ssd->total_read_request_count = new unsigned int[ssd->parameter->consolidation_degree];
	ssd->write_request_count = new unsigned int[ssd->parameter->consolidation_degree];
	ssd->total_write_request_count = new unsigned int[ssd->parameter->consolidation_degree];

	ssd->read_avg = new int64_t[ssd->parameter->consolidation_degree];
	ssd->write_avg = new int64_t[ssd->parameter->consolidation_degree];
	ssd->total_RT = new int64_t[ssd->parameter->consolidation_degree];
	ssd->total_read_RT = new int64_t[ssd->parameter->consolidation_degree];
	ssd->total_write_RT = new int64_t[ssd->parameter->consolidation_degree];

	ssd->read_request_size = new unsigned int[ssd->parameter->consolidation_degree];
	ssd->total_read_request_size = new unsigned int[ssd->parameter->consolidation_degree];
	ssd->write_request_size = new unsigned int[ssd->parameter->consolidation_degree];
	ssd->total_write_request_size = new unsigned int[ssd->parameter->consolidation_degree];

	for (int i = 0; i < ssd->parameter->consolidation_degree; i++){
		ssd->read_request_count[i] = 0;
		ssd->total_read_request_count[i] = 0;

		ssd->write_request_count[i] = 0;
		ssd->total_write_request_count[i] = 0;

		ssd->write_avg[i] = 0;
		ssd->read_avg[i] = 0;
		ssd->total_RT[i] = 0; 

		ssd->read_request_size[i] = 0;
		ssd->total_read_request_size[i] = 0;
		ssd->write_request_size[i] = 0;
		ssd->total_write_request_size[i] = 0;
	}

	ssd->flash_read_count = 0; 
	ssd->total_flash_read_count = 0; 
	ssd->flash_prog_count = 0; 
	ssd->total_flash_prog_count = 0; 
	ssd->flash_erase_count = 0; 
	ssd->total_flash_erase_count = 0; 
	
	ssd->direct_erase_count = 0; 
	ssd->total_direct_erase_count = 0; 
	
	ssd->m_plane_read_count = 0; 
	ssd->total_m_plane_read_count = 0; 
	ssd->m_plane_prog_count = 0; 
	ssd->total_m_plane_prog_count = 0; 
	ssd->m_plane_erase_count = 0; 
	ssd->total_m_plane_erase_count = 0; 
	
	ssd->interleave_read_count = 0; 
	ssd->total_interleave_read_count = 0; 
	ssd->interleave_prog_count = 0; 
	ssd->total_interleave_prog_count = 0; 
	ssd->interleave_erase_count = 0; 
	ssd->total_interleave_erase_count = 0; 
	
	ssd->gc_copy_back = 0; 
	ssd->total_gc_copy_back = 0; 
	
	ssd->waste_page_count = 0; 
	ssd->total_waste_page_count = 0; 
	
	ssd->update_read_count = 0; 
	ssd->total_update_read_count = 0; 
	
	ssd->write_worst_case_rt = 0; 
	ssd->read_worst_case_rt = 0; 
	
	ssd->gc_moved_page = 0; 

	ssd->lun_token = 0; 	
	ssd->subreq_state_time = new int64_t[SR_MODE_NUM]; 
	for (unsigned int i = 0; i < SR_MODE_NUM; i++){
		ssd->subreq_state_time[i] = 0; 	
	}

	ssd->read_multiplane_count = 0; 
	ssd->write_multiplane_count = 0; 
	ssd->erase_multiplane_count = 0; 


	printf("initiation is completed!\n");
 
	cout << "+++++++++++++++++++++++++++++   " << ssd->parameter->syn_rd_ratio << endl; 
	return ssd;
}

dram_info * initialize_dram(ssd_info * ssd)
{
	unsigned int page_num;

	dram_info *dram=ssd->dram;
	dram->dram_capacity = ssd->parameter->dram_capacity;		
	dram->map = new map_info(); 

	page_num = ssd->parameter->page_block*ssd->parameter->block_plane*ssd->parameter->plane_lun*ssd->parameter->lun_num;
	
	dram->map->map_entry = new entry[page_num];

	return dram;
}

page_info * initialize_page(page_info * p_page )
{
	p_page->valid_state =0;
	p_page->free_state = PG_SUB;
	p_page->lpn = -1;
	p_page->written_count=0;
	return p_page;
}

blk_info * initialize_block(blk_info * p_block,parameter_value *parameter)
{
	unsigned int i;
	page_info * p_page;
	
	p_block->free_page_num = parameter->page_block;	// all pages are free
	p_block->last_write_page = -1;	// no page has been programmed

	p_block->invalid_page_num = 0; 
	p_block->page_head = new page_info[parameter->page_block]; 

	for(i = 0; i<parameter->page_block; i++)
	{
		p_page = &(p_block->page_head[i]);
		initialize_page(p_page );
	}
	return p_block;

}

plane_info * initialize_plane(plane_info * p_plane,parameter_value *parameter )
{
	unsigned int i;
	blk_info * p_block;
	p_plane->add_reg_ppn = -1;  
	p_plane->free_page=parameter->block_plane*parameter->page_block;
	p_plane->blk_head = new blk_info[parameter->block_plane]; 

	for(i = 0; i<parameter->block_plane; i++)
	{
		p_block = &(p_plane->blk_head[i]);
		initialize_block( p_block ,parameter);			
	}

	p_plane->active_block = 0; 
		
	p_plane->second_active_block = 1; 

	p_plane->erase_count = 0; 
	p_plane->program_count = 0; 

	p_plane->GCMode = PLANE_MODE_IO; 
		
	p_plane->current_state = PLANE_MODE_IDLE; 
	p_plane->next_state = PLANE_MODE_IDLE; 
	p_plane->current_time = 0; 
	p_plane->next_state_predict_time = 0; 
	p_plane->state_time = new int64_t[PLANE_MODE_NUM]; 
	for (int i = 0; i < PLANE_MODE_NUM; i++){
		p_plane->state_time[i] = 0; 
	}
	return p_plane; 
}

lun_info * initialize_lun(lun_info * p_lun,parameter_value *parameter,long long current_time )
{
		
	p_lun->current_state = LUN_MODE_IDLE;
	p_lun->next_state = LUN_MODE_IDLE;
	p_lun->current_time = current_time;
	p_lun->next_state_predict_time = 0;		
	p_lun->plane_num = parameter->plane_lun; 	
	p_lun->read_count = 0;
	p_lun->program_count = 0;
	p_lun->erase_count = 0;
	p_lun->read_avg = 0; 
	p_lun->program_avg = 0; 
	p_lun->gc_avg = 0; 
	
	p_lun->plane_head = new plane_info[parameter->plane_lun];

	p_lun->GCMode = LUN_MODE_IO; 
	p_lun->scheduled_gc = new gc_operation *[parameter->plane_lun]; 
	plane_info * p_plane; 
	for (unsigned int i = 0; i<parameter->plane_lun; i++)
	{
		p_plane = &(p_lun->plane_head[i]);
		initialize_plane(p_plane,parameter );
		p_lun->scheduled_gc[i] = NULL; 
	}
	
	p_lun->state_time = new int64_t [LUN_MODE_NUM]; 
	for (unsigned int i = 0; i < LUN_MODE_NUM; i++)
		p_lun->state_time[i] = 0; 
	
	return p_lun;
}

ssd_info * initialize_channels(ssd_info * ssd )
{
	unsigned int i,j;
	channel_info * p_channel;
	lun_info * p_lun;

	// set the parameter of each channel
	for (i = 0; i< ssd->parameter->channel_number; i++)
	{
		p_channel = &(ssd->channel_head[i]);
		p_channel->lun = ssd->parameter->lun_channel[i];
		p_channel->current_state = CHANNEL_MODE_IDLE;
		p_channel->next_state = CHANNEL_MODE_IDLE;
		
		p_channel->lun_head = new lun_info[ssd->parameter->lun_channel[i]]; 
		
		for (j = 0; j< ssd->parameter->lun_channel[i]; j++)
		{
			p_lun = &(p_channel->lun_head[j]);
			initialize_lun(p_lun,ssd->parameter,ssd->current_time );
		}
		
		p_channel->state_time = new int64_t[CHANNEL_MODE_NUM]; 
		for (unsigned int i = 0; i < CHANNEL_MODE_NUM; i++){
			p_channel->state_time[i] = 0; 
		}
	
	}

	
	return ssd;
}


parameter_value *load_parameters(char parameter_file[30])
{
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

	p = new parameter_value(); 
	p->queue_length=QUEUE_LENGTH; // NARGES why 5, it's better to be more than 5, like 32 or 20  // FIXME 
	memset(buf,0,BUFSIZE);
	
	fp=fopen(parameter_file,"r");
	if(fp == NULL)
	{	
		printf("the file parameter_file error!\n");	
		return p;
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
			sscanf(buf + next_eql,"%d",&p->lun_num);
		}else if((res_eql=strcmp(buf,"dram capacity")) ==0){
			sscanf(buf + next_eql,"%d",&p->dram_capacity);
		}else if((res_eql=strcmp(buf,"cpu sdram")) ==0){
			sscanf(buf + next_eql,"%d",&p->cpu_sdram);
		}else if((res_eql=strcmp(buf,"channel number")) ==0){
			sscanf(buf + next_eql,"%d",&p->channel_number); 
		}else if((res_eql=strcmp(buf,"plane number")) ==0){
			sscanf(buf + next_eql,"%d",&p->plane_lun); 
		}else if((res_eql=strcmp(buf,"block number")) ==0){
			sscanf(buf + next_eql,"%d",&p->block_plane); 
		}else if((res_eql=strcmp(buf,"page number")) ==0){
			sscanf(buf + next_eql,"%d",&p->page_block); 
		}else if((res_eql=strcmp(buf,"subpage page")) ==0){
			sscanf(buf + next_eql,"%d",&p->subpage_page); 
		}else if((res_eql=strcmp(buf,"page capacity")) ==0){
			sscanf(buf + next_eql,"%d",&p->page_capacity); 
		}else if((res_eql=strcmp(buf,"subpage capacity")) ==0){
			sscanf(buf + next_eql,"%d",&p->subpage_capacity); 
		}else if((res_eql=strcmp(buf,"t_PROG")) ==0){
			sscanf(buf + next_eql,"%d",&p->time_characteristics.tPROG); 
		}else if((res_eql=strcmp(buf,"t_DBSY")) ==0){
			sscanf(buf + next_eql,"%d",&p->time_characteristics.tDBSY); 
		}else if((res_eql=strcmp(buf,"t_BERS")) ==0){
			sscanf(buf + next_eql,"%d",&p->time_characteristics.tBERS); 
		}else if((res_eql=strcmp(buf,"t_WC")) ==0){
			sscanf(buf + next_eql,"%d",&p->time_characteristics.tWC); 
		}else if((res_eql=strcmp(buf,"t_R")) ==0){
			sscanf(buf + next_eql,"%d",&p->time_characteristics.tR); 
		}else if((res_eql=strcmp(buf,"t_RC")) ==0){
			sscanf(buf + next_eql,"%d",&p->time_characteristics.tRC); 
		}else if((res_eql=strcmp(buf,"erase limit")) ==0){
			sscanf(buf + next_eql,"%d",&p->ers_limit); 
		}else if((res_eql=strcmp(buf,"address mapping")) ==0){
			sscanf(buf + next_eql,"%d",&p->address_mapping); 
		}else if((res_eql=strcmp(buf,"wear leveling")) ==0){
			sscanf(buf + next_eql,"%d",&p->wear_leveling); 
		}else if((res_eql=strcmp(buf,"gc")) ==0){
			sscanf(buf + next_eql,"%d",&p->gc); 
		}else if((res_eql=strcmp(buf,"clean in background")) ==0){
			sscanf(buf + next_eql,"%d",&p->clean_in_background); 
		}else if((res_eql=strcmp(buf,"overprovide")) ==0){
			sscanf(buf + next_eql,"%f",&p->overprovide); 
		}else if((res_eql=strcmp(buf,"gc threshold")) ==0){
			sscanf(buf + next_eql,"%f",&p->gc_threshold); 
		}else if((res_eql=strcmp(buf,"scheduling algorithm")) ==0){
			sscanf(buf + next_eql,"%d",&p->scheduling_algorithm); 
		}else if((res_eql=strcmp(buf,"quick table radio")) ==0){
			sscanf(buf + next_eql,"%f",&p->quick_radio); 
		}else if((res_eql=strcmp(buf,"related mapping")) ==0){
			sscanf(buf + next_eql,"%d",&p->related_mapping); 
		}else if((res_eql=strcmp(buf,"striping")) ==0){
			sscanf(buf + next_eql,"%d",&p->striping); 
		}else if((res_eql=strcmp(buf,"interleaving")) ==0){
			sscanf(buf + next_eql,"%d",&p->interleaving); 
		}else if((res_eql=strcmp(buf,"pipelining")) ==0){
			sscanf(buf + next_eql,"%d",&p->pipelining); 
		}else if((res_eql=strcmp(buf,"time_step")) ==0){
			sscanf(buf + next_eql,"%d",&p->time_step); 
		}else if((res_eql=strcmp(buf,"small large write")) ==0){
			sscanf(buf + next_eql,"%d",&p->small_large_write); 
		}
		//else if((res_eql=strcmp(buf,"active write threshold")) ==0){
		//	sscanf(buf + next_eql,"%d",&p->threshold_fixed_adjust); 
		//}else if((res_eql=strcmp(buf,"threshold value")) ==0){
		//	sscanf(buf + next_eql,"%f",&p->threshold_value); 
		//}
		else if((res_eql=strcmp(buf,"active write")) ==0){
			sscanf(buf + next_eql,"%d",&p->active_write); 
		}else if((res_eql=strcmp(buf,"gc down threshold")) ==0){
			sscanf(buf + next_eql,"%f",&p->gc_down_threshold); 
		}else if ((res_eql = strcmp(buf, "gc up threshold")) == 0){
			sscanf(buf + next_eql, "%f", &p->gc_up_threshold);
		}else if ((res_eql = strcmp(buf, "gc mplane threshold")) == 0){
			sscanf(buf + next_eql, "%f", &p->gc_mplane_threshold);
		}else if((res_eql=strcmp(buf,"advanced command")) ==0){
			sscanf(buf + next_eql,"%d",&p->advanced_commands); 
		}else if((res_eql=strcmp(buf,"queue_length")) ==0){
			sscanf(buf + next_eql,"%d",&p->queue_length); 
		}else if((res_eql=strcmp(buf,"consolidation degree")) == 0) {
			sscanf(buf + next_eql,"%d",&p->consolidation_degree); 
		}else if((res_eql=strcmp(buf,"MP address check")) == 0){
			sscanf(buf + next_eql,"%d",&p->MP_address_check); 
		}else if((res_eql=strcmp(buf,"repeat trace")) == 0) {
			sscanf(buf + next_eql,"%d",&p->repeat_trace); 
		}else if((res_eql=strcmp(buf,"mplane gc")) == 0) {
			sscanf(buf + next_eql,"%d",&p->mplane_gc); 
		}else if ((res_eql = strcmp(buf, "pargc approach")) == 0) {
			sscanf(buf + next_eql, "%d", &p->pargc_approach);
		}else if ((res_eql = strcmp(buf, "gc algorithm")) == 0) {
			sscanf(buf + next_eql,"%d",&p->gc_algorithm); 
		}else if((res_eql=strncmp(buf,"lun number",11)) ==0)
		{
			sscanf(buf+12,"%d",&i);
			sscanf(buf + next_eql,"%d",&p->lun_channel[i]); 
		}else if((res_eql=strcmp(buf,"syn rd ratio")) == 0){
			sscanf(buf + next_eql, "%f",&p->syn_rd_ratio); 
		}else if((res_eql=strcmp(buf,"syn interarrival mean")) == 0){
			sscanf(buf + next_eql, "%d",&p->syn_interarrival_mean); 
		}else if((res_eql=strcmp(buf,"syn req size")) == 0){
			sscanf(buf + next_eql, "%d",&p->syn_req_size); 
		}else if((res_eql=strcmp(buf,"syn req count")) == 0){
			sscanf(buf + next_eql, "%d",&p->syn_req_count); 	
		}else{
			printf("don't match\t %s\n",buf);
		}
		
		memset(buf,0,BUFSIZE);
		
	}
	fclose(fp);
	

	return p;
}
