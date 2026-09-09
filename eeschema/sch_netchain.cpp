/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 KiCad Developers
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

#include <sch_netchain.h>


wxString SCH_NETCHAIN::MakeKey( const wxString& aName, uint32_t aComponent )
{
    if( !aName.IsEmpty() && aName.Find( wxS( "<NO NET>" ) ) == wxNOT_FOUND )
        return aName;

    return wxString( SYNTHETIC_NET_PREFIX ) << aComponent;
}
