#ifndef _COMMON_H_
#define _COMMON_H_ 10000

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <iostream>
using namespace std; 

#define BUFSIZE 200
#define PG_SUB 0xffffffff			
#define EPOCH_LENGTH (int64_t) 100000000000 
#define MAX(A,B) (A>B)?A:B;  
#define NSEC 1000000000

#define MAX_INT64  0x7fffffffffffffffll

enum GC_PRIORITY {GC_EARLY, GC_ONDEMAND}; 
enum GC_STATES {GC_WAIT, GC_ERASE_C_A, GC_COPY_BACK, GC_COMPLETE}; 
enum OPERATIONS {WRITE = 0, READ, ERASE, NOOP_READ, NOOP_WRITE, NOOP, OP_NUM}; 
enum PLANE_LEVEL_PARALLEL {BASE, GCIO, IOGC, GCGC}; 
enum CHANNEL_MODE {CHANNEL_MODE_IDLE, CHANNEL_MODE_GC, CHANNEL_MODE_IO, CHANNEL_MODE_NUM}; 
enum LUN_MODE {LUN_MODE_IDLE, LUN_MODE_GC, LUN_MODE_IO, LUN_MODE_NUM}; 
enum PLANE_MODE {PLANE_MODE_IDLE, PLANE_MODE_GC, PLANE_MODE_IO, PLANE_MODE_NUM}; 
enum SUBREQ_MODE {SR_MODE_WAIT, SR_MODE_ST_S, SR_MODE_ST_M, SR_MODE_IOC_S, SR_MODE_IOC_O, SR_MODE_IOC_M, SR_MODE_GCC_S, SR_MODE_GCC_O, SR_MODE_GCC_M, SR_MODE_COMPLETE, SR_MODE_NUM}; 
enum PARGC_APPROACH{BLIND = 0, PATTERN, COST_AWARE, DYN_COST_AWARE, PREEMPTIVE, CACHE_INVOLVED, PRE_MOVE, NO_PGC};
enum STATE {ERROR = -1, FAIL, SUCCESS};
enum GC_ALG{GREEDY, FIFO, WINDOWED, RGA, RANDOM, RANDOMP, RANDOMPP, SIMILAR};

enum LUN_STATE {LUN_STATE_IDLE, LUN_STATE_HIO, LUN_STATE_FIO, LUN_STATE_HGC, LUN_STATE_FGC, LUN_STATE_IOGC, LUN_STATE_NUM}; // to keep track of utilization 


class ac_time_characteristics{
public: 
	int tPROG;     //program time
	int tDBSY;     //bummy busy time for two-plane program
	int tBERS;     //block erase time
	int tCLS;      //CLE setup time
	int tCLH;      //CLE hold time
	int tCS;       //CE setup time
	int tCH;       //CE hold time
	int tWP;       //WE pulse width
	int tALS;      //ALE setup time
	int tALH;      //ALE hold time
	int tDS;       //data setup time
	int tDH;       //data hold time
	int tWC;       //write cycle time
	int tWH;       //WE high hold time
	int tADL;      //address to data loading time
	int tR;        //data transfer from cell to register
	int tAR;       //ALE to RE delay
	int tCLR;      //CLE to RE delay
	int tRR;       //ready to RE low
	int tRP;       //RE pulse width
	int tWB;       //WE high to busy
	int tRC;       //read cycle time
	int tREA;      //RE access time
	int tCEA;      //CE access time
	int tRHZ;      //RE high to output hi-z
	int tCHZ;      //CE high to output hi-z
	int tRHOH;     //RE high to output hold
	int tRLOH;     //RE low to output hold
	int tCOH;      //CE high to output hold
	int tREH;      //RE high to output time
	int tIR;       //output hi-z to RE low
	int tRHW;      //RE high to WE low
	int tWHR;      //WE high to RE low
	int tRST;      //device resetting time
};


class Tuple {
public: 
	int64_t active_time; 
	int64_t total_capacity; 
	int64_t total_count; 
	int64_t last_time; 

	Tuple (){
		active_time = 0; 
		total_capacity = 0; 
		total_count = 0; 
	}

