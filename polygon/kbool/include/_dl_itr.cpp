/*! \file kbool/include/kbool/_dl_itr.cpp
    \brief Double Linked list with Iterators on list
    \author Probably Klaas Holwerda

    Copyright: 2001-2004 (C) Probably Klaas Holwerda

    Licence: wxWidgets Licence

    RCS-ID: $Id: _dl_itr.cpp,v 1.1 2005/05/24 19:13:35 titato Exp $
*/

#ifdef __GNUG__
#pragma implementation
#endif

#ifdef __UNIX__
#include "../include/_dl_itr.h"
#endif

//=======================================================================
// implementation class DL_Node
//=======================================================================
/*! \class DL_Node
* This class is used in the class DL_List to contain the items of the list. This can be a
* pointer to an object or the object itself. The class contains a next and a previous pointer
* to connect the nodes in the list. \n
* class Dtype | Object stored at the node
*/



/*!
Construct a node for a list object
\param it Item the node will contain
*/
template <class Dtype>
DL_Node<Dtype>::DL_Node(Dtype it)	// + init nodeitem
:_item(it)
{}

/*!
 Template constructor no contents
 Construct a node for a list object
*/
template <class Dtype>
DL_Node<Dtype>::DL_Node()
:_item(0)
{}

/*!
Destruct a node object
*/
template <class Dtype>
DL_Node<Dtype>::~DL_Node()
{
}


//=======================================================================
// implementation class DL_List
//=======================================================================
/*! \class DL_List
* class is the base class for implementing a double linked list. The Root node marks the begining and end of the list. The
* lists consists of nodes double linked with a next and previous pointer DL_Node The nodes are cyclic connected to the root
* node. The list is meant to be used with an iterator class, to traverse the nodes. More then 1 iterator can be attached to the
* list. The list keeps track of the number of iterators that are attached to it. Depending on this certain operations are allowed
* are not. For instance a node can only be deleted if there is only one iterator attached to the list.
* class | Dtype | Object contaning List Nodes
*/

/*!
Construct a node object
\par Example:
    How to construct a list of type integer:
\code
	DL_List<int> * a_list = new DL_List<int>();
\endcode
*/
template <class Dtype>
DL_List<Dtype>::DL_List()
:_nbitems(0), _iterlevel(0)
{
	_root = new DL_Node<Dtype>();
   _root->_next=_root;
   _root->_prev=_root;
}


/*!
//Destruct a list object
\par Example:
    How to construct a list of type integer:
\code
   DL_List<int> * a_list = new DL_List<int>(); # declaration and allocation
   delete a_list; #delete it (must have no iterators attached to it)
\endcode
*/
template <class Dtype>
DL_List<Dtype>::~DL_List()
{
	if (_iterlevel != 0)
      throw Bool_Engine_Error("DL_List::~DL_List()\n_iterlevel > 0 ","list error", 0, 1); 

   remove_all(false);
	delete _root;
	_root=0;_nbitems=0; //reset memory used (no lost pointers)
}

/*!
Error report for list error inside DL_List class
the error function is used internally in the list class to report errors,
the function will generate a message based on the error code.
Then an exeption will be generated using the global booleng class instance. \n
tcarg: class | Dtype | item object in list
\par Example
   to call error from inside an DL_List class
\code
Error("remove_all",ITER_GT_O);
\endcode
\param function string that generated this error
\param error code to generate a message for
*/
template <class Dtype>
void DL_List<Dtype>::Error(const char* function,Lerror a_error)
{
   char buf[100];
   strcpy(buf,"DL_List<Dtype>::");
   strcat(buf,function);
   switch (a_error)
   {
		case NO_MES:             strcat(buf,""); break;
		case EMPTY:              strcat(buf,"list is empty"); break;
		case ITER_GT_0:          strcat(buf,"more then zero iter"); break;
		case NO_LIST:            strcat(buf,"no list attached"); break;
		case SAME_LIST:          strcat(buf,"same list not allowed"); break;
		case AC_ITER_LIST_OTHER: strcat(buf,"iter not allowed on other list"); break;
		default:						 strcat(buf,"unhandled error"); break;
   }

	throw Bool_Engine_Error(buf,"list error", 0, 1); 
}

/*!
is list empty (contains items or not)? \n
class | Dtype | item object in list
\return returns true is list is empty else false
\par Example
   too see if list is empty
\code
DL_List<int> _intlist; #create a list of integers

          if (_intlist.Empty())

                  cout << "empty";
\endcode
*/
template <class Dtype>
bool DL_List<Dtype>::empty()
{
	return(bool)(_nbitems==0);
}

/*!
   number of items in list \n
   class | Dtype | item object in list
\return return number of items in the list
\par Example
   too see if list contains only one object
\code
DL_List <int> _intlist; #create a list of integers

          if (_intlist.count() == 1)

                  cout << "one object in list";
\endcode
*/
template <class Dtype>
int DL_List<Dtype>::count()
{
	return _nbitems;
}

/*!
 remove all objects from the list\n
 class | Dtype | item object in list
\note
 The objects itself are not deleted, only removed from the list.
 The user is responsible for memory management.

\note
   The iterator level must be zero to be able to use this function,
   else an error will be generated

\note
   Use this function if an iterator is not needed to do more complex things.
   This will save time, since the iterator does not have to be created.
\par Example
   too insert integer a and b into list and remove_all directly
\code
   DL_List<int> _intlist; #create a list of integers
   int a=123;

   int b=345;

   _intlist.insbegin(a);

   _intlist.insbegin(b);

   _intlist.remove_all();
\endcode
*/
template <class Dtype>
void DL_List<Dtype>::remove_all( bool deleteObject )
{
	if (_iterlevel > 0 )
		Error("remove_all()",ITER_GT_0);

   Dtype* obj; 

	DL_Node<Dtype> *node;
	for (int i=0; i<_nbitems; i++)
	{
		node = _root->_next;
		_root->_next = node->_next;
      if ( deleteObject == true )
      {    
         obj=(Dtype*)(node->_item);
         delete obj; 
      }
		delete node;
	}
	_nbitems=0;_iterlevel=0;  //reset memory used (no lost pointers)
   _root->_prev=_root;
}

/*!
remove the object at the begin of the list (head).
\note
 The object itself is not deleted, only removed from the list.
 The user is responsible for memory management.

\note
 The iterator level must be zero to be able to use this function, else an error will be generated

\note
 The list must contain objects, else an error will be generated.

\note
Use this function if an iterator is not needed to do more complex things. This will save time, since the iterator does not
have to be created.

\par Example:
    too insert integer a at begin of list and remove it directly.
\code
DL_List<int> _intlist; #create a list of integers

         int a=123;

         _intlist.insbegin(a)

         _intlist.removehead();

\endcode
*/
template <class Dtype>
void DL_List<Dtype>::removehead()
{
	if (_iterlevel > 0 )
		Error("removehead()",ITER_GT_0);
	if(_nbitems==0)
		Error("removehead()",EMPTY);

	DL_Node<Dtype>* node=_root->_next;

	node->_prev->_next = node->_next; // update forward link
	node->_next->_prev = node->_prev; // update backward link

	_nbitems--;
	delete node;                      // delete list node
}


/*!
remove the object at the begin of the list (head).

\note
   - The object itself is not deleted, only removed from the list.
     The user is responsible for memory management.
   - The iterator level must be zero to be able to use this function,
     else an error will be generated
   - The list must contain objects, else an error will be generated.
   - Use this function if an iterator is not needed to do more complex things.
     This will save time, since the iterator does not have to be created.

\par Example:
    too insert integer a at end of list and remove it directly.
\code
DL_List<int> _intlist; #create a list of integers

         int a=123;

         _intlist.insend(a)

         _intlist.removetail();

\endcode
*/
template <class Dtype>
void DL_List<Dtype>::removetail()
{
	if (_iterlevel > 0)
		Error("removetail()",ITER_GT_0);
	if (_nbitems==0)
		Error("removehead()",EMPTY);

	DL_Node<Dtype>* node=_root->_prev;

   node->_prev->_next = node->_next; // update forward link
   node->_next->_prev = node->_prev; // update backward link

   _nbitems--;
   delete node;                      // delete list node
}

