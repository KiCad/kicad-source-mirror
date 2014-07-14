/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2008 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2008 Kicad Developers, see change_log.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


#ifndef DLIST_H_
#define DLIST_H_


#include <stdio.h>          // NULL definition.


class EDA_ITEM;


/**
 * Class DHEAD
 * is only for use by template class DLIST, use that instead.
 */
class DHEAD
{
protected:
    EDA_ITEM*     first;          ///< first element in list, or NULL if list empty
    EDA_ITEM*     last;           ///< last elment in list, or NULL if empty
    unsigned      count;          ///< how many elements are in the list, automatically maintained.
    bool          meOwner;        ///< I must delete the objects I hold in my destructor

    /**
     * Constructor DHEAD
     * is protected so that a DHEAD can only be instantiated from within a
     * DLIST template.
     */
    DHEAD() :
        first(0),
        last(0),
        count(0),
        meOwner(true)
    {
    }

    ~DHEAD();

    /**
     * Function append
     * adds \a aNewElement to the end of the list.
     * @param aNewElement The element to insert.
     */
    void append( EDA_ITEM* aNewElement );

    /**
     * Function append
     * adds \a aList to the end of the list.
     * @param aList The list to aList.
     */
    void append( DHEAD& aList );

    /**
     * Function insert
     * puts \a aNewElement just in front of \a aElementAfterMe in the list sequence.
     * If \a aElementAfterMe is NULL, then simply append().
     * @param aNewElement The element to insert.
     * @param aElementAfterMe The element to insert \a aNewElement before,
     *                        if NULL then append \a aNewElement onto end of list.
     */
    void insert( EDA_ITEM* aNewElement, EDA_ITEM* aElementAfterMe );

    /**
     * Function insert
     * puts \a aNewElement in front of list sequence.
     * @param aNewElement The element to insert.
     */
    void insert( EDA_ITEM* aNewElement )
    {
        insert( aNewElement, first );
    }

    /**
     * Function remove
     * removes \a aElement from the list, but does not delete it.
     * @param aElement The element to remove.
     */
    void remove( EDA_ITEM* aElement );


public:

    /**
     * Function DeleteAll
     * deletes all items on the list and leaves the list empty.  The destructor
     * for each item is called.
     */
    void DeleteAll();

    /**
     * Function SetOwnership
     * controls whether the list owns the objects and is responsible for
     * deleteing their memory at time of this object's destruction.
     */
    void SetOwnership( bool Iown ) { meOwner = Iown; }


    /**
     * Function GetCount
     * returns the number of elements in the list.
     */
    unsigned GetCount() const { return count; }

#if defined(DEBUG)
    void VerifyListIntegrity();
#endif
};


/**
 * Class DLIST
 * is the head of a doubly linked list.  It contains pointers to the first
 * and last elements in a doubly linked list.  The elements in the list must
 * be of class T or derived from T, and T must be derived from EDA_ITEM.
 * @see DHEAD for additional public functions.
 */
template <class T>
class DLIST : public DHEAD
{
public:

// Without the following ifdef, SWIG appends methods from the templated class
#ifndef SWIG
    /**
     * operator T*
     * is a casting operator that returns \a GetFirst(), a T*
     */
    operator T* () const { return GetFirst(); }

    /**
     * operator ->
     * is a dereferencing operator that returns \a GetFirst(), a T*
     */
    T* operator -> () const { return GetFirst(); }
#endif /* SWIG */

    /**
     * Function GetFirst
     * returns the first T* in the list without removing it, or NULL if
     * the list is empty.
     */
    T*  GetFirst() const { return (T*) first; }

    /**
     * Function GetLast
     * returns the last T* in the list without removing it,
     * or NULL if the list is empty.
     */
    T*  GetLast() const { return (T*) last; }

    /**
     * Function Append
     * adds \a aNewElement to the end of the list.
     * @param aNewElement The element to insert.
     */
    void Append( T* aNewElement )
    {
        append( aNewElement );
    }

    /**
     * Function Append
     * adds \a aList to the end of the list.
     * @param aList The list to append to the end of the list.
     */
    void Append( DLIST& aList )
    {
        append( aList );
    }

    /**
     * Function Insert
     * puts \a aNewElement just in front of \a aElementAfterMe in the list sequence.
     * If aElementAfterMe is NULL, then simply Append()
     * @param aNewElement The element to insert.
     * @param aElementAfterMe The element to insert \a aNewElement before,
     *                        if NULL then append \a aNewElement onto end of list.
     */
    void Insert( T* aNewElement, T* aElementAfterMe )
    {
        insert( aNewElement, aElementAfterMe );
    }

    /**
     * Function Remove
     * removes \a aElement from the list, but does not delete it.
     * @param aElement The element to remove from the list.
     * @return T* - the removed element, so you can easily delete it upon return.
     */
    T* Remove( T* aElement )
    {
        remove( aElement );
        return aElement;
    }

    //-----< STL like functions >---------------------------------------
    T* begin() const { return GetFirst(); }
    T* end() const { return NULL; }

    T* PopFront()
    {
        if( GetFirst() )
            return Remove( GetFirst() );
        return NULL;
    }

    T* PopBack()
    {
        if( GetLast() )
            return Remove( GetLast() );
        return NULL;
    }

    /**
     * Function PushFront
     * puts aNewElement at front of list sequence.
     * @param aNewElement The element to insert at the front of the list.
     */
    void PushFront( T* aNewElement )
    {
        insert( aNewElement );
    }

    /**
     * Function PushBack
     * puts aNewElement at the end of the list sequence.
     * @param aNewElement The element to push to the end of the list.
     */
    void PushBack( T* aNewElement )
    {
        append( aNewElement );
    }

    //-----</ STL like functions >--------------------------------------
};

#endif      // DLIST_H_
