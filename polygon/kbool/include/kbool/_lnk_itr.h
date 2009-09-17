/*! \file kbool/include/kbool/_lnk_itr.h
    \author Klaas Holwerda
 
    Copyright: 2001-2004 (C) Klaas Holwerda
 
    Licence: see kboollicense.txt 
 
    RCS-ID: $Id: _lnk_itr.h,v 1.4 2009/09/10 17:04:09 titato Exp $
*/

//! author="Klaas Holwerda"
//! version="1.0"
/*
 * Definitions of classes, for list implementation
 * template list and iterator for any list node type
*/
#ifndef _LinkBaseIter_H
#define _LinkBaseIter_H

//! headerfiles="_dl_itr.h stdlib.h misc.h gdsmes.h"
#include <stdlib.h>
#include "kbool/booleng.h"

#define SWAP(x,y,t)((t)=(x),(x)=(y),(y)=(t))

#include "kbool/_dl_itr.h"

//! codefiles="_dl_itr.cpp"

//!  Template class TDLI
/*!
 class for iterator on DL_List<void*> that is type casted version of DL_Iter
 \sa DL_Iter for further documentation
*/
template<class Type> class TDLI : public DL_Iter<void*>
{
public:
    //!constructor
    /*!
    \param list to iterate on.
    */
    TDLI( DL_List<void*>* list );

    //!constructor
    TDLI( DL_Iter<void*>* otheriter );

    //! nolist constructor
    TDLI();

    //! destructor
    ~TDLI();

    //!call fp for each item
    void    foreach_f( void ( *fp ) ( Type* item ) );

    //!call fp for each item
    void    foreach_mf( void ( Type::*fp ) () );

    /* list mutations */


    //! delete all items
    void    delete_all  ();


    //! insert at end
    void    insend        ( Type* n );

    //! insert at begin
    void    insbegin     ( Type* n );

    //! insert before current
    void    insbefore       ( Type* n );

    //! insert after current
    void    insafter        ( Type* n );

    //! insert at end unsave (works even if more then one iterator is on the list
    //! the user must be sure not to delete/remove items where other iterators
    //! are pointing to.
    void    insend_unsave  ( Type* n );

    //! insert at begin unsave (works even if more then one iterator is on the list
    //! the user must be sure not to delete/remove items where other iterators
    //! are pointing to.
    void    insbegin_unsave  ( Type* n );

    //! insert before iterator position unsave (works even if more then one iterator is on the list
    //! the user must be sure not to delete/remove items where other iterators
    //! are pointing to.
    void    insbefore_unsave ( Type* n );

    //! insert after iterator position unsave (works even if more then one iterator is on the list
    //! the user must be sure not to delete/remove items where other iterators
    //! are pointing to.
    void    insafter_unsave  ( Type* n );

    //! \sa DL_Iter::takeover(DL_List< Dtype >* otherlist )
    void    takeover        ( DL_List<void*>* otherlist );
    //! \sa DL_Iter::takeover(DL_Iter* otheriter)
    void    takeover        ( TDLI* otheriter );
    //! \sa DL_Iter::takeover(DL_Iter* otheriter, int maxcount)
    void    takeover        ( TDLI* otheriter, int maxcount );

    //! \sa DL_Iter::has
    bool  has             ( Type* );
    //! \sa DL_Iter::toitem
    bool  toitem          ( Type* );

    //!get the item then iterator is pointing at
    Type*   item            ();

    //! \sa DL_Iter::mergesort
    void    mergesort             ( int ( *f )( Type* a, Type* b ) );
    //! \sa DL_Iter::cocktailsort
    int  cocktailsort( int ( * ) ( Type* a, Type* b ), bool ( * ) ( Type* c, Type* d ) = NULL );

};

//!  Template class TDLIsort
/*!
// class for sort iterator on DL_List<void*> that is type casted version of DL_SortIter
// see also inhereted class DL_SortIter for further documentation
*/
template<class Type> class TDLISort : public DL_SortIter<void*>
{
public:

    //!constructor givin a list and a sort function
    TDLISort( DL_List<void*>* list, int ( *newfunc )( void*, void* ) );
    ~TDLISort();

    //!delete all items from the list
    void       delete_all();
    bool       has     ( Type* );
    bool       toitem  ( Type* );
    Type* item    ();
};

//!  Template class TDLIStack
/*!
 class for iterator on DL_List<void*> that is type casted version of DL_StackIter
 see also inhereted class DL_StackIter for further documentation
*/
template<class Type> class TDLIStack : public DL_StackIter<void*>
{
public:
    //constructor givin a list
    TDLIStack( DL_List<void*>* list );

    ~TDLIStack();

    void            push( Type* );
    Type*           pop();
};

#include"kbool/_lnk_itr.cpp"

#endif
