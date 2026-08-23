/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <widgets/mathplot.h>

BOOST_AUTO_TEST_SUITE( MathplotLayers )


BOOST_AUTO_TEST_CASE( ConstructWithoutToolkit )
{
    // A layer that took a stock font in its constructor would abort this console-mode binary
    // outright, so the checks below can only be reached, let alone pass, if none of them do
    mpInfoLegend legend;
    mpScaleX     scaleX( wxS( "freq" ) );
    mpScaleY     scaleY( wxS( "gain" ) );

    BOOST_CHECK( !legend.GetFont().IsOk() );
    BOOST_CHECK( !scaleX.GetFont().IsOk() );
    BOOST_CHECK( !scaleY.GetFont().IsOk() );

    // GetPlotFont() only reaches the toolkit for the default, so an explicitly set font
    // resolves without one
    wxFont font( wxFontInfo( 11 ).Family( wxFONTFAMILY_SWISS ) );

    scaleX.SetFont( font );
    BOOST_CHECK( scaleX.GetFont().IsOk() );
    BOOST_CHECK_EQUAL( scaleX.GetPlotFont().GetPointSize(), 11 );
}


BOOST_AUTO_TEST_SUITE_END()