/*!
insert the object given at the end of the list, after tail
\note
The iterator level must be zero to be able to use this function,
else an error will be generated

\note
Use this function if an iterator is not needed to do more complex things.
This will save time, since the iterator does not have to be created.
\par Example:
too insert integer a at end of list
\code
			DL_List<int> _intlist; #create a list of integers

         int a=123;

         _intlist.insend(a)
\endcode
\param newitem an object for which the template list was generated
*/
template <class Dtype>
DL_Node<Dtype>* DL_List<Dtype>::insend(Dtype newitem)
{
	if (_iterlevel > 0)
		Error("insend()",ITER_GT_0);

	DL_Node<Dtype>* newnode = new DL_Node<Dtype>(newitem);

   newnode ->_next = _root;
   newnode ->_prev = _root->_prev;
   _root->_prev->_next = newnode;
   _root->_prev = newnode;

	_nbitems++;
    
    return newnode;
}

/*!
insert the object given at the begin of the list, before head
\note
The iterator level must be zero to be able to use this function,
else an error will be generated

\note
Use this function if an iterator is not needed to do more complex things.
This will save time, since the iterator does not have to be created.

\par Example:
too insert integer a at begin of list
\code
			DL_List<int> _intlist; #create a list of integers

         int a=123;

         _intlist.insbegin(a)
\endcode
\param newitem an object for which the template list was generated
*/
template <class Dtype>
DL_Node<Dtype>* DL_List<Dtype>::insbegin(Dtype newitem)
{
	if (_iterlevel > 0)
		Error("insbegin()",ITER_GT_0);

	DL_Node<Dtype>* newnode = new DL_Node<Dtype>(newitem);

   newnode ->_prev = _root;
   newnode ->_next = _root->_next;
   _root->_next->_prev = newnode;
   _root->_next = newnode;

	_nbitems++;
    return newnode;
}

/*!
get head item
\return returns the object at the head of the list.
\par Example:
   too insert integer a and b into list and make c be the value of b
   which is at head of list|
\code
				DL_List<int> _intlist; #create a list of integers

          int a=123;

          int b=345;

          int c;

          _intlist.insbegin(a)

          _intlist.insbegin(b)
          c=_intlist.headitem()
\endcode
*/
template <class Dtype>
Dtype DL_List<Dtype>::headitem()
{
	return _root->_next->_item;
}

/*!
get tail item
\return returns the object at the tail/end of the list.
\par Example:
   too insert integer a and b into list and make c be the value of b which
   is at the tail of list
\code
				DL_List<int> _intlist; #create a list of integers

          int a=123;

          int b=345;

          int c;
          _intlist.insbegin(a)
          _intlist.insbegin(b)

          c=_intlist.headitem()
\endcode
*/
template <class Dtype>
Dtype DL_List<Dtype>::tailitem()
{
	return _root->_prev->_item;
}

/*!
* \note
  The iterator level must be zero to be able to use this function, else an error will be generated

* \note  The list may not be the same list as this list
* \param otherlist the list to take the items from
*/
template <class Dtype>
void DL_List<Dtype>::takeover(DL_List<Dtype>* otherlist)
{
	if (otherlist==0)
		Error("takeover(DL_List*)",NO_LIST);
	// no iterators allowed on otherlist
	if (otherlist->_iterlevel > 0)
		Error("takeover(DL_List*)",AC_ITER_LIST_OTHER);
	// otherlist not this list
	else if (otherlist == this)
		Error("takeover(DL_List*)",SAME_LIST);

	if (otherlist->_nbitems == 0)
		return;

	//link other list into this list at the end
   _root->_prev->_next=otherlist->_root->_next;
   otherlist->_root->_next->_prev=_root->_prev;
   otherlist->_root->_prev->_next=_root;
   _root->_prev=otherlist->_root->_prev;

	//empty other list
	_nbitems+=otherlist->_nbitems;
	otherlist->_nbitems=0;
	otherlist->_root->_next=otherlist->_root;
	otherlist->_root->_prev=otherlist->_root;
}

//=======================================================================
// implementation class DL_Iter
//=======================================================================
/*! \class DL_Iter
 template iterator for any list/node type\n
 This class is the base class to attach/instantiate an iterator on a double linked list. \n
 DL_List The iterator is used to traverse and perform functions on the nodes of a list.  \n
 More then 1 iterator can be attached to a list. The list keeps track of the
 number of iterators that are attached to it. \n
 Depending on this certain operations are allowed are not. For instance a node can
 only be deleted if there is only one iterator attached to the list. \n
 class | Dtype | Object for traversing a DL_List of the same Dtype
// \par Example
   to insert integer a and b into list and remove_all directly using an iterator
\code
     DL_List<int>* a_list = new DL_List<int>(); // declaration and allocation

     int a=123;

     int b=345;

     {

             DL_Iter<int>*  a_listiter=new DL_Iter<int>(a_list);

             a_listiter->insbegin(a)

             a_listiter->insbegin(b)

             a_listiter->remove_all()

     } //to destruct the iterator before the list is deleted

     delete a_list; #delete it (must have no iterators attached to it)
\endcode
*/

/*!
 Error report for list error inside DL_Iter class
 the error function is used internally in the iterator class to report errors,
 the function will generate a message based on the error code.
 Then an exception will be generated using the global booleng class instance.|
 \par Example
 to call error from inside an DL_List class|
 \code
 Error("remove_all",ITER_GT_O);
 \endcode
 \param function: function string that generated this error
 \param a_error:  error code to generate a message for
*/
template <class Dtype>
void DL_Iter<Dtype>::Error(const char* function,Lerror a_error)
{
   char buf[100];
   strcpy(buf,"DL_Iter<Dtype>::");
   strcat(buf,function);
   switch (a_error)
   {
		case NO_MES:             strcat(buf,""); break;
		case NO_LIST:            strcat(buf,"no list attached"); break;
		case NO_LIST_OTHER:      strcat(buf,"no list on other iter"); break;
		case AC_ITER_LIST_OTHER: strcat(buf,"iter not allowed on other list"); break;
		case SAME_LIST:          strcat(buf,"same list not allowed"); break;
		case NOT_SAME_LIST:      strcat(buf,"must be same list"); break;
		case ITER_GT_1:          strcat(buf,"more then one iter"); break;
		case ITER_HITROOT:          strcat(buf,"iter at root"); break;
		case NO_ITEM:            strcat(buf,"no item at current"); break;
		case NO_NEXT:            strcat(buf,"no next after current"); break;
		case NO_PREV:            strcat(buf,"no prev before current"); break;
		case EMPTY:              strcat(buf,"list is empty"); break;
		case NOT_ALLOW:          strcat(buf,"not allowed"); break;
		case ITER_NEG:           strcat(buf,"to much iters deleted"); break;
		default:						 strcat(buf,"unhandled error"); break;
   }
   throw Bool_Engine_Error(buf,"list error", 0, 1); 
}

/*!
   Construct an iterator object for a given list of type Dtype \n
   tcarg: class | Dtype | list item object
\par Example
    How to construct a list of type integer and an iterator for it:
\code
    DL_List<int>* IntegerList;
    IntegerList = new DL_List<int>();
    DL_Iter<int>*  a_listiter=new DL_Iter<int>(IntegerList);
\endcode
\param newlist: list for the iterator
*/
template <class Dtype>
DL_Iter<Dtype>:: DL_Iter(DL_List<Dtype>* newlist)
:_list(newlist), _current(RT)
{
	_list->_iterlevel++;                    // add 1 to  DL_Iters on list
}

