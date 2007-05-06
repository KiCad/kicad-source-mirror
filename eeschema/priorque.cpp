/*****************************************************************************
* PriorQue.c - implement priority queue, using regular binary trees		 *
* (that guarantees only average time of NlogN...) and with the following	 *
* operations:									  *
* 1. PQInit(&PQ) - initialize the queue, must be called before usage.			  *
* 2. PQEmpty(PQ) - returns TRUE iff PQ is empty.					*
* 3. PQCompFunc(&CompFunc) - sets (a pointer to) the function that is			  *
*	 used to compere two items in the queue. the function should get two	 *
*	 item pointers, and should return >1, 0, <1 as comparison result for	 *
*	 greater than, equal, or less than respectively.					*
* 4. PQFirst(&PQ, Delete) - returns the first element of the priority			  *
*	 queue, and delete it from queue if delete is TRUE.					*
* 5. PQInsert(&PQ, NewItem) - insert NewItem into the queue			  *
*	 (NewItem is a pointer to it), using the comparison function CompFunc.	 *
* 6. PQDelete(&PQ, OldItem) - Delete old item from the queue				  *
*	 again using the comparison function CompFunc.				  *
* 7. PQFind(PQ, OldItem)  - Find old item in queue, again using the		 *
*	 comparison function CompFunc.							*
* 8. PQNext(PQ, CmpItem, NULL) - returns the smallest item which is bigger	 *
*	 than given item CmpItem. PQ is not modified. Return NULL if none.			*
* 9. PQPrint(PQ, &PrintFunc) - print the queue in order using the given		 *
*	 printing function PrintFunc.							*
*10. PQFree(&PQ, FreeItems) - Free the queue. The items are also freed if	 *
*	 FreeItems is TRUE.									*
*											  *
* 			Written by Gershon Elber,	 Dec 88					  *
*****************************************************************************/

#include <stdio.h>
#include <string.h>


#include "priorque.h"

/* Definition des fonction MyFree() ( free() ) et MyMalloc() (malloc() */
void * MyMalloc( size_t );
void MyFree(void*);

#define SIGN(x)			((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))

#define PQ_NEW_NODE(PQ)  { \
	PQ = (PriorQue *) MyMalloc(sizeof(PriorQue)); \
	(PQ) -> Right = (PQ) -> Left = NULL; \
	(PQ) -> Data = NULL; }
#define PQ_FREE_NODE(PQ) { MyFree((char *) (PQ)); PQ = NULL; }

static PQCompFuncType CompFunc; // = (PQCompFuncType) strcmp;

/******************************************************************************
* PQEmpty - initialize the queue...						  *
******************************************************************************/
void PQInit(PriorQue **PQ)
{
	*PQ = NULL;
}

/******************************************************************************
* PQEmpty - returns TRUE iff PQ priority queue is empty.				*
******************************************************************************/
int PQEmpty(PriorQue *PQ)
{
	return PQ == NULL;
}

/******************************************************************************
* PQCompFunc - sets (a pointer to) the function that is used to compere two	  *
* items in the queue. The function should get two item pointers, and should	  *
* return >1, 0, <1 as comparison result for greater than, equal, or less than *
* respectively.										 *
******************************************************************************/
void PQCompFunc(PQCompFuncType NewCompFunc)
{
	CompFunc = NewCompFunc;
}

/******************************************************************************
* PQFirst - returns the first element of the priority queue, and delete it	  *
* from queue if Delete is TRUE. return NULL if empty queue			  *
******************************************************************************/
void * PQFirst(PriorQue **PQ, int Delete)
{
	void * Data;
	PriorQue *Ptemp = (*PQ);

	if (*PQ == NULL) return NULL;

	while (Ptemp -> Right != NULL)
	Ptemp = Ptemp -> Right;					 /* Find smallest item. */
	Data = Ptemp -> Data;

	if (Delete) PQDelete(PQ, Data);

	return Data;
}

/******************************************************************************
* PQInsert - insert new item into the queue (NewItem is a pointer to it),	  *
* using given compare function CompFunc. CompFunc should return >1, 0, <1	  *
* as two item comparison result for greater than, equal, or less than		  *
* respectively.										 *
*	Insert is always as a leaf of the tree.						 *
*	Return pointer to old data if was replaced or NULL if the item is new.	  *
******************************************************************************/
void * PQInsert(PriorQue **PQ, void * NewItem)
{
	int Compare;
	void * Data;

	if ((*PQ) == NULL) {
	PQ_NEW_NODE(*PQ);
	(*PQ) -> Data = NewItem;
	return NULL;
	}
	else {
	Compare = (*CompFunc)(NewItem, (*PQ) -> Data);
	Compare = SIGN(Compare);
	switch (Compare) {
		 case -1:
		return PQInsert(&((*PQ) -> Right), NewItem);
		 case 0:
		Data = (*PQ) -> Data;
		(*PQ) -> Data = NewItem;
		return Data;
		 case 1:
		return PQInsert(&((*PQ) -> Left), NewItem);
	}
	}
	return NULL;					/* Makes warning silent. */
}

