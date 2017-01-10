
#ifndef INITIALIZE_H
#define INITIALIZE_H 10000
#include <iostream>
using namespace std; 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include "avlTree.hh"
#include "common.hh"



#define QUEUE_LENGTH 32
#define ADDRESS_CHUNK 536870912

#define SECTOR 512
#define BUFSIZE 200

#define READ 1
#define WRITE 0
#define ERASE 2

#define REQUEST_IN 300        
#define OUTPUT 301            

#define GC_WAIT 400
#define GC_ERASE_C_A 401
#define GC_COPY_BACK 402
#define GC_COMPLETE 403
#define GC_EARLY 0
#define GC_ONDEMAND 1

#define PLANE_STATE_IDLE 500
#define PLANE_STATE_BUSY 501 

#define PG_SUB 0xffffffffffffffff			

#define TRUE		1
#define FALSE		0
#define INFEASIBLE	-2


enum PLANE_LEVEL_PARALLEL {BASE, GCIO, IOGC, GCGC}; 
enum CHANNEL_MODE {CHANNEL_MODE_IDLE, CHANNEL_MODE_GC, CHANNEL_MODE_IO, CHANNEL_MODE_NUM}; 
enum LUN_MODE {LUN_MODE_IDLE, LUN_MODE_GC, LUN_MODE_IO, LUN_MODE_NUM}; 
enum PLANE_MODE {PLANE_MODE_IDLE, PLANE_MODE_GC, PLANE_MODE_IO, PLANE_MODE_NUM}; 
enum SUBREQ_MODE {SR_MODE_WAIT, SR_MODE_ST_S, SR_MODE_ST_M, SR_MODE_IOC_S, SR_MODE_IOC_O, SR_MODE_IOC_M, SR_MODE_GCC_S, SR_MODE_GCC_O, SR_MODE_GCC_M, SR_MODE_COMPLETE, SR_MODE_NUM}; 

enum PARGC_APPROACH{BLIND = 0, PATTERN, COST_AWARE, DYN_COST_AWARE, PREEMPTIVE, CACHE_INVOLVED, PRE_MOVE, NO_PGC};


enum STATE {ERROR = -1, FAIL, SUCCESS};
enum GC_ALG{GREEDY, FIFO, WINDOWED, RGA, RANDOM, RANDOMP, RANDOMPP, SIMILAR};

class local;
class plane_info;
class direct_erase; 
class lun_ifo; 
class channel_info; 
class event_node; 
class dram_info; 
class parameter_value; 
class lun_info; 
class blk_info; 
class map_info; 
class page_info;
class sub_request; 
class request; 
class gc_operation; 
class entry;   



class local{        
public: 
	local(unsigned int channel, unsigned int lun, unsigned int plane);  
	local(unsigned int channel, unsigned int lun, unsigned int plane, unsigned int block, unsigned int page ); 
	~local(){}
	unsigned int channel;
	unsigned int lun;
	unsigned int plane;
	unsigned int block;
	unsigned int page;
	unsigned int sub_page;
};


class direct_erase{
public:
	unsigned int block;
	direct_erase *next_node;
};

class gc_operation{   
public:      
	gc_operation(local * loc){
		static int sn = 0;  
		next_node = NULL; 
		state = GC_WAIT; 
		if (loc == NULL) return; 
		location = new local(loc->channel, loc->lun, loc->plane, 0, 0); 
		seq_number = sn++;
	}
	~gc_operation(){
	//	if (location) delete location; 
	}
	local * location;
	unsigned int seq_number;  
	unsigned int state;          
	unsigned int priority;    
	gc_operation *next_node;
};




class sub_request{
public: 
 
	sub_request(int64_t ct){
		next_node = NULL; 
		next_subs = NULL; 
		update = NULL; 
		location = new local(0,0,0,0,0);
		state_time = new int64_t[SR_MODE_NUM]; 
		for (int i = 0; i < SR_MODE_NUM; i++) state_time[i] = 0;
		current_time = ct; 
		current_state = SR_MODE_WAIT; 
		next_state = SR_MODE_WAIT; 
		next_state_predict_time = ct; 
	}
	~sub_request(){
		if (location) delete location; 
		if (update) delete update; 
	}
	
	
	unsigned int app_id; 
	unsigned int io_num; 
	unsigned int seq_num;

	unsigned int lpn;                  
	int ppn;                  
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
	sub_request * target_request(unsigned int plane, unsigned int block, unsigned int page); 
	bool is_empty();
	
	
};

   

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





/********************************************************
*mapping information,state
*********************************************************/
class entry{  
public:                      
	int pn;   
	uint64_t state;                      
};



class map_info{
public: 
	~map_info(){
		delete map_entry; 
	//	delete attach_info; 
	}

	entry *map_entry;        //each entry indicate a mapping information
	tAVLTree *attach_info;	// info about attach map
};


