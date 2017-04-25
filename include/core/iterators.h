/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
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

#ifndef __ITERATORS_H
#define __ITERATORS_H

#include <dlist.h>
#include <iterator>

template <class T>
class DLIST_ITERATOR : public std::iterator<std::bidirectional_iterator_tag, T>
{
private:
    T m_obj;

    using reference = typename DLIST_ITERATOR<T>::reference;

public:
    explicit DLIST_ITERATOR<T>( T obj ) :
        m_obj(obj) {}

    DLIST_ITERATOR<T>& operator++()
    {
        m_obj = m_obj->Next(); return *this;
    }

    DLIST_ITERATOR<T>& operator--()
    {
        m_obj = m_obj->Prev(); return *this;
    }

    bool operator==( DLIST_ITERATOR<T> other ) const
    {
        return m_obj == other.m_obj;
    }

    bool operator!=( DLIST_ITERATOR<T> other ) const
    {
        return !(*this == other);
    }

    reference operator*()
    {
        return m_obj;
    }
};

// helper object, used to convert a DLIST<T> to an iterator
template <class T>
class DLIST_ITERATOR_WRAPPER
{
public:
    explicit DLIST_ITERATOR_WRAPPER<T> ( DLIST<T>& list ) :
        m_list(list) {}

    DLIST_ITERATOR<T*> begin()
    {
        return DLIST_ITERATOR<T*> ( m_list.GetFirst() );
    }

    DLIST_ITERATOR<T*> end()
    {
        return DLIST_ITERATOR<T*> ( nullptr );
    }

    unsigned int Size() const
    {
        return m_list.GetCount();
    }

private:
    DLIST<T>& m_list;
};

#endif
