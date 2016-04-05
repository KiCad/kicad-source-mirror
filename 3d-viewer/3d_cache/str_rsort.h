/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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

/**
 * @file str_rsort.h
 * provides a wxString sorting functino which works from the
 * end of the string towards the beginning
 */

#ifndef STR_RSORT_H
#define STR_RSORT_H

#include <wx/string.h>

namespace S3D
{

    struct rsort_wxString
    {
        bool operator() (const wxString& strA, const wxString& strB ) const
        {
            // sort a wxString using the reverse character order; for 3d model
            // filenames this will typically be a much faster operation than
            // a normal alphabetic sort
            wxString::const_reverse_iterator sA = strA.rbegin();
            wxString::const_reverse_iterator eA = strA.rend();

            wxString::const_reverse_iterator sB = strB.rbegin();
            wxString::const_reverse_iterator eB = strB.rend();

            if( strA.empty() )
            {
                if( strB.empty() )
                    return false;

                // note: this rule implies that a null string is first in the sort order
                return true;
            }

            if( strB.empty() )
                return false;

            while( sA != eA && sB != eB )
            {
                if( (*sA) == (*sB) )
                {
                    ++sA;
                    ++sB;
                    continue;
                }

                if( (*sA) < (*sB) )
                    return true;
                else
                    return false;
            }

            if( sB == eB )
                return false;

            return true;
        }
    };

};  // end NAMESPACE

#endif  // STR_RSORT_H
