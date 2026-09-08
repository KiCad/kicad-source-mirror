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
#include <import_net_map.h>
#include <json_common.h>
#include <reporter.h>
#include <schematic.h>
#include <eeschema_helpers.h>
#include <connection_graph.h>
#include <sch_connection.h>
#include <sch_label.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <richio.h>
#include <wx/file.h>
#include <wx/filename.h>

BOOST_AUTO_TEST_SUITE( ImportNetMap )

BOOST_AUTO_TEST_CASE( RoundTripAndSaveAsPreserveOccurrenceIdentity )
{
    wxString root = wxFileName::CreateTempFileName( wxS( "import-net-map" ) );
    wxString copy = root + wxS( "-copy.kicad_sch" );
    KIID uuid;
    IMPORT_NET_MAP map;
    map.sourceDesignDigest = wxS( "source-digest" );
    IMPORT_NET_MAP_ENTRY entry;
    entry.view = wxS( "view/with/slashes" );
    entry.occurrence = { wxS( "A/B" ), wxS( "C" ) };
    entry.sourceNetId = 42;
    entry.originalName = wxS( "N123" );
    entry.nameAtImport = wxS( "Net-(R1-Pad1)" );
    entry.status = wxS( "mapped" );
    IMPORT_NET_TERMINAL terminal;
    terminal.unit = 2;
    terminal.sourcePinId = 7;
    terminal.pinNumber = wxS( "1" );
    terminal.duplicateIndex = 1;
    entry.terminals.push_back( terminal );
    map.entries.push_back( entry );
    entry.occurrence = { wxS( "A" ), wxS( "B/C" ) };
    map.entries.push_back( entry );
    WX_STRING_REPORTER reporter;
    BOOST_REQUIRE( WriteImportNetMap( map, uuid, root, reporter ) );
    auto loaded = ReadImportNetMap( uuid, root, reporter );
    BOOST_REQUIRE( loaded );
    BOOST_REQUIRE_EQUAL( loaded->entries.size(), 2 );
    BOOST_CHECK_EQUAL( loaded->entries[0].occurrence[0], wxString( "A/B" ) );
    BOOST_CHECK_EQUAL( loaded->entries[1].occurrence[0], wxString( "A" ) );
    BOOST_CHECK_EQUAL( loaded->entries[0].terminals[0].duplicateIndex, 1 );
    BOOST_CHECK( loaded->entries[0].terminals[0].symbolUuid == terminal.symbolUuid );
    BOOST_CHECK_EQUAL( loaded->entries[0].nameAtImport, entry.nameAtImport );
    BOOST_CHECK( WriteImportNetMap( *loaded, uuid, copy, reporter ) );
    BOOST_CHECK( wxFileExists( ImportNetMapPath( root ) ) );
    BOOST_CHECK( wxFileExists( ImportNetMapPath( copy ) ) );
    wxRemoveFile( ImportNetMapPath( root ) );
    wxRemoveFile( ImportNetMapPath( copy ) );
    wxRemoveFile( root );
}

BOOST_AUTO_TEST_CASE( IncompatibleCompanionIsPreserved )
{
    wxString root = wxFileName::CreateTempFileName( wxS( "import-net-map" ) );
    IMPORT_NET_MAP map;
    map.sourceDesignDigest = wxS( "source-digest" );
    KIID uuid;
    WX_STRING_REPORTER reporter;
    BOOST_REQUIRE( WriteImportNetMap( map, uuid, root, reporter ) );
    BOOST_CHECK( !ReadImportNetMap( KIID(), root, reporter ) );
    BOOST_CHECK( !WriteImportNetMap( map, KIID(), root, reporter ) );
    map.sourceDesignDigest = wxS( "other-source" );
    BOOST_CHECK( !WriteImportNetMap( map, uuid, root, reporter ) );
    auto original = ReadImportNetMap( uuid, root, reporter );
    BOOST_REQUIRE( original );
    BOOST_CHECK_EQUAL( original->sourceDesignDigest, wxString( "source-digest" ) );
    BOOST_CHECK( !reporter.GetMessages().IsEmpty() );
    wxRemoveFile( ImportNetMapPath( root ) );
    wxRemoveFile( root );
}

