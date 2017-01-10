#include "avlTree.hh"



/******************************************************************** 
* 
* avlTreeHigh(TREE_NODE *pNode)
* 
* 
*********************************************************************/ 
int avlTreeHigh(TREE_NODE *pNode)
{
	int lh=0,rh=0;
	if(!pNode)
		return 0;

	lh = avlTreeHigh(pNode->left_child);
	rh = avlTreeHigh(pNode->right_child);

	return (1+((lh>rh)?lh:rh));
}


/******************************************************************** 
* 
* avlTreeCheck(tAVLTree *pTree , TREE_NODE *pNode)
* 
*********************************************************************/ 
int avlTreeCheck(tAVLTree *pTree , TREE_NODE *pNode)
{
	int lh=0,rh=0;
	TREE_NODE *tree_root = AVL_NULL;

	if(!pTree || !pNode)
		return 0;

	lh = avlTreeHigh(pNode->left_child);
	rh = avlTreeHigh(pNode->right_child);
	if(pNode->bf != lh-rh)  
		return 0;

	
	if(pNode->left_child && ((*pTree->keyCompare)(pNode , pNode->left_child))>=0)
		return 0;


	if(pNode->right_child && ((*pTree->keyCompare)(pNode , pNode->right_child))<=0)
		return 0;

	tree_root = pNode->tree_root;
	if(!tree_root && (pTree->pTreeHeader != pNode))
		return 0;

	if(tree_root)
	{
		
		if((tree_root->left_child != pNode && tree_root->right_child != pNode) ||
			(tree_root->left_child == pNode && tree_root->right_child == pNode))
			return 0;
	}

	
	if((pNode->left_child && pNode->left_child->tree_root != pNode) ||
		(pNode->right_child && pNode->right_child->tree_root != pNode))
		return 0;

	if(pNode->left_child && !avlTreeCheck(pTree, pNode->left_child))
		return 0;

	if(pNode->right_child && !avlTreeCheck(pTree, pNode->right_child))
		return 0;

	return 1;
}


/******************************************************************** 
* 
* R_Rotate(TREE_NODE **ppNode)
* 
* 
* 
* Returns         :  
*
*          
*
*             E2                C0  
*            / \               / \                    
*           C1  F0            B1  E0                  
*          / \       ==>     /   / \                        
*         B1  D0            A0  D0  F0                      
*        /                                                
*       A0                                                 
*                                              

*                                              
**********************************************************************/
static void R_Rotate(TREE_NODE **ppNode)
{
	TREE_NODE *l_child = AVL_NULL;
	TREE_NODE *pNode = (TREE_NODE *)(*ppNode);

	l_child = pNode->left_child;
	pNode->left_child = l_child->right_child;
	if(l_child->right_child)
		l_child->right_child->tree_root = pNode;
	l_child->right_child = pNode;
	l_child->tree_root = pNode->tree_root;
	pNode->tree_root = l_child;
	(*ppNode) = l_child;
}


/******************************************************************** 
* 
* L_Rotate(TREE_NODE **ppNode)
* 
* 
* 
* Returns        
*                   
*       
*                             
*           B-2                  D0                
*          / \       ==>        / \                      
*         A0  D-1              B0  E0                     
*            / \              / \   \                   
*           C0  E-1          A0  C0  F0                  
*                \
*                 F0       
*******************************************************************/ 
static void L_Rotate(TREE_NODE **ppNode)
{
	TREE_NODE *r_child = AVL_NULL;
	TREE_NODE *pNode = (TREE_NODE *)(*ppNode);

	r_child = pNode->right_child;
	pNode->right_child = r_child->left_child;
	if(r_child->left_child)
		r_child->left_child->tree_root = pNode;
	r_child->left_child = pNode;
	r_child->tree_root = pNode->tree_root;
	pNode->tree_root = r_child;
	(*ppNode) = r_child;
}