class dram_info{
public:

	~dram_info(){
		delete map; 
		delete buffer; 
	}
	unsigned int dram_capacity;     
	int64_t current_time;
      
	map_info *map;
	tAVLTree *buffer; 

};

class controller_info{
public: 
	unsigned int frequency;             
	int64_t clock_time;                 
	float power;                        
};

class channel_info{
public:           
	unsigned int lun; 
	unsigned long read_count;
	unsigned long program_count;
	unsigned long erase_count;
	unsigned int token;                  

	unsigned long epoch_read_count;  
	unsigned long epoch_program_count ; 
	unsigned long epoch_erase_count; 

	int current_state;                   //channel has serveral states, including idle, command/address transfer,data transfer,unknown
	int next_state;
	int64_t current_time;               
	int64_t next_state_predict_time;     //the predict time of next state, used to decide the sate at the moment

	event_node *event;
	
	lun_info *lun_head;     
	int64_t * state_time; 
};

class lun_info{
public: 
           
	plane_info *plane_head;
	unsigned int erase_count;  
	unsigned int program_count; 
	unsigned int read_count; 
	unsigned int plane_num; 

	gc_operation **scheduled_gc; // is GC scheduled for any of the planes 
	
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
	int GCMode; // GC, IO, MIX 
}; 

class plane_info{
public: 
	
	int add_reg_ppn;                   
	unsigned int free_page;            
	unsigned int ers_invalid;          
	unsigned int active_block;     
	unsigned int second_active_block; 
	int can_erase_block;              
	direct_erase *erase_node;    
	blk_info *blk_head;
	unsigned int erase_count;
	unsigned int read_count; 
	unsigned int program_count; 
	
	int current_state; 
	int next_state; 
	int64_t current_time ; 
	int64_t next_state_predict_time; 
	
	int64_t * state_time; 
	int GCMode; // GC, IO 
	
};


class blk_info{
public: 
	unsigned int erase_count;          
	unsigned int free_page_num;       
	unsigned int invalid_page_num;     
	int last_write_page;
	int64_t last_write_time; 
	page_info *page_head;       
};


class page_info{    
public:                  
	uint64_t valid_state;                
	uint64_t free_state;                    //each bit indicates the subpage is free or occupted. 1 indicates that the bit is free and 0 indicates that the bit is used
	unsigned int lpn;                 
	unsigned int written_count;       
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
		
		energy_consumption = 0; 
		distri_flag = 0; 
		subs = NULL; 
		need_distr_flag = NULL; 
		complete_lsn_count = 0; 
	}
	~request(){
		if (need_distr_flag) delete need_distr_flag; 
		
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

	long long int  time;
	unsigned int lsn;
	unsigned int size;
	unsigned int operation;
	unsigned int* need_distr_flag;
	unsigned int complete_lsn_count;   //record the count of lsn served by buffer
	int distri_flag;		           // indicate whether this request has been distributed already
	int64_t begin_time;
	int64_t response_time;
	double energy_consumption;         
	sub_request *subs;          
	request *next_node;       
	sub_request * critical_sub;
};



class event_node{
public: 
	int type;                        
	int64_t predict_time;          
	event_node *next_node;
	event_node *pre_node;
};


class parameter_value{
public: 

	unsigned int consolidation_degree; 
	float time_scale; 
	int MP_address_check; 
	int repeat_trace; 
	int mplane_gc; 
	int gc_algorithm; 
	int checkpoint; 

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
	double operating_current;      
	double supply_voltage;	
	double dram_active_current;     //cpu sdram work current   uA
	double dram_standby_current;    //cpu sdram work current   uA
	double dram_refresh_current;    //cpu sdram work current   uA
	double dram_voltage;            //cpu sdram work voltage  V

	int buffer_management;          //indicates that there are buffer management or not
	int scheduling_algorithm;       
	float quick_radio;
	int related_mapping;

	unsigned int time_step;
	unsigned int small_large_write; //the threshould of large write, large write do not occupt buffer, which is written back to flash directly

	int striping;                 
	int interleaving;
	int pipelining;
	//int threshold_fixed_adjust;
	//int threshold_value;
	int active_write;               //yes;0,no
	float gc_up_threshold;        
	float gc_down_threshold; 
	float gc_mplane_threshold;
	int allocation_scheme;        
	int static_allocation;          
	int dynamic_allocation;       
	int advanced_commands;  
	int ad_priority;                //record the priority between two plane operation and interleave operation
	int ad_priority2;               //record the priority of channel-level, 0 indicates that the priority order of channel-level is highest; 1 indicates the contrary
	int greed_CB_ad;                //0 don't use copyback advanced commands greedily; 1 use copyback advanced commands greedily
	int greed_MPW_ad;               //0 don't use multi-plane write advanced commands greedily; 1 use multi-plane write advanced commands greedily
	int aged;                    
	float aged_ratio; 
	int queue_length;
	int pargc_approach; 
	float pargc_cost_threshold; 

