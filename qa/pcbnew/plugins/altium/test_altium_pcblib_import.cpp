/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.TXT for contributors.
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
 * @file test_altium_pcblib_import.cpp
 * Test suite for import of *.PcbLib libraries
 */

#include <board_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcbnew/plugins/altium/altium_designer_plugin.h>
#include <pcbnew/plugins/kicad/pcb_plugin.h>

#include <footprint.h>
#include <fp_shape.h>
#include <fp_text.h>
#include <pad.h>
#include <zone.h>


#define CHECK_ENUM_CLASS_EQUAL( L, R )                                                             \
    BOOST_CHECK_EQUAL( static_cast<int>( L ), static_cast<int>( R ) )


struct ALTIUM_PCBLIB_IMPORT_FIXTURE
{
    ALTIUM_PCBLIB_IMPORT_FIXTURE() {}

    ALTIUM_DESIGNER_PLUGIN altiumPlugin;
    PCB_PLUGIN             kicadPlugin;

    void CompareFootprints( const FOOTPRINT* fp1, const FOOTPRINT* fp2 )
    {
        BOOST_CHECK_EQUAL( fp1->GetPosition(), fp2->GetPosition() );
        BOOST_CHECK_EQUAL( fp1->GetOrientation(), fp2->GetOrientation() );

        BOOST_CHECK_EQUAL( fp1->GetReference(), fp2->GetReference() );
        BOOST_CHECK_EQUAL( fp1->GetValue(), fp2->GetValue() );
        BOOST_CHECK_EQUAL( fp1->GetDescription(), fp2->GetDescription() );
        BOOST_CHECK_EQUAL( fp1->GetKeywords(), fp2->GetKeywords() );
        BOOST_CHECK_EQUAL( fp1->GetAttributes(), fp2->GetAttributes() );
        BOOST_CHECK_EQUAL( fp1->GetFlag(), fp2->GetFlag() );
        //BOOST_CHECK_EQUAL( fp1->GetProperties(), fp2->GetProperties() );
        BOOST_CHECK_EQUAL( fp1->GetTypeName(), fp2->GetTypeName() );

        BOOST_CHECK_EQUAL( fp1->Pads().size(), fp2->Pads().size() );
        BOOST_CHECK_EQUAL( fp1->GraphicalItems().size(), fp2->GraphicalItems().size() );
        BOOST_CHECK_EQUAL( fp1->Zones().size(), fp2->Zones().size() );
        BOOST_CHECK_EQUAL( fp1->Groups().size(), fp2->Groups().size() );
        BOOST_CHECK_EQUAL( fp1->Models().size(), fp2->Models().size() );

        std::set<PAD*, FOOTPRINT::cmp_pads> fp1Pads( fp1->Pads().begin(), fp1->Pads().end() );
        std::set<PAD*, FOOTPRINT::cmp_pads> fp2Pads( fp2->Pads().begin(), fp2->Pads().end() );
        for( auto it1 = fp1Pads.begin(), it2 = fp2Pads.begin();
             it1 != fp1Pads.end() && it2 != fp2Pads.end(); it1++, it2++ )
        {
            BOOST_CHECK( PAD::Compare( *it1, *it2 ) == 0 );
        }

        std::set<BOARD_ITEM*, FOOTPRINT::cmp_drawings> fp1GraphicalItems(
                fp1->GraphicalItems().begin(), fp1->GraphicalItems().end() );
        std::set<BOARD_ITEM*, FOOTPRINT::cmp_drawings> fp2GraphicalItems(
                fp2->GraphicalItems().begin(), fp2->GraphicalItems().end() );
        for( auto it1 = fp1GraphicalItems.begin(), it2 = fp2GraphicalItems.begin();
             it1 != fp1GraphicalItems.end() && it2 != fp2GraphicalItems.end(); it1++, it2++ )
        {
            BOOST_CHECK_EQUAL( ( *it1 )->GetLayer(), ( *it2 )->GetLayer() );
            BOOST_CHECK_EQUAL( ( *it1 )->GetLayerSet(), ( *it2 )->GetLayerSet() );
            BOOST_CHECK_EQUAL( ( *it2 )->GetCenter(), ( *it2 )->GetCenter() );

            BOOST_CHECK_EQUAL( ( *it1 )->Type(), ( *it2 )->Type() );
            switch( ( *it1 )->Type() )
            {
            case PCB_FP_TEXT_T:
            {
                const FP_TEXT* text1 = static_cast<const FP_TEXT*>( *it1 );
                const FP_TEXT* text2 = static_cast<const FP_TEXT*>( *it2 );

                // TODO: text is not sorted the same way!
                /*CHECK_ENUM_CLASS_EQUAL( text1->GetType(), text2->GetType() );

                BOOST_CHECK_EQUAL( text1->GetText(), text2->GetText() );
                BOOST_CHECK_EQUAL( text1->GetPosition(), text2->GetPosition() );
                BOOST_CHECK_EQUAL( text1->GetTextAngle(), text2->GetTextAngle() );
                BOOST_CHECK_EQUAL( text1->GetTextThickness(), text2->GetTextThickness() );

                BOOST_CHECK( text1->Compare( text2 ) == 0 );*/
            }
            break;
            case PCB_FP_SHAPE_T:
            {
                const FP_SHAPE* shape1 = static_cast<const FP_SHAPE*>( *it1 );
                const FP_SHAPE* shape2 = static_cast<const FP_SHAPE*>( *it2 );

                CHECK_ENUM_CLASS_EQUAL( shape1->GetShape(), shape2->GetShape() );

                BOOST_CHECK_EQUAL( shape1->GetStroke().GetWidth(), shape2->GetStroke().GetWidth() );
                CHECK_ENUM_CLASS_EQUAL( shape1->GetStroke().GetPlotStyle(),
                                        shape2->GetStroke().GetPlotStyle() );

                BOOST_CHECK_EQUAL( shape1->GetPosition(), shape2->GetPosition() );
                BOOST_CHECK_EQUAL( shape1->GetStart(), shape2->GetStart() );
                BOOST_CHECK_EQUAL( shape1->GetEnd(), shape2->GetEnd() );

                CHECK_ENUM_CLASS_EQUAL( shape1->GetFillMode(), shape2->GetFillMode() );
            }
            break;
            /*case PCB_FP_DIM_ALIGNED_T: break;
            case PCB_FP_DIM_LEADER_T: break;
            case PCB_FP_DIM_CENTER_T: break;
            case PCB_FP_DIM_RADIAL_T: break;
            case PCB_FP_DIM_ORTHOGONAL_T: break;*/
            default: BOOST_ERROR( "KICAD_T not known" ); break;
            }
        }

        std::set<FP_ZONE*, FOOTPRINT::cmp_zones> fp1Zones( fp1->Zones().begin(),
                                                           fp1->Zones().end() );
        std::set<FP_ZONE*, FOOTPRINT::cmp_zones> fp2Zones( fp2->Zones().begin(),
                                                           fp2->Zones().end() );
        for( auto it1 = fp1Zones.begin(), it2 = fp2Zones.begin();
             it1 != fp1Zones.end() && it2 != fp2Zones.end(); it1++, it2++ )
        {
            // TODO: BOOST_CHECK( (*it1)->IsSame( **it2 ) );
        }

        // TODO: Groups
    }
};


