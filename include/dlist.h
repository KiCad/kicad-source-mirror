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


class EDA_BaseStruct;


/**
 * Class DHEAD
 * is only for use by template class DLIST, use that instead.
 */
class DHEAD
{
protected:
    EDA_BaseStruct*     first;          ///< first element in list, or NULL if list empty
    EDA_BaseStruct*     last;           ///< last elment in list, or NULL if empty
    unsigned            count;          ///< how many elements are in the list
    bool                meOwner;        ///< I must delete the objects I hold in my destructor

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
     */
    void append( EDA_BaseStruct* aNewElement );

    /**
     * Function insert
     * puts aNewElement just in front of aElementAfterMe in the list sequence.
     * If aElementAfterMe is NULL, then simply Append()
     */
    void insert( EDA_BaseStruct* aNewElement, EDA_BaseStruct* aElementAfterMe );

    /**
     * Function insert
     * puts aNewElement in front of list sequence.
     */
    void insert( EDA_BaseStruct* aNewElement )
    {
        insert( aNewElement, first );
    }

    /**
     * Function remove
     * removes \a aElement from the list, but does not delete it.
     */
    void remove( EDA_BaseStruct* aElement );


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
    unsigned GetCount() { return count; }
};


/**
 * Class DLIST
 * is the head of a doubly linked list.  It contains pointers to the first
 * and last elements in a doubly linked list.  The elements in the list must
 * be of class T or derived from T, and T must be derived from EDA_BaseStruct.
 * @see DHEAD for additional public functions.
 */
template <class T>
class DLIST : public DHEAD
{
public:

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
     */
    void Append( T* aNewElement )
    {
        append( aNewElement );
    }

    /**
     * Function Insert
     * puts aNewElement just in front of aElementAfterMe in the list sequence.
     * If aElementAfterMe is NULL, then simply Append()
     */
    void Insert( T* aNewElement, T* aElementAfterMe )
    {
        insert( aNewElement, aElementAfterMe );
    }

    /**
     * Function Remove
     * removes \a aElement from the list, but does not delete it.
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
     */
    void PushFront( T* aNewElement )
    {
        insert( aNewElement );
    }

    void PushBack( T* aElement )
    {
        append( aElement );
    }

    //-----</ STL like functions >--------------------------------------
};

#endif      // DLIST_H_
