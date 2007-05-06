/*****************************************************************************
* Definitions, visible to others, of Priority Queue module:			  *
*****************************************************************************/

#ifndef PRIOR_Q_GH
#define PRIOR_Q_GH

typedef struct PriorQue
	{
	struct PriorQue *Right, *Left;	/* Pointers to two sons of this node. */
	void * Data;				  /* Pointers to the data itself. */
	} PriorQue;

typedef int (*PQCompFuncType)( void *, void *);	  /* Comparison function. */

/* And global function prototypes: */
void PQInit(PriorQue **PQ);
int PQEmpty(PriorQue *PQ);
void PQCompFunc(PQCompFuncType NewCompFunc);
void * PQFirst(PriorQue **PQ, int Delete);
void * PQInsert(PriorQue **PQ, void * NewItem);
void * PQDelete(PriorQue **PQ, void * NewItem);
void * PQFind(PriorQue *PQ, void * OldItem);
void * PQNext(PriorQue *PQ, void * CmpItem, void * BiggerThan);
void PQPrint(PriorQue *PQ, void (*PrintFunc)());
void PQFree(PriorQue *PQ, int FreeItems);
void PQFreeFunc(PriorQue *PQ, void (*FreeFunc)(void *));

#endif /* PRIOR_Q_GH */