// TODO: see https://gitlab.com/kicad/code/kicad/-/blob/master/qa/pcbnew/test_save_load.cpp

/**
 * Declares the struct as the Boost test fixture.
 */
BOOST_FIXTURE_TEST_SUITE( AltiumPcbLibImport, ALTIUM_PCBLIB_IMPORT_FIXTURE )


/**
 * Compare all footprints declared in a *.PcbLib file with their KiCad reference footprint
 */
BOOST_AUTO_TEST_CASE( AltiumPcbLibImport )
{
    std::vector<std::pair<wxString, wxString>> tests = { { "TracksTest.PcbLib",
                                                           "TracksTest.pretty" } };

    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/altium/pcblib/";

    for( const std::pair<wxString, wxString>& libName : tests )
    {
        wxString altiumLibraryPath = dataPath + libName.first;
        wxString kicadLibraryPath = dataPath + libName.second;

        wxArrayString altiumFootprintNames;
        altiumPlugin.FootprintEnumerate( altiumFootprintNames, altiumLibraryPath, true, nullptr );
        wxArrayString kicadFootprintNames;
        kicadPlugin.FootprintEnumerate( kicadFootprintNames, kicadLibraryPath, true, nullptr );
        BOOST_CHECK_EQUAL( altiumFootprintNames.GetCount(), kicadFootprintNames.GetCount() );

        for( size_t i = 0; i < altiumFootprintNames.GetCount(); i++ )
        {
            wxString footprintName = altiumFootprintNames[i];

            BOOST_TEST_CONTEXT( wxString::Format( wxT( "Import '%s' from '%s'" ), footprintName,
                                                  libName.first ) )
            {
                FOOTPRINT* altiumFp = altiumPlugin.FootprintLoad( altiumLibraryPath, footprintName,
                                                                  false, nullptr );
                BOOST_CHECK( altiumFp );

                BOOST_CHECK_EQUAL( wxT( "REF**" ), altiumFp->GetReference() );
                BOOST_CHECK_EQUAL( footprintName, altiumFp->GetValue() );

                FOOTPRINT* kicadFp = kicadPlugin.FootprintLoad( kicadLibraryPath, footprintName,
                                                                false, nullptr );
                BOOST_CHECK( kicadFp );

                CompareFootprints( altiumFp, kicadFp );
            }
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