/******************************************************************** 
* 
* LeftBalance(TREE_NODE **ppNode)
* 
* 
* 
********************************************************************/ 
static void LeftBalance(TREE_NODE **ppNode)
{
	TREE_NODE *left_child = AVL_NULL;
	TREE_NODE *right_child = AVL_NULL;
	TREE_NODE *tree_root = AVL_NULL;
	TREE_NODE *pNode = (TREE_NODE *)(*ppNode);

	tree_root = pNode->tree_root;              
	left_child = pNode->left_child;          
	switch(left_child->bf)
	{
	case LH_FACTOR:                            
		pNode->bf = left_child->bf = EH_FACTOR; 
		R_Rotate(ppNode);  
		break;
	case RH_FACTOR:                            
		                                       
		right_child = left_child->right_child;
		switch(right_child->bf)
		{
		case LH_FACTOR:
			pNode->bf = RH_FACTOR;
			left_child->bf = EH_FACTOR;
			break;
		case EH_FACTOR:
			pNode->bf = left_child->bf = EH_FACTOR;
			break;
		case RH_FACTOR:
			pNode->bf = EH_FACTOR;
			left_child->bf = LH_FACTOR;
			break;
		}
		right_child->bf = EH_FACTOR;
		L_Rotate(&pNode->left_child);         
		R_Rotate(ppNode);                     
		break;
	case EH_FACTOR:                           
		pNode->bf = LH_FACTOR;
		left_child->bf = RH_FACTOR;
		R_Rotate(ppNode);                    
		break;
	}
	(*ppNode)->tree_root = tree_root;
	if(tree_root && tree_root->left_child == pNode)
		tree_root->left_child = *ppNode;
	if(tree_root && tree_root->right_child == pNode)
		tree_root->right_child = *ppNode;
}


/******************************************************************** 
* 
* RightBalance(TREE_NODE **ppNode)
* 
* Returns       
********************************************************************/ 
static void RightBalance(TREE_NODE **ppNode)
{
	TREE_NODE *left_child = AVL_NULL;
	TREE_NODE *right_child = AVL_NULL;
	TREE_NODE *tree_root = AVL_NULL;
	TREE_NODE *pNode = (TREE_NODE *)(*ppNode);

	tree_root = pNode->tree_root;
	right_child = pNode->right_child;
	switch(right_child->bf)
	{
	case RH_FACTOR:
		pNode->bf = right_child->bf = EH_FACTOR;
		L_Rotate(ppNode);
		break;
	case LH_FACTOR:
		left_child = right_child->left_child;
		switch(left_child->bf)
		{
		case RH_FACTOR:
			pNode->bf = LH_FACTOR;
			right_child->bf = EH_FACTOR;
			break;
		case EH_FACTOR:
			pNode->bf = right_child->bf = EH_FACTOR;
			break;
		case LH_FACTOR:
			pNode->bf = EH_FACTOR;
			right_child->bf = RH_FACTOR;
			break;
		}
		left_child->bf = EH_FACTOR;
		R_Rotate(&pNode->right_child);
		L_Rotate(ppNode);
		break;
	case EH_FACTOR:
		pNode->bf = RH_FACTOR;
		right_child->bf = LH_FACTOR;
		L_Rotate(ppNode);
		break;
	}
	(*ppNode)->tree_root = tree_root;
	if(tree_root && tree_root->left_child == pNode)
		tree_root->left_child = *ppNode;
	if(tree_root && tree_root->right_child == pNode)
		tree_root->right_child = *ppNode;
}


/******************************************************************** 
* 
* avlDelBalance(tAVLTree *pTree , TREE_NODE *pNode,int L_R_MINUS)
* 
* pTree 		
* pNode		
* L_R_MINUS	:  
*			LEFT_MINUS    -- 
*                      RIGHT_MINUS  --
*     
******************************************************************/ 
static int avlDelBalance
(
 tAVLTree *pTree , 
 TREE_NODE *pNode,
 int L_R_MINUS
 )
{
	TREE_NODE *tree_root = AVL_NULL;

	tree_root = pNode->tree_root;
	if(L_R_MINUS == LEFT_MINUS)
	{
		switch(pNode->bf)
		{
		case EH_FACTOR:
			pNode->bf = RH_FACTOR;
			break;
		case RH_FACTOR:
			RightBalance(&pNode);
			if(!tree_root)
				pTree->pTreeHeader = pNode;
			if(pNode->tree_root && pNode->bf == EH_FACTOR)
			{
				if(pNode->tree_root->left_child == pNode)
					avlDelBalance(pTree , pNode->tree_root , LEFT_MINUS);
				else
					avlDelBalance(pTree , pNode->tree_root , RIGHT_MINUS);
			}
			break;
		case LH_FACTOR:
			pNode->bf = EH_FACTOR;
			if(pNode->tree_root && pNode->bf == EH_FACTOR)
			{
				if(pNode->tree_root->left_child == pNode)
					avlDelBalance(pTree , pNode->tree_root , LEFT_MINUS);
				else
					avlDelBalance(pTree , pNode->tree_root , RIGHT_MINUS);
			}
			break;
		}
	}

	if(L_R_MINUS == RIGHT_MINUS)
	{
		switch(pNode->bf)
		{
		case EH_FACTOR:
			pNode->bf = LH_FACTOR;
			break;
		case LH_FACTOR:
			LeftBalance(&pNode);
			if(!tree_root)
				pTree->pTreeHeader = pNode;
			if(pNode->tree_root && pNode->bf == EH_FACTOR)
			{
				if(pNode->tree_root->left_child == pNode)
					avlDelBalance(pTree , pNode->tree_root , LEFT_MINUS);
				else
					avlDelBalance(pTree , pNode->tree_root , RIGHT_MINUS);
			}
			break;
		case RH_FACTOR:
			pNode->bf = EH_FACTOR;
			if(pNode->tree_root && pNode->bf == EH_FACTOR)
			{
				if(pNode->tree_root->left_child == pNode)
					avlDelBalance(pTree , pNode->tree_root , LEFT_MINUS);
				else
					avlDelBalance(pTree , pNode->tree_root , RIGHT_MINUS);
			}
			break;
		}
	}

	return 1;
}