/*!
This constructs an iterator for a list using an other iterator on the same list,
The new iterator will be pointing to the same list item as the other iterator.\n
tcarg: class | Dtype | list item object
\par Example
   How to construct a list of type integer and a second iterator for it:|
\code
	DL_List<int>* IntegerList;

  IntegerList = new DL_List<int>();

 DL_Iter<int>*  a_listiter=new DL_Iter<int>(IntegerList);

 DL_Iter<int>*  a_secondlistiter=new DL_Iter<int>(a_listiter);
\endcode
\param otheriter other iterator on same list
*/
template <class Dtype>
DL_Iter<Dtype>:: DL_Iter(DL_Iter* otheriter)
{
	if (otheriter->_current==0)
		Error("DL_Iter(otheriter)",NO_LIST_OTHER);
	_list=otheriter->_list;
	_list->_iterlevel++;                    // add 1 to DL_Iters on List
	_current=otheriter->_current;
}

/*!
This constructs an iterator for a list of a given type, the list does not have to exist.
Later on when a list is constructed,the iterator can be attached to it.
This way an iterator to a specific list can be made static to a class, and can be used
for several lists at the same time. \n
tcarg: class | Dtype | list item object

\par Example
   How to construct an iterator, without having a list first.
   This constructs an iterator for a list of the given type, but the list thus not yet exist.
\code
   DL_Iter<int>*  a_iter=new DL_Iter<int>();

   DL_List<int>* IntegerList;

   IntegerList = new DL_List<int>();

   a_iter.Attach(IntegerList);

   a_iter.insend(123);

   a_iter.Detach();
\endcode
*/
template <class Dtype>
DL_Iter<Dtype>:: DL_Iter()
:_list(0), _current(0)
{
}

/*!
destruct an iterator for a list of a given type.
*/
template <class Dtype>
DL_Iter<Dtype>::~DL_Iter()
{
	if (_current==0)
      return;
	_list->_iterlevel--;              // decrease iterators
	if (_list->_iterlevel < 0)
		Error("~DL_Iter()",ITER_NEG);
}

/*!
This attaches an iterator to a list of a given type, the list must exist.
This way an iterator to a specific list can be made
static to a class, and can be used for several lists at the same time.\n
!tcarg: class | Dtype | list item object
\par Example
   How to construct an iterator, without having a list first, and attach an iterator later:|
\code
DL_Iter<int>*  a_iter=new DL_Iter<int>();

DL_List<int>* IntegerList;

IntegerList = new DL_List<int>();

a_iter.Attach(IntegerList);

a_iter.insend(123);

a_iter.Detach();
\endcode
\param newlist the list to attached the iterator to
*/
template <class Dtype>
void DL_Iter<Dtype>::Attach(DL_List<Dtype>* newlist)
{
	if (_current!=0)
		Error("Attach(list)",NOT_ALLOW);
   _list=newlist;
   _current=HD;
	_list->_iterlevel++;                    // add 1 to  DL_Iters on list
}

/*!
This detaches an iterator from a list of a given type, the list must exist.
This way an iterator to a specific list can be made static to a class,
and can be used for several lists at the same time. \n
!tcarg: class | Dtype | list item object
\par Example:
How to construct an iterator, without having a list first, and attach an iterator later:
\code
DL_Iter<int>*  a_iter=new DL_Iter<int>();

DL_List<int>* IntegerList;

IntegerList = new DL_List<int>();

a_iter.Attach(IntegerList);

a_iter.insend(123);

a_iter.Detach();
\endcode
\param newlist: the list to attached the iterator to
*/
template <class Dtype>
void DL_Iter<Dtype>::Detach()
{
	if (_current==0)
		Error("Attach()",NO_LIST);
	_list->_iterlevel--;                    // subtract 1 from DL_Iters on list
   _list=0;
   _current=0;
}

/*
// copy pointers to items from other list
template <class Dtype> void DL_Iter<Dtype>::merge(DL_List<Dtype>* otherlist)
{
	DL_Node* node=otherlist->HD; //can be 0 if empty
	for(int i=0; i<otherlist->NB; i++)
	{
	  insend(node->new_item);  // insert item at end
	  node=node->_next;        // next item of otherlist
	}
}
*/
/*
//call Dtype::mfp for each item
template <class Dtype>
void DL_Iter<Dtype>::foreach_mf(void (Dtype::*mfp)())
{
	DL_Node<Dtype>* node=HD; //can be 0 if empty
   for(int i=0; i< NB; i++)
   {
     ((node->_item).*mfp)();
     node=node->_next;
   }
}
*/


/*! call given function for each item*/
template <class Dtype>
void DL_Iter<Dtype>::foreach_f(void (*fp) (Dtype n) )
{
	DL_Node<Dtype>* node=HD; //can be 0 if empty
   for(int i=0; i< NB; i++)
   {
     fp (node->_item);
     node=node->_next;
   }
}


/*!
to move all objects in a list to the list of the iterator.
\note
 The iterator level must be one to be able to use this function,
 else an error will be generated

\note
 The list may not be the same list as the iterator list
\par Example
 to take over all items in _intlist|
\code
DL_List<int> _intlist; #create a list of integers

DL_List<int> _intlist2; #create a list of integers

int a=123;

DL_Iter<int>*  a_listiter2=new DL_Iter<int>(&_intlist2);

_intlist->insend(a) // insert at end

a_listiter2->takeover(_intlist)
\endcode
\param otherlist  the list to take the items from
*/
template <class Dtype>
void DL_Iter<Dtype>::takeover(DL_List<Dtype>* otherlist)
{
	if (_current==0)
		Error("takeover(DL_List*)",NO_LIST);
	// no iterators allowed on otherlist
	if (otherlist->_iterlevel > 0)
		Error("takeover(DL_List*)",AC_ITER_LIST_OTHER);
	// otherlist not this list
	else if (otherlist == _list)
		Error("takeover(DL_List*)",SAME_LIST);

	if (otherlist->_nbitems == 0)
		return;

	//link other list into this list at the end
   TL->_next=otherlist->_root->_next;
   otherlist->_root->_next->_prev=TL;
   otherlist->_root->_prev->_next=RT;
   TL=otherlist->_root->_prev;

	//empty other list
	NB+=otherlist->_nbitems;
	otherlist->_nbitems=0;
	otherlist->_root->_next=otherlist->_root;
	otherlist->_root->_prev=otherlist->_root;
}


/*!
to move all objects in a list (using iterator of that list) to the list of the iterator.
\note
   The iterator level for both iterators must be one to be able to use this function,

\note
   else an error will be generated

\note
   The list may not be the same list as the iterator list

\par Example
   to take over all items in a_listiter1 it's list|
\code
DL_List<int> _intlist; #create a list of integers

DL_List<int> _intlist2; #create a list of integers

int a=123;

DL_Iter<int>*  a_listiter1=new DL_Iter<int>(&_intlist);

DL_Iter<int>*  a_listiter2=new DL_Iter<int>(&_intlist2);

a_listiter1->insend(a) // insert at end

a_listiter2->takeover(a_listiter1)
\\!to move all objects in a list (using iterator of that list) to the list of the iterator
\endcode
\param otheriter: the iterator to take the items from
*/
template <class Dtype>
void DL_Iter<Dtype>::takeover(DL_Iter* otheriter)
{
	if (otheriter->_current==0)
		Error(" DL_Iter",NO_LIST_OTHER);
	if (_current==0)
		Error(" DL_Iter",NO_LIST);

	// only one iterator allowed on other list?
	if (otheriter->_list->_iterlevel > 1)
		Error("takeover(DL_Iter*)",AC_ITER_LIST_OTHER);
	// otherlist not this list?
	else if (otheriter->_list == _list)
		Error("takeover(DL_Iter*)",SAME_LIST);

	if (otheriter->NB == 0)
		return;

   //link other list into this list at the end
   TL->_next=otheriter->HD;
   otheriter->HD->_prev=TL;
   otheriter->TL->_next=RT;
   TL=otheriter->TL;

	//empty other iter & list
	NB+=otheriter->NB;
	otheriter->NB=0;
	otheriter->HD=otheriter->RT;
	otheriter->TL=otheriter->RT;
	otheriter->_current=otheriter->RT;
}

