#include "common.hh"


void file_assert(int error,const char *s){
	if(error == 0) return;
	printf("open %s error\n",s);
	getchar();
	exit(-1);
}


void trace_assert(int64_t time_t,int device,unsigned int lsn,int size,int ope)
{
	if(time_t <0 || device < 0  || size < 0 || ope < 0)
	{
		printf("trace error:%lld %d %d %d %d\n",time_t,device,lsn,size,ope);
		getchar();
		exit(-1);
	}
	if(time_t == 0 && device == 0 && lsn == 0 && size == 0 && ope == 0)
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
		
		if (qsub->location->channel == sub->location->channel && qsub->location->lun == sub->location->lun && qsub->location->plane == sub->location->plane){
			
			if (qsub->location->block == sub->location->block && qsub->location->page == sub->location->page) {
				// FIXME do we need to compare state?? 
				return TRUE; 
				
			}
			
		}
		
		qsub = qsub->next_node; 
	}
	return FALSE; 
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

sub_request * SubQueue::target_request(unsigned int plane, unsigned int block, unsigned int page ){
	
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