	ac_time_characteristics time_characteristics;
	
	float syn_rd_ratio; 
	int syn_req_count; 
	int syn_req_size;
	int syn_interarrival_mean;  

	int plane_level_tech; 	
	
};




class ssd_info{ 

public:
	
	~ssd_info(){

		unsigned int i,j,k,l,n;
		buffer_group *pt=NULL;
		direct_erase * erase_node=NULL;
		for (i=0;i<parameter->channel_number;i++)
		{
			for (j=0;j<parameter->lun_channel[i];j++)
			{
				for (l=0;l<parameter->plane_lun;l++)
				{
					for (n=0;n<parameter->block_plane;n++)
					{
						delete channel_head[i].lun_head[j].plane_head[l].blk_head[n].page_head; 
					}
					
					delete channel_head[i].lun_head[j].plane_head[l].blk_head; 
					while(channel_head[i].lun_head[j].plane_head[l].erase_node!=NULL)
					{
						erase_node=channel_head[i].lun_head[j].plane_head[l].erase_node;
						channel_head[i].lun_head[j].plane_head[l].erase_node=erase_node->next_node;
						delete erase_node; 
					}
				}
				delete channel_head[i].lun_head[j].plane_head; 
				 
			}
			delete  channel_head[i].lun_head; 
		}
		delete channel_head; 
		//avlTreeDestroy( dram->buffer);
	
		//delete dram; 
		delete parameter; 
	}
	int start;  
	double ssd_energy;                  
	int64_t current_time;                
	int64_t next_request_time;
	unsigned int real_time_subreq;       
	int flag;
	int active_flag;                     
	unsigned int page;

	unsigned int token;                  
	unsigned int gc_request;            

	unsigned int * read_request_count;
	unsigned int * total_read_request_count;
	unsigned int * write_request_count;
	unsigned int * total_write_request_count;
	int64_t * write_avg;
	int64_t * read_avg;
	int64_t * total_RT; 
	int64_t * total_read_RT; 
	int64_t * total_write_RT; 
	unsigned int * read_request_size; 
	unsigned int * total_read_request_size;
	unsigned int * write_request_size;
	unsigned int * total_write_request_size;

	unsigned int flash_read_count, total_flash_read_count; 
	unsigned int flash_prog_count, total_flash_prog_count; 
	unsigned int flash_erase_count, total_flash_erase_count; 
	
	unsigned int direct_erase_count, total_direct_erase_count; 
	
	unsigned int copy_back_count, total_copy_back_count; 

	unsigned int read_multiplane_count, write_multiplane_count, erase_multiplane_count; 

		
	unsigned int m_plane_read_count, total_m_plane_read_count; 
	unsigned int m_plane_prog_count, total_m_plane_prog_count; 
	unsigned int m_plane_erase_count, total_m_plane_erase_count; 
	
	unsigned int interleave_read_count, total_interleave_read_count; 
	unsigned int interleave_prog_count, total_interleave_prog_count; 
	unsigned int interleave_erase_count, total_interleave_erase_count; 
	
	unsigned int gc_copy_back, total_gc_copy_back; 
	unsigned int waste_page_count, total_waste_page_count;
	unsigned int update_read_count, total_update_read_count; 
	
	unsigned int request_queue_length; 
	int64_t write_worst_case_rt;
	int64_t read_worst_case_rt; 
		
	int64_t total_execution_time; 
	
	int * repeat_times; // repeate trace for each application 
	int64_t * last_times; // last time of each trace 
	int steady_state_counter; 
	int steady_state; 
	
	int gc_moved_page; 
	
	unsigned int min_lsn;
	unsigned int max_lsn;
	
	char * parameterfilename;
	char *tracefilename[10];
	char *statisticfilename;
	
	FILE * tracefile[10];
	FILE * statisticfile;

    parameter_value *parameter;   
	dram_info *dram;
	request *request_queue;      
	request *request_tail;	 

	SubQueue ssd_wsubs;    
	event_node *event;          
	channel_info *channel_head; 

	int64_t * subreq_state_time; 

	
};


ssd_info *initiation(ssd_info *, char **);
parameter_value *load_parameters(char parameter_file[30]);
page_info * initialize_page(page_info * p_page);
blk_info * initialize_block(blk_info * p_block,parameter_value *parameter);
plane_info * initialize_plane(plane_info * p_plane,parameter_value *parameter );
lun_info * initialize_lun(lun_info * p_lun,parameter_value *parameter,long long current_time );
ssd_info * initialize_channels(ssd_info * ssd );
dram_info * initialize_dram(ssd_info * ssd);


#endif
