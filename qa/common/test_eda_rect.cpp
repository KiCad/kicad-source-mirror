/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.TXT for contributors.
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

#include "math/util.h"
#include <boost/test/tools/old/interface.hpp>
#include <qa_utils/wx_utils/unit_test_utils.h>

// Code under test
#include <eda_rect.h>


/**
 * Declare the test suite
 */
BOOST_AUTO_TEST_SUITE( EdaRect )

/**
 * Check inflation and deflation
 */
BOOST_AUTO_TEST_CASE( Inflate )
{
    int    width = 20;
    int    height = 30;
    wxSize sizes[] = { wxSize( width, height ), wxSize( width, -height ), wxSize( -width, height ),
                       wxSize( -width, -height ) };

    // We need also to check what happens when deflation exceeds the size.
    int deltas[] = { width, width / 2, width / 4 };

    for( int delta : deltas )
    {
        for( wxSize size : sizes )
        {
            wxPoint  origin = wxPoint( 100, 100 );
            EDA_RECT rect( origin, size );

            // The four corners of the rectangle.
            wxPoint corners[] = { origin + wxSize( 0, 0 ), origin + wxSize( 0, size.y ),
                                  origin + wxSize( size.x, 0 ), origin + wxSize( size.x, size.y ) };

            // Inflation

            wxPoint inflation_corners[] = {
                corners[0] + wxSize( -delta * sign( size.x ), -delta * sign( size.y ) ),
                corners[1] + wxSize( -delta * sign( size.x ), delta * sign( size.y ) ),
                corners[2] + wxSize( delta * sign( size.x ), -delta * sign( size.y ) ),
                corners[3] + wxSize( delta * sign( size.x ), delta * sign( size.y ) )
            };

            for( wxPoint corner : inflation_corners )
            {
                EDA_RECT inflated_rect = rect;
                inflated_rect.Inflate( delta );

                BOOST_CHECK( !rect.Contains( corner ) );
                BOOST_CHECK( inflated_rect.Contains( corner ) );
            }

            // Deflation

            wxPoint deflation_corners[] = {
                corners[0]
                        + wxSize( ( delta - 1 ) * sign( size.x ), ( delta - 1 ) * sign( size.y ) ),
                corners[1]
                        + wxSize( ( delta - 1 ) * sign( size.x ), -( delta - 1 ) * sign( size.y ) ),
                corners[2]
                        + wxSize( -( delta - 1 ) * sign( size.x ), ( delta - 1 ) * sign( size.y ) ),
                corners[3]
                        + wxSize( -( delta - 1 ) * sign( size.x ), -( delta - 1 ) * sign( size.y ) )
            };

            for( wxPoint corner : deflation_corners )
            {
                EDA_RECT deflated_rect = rect;
                deflated_rect.Inflate( -delta );

                // If true, deflation exceeds the size.
                bool zeroed = false;

                if( abs( rect.GetSize().x ) < 2 * delta )
                {
                    BOOST_CHECK_EQUAL( deflated_rect.GetSize().x, 0 );
                    zeroed = true;
                }

                if( abs( rect.GetSize().y ) < 2 * delta )
                {
                    BOOST_CHECK_EQUAL( deflated_rect.GetSize().y, 0 );
                    zeroed = true;
                }

                if( !zeroed )
                {
                    BOOST_CHECK( rect.Contains( corner ) );
                    BOOST_CHECK( !deflated_rect.Contains( corner ) );
                }
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
