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

#ifndef PCBNEW_ARRAY_PAD_NAME_PROVIDER__H
#define PCBNEW_ARRAY_PAD_NAME_PROVIDER__H


#include <array_options.h>
#include <class_module.h>

/**
 * Simple class that sequentially provides names from an #ARRAY_OPTIONS
 * object, making sure that they do not conflict with names already existing
 * in a #MODULE.
 */
class ARRAY_PAD_NAME_PROVIDER
{
public:
    /**
     * @param aMod          the module to gather existing names from (nullptr for no module)
     * @oaram aArrayOpts    the array options that provide the candidate names
     */
    ARRAY_PAD_NAME_PROVIDER( const MODULE* aMod, const ARRAY_OPTIONS& aArrayOpts );

    /**
     * Get the next available pad name.
     */
    wxString GetNextPadName();

private:
    /**
     * Get the next name from a given index/list combo
     * @param  aIndex    index to start at, will be updated
     * @param  aExisting the set of existing names to skip
     * @return           the first name found that's not in aExisting
     */
    wxString getNextName( int& aIndex, const std::set<wxString>& aExisting );

    const ARRAY_OPTIONS& m_arrayOpts;
    std::set<wxString>   m_existing_pad_names;
    int                  m_current_pad_index;
};

#endif // PCBNEW_ARRAY_PAD_NAME_PROVIDER__H