/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef INTRUSIVE_LIST_H
#define INTRUSIVE_LIST_H

///< A lightweight intrusive list container
template <class T>
class INTRUSIVE_LIST
{
public:
    INTRUSIVE_LIST<T>()
    {
        ListClear();
    }

    void ListClear()
    {
        m_prev  = nullptr;
        m_next  = nullptr;
        m_root  = (T*) this;
        m_count = 1;
    }

    T* ListRemove()
    {
        if( m_prev )
            m_prev->m_next = m_next;

        if( m_next )
            m_next->m_prev = m_prev;

        m_root->m_count--;

        T* rv = nullptr;

        if( m_prev )
            rv = m_prev;
        else if( m_next )
            rv = m_next;

        m_root  = nullptr;
        m_prev  = nullptr;
        m_next  = nullptr;
        return rv;
    }

    int ListSize() const
    {
        return m_root ? m_root->m_count : 0;
    }

    void ListInsert( T* item )
    {
        if( !m_root )
            m_root = item;

        if( m_next )
            m_next->m_prev = item;

        item->m_prev    = (T*) this;
        item->m_next    = m_next;
        item->m_root    = m_root;
        m_root->m_count++;

        m_next = item;
    }

    T* ListNext() const { return m_next; };
    T* ListPrev() const { return m_prev; };

private:
    int m_count;
    T* m_prev;
    T* m_next;
    T* m_root;
};

#endif /* INTRUSIVE_LIST_H */
