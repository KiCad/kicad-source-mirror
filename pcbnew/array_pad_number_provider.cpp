/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <array_pad_number_provider.h>

#include <pad.h>


ARRAY_PAD_NUMBER_PROVIDER::ARRAY_PAD_NUMBER_PROVIDER( const std::set<wxString>& aExistingPadNumbers,
                                                      const ARRAY_OPTIONS& aArrayOpts )
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
        m_existing_pad_numbers = aExistingPadNumbers;
    }
}


wxString ARRAY_PAD_NUMBER_PROVIDER::GetNextPadNumber()
{
    return getNextNumber( m_current_pad_index, m_existing_pad_numbers );
}


wxString ARRAY_PAD_NUMBER_PROVIDER::getNextNumber( int& aIndex,
                                                   const std::set<wxString>& aExisting )
{
    wxString next_number;

    do
    {
        next_number = m_arrayOpts.GetItemNumber( aIndex );
        aIndex++;
    } while( aExisting.count( next_number ) != 0 );

    return next_number;
}