	void add_time(int64_t start, int64_t end) {
		if (start <=  last_time  && end >= last_time){ 
			active_time += end - last_time; 
			last_time = end; 
		}
		else if (start > last_time && end > last_time) {
			active_time += end - start; 
			last_time = end; 
		}
	}
	void add_capacity(int64_t cap) {total_capacity += cap;} // in sector
	void add_count (int64_t count){ total_count += count; }

	double get_IOPS(){ // per second
		if (active_time == 0) return 0;
		return (double)total_count * NSEC / last_time; // active_time; 
	}	
	double get_BW(){ // MB/s
		if (active_time == 0) return 0; 
		return (double)total_capacity * NSEC / (last_time /*active_time*/ * 2 * 1024); 
	}

}; 

class parameter_value{
public: 
	parameter_value(int argc, char ** argv);
	void load_parameters(char * parameter_filename);  
	void print_all_parameters(FILE * stat_file); 
	char filename[100]; 
	unsigned int consolidation_degree; 
	int MP_address_check; 
	int repeat_trace; 
	int mplane_gc; 
	int gc_algorithm; 
	int queue_length; 
	double time_scale; 
	unsigned int lun_num;          
	unsigned int dram_capacity;    
	unsigned int cpu_sdram;        

	unsigned int channel_number;    
	unsigned int lun_channel[100]; 
   
	unsigned int plane_lun;
	unsigned int block_plane;
	unsigned int page_block;
	unsigned int subpage_page;

	unsigned int page_capacity;
	unsigned int subpage_capacity;

	unsigned int ers_limit;         
	int address_mapping;           
	int wear_leveling;              
	int gc;                        
	int clean_in_background;       
	int alloc_pool;                 //allocation pool 
	float overprovide;
	float gc_threshold;             
	int scheduling_algorithm;       
	float quick_radio;
	int related_mapping;

	unsigned int time_step;
	unsigned int small_large_write; //the threshould of large write, large write do not occupt buffer, which is written back to flash directly

	int striping;                 
	int interleaving;
	int pipelining;
	int active_write;               //yes;0,no
	float gc_up_threshold;        
	float gc_down_threshold; 
	float gc_mplane_threshold;
	int advanced_commands;  
	int pargc_approach; 

	ac_time_characteristics time_characteristics;
	
	float syn_rd_ratio; 
	int syn_req_count; 
	int syn_req_size;
	int syn_interarrival_mean;  

	int plane_level_tech; 	
	
};

class local{        
public: 
	local(int c, int w, int p){
		channel = c; 
		lun = w; 
		plane = p; 
	}
	local(int c, int w, int p, int b, int pg ) {
		channel = c; 
		lun = w; 
		plane = p; 
		block = b; 
		page = pg; 
	} 
	
	~local(){}
	void print () const {
		cout << "channel: " << channel << " lun: " << lun << " plane: "<< plane << " block:"<< block << " page: " << page << endl; 
	}
	int channel;
	int lun;
	int plane;
	int block;
	int page;
	int sub_page;
};

/********************************************************
*mapping information,state
*********************************************************/

class buffer_entry{
public: 
	bool modified; 
	bool evicted; 
	int lpn; 
	int read_hit; 
	int write_hit; 
	int trim_hit; 
	buffer_entry(){
		modified = false; 
		evicted = false; 
		lpn = -1;  
		read_hit = 0; 
		write_hit = 0; 
		trim_hit = 0; 
	}
	buffer_entry(int l){
		buffer_entry(); 
		lpn = l; 
	}
	buffer_entry * next_entry; 
	buffer_entry * prev_entry; 	
}; 

class entry{  
public: 
	entry(){
		pn = -1; 
		state = false; 
		buf_ent = NULL; 
	}                     
	int pn;   
	bool state;
	buffer_entry * buf_ent;                  
};

class map_info{
public: 
	~map_info(){
		delete map_entry; 
	//	delete attach_info; 
	}

