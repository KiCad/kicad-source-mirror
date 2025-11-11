/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Simon Richter
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#pragma once

#include <wx/string.h>

#include <set>

class PIN_NUMBERS
{
public:
    wxString GetSummary() const;

    /// <summary>
    /// Gets a formatted string of all the pins that have duplicate numbers.
    /// </summary>
    /// <returns></returns>
    wxString GetDuplicates() const;

    static int Compare( const wxString& lhs, const wxString& rhs );

private:
    struct less
    {
        bool operator()( const wxString& lhs, const wxString& rhs ) const
        {
            return Compare( lhs, rhs ) < 0;
        }
    };

    typedef std::set<wxString, less> container_type;

    static wxString getNextSymbol( const wxString& str, wxString::size_type& cursor );

public:
    typedef container_type::value_type value_type;
    typedef container_type::iterator iterator;
    typedef container_type::const_iterator const_iterator;

    void insert( value_type const& v )
    {
        // Check if the insertion occurred. If no insertion was performed then
        // the pin number is a duplicate so add it to the duplicate set.
        bool not_duplicate = pins.insert( v ).second;

        if( not_duplicate == false )
        {
            duplicate_pins.insert( v );
        }
    }

    container_type::size_type size() const { return pins.size(); }

    iterator begin() { return pins.begin(); }
    iterator end() { return pins.end(); }
    const_iterator begin() const { return pins.begin(); }
    const_iterator end() const { return pins.end(); }

private:
    container_type pins;
    std::set<wxString> duplicate_pins;
};