/******************************************************************** 
* 
* AVL_TREE_LOCK(tAVLTree *pTree , int timeout)
* 
*********************************************************************/ 
void AVL_TREE_LOCK
(
 tAVLTree *pTree,
 int timeout
 )
{
	if(!pTree
#if OS==3 || OS==4		
		|| !pTree->sem
#endif		
		)
		return;

#if OS==3 || OS==4
	semTake(pTree->sem,timeout);
#endif
	return;
}

/********************************************************************* 
* 
* AVL_TREE_UNLOCK(tAVLTree *pTree , int timeout)
* 
*********************************************************************/ 
void AVL_TREE_UNLOCK
(
 tAVLTree *pTree
 )
{
	if(!pTree
#if OS==3 || OS==4		
		|| !pTree->sem
#endif		
		)
		return;

#if OS==3 || OS==4
	semGive(pTree->sem);
#endif
	return;
}

/******************************************************************** 
* 
* AVL_TREENODE_FREE(tAVLTree *pTree , TREE_NODE *pNode)
* 
*********************************************************************/ 
void AVL_TREENODE_FREE
(
 tAVLTree *pTree,
 TREE_NODE *pNode
 )
{
	if(!pTree || !pNode)
		return;

	(*pTree->free)(pNode);
	return ;
}

#ifdef ORDER_LIST_WANTED
/******************************************************************************** 
* 
* orderListInsert
*	(
*	tAVLTree *pTree,	      
*	TREE_NODE *pNode ,  
*	TREE_NODE *pInsertNode, 
*	int prev_or_next               
*	)
* 
*********************************************************************************/ 
static int orderListInsert
(
 tAVLTree *pTree,
 TREE_NODE *pNode , 
 TREE_NODE *pInsertNode,
 int prev_or_next
 )
{
	TREE_NODE *p = AVL_NULL;

	if(!pNode)
		return 0;

	if(prev_or_next == INSERT_PREV)
	{
		p = pNode->prev;
		if(p)	p->next = pInsertNode;
		else	pTree->pListHeader = pInsertNode;

		pInsertNode->prev = p;
		pInsertNode->next = pNode;
		pNode->prev = pInsertNode;
	}

	if(prev_or_next == INSERT_NEXT)
	{
		p = pNode->next;
		if(p)	p->prev = pInsertNode;
		else	pTree->pListTail = pInsertNode;

		pInsertNode->prev = pNode;
		pInsertNode->next = p;
		pNode->next = pInsertNode;
	}
	return 1;
}

/******************************************************************** 
* int orderListRemove
*	(
*	tAVLTree *pTree,    
*	TREE_NODE *pRemoveNode 
*	)
* 
********************************************************************/ 
static int orderListRemove
(
 tAVLTree *pTree,
 TREE_NODE *pRemoveNode
 )
{
	TREE_NODE *pPrev = AVL_NULL;
	TREE_NODE *pNext = AVL_NULL;

	if(!pRemoveNode)
		return 0;

	pPrev = pRemoveNode->prev;
	pNext = pRemoveNode->next;
	if(!pPrev && !pNext)
	{
		pTree->pListHeader = pTree->pListTail = AVL_NULL;
		return 1;
	}
	if(pPrev && pNext)
	{
		pPrev->next = pNext;
		pNext->prev = pPrev;
		return 1;
	}

	if(pPrev)
	{
		pPrev->next = AVL_NULL;
		pTree->pListTail = pPrev;
		return 1;
	}

	if(pNext)
	{
		pNext->prev = AVL_NULL;
		pTree->pListHeader = pNext;
		return 1;
	}
	else 
	{
		return 0;
	}
}