BOOST_AUTO_TEST_CASE( InvalidSchemaAndMalformedDocumentsArePreserved )
{
    wxString root = wxFileName::CreateTempFileName( wxS( "import-net-map" ) );
    const wxString companion = ImportNetMapPath( root );
    IMPORT_NET_MAP map;
    KIID uuid;
    WX_STRING_REPORTER reporter;

    for( const wxString& content : { wxString( "{broken" ),
                                    wxString( "{\"schemaVersion\":2}" ) } )
    {
        {
            wxFile file( companion, wxFile::write );
            BOOST_REQUIRE( file.Write( content ) );
        }

        BOOST_CHECK( !ReadImportNetMap( uuid, root, reporter ) );
        BOOST_CHECK( !WriteImportNetMap( map, uuid, root, reporter ) );
        wxFile file( companion );
        wxString preserved;
        BOOST_REQUIRE( file.ReadAll( &preserved ) );
        BOOST_CHECK_EQUAL( preserved, content );
    }

    wxRemoveFile( companion );
    wxRemoveFile( root );
}

BOOST_AUTO_TEST_CASE( InvalidTerminalReferenceDoesNotCreateRandomIdentity )
{
    wxString root = wxFileName::CreateTempFileName( wxS( "import-net-map" ) );
    const wxString companion = ImportNetMapPath( root );
    IMPORT_NET_MAP map;
    IMPORT_NET_MAP_ENTRY entry;
    entry.terminals.emplace_back();
    map.entries.push_back( entry );
    KIID uuid;
    WX_STRING_REPORTER reporter;
    BOOST_REQUIRE( WriteImportNetMap( map, uuid, root, reporter ) );
    wxString content;
    {
        wxFile file( companion );
        BOOST_REQUIRE( file.ReadAll( &content ) );
    }
    auto document = nlohmann::json::parse( content.ToStdString() );
    document["entries"][0]["terminals"][0]["symbolUuid"] = "invalid";
    const wxString invalid = wxString::FromUTF8( document.dump() );
    {
        wxFile file( companion, wxFile::write );
        BOOST_REQUIRE( file.Write( invalid ) );
    }
    BOOST_CHECK( !ReadImportNetMap( uuid, root, reporter ) );
    BOOST_CHECK( !WriteImportNetMap( map, uuid, root, reporter ) );
    {
        wxFile file( companion );
        BOOST_REQUIRE( file.ReadAll( &content ) );
    }
    BOOST_CHECK_EQUAL( content, invalid );
    wxRemoveFile( companion );
    wxRemoveFile( root );
}

BOOST_AUTO_TEST_CASE( NumericIdentityTypesAndRangesAreStrict )
{
    wxString root = wxFileName::CreateTempFileName( wxS( "import-net-map" ) );
    const wxString companion = ImportNetMapPath( root );
    IMPORT_NET_MAP map;
    IMPORT_NET_MAP_ENTRY entry;
    entry.terminals.emplace_back();
    map.entries.push_back( entry );
    KIID uuid;
    WX_STRING_REPORTER reporter;
    BOOST_REQUIRE( WriteImportNetMap( map, uuid, root, reporter ) );
    wxString content;
    {
        wxFile file( companion );
        BOOST_REQUIRE( file.ReadAll( &content ) );
    }
    const auto original = nlohmann::json::parse( content.ToStdString() );
    const std::vector<std::string> fields = { "/schemaVersion", "/entries/0/sourceNetId",
            "/entries/0/terminals/0/sourcePinId", "/entries/0/terminals/0/unit",
            "/entries/0/terminals/0/duplicateIndex" };

    for( const std::string& field : fields )
    {
        for( const nlohmann::json& value : std::vector<nlohmann::json>{ 1.5, -1, true, "1", 4294967296ULL } )
        {
            BOOST_TEST_CONTEXT( field << "=" << value.dump() )
            {
                auto document = original;
                document[nlohmann::json::json_pointer( field )] = value;
                const wxString invalid = wxString::FromUTF8( document.dump() );
                {
                    wxFile file( companion, wxFile::write );
                    BOOST_REQUIRE( file.Write( invalid ) );
                }
                BOOST_CHECK( !ReadImportNetMap( uuid, root, reporter ) );
                BOOST_CHECK( !WriteImportNetMap( map, uuid, root, reporter ) );
                {
                    wxFile file( companion );
                    BOOST_REQUIRE( file.ReadAll( &content ) );
                }
                BOOST_CHECK_EQUAL( content, invalid );
            }
        }
    }

    wxRemoveFile( companion );
    wxRemoveFile( root );
}

