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

#include <boost/test/unit_test.hpp>

#include <wx/filefn.h>
#include <wx/filename.h>

#include <gerber_file_image.h>
#include <gerber_draw_item.h>
#include <qa_utils/wx_utils/unit_test_utils.h>


static std::string GetGerbviewTestDataDir()
{
    std::string dir = KI_TEST::GetTestDataRootDir();
    dir += "gerbview/";
    return dir;
}


BOOST_AUTO_TEST_SUITE( StepAndRepeat )


/**
 * Verifies that an SR block replicates all items as a unit rather than
 * replicating each item individually.
 *
 * The test file has %SRX3Y1I10.0J5.0*% with two region items (one LPD, one LPC)
 * followed by %SR*%. With block-level replication, each copy of the block must
 * contain items in the same order (positive then negative). Individual item
 * replication would interleave copies incorrectly.
 */
BOOST_AUTO_TEST_CASE( BlockReplication )
{
    GERBER_FILE_IMAGE image( 0 );

    std::string fn = GetGerbviewTestDataDir() + "step_and_repeat_block.gbr";
    bool loaded = image.LoadGerberFile( wxString::FromUTF8( fn ) );
    BOOST_REQUIRE_MESSAGE( loaded, "Failed to load test gerber file" );

    GERBER_DRAW_ITEMS& items = image.GetItems();

    // The file has 2 region items in the SR block, X repeat = 3, Y repeat = 1.
    // Total items = 2 original + 2*2 copies = 6
    BOOST_CHECK_EQUAL( items.size(), 6 );

    if( items.size() != 6 )
        return;

    // Original block items (indices 0,1) at X offset 0
    // Copy 1 items (indices 2,3) at X offset 10mm
    // Copy 2 items (indices 4,5) at X offset 20mm
    //
    // Within each pair, first item must be positive (LPD), second negative (LPC).
    // This ordering is what distinguishes correct block-level replication from
    // the old per-item replication which would produce:
    //   [pos0, pos1, pos2, neg0, neg1, neg2]

    // Verify the block ordering: items alternate as [pos, neg, pos, neg, pos, neg]
    for( int copy = 0; copy < 3; copy++ )
    {
        int posIdx = copy * 2;
        int negIdx = copy * 2 + 1;

        BOOST_CHECK_MESSAGE( !items[posIdx]->GetLayerPolarity(),
                             "Item " << posIdx << " (copy " << copy << " positive) "
                                     << "should have positive polarity" );

        BOOST_CHECK_MESSAGE( items[negIdx]->GetLayerPolarity(),
                             "Item " << negIdx << " (copy " << copy << " negative) "
                                     << "should have negative polarity" );
    }

    // Verify spatial offsets: each copy should be shifted by 10mm in X.
    // Gerbview IU is 10nm, so 10mm = 10 * 1e5 = 1000000 IU.
    int stepX = 1000000;

    for( int copy = 1; copy < 3; copy++ )
    {
        VECTOR2I origPos = items[0]->GetPosition();
        VECTOR2I copyPos = items[copy * 2]->GetPosition();
        int expectedDx = copy * stepX;

        BOOST_CHECK_EQUAL( copyPos.x - origPos.x, expectedDx );
        BOOST_CHECK_EQUAL( copyPos.y - origPos.y, 0 );
    }
}


/**
 * Verifies that files without SR blocks still load correctly (no regression).
 */
BOOST_AUTO_TEST_CASE( NoStepAndRepeat )
{
    wxString path = wxFileName::CreateTempFileName( wxS( "kicad_gbr_no_sr" ) );
    BOOST_REQUIRE( !path.IsEmpty() );

    FILE* file = wxFopen( path, wxT( "wt" ) );
    BOOST_REQUIRE( file );

    fputs( "%MOMM*%\n%FSLAX26Y26*%\n%ADD10C,1.00000*%\nD10*\nX0Y0D03*\nM02*\n", file );
    fclose( file );

    GERBER_FILE_IMAGE image( 0 );
    BOOST_REQUIRE( image.LoadGerberFile( path ) );

    BOOST_CHECK_EQUAL( image.GetItems().size(), 1 );
    BOOST_CHECK_EQUAL( image.GetLayerParams().m_XRepeatCount, 1 );
    BOOST_CHECK_EQUAL( image.GetLayerParams().m_YRepeatCount, 1 );
    BOOST_CHECK_EQUAL( image.m_SRBlockCollecting, false );

    wxRemoveFile( path );
}


BOOST_AUTO_TEST_SUITE_END()