	entry *map_entry;        //each entry indicate a mapping information
};
class gc_operation{   
public:      
	gc_operation(local * loc, int gc_seq_num){
		next_node = NULL; 
		state = GC_WAIT; 
		if (loc == NULL) return; 
		location = new local(loc->channel, loc->lun, loc->plane, 0, 0); 
		seq_number = gc_seq_num; 
	}
	~gc_operation(){
	//	if (location ) delete location; 
	}
	local * location;
	unsigned int seq_number;  
	unsigned int state;          
	unsigned int priority;    
	gc_operation *next_node;
};
class write_buffer{
public: 
	int buffer_capacity; 
	int entry_count; 
	buffer_entry * buffer_head; 
	buffer_entry * buffer_tail; 
	write_buffer(int size, int sector_page){ // size in MB , sector_page total number of sector in a page (16 means 8KB page) 
		buffer_capacity = size * 1024 * 2 / sector_page; 
		entry_count = 0; 
		buffer_head = NULL; 
		buffer_tail = NULL; 
	}
	bool check_buffer(){
		if (buffer_capacity == 0) return true; 
		int count = 0; 
		if ((buffer_head == NULL || buffer_tail == NULL) && entry_count != 0) return false; 
		if (buffer_head == NULL && buffer_tail == NULL && entry_count == 0) return true; 

		if (buffer_head->prev_entry != NULL || buffer_tail->next_entry != NULL) return false; 
		buffer_entry * entry = buffer_head; 
		while (entry != NULL) {
			count++; 
			entry = entry->next_entry; 
		}
		if (count != entry_count || count > buffer_capacity) return false; 
		count = 0; 
		entry = buffer_tail; 
		while (entry != NULL) {
			count++; 
			entry = entry->prev_entry; 
		}
		if (count != entry_count || count > buffer_capacity) return false; 	 
	
		return true;  
	}
	
	buffer_entry * select_eviction(){
		buffer_entry * evict_candidate = buffer_tail; 
		if (evict_candidate == NULL) return NULL; 
		while(evict_candidate != NULL) {
			if (evict_candidate->evicted) evict_candidate = evict_candidate->prev_entry;
			else break;  
		}
		if (evict_candidate != NULL)  evict_candidate->evicted = true; 
		return evict_candidate; 
	}
	
	bool need_eviction(){
		if (entry_count >= (buffer_capacity * 80 / 100) ) return true; 
		return false; 
	}

	bool add_tail(buffer_entry * entry){ 
		if (entry_count >= buffer_capacity){return false;}
		if (buffer_tail == NULL) {entry_count++; buffer_tail = entry; buffer_head = entry; return true; }
		buffer_tail->next_entry = entry; 
		entry->prev_entry = buffer_tail; 
		buffer_tail = entry;
		entry_count++;  
		return true; 
	}
	
	bool add_head(buffer_entry * entry){
		if (entry_count >= buffer_capacity){return false;}
		if (buffer_head == NULL) {
			entry_count++; 
			buffer_head = entry; 
			buffer_tail = entry; 
			buffer_head->prev_entry = NULL; // just to make sure everything is fine 
			buffer_tail->next_entry = NULL; // just to make sure everything is fine 
			return true; 
		}
		buffer_head->prev_entry = entry; 
		entry->next_entry = buffer_head; 
		buffer_head = entry; 
		buffer_head->prev_entry = NULL; 
		entry_count++;
		return true; 
	}
	buffer_entry * add_head(int lpn){
		buffer_entry * buf_ent = new buffer_entry(lpn); 
		if (!add_head(buf_ent)) return NULL; 
		return buf_ent; 
	}	
	buffer_entry * add_tail(int lpn){
		buffer_entry * buf_ent = new buffer_entry(lpn); 
		if (!add_tail(buf_ent)) return NULL; 
		return buf_ent; 
	}