/*!
to move maxcount objects in a list (using iterator of that list)
to the list of the iterator.
\note The iterator level for both iterators must be one to be able to use this function,
    else an error will be generated

\note The list may not be the same list as the iterator list

\note If less then maxcount objects are available in the source iterator,
    all of them are taken and no error will accur

\par Example
 to take over 1 item from a_listiter1 it's list
\code
DL_List<int> _intlist; #create a list of integers

DL_List<int> _intlist2; #create a list of integers

int a=123;

DL_Iter<int>*  a_listiter1=new DL_Iter<int>(&_intlist);

DL_Iter<int>*  a_listiter2=new DL_Iter<int>(&_intlist2);

a_listiter1->insend(a) // insert at end

a_listiter2->takeover(a_listiter1,1);
//! to move maxcount objects in a list (using iterator of that list) to the list of the iterator
\endcode
\param otheriter the iterator to take the items from
\param maxcount  maximum number of objects to take over
*/
template <class Dtype>
void DL_Iter<Dtype>::takeover(DL_Iter* otheriter, int maxcount)
{
	if (otheriter->_current==0)
		Error("takeover(DL_Iter*,int)",NO_LIST_OTHER);
	if (_current==0)
		Error("takeover(DL_Iter*,int)",NO_LIST);

	if (otheriter->_list->_iterlevel > 1)
		Error("takeover(DL_Iter*,int)",AC_ITER_LIST_OTHER);
	else if (otheriter->_list == _list)
		Error("takeover(DL_Iter*,int)",SAME_LIST);

	if (maxcount<0)
		Error("takeover(DL_Iter*,int), maxcount < 0",NO_MES);

	if (otheriter->NB == 0)
		return;


	if (otheriter->NB <= maxcount)
	{  //take it all
      //link other list into this list at the end
      TL->_next=otheriter->HD;
      otheriter->HD->_prev=TL;
      otheriter->TL->_next=RT;
      TL=otheriter->TL;

      //empty other iter & list
      NB+=otheriter->NB;
      otheriter->NB=0;
      otheriter->HD=otheriter->RT;
      otheriter->TL=otheriter->RT;
      otheriter->_current=otheriter->RT;
	}
	else
	{  //take maxcount elements from otheriter
      //set cursor in otherlist to element maxcount
		DL_Node<Dtype>* node;

  		if (NB/2 < maxcount)
      {	// this is faster (1st half)
			node=otheriter->HD;
         for(int i=1; i<maxcount; i++)
           node=node->_next;
		}
		else
		{	// no, this is faster (2nd half)
			node=otheriter->TL;
         for(int i=NB; i>maxcount+1; i--)
           node=node->_prev;
		}

		// link this->tail to other->head
		if (NB>0)
		{
         TL->_next=otheriter->HD;
         otheriter->HD->_prev=TL;
		}
		else	// target is empty
      {
			HD=otheriter->HD;
			otheriter->HD->_prev=RT;
      }

		// set other root to node-> next (after last to copy)
		otheriter->HD=node->_next;
		otheriter->HD->_prev=otheriter->RT;

		// set this->tail to other->item()->prev (last element to be copied)
		TL=node;
      node->_next=RT;

		// still need to update element counter
		NB+=maxcount;

		// update other list
		otheriter->NB-=maxcount;
		otheriter->_current=otheriter->HD;	// other->current is moved to this!
	}
}


/*!
put the iterator root object before the current iterator position in the list.
The current object will become the new head of the list.
\note The iterator level must be one to be able to use this function,
else an error will be generated

\par Example
 move the root object to make the new head the old tail object|
\code
DL_List <int> _intlist; #create a list of integers
DL_Iter<int>*  a_listiter=new DL_Iter<int>(&_intlist);

a_listiter->insend(1234);
a_listiter->insend(2345);
a_listiter->insend(3456);
a_listiter->totail();
a_listiter->reset_head();
a_listiter->tohead(); //the new head will be at object 3456
\endcode
*/
template <class Dtype>
void DL_Iter<Dtype>::reset_head()
{
	if (_current==0)
		Error("reset_head()",NO_LIST);
	if (_list->_iterlevel > 1 )
		Error("reset_head()",ITER_GT_1);

	if(_current==RT)
		Error("reset head()",ITER_HITROOT);

   //link out RT
   HD->_prev=TL;
   TL->_next=HD;

   //link in RT before current
   HD=_current;
   TL=_current->_prev;

   TL->_next=RT;
   HD->_prev=RT;
}

/*!
put the iterator root object after the current iterator position in the list.
The current object will become the new tail of the list.
\note
 The iterator level must be one to be able to use this function,
  else an error will be generated
\par Example
 move the root object to make the new tail the old head object
\code
DL_List <int> _intlist; #create a list of integers
DL_Iter<int>*  a_listiter=new DL_Iter<int>(&_intlist);

a_listiter->insend(1234);
a_listiter->insend(2345);
a_listiter->insend(3456);
a_listiter->tohead();
a_listiter->reset_tail();
a_listiter->totail(); //the new tail will be at object 1234
\endcode
*/
template <class Dtype>
void DL_Iter<Dtype>::reset_tail()
{
	if (_current==0)
		Error("reset_tail()",NO_LIST);
	if (_list->_iterlevel > 1 )
		Error("reset_tail()",ITER_GT_1);

	if(_current==RT)
		Error("reset head()",ITER_HITROOT);

   //link out RT
   HD->_prev=TL;
   TL->_next=HD;

   //link in RT after current
   TL=_current;
   HD=_current->_next;

   HD->_prev=RT;
   TL->_next=RT;
}

/*!
is list empty (contains items or not)?
\return returns true is list is empty else false
\par exmaple:
   too see if list is empty
\code
DL_List<int> _intlist; #create a list of integers
DL_Iter<int>*  a_listiter=new DL_Iter<int>(&_intlist);

if (a_listiter->Empty())
   cout << "empty"
\endcode
*/
template <class Dtype>
bool DL_Iter<Dtype>::empty()
{
	if (_current==0)
		Error("empty()",NO_LIST);

	return(bool)(NB==0);
}

/*!
is the iterator at the root of the list.
\note Traversing the list in a certain direction using a while loop,
the end can be tested with this function.
\return returns true if the iterator is at the root of the list (after the last/tail/head object), else false.
\par example:
   to traverse in both directions|
\code
DL_List <int> _intlist; #create a list of integers
DL_Iter<int>*  a_listiter=new DL_Iter<int>(&_intlist);

a_listiter->tohead();
//traverse forwards
while ( ! a_listiter->hitroot())
{
	cout << "The item =" << a_listiter->item();
	a_listiter++; //goto next object
}

a_listiter->totail();
//traverse backwards
while ( ! a_listiter->hitroot())
{
 	cout << "The item =" << a_listiter->item();
 	a_listiter--; //goto next object
}
\endcode
*/
template <class Dtype>
bool DL_Iter<Dtype>::hitroot()
{
	if (_current==0)
		Error("hitroot()",NO_LIST);

	return(bool)(_current == RT);
}

/*!
is the iterator at the head of the list.
\return returns true if the iterator is at the head object of the list, else false.
\par exmaple:
   too see the object at the head
\code
DL_List <int> _intlist; #create a list of integers
DL_Iter<int>*  a_listiter=new DL_Iter<int>(&_intlist);

a_listiter->tohead();
if (a_listiter->athead())
      cout << "at the head The item =" << a_listiter->item();
\endcode
*/
template <class Dtype>
bool DL_Iter<Dtype>::athead()
{
	if (_current==0)
		Error("athead()",NO_LIST);

	return(bool)(_current == HD);
}

/*!
is the iterator at the tail of the list.
\return returns true if the iterator is at the tail object of the list,
 else false.
\par Example
 too see the object at the tail|
\code
DL_List <int> _intlist; #create a list of integers
DL_Iter<int>*  a_listiter=new DL_Iter<int>(&_intlist);

a_listiter->totail();
if (a_listiter->attail())
      cout << "at the tail The item =" << a_listiter->item();
\endcode
*/
template <class Dtype>
bool DL_Iter<Dtype>::attail()
{
	if (_current==0)
		Error("attail()",NO_LIST);

	return(bool)(_current == TL);
}

