#include <fstream>
#include <math.h>
#include <iostream>
#include "ssd.hh"
using namespace std;
#define EPOCH_LENGTH (int64_t) 100000000000 // 00// 1 sec 

int  main(int argc, char * argv[]){
	ssd_info *ssd= new ssd_info(); 
	
	if (argc < 10) return 0; 
	ssd=initiation(ssd, argv);	
	
	full_sequential_write(ssd);
	ssd=simulate(ssd);
		
	for (int cd = 0; cd < ssd->parameter->consolidation_degree; cd++){
		collect_gc_statistics(ssd, cd);
		print_epoch_statistics(ssd, cd);
		print_statistics(ssd, cd);
	}

	close_files(ssd); 
	free_all_node(ssd);	
	printf("\nthe simulation is completed!\n");
	return 1;
}
ssd_info *simulate(ssd_info *ssd){

	int flag=0;
	unsigned int a=0,b=0;

	printf("\n");
	printf("begin simulating.......................\n");
	printf("\n");

	int cd; 
	for (cd = 0; cd < ssd->parameter->consolidation_degree; cd++){
		ssd->tracefile[cd] = fopen(ssd->tracefilename[cd],"r");
		if(ssd->tracefile[cd] == NULL)
		{  
			printf("the trace file can't open\n");
			return NULL;
		}
	}

	
	int i = 0; 
	while(flag!=100)      
	{ 
		flag=get_requests_consolidation(ssd);
		
		if(flag == 1)
		{   
			no_buffer_distribute(ssd);
		}
		
		process(ssd); 
		
		trace_output(ssd);   

		if(flag == 0 && ssd->request_queue == NULL){ 
			flag = 100;
		}
		
		
		static int second = 0; 
		if (ssd->current_time / (EPOCH_LENGTH) > second){
			for (int cd = 0; cd < ssd->parameter->consolidation_degree; cd++)
				print_epoch_statistics(ssd, cd);
			while (ssd->current_time / (EPOCH_LENGTH) > second)
				second++; 
		}
		
	}
	
	return ssd;
}

int add_fetched_request(ssd_info * ssd, request * request1, uint64_t nearest_event_time) {
	if (request1 == NULL){	// no request before nearest_event_time	
		if (ssd->current_time <= nearest_event_time){
			if ((ssd->request_queue_length >= ssd->parameter->queue_length) && 
			 	(nearest_event_time == MAX_INT64)){
				printf("2. HERE\n"); 
			}
			else {
				ssd->current_time = nearest_event_time;
			}
		}
		return -1; 
	}
	
	if (request1->time == MAX_INT64){ // nothing left to read 
		delete request1;
		if (ssd->current_time < nearest_event_time)  
			ssd->current_time = nearest_event_time;  
		return 0; 
	}
	
	if (ssd->current_time <= request1->time) {
		ssd->current_time=request1->time ;   
		if (ssd->current_time == MAX_INT64) 
			printf("3. HERE \n");
	}
	
	add_to_request_queue(ssd, request1);
	if (request1->io_num % 100000 == 0){
		printf(" fetching io request number: %d \n", request1->io_num);
	}
	return 1; 
}

double expo_dist(double const exp_dist_mean ) {
	double uniform = (rand() % 100) / 100.0;
	double time_int = -1 * (log(uniform) / 0.434294) / (1.0/exp_dist_mean); 
	return time_int;
}

