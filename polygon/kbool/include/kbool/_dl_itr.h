/*! \file kbool/include/kbool/_dl_itr.h
    \author Klaas Holwerda 
 
    Copyright: 2001-2004 (C) Klaas Holwerda
 
    Licence: see kboollicense.txt 
 
    RCS-ID: $Id: _dl_itr.h,v 1.6 2009/09/10 17:04:09 titato Exp $
*/

//! author="Klaas Holwerda"
/*
 * Definitions of classes, for list implementation
 * template list and iterator for any list node type
*/

#ifndef _DL_Iter_H
#define _DL_Iter_H

#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1
#define _CRT_SECURE_NO_DEPRECATE 1

#include "kbool/booleng.h"
#include <stdlib.h>
#include <string>

using namespace std;

#ifndef _STATUS_ENUM
#define _STATUS_ENUM
//!<enum Error codes for List and iterator class
enum  Lerror  {
    NO_MES,              /*!<No Message will be generated */
    NO_LIST,             /*!<List is not attached to the iterator*/
    NO_LIST_OTHER,       /*!<no attached list on other iter*/
    AC_ITER_LIST_OTHER,  /*!<iter not allowed on other list  */
    SAME_LIST,           /*!<same list not allowed*/
    NOT_SAME_LIST,       /*!<must be same list*/
    ITER_GT_1,           /*!<more then one iteriter at root*/
    ITER_GT_0,           /*!<iter not allowed*/
    ITER_HITROOT,        /*!<iter at root*/
    NO_ITEM,             /*!<no item at current*/
    NO_NEXT,             /*!<no next after current*/
    NO_PREV,             /*!<no prev before current */
    EMPTY,               /*!<list is empty*/
    NOT_ALLOW,           /*!<not allowed*/
    ITER_NEG             /*!<to much iters deleted*/
};
#endif

#define SWAP(x,y,t)((t)=(x),(x)=(y),(y)=(t))
#define RT _list->_root
#define HD _list->_root->_next
#define TL _list->_root->_prev
#define NB _list->_nbitems

template <class Dtype> class DL_List;
template <class Dtype> class DL_Iter;
template <class Dtype> class DL_SortIter;

//!   Template class DL_Node
template <class Dtype>  class DL_Node
{
    friend class DL_List<Dtype>;
    friend class DL_Iter<Dtype>;
    friend class DL_SortIter<Dtype>;

    //!Public members
public:
    //!Template constructor no contents
    //!Construct a node for a list object
    DL_Node();

    //!constructor with init of Dtype
    DL_Node( Dtype n );

    //!Destructor
    ~DL_Node();

    //!Public members
public:
    //!data in node
    Dtype _item;

    //!pointer to next node
    DL_Node* _next;

    //!pointer to previous node
    DL_Node* _prev;
};

//!Template class DL_List
template <class Dtype> class DL_List
{
    friend class DL_Iter<Dtype>;
    friend class DL_SortIter<Dtype>;

public:
    //!Constructor
    //!Construct a list object
    //!!tcarg class | Dtype | list object
    DL_List();

    //!destructor
    ~DL_List();

    //!Report off List Errors
	void Error( string function, Lerror a_error );

    //!Number of items in the list
    int  count();

    //!Empty List?
    bool empty();

    //!insert the object given at the end of the list, after tail
    DL_Node<Dtype>* insend( Dtype n );

    //!insert the object given at the begin of the list, before head
    DL_Node<Dtype>* insbegin( Dtype n );

    //!remove the object at the begin of the list (head)
    void removehead();

    //! remove the object at the end of the list (tail)
    void removetail();

    //!remove all objects from the list
    void remove_all( bool deleteObject = false );

    //!Get the item at the head of the list
    Dtype headitem();

    //!Get the item at the tail of the list
    Dtype tailitem();

    //! to move all objects in a list to this list.
    void takeover( DL_List<Dtype>* otherlist );

public:
    //!the root node pointer of the list, the first and last node
    //! in the list are connected to the root node. The root node is used
    //! to detect the end / beginning of the list while traversing it.
    DL_Node<Dtype>*  _root;

    //!the number of items in the list, if empty list it is 0
    int _nbitems;

    //!number of iterators on the list, Attaching or instantiating an iterator to list,
    //! will increment this member, detaching and
    //! destruction of iterator for a list will decrement this number
    short int _iterlevel;
};

//!  Template class DL_Iter  for iterator on DL_List
template <class Dtype>
class DL_Iter
{
public:
    //!Construct an iterator object for a given list of type Dtype
    DL_Iter( DL_List<Dtype>* newlist );

    //!Constructor of iterator for the same list as another iterator
    DL_Iter( DL_Iter* otheriter );

    //!Constructor without an attached list
    DL_Iter();

    //!destructor
    ~DL_Iter();

    //!Report off Iterator Errors
    void    Error( string function, Lerror a_error );

    //!This attaches an iterator to a list of a given type.
    void    Attach( DL_List<Dtype>* newlist );

    //!This detaches an iterator from a list
    void    Detach();

    //!execute given function for each item in the list/iterator
    void    foreach_f( void ( *fp ) ( Dtype n ) );

    //! list mutations

    //!insert after tail item
    DL_Node<Dtype>*  insend( Dtype n );

    //!insert before head item
    DL_Node<Dtype>* insbegin( Dtype n );

    //!insert before current iterator position
    DL_Node<Dtype>* insbefore( Dtype n );

    //!insert after current iterator position
    DL_Node<Dtype>* insafter( Dtype n );

    //!to move all objects in a list to the list of the iterator.
    void    takeover( DL_List<Dtype>* otherlist );

    //!to move all objects in a list (using iterator of that list) to the list of the iterator
    void    takeover( DL_Iter* otheriter );

