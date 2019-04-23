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

#include "timestamp_test_utils.h"

#include <unit_test_utils/unit_test_utils.h>

namespace KI_TEST
{

bool EndsInTimestamp( const std::string& aStr )
{
    if( aStr.size() < 8 )
    {
        BOOST_TEST_INFO( "Too short to be timestamp: " << aStr.size() );
        return false;
    }

    return IsTimeStampish( aStr.end() - 8, aStr.end() );
}

bool IsTimestampStringWithLevels( const std::string& aStr, unsigned aLevels )
{
    const unsigned tsLen = 8;
    const unsigned levelLen = tsLen + 1; // add the /

    if( aStr.size() != aLevels * levelLen + 1 )
    {
        BOOST_TEST_INFO( "String is the wrong length for " << aLevels << " levels." );
        return false;
    }

    if( aStr[0] != '/' )
    {
        BOOST_TEST_INFO( "Doesn't start with '/'" );
        return false;
    }

    auto tsBegin = aStr.begin() + 1;

    for( unsigned i = 0; i < aLevels; i++ )
    {
        if( !IsTimeStampish( tsBegin, tsBegin + tsLen ) )
        {
            BOOST_TEST_INFO( "Not a timeStamp at level "
                             << i << ": " << std::string( tsBegin, tsBegin + tsLen ) );
            return false;
        }

        if( *( tsBegin + tsLen ) != '/' )
        {
            BOOST_TEST_INFO( "level doesn't end in '/'" );
            return false;
        }

        tsBegin += levelLen;
    }

    return true;
}

} // namespace KI_TEST