	buffer_entry * remove_entry (buffer_entry * entry){
		if (entry == NULL) return NULL; 
		if (entry_count == 0) return NULL; 
		if (entry == buffer_head) {
			return remove_head(); 
		}
		if (entry == buffer_tail){
			return remove_tail();  
		}
		entry->prev_entry->next_entry = entry->next_entry; 
		entry->next_entry->prev_entry = entry->prev_entry;
		entry->next_entry = NULL;
		entry->prev_entry = NULL; 
		entry_count--;  
		return entry; 
	}
	buffer_entry * remove_head(){
		if (entry_count == 0) return NULL; 
		buffer_entry * temp = buffer_head; 
		buffer_head = temp->next_entry; 
		buffer_head->prev_entry = NULL; 
		temp->next_entry = NULL; 
		entry_count--; 	
		return temp; 
	}
	buffer_entry * remove_tail(){
		if (entry_count == 0) return NULL; 
		buffer_entry * temp = buffer_tail; 
		buffer_tail = temp->prev_entry; 
		buffer_tail->next_entry = NULL; 
		temp->prev_entry = NULL;  
		entry_count--;
		return temp; 
	}

	bool is_full(){
		if (entry_count < buffer_capacity) return false; 
		return true; 
	}
	int evict(){ 
		int lpn = buffer_tail->lpn; 
		buffer_entry * buf_tail = remove_tail(); 
		delete buf_tail; 
		return lpn; 
	}
	void hit_read(buffer_entry * entry){
		entry->read_hit++;
		add_head(remove_entry(entry)); 
	}
	void hit_write(buffer_entry * entry){
		entry->write_hit++; 
		entry->modified = true; 
		add_head(remove_entry(entry)); 
	}
	void hit_trim(buffer_entry * entry){
		entry->trim_hit++; 	
		buffer_entry * buf_ent = remove_entry(entry); 
		delete buf_ent; // useless to increase trim hit, but anyways 
	}
}; 


class dram_info{
public:
	dram_info(parameter_value * parameter); 
	~dram_info(){
		delete map; 
	}
	unsigned int dram_capacity;     
	int64_t current_time;
	map_info *map;
	write_buffer * buffer; 	
};

class sub_request{
public: 
 
	sub_request(int64_t ct, int lpnum, int s, int sn, int op){	
		app_id = -1; 
		io_num = -1; 
		seq_num = sn;  
		lpn = lpnum; 
		ppn = -1;
		buf_entry = NULL; 
		operation = op;  
		size = s; 
		state = 0; 
		begin_time = ct; 
		wait_time = 0; 
		complete_time = MAX_INT64; 
		gc_node = NULL; 

		next_node = NULL; 
		next_subs = NULL; 
		update = NULL; 
		location = new local(0,0,0,0,0);
		state_time = new int64_t[SR_MODE_NUM]; 
		for (int i = 0; i < SR_MODE_NUM; i++) state_time[i] = 0;
		current_time = ct; 
		current_state = SR_MODE_WAIT; 
		state_current_time = ct; 
		next_state = SR_MODE_WAIT; 
		next_state_predict_time = ct; 
	}
	~sub_request(){
		if (location != NULL) delete location; 
		if (update != NULL) delete update;	
		if (buf_entry != NULL) delete buf_entry;  
		if (state_time != NULL) delete state_time;  
	}
	
	unsigned int app_id; 
	int io_num; 
	unsigned int seq_num;

	unsigned int lpn;                  
	int ppn;    
	buffer_entry * buf_entry;               
	unsigned int operation;            
	int size;
	unsigned int current_state;        
	int64_t current_time;
	unsigned int next_state;
	int64_t next_state_predict_time;
	uint64_t state;              
	
	int64_t begin_time;               
	int64_t wait_time;
	int64_t complete_time;            

	local *location;           
	sub_request *next_subs;    
	sub_request *next_node;    
	sub_request *update;  
	int64_t * state_time; 
	int64_t state_current_time; 
	gc_operation * gc_node; 
};

class request{
public: 
	request(){
		app_id = 0; 
		io_num = 0; 
		time = 0; 
		size = 0; 
		operation = 0; 
		begin_time = 0; 
		response_time = 0; 
		next_node = NULL; 
		subs = NULL; 
		critical_sub = NULL; 
		
		subs = NULL; 
		complete_lsn_count = 0; 
	}
	~request(){
		
		sub_request * tmp; 
		while(subs!=NULL)
		{
			tmp = subs->next_subs; 
			delete subs; 
			subs = tmp; 
		}
		
	}
	unsigned int app_id; 
	unsigned int io_num; 

