/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <algorithm>

#include <eda_cursor_broadcaster.h>


EDA_CURSOR_BROADCASTER& EDA_CURSOR_BROADCASTER::Instance()
{
    static EDA_CURSOR_BROADCASTER instance;
    return instance;
}


void EDA_CURSOR_BROADCASTER::Register( EDA_CURSOR_OWNER* aOwner )
{
    m_owners.push_back( aOwner );
}


void EDA_CURSOR_BROADCASTER::Unregister( EDA_CURSOR_OWNER* aOwner )
{
    m_owners.erase( std::remove( m_owners.begin(), m_owners.end(), aOwner ), m_owners.end() );
}


void EDA_CURSOR_BROADCASTER::NotifyCursorEntered( EDA_CURSOR_OWNER* aEnteredOwner )
{
    for( EDA_CURSOR_OWNER* owner : m_owners )
    {
        if( owner != aEnteredOwner && owner->IsCursorInCanvas() )
            owner->HideCursorInCanvas();
    }
}