/******************************************************************** 
*      avlTreeFirst(tAVLTree *pTree)
* 
*********************************************************************/ 
TREE_NODE *avlTreeFirst
(
 tAVLTree *pTree
 )
{
	if(!pTree)
		return AVL_NULL;

	if(!pTree->count || !pTree->pTreeHeader)
		return AVL_NULL;

	return (TREE_NODE *)pTree->pListHeader;
}


/******************************************************************** 
*      avlTreeLast(tAVLTree *pTree)
* 
*********************************************************************/ 
TREE_NODE *avlTreeLast
(
 tAVLTree *pTree
 )
{
	if(!pTree)
		return AVL_NULL;

	if(!pTree->count || !pTree->pTreeHeader)
		return AVL_NULL;

	return (TREE_NODE *)pTree->pListTail;
}

/******************************************************************** 
*      avlTreeNext(TREE_NODE *pNode)
* 
*********************************************************************/ 
TREE_NODE *avlTreeNext
(
 TREE_NODE *pNode
 )
{
	if(!pNode)
		return AVL_NULL;

	return (TREE_NODE *)pNode->next;
}

/******************************************************************** 
*      avlTreePrev(TREE_NODE *pNode)
* 
*********************************************************************/ 
TREE_NODE *avlTreePrev
(
 TREE_NODE *pNode
 )
{
	if(!pNode)
		return AVL_NULL;

	return (TREE_NODE *)pNode->prev;
}
#endif

/*****************************************************************************************
*      int avlTreeInsert
*	(
*	tAVLTree *pTree ,      
*	TREE_NODE **ppNode , 
*	TREE_NODE *pInsertNode,  
*	int *growthFlag  
*	)
* 
******************************************************************************************/ 
static int avlTreeInsert
(
 tAVLTree *pTree , 
 TREE_NODE **ppNode , 
 TREE_NODE *pInsertNode,
 int *growthFlag
 )
{
	int compFlag = 0;
	TREE_NODE *pNode = (TREE_NODE *)(*ppNode);

	if(pTree->count == 0)
	{
		pTree->pTreeHeader = pInsertNode;
		pInsertNode->bf = EH_FACTOR;
		pInsertNode->left_child = pInsertNode->right_child = AVL_NULL;
		pInsertNode->tree_root = AVL_NULL;
#ifdef ORDER_LIST_WANTED
		pTree->pListHeader = pTree->pListTail = pInsertNode;
		pInsertNode->prev = pInsertNode->next = AVL_NULL;
#endif
		return 1;
	}

	compFlag = ((*pTree->keyCompare)(pNode , pInsertNode));
	if(!compFlag)
	{
		*growthFlag = 0;
		return 0;
	}

	if(compFlag < 0)
	{
		if(!pNode->left_child)
		{
			pNode->left_child = pInsertNode;
			pInsertNode->bf = EH_FACTOR;
			pInsertNode->left_child = pInsertNode->right_child = AVL_NULL;
			pInsertNode->tree_root = (TREE_NODE *)pNode;
#ifdef ORDER_LIST_WANTED
			orderListInsert(pTree,pNode, pInsertNode, INSERT_PREV);
#endif
			switch(pNode->bf)
			{
			case EH_FACTOR:
				pNode->bf = LH_FACTOR;
				*growthFlag = 1;
				break;
			case RH_FACTOR:
				pNode->bf = EH_FACTOR;
				*growthFlag = 0;
				break;
			}
		}
		else
		{
			if(!avlTreeInsert(pTree, &pNode->left_child,pInsertNode, growthFlag))
				return 0;

			if(*growthFlag)
			{
				switch(pNode->bf)
				{
				case LH_FACTOR:
					LeftBalance(ppNode);
					*growthFlag = 0;
					break;
				case EH_FACTOR:
					pNode->bf = LH_FACTOR;
					*growthFlag = 1;
					break;
				case RH_FACTOR:
					pNode->bf = EH_FACTOR;
					*growthFlag = 0;
					break;
				}
			}
		}
	}

	if(compFlag > 0)
	{
		if(!pNode->right_child)
		{
			pNode->right_child = pInsertNode;
			pInsertNode->bf = EH_FACTOR;
			pInsertNode->left_child = pInsertNode->right_child = AVL_NULL;
			pInsertNode->tree_root = (TREE_NODE *)pNode;
#ifdef ORDER_LIST_WANTED
			orderListInsert(pTree,pNode, pInsertNode, INSERT_NEXT);
#endif
			switch(pNode->bf)
			{
			case EH_FACTOR:
				pNode->bf = RH_FACTOR;
				*growthFlag = 1;
				break;
			case LH_FACTOR:
				pNode->bf = EH_FACTOR;
				*growthFlag = 0;
				break;
			}
		}
		else
		{
			if(!avlTreeInsert(pTree, &pNode->right_child,pInsertNode, growthFlag))
				return 0;

			if(*growthFlag)
			{
				switch(pNode->bf)
				{
				case LH_FACTOR:
					pNode->bf = EH_FACTOR;
					*growthFlag = 0;
					break;
				case EH_FACTOR:
					pNode->bf = RH_FACTOR;
					*growthFlag = 1;
					break;
				case RH_FACTOR:
					RightBalance(ppNode);
					*growthFlag = 0;
					break;
				}
			}
		}
	}

	return 1;
}