/*!
does the iterator/list contain the given object
\return returns true if the iterator/list contains the given object in the list, else false.
\par Example
   too see if the object is already in the list
\code
DL_List <int> _intlist; #create a list of integers
DL_Iter<int>*  a_listiter=new DL_Iter<int>(&_intlist);
a_listiter->insend(1234);

if (a_listiter->has(1234))
   cout << "yes it does";
\endcode
\param otheritem item to search for
*/
template <class Dtype>
bool DL_Iter<Dtype>::has(Dtype otheritem)
{
	if (_current==0)
		Error("has()",NO_LIST);

	DL_Node<Dtype>* node=HD; //can be 0 if empty
	for(int i=0; i<NB; i++)
	{ if (node->_item == otheritem)
			return true;
	  node=node->_next;
	}
	return false;
}

/*!
number of items in list
\return  number of items in the list
\par Example:
   to see if a list contains only one object
\code
DL_List <int> _intlist; #create a list of integers
DL_Iter<int>*  a_listiter=new DL_Iter<int>(&_intlist);
if (a_listiter->count() == 1)
      cout << "one object in list";
\endcode
*/
template <class Dtype>
int DL_Iter<Dtype>::count()
{
	if (_current==0)
		Error("count()",NO_LIST);

	return NB;
}

/*!
go to first item,  if list is empty goto hite
\par Example
   set iterator to head of list
\code
DL_List <int> _intlist; #create a list of integers
DL_Iter<int>*  a_listiter=new DL_Iter<int>(&_intlist);

a_listiter->insend(1234);
a_listiter->tohead();
\endcode
*/
template <class Dtype>
void DL_Iter<Dtype>::tohead()
{
	if (_current==0)
		Error("tohead()",NO_LIST);

	_current=HD;
}

/*!
go to last item,  if list is empty goto hite
\par Example
   set iterator to tail of list
\code
DL_List <int> _intlist; #create a list of integers
DL_Iter<int>*  a_listiter=new DL_Iter<int>(&_intlist);

a_listiter->insend(1234);
a_listiter->totail();
\endcode
*/
template <class Dtype>
void DL_Iter<Dtype>::totail()
{
	if (_current==0)
		Error("totail()",NO_LIST);

	_current=TL;
}

/*!
set the iterator position to the root (empty dummy) object in the list.
\par Example
   set iterator to root of list and iterate
\code
DL_List <int> _intlist; #create a list of integers
DL_Iter<int>*  a_listiter=new DL_Iter<int>(&_intlist);

a_listiter->insend(1234);
a_listiter->toroot();
while (a_listiter->iterate())
  cout << a_listiter->item();
\endcode
*/
template <class Dtype>
void DL_Iter<Dtype>::toroot()
{
	if (_current==0)
		Error("toroot()",NO_LIST);

  _current=RT;
}

/*!
set the iterator position to next object in the list ( can be the root also)(prefix).
\par Example
how to iterate backwards
\code
DL_List <int> _intlist; //create a list of integers
DL_Iter<int>*  a_listiter=new DL_Iter<int>(&_intlist);

a_listiter->insend(1234);
a_listiter->tohead();
while (!a_listiter->hitroot())
{
      cout << a_listiter->item();
      _listiter++;
}
\endcode
*/
template <class Dtype>
void DL_Iter<Dtype>::operator++(void)
{
	if (_current==0)
		Error("operator++()",NO_LIST);

	_current=_current->_next;
}

/*!
set the iterator position to next object in the list ( can be the root also)(prefix).
\par Example
how to iterate backwards
\code
DL_List <int> _intlist; //create a list of integers
DL_Iter<int>*  a_listiter=new DL_Iter<int>(&_intlist);

a_listiter->insend(1234);
a_listiter->tohead();
while (!a_listiter->hitroot())
{
      cout << a_listiter->item();
      ++_listiter;
}
\endcode
*/
template <class Dtype>
void DL_Iter<Dtype>::operator++(int)
{
	if (_current==0)
		Error("operator++(int)",NO_LIST);

	_current=_current->_next;
}


/*!
set the iterator position to previous object in the list ( can be the root also)(prefix).
\par Example
how to iterate backwards
\code
DL_List <int> _intlist; //create a list of integers
DL_Iter<int>*  a_listiter=new DL_Iter<int>(&_intlist);

a_listiter->insend(1234);
a_listiter->totail();
while (!a_listiter->hitroot())
{
      cout << a_listiter->item();
      _listiter--;
}
\endcode
*/
template <class Dtype>
void DL_Iter<Dtype>::operator--(void)
{
	if (_current==0)
		Error("operator++()",NO_LIST);

	_current=_current->_prev;
}


/*!
set the iterator position to previous object in the list ( can be the root also)(prefix).
\par Example
how to iterate backwards
\code
DL_List <int> _intlist; //create a list of integers
DL_Iter<int>*  a_listiter=new DL_Iter<int>(&_intlist);

a_listiter->insend(1234);
a_listiter->totail();
while (!a_listiter->hitroot())
{
      cout << a_listiter->item();
      --_listiter;
}
\endcode
*/
template <class Dtype>
void DL_Iter<Dtype>::operator--(int)
{
	if (_current==0)
		Error("operator++(int)",NO_LIST);

	_current=_current->_prev;
}


/*!
set the iterator position n objects in the next direction ( can be the root also).
\par Example:
how to set iterator 2 items forward
\code
DL_List <int> _intlist; #create a list of integers
DL_Iter<int>*  a_listiter=new DL_Iter<int>(&_intlist);
a_listiter->insend(1234);
a_listiter->tohead();
a_listiter>>2;//at root now
\endcode
\param n go n places forward
*/
template <class Dtype>
void DL_Iter<Dtype>::operator>>(int n)
{
	if (_current==0)
		Error("operator>>()",NO_LIST);

	for(int i=0; i<n; i++)
 	   _current=_current->_next;
}


/*!
set the iterator position n objects in the previous direction ( can be the root also).
\par Example:
   how to set iterator 2 items backwards
\code
DL_List <int> _intlist; #create a list of integers
DL_Iter<int>*  a_listiter=new DL_Iter<int>(&_intlist);
a_listiter->insend(1234);
a_listiter->totail();
a_listiter<<2;//at root now
\endcode
\param n go n places back
*/
template <class Dtype>
void DL_Iter<Dtype>::operator<<(int n)
{
	if (_current==0)
		Error("operator<<()",NO_LIST);

	for(int i=0; i<n; i++)
		_current=_current->_prev;
}


/*!
put the iterator at the position of the given object in the list.
\return returns true if the object was in the list, else false
\par Example:
  goto element 2345
\code
DL_List <int> _intlist; #create a list of integers
DL_Iter<int>*  a_listiter=new DL_Iter<int>(&_intlist);

a_listiter->insend(1234);
a_listiter->insend(2345);
a_listiter->insend(3456);

a_listiter->toitem(2345); template <class Dtype>
\endcode
*/
template <class Dtype>
bool DL_Iter<Dtype>::toitem(Dtype item)
{
	if (_current==0)
		Error("toitem(item)",NO_LIST);
	DL_Node<Dtype>* node=HD; //can be 0 if empty
	for(int i=0; i<NB; i++)
	{ if (node->_item == item)
	  {
		  _current = node;
			return true;
	  }
	  node=node->_next;
	}
	return false;
}

