/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Simon Richter
 * Copyright (C) 2015 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#ifndef PIN_NUMBER_H_
#define PIN_NUMBER_H_ 1

#include <wx/string.h>

#include <set>

typedef wxString PinNumber;

class PinNumbers
{
public:
    static int Compare( PinNumber const &lhs, PinNumber const &rhs );

private:
    struct less
    {
        bool operator()( PinNumber const &lhs, PinNumber const &rhs ) const
        {
            return Compare( lhs, rhs ) < 0;
        }
    };

    typedef std::set< PinNumber, less > container_type;

public:
    typedef container_type::value_type value_type;
    typedef container_type::iterator iterator;
    typedef container_type::const_iterator const_iterator;

    void insert( value_type const &v ) { pins.insert( v ); }
    container_type::size_type size() const { return pins.size(); }

    iterator begin() { return pins.begin(); }
    iterator end() { return pins.end(); }
    const_iterator begin() const { return pins.begin(); }
    const_iterator end() const { return pins.end(); }

private:
    container_type pins;
};

#endif
