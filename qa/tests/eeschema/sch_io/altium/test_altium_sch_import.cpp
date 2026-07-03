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

#include <boost/test/unit_test.hpp>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <lib_id.h>
#include <schematic.h>
#include <sch_io/altium/sch_io_altium.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_symbol.h>
#include <settings/settings_manager.h>

#include <set>
#include <vector>


namespace
{

struct ALTIUM_SCH_IMPORT_FIXTURE
{
    ALTIUM_SCH_IMPORT_FIXTURE() : m_schematic( nullptr )
    {
        m_settingsManager.LoadProject( "" );
        m_schematic.SetProject( &m_settingsManager.Prj() );
        m_schematic.Reset();
    }

    ~ALTIUM_SCH_IMPORT_FIXTURE() { m_schematic.Reset(); }

    wxString dataFile( const wxString& aName ) const
    {
        return wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir()
                                   + "/plugins/altium/issue22943/" )
               + aName;
    }

    SETTINGS_MANAGER m_settingsManager;
    SCHEMATIC        m_schematic;
};

} // namespace


BOOST_FIXTURE_TEST_SUITE( AltiumSchImport, ALTIUM_SCH_IMPORT_FIXTURE )


// https://gitlab.com/kicad/code/kicad/-/issues/22943
// A component placed from an external Altium library must be addressed by a well-formed library
// id (source library nickname + real library reference) so it resolves against the library that
// project import registers, instead of the importer's internal per-placement name.
BOOST_AUTO_TEST_CASE( Issue22943_SourceLibrarySymbolLibId )
{
    SCH_IO_ALTIUM plugin;

    SCH_SHEET* rootSheet = plugin.LoadSchematicFile( dataFile( "1_cover.SchDoc" ), &m_schematic );
    BOOST_REQUIRE( rootSheet );
    BOOST_REQUIRE( rootSheet->GetScreen() );

    // The real reference names of every symbol contained in the source library.
    wxArrayString libNames;
    plugin.EnumerateSymbolLib( libNames, dataFile( "mounting_holes.SchLib" ) );

    std::set<wxString> libSymbolNames( libNames.begin(), libNames.end() );
    BOOST_REQUIRE( libSymbolNames.count( wxT( "MH M3" ) ) );

    std::vector<SCH_SYMBOL*> fromMountingHoles;

    for( SCH_ITEM* item : rootSheet->GetScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );

        if( sym->GetLibId().GetUniStringLibNickname() == wxT( "mounting_holes" ) )
            fromMountingHoles.push_back( sym );
    }

    BOOST_REQUIRE( !fromMountingHoles.empty() );

    // Every symbol drawn from the source library must name an item that actually exists there.
    for( SCH_SYMBOL* sym : fromMountingHoles )
    {
        const LIB_ID& libId = sym->GetLibId();
        BOOST_CHECK( libId.IsValid() );
        BOOST_CHECK_MESSAGE( libSymbolNames.count( libId.GetUniStringLibItemName() ),
                             "Library id '" << libId.Format().wx_str()
                                            << "' does not resolve in mounting_holes.SchLib" );
    }
}


BOOST_AUTO_TEST_SUITE_END()