/*!
put the iterator at the same position as the given iterator in the list.
\par Example:
  goto element 2345 and let a_listiter2 point to the same position
\code
DL_List <int> _intlist; #create a list of integers
DL_Iter<int>*  a_listiter=new DL_Iter<int>(&_intlist);
DL_Iter<int>*  a_listiter2=new DL_Iter<int>(&_intlist);

a_listiter->insend(1234);
a_listiter->insend(2345);
a_listiter->insend(3456);
a_listiter->toitem(2345);
a_listiter2->toiter(a_listiter2);
\endcode
\param otheriter other iterator to let this iterator point to.
*/
template <class Dtype>
void DL_Iter<Dtype>::toiter(DL_Iter *otheriter)
{
	if (otheriter->_current==0)
		Error("toiter(otheriter)",NO_LIST);
	// both iters must have the same list
	if (_list != otheriter->_list)
		Error("toiter(otheriter)",NOT_SAME_LIST);

	_current = otheriter->_current;
}


/*!
put the iterator at the position of the given object in the list.
\note  DO NOT use this function. Normally you will not be able to address the nodes in a list.
\return  returns true if the node was in the list, else false
\param othernode a node to let this iterator point to.
*/
template <class Dtype>
bool DL_Iter<Dtype>::tonode(DL_Node<Dtype> *othernode)
{
	DL_Node<Dtype>* node=HD; //can be 0 if empty  //node is a temporary cursor
	for(int i=0; i<NB; i++)
	{ if (node == othernode)
	  {
		  _current = othernode;
			return true;
	  }
	  node=node->_next;
	}
	return false;
}

/*!
advance the iterator one position in the next direction in the list.
\return  returns true if not at the end/root of the list else false.

\note  This function combines iteration and testing for the end of
the list in one.

\note  Therefore we do not have to advance the iterator ourselves.

\note
The iterator is first put to the next object, before testing for the end of the list. |
This is why we need to start at the root element in general usage.

\par Example
   iterate through all the items in a list
\code
DL_List <int> _intlist; #create a list of integers
DL_Iter<int>*  a_listiter=new DL_Iter<int>(&_intlist);

a_listiter->insend(1234);
a_listiter->insend(2345);
a_listiter->insend(3456);
a_listiter->tobegin(2345);

while (a_listiter->iterate())
{ cout << a_listiter->item(); }
\endcode
*/
template <class Dtype>
bool DL_Iter<Dtype>::iterate(void)
{
	if (_current==0)
		Error("iterate()",NO_LIST);

   _current=_current->_next;
	if (_current==RT)
		return false;
	return true;
}

/*!
To get the item at the current iterator position
\return  returns the object where the iterator is pointing to at the moment.
\note
If the iterator is at the root of the list an error will be generated,
since there is no item there.
\par Example:
   get the element at the head of the list|
\code
DL_List <int> _intlist; //create a list of integers
DL_Iter<int>*  a_listiter=new DL_Iter<int>(&_intlist);

a_listiter->insend(1234);
a_listiter->tohead();
int theitem=a_listiter->item();
\endcode
*/
template <class Dtype>
Dtype DL_Iter<Dtype>::item()
{
	if (_current==0)
		Error("item()",NO_LIST);
	if (_current==RT)
		Error("item()",NO_ITEM);

	return _current->_item;
}

//! get the node at iterater position
template <class Dtype>
DL_Node<Dtype>* DL_Iter<Dtype>::node()
{
	if (_current==0)
		Error("item()",NO_LIST);
	if (_current==RT)
		Error("item()",NO_ITEM);

	return _current;
}

/*!
set the iterator position to next object in the list ( can be the root also).
\note  Use this function inside a new class derived from DL_Iter.
*/
template <class Dtype>
void DL_Iter<Dtype>::next()
{
	if (_current==0)
		Error("item()",NO_LIST);

   _current=_current->_next;
}


/*!
set the iterator position to next object in the list, if this would be the root object,
then set the iterator at the head object
\par Example
cycle the list twice
\code
DL_List <int> _intlist; #create a list of integers
DL_Iter<int>*  a_listiter=new DL_Iter<int>(&_intlist);

a_listiter->insend(1234);
a_listiter->insend(2345);
a_listiter->tohead();

int count=2*a_listiter->count();
while (count)
{
      cout << a_listiter->item();
      next_wrap();
      count--;
}
\endcode
*/
template <class Dtype>
void DL_Iter<Dtype>::next_wrap()
{
	if (_current==0)
		Error("item()",NO_LIST);

   _current=_current->_next;
	if (_current==RT)
	   _current=_current->_next;
}


/*!
set the iterator position to previous object in the list ( can be the root also).
\note  Use this function inside a new class derived from DL_Iter.
*/
template <class Dtype>
void DL_Iter<Dtype>::prev()
{
	if (_current==0)
		Error("item()",NO_LIST);

   _current=_current->_prev;
}

/*!
set the iterator position to previous object in the list, if this would be the root object,
then set the iterator at the tail object
\par Example
cycle the list twice
\code
DL_List <int> _intlist; #create a list of integers
DL_Iter<int>*  a_listiter=new DL_Iter<int>(&_intlist);

a_listiter->insend(1234);
a_listiter->insend(2345);
a_listiter->totail();

int count=2*a_listiter->count();
while (count)
{
      cout << a_listiter->item();
      prev_wrap();
      count--;
}
\endcode
*/
template <class Dtype>
void DL_Iter<Dtype>::prev_wrap()
{
	if (_current==0)
		Error("item()",NO_LIST);

   _current=_current->_prev;
	if (_current==RT)
	   _current=_current->_prev;
}

template <class Dtype>
void DL_Iter<Dtype>::remove_all()
{
	if (_current==0)
		Error("remove_all()",NO_LIST);
	if (_list->_iterlevel > 1 )
		Error("remove_all()",ITER_GT_1);

	_list->_iterlevel--;
   _list->remove_all();
	_list->_iterlevel++;
	_current=RT;
}

/*!
remove object at current iterator position from the list.
\note  The object itself is not deleted, only removed from the list. The user is responsible for memory management.

\note  The iterator level must be one to be able to use this function, else an error will be generated

\note  The list must contain an object at the current iterator position, else an error will be generated.
\par Example:
   to insert integer a at begin of list and remove it directly
\code
DL_List<int> _intlist; #create a list of integers

int a=123;

DL_Iter<int>*  a_listiter=new DL_Iter<int>(&_intlist);

a_listiter->insbegin(a);

a_listiter->tohead();

a_listiter->remove();
\endcode
*/
template <class Dtype>
void DL_Iter<Dtype>::remove()
{
	if (_current==0)
		Error("remove()",NO_LIST);
	if (_list->_iterlevel > 1 )
		Error("remove()",ITER_GT_1);
   if (_current==RT)
		Error("remove()",ITER_HITROOT);

	DL_Node<Dtype>* node=_current;

	_current=_current->_next;

   node->_prev->_next = node->_next; // update forward link
   node->_next->_prev = node->_prev; // update backward link

	NB--;
	delete node;                      // delete list node
}

/*!
remove the object at the begin of the list using an iterator
\note  The object itself is not deleted, only removed from the list. The user is responsible for memory management.

\note  The iterator level must be one to be able to use this function, else an error will be generated

\note  The list must contain objects, else an error will be generated.

\note  Use this function if an iterator is needed to do more complex things. Else use the list member functions directly.
\par Example:
   to insert integer a at begin of list and remove it directly|
\code
DL_List<int> _intlist; #create a list of integers

int a=123;

DL_Iter<int>*  a_listiter=new DL_Iter<int>(&_intlist);

a_listiter->insbegin(a);
a_listiter->removehead();
\endcode
*/
template <class Dtype>
void DL_Iter<Dtype>::removehead()
{
	if (_current==0)
		Error("removehead()",NO_LIST);
	if (_list->_iterlevel > 1 )
		Error("removehead()",ITER_GT_1);
	if(NB==0)
		Error("removehead()",EMPTY);

   if (_current==HD)
		_current=_current->_next;

	_list->_iterlevel--;
   _list->removehead();
	_list->_iterlevel++;
}


