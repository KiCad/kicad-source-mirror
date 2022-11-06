#ifndef _PROPERTIES_H_
#define _PROPERTIES_H_

/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2016 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2016-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string>
#include <map>
#include <utf8.h>


/**
 * A name/value tuple with unique names and optional values.  The names
 * may be iterated alphabetically.
 */
class STRING_UTF8_MAP : public std::map< std::string, UTF8 >
{
public:
    bool Clear( const std::string& aProperty )
    {
        return erase( aProperty ) > 0;
    }

    bool Exists( const std::string& aProperty ) const
    {
        return count( aProperty ) > 0;
    }

    /**
     * Fetch a property by \a aName and returns true if that property was found, else false.
     * If not found, \a aFetchedValue is not touched.
     *
     * @param aName is the property or option to look for.
     * @param aFetchedValue is where to put the value of the property if it exists and
     *                      \a  aFetchedValue is not NULL.
     * @return true if property is found, else false.
     */
    bool Value( const char* aName, UTF8* aFetchedValue = nullptr ) const;
};

#endif  // _PROPERTIES_H_

//  LocalWords:  aName aFetchedValue