/******************************************************************** 
*      int avlTreeRemove
*	(
*	tAVLTree *pTree ,    
*	TREE_NODE *pRemoveNode 
*	)
* 
* Returns        
*                
*                                                    
*          C               C                                                           
*         / \             / \                     C                                     
*        B   E    ==>    B  .F.      ==>         / \                                     
*       /   / \         /   / \                 B  .F.                                   
*      A   D   G       A   D   G               /   / \                                   
*             / \             / \             A   D   G                                 
*            F   H          .E.  H                     \                                  
*                                                       H                  
*      删除E节点  ==> 找到比E大一点的F ==>  删除E节点，自平衡                                                           
*                     F和E互换指针                                                
********************************************************************/ 
static int avlTreeRemove
(
 tAVLTree *pTree , 
 TREE_NODE *pRemoveNode
 )
{
	int compFlag = 0;
	TREE_NODE *tree_root = AVL_NULL;
	TREE_NODE *p = AVL_NULL;
	TREE_NODE *root_p = AVL_NULL;
	TREE_NODE swapNode;

	tree_root = pRemoveNode->tree_root;
	if(!pRemoveNode->left_child && !pRemoveNode->right_child)
	{
		if(!tree_root)
		{
			pTree->pTreeHeader = AVL_NULL;
#ifdef ORDER_LIST_WANTED
			pTree->pListHeader = pTree->pListTail = AVL_NULL;
#endif
			return 1;
		}
		else if(tree_root->left_child == pRemoveNode)
		{
#ifdef ORDER_LIST_WANTED
			orderListRemove(pTree, pRemoveNode);
#endif
			tree_root->left_child = AVL_NULL;
			avlDelBalance(pTree, tree_root , LEFT_MINUS);
		}
		else
		{
#ifdef ORDER_LIST_WANTED
			orderListRemove(pTree, pRemoveNode);
#endif
			tree_root->right_child = AVL_NULL;
			avlDelBalance(pTree, tree_root , RIGHT_MINUS);
		}
	}

	if(pRemoveNode->left_child && pRemoveNode->right_child)
	{
		TREE_NODE *prev = AVL_NULL;
		TREE_NODE *next = AVL_NULL;
		TREE_NODE *r_child = AVL_NULL;
		root_p = pRemoveNode;
		p = pRemoveNode->right_child;
		while(p->left_child)
		{
			root_p = p;
			p = p->left_child;
		}
		if(p == pRemoveNode->right_child)
		{
			p->tree_root = p;
			pRemoveNode->right_child = pRemoveNode;
		}
		swapNode = *p;
		prev = p->prev;
		next = p->next;
		*p = *pRemoveNode;
		p->prev = prev;
		p->next = next;
		prev = pRemoveNode->prev;
		next = pRemoveNode->next;
		*pRemoveNode = swapNode;
		pRemoveNode->prev = prev;
		pRemoveNode->next = next;
		if(!tree_root) 
			pTree->pTreeHeader = p;
		else if(tree_root->left_child == pRemoveNode)
			tree_root->left_child = p;
		else
			tree_root->right_child = p;

		if(p->left_child) 
			p->left_child->tree_root = p;
		if(p->right_child)  
			p->right_child->tree_root = p;

		if(pRemoveNode->left_child) 
			pRemoveNode->left_child->tree_root = pRemoveNode;
		if(pRemoveNode->right_child)  
			pRemoveNode->right_child->tree_root = pRemoveNode;

		if(root_p != pRemoveNode)
		{
			if(root_p->left_child == p)
				root_p->left_child = pRemoveNode;
			else 
				root_p->right_child = pRemoveNode;
		}

		return avlTreeRemove(pTree, pRemoveNode);
	}

	if(pRemoveNode->left_child)
	{
#ifdef ORDER_LIST_WANTED
		orderListRemove(pTree, pRemoveNode);
#endif
		if(!tree_root)
		{
			pTree->pTreeHeader = pRemoveNode->left_child;
			pRemoveNode->left_child->tree_root = AVL_NULL;
			return 1;
		}

		if(tree_root->left_child == pRemoveNode)
		{
			tree_root->left_child = pRemoveNode->left_child;
			pRemoveNode->left_child->tree_root= tree_root;
			avlDelBalance(pTree , tree_root , LEFT_MINUS);
		}
		else
		{
			tree_root->right_child = pRemoveNode->left_child;
			pRemoveNode->left_child->tree_root = tree_root;
			avlDelBalance(pTree , tree_root , RIGHT_MINUS);
		}

		return 1;
	}

	if(pRemoveNode->right_child)
	{
#ifdef ORDER_LIST_WANTED
		orderListRemove(pTree, pRemoveNode);
#endif
		if(!tree_root)
		{
			pTree->pTreeHeader = pRemoveNode->right_child;
			pRemoveNode->right_child->tree_root = AVL_NULL;
			return 1;
		}

		if(tree_root->left_child == pRemoveNode)
		{
			tree_root->left_child = pRemoveNode->right_child;
			pRemoveNode->right_child->tree_root = tree_root;
			avlDelBalance(pTree , tree_root , LEFT_MINUS);
		}
		else
		{
			tree_root->right_child = pRemoveNode->right_child;
			pRemoveNode->right_child->tree_root = tree_root;
			avlDelBalance(pTree , tree_root , RIGHT_MINUS);
		}

		return 1;
	}

	return 1;
}

