/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
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

#include "generators_mgr.h"


GENERATORS_MGR& GENERATORS_MGR::Instance()
{
    static GENERATORS_MGR pm;
    return pm;
}


void GENERATORS_MGR::Register( const wxString& aTypeStr, const wxString& aName,
                               std::function<PCB_GENERATOR*( void )> aCreateFunc )
{
    wxASSERT( !aName.empty() );
    wxASSERT( !aTypeStr.empty() );
    wxASSERT( aCreateFunc );

    ENTRY ent;
    ent.m_createFunc = aCreateFunc;
    ent.m_type = aTypeStr;
    ent.m_displayName = aName;
    m_registry.emplace( aTypeStr, ent );
}


PCB_GENERATOR* GENERATORS_MGR::CreateFromType( const wxString& aTypeStr )
{
    auto it = m_registry.find( aTypeStr );

    if( it == m_registry.end() )
    {
        // TODO: placeholder
        return nullptr;
    }

    ENTRY& entry = it->second;

    return entry.m_createFunc();
}