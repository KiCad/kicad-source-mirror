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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <qa_utils/file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <diff_merge/sym_digest.h>
#include <erc/erc.h>
#include <erc/erc_item.h>
#include <libraries/library_manager.h>
#include <libraries/library_table.h>
#include <lib_symbol.h>
#include <mdw/client/mdw_library_sources.h>
#include <pgm_base.h>
#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>
#include <sch_marker.h>
#include <sch_screen.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <widgets/mdw_member_index.h>

#include <wx/ffile.h>

#include <filesystem>
#include <map>
#include <set>


BOOST_AUTO_TEST_SUITE( ERCLibPartNotReleased )


namespace
{

// The release digest of a symbol in the acme-library fixture
wxString libraryDigest( const wxString& aName )
{
    std::filesystem::path root = KI_TEST::GetTestDataRootDir();
    wxString path = wxString::FromUTF8( ( root / "common" / "git" / "acme-library" / "passives.kicad_sym" ).string() );

    IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
    LIB_SYMBOL*         symbol = plugin->LoadSymbol( path, aName );

    BOOST_REQUIRE( symbol );
    return KICAD_DIFF::SymbolDigest( *symbol );
}


MDW_MEMBER_ENTRY entry( const wxString& aName, const std::vector<wxString>& aDigests )
{
    MDW_MEMBER_ENTRY entry;
    entry.member.name = aName;

    for( const wxString& digest : aDigests )
    {
        MDW_MEMBER_RELEASE release;
        release.label = wxString::Format( wxS( "%d" ), static_cast<int>( entry.member.releases.size() ) + 1 );
        release.content_digest = digest;
        release.state.category = MDW_LIFECYCLE_CATEGORY::RELEASED;
        entry.member.releases.push_back( release );
    }

    return entry;
}


// The messages of every ERCE_LIB_PART_NOT_RELEASED marker, by the symbol name each quotes
std::map<wxString, std::vector<wxString>> notReleasedMarkers( SCHEMATIC& aSchematic )
{
    std::map<wxString, std::vector<wxString>> markers;
    SCH_SCREENS                               screens( aSchematic.Root() );

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
    {
        for( SCH_ITEM* item : screen->Items().OfType( SCH_MARKER_T ) )
        {
            const std::shared_ptr<RC_ITEM> rcItem = static_cast<SCH_MARKER*>( item )->GetRCItem();

            if( rcItem->GetErrorCode() != ERCE_LIB_PART_NOT_RELEASED )
                continue;

            wxString message = rcItem->GetErrorMessage( false );
            markers[message.AfterFirst( '\'' ).BeforeFirst( '\'' )].push_back( message );
        }
    }

    return markers;
}


// The distinct references ctrl-board gives the symbols placed from aLibId
std::set<wxString> referencesOf( SCHEMATIC& aSchematic, const wxString& aLibId )
{
    std::set<wxString> references;

    for( const SCH_SHEET_PATH& path : aSchematic.Hierarchy() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetLibId().Format().wx_str() == aLibId )
                references.insert( symbol->GetRef( &path ) );
        }
    }

    return references;
}

} // namespace


