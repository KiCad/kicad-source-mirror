/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <array_pad_name_provider.h>

#include <class_pad.h>


ARRAY_PAD_NAME_PROVIDER::ARRAY_PAD_NAME_PROVIDER(
        const MODULE* aMod, const ARRAY_OPTIONS& aArrayOpts )
        : m_arrayOpts( aArrayOpts )
{
    // start by numbering the first new item
    m_current_pad_index = 0;

    // construct the set of existing pad numbers
    if( aArrayOpts.GetNumberingStartIsSpecified() )
    {
        // if we start from a specified point, we don't look at existing
        // names, so it's just an empty "reserved" set
    }
    else
    {
        // no module, no reserved names either
        if( aMod )
        {
            // reserve the name of each existing pad
            for( auto pad : aMod->Pads() )
            {
                m_existing_pad_names.insert( pad->GetName() );
            }
        }
    }
}


wxString ARRAY_PAD_NAME_PROVIDER::GetNextPadName()
{
    return getNextName( m_current_pad_index, m_existing_pad_names );
}


wxString ARRAY_PAD_NAME_PROVIDER::getNextName( int& aIndex, const std::set<wxString>& aExisting )
{
    wxString next_name;

    do
    {
        next_name = m_arrayOpts.GetItemNumber( aIndex );
        aIndex++;
    } while( aExisting.count( next_name ) != 0 );

    return next_name;
}
