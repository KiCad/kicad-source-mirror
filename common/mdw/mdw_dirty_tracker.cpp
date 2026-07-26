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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <mdw/mdw_dirty_tracker.h>


void MDW_DIRTY_TRACKER::SetDirty( const wxString& aItemId )
{
    std::lock_guard<std::mutex> lock( m_mutex );
    m_dirty.insert( aItemId );
    m_generation[aItemId] = ++m_lastGeneration;
}


bool MDW_DIRTY_TRACKER::IsDirty( const wxString& aItemId ) const
{
    std::lock_guard<std::mutex> lock( m_mutex );

    return m_dirty.find( aItemId ) != m_dirty.end();
}


std::uint64_t MDW_DIRTY_TRACKER::Generation( const wxString& aItemId ) const
{
    std::lock_guard<std::mutex> lock( m_mutex );
    auto                        it = m_generation.find( aItemId );

    return it == m_generation.end() ? 0 : it->second;
}


void MDW_DIRTY_TRACKER::ClearIfUnchanged( const wxString& aItemId, std::uint64_t aGeneration )
{
    std::lock_guard<std::mutex> lock( m_mutex );
    auto                        it = m_generation.find( aItemId );

    if( ( it == m_generation.end() ? 0 : it->second ) == aGeneration )
        m_dirty.erase( aItemId );
}