// ctrl-board places acme-passives and acme-ics parts, which a team lists, and acme-power and acme-connectors, which
// no team does
BOOST_AUTO_TEST_CASE( UnreleasedTeamPartsWarnAndOthersStaySilent )
{
    SETTINGS_MANAGER           manager;
    std::unique_ptr<SCHEMATIC> schematic;

    KI_TEST::LoadSchematic( manager, "../common/git/mdw_ctrl_board/ctrl-board", schematic );

    BOOST_CHECK( schematic->ErcSettings().m_ERCSeverities[ERCE_LIB_PART_NOT_RELEASED] == RPT_SEVERITY_WARNING );

    // C is placed from Rev 1 and a later Rev 2 exists. R was never released, CP changed since its only release,
    // and the rest of the library was never checked in. acme-ics has not been listed yet
    const wxString dC = libraryDigest( wxS( "C" ) );
    const wxString dL = libraryDigest( wxS( "L" ) );

    std::map<wxString, MDW_MEMBER_ENTRY> passives;
    passives.emplace( wxS( "C" ), entry( wxS( "C" ), { dC, dL } ) );
    passives.emplace( wxS( "R" ), entry( wxS( "R" ), {} ) );
    passives.emplace( wxS( "CP" ), entry( wxS( "CP" ), { dL } ) );

    ERC_TESTER tester( schematic.get() );
    tester.SetLibraryListing(
            [&]( const wxString& aNickname ) -> MDW_LIBRARY_LISTING
            {
                if( aNickname == wxS( "acme-passives" ) )
                    return { true, &passives };

                if( aNickname == wxS( "acme-ics" ) )
                    return { true, nullptr };

                return {};
            } );

    tester.RunTests( nullptr, nullptr, nullptr, &schematic->Project(), nullptr );

    std::map<wxString, int> flagged;
    std::set<wxString>      unchecked;
    SCH_SCREENS             screens( schematic->Root() );

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
    {
        for( SCH_ITEM* item : screen->Items().OfType( SCH_MARKER_T ) )
        {
            const std::shared_ptr<RC_ITEM> rcItem = static_cast<SCH_MARKER*>( item )->GetRCItem();

            if( rcItem->GetErrorCode() != ERCE_LIB_PART_NOT_RELEASED )
                continue;

            wxString message = rcItem->GetErrorMessage( false );

            if( message.StartsWith( wxS( "Symbol '" ) ) )
                ++flagged[message.AfterFirst( '\'' ).BeforeFirst( '\'' )];
            else
                unchecked.insert( message.AfterFirst( '\'' ).BeforeFirst( '\'' ) );
        }
    }

    // What each placed acme-passives symbol should draw, from the parts the hierarchy places
    std::map<wxString, int> expected;

    for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            const LIB_ID& id = static_cast<SCH_SYMBOL*>( item )->GetLibId();

            if( id.GetLibNickname() == "acme-passives" && id.GetLibItemName() != "C" )
                ++expected[id.GetUniStringLibItemName()];
        }
    }

    BOOST_REQUIRE( expected.count( wxS( "R" ) ) && expected.count( wxS( "CP" ) ) && expected.size() > 3 );
    BOOST_CHECK( flagged == expected );
    BOOST_CHECK( unchecked == std::set<wxString>{ wxS( "acme-ics" ) } );
}


// kicad-cli lists no team library, so ctrl-board's acme-passives parts must read as unchecked rather than pass
BOOST_AUTO_TEST_CASE( UnintegratedTeamLibraryIsReportedUnchecked )
{
    SETTINGS_MANAGER           manager;
    std::unique_ptr<SCHEMATIC> schematic;

    KI_TEST::LoadSchematic( manager, "../common/git/mdw_ctrl_board/ctrl-board", schematic );

    KI_TEST::SCOPED_TEMP_DIR  temp( wxS( "erc-unintegrated" ) );
    MDW_LIBRARY_SOURCES       sources( temp.ChildPathStr( wxS( "sources.json" ) ),
                                       temp.ChildPathStr( wxS( "site.json" ) ),
                                       temp.CreateChildDirStr( wxS( "copies" ) ) );
    MDW_LIBRARY_SOURCE_CONFIG config;
    MDW_LIBRARY_COPY          copy;

    config.backend_id = wxS( "org.kicad.mdw.git" );
    config.locator = wxS( "https://git.example.com/acme-library.git" );
    config.name = wxS( "acme-library" );
    config.prefix = wxS( "acme-" );
    BOOST_REQUIRE( sources.Add( config ).IsOk() );

    copy.source_id = config.id;
    copy.backend_id = config.backend_id;
    copy.locator = config.locator;
    copy.local_root = sources.LocalRoot( config );
    copy.libraries.push_back( { config.id + wxS( ":passives" ), wxS( "acme-passives" ), MDW_LIBRARY_TYPE::SYMBOL,
                                wxS( "passives.kicad_sym" ) } );
    BOOST_REQUIRE( sources.WriteCopy( copy ).IsOk() );

    struct TABLE_RESTORE
    {
        LIBRARY_MANAGER& manager;
        wxString         directory;
        ~TABLE_RESTORE() { manager.LoadProjectTables( directory, { LIBRARY_TABLE_TYPE::SYMBOL } ); }
    };

    LIBRARY_MANAGER&     libraries = Pgm().GetLibraryManager();
    const LIBRARY_TABLE* before =
            libraries.Table( LIBRARY_TABLE_TYPE::SYMBOL, LIBRARY_TABLE_SCOPE::PROJECT ).value_or( nullptr );
    TABLE_RESTORE        restore{ libraries, before ? wxFileName( before->Path() ).GetPath() : wxString() };

    wxFFile( temp.ChildPathStr( wxS( "sym-lib-table" ) ), wxS( "w" ) ).Write( wxS( "(sym_lib_table (version 7))\n" ) );
    libraries.LoadProjectTables( wxString::FromUTF8( temp.Path().string() ), { LIBRARY_TABLE_TYPE::SYMBOL } );

    LIBRARY_TABLE* table =
            libraries.Table( LIBRARY_TABLE_TYPE::SYMBOL, LIBRARY_TABLE_SCOPE::PROJECT ).value_or( nullptr );
    BOOST_REQUIRE( table );

    // The row a sync writes for the copy's passives library
    LIBRARY_TABLE_ROW& row = table->InsertRow();
    row.SetNickname( wxS( "acme-passives" ) );
    row.SetURI( copy.local_root + wxFILE_SEP_PATH + wxS( "passives.kicad_sym" ) );
    row.SetType( wxS( "KiCad" ) );
    row.SetOptions( wxS( "mdw=" ) + config.id );
    row.SetScope( LIBRARY_TABLE_SCOPE::PROJECT );

    ERC_TESTER tester( schematic.get() );
    tester.SetLibraryListing( ERC_TESTER::UnintegratedListing( sources ) );
    tester.RunTests( nullptr, nullptr, nullptr, &schematic->Project(), nullptr );

    std::set<wxString> unchecked;
    SCH_SCREENS        screens( schematic->Root() );

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
    {
        for( SCH_ITEM* item : screen->Items().OfType( SCH_MARKER_T ) )
        {
            const std::shared_ptr<RC_ITEM> rcItem = static_cast<SCH_MARKER*>( item )->GetRCItem();

            if( rcItem->GetErrorCode() == ERCE_LIB_PART_NOT_RELEASED )
                unchecked.insert( rcItem->GetErrorMessage( false ).AfterFirst( '\'' ).BeforeFirst( '\'' ) );
        }
    }

    BOOST_CHECK( unchecked == std::set<wxString>{ wxS( "acme-passives" ) } );
}