	int64_t  time;
	unsigned int lsn;
	unsigned int size;
	unsigned int operation;
	unsigned int complete_lsn_count;   //record the count of lsn served by buffer
	int64_t begin_time;
	int64_t response_time;
	sub_request *subs;          
	request *next_node;       
	sub_request * critical_sub;
};



class SubQueue{
public: 
	sub_request * queue_head; 
	sub_request * queue_tail;
	 
	int size; 
	
	SubQueue(){
		queue_head = NULL; 
		queue_tail = NULL; 
		size = 0; 
	}
	
	bool find_subreq(sub_request * sub);
	sub_request * get_subreq(int index); 
	void push_tail(sub_request * sub);
	
	void push_head(sub_request * sub);
	
	void remove_node(sub_request * sub);
	sub_request * target_request(int plane, int block, int page); 
	bool is_empty();
	
	
};


class page_info{    
public:                  
	page_info(); 
	bool valid_state;                
	unsigned int lpn;                 
};


class blk_info{
public: 
	blk_info(parameter_value *); 
	~blk_info(){
		for (int  i= 0; i < page_num; i++){
			delete page_head[i]; 
		}
		delete page_head; 
	}
	int erase_count;          
	int free_page_num;       
	int invalid_page_num; 
	int page_num;     
	int last_write_page;
	int64_t last_write_time; 
	page_info **page_head;       
};


class plane_info{
public: 
	plane_info(parameter_value *); 
	~plane_info(){
		for (int i = 0; i < block_num; i++){
			delete blk_head[i]; 
		}
		delete blk_head; 
		delete state_time; 
	}
	void reset_plane_stats(); 	
	int64_t add_reg_ppn;                   
	int64_t free_page;            
	int64_t ers_invalid;          
	int64_t active_block;     
	int64_t can_erase_block;              
	blk_info **blk_head;
	int64_t block_num; 
	int64_t erase_count;
	int64_t read_count; 
	int64_t program_count; 
	
	int current_state; 
	int next_state; 
	int64_t current_time ; 
	int64_t next_state_predict_time; 
	
	int64_t * state_time; 
	bool GCMode; 	
	gc_operation *scheduled_gc; // list of scheduled GCs 
};

class lun_info{
public: 
    lun_info(parameter_value *); 
	~lun_info(){
		for (int i = 0; i < 2; i++){
			delete plane_head[i]; 
		}
		delete plane_head; 
		delete state_time; 
	}
	void reset_lun_stats(int plane_num);      
	plane_info **plane_head;
	int erase_count;  
	int program_count; 
	int read_count; 

	int current_state; 
	int64_t current_time; 
	int next_state;
	int64_t next_state_predict_time; 
	
	int64_t program_avg; 
	int64_t read_avg; 
	int64_t gc_avg; 
	
	SubQueue GCSubs; 
	
	SubQueue rsubs_queue; 
	SubQueue wsubs_queue; 
	
	int64_t * state_time; 
	bool GCMode; 
	int plane_token; 

	void update_stat(int64_t start, int64_t end, int type, int count, int page_size) {
		switch (type) {
			case READ: 
				stat_read_throughput.add_time(start, end); 
	//			stat_write_throughput.add_time(start, end); 
				stat_rw_throughput.add_time(start, end); 
			
				stat_read_throughput.add_capacity(page_size * count); 
				stat_read_throughput.add_count(count); 
				stat_rw_throughput.add_capacity(page_size * count); 
				stat_rw_throughput.add_count(count); 
				break; 
			case WRITE: 
				stat_write_throughput.add_time(start, end); 
	//			stat_read_throughput.add_time(start, end); 
					stat_rw_throughput.add_time(start, end); 
					
				stat_write_throughput.add_capacity(page_size * count); 
				stat_write_throughput.add_count(count); 
				stat_rw_throughput.add_capacity(page_size * count); 
				stat_rw_throughput.add_count(count); 
				break; 
			default: 
				
				cout << "lun stat update: type not known! " << endl; 
				
		}
	}
		

	Tuple stat_read_throughput; 
	Tuple stat_write_throughput; 
	Tuple stat_rw_throughput; 

}; 