/*!
//remove the object at the end of the list using an iterator
\note  The object itself is not deleted, only removed from the list. The user is responsible for memory management.

\note  The iterator level must be one to be able to use this function, else an error will be generated

\note  The list must contain objects, else an error will be generated.

\note  Use this function if an iterator is needed to do more complex things. Else use the list member functions directly.
\par Example:
   to insert integer a at end of list and remove it directly
\code
DL_List<int> _intlist; #create a list of integers

int a=123;

DL_Iter<int>*  a_listiter=new DL_Iter<int>(&_intlist);

a_listiter->insend(a);
a_listiter->removetail();
\endcode
*/
template <class Dtype>
void DL_Iter<Dtype>::removetail()
{
	if (_current==0)
		Error("removetail()",NO_LIST);
	if (_list->_iterlevel > 1 )
		Error("removetail()",ITER_GT_1);
	if (NB==0)
		Error("removehead()",EMPTY);

   if (_current==TL)
		_current=_current->_prev;

	_list->_iterlevel--;
   _list->removetail();
	_list->_iterlevel++;

}

/*!
insert the object given at the end of the list

\note  The iterator level must be one to be able to use this function, else an error will be generated

\note  Use this function if an iterator is needed to do more complex things. Else use the list member functions directly.
\par Example:
   to insert integer a at end of list|
\code
DL_List<int> _intlist; #create a list of integers

int a=123;

DL_Iter<int>*  a_listiter=new DL_Iter<int>(&_intlist);

a_listiter->insend(a);
\endcode
*/
template <class Dtype>
DL_Node<Dtype>* DL_Iter<Dtype>::insend(Dtype newitem)
{
	if (_current==0)
		Error("insend()",NO_LIST);
	if (_list->_iterlevel > 1)
		Error("insend()",ITER_GT_1);

	_list->_iterlevel--;
   DL_Node<Dtype>* ret = _list->insend(newitem);
	_list->_iterlevel++;
    return ret;
}


/*!
insert the object given at the begin of the list
\note  The iterator level must be one to be able to use this function, else an error will be generated

\note  Use this function if an iterator is needed to do more complex things. Else use the list member functions directly.

\par Example:
 to insert integer a at begin of list|
\code
DL_List<int> _intlist; #create a list of integers

int a=123;

DL_Iter<int>*  a_listiter=new DL_Iter<int>(&_intlist);

a_listiter->insbegin(a);
\endcode
\param newitem an object for which the template list/iterator was generated
*/
template <class Dtype>
DL_Node<Dtype>* DL_Iter<Dtype>::insbegin(Dtype newitem)
{
	if (_current==0)
		Error("insbegin()",NO_LIST);
	if (_list->_iterlevel > 1)
		Error("insbegin()",ITER_GT_1);

	_list->_iterlevel--;
   DL_Node<Dtype>* ret = _list->insbegin(newitem);
	_list->_iterlevel++;
    return ret;    
}

/*!
//insert the object given before the current position of the iterator in list
\note  The iterator level must be one to be able to use this function, else an error will be generated
\par Example:
   to insert integer before the iterator position in the list|
\code
DL_List<int> _intlist; #create a list of integers

int a=123;

DL_Iter<int>*  a_listiter=new DL_Iter<int>(&_intlist);
a_listiter->totail();
a_listiter->insbefore(a);   // insert before tail
\endcode
\param newitem an object for which the template list/iterator was generated
*/
template <class Dtype>
DL_Node<Dtype>* DL_Iter<Dtype>::insbefore(Dtype newitem)
{
	if (_current==0)
		Error("insbefore()",NO_LIST);
	if (_list->_iterlevel > 1)
		Error("insbefore()",ITER_GT_1);

	DL_Node<Dtype>* newnode = new DL_Node<Dtype>(newitem);

   newnode ->_next = _current;
   _current->_prev->_next = newnode;
   newnode ->_prev = _current->_prev;
   _current->_prev = newnode;

	NB++;
    return newnode;
}


/*!
insert the object given after the current position of the iterator in list
\note  The iterator level must be one to be able to use this function, else an error will be generated
\par Example:  to insert integer after the iterator position in the list|
\code
DL_List<int> _intlist; #create a list of integers

int a=123;

DL_Iter<int>*  a_listiter=new DL_Iter<int>(&_intlist);
a_listiter->tohead();
a_listiter->insafter(a);   // insert after head
\endcode
\param newitem an object for which the template list/iterator was generated
*/
template <class Dtype>
DL_Node<Dtype>* DL_Iter<Dtype>::insafter(Dtype newitem)
{
	if (_current==0)
		Error("insafter()",NO_LIST);
	if (_list->_iterlevel > 1)
		Error("insafter()",ITER_GT_1);

	DL_Node<Dtype>* newnode = new DL_Node<Dtype>(newitem);

   newnode ->_next = _current->_next;
   newnode ->_prev = _current;
   _current->_next->_prev = newnode;
   _current->_next = newnode;

	NB++;
    return newnode;
}

/*!
sort all items in the list according to the compare function.
when items need to be swapped to reach the right order the swap function will be called also.
\note  There are no restrictions on the internal decision in the compare function when to return -1,0,1.

\note  The swap function can be used to change items when they are swapped.
       fcmp (function, fcmp)
\verbatim

          Declaration: int (*fcmp) (Dtype,Dtype)

          compare function pointer, the function takes two objects in the list. It must return -1, 0, 1, to sort the list in upgoing
          order the function should return:

               -1 is returned if the first object is bigger then the second.
               0 is returned if the objects are equal.
               1 is returned if the first object is smaller then the second.

          To sort the list in downgoing order:

               1 is returned if the first object is bigger then the second.
               0 is returned if the objects are equal.
               -1 is returned if the first object is smaller then the second.

          fswap (function, fswap)

          Declaration: void (*fswap) (Dtype,Dtype)

          swap function pointer, the function takes two objects in the list. It will be called when the objects are swapped to
          reach the right order. If it is NULL, it will not be called.
\endverbatim
\par Example:  sort the list in upgoing order using cocktailsort and the function numbersorter|
\code
int numbersorter(int a,int b)
{
      if(a < b) return(1);
      else
      if(a == b) return(0);
      return(-1);
}

DL_List <int> _intlist; #create a list of integers
DL_Iter<int>*  a_listiter=new DL_Iter<int>(&_intlist);

a_listiter->insend(2345);
a_listiter->insend(3456);
a_listiter->insend(1234);
a_listiter->cocktailsort(numbersorter,NULL);
\endcode
\param fcmp sortfunction
\param fswap swapfunction
*/
template <class Dtype>
int DL_Iter<Dtype>::cocktailsort(int (*fcmp) (Dtype, Dtype), bool (*fswap)(Dtype, Dtype))
{
	if (_current==0)
		Error("cocktailsort()",NO_LIST);
	if (NB <= 1)
		return 0;

	DL_Node<Dtype>* cursor;
	Dtype swap;
    
    int swapResult = 0;

	// boven/ondergrens setten
	DL_Node<Dtype> *bg = TL, *bgold = TL;
	DL_Node<Dtype> *og = HD, *ogold = HD;

	bool swapped = true;

	// while swaping is done  &  lowerborder upperborder don't touch
	while (swapped && (og!=bg))
	{
		swapped = false;

		// BUBBELSLAG  lowerborder--->> upperborder
		cursor=og;
		while(!(cursor == bgold))
		{
			// (current.next < current)?
			if( fcmp(cursor->_next->_item, cursor->_item)==1)
			{
				// user function
				if ( fswap != NULL )
					if ( fswap(cursor->_item, cursor->_next->_item) )
                    swapResult++;
				// update swap-flag en upperborder
				swapped = true;
				bg = cursor;
				// swap the items of the nodes
				swap = cursor->_item;
				cursor->_item = cursor->_next->_item;
				cursor->_next->_item = swap;
			}
			cursor=cursor->_next;
		}// end bubbelslag
		bgold = bg;

		// BRICKSLAG   lowerborder <<---upperborder
		cursor=bg;
		while(!(cursor == ogold))
		{
			// (current < current.next)?
			if( fcmp(cursor->_item, cursor->_prev->_item)==1)
			{
				// user function
				if ( fswap != NULL )
					if (  fswap(cursor->_item, cursor->_prev->_item) )
                        swapResult++;
				// update swap-flag and lowerborder
				swapped = true;
				og = cursor;
				// swap de items van de nodes
				swap = cursor->_item;
				cursor->_item = cursor->_prev->_item;
				cursor->_prev->_item = swap;
			}
			cursor=cursor->_prev;
		}// end brickslag
		ogold = og;
	}// end while(ongesorteerd)
    
    return swapResult;
}