// ctrl-board places C from its only release, which the team has since marked Not for New Designs
BOOST_AUTO_TEST_CASE( NotForNewDesignsPartWarns )
{
    SETTINGS_MANAGER           manager;
    std::unique_ptr<SCHEMATIC> schematic;

    KI_TEST::LoadSchematic( manager, "../common/git/mdw_ctrl_board/ctrl-board", schematic );

    MDW_MEMBER_ENTRY c = entry( wxS( "C" ), { libraryDigest( wxS( "C" ) ) } );
    c.member.releases.back().state.not_for_new_designs = true;

    std::map<wxString, MDW_MEMBER_ENTRY> passives;
    passives.emplace( wxS( "C" ), c );

    ERC_TESTER tester( schematic.get() );
    tester.SetLibraryListing(
            [&]( const wxString& aNickname ) -> MDW_LIBRARY_LISTING
            {
                if( aNickname == wxS( "acme-passives" ) )
                    return { true, &passives };

                return {};
            } );

    tester.RunTests( nullptr, nullptr, nullptr, &schematic->Project(), nullptr );

    const std::vector<wxString> warnings = notReleasedMarkers( *schematic )[wxS( "C" )];
    const std::set<wxString>    references = referencesOf( *schematic, wxS( "acme-passives:C" ) );

    BOOST_REQUIRE( !references.empty() );
    BOOST_CHECK_EQUAL( warnings.size(), references.size() );

    for( const wxString& warning : warnings )
        BOOST_CHECK_MESSAGE( warning.Contains( wxS( "revision 1, which is not for new designs" ) ), warning );
}


// U2 is a 74LS125 whose four units ctrl-board places separately, and the part was never checked in
BOOST_AUTO_TEST_CASE( MultiUnitSymbolWarnsOncePerReference )
{
    SETTINGS_MANAGER           manager;
    std::unique_ptr<SCHEMATIC> schematic;

    KI_TEST::LoadSchematic( manager, "../common/git/mdw_ctrl_board/ctrl-board", schematic );

    const std::map<wxString, MDW_MEMBER_ENTRY> ics;

    ERC_TESTER tester( schematic.get() );
    tester.SetLibraryListing(
            [&]( const wxString& aNickname ) -> MDW_LIBRARY_LISTING
            {
                if( aNickname == wxS( "acme-ics" ) )
                    return { true, &ics };

                return {};
            } );

    tester.RunTests( nullptr, nullptr, nullptr, &schematic->Project(), nullptr );

    BOOST_CHECK( referencesOf( *schematic, wxS( "acme-ics:74LS125" ) ) == std::set<wxString>{ wxS( "U2" ) } );
    BOOST_CHECK_EQUAL( notReleasedMarkers( *schematic )[wxS( "74LS125" )].size(), 1 );
}


BOOST_AUTO_TEST_SUITE_END()
