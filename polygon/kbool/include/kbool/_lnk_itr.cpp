/*! \file kbool/_lnk_itr.cpp
    \author Probably Klaas Holwerda
 
    Copyright: 2001-2004 (C) Probably Klaas Holwerda
 
    Licence: see kboollicense.txt 
 
    RCS-ID: $Id: _lnk_itr.cpp,v 1.4 2009/02/06 21:33:03 titato Exp $
*/

#ifdef __UNIX__
#include "kbool/_lnk_itr.h"
#endif

//=======================================================================
// implementation class LinkBaseIter
//=======================================================================

template<class Type>
TDLI<Type>::TDLI( DL_List<void*>* newlist ): DL_Iter<void*>( newlist )
{}

template<class Type>
TDLI<Type>::TDLI( DL_Iter<void*>* otheriter ): DL_Iter<void*>( otheriter )
{}

template<class Type>
TDLI<Type>::TDLI(): DL_Iter<void*>()
{}

// destructor TDLI
template<class Type>
TDLI<Type>::~TDLI()
{}

template<class Type>
void TDLI<Type>::delete_all()
{
    DL_Node<void*>* node;
    Type* obj;
    for ( int i = 0; i < NB; i++ )
    {
        node = HD;
        HD = node->_next;
        obj = ( Type* )( node->_item );
        delete obj;
        delete node;
    }
    NB = 0; //reset memory used (no lost pointers)
    TL = RT;
    _current = RT;
}

template<class Type>
void TDLI<Type>::foreach_f( void ( *fp ) ( Type* item ) )
{
    DL_Iter<void*>::foreach_f( ( void ( * )( void* ) )fp ); //call fp for each item
}

template<class Type>
void TDLI<Type>::foreach_mf( void ( Type::*mfp ) () )
{

    DL_Node<void*>* node = HD; //can be 0 if empty
    Type* obj;
    for( int i = 0; i < NB; i++ )
    {
        obj = ( Type* )( node->_item );
        ( obj->*mfp )();
        node = node->_next;
    }
}

template<class Type>
void TDLI<Type>::takeover( DL_List<void*>* otherlist )
{
    DL_Iter<void*>::takeover( ( DL_List<void*>* ) otherlist );
}

template<class Type>
void TDLI<Type>::takeover( TDLI* otheriter )
{
    DL_Iter<void*>::takeover( ( DL_Iter<void*>* ) otheriter );
}

template<class Type>
void TDLI<Type>::takeover( TDLI* otheriter, int maxcount )
{
    DL_Iter<void*>::takeover( ( DL_Iter<void*>* ) otheriter, maxcount );
}

// is item element of the list?
template<class Type>
bool TDLI<Type>::has( Type* otheritem )
{
    return DL_Iter<void*>::has( ( void* ) otheritem );
}

// goto to item
template<class Type>
bool TDLI<Type>::toitem( Type* item )
{
    return DL_Iter<void*>::toitem( ( void* ) item );
}

// get current item
template<class Type>
Type*    TDLI<Type>::item()
{
    return ( Type* ) DL_Iter<void*>::item();
}

template<class Type>
void TDLI<Type>::insend( Type* newitem )
{
    DL_Iter<void*>::insend( ( void* ) newitem );
}

template<class Type>
void TDLI<Type>::insbegin( Type* newitem )
{
    DL_Iter<void*>::insbegin( ( void* ) newitem );
}

template<class Type>
void TDLI<Type>::insbefore( Type* newitem )
{
    DL_Iter<void*>::insbefore( ( void* ) newitem );
}

template<class Type>
void TDLI<Type>::insafter( Type* newitem )
{
    DL_Iter<void*>::insafter( ( void* ) newitem );
}

template<class Type>
void TDLI<Type>::insend_unsave( Type* newitem )
{
    short int iterbackup = _list->_iterlevel;
    _list->_iterlevel = 0;
    DL_Iter<void*>::insend( ( void* ) newitem );
    _list->_iterlevel = iterbackup;
}

template<class Type>
void TDLI<Type>::insbegin_unsave( Type* newitem )
{
    short int iterbackup = _list->_iterlevel;
    _list->_iterlevel = 0;
    DL_Iter<void*>::insbegin( ( void* ) newitem );
    _list->_iterlevel = iterbackup;
}

template<class Type>
void TDLI<Type>::insbefore_unsave( Type* newitem )
{
    short int iterbackup = _list->_iterlevel;
    _list->_iterlevel = 0;
    DL_Iter<void*>::insbefore( ( void* ) newitem );
    _list->_iterlevel = iterbackup;
}

template<class Type>
void TDLI<Type>::insafter_unsave( Type* newitem )
{
    short int iterbackup = _list->_iterlevel;
    _list->_iterlevel = 0;
    DL_Iter<void*>::insafter( ( void* ) newitem );
    _list->_iterlevel = iterbackup;
}

template<class Type>
void TDLI<Type>::mergesort( int ( *f )( Type* a, Type* b ) )
{
    DL_Iter<void*>::mergesort( ( int ( * )( void*, void* ) ) f );
}

template<class Type>
int TDLI<Type>::cocktailsort( int ( *f )( Type* a, Type* b ), bool ( *f2 )( Type* c, Type* d ) )
{
    return DL_Iter<void*>::cocktailsort( ( int ( * )( void*, void* ) ) f, ( bool( * )( void*, void* ) ) f2 );
}

template<class Type>
TDLISort<Type>::TDLISort( DL_List<void*>* lista, int ( *newfunc )( void*, void* ) )
        : DL_SortIter<void*>( lista, newfunc )
{}

template<class Type>
TDLISort<Type>::~TDLISort()
{}

template<class Type>
void TDLISort<Type>::delete_all()
{
    DL_Node<void*>* node;
    Type* obj;
    for ( int i = 0; i < NB; i++ )
    {
        node = HD;
        HD = node->_next;
        obj = ( Type* )( node->_item );
        delete obj;
        delete node;
    }
    NB = 0; //reset memory used (no lost pointers)
    TL = RT;
    _current = RT;
}

// is item element of the list?
template<class Type>
bool TDLISort<Type>::has( Type* otheritem )
{
    return DL_Iter<void*>::has( ( void* ) otheritem );
}

// goto to item
template<class Type>
bool TDLISort<Type>::toitem( Type* item )
{
    return DL_Iter<void*>::toitem( ( void* ) item );
}

// get current item
template<class Type>
Type*    TDLISort<Type>::item()
{
    return ( Type* ) DL_Iter<void*>::item();
}

template<class Type>
TDLIStack<Type>::TDLIStack( DL_List<void*>* newlist ): DL_StackIter<void*>( newlist )
{}

// destructor TDLI
template<class Type>
TDLIStack<Type>::~TDLIStack()
{}

// plaats nieuw item op stack
template<class Type>
void TDLIStack<Type>::push( Type* newitem )
{
    DL_StackIter<void*>::push( ( Type* ) newitem );
}


// haal bovenste item van stack
template<class Type>
Type* TDLIStack<Type>::pop()
{
    return ( Type* ) DL_StackIter<void*>::pop();
}