/*!
   sort all items in the list according to the compare function.

\note
   There are no restrictions on the internal decision in the compare function when to return -1,0,1.

\note
   For the mergesort function the objects will be swapped when the return value is -1.

\note
\verbatim

      fcmp (function, fcmp)

         Declaration: int (*fcmp) (Dtype,Dtype)

          compare function pointer, the function takes two objects in the list. It must return -1, 0, 1, to sort the list in upgoing
          order the function should return:

               -1 is returned if the first object is bigger then the second.
               0 is returned if the objects are equal.
               1 is returned if the first object is smaller then the second.

          To sort the list in downgoing order:

               1 is returned if the first object is bigger then the second.
               0 is returned if the objects are equal.
               -1 is returned if the first object is smaller then the second.
\endverbatim

!tcarg: class | Dtype | list item object
\par example
   sort the list in upgoing order using mergesort and the function numbersorter|
\code
int numbersorter(int a,int b)
{
      if(a < b) return(1);
      else
      if(a == b) return(0);
      return(-1);
}

DL_List <int> _intlist; #create a list of integers
DL_Iter<int>*  a_listiter=new DL_Iter<int>(&_intlist);

a_listiter->insend(2345);
a_listiter->insend(3456);
a_listiter->insend(1234);
a_listiter->mergesort(numbersorter);
\endcode
*/
template <class Dtype>
void DL_Iter<Dtype>::mergesort(int (*fcmp) (Dtype, Dtype))
{
	if (_current==0)
		Error("mergesort()",NO_LIST);
 	mergesort_rec(fcmp, RT, NB);
}


template <class Dtype>
void DL_Iter<Dtype>::mergesort_rec(int (*fcmp)(Dtype,Dtype), DL_Node<Dtype> *RT1, int n1)
{
   if (n1 > 1)  //one element left then stop
   {
		DL_Node<Dtype>       RT2;
	   int n2;

      RT2._prev=RT1->_prev;
      RT2._next=RT1->_next;
      // goto middle
      n2=n1;n1>>=1;n2-=n1;
      for (int i=0; i <n1;i++)
         RT2._next=RT2._next->_next;

      //RT2 is at half
      RT1->_prev->_next=&RT2;
      RT2._prev=RT1->_prev;
      RT1->_prev=RT2._next->_prev;
      RT2._next->_prev->_next=RT1;

     	mergesort_rec(fcmp,RT1,n1);
     	mergesort_rec(fcmp,&RT2,n2);
     	mergetwo(fcmp,RT1,&RT2);
   }
}

template <class Dtype>
void DL_Iter<Dtype>::mergetwo(int (*fcmp)(Dtype,Dtype), DL_Node<Dtype> *RT1,DL_Node<Dtype> *RT2)
{
	DL_Node<Dtype>       *c,*a,*b;
   a=RT1->_next;b=RT2->_next;
   c=RT1;
   do
   {
      if (fcmp(a->_item , b->_item) > -1)
      { c->_next=a;a->_prev=c;c=a;a=a->_next;}
      else
      { c->_next=b;b->_prev=c;c=b;b=b->_next;}
      if (a == RT1)
      { 	c->_next=
          b;b->_prev=c; //connect list b to the list made sofar
         RT1->_prev=RT2->_prev;
         RT1->_prev->_next=RT1;
      	break;
      }
      if (b == RT2)
      { 	c->_next=a;a->_prev=c; //connect list a to the list made sofar
      	break;
      }
   }
   while (true);
}


//=======================================================================
// implementation class DL_StackIter
//=======================================================================
/*! \class DL_StackIter
*  template class DL_StackIter class for stack iterator on DL_List
*  template stack iterator for any list/node type \n
*  This class is the base class to attach/instantiate a stack iterator on a double linked list
*  DL_List. The stack iterator is used to push and pop objects
*  to and from the beginning of a list.
*  class | Dtype | Object for traversing a DL_List of the same Dtype
*\par Example
   How to work with a stack iterator for a list of type integer \n
   to push a and b, pop a into list and remove_all directly using a stack iterator
*
*\code     DL_List<int>* a_list = new DL_List<int>();# declaration and allocation
*
*     int a=123;
*
*     int b=345;
*
*     {
*
*             DL_StackIter<int>*  a_listiter=new DL_StackIter<int>(a_list);
*
*             a_listiter->push(a)
*
*             a_listiter->push(b)
*
*             a_listiter->pop()
*
*             a_listiter->remove_all()
*
*     } //to destruct the iterator before the list is deleted
*
*     delete a_list; #delete it (must have no iterators attached to it)
*\endcode
*/

// constructor
template <class Dtype>
DL_StackIter<Dtype>::DL_StackIter(DL_List<Dtype> *newlist)
:DL_Iter<Dtype>(newlist)  // initialiseer BaseIter
{}


// destructor
template <class Dtype>
DL_StackIter<Dtype>::~DL_StackIter()
{
}

// plaats nieuw item op stack
template <class Dtype>
void DL_StackIter<Dtype>::push(Dtype newitem)
{
	DL_Iter<Dtype>::insbegin(newitem);
}
// remove current item
template <class Dtype>
void DL_StackIter<Dtype>::remove_all()
{
	DL_Iter<Dtype>::remove_all();
}

// is stack leeg?
template <class Dtype>
bool DL_StackIter<Dtype>::empty()
{
	return DL_Iter<Dtype>::empty();
}

// aantal items op stack
template <class Dtype>
int  DL_StackIter<Dtype>::count()
{
	return DL_Iter<Dtype>::count();
}

// haal bovenste item van stack
template <class Dtype>
Dtype DL_StackIter<Dtype>::pop()
{
	if(DL_Iter<Dtype>::empty())
		this->Error("pop()",EMPTY);

	DL_Iter<Dtype>::tohead();
	Dtype temp = DL_Iter<Dtype>::item();
	DL_Iter<Dtype>::removehead();
	return temp;
}

//=======================================================================
// implementation class DL_SortIter
//=======================================================================
/*! \class DL_SortIter
* template class DL_SortIter
* class for sort iterator on DL_List
* template sort iterator for any list/node type
* This class is a derived class to attach/instantiate a sorted iterator on a double linked list
* DL_List. The iterator is used to insert items in sorted order into a list.
//!tcarg: class | Dtype | Object for traversing a DL_List of the same Dtype
*/

// constructor
template <class DType>
DL_SortIter<DType>::DL_SortIter(DL_List<DType>* nw_list, int (*new_func)(DType ,DType ))
:DL_Iter<DType>(nw_list), comparef(new_func)
{}

// destructor
template <class DType>
DL_SortIter<DType>::~DL_SortIter()
{}

// general function to insert item
template <class DType>
void DL_SortIter<DType>::insert(DType new_item)
{
	DL_Node<DType>* cursor=this->_current; //can be 0 if empty  //node is a temporary cursor

	// if list is empty directly insert
	if (DL_Iter<DType>::empty())
	{
		DL_Iter<DType>::insend(new_item);
	}
	else
	{
		// put new item left of item
		DL_Iter<DType>::tohead();
		while(!DL_Iter<DType>::hitroot())
		{
			if (!(*comparef)(DL_Iter<DType>::item(), new_item))
				break;
			DL_Iter<DType>::next();
		}

		//if at root
		DL_Iter<DType>::insbefore(new_item);
	}

	this->_current=cursor; //set to old cursor position
}

template <class DType>
void DL_SortIter<DType>::sortitererror()
{
 		this->Error("sortiter()",NOT_ALLOW);
}


