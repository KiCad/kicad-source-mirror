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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <boost/test/data/test_case.hpp>
#include <qa_utils/file_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <algorithm>
#include <map>
#include <fstream>

#include <bus_alias.h>
#include <locale_io.h>
#include <project.h>
#include <project/project_file.h>
#include <schematic.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <settings/settings_manager.h>


BOOST_DATA_TEST_CASE( DeletedProjectAliasesStayDeletedWhenLegacySheetsReopen,
                      boost::unit_test::data::make( { false, true } ), oldEmptyTable )
{
    LOCALE_IO locale;
    KI_TEST::SCOPED_TEMP_DIR directory( "connectivity-alias-reopen" );
    const wxString source = KI_TEST::GetEeschemaTestDataDir() + "netlists/hierarchy_aliases/";

    for( const wxString& filename : { wxString( "hierarchy_aliases.kicad_pro" ),
                                     wxString( "hierarchy_aliases.kicad_sch" ),
                                     wxString( "sub1.kicad_sch" ), wxString( "sub2.kicad_sch" ) } )
    {
        BOOST_REQUIRE( wxCopyFile( source + filename, directory.PathStr() + "/" + filename ) );
    }

    const wxString projectFile = directory.PathStr() + "/hierarchy_aliases.kicad_pro";
    const wxString schematicFile = directory.PathStr() + "/hierarchy_aliases.kicad_sch";

    if( oldEmptyTable )
    {
        // Older writers emitted an empty table even without opening the schematic
        std::ifstream input( projectFile.ToStdString() );
        nlohmann::json document = nlohmann::json::parse( input );
        input.close();
        document["meta"]["version"] = 3;
        document["schematic"]["bus_aliases"] = nlohmann::json::object();
        std::ofstream output( projectFile.ToStdString() );
        output << document.dump( 2 );
        output.close();
        BOOST_REQUIRE( output.good() );
    }

    std::vector<wxString> retainedMembers;
    const auto load = [&]( PROJECT& project )
    {
        auto schematic = std::make_unique<SCHEMATIC>( &project );
        schematic->Reset();
        SCH_SHEET* empty = schematic->GetTopLevelSheet( 0 );
        SCH_IO_KICAD_SEXPR io;
        SCH_SHEET* root = io.LoadSchematicFile( schematicFile, schematic.get() );
        schematic->AddTopLevelSheet( root );
        schematic->RemoveTopLevelSheet( empty );
        delete empty;
        BOOST_REQUIRE_MESSAGE( io.GetError().IsEmpty(), io.GetError() );
        return schematic;
    };

    {
        SETTINGS_MANAGER settings;
        BOOST_REQUIRE( settings.LoadProject( projectFile ) );
        BOOST_REQUIRE( settings.Prj().GetProjectFile().SaveToFile( directory.PathStr(), true ) );
    }

    {
        SETTINGS_MANAGER settings;
        BOOST_REQUIRE( settings.LoadProject( projectFile ) );
        // Project migration can write defaults before legacy sheet aliases are imported
        BOOST_REQUIRE( settings.Prj().GetProjectFile().SaveToFile( directory.PathStr(), true ) );
        auto schematic = load( settings.Prj() );
        BOOST_REQUIRE( schematic->GetBusAlias( "ALIAS1" ) );
        BOOST_REQUIRE( schematic->GetBusAlias( "ALIAS2" ) );
        schematic->SetBusAliases( {} );
        BOOST_REQUIRE( settings.Prj().GetProjectFile().SaveToFile( directory.PathStr() ) );
    }

    {
        SETTINGS_MANAGER settings;
        BOOST_REQUIRE( settings.LoadProject( projectFile ) );
        BOOST_REQUIRE( settings.Prj().GetProjectFile().m_BusAliases.empty() );
        auto schematic = load( settings.Prj() );
        BOOST_CHECK( schematic->GetAllBusAliases().empty() );
        BOOST_CHECK( settings.Prj().GetProjectFile().m_BusAliases.empty() );

        auto sheets = schematic->RootScreen()->Items().OfType( SCH_SHEET_T );
        BOOST_REQUIRE( sheets.begin() != sheets.end() );
        std::unique_ptr<SCH_SHEET> appended(
                static_cast<SCH_SHEET*>( ( *sheets.begin() )->Duplicate( false ) ) );
        appended->SetScreen( nullptr );
        appended->SetFileName( source + "sub1.kicad_sch" );
        SCH_IO_KICAD_SEXPR io;
        BOOST_REQUIRE( io.LoadSchematicFile( appended->GetFileName(), schematic.get(), appended.get() )
                       == appended.get() );
        BOOST_REQUIRE_MESSAGE( io.GetError().IsEmpty(), io.GetError() );
        schematic->AddTopLevelSheet( appended.release() );
        auto imported = schematic->GetBusAlias( "ALIAS2" );
        BOOST_REQUIRE( imported );
        BOOST_CHECK( !schematic->GetBusAlias( "ALIAS1" ) );
        auto changed = imported->Clone();
        retainedMembers = imported->Members();
        std::reverse( retainedMembers.begin(), retainedMembers.end() );
        BOOST_REQUIRE( retainedMembers != imported->Members() );
        changed->SetMembers( retainedMembers );
        schematic->SetBusAliases( { changed } );
        BOOST_REQUIRE( settings.Prj().GetProjectFile().SaveToFile( directory.PathStr(), true ) );
    }

    {
        SETTINGS_MANAGER settings;
        BOOST_REQUIRE( settings.LoadProject( projectFile ) );
        auto schematic = load( settings.Prj() );
        BOOST_REQUIRE( schematic->GetBusAlias( "ALIAS2" ) );
        BOOST_CHECK( schematic->GetBusAlias( "ALIAS2" )->Members() == retainedMembers );
        BOOST_CHECK( !schematic->GetBusAlias( "ALIAS1" ) );

        SCH_IO_KICAD_SEXPR io;
        const std::map<std::string, UTF8> properties{ { "hierarchical_sheet_load", "" } };
        std::unique_ptr<SCH_SHEET> imported( io.LoadSchematicFile(
                source + "hierarchy_aliases.kicad_sch", schematic.get(), nullptr, &properties ) );
        BOOST_REQUIRE( imported );
        BOOST_REQUIRE_MESSAGE( io.GetError().IsEmpty(), io.GetError() );
        schematic->AddTopLevelSheet( imported.release() );
        BOOST_REQUIRE( schematic->GetBusAlias( "ALIAS1" ) );
        BOOST_REQUIRE( schematic->GetBusAlias( "ALIAS2" ) );
        BOOST_CHECK( schematic->GetBusAlias( "ALIAS2" )->Members() != retainedMembers );
    }
}
