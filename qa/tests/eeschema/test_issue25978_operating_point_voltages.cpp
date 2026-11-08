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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <advanced_config.h>
#include <connection_graph.h>
#include <connectivity/conn_facade.h>
#include <eeschema_helpers.h>
#include <netlist_exporters/netlist_exporter_spice.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_painter.h>
#include <sch_screen.h>
#include <schematic.h>
#include <scoped_set_reset.h>
#include <sim/operating_point.h>
#include <view/view.h>

#include <algorithm>
#include <limits>


BOOST_AUTO_TEST_SUITE( OperatingPointVoltages )

BOOST_AUTO_TEST_CASE( BothEnginesAssignVoltageToLongestWire )
{
    auto&            enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );

    for( bool published : { false, true } )
    {
        BOOST_TEST_CONTEXT( "published=" << published )
        {
            enabled = published;
            wxString path =
                    wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() ) + wxS( "issue25198/SVG-Test.kicad_sch" );
            std::unique_ptr<SCHEMATIC> schematic( EESCHEMA_HELPERS::LoadSchematic( path, true, false ) );
            BOOST_REQUIRE( schematic );
            SCH_SCREEN* screen = schematic->RootScreen();
            auto*       longest = new SCH_LINE( VECTOR2I( 100000, 100000 ), LAYER_WIRE );
            longest->SetEndPoint( { 500000, 100000 } );
            screen->Append( longest );
            auto* shorter = new SCH_LINE( longest->GetEndPoint(), LAYER_WIRE );
            shorter->SetEndPoint( { 550000, 100000 } );
            screen->Append( shorter );
            screen->Append( new SCH_LABEL( longest->GetStartPoint(), wxS( "OP_TEST" ) ) );
            schematic->RebuildConnectivity();

            const auto name = longest->GetConnectionName( &schematic->CurrentSheet() );
            BOOST_REQUIRE( name );
            wxString spiceName = *name;
            NETLIST_EXPORTER_SPICE::ConvertToSpiceMarkup( &spiceName );
            spiceName.MakeLower();
            schematic->SetOperatingPoint( spiceName, 12.345 );
            const auto&    settings = schematic->Settings();
            const wxString expected =
                    schematic->GetOperatingPoint( *name, settings.m_OPO_VPrecision, settings.m_OPO_VRange );
            BOOST_REQUIRE( expected != wxS( "?" ) && expected != wxS( "--" ) && !expected.IsEmpty() );

            if( published )
            {
                BOOST_REQUIRE( !schematic->Connectivity().GetNetMap().empty() );
                BOOST_CHECK( schematic->ConnectionGraph()->GetNetMap().empty() );
            }

            std::vector<SCH_LINE*> changed;
            UpdateOperatingPointVoltages( *schematic,
                                          [&]( SCH_LINE* aWire )
                                          {
                                              changed.push_back( aWire );
                                          } );
            BOOST_CHECK_EQUAL( longest->GetOperatingPoint(), expected );
            BOOST_CHECK( shorter->GetOperatingPoint().IsEmpty() );
            BOOST_REQUIRE_EQUAL( changed.size(), 1u );
            BOOST_CHECK( changed[0] == longest );

            // The voltage layer is present; data assignment must not depend on its visibility.
            const auto layers = longest->ViewGetLayers();
            BOOST_CHECK( std::find( layers.begin(), layers.end(), LAYER_OP_VOLTAGES ) != layers.end() );
            KIGFX::SCH_PAINTER painter( nullptr );
            KIGFX::VIEW        view;
            view.SetPainter( &painter );
            const double lod = longest->ViewGetLOD( LAYER_OP_VOLTAGES, &view );
            BOOST_CHECK( lod != std::numeric_limits<double>::max() );
            BOOST_CHECK_LT( lod, 1.0 );

            longest->SetOperatingPoint( wxEmptyString );
            changed.clear();
            schematic->SetOperatingPoint( spiceName, 0.0 );
            UpdateOperatingPointVoltages( *schematic,
                                          [&]( SCH_LINE* aWire )
                                          {
                                              changed.push_back( aWire );
                                          } );
            BOOST_CHECK( !longest->GetOperatingPoint().IsEmpty() );
            BOOST_CHECK_EQUAL( changed.size(), 1u );

            longest->SetOperatingPoint( wxEmptyString );
            changed.clear();
            schematic->ClearOperatingPoints();
            UpdateOperatingPointVoltages( *schematic,
                                          [&]( SCH_LINE* aWire )
                                          {
                                              changed.push_back( aWire );
                                          } );
            BOOST_CHECK( longest->GetOperatingPoint().IsEmpty() );
            BOOST_CHECK( changed.empty() );
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