class channel_info {
public: 
	channel_info(int channel_number, parameter_value*); 
	~channel_info(){
		for (int i = 0; i < lun_num; i++){
			delete lun_head[i]; 
		}
		delete lun_head; 
		delete state_time; 
	}
	void reset_channel_stats(int lun_number, int plane_per_lun); 
	int lun_num; 
	int64_t read_count;
	int64_t program_count;
	int64_t erase_count;

	int64_t epoch_read_count;  
	int64_t epoch_program_count ; 
	int64_t epoch_erase_count; 

	int current_state;                   //channel has serveral states, including idle, command/address transfer,data transfer,unknown
	int next_state;
	int64_t current_time;               
	int64_t next_state_predict_time;     //the predict time of next state, used to decide the sate at the moment

	lun_info **lun_head;     
	int64_t * state_time; 
};	


class statistics{
public:
	statistics(int cons_deg );
	~statistics(); 
	void reset_all();  
	void print_all(){}
	
	int consolidation_degree; 	
	int64_t * read_request_count;
	int64_t * total_read_request_count;
	int64_t * write_request_count;
	int64_t * total_write_request_count;
	int64_t * write_avg;
	int64_t * read_avg;
	int64_t * total_RT; 
	int64_t * total_read_RT; 
	int64_t * total_write_RT; 
	int64_t * read_request_size; 
	int64_t * total_read_request_size;
	int64_t * write_request_size;
	int64_t * total_write_request_size;

	int64_t flash_read_count, total_flash_read_count; 
	int64_t flash_prog_count, total_flash_prog_count; 
	int64_t flash_erase_count, total_flash_erase_count; 

	int64_t queue_prog_count, queue_read_count; 	
	int64_t direct_erase_count, total_direct_erase_count; 
	int gc_moved_page; 	
	int64_t copy_back_count, total_copy_back_count; 
	int64_t read_multiplane_count, write_multiplane_count, erase_multiplane_count; 
	int64_t m_plane_read_count, total_m_plane_read_count; 
	int64_t m_plane_prog_count, total_m_plane_prog_count; 
	int64_t m_plane_erase_count, total_m_plane_erase_count; 
	int64_t interleave_read_count, total_interleave_read_count; 
	int64_t interleave_prog_count, total_interleave_prog_count; 
	int64_t interleave_erase_count, total_interleave_erase_count; 
	int64_t gc_copy_back, total_gc_copy_back; 
	int64_t waste_page_count, total_waste_page_count;
	int64_t update_read_count, total_update_read_count; 
	int64_t write_worst_case_rt;
	int64_t read_worst_case_rt; 
	int64_t * subreq_state_time; 	
	Tuple read_throughput; 
	Tuple write_throughput; 

}; 




class ssd_info{ 

public:
	ssd_info(parameter_value *, char * statistics_filename, char * trace_filename); 	
	~ssd_info(){
		delete repeat_times; 
		delete last_times; 
		delete tracefile; 
		for (int i=0;i<parameter->channel_number;i++)
		{
			delete channel_head[i]; 
		}
		delete channel_head; 
		delete dram; 
		delete parameter; 
		delete stats; 
	}
	void reset_ssd_stats(); 
	int64_t current_time;                
	int request_sequence_number;
	int subrequest_sequence_number;  
	int gc_sequence_number; 
	int lun_token;                  
	int gc_request;            
	int64_t request_queue_length; 
	
	int64_t total_execution_time; 
	
	int * repeat_times; // repeate trace for each application 
	int64_t * last_times; // last time of each trace 
	int steady_state_counter; 
	int steady_state; 
	
	int64_t min_lsn;
	int64_t max_lsn;
	
	FILE * *tracefile;
	FILE * statisticfile;
	parameter_value *parameter;   
	dram_info *dram;
	request *request_queue;      
	request *request_tail;	 

	SubQueue ssd_wsubs;    
	channel_info **channel_head; 
	statistics * stats; 
};

void file_assert(int error,const char *s);
void alloc_assert(void *p,const char *s);
void trace_assert(int64_t time_t,int device,int64_t lsn,int size,int ope);
unsigned int size(uint64_t stored);
parameter_value *load_parameters(char parameter_file[30]);


#endif
