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

#include <qa_utils/uuid_test_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

namespace KI_TEST
{

bool IsUUID( const std::string& aStr )
{
    std::regex uuid( "[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}" );
    std::smatch match;

    return std::regex_match( aStr, match, uuid );
}


bool IsUUIDPathWithLevels( const std::string& aStr, unsigned aLevels )
{
    // A UUID is formatted as 8-4-4-4-12
    const unsigned tsLen = 36;
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
        if( !IsUUID( std::string( tsBegin, tsBegin + tsLen ) ) )
        {
            BOOST_TEST_INFO( "Not a UUID at level "
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