request * generate_next_request(ssd_info * ssd, int64_t nearest_event_time){
	if (ssd->request_queue_length >= ssd->parameter->queue_length){
		return NULL; 
	}
	
	static int seq_number = 0; 
	static uint64_t previous_time = 0; 
	double rd_ratio = ssd->parameter->syn_rd_ratio; 
	int lun_number = ssd->parameter->lun_num; 
	uint64_t min_sector_address = 0; 
	uint64_t max_sector_address = lun_number * ssd->parameter->plane_lun * 
					ssd->parameter->block_plane * 
					ssd->parameter->page_block * 
					ssd->parameter->subpage_page; 
	
	uint64_t size = ssd->parameter->syn_req_size; 
	uint64_t avg_time = ssd->parameter->syn_interarrival_mean; // 1ms 
	int event_count = ssd->parameter->syn_req_count; 

	// Time 
 	uint64_t new_time = 0;
 	uint64_t time_interval = expo_dist(avg_time);
 	new_time = previous_time + time_interval;
	
	if (new_time > nearest_event_time) 
		return NULL; 
	
 	previous_time = new_time;
	request * request1 = new request();
	request1->app_id = 0; 
	request1->io_num = seq_number;  
	request1->time = new_time; 
	request1->begin_time = new_time;  
 	
 	// Operation
 	int r = rand() % 100;
 	if (r < (rd_ratio*100)) {
		request1->operation = 1; // Read  
 	}else {
		request1->operation = 0; // write 
 	}
 
 	// Address 
 	uint64_t address = (((rand() % (max_sector_address - min_sector_address)) + min_sector_address ) / 16) * 16;
	request1->lsn = address; 
 
 	// Size 
 	int req_size = size;
	request1->size = size;  

	if (request1->io_num > event_count) {
		request1->time = MAX_INT64; 
	}else {
		seq_number++; 
	}
	if (seq_number %10000 == 0) {
		cout << "seq number: " << seq_number << "  erase count: " << ssd->flash_erase_count << " wq size: " << ssd->channel_head[0].lun_head[0].wsubs_queue.size << "  q length: " << ssd->parameter->queue_length << endl; 
	} 
	return request1; 

}