BOOST_AUTO_TEST_CASE( FailedCompanionWriteIsReported )
{
    wxString existingFile = wxFileName::CreateTempFileName( wxS( "import-net-map" ) );
    const wxString impossibleRoot = existingFile + wxS( "/root.kicad_sch" );
    WX_STRING_REPORTER reporter;
    BOOST_CHECK( !WriteImportNetMap( IMPORT_NET_MAP(), KIID(), impossibleRoot, reporter ) );
    BOOST_CHECK( !reporter.GetMessages().IsEmpty() );
    BOOST_CHECK( wxFileExists( existingFile ) );
    wxRemoveFile( existingFile );
}

BOOST_AUTO_TEST_CASE( HelperReloadFromSecondaryRootOwnsMapAndRetainsImportNameAfterRename )
{
    wxString directory = wxFileName::CreateTempFileName( wxS( "import-net-map-project" ) );
    wxRemoveFile( directory );
    BOOST_REQUIRE( wxFileName::Mkdir( directory ) );
    const wxString source = wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() ) + wxS( "issue25198/" );

    for( const wxString& name : { wxString( "SVG-Test.kicad_pro" ), wxString( "SVG-Test.kicad_sch" ),
                                  wxString( "toplevel2.kicad_sch" ), wxString( "toplevel3.kicad_sch" ) } )
    {
        BOOST_REQUIRE( wxCopyFile( source + name, directory + wxS( "/" ) + name ) );
    }

    const wxString firstRoot = directory + wxS( "/SVG-Test.kicad_sch" );
    const wxString secondRoot = directory + wxS( "/toplevel2.kicad_sch" );
    std::unique_ptr<SCHEMATIC> schematic( EESCHEMA_HELPERS::LoadSchematic( firstRoot, false, false ) );
    BOOST_REQUIRE( schematic );
    BOOST_REQUIRE_EQUAL( schematic->GetTopLevelSheets().size(), 3 );
    auto* label = new SCH_LABEL( VECTOR2I( 10000000, 10000000 ), wxS( "NameAtImport" ) );
    const KIID labelId = label->m_Uuid;
    schematic->RootScreen()->Append( label );
    IMPORT_NET_MAP map;
    map.sourceDesignDigest = wxS( "fixture-provenance" );
    IMPORT_NET_MAP_ENTRY entry;
    entry.originalName = wxS( "N123" );
    entry.nameAtImport = label->GetText();
    entry.itemUuids.push_back( labelId );
    map.entries.push_back( entry );
    schematic->SetImportNetMap( std::move( map ) );
    SCH_IO_KICAD_SEXPR io;
    io.SaveSchematicFile( firstRoot, schematic->GetTopLevelSheet(), schematic.get() );
    WX_STRING_REPORTER reporter;
    BOOST_REQUIRE( schematic->SaveImportNetMap( firstRoot, reporter ) );
    PROJECT* project = &schematic->Project();
    schematic.reset();

    std::unique_ptr<SCHEMATIC> reloaded(
            EESCHEMA_HELPERS::LoadSchematic( secondRoot, false, false, project ) );
    BOOST_REQUIRE( reloaded );
    BOOST_REQUIRE_EQUAL( reloaded->GetTopLevelSheets().size(), 3 );
    BOOST_REQUIRE( reloaded->GetImportNetMap() );
    BOOST_REQUIRE_EQUAL( reloaded->GetImportNetMap()->entries.size(), 1 );
    BOOST_CHECK_EQUAL( reloaded->GetImportNetMap()->entries[0].nameAtImport, wxString( "NameAtImport" ) );
    SCH_SHEET_PATH path;
    auto* reloadedLabel = dynamic_cast<SCH_LABEL*>( reloaded->ResolveItem( labelId, &path ) );
    BOOST_REQUIRE( reloadedLabel );
    reloadedLabel->SetText( wxS( "RenamedAfterImport" ) );
    reloaded->ConnectionGraph()->Recalculate( reloaded->Hierarchy(), true );
    BOOST_REQUIRE( reloadedLabel->Connection( &path ) );
    BOOST_CHECK_EQUAL( reloadedLabel->Connection( &path )->Name( true ), wxString( "RenamedAfterImport" ) );
    io.SaveSchematicFile( firstRoot, reloaded->GetTopLevelSheet(), reloaded.get() );
    BOOST_REQUIRE( reloaded->SaveImportNetMap( firstRoot, reporter ) );
    reloaded.reset();
    reloaded.reset( EESCHEMA_HELPERS::LoadSchematic( secondRoot, false, false, project ) );
    BOOST_REQUIRE( reloaded );
    BOOST_REQUIRE( reloaded->GetImportNetMap() );
    BOOST_CHECK_EQUAL( reloaded->GetImportNetMap()->entries[0].nameAtImport, wxString( "NameAtImport" ) );
    reloadedLabel = dynamic_cast<SCH_LABEL*>( reloaded->ResolveItem( labelId ) );
    BOOST_REQUIRE( reloadedLabel );
    BOOST_CHECK_EQUAL( reloadedLabel->GetText(), wxString( "RenamedAfterImport" ) );
    reloaded.reset();
    wxFileName::Rmdir( directory, wxPATH_RMDIR_RECURSIVE );
}

