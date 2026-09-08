/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>
#include <netlist_exporters/netlist_exporter_kicad.h>
#include <netlist_exporters/netlist_exporter_spice.h>
#include <richio.h>
#include <reporter.h>
#include <schematic.h>
#include <sch_screen.h>
#include <sch_symbol.h>
#include <sch_rule_area.h>
#include <wx/xml/xml.h>
#include <wx/filename.h>
#include <settings/settings_manager.h>
#include <algorithm>

BOOST_AUTO_TEST_SUITE( ConnectivityExport )

BOOST_AUTO_TEST_CASE( ExportRefreshesSymbolRuleAreaMembership )
{
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "netlists/multinetclasses/multinetclasses", schematic );
    const auto sheet = schematic->Hierarchy().front();
    SCH_SCREEN& screen = *sheet.LastScreen();
    SCH_SYMBOL* symbol = nullptr;

    for( SCH_ITEM* item : screen.Items().OfType( SCH_SYMBOL_T ) )
    {
        auto* candidate = static_cast<SCH_SYMBOL*>( item );

        if( !candidate->IsPower() && !candidate->GetRef( &sheet ).StartsWith( "#" ) )
        {
            symbol = candidate;
            break;
        }
    }

    BOOST_REQUIRE( symbol );
    auto areas = screen.Items().OfType( SCH_RULE_AREA_T );
    BOOST_REQUIRE( areas.begin() != areas.end() );
    auto* area = static_cast<SCH_RULE_AREA*>( *areas.begin() );
    screen.Remove( symbol, false );
    symbol->SetPosition( area->GetBoundingBox().Centre() );
    screen.Append( symbol );
    std::unordered_set<SCH_SCREEN*> screens{ &screen };
    SCH_RULE_AREA::UpdateRuleAreasInScreens( screens, nullptr );
    BOOST_REQUIRE( symbol->GetRuleAreaCache().contains( area ) );
    const VECTOR2I originalPosition = symbol->GetPosition();
    area->SetDNP( true );
    symbol->SetDNP( false, &sheet );
    symbol->SetPosition( VECTOR2I( -100000000, -100000000 ) );
    BOOST_REQUIRE( symbol->ResolveDNP( &sheet ) );
    const wxString reference = symbol->GetRef( &sheet );
    const wxString path = wxFileName::CreateTempFileName(
            wxFileName::GetTempDir() + "/connectivity-export-areas-" );
    NETLIST_EXPORTER_XML exporter( schematic.get() );
    auto exportDnp = [&]()
    {
        WX_STRING_REPORTER reporter;
        BOOST_REQUIRE_MESSAGE( exporter.WriteNetlist( path, 0, reporter ), reporter.GetMessages() );
        wxXmlDocument document;
        BOOST_REQUIRE( document.Load( path ) );
        bool found = false;
        bool dnp = false;

        for( wxXmlNode* section = document.GetRoot()->GetChildren(); section; section = section->GetNext() )
        {
            if( section->GetName() != "components" )
                continue;

            for( wxXmlNode* component = section->GetChildren(); component; component = component->GetNext() )
            {
                if( component->GetAttribute( "ref" ) != reference )
                    continue;

                found = true;

                for( wxXmlNode* property = component->GetChildren(); property; property = property->GetNext() )
                    dnp |= property->GetName() == "property" && property->GetAttribute( "name" ) == "dnp";
            }
        }

        BOOST_REQUIRE( found );
        return dnp;
    };

    BOOST_CHECK( !exportDnp() );
    BOOST_CHECK( !symbol->ResolveDNP( &sheet ) );
    symbol->SetPosition( originalPosition );
    BOOST_CHECK( exportDnp() );
    BOOST_CHECK( symbol->ResolveDNP( &sheet ) );
}

BOOST_AUTO_TEST_SUITE_END()
