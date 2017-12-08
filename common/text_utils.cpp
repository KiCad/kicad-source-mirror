/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include <text_utils.h>

std::pair<UTF8, std::vector<bool>> ProcessOverbars( const UTF8& aText )
{
    UTF8 text;
    std::vector<bool> flags;
    bool overbar = false;

    for( UTF8::uni_iter chIt = aText.ubegin(), end = aText.uend(); chIt < end; ++chIt )
    {
        if( *chIt == '~' )
        {
            if( ++chIt >= end )
                break;

            // It was a single tilda, it toggles overbar
            if( *chIt != '~' )
                overbar = !overbar;

            // If it is a double tilda, just process the second one
        }

        // remember: *chIt is not necessary a ASCII7 char.
        // it is an unsigned ( wchar_t ) giving a multibyte char in UTF8 strings
        text += wchar_t( *chIt );

        flags.push_back( overbar );
    }

    return std::make_pair( text, flags );
}