/******************************************************************** 
*      int avlTreeLookup
*	(
*	tAVLTree *pTree,
*	TREE_NODE *pNode , 
*	TREE_NODE *pSearchKey
*	)
* 
*    递归查找关键字比较完全匹配的节点，比较函数是在
*     树创建的时候就指定好的
*
* Returns         :  1:成功
*                         0:失败
*********************************************************************/ 
static TREE_NODE *avlTreeLookup
(
 tAVLTree *pTree,
 TREE_NODE *pNode , 
 TREE_NODE *pSearchKey
 )
{
	int compFlag = 0;
	if(!pTree || !pNode)
		return AVL_NULL;

	compFlag = (*pTree->keyCompare)(pNode , pSearchKey);
	if(!compFlag)
		return (TREE_NODE *)pNode;

	if(compFlag>0) pNode = pNode->right_child;
	else pNode = pNode->left_child;

	return (TREE_NODE *)avlTreeLookup(pTree, pNode, pSearchKey);
}


/*******************************************************************/
/**************************AVL TREE API*****************************/
/*******************************************************************/
/*
★描述            : 创建一颗有序平衡二叉树
★参数描述: 
keyCompareFunc 
★返回值      :
成功 :   平衡二叉树的指针
失败 :   空指针
*******************************************************************/
tAVLTree *avlTreeCreate(int *keyCompareFunc,int *freeFunc)
{
	tAVLTree *pTree = (tAVLTree *)0;

	if(!keyCompareFunc || !freeFunc)
		return (tAVLTree *)0;

	pTree = new tAVLTree(); 
	
	if(pTree != (tAVLTree *)0)
	{
		memset((void *)pTree , 0 , sizeof(tAVLTree));
		pTree->keyCompare = (int (*) (TREE_NODE *, TREE_NODE *) )keyCompareFunc;
		pTree->free = (int (*) (TREE_NODE *))freeFunc;
#ifdef ORDER_LIST_WANTED
		pTree->pListHeader = pTree->pListTail = AVL_NULL;
#endif

#if OS==3 || OS==4 
		pTree->sem = semBCreate(0 , 1);
		if(!pTree->sem)
		{
			free((void *)pTree);
			return (tAVLTree *)0;
		}
#endif
	}

	return (tAVLTree *)pTree;
}