BOOST_AUTO_TEST_CASE( FailedCompanionWriteRetriesWithoutSchematicChanges )
{
    wxString root = wxFileName::CreateTempFileName( wxS( "import-net-map" ) );
    const wxString companion = ImportNetMapPath( root );
    SCHEMATIC schematic( nullptr );
    IMPORT_NET_MAP map;
    map.sourceDesignDigest = wxS( "retry" );
    schematic.SetImportNetMap( std::move( map ) );
    WX_STRING_REPORTER reporter;
    BOOST_REQUIRE( wxFileName::Mkdir( companion ) );
    BOOST_CHECK( !schematic.SaveImportNetMap( root, reporter ) );
    schematic.RootScreen()->SetContentModified( false );
    BOOST_CHECK( !schematic.RetryImportNetMap( reporter ) );
    BOOST_REQUIRE( wxFileName::Rmdir( companion ) );
    BOOST_CHECK( schematic.RetryImportNetMap( reporter ) );
    BOOST_CHECK( !schematic.RootScreen()->IsContentModified() );
    auto loaded = ReadImportNetMap( schematic.GetTopLevelSheet()->m_Uuid, root, reporter );
    BOOST_REQUIRE( loaded );
    BOOST_CHECK_EQUAL( loaded->sourceDesignDigest, wxString( "retry" ) );

    const wxString incompatible = wxS( "{\"schemaVersion\":999}" );
    {
        wxFile file( companion, wxFile::write );
        BOOST_REQUIRE( file.Write( incompatible ) );
    }
    schematic.LoadImportNetMap( root, reporter );
    BOOST_CHECK( !schematic.GetImportNetMap() );
    BOOST_CHECK( schematic.RetryImportNetMap( reporter ) );
    {
        wxFile file( companion );
        wxString content;
        BOOST_REQUIRE( file.ReadAll( &content ) );
        BOOST_CHECK_EQUAL( content, incompatible );
    }
    wxRemoveFile( companion );
    wxRemoveFile( root );
}

BOOST_AUTO_TEST_CASE( SchematicOwnsImportData )
{
    SCHEMATIC schematic( nullptr );
    {
        IMPORT_NET_MAP map;
        map.sourceDesignDigest = wxS( "owned" );
        schematic.SetImportNetMap( std::move( map ) );
    }
    BOOST_REQUIRE( schematic.GetImportNetMap() );
    BOOST_CHECK_EQUAL( schematic.GetImportNetMap()->sourceDesignDigest, wxString( "owned" ) );
    schematic.Reset();
    BOOST_CHECK( !schematic.GetImportNetMap() );
}

BOOST_AUTO_TEST_SUITE_END()
