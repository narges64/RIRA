#ifndef AVLTREE_H
#define AVLTREE_H 10000

#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#define AVL_NULL		(TREE_NODE *)0

#define EH_FACTOR	0
#define LH_FACTOR	1
#define RH_FACTOR	-1
#define LEFT_MINUS	0
#define RIGHT_MINUS	1

#define ORDER_LIST_WANTED

#define INSERT_PREV	0
#define INSERT_NEXT	1

class tAVLTree;  
class buffer_group; 
class TREE_NODE
{
public: 
#ifdef ORDER_LIST_WANTED
	TREE_NODE *prev;
	TREE_NODE *next;
#endif
	TREE_NODE *tree_root;
	TREE_NODE *left_child;
	TREE_NODE *right_child;
	int  bf;    			                    
};

class tAVLTree
{
public: 
	unsigned long read_hit;                   
	unsigned long read_miss_hit;  
	unsigned long write_hit;   
	unsigned long write_miss_hit;

	buffer_group *buffer_head;            /*as LRU head which is most recently used*/
	buffer_group *buffer_tail;            /*as LRU tail which is least recently used*/
	TREE_NODE	*pTreeHeader;     				 /*for search target lsn is LRU table*/

	unsigned int max_buffer_sector;
	unsigned int buffer_sector_count;

#ifdef ORDER_LIST_WANTED
	TREE_NODE	*pListHeader;
	TREE_NODE	*pListTail;
#endif
	unsigned int	count;		                
	int 			(*keyCompare)(TREE_NODE * , TREE_NODE *);
	int			(*free)(TREE_NODE *);
};

class buffer_group{
public: 
	TREE_NODE node;                     //Tree nodes must be placed in front of a user-defined structure, pay attention! 
	buffer_group *LRU_link_next;	// next node in LRU list
	buffer_group *LRU_link_pre;	// previous node in LRU list

	unsigned int group;                 //the first data logic sector number of a group stored in buffer 
	uint64_t stored;                //indicate the sector is stored in buffer or not. 1 indicates the sector is stored and 0 indicate the sector isn't stored.
										//EX.  00110011 indicates the first, second, fifth, sixth sector is stored in buffer.
	unsigned int dirty_clean;           //it is flag of the data has been modified, one bit indicates one subpage. EX. 0001 indicates the first subpage is dirty
	int flag;			                //indicates if this node is the last 20% of the LRU list	
};


int avlTreeHigh(TREE_NODE *);
int avlTreeCheck(tAVLTree *,TREE_NODE *);
static void R_Rotate(TREE_NODE **);
static void L_Rotate(TREE_NODE **);
static void LeftBalance(TREE_NODE **);
static void RightBalance(TREE_NODE **);
static int avlDelBalance(tAVLTree *,TREE_NODE *,int);
void AVL_TREE_LOCK(tAVLTree *,int);
void AVL_TREE_UNLOCK(tAVLTree *);
void AVL_TREENODE_FREE(tAVLTree *,TREE_NODE *);

#ifdef ORDER_LIST_WANTED
static int orderListInsert(tAVLTree *,TREE_NODE *,TREE_NODE *,int);
static int orderListRemove(tAVLTree *,TREE_NODE *);
TREE_NODE *avlTreeFirst(tAVLTree *);
TREE_NODE *avlTreeLast(tAVLTree *);
TREE_NODE *avlTreeNext(TREE_NODE *pNode);
TREE_NODE *avlTreePrev(TREE_NODE *pNode);
#endif

static int avlTreeInsert(tAVLTree *,TREE_NODE **,TREE_NODE *,int *);
static int avlTreeRemove(tAVLTree *,TREE_NODE *);
static TREE_NODE *avlTreeLookup(tAVLTree *,TREE_NODE *,TREE_NODE *);
tAVLTree *avlTreeCreate(int *,int *);
int avlTreeDestroy(tAVLTree *);
int avlTreeFlush(tAVLTree *);
int avlTreeAdd(tAVLTree *,TREE_NODE *);
int avlTreeDel(tAVLTree *,TREE_NODE *);
TREE_NODE *avlTreeFind(tAVLTree *,TREE_NODE *);
unsigned int avlTreeCount(tAVLTree *);

#endif