/*******************************************************************/
/**************************AVL TREE API*****************************/
/*******************************************************************/
/*
★描述            :  删除一个节点

★参数描述: 
pTree:树结构的指针
pDelNode : 待删除的节点指针
★返回值      :
成功 :  1
失败 :   0
*******************************************************************/
int avlTreeDel( tAVLTree *pTree ,TREE_NODE *pDelNode)
{
	int ret = 0;

	if(!pTree || !pDelNode || !pTree->count)
		return 0;

	ret = avlTreeRemove(pTree, pDelNode);
	if(ret)
		pTree->count--;

	return 1;
}


/*******************************************************************/
/**************************AVL TREE API*****************************/
/*******************************************************************/
/*
★描述            : 摧毁一颗平衡二叉树，并释放所有成员节点占用的内存
释放内存的函数在创建树的时候已经指定好
★参数描述: 
pTree:树结构的指针
★返回值      :
成功 :  1
失败 :   0
********************************************************************/
int avlTreeDestroy
(
 tAVLTree *pTree
 )
{
	TREE_NODE *pNode = AVL_NULL;
	if(!pTree)
		return 0;

	while(pNode == pTree->pTreeHeader)
	{
		avlTreeDel(pTree,pNode);
		AVL_TREENODE_FREE(pTree, pNode);
	}

	if(!pTree->count || !pTree->pTreeHeader)
	{
#if OS==3 || OS==4
		semDelete(pTree->sem);
#endif
		free((void *)pTree);
		return 1;
	}

	return 0;
}


/*******************************************************************/
/**************************AVL TREE API*****************************/
/*******************************************************************/
/*
★描述            : 清空一颗树，释放所有成员节点占用的内存，
但是不释放树结构所占用的内存
★参数描述: 
pTree:树结构的指针
★返回值      :
成功 :  1
失败 :   0
********************************************************************/
int avlTreeFlush
(
 tAVLTree *pTree
 )
{
	TREE_NODE *pNode = AVL_NULL;

	if(!pTree)
		return 0;

	if(!pTree->count || !pTree->pTreeHeader)
		return 1;

	while(pNode == pTree->pTreeHeader)
	{
		avlTreeDel(pTree,pNode);
		AVL_TREENODE_FREE(pTree, pNode);
	}

	return 0;
}


/*******************************************************************/
/**************************AVL TREE API*****************************/
/*******************************************************************/
/*
★描述            :  增加一个节点

★参数描述: 
pTree:树结构的指针
pInsertNode : 待添加的节点指针
★返回值      :
成功 :  1
失败 :   0
*******************************************************************/
int avlTreeAdd
(
 tAVLTree *pTree , 
 TREE_NODE *pInsertNode
 )
{
	int growthFlag=0 , ret = 0;

	if(!pTree || !pInsertNode)
		return 0;

	ret = avlTreeInsert(pTree , &pTree->pTreeHeader , pInsertNode , &growthFlag);
	if(ret)
		pTree->count++;
	return ret;
}



/*******************************************************************/
/**************************AVL TREE API*****************************/
/*******************************************************************/
/*
★描述            : 根据关键字结构来查询一个节点是否存在

★参数描述: 
pTree:树结构的指针
pKeyNode : 关键字结构指针
★返回值      :
成功 :  查找到的节点指针
失败 :   AVL_NULL
********************************************************************/
TREE_NODE *avlTreeFind
(
 tAVLTree *pTree,
 TREE_NODE *pKeyNode
 )
{
	if(!pTree || !pTree->count || !pTree->pTreeHeader)
		return AVL_NULL;

	return (TREE_NODE *)avlTreeLookup(pTree, pTree->pTreeHeader , pKeyNode);
}

/*******************************************************************/
/**************************AVL TREE API*****************************/
/*******************************************************************/
/*
★描述            : 获取树里面的所有节点总数

★参数描述: 
pTree:树结构的指针
★返回值      :
树里面的节点成员总数
********************************************************************/
unsigned int avlTreeCount
(
 tAVLTree *pTree
 )
{
	if(!pTree)
		return 0;

	return pTree->count;
}


