/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef EDA_ITEM_TEST_UTILS_H
#define EDA_ITEM_TEST_UTILS_H

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <base_units.h>
#include <eda_item.h>


template <typename T>
static void IterateOverPositionsAndReferences( T* aItem, void ( *aCallback )( T*, VECTOR2I ) )
{
    constexpr int XSTEP = static_cast<int>( schIUScale.mmToIU( 100 ) );
    constexpr int YSTEP = static_cast<int>( schIUScale.mmToIU( 50 ) );
    constexpr int XMIN = -1 * XSTEP;
    constexpr int XMAX = 1 * XSTEP;
    constexpr int YMIN = -1 * YSTEP;
    constexpr int YMAX = 1 * YSTEP;

    for( int posX = XMIN; posX <= XMAX; posX += XSTEP )
    {
        for( int posY = YMIN; posY <= YMAX; posY += YSTEP )
        {
            for( int refX = XMIN; refX <= XMAX; refX += XSTEP )
            {
                for( int refY = YMIN; refY <= YMAX; refY += YSTEP )
                {
                    BOOST_TEST_CONTEXT( wxString::Format( "Position: %d %d, Reference: %d %d",
                                                          posX, posY, refX, refY ) )
                    {
                        aItem->SetPosition( VECTOR2I( posX, posY ) );
                        aCallback( aItem, VECTOR2I( refX, refY ) );
                    }
                }
            }
        }
    }
}

#endif // EDA_ITEM_TEST_UTILS_H
