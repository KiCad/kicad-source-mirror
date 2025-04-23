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

#ifndef PCBNEW_ARRAY_PAD_NAME_PROVIDER__H
#define PCBNEW_ARRAY_PAD_NAME_PROVIDER__H


#include <array_options.h>
#include <footprint.h>
#include <board_commit.h>

/**
 * Simple class that sequentially provides numbers from an #ARRAY_OPTIONS object, making sure
 * that they do not conflict with numbers already existing in a #FOOTPRINT.
 */
class ARRAY_PAD_NUMBER_PROVIDER
{
public:
    /**
     * @param aFootprint the footprint to gather existing numbers from (nullptr for no footprint)
     * @param aArrayOpts the array options that provide the candidate numbers
     */
    ARRAY_PAD_NUMBER_PROVIDER( const std::set<wxString>& aExistingPadNumbers, const ARRAY_OPTIONS& aArrayOpts );

    /**
     * Get the next available pad name.
     */
    wxString GetNextPadNumber();

private:
    /**
     * Get the next number from a given index/list combo
     * @param  aIndex    index to start at, will be updated
     * @param  aExisting the set of existing numbers to skip
     * @return           the first number found that's not in aExisting
     */
    wxString getNextNumber( int& aIndex, const std::set<wxString>& aExisting );

    const ARRAY_OPTIONS& m_arrayOpts;
    std::set<wxString>   m_existing_pad_numbers;
    int                  m_current_pad_index;
};

#endif // PCBNEW_ARRAY_PAD_NAME_PROVIDER__H