int get_requests_consolidation(ssd_info *ssd)  {  
	int64_t  nearest_event_time=find_nearest_event(ssd);
	struct request *request1;
	bool trace_based = false; 
	if (trace_based) { // trace or synthetic 
	
		restart_trace_files(ssd); 
		int selected = select_trace_file(ssd);
		if (selected == -1 ){	// all files are done (including repeating files)
			if (nearest_event_time != MAX_INT64) 
				ssd->total_execution_time = ssd->current_time; 
		
			ssd->current_time = nearest_event_time;
		
			if (ssd->current_time == MAX_INT64) 
				printf("1. HERE \n");
		
			return 0; 
		}

		// read request before nearest_event_time, if any 
		request1 = read_request_from_file(ssd, selected, nearest_event_time);
	} else {
		request1 = generate_next_request(ssd, nearest_event_time);
	}
	int ret = add_fetched_request(ssd, request1, nearest_event_time); 
	return ret; 
}
int select_trace_file(ssd_info * ssd){
	char buffer[200];
	unsigned int lsn=0;
	int device,  size, ope,large_lsn, i = 0,j=0;

	long filepoint;
	int64_t time_tt = MAX_INT64;
	int app_id = 0; 
	int io_num = 0; 
	int file_desc; 
	char * type = new char[5]; 
	
	
	int64_t min_time_tt = MAX_INT64;
	int selected = -1; 
	for (int cd = 0; cd < ssd->parameter->consolidation_degree; cd++){
		
		//if (feof(ssd->tracefile[cd])) {continue; }
		time_tt = MAX_INT64; 
		filepoint = ftell(ssd->tracefile[cd]);		
		sprintf(buffer, ""); 
		fgets(buffer, 200, ssd->tracefile[cd]); 
		sscanf(buffer,"%d %lld %d %d %s %d %d %d",&io_num,&time_tt,&device,&file_desc,type,&lsn,&size,&app_id); 
		
		if ((feof(ssd->tracefile[cd]) || time_tt == MAX_INT64) && (ssd->repeat_times[cd] < ssd->parameter->repeat_trace )){
			
			restart_trace_files(ssd);
			
			filepoint = ftell(ssd->tracefile[cd]);		
			sprintf(buffer, ""); 
			fgets(buffer, 200, ssd->tracefile[cd]); 
			sscanf(buffer,"%d %lld %d %d %s %d %d %d",&io_num,&time_tt,&device,&file_desc,type,&lsn,&size,&app_id); 

		}
		
		if (time_tt < MAX_INT64){
			time_tt = time_tt * ssd->parameter->time_scale; 
			time_tt = time_tt + ssd->last_times[cd];
			if (time_tt > MAX_INT64 || time_tt < 0)
				time_tt = MAX_INT64; 
		}
			
		if (time_tt < min_time_tt) {
			min_time_tt = time_tt;
			selected = cd; 
		}
		if (time_tt != MAX_INT64)
			fseek(ssd->tracefile[cd],filepoint,0); 
	}
	
	return selected; 
	
}
void restart_trace_files(ssd_info * ssd){
	int cd; 
	
	// If we need to restart trace file 
	for (cd = 0; cd < ssd->parameter->consolidation_degree; cd++) 
	{
		if(feof(ssd->tracefile[cd]) && (ssd->repeat_times[cd] < ssd->parameter->repeat_trace ))
		{

			ssd->total_execution_time = ssd->current_time; 
			printf("1. total time %0.1f sec , round %d of application %d \n", ssd->total_execution_time/1000000000.0, ssd->repeat_times[cd], cd); 
			
			ssd->repeat_times[cd]++; 
			ssd->last_times[cd] = ssd->current_time; 
			fseek(ssd->tracefile[cd],0,SEEK_SET); 
		
			collect_gc_statistics(ssd, cd);
			//print_epoch_statistics(ssd, cd);
			//print_statistics(ssd, cd); 
		}
	}	
}
request * read_request_from_file(ssd_info * ssd, int selected, int64_t nearest_event_time){
	
	if (ssd->request_queue_length >= ssd->parameter->queue_length){
		return NULL; 
	}
	
	char buffer[200];
	unsigned int lsn=0;
	long int size, ope,large_lsn, i = 0,j=0;
	int device = 0; 
	request *request1;
	long filepoint;
	int64_t time_tt = MAX_INT64;
	int app_id = 0; 
	int io_num = 0; 
	int file_desc; 
	char * type = new char[5]; 
	
	time_tt = MAX_INT64; lsn=0; 
	filepoint= ftell(ssd->tracefile[selected]);		
	sprintf(buffer, ""); 
	fgets(buffer, 200, ssd->tracefile[selected]); 
	sscanf(buffer,"%d %lld %d %d %s %d %ld %d",&io_num,&time_tt,&device,&file_desc,type,&lsn,&size,&app_id); 
	if (time_tt != MAX_INT64){
		time_tt = time_tt * ssd->parameter->time_scale; 
		time_tt = time_tt + ssd->last_times[selected];
		if (time_tt > MAX_INT64 || time_tt < 0)
			time_tt = MAX_INT64; 
	}
	
	if (time_tt > nearest_event_time){
		fseek(ssd->tracefile[selected],filepoint,0);
		return NULL; 
	}
	
	//lsn = (lsn % ADDRESS_CHUNK) + (selected * ADDRESS_CHUNK); 	
	if (strcasecmp(type, "Read") == 0)
		ope = 1; 
	else 
		ope = 0; 
	
	if ((device<0)&&(size<0)&&(ope<0))
	{
	
		return NULL; 
	}
	
	
	if (lsn<ssd->min_lsn) 
		ssd->min_lsn=lsn;
	if (lsn>ssd->max_lsn)
		ssd->max_lsn=lsn;

	large_lsn=(long int)(((long int)ssd->parameter->subpage_page*ssd->parameter->page_block*ssd->parameter->block_plane*ssd->parameter->plane_lun*ssd->parameter->lun_num)*(1-ssd->parameter->overprovide));
	lsn = lsn%large_lsn;

	request1 = new request();
	
	request1->app_id = selected;
	request1->time = time_tt; 
	request1->lsn = lsn;
	request1->size = size;
	request1->io_num = io_num; 
	request1->operation = ope;	
	request1->begin_time = time_tt; 
	
	return request1; 
	
}
void add_to_request_queue(ssd_info * ssd, request * req){
	if(ssd->request_queue == NULL)          //The queue is empty
	{
		ssd->request_queue = req;
		ssd->request_tail = req;
		ssd->request_queue_length++;
	}
	else
	{			
		(ssd->request_tail)->next_node = req;	
		ssd->request_tail = req;			
		ssd->request_queue_length++;
	}

	if (req->operation==1)       
	{
		ssd->read_request_size[req->app_id]=
			(ssd->read_request_size[req->app_id]*
			ssd->read_request_count[req->app_id]+req->size)/
			(ssd->read_request_count[req->app_id]+1);
	} 
	else
	{
		ssd->write_request_size[req->app_id]=
			(ssd->write_request_size[req->app_id]*
			ssd->write_request_count[req->app_id]+req->size)/
			(ssd->write_request_count[req->app_id]+1);
	}
	
}
void collect_statistics(ssd_info * ssd, request * req){
	
	sub_request * sub; 
	sub = req->subs; //  critical_sub;
	 
	while (sub != NULL) 
	{
		if (sub->complete_time != req->response_time) { sub = sub->next_subs; continue;  }
	for (unsigned int i = 0; i < SR_MODE_NUM; i++){
			ssd->subreq_state_time[i] += sub->state_time[i];
		}
		
		sub = sub->next_subs; 

	}
	
	if(req->response_time-req->time==0)
	{
		printf("1. the response time is 0?? %lld %lld \n", req->response_time, req->time);
		getchar();
	}

	if (req->operation==READ)
	{
		ssd->read_request_size[req->app_id] += req->size; 
		ssd->read_request_count[req->app_id]++; 
		ssd->read_avg[req->app_id] += (req->response_time-req->time);
		if (req->response_time - req->time > ssd->read_worst_case_rt){
			ssd->read_worst_case_rt = req->response_time - req->time; 
		}
	} 
	else
	{
		ssd->write_request_size[req->app_id] += req->size;
		ssd->write_request_count[req->app_id]++;
		ssd->write_avg[req->app_id]+=(req->response_time - req->time);

		if (req->response_time - req->time > ssd->write_worst_case_rt){
			ssd->write_worst_case_rt = req->response_time - req->time; 
		}
	}

}
void print_epoch_statistics(ssd_info * ssd, int app_id){
	int epoch_num = ssd->current_time / EPOCH_LENGTH; 
	// =================== LATENCY ================================================================
	long int rw_count = ssd->write_request_count[app_id] + ssd->read_request_count[app_id]; if(rw_count == 0) return;
	int64_t RT = ((ssd->write_avg[app_id] / rw_count) + (ssd->read_avg[app_id] / rw_count));
	int64_t read_RT = (ssd->read_request_count[app_id] > 0) ? ssd->read_avg[app_id] / ssd->read_request_count[app_id] : 0; 
	int64_t write_RT = (ssd->write_request_count[app_id] > 0) ? ssd->write_avg[app_id] / ssd->write_request_count[app_id] : 0; 
	long int prev_total_rw = ssd->total_read_request_count[app_id] + ssd->total_write_request_count[app_id];
	long int prev_total_r = ssd->total_read_request_count[app_id]; 
	long int prev_total_w = ssd->total_write_request_count[app_id]; 
	ssd->total_read_request_count[app_id] += ssd->read_request_count[app_id];
	ssd->total_write_request_count[app_id] += ssd->write_request_count[app_id];
	long int next_total_rw = ssd->total_read_request_count[app_id] + ssd->total_write_request_count[app_id]; 
	long int next_total_r = ssd->total_read_request_count[app_id]; 
	long int next_total_w = ssd->total_write_request_count[app_id]; 
	ssd->total_flash_erase_count += ssd->flash_erase_count;
	ssd->total_RT[app_id] = ((ssd->total_RT[app_id] * (double)prev_total_rw) + ssd->read_avg[app_id] + ssd->write_avg[app_id]) / (next_total_rw);
	ssd->total_read_RT[app_id] = ((ssd->total_read_RT[app_id] * (double)prev_total_r ) + ssd->read_avg[app_id]) / next_total_r;  	
	ssd->total_write_RT[app_id] = ((ssd->total_write_RT[app_id] * (double)prev_total_w ) + ssd->write_avg[app_id]) / next_total_w;  	

	fprintf(ssd->statisticfile, "LATENCY %d %d RT %lld , count  %ld\n", epoch_num, app_id, RT, rw_count);
	fprintf(ssd->statisticfile, "LATENCY %d %d read RT %lld , count %d\n", epoch_num, app_id, read_RT, ssd->read_request_count[app_id]);
	fprintf(ssd->statisticfile, "LATENCY %d %d write RT %lld , count %d\n", epoch_num, app_id, write_RT, ssd->write_request_count[app_id]);
	fprintf(ssd->statisticfile, "LATENCY %d %d read worst case %lld \n", epoch_num, app_id, ssd->read_worst_case_rt);
	fprintf(ssd->statisticfile, "LATENCY %d %d write worst case %lld \n", epoch_num, app_id, ssd->write_worst_case_rt);
	fprintf(ssd->statisticfile, "LATENCY %d %d erase(gc) count %d , total %d\n", epoch_num, app_id, ssd->flash_erase_count, ssd->total_flash_erase_count);
	fprintf(ssd->statisticfile, "LATENCY %d %d Total RT %lld , count %ld \n", epoch_num, app_id, ssd->total_RT[app_id], next_total_rw);
	fprintf(ssd->statisticfile, "LATENCY %d %d Total read RT %lld , count %ld \n", epoch_num, app_id, ssd->total_read_RT[app_id], next_total_r);
	fprintf(ssd->statisticfile, "LATENCY %d %d Total write RT %lld , count %ld \n", epoch_num, app_id, ssd->total_write_RT[app_id], next_total_w);

	ssd->read_request_count[app_id] = 0;
	ssd->write_request_count[app_id] = 0;
	ssd->read_avg[app_id] = 0;
	ssd->write_avg[app_id] = 0;
	ssd->flash_erase_count = 0;
	ssd->gc_moved_page = 0;

	
	
	// =================== SUBREQ STATES ===========================================================
	fprintf(ssd->statisticfile, "SUBREQ %d \t", epoch_num); 
	
	for (int i = 0; i < SR_MODE_NUM; i++){
		fprintf(ssd->statisticfile, "%lld\t",ssd->subreq_state_time[i]); 
		ssd->subreq_state_time[i] = 0; 
	}
	fprintf(ssd->statisticfile, "\n"); 
	
	// =================== LUN STATES ============================================================
	fprintf(ssd->statisticfile, "LUN epoch: %d appid: %d , LUN_IO LUN_GC \n", epoch_num, app_id); 
	for (int i = 0; i < ssd->parameter->lun_num; i++){
		int channel_num = i % ssd->parameter->channel_number;
		int lun_num = (i / ssd->parameter->channel_number) % ssd->parameter->lun_channel[channel_num];
		lun_info * the_lun = &ssd->channel_head[channel_num].lun_head[lun_num];

		fprintf(ssd->statisticfile, "LUN STAT (%d,%d) ",  channel_num, lun_num); 
		for (int i = 0; i < LUN_MODE_NUM; i++){
			fprintf(ssd->statisticfile, "%lld\t", the_lun->state_time[i]); 
			the_lun->state_time[i] = 0; 
		}
		fprintf(ssd->statisticfile, "\n");
		for (int j = 0; j < ssd->parameter->plane_lun; j++){
			int plane_num = j; 
		
			plane_info * the_plane = &ssd->channel_head[channel_num].lun_head[lun_num].plane_head[plane_num];
			fprintf(ssd->statisticfile, "PLANE STAT (%d,%d,%d) ", channel_num, lun_num, plane_num);
			
			for (int i = 0; i < PLANE_MODE_NUM; i++){
				fprintf(ssd->statisticfile, "%lld\t",the_plane->state_time[i]); 
				the_plane->state_time[i] = 0; 
			}
			fprintf(ssd->statisticfile, "\n");
		}
			 
	}
	// ==================== OTHER STATS ===================================
	fprintf(ssd->statisticfile, "Multiplane read %d , write %d , erase %d \n", ssd->read_multiplane_count , ssd->write_multiplane_count , ssd->erase_multiplane_count); 
	
}
void print_statistics(ssd_info *ssd, int app){
	
	//ssd->total_read_request_count[app] += ssd->read_request_count[app];
	//ssd->total_write_request_count[app] += ssd->write_request_count[app];
	//ssd->total_read_avg[app] += ssd->read_avg[app];
	//ssd->total_write_avg[app] += ssd->write_avg[app];
	//ssd->total_flash_erase_count += ssd->flash_erase_count; 
	
	
	fprintf(ssd->statisticfile, "======== Round %d for application %d , time %lld ============ \n", ssd->repeat_times[app], app, ssd->total_execution_time); 
	
	long int rw_count = ssd->total_read_request_count[app] + ssd->total_write_request_count[app]; 
	long int r_count = ssd->total_read_request_count[app];
	long int w_count = ssd->total_write_request_count[app]; 
	
	fprintf(ssd->statisticfile, "request average response time[%d]: ( %lld ) , count %ld \n", app, ssd->total_RT[app] ,rw_count);
	fprintf(ssd->statisticfile, "request average read response time[%d]: ( %lld ) , count %ld \n", app, ssd->total_read_RT[app] ,r_count);
	fprintf(ssd->statisticfile, "request average write response time[%d]: ( %lld ) , count %ld \n", app, ssd->total_write_RT[app] ,w_count);

	fprintf(ssd->statisticfile,"erase: ( %13d )\n", ssd->total_flash_erase_count);
	fprintf(ssd->statisticfile,"total gc move count: %d \n", ssd->gc_moved_page);
	
	
 
	fprintf(ssd->statisticfile, "========================\n"); 
	
	ssd->read_request_count[app] = 0;
	ssd->write_request_count[app] = 0;
	ssd->read_avg[app] = 0;
	ssd->write_avg[app] = 0;
	ssd->flash_erase_count = 0; 
	ssd->gc_moved_page = 0; 
	
	// Print lun statistic output
		
	int chan,lun = 0; 
	
	
	fprintf(ssd->statisticfile, "lun read count \n"); 
	for (chan = 0; chan < ssd->parameter->channel_number; chan++){
		for (lun = 0; lun < ssd->channel_head[chan].lun; lun++){
			fprintf(ssd->statisticfile, "%d\t", ssd->channel_head[chan].lun_head[lun].read_count); 
		}
		fprintf(ssd->statisticfile, "\n"); 
	}
	
	fprintf(ssd->statisticfile, "\n\nlun write count \n"); 
	
	for (chan = 0; chan < ssd->parameter->channel_number; chan++){
		for (lun = 0; lun < ssd->channel_head[chan].lun; lun++){
			fprintf(ssd->statisticfile, "%d\t", ssd->channel_head[chan].lun_head[lun].program_count); 
		}
		fprintf(ssd->statisticfile, "\n"); 
	}
	
	fprintf(ssd->statisticfile, "\n\nlun erase count \n"); 
	
	for (chan = 0; chan < ssd->parameter->channel_number; chan++){
		for (lun = 0; lun < ssd->channel_head[chan].lun; lun++){
			fprintf(ssd->statisticfile, "%d\t", ssd->channel_head[chan].lun_head[lun].erase_count); 
		}
		fprintf(ssd->statisticfile, "\n"); 
	}


	int plane = 0; 
	for (chan = 0; chan < ssd->parameter->channel_number; chan++){
		for (lun = 0; lun < ssd->channel_head[chan].lun; lun++){
			for (plane = 0; plane < ssd->parameter->plane_lun; plane++){
				int block = 0;
				double sum = 0; 
				double avg = 0;  
				for (block = 0; block < ssd->parameter->block_plane; block++){
					sum += ssd->channel_head[chan].lun_head[lun].plane_head[plane].blk_head[block].erase_count; 
				}
				avg = sum / ssd->parameter->block_plane; 

				fprintf(ssd->statisticfile, "(cwdp %d%d%d avg: %f )\t ", chan, lun, plane, avg);
				sum = 0; 
				for (block = 0; block < ssd->parameter->block_plane; block++){
					double temp= ssd->channel_head[chan].lun_head[lun].plane_head[plane].blk_head[block].erase_count - avg; 
					temp = temp * temp; 
					sum += temp; 
				}
				double variance = sum / ssd->parameter->block_plane; 
				double std_dev = sqrt (variance); 

				fprintf(ssd->statisticfile, "(cwdp %d%d%d standard deviation: %f )\t ", chan, lun, plane, std_dev);
			}
			
		}
	}	

	fprintf(ssd->statisticfile, "\n"); 

	int i, j, k, l; 
	fprintf(ssd->statisticfile, "planes erase number (non-zeros)\n\n"); 
	for(i=0;i<ssd->parameter->channel_number;i++)
	{
		for (j = 0; j < ssd->parameter->lun_channel[i]; j++)
		{
			for(l=0;l<ssd->parameter->plane_lun;l++)
			{
				int plane_erase = ssd->channel_head[i].lun_head[j].plane_head[l].erase_count; 

				fprintf(ssd->statisticfile,"erase (%d,%d,%d):%13d\n",i,j,l, plane_erase);
				
			}
			
			
		}
		
	}

}