/******************************************************************************
* PQDelete - Delete old item from the queue, again using the comparison		  *
* function CompFunc.									 *
* Returns pointer to Deleted item if was fould and deleted, NULL otherwise.	  *
******************************************************************************/
void * PQDelete(PriorQue **PQ, void * OldItem)
{
int Compare;
PriorQue *Ptemp;
void * Data;
void * OldData;

	if ((*PQ) == NULL) return NULL;
	else
		{
		Compare = (*CompFunc)(OldItem, (*PQ) -> Data);
		Compare = SIGN(Compare);
		switch (Compare)
			{
			case -1:
				return PQDelete(&((*PQ) -> Right), OldItem);
			case 0:
				OldData = (*PQ) -> Data;		 /* The returned deleted item. */

				if ((*PQ) -> Right == NULL && (*PQ) -> Left == NULL)
					{
					/* Thats easy - we delete a leaf: */
					Data = (*PQ) -> Data;
					PQ_FREE_NODE(*PQ); /* Note *PQ is also set to NULL here. */
					}
				else if ((*PQ) -> Right != NULL)
					{
					/* replace this node by the biggest in the Right branch: */
					/* move once to the Right and then Left all the time...  */
					Ptemp = (*PQ) -> Right;
					if (Ptemp -> Left != NULL)
						{
						while (Ptemp -> Left -> Left != NULL)
							 Ptemp = Ptemp -> Left;
						Data = Ptemp -> Left -> Data;/*Save the data pointer.*/
						PQDelete(&(Ptemp -> Left), Data);  /* Delete recurs. */
						(*PQ) -> Data = Data; /* And put that data instead...*/
						}
					else
						{
						Data = Ptemp -> Data;	  /* Save the data pointer. */
						PQDelete(&((*PQ) -> Right), Data); /* Delete recurs. */
						(*PQ) -> Data = Data; /* And put that data instead...*/
						}
					}
				else				  /* Left != NULL. */
					{
					/* replace this node by the biggest in the Left branch:  */
					/* move once to the Left and then Right all the time...  */
					Ptemp = (*PQ) -> Left;
					if (Ptemp -> Right != NULL)
						{
						while (Ptemp -> Right -> Right != NULL)
							Ptemp = Ptemp -> Right;
						Data = Ptemp -> Right -> Data; /* Save data pointer. */
						PQDelete(&(Ptemp -> Right), Data);  /*Delete recurs. */
						(*PQ) -> Data = Data; /* And put that data instead...*/
						}
					 else
						{
						Data = Ptemp -> Data;	  /* Save the data pointer. */
						PQDelete(&((*PQ) -> Left), Data);  /* Delete recurs. */
						(*PQ) -> Data = Data; /* And put that data instead...*/
						}
					}
				return OldData;
			case 1:
				return PQDelete(&((*PQ) -> Left), OldItem);
			}
		}
	return NULL;					/* Makes warning silent. */
}

/******************************************************************************
* PQFind - Find old item from the queue, again using the comparison		  *
* function CompFunc.									 *
* Returns pointer to item if was fould, NULL otherwise.					*
******************************************************************************/
void * PQFind(PriorQue *PQ, void * OldItem)
{
	int Compare;

	if (PQ == NULL) {
	return NULL;
	}
	else {
	Compare = (*CompFunc)(OldItem, PQ -> Data);
	Compare = SIGN(Compare);
	switch (Compare) {
		 case -1:
		return PQFind(PQ -> Right, OldItem);
		 case 0:
		return PQ -> Data;
		 case 1:
		return PQFind(PQ -> Left, OldItem);
	}
	}
	return NULL;					/* Makes warning silent. */
}

/******************************************************************************
* PQNext - returns the smallest item which is bigger than given item CmpItem. *
* PQ is not modified. Return NULL if none was found.					 *
* BiggerThan will allways hold the smallest Item bigger than current one.	  *
******************************************************************************/
void * PQNext(PriorQue *PQ, void * CmpItem, void * BiggerThan)
{
	int Compare;
	PriorQue *Ptemp;

	if (PQ == NULL)
	return BiggerThan;
	else {
	Compare = (*CompFunc)(CmpItem, PQ -> Data);
	Compare = SIGN(Compare);
	switch (Compare) {
		 case -1:
		return PQNext(PQ -> Right, CmpItem, PQ -> Data);
		 case 0:
		/* Found it - if its right tree is not empty, returns its	   */
		/* smallest, else returns BiggerThan...				 */
		if (PQ -> Left != NULL) {
			  Ptemp = PQ -> Left;
			  while (Ptemp -> Right != NULL) Ptemp = Ptemp -> Right;
			  return Ptemp -> Data;
		}
		else
			  return BiggerThan;
		 case 1:
		return PQNext(PQ -> Left, CmpItem, BiggerThan);
	}
	}
	return NULL;					/* Makes warning silent. */
}

/******************************************************************************
* PQPrint - print the queue in order using the given printing functionq		  *
* PrintFunc.									  *
******************************************************************************/
void PQPrint(PriorQue *PQ, void (*PrintFunc)(void *))
{
	if (PQ == NULL) return;

	PQPrint(PQ -> Right, PrintFunc);

	(*PrintFunc)(PQ -> Data);

	PQPrint(PQ -> Left, PrintFunc);
}

/******************************************************************************
* PQFree - Free the queue. The items are also freed if FreeItems is TRUE.	  *
******************************************************************************/
void PQFree(PriorQue *PQ, int FreeItem)
{
	if (PQ == NULL) return;

	PQFree(PQ -> Right, FreeItem);
	PQFree(PQ -> Left, FreeItem);

	if (FreeItem) MyFree((char *) PQ -> Data);
	MyFree((char *) PQ);
}

/******************************************************************************
* PQFreeFunc - Free queue. Items are also freed by calling FreeFunc on them.  *
******************************************************************************/
void PQFreeFunc(PriorQue *PQ, void (*FreeFunc)(void *))
{
	if (PQ == NULL) return;

	PQFreeFunc(PQ -> Right, FreeFunc);
	PQFreeFunc(PQ -> Left, FreeFunc);

	(FreeFunc)(PQ -> Data);
	MyFree((char *) PQ);
}