    //! to move maxcount objects in a list (using iterator of that list) to the list of the iterator
    void    takeover( DL_Iter* otheriter, int maxcount );

    //!remove object at current iterator position from the list.
    void    remove();

    //!Remove head item
    void    removehead();

    //!Remove tail item
    void    removetail();

    //!Remove all items
    void    remove_all();


    /*  void    foreach_mf(void (Dtype::*mfp)() ); //call Dtype::mfp for each item */

    //!is list empty (contains items or not)?
    bool  empty();

    //!is iterator at root node (begin or end)?
    bool  hitroot();

    //!is iterator at head/first node?
    bool  athead();

    //!is iterator at tail/last node?
    bool  attail();

    //!is given item member of the list
    bool  has( Dtype otheritem );

    //!Number of items in the list
    int count();

    /* cursor movements */

    //!go to last item,  if list is empty goto hite
    void  totail();

    //!go to first item, if list is empty goto hite
    void  tohead();

    //!set the iterator position to the root (empty dummy) object in the list.
    void  toroot();

    //! set the iterator position to next object in the list ( can be the root also).
    void    operator++      ( void );

    //!set iterator to next item (pre fix)
    void    operator++      ( int );

    //!set the iterator position to previous object in the list ( can be the root also)(postfix).
    void    operator--      ( void );

    //!set the iterator position to previous object in the list ( can be the root also)(pre fix).
    void    operator--      ( int );

    //!set the iterator position n objects in the next direction ( can be the root also).
    void    operator>>      ( int );

    //!set the iterator position n objects in the previous direction ( can be the root also).
    void    operator<<      ( int );

    //!set the iterator position to next object in the list, if this would be the root object,
    //!then set the iterator at the head object
    void next_wrap();

    //!set the iterator position to previous object in the list, if this would be the root object,
    //!then set the iterator at the tail object
    void prev_wrap();

    //!move root in order to make the current node the tail
    void reset_tail();

    //!move root in order to make the current node the head
    void reset_head();

    //!put the iterator at the position of the given object in the list.
    bool toitem( Dtype );

    //!put the iterator at the same position as the given iterator in the list.
    void toiter( DL_Iter* otheriter );

    //!put the iterator at the position of the given node in the list.
    bool tonode( DL_Node<Dtype>* );

    //!iterate through all items of the list
    bool iterate( void );

    //!To get the item at the current iterator position
    Dtype item();

    //! get node at iterator
    DL_Node<Dtype>* node();

    //!sort list with mergesort
    void mergesort( int ( *fcmp ) ( Dtype, Dtype ) );

    //!sort list with cocktailsort
    /*!
            \return number of swaps done.
      */
    int cocktailsort( int ( * )( Dtype, Dtype ), bool ( * )( Dtype, Dtype ) = NULL );

protected:

    //!sort list with mergesort
    void mergesort_rec( int ( *fcmp )( Dtype, Dtype ), DL_Node<Dtype> *RT1, int n );

    //!sort list with mergesort
    void mergetwo( int ( *fcmp )( Dtype, Dtype ), DL_Node<Dtype> *RT1, DL_Node<Dtype> *RT2 );

    //!set the iterator position to next object in the list ( can be the root also).
    void next();

    //!set the iterator position to previous object in the list ( can be the root also).
    void prev();

    //!the list for this iterator
    DL_List<Dtype> *_list;

    //!the current position of the iterator
    DL_Node<Dtype> *_current;
};


//!  template class DL_StackIter class for stack iterator on DL_List
template <class Dtype>
class DL_StackIter : protected DL_Iter<Dtype>
{
public:
    //!Constructor of stack iterator for given list
    DL_StackIter( DL_List<Dtype> * );
    //!Constructor of stack iterator no list attached
    DL_StackIter();

    //!Destructor of stack iterator
    ~DL_StackIter();

    //!Remove all items from the stack
    void remove_all();
    //!push given item on the stack
    void push( Dtype n );
    //!get last inserted item from stack
    Dtype pop();
    //!is stack empty?
    bool empty();
    //!number of items on the stack
    int count();
};

//!template class DL_SortIter
template <class DType> class DL_SortIter : public DL_Iter<DType>
{
public:
    //!Constructor of sort iterator for given list and sort function
    DL_SortIter( DL_List<DType>* nw_list, int ( *new_func )( DType , DType ) );

    //!Constructor of sort iterator with sort function and no list attached
    DL_SortIter( int ( *newfunc )( DType, DType ) );

    //!Destructor of sort iterator
    ~DL_SortIter();

    //!insert item in sorted order
    void insert ( DType new_item );

    /*override following functions to give an error */
    //!Not allowed
    void   insend ( bool n ){sortitererror();};
    //!Not allowed
    void   insbegin ( bool n ){sortitererror();};
    //!Not allowed
    void   insbefore ( bool n ){sortitererror();};
    //!Not allowed
    void   insafter ( bool n ){sortitererror();};
    //!Not allowed
    void   takeover ( DL_List<DType>* ){sortitererror();};
    //!Not allowed
    void   takeover ( DL_Iter<DType>* ){sortitererror();};
    //!Not allowed
    void   takeover ( DL_Iter<DType>* otheriter, int maxcount ){sortitererror();};
    //!Not allowed
    void   next_wrap() {sortitererror();};
    //!Not allowed
    void   prev_wrap() {sortitererror();};
    //!Not allowed
    void   reset_tail() {sortitererror();};
    //!Not allowed
    void   reset_head() {sortitererror();};

private:
    //!Report off Iterator Errors
    void sortitererror();

    //!comparefunction used to insert items in sorted order
    int  ( *comparef )( DType, DType );
};

#include "kbool/_dl_itr.cpp"

#endif