void remove_request(ssd_info * ssd, request ** req, request ** pre_node){
	if(*pre_node == NULL)
	{
		if((*req)->next_node == NULL)
		{
			delete (*req); 
			*req = NULL; 
			ssd->request_queue = NULL;
			ssd->request_tail = NULL;
			ssd->request_queue_length--;
		}
		else
		{
			ssd->request_queue = (*req)->next_node;
			(*pre_node) = (*req);
			(*req) = (*req)->next_node;
			delete (*pre_node); 
			*pre_node = NULL; 
			ssd->request_queue_length--;
		}
	}
	else
	{
		if((*req)->next_node == NULL)
		{
			(*pre_node)->next_node = NULL;
			delete (*req); 
			*req = NULL; 
			ssd->request_tail = (*pre_node);
			ssd->request_queue_length--;
		}
		else
		{
			(*pre_node)->next_node = (*req)->next_node;
			delete (*req);  
			(*req) = (*pre_node)->next_node;
			ssd->request_queue_length--;
		}
	}
	
}

void trace_output(ssd_info* ssd){
	int flag = 1;	
	int64_t start_time, end_time; 
	request *req, *pre_node;
	sub_request *sub, *tmp;

	pre_node=NULL;
	req = ssd->request_queue;
	
	if(req == NULL)
		return;
	
	while(req != NULL)	
	{
		if(req->response_time != 0)
		{
			
			collect_statistics(ssd, req); 
			remove_request(ssd, &req, &pre_node);	
		}
		else
		{
			sub = req->subs;
			flag = 1;
			start_time = 0;
			end_time = 0;
			
			while(sub != NULL)
			{
				if(start_time == 0)
					start_time = sub->begin_time;
				if(start_time > sub->begin_time)
					start_time = sub->begin_time;
				if (end_time < sub->complete_time){
					end_time = sub->complete_time;
					req->critical_sub = sub; 
				}
				
				// if any sub-request is not completed, the request is not completed
				if( find_subrequest_state(ssd, sub) == SR_MODE_COMPLETE )
				{
					sub = sub->next_subs;
				}
				else
				{	
					flag=0;
					break;
				}
				
			}		
			
			if (flag == 1)
			{	
				req->response_time = end_time;
				req->begin_time = start_time; 
				
				collect_statistics(ssd, req ); 
				remove_request(ssd, &req, &pre_node);
				
			}
			else
			{	
				// request is not complete yet, go to the next node 
				pre_node = req;
				req = req->next_node;
			}
		}	
	}

}
unsigned int transfer_size(ssd_info *ssd,int need_distribute,unsigned int lpn,request *req){
	unsigned int first_lpn,last_lpn,trans_size;
	uint64_t state; 
	uint64_t mask=0,offset1=0,offset2=0;

	first_lpn=req->lsn/ssd->parameter->subpage_page;
	last_lpn=(req->lsn+req->size-1)/ssd->parameter->subpage_page;

	mask=~(0xffffffffffffffff<<(ssd->parameter->subpage_page));
	state=mask;
	if(lpn==first_lpn)
	{
		offset1=ssd->parameter->subpage_page-((lpn+1)*ssd->parameter->subpage_page-req->lsn);
		state=state&(0xffffffffffffffff<<offset1);
	}
	if(lpn==last_lpn)
	{
		offset2=ssd->parameter->subpage_page-((lpn+1)*ssd->parameter->subpage_page-(req->lsn+req->size));
		state=state&(~(0xffffffffffffffff<<offset2));
	}

	trans_size=size(state&need_distribute);

	return trans_size;
}
int64_t find_nearest_event(ssd_info *ssd) {
	unsigned int i,j;
	int64_t time=MAX_INT64;
	int64_t time1=MAX_INT64;
	int64_t time2=MAX_INT64;
	
	
	
	for (i=0;i<ssd->parameter->channel_number;i++)
	{
		if (ssd->channel_head[i].next_state==CHANNEL_MODE_IDLE)
			if(time1>ssd->channel_head[i].next_state_predict_time)
				if (ssd->channel_head[i].next_state_predict_time>ssd->current_time)    
					time1=ssd->channel_head[i].next_state_predict_time;
		for (j=0;j<ssd->parameter->lun_channel[i];j++)
		{
	
			if (ssd->channel_head[i].lun_head[j].next_state==LUN_MODE_IDLE)
				if(time2>ssd->channel_head[i].lun_head[j].next_state_predict_time)
					if (ssd->channel_head[i].lun_head[j].next_state_predict_time>ssd->current_time)    
						time2=ssd->channel_head[i].lun_head[j].next_state_predict_time;	
		
		}   
	} 

	
	time=(time1>time2)?time2:time1;
	
	
	return time;
}
ssd_info *no_buffer_distribute(ssd_info *ssd){
	unsigned int lsn, lpn, last_lpn, first_lpn, complete_flag = 0; 
	uint64_t state;
	unsigned int flag=0,flag1=1,active_region_flag=0;           //to indicate the lsn is hitted or not
	request *req=NULL;
	sub_request *sub=NULL,*sub_r=NULL,*update=NULL;
	local *loc=NULL;
	channel_info *p_ch=NULL;

	
	uint64_t mask=0; 
	uint64_t offset1=0, offset2=0;
	unsigned int sub_size=0;
	uint64_t sub_state=0;

	
	ssd->dram->current_time=ssd->current_time;
	req=ssd->request_tail;       
	lsn=req->lsn;
	lpn=req->lsn/ssd->parameter->subpage_page;
	last_lpn=(req->lsn+req->size-1)/ssd->parameter->subpage_page;
	first_lpn=req->lsn/ssd->parameter->subpage_page;

	
	if(req->operation==READ)        
	{		
	
		while(lpn<=last_lpn) 		
		{
			sub_state=(ssd->dram->map->map_entry[lpn].state&0x7fffffffffffffff);
			sub_size=size(sub_state);
			sub=create_sub_request(ssd,lpn,sub_size,sub_state,req,req->operation);
	
			lpn++;
		}
	}
	else if(req->operation==WRITE)
	{
		while(lpn<=last_lpn)     	
		{	
			mask=~(0xffffffffffffffff<<(ssd->parameter->subpage_page));
			state=mask;
			if(lpn==first_lpn)
			{
				offset1=ssd->parameter->subpage_page-((lpn+1)*ssd->parameter->subpage_page-req->lsn);
				state=state&(0xffffffffffffffff<<offset1);
				
			}
			if(lpn==last_lpn)
			{
				offset2=ssd->parameter->subpage_page-((lpn+1)*ssd->parameter->subpage_page-(req->lsn+req->size));
				state=state&(~(0xffffffffffffffff<<offset2));
			}

			sub_size=size(state);
			if (sub_size > 32){
				printf("%lld ************** \n", state);
			}
			sub=create_sub_request(ssd,lpn,sub_size,state,req,req->operation);
			lpn++;
		}
	}
	
	return ssd;
}
void collect_gc_statistics(ssd_info * ssd, int app){
	// FIXME 
}
void free_all_node(ssd_info *ssd){
		delete ssd; 
}
void close_files(ssd_info * ssd) {
	for (int cd = 0; cd < ssd->parameter->consolidation_degree; cd++) 
		fclose(ssd->tracefile[cd]);
	
	fflush(ssd->statisticfile);
	fclose(ssd->statisticfile);
}
