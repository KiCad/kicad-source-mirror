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

#include <memory>
#include <set>
#include <vector>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>

#include <wx/dir.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>

#include <board.h>
#include <footprint.h>
#include <lib_id.h>
#include <pgm_base.h>
#include <project.h>
#include <project_pcb.h>
#include <reporter.h>
#include <settings/settings_manager.h>
#include <footprint_library_adapter.h>
#include <io/easyedapro/easyedapro_import_utils.h>
#include <libraries/library_manager.h>
#include <libraries/library_table.h>
#include <netlist_reader/board_netlist_updater.h>
#include <netlist_reader/pcb_netlist.h>
#include <pcb_io/altium/pcb_io_altium_designer.h>
#include <pcb_io/eagle/pcb_io_eagle.h>
#include <pcb_io/easyedapro/pcb_io_easyedapro_v3.h>
#include <tool/tool_manager.h>

#include <footprint_import_reconciler.h>


namespace
{
/// Stage a fresh, private project directory and wire up the global library manager for it.
wxString stageProject( const wxString& aStem )
{
    wxString sep = wxFileName::GetPathSeparator();
    wxString dir = wxStandardPaths::Get().GetTempDir() + sep + aStem + wxT( "-fpreconcile-qa" );

    if( wxDirExists( dir ) )
        wxFileName::Rmdir( dir, wxPATH_RMDIR_RECURSIVE );

    wxFileName::Mkdir( dir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

    wxString projectPath = dir + sep + aStem + wxT( ".kicad_pro" );

    Pgm().GetSettingsManager().LoadProject( projectPath );
    Pgm().GetLibraryManager().LoadProjectTables(
            Pgm().GetSettingsManager().Prj().GetProjectDirectory() );

    return Pgm().GetSettingsManager().Prj().GetProjectPath();
}


/// The EasyEDA Pro v3 sample board, loaded from a private copy of the archive.
struct IMPORTED_BOARD
{
    std::unique_ptr<PCB_IO_EASYEDAPRO_V3>   m_plugin;
    std::unique_ptr<BOARD>                  m_board;
    std::vector<std::unique_ptr<FOOTPRINT>> m_definitions;
    std::unique_ptr<KI_TEST::TEMPORARY_DIRECTORY> m_sourceDir;
};


IMPORTED_BOARD importSampleBoard( PROJECT& aProject, const std::string& aTag )
{
    IMPORTED_BOARD sample;

    const wxString archiveName = wxS( "ProProject_LS2K0300Core_2025-11-14.epro2" );

    wxFileName srcFn( wxString::FromUTF8( KI_TEST::GetPcbnewTestDataDir() ) );
    srcFn.AppendDir( wxS( "plugins" ) );
    srcFn.AppendDir( wxS( "easyedapro" ) );
    srcFn.SetFullName( archiveName );
    BOOST_REQUIRE_MESSAGE( srcFn.FileExists(), "Missing EasyEDA Pro v3 board fixture" );

    sample.m_sourceDir = std::make_unique<KI_TEST::TEMPORARY_DIRECTORY>( aTag, "" );

    wxFileName importFn( wxString::FromUTF8( sample.m_sourceDir->GetPath().string() ),
                         archiveName );
    BOOST_REQUIRE( wxCopyFile( srcFn.GetFullPath(), importFn.GetFullPath() ) );

    std::map<std::string, UTF8> properties;
    properties["pcb_id"] = "eb9fbfba682940f7a002816e66fbb3d7";

    sample.m_plugin = std::make_unique<PCB_IO_EASYEDAPRO_V3>();
    sample.m_board = std::make_unique<BOARD>();
    sample.m_board->SetProject( &aProject );
    sample.m_plugin->LoadBoard( importFn.GetFullPath(), sample.m_board.get(), &properties,
                                &aProject );

    BOOST_REQUIRE_GT( sample.m_board->Footprints().size(), 0 );

    for( FOOTPRINT* fp : sample.m_plugin->GetImportedCachedLibraryFootprints() )
        sample.m_definitions.emplace_back( fp );

    BOOST_REQUIRE_GT( sample.m_definitions.size(), 0 );

    return sample;
}


/// Write @p aFootprint into a project .pretty and register it, standing in for a user library.
void publishLibrary( PROJECT& aProject, FOOTPRINT_LIBRARY_ADAPTER& aAdapter,
                     const wxString& aNickname, const wxString& aDirStem,
                     const FOOTPRINT& aFootprint )
{
    wxFileName libDir( aProject.GetProjectPath(), aDirStem,
                       wxString( FILEEXT::KiCadFootprintLibPathExtension ) );
    wxString   libPath = libDir.GetFullPath();

    IO_RELEASER<PCB_IO> pi( PCB_IO_MGR::FindPlugin( PCB_IO_MGR::KICAD_SEXP ) );
    BOOST_REQUIRE( pi );

    pi->CreateLibrary( libPath );

    std::unique_ptr<FOOTPRINT> copy( static_cast<FOOTPRINT*>( aFootprint.Clone() ) );
    copy->SetFPID( LIB_ID( aNickname, aFootprint.GetFPID().GetUniStringLibItemName() ) );
    copy->SetReference( wxS( "REF**" ) );
    pi->FootprintSave( libPath, copy.get() );

    LIBRARY_TABLE* table = aAdapter.ProjectTable().value_or( nullptr );
    BOOST_REQUIRE( table );

    LIBRARY_TABLE_ROW& row = table->InsertRow();
    row.SetNickname( aNickname );
    row.SetURI( wxS( "${KIPRJMOD}/" ) + libDir.GetFullName() );
    row.SetType( wxS( "KiCad" ) );
    row.SetScope( LIBRARY_TABLE_SCOPE::PROJECT );

    BOOST_REQUIRE( table->Save().has_value() );
    aAdapter.LoadOne( aNickname );
}


/// A placed footprint the importer also defined, i.e. one the reconciler has to place somewhere.
struct COLLISION_CANDIDATE
{
    FOOTPRINT* m_footprint = nullptr;
    wxString   m_nickname;
    wxString   m_name;
};


COLLISION_CANDIDATE findCandidate( IMPORTED_BOARD& aSample )
{
    COLLISION_CANDIDATE candidate;

    for( FOOTPRINT* fp : aSample.m_board->Footprints() )
    {
        wxString nick = fp->GetFPID().GetUniStringLibNickname();
        wxString name = fp->GetFPID().GetUniStringLibItemName();

        if( nick.IsEmpty() || name.IsEmpty() )
            continue;

        for( const std::unique_ptr<FOOTPRINT>& def : aSample.m_definitions )
        {
            if( def->GetFPID().GetUniStringLibItemName() != name || def->Pads().empty() )
                continue;

            candidate.m_footprint = fp;
            candidate.m_nickname = nick;
            candidate.m_name = name;

            return candidate;
        }
    }

    return candidate;
}
} // namespace


BOOST_AUTO_TEST_SUITE( FootprintImportReconciler )


// Eagle board reconciles to a generated cache: .pretty published, row added, every FPID resolves
// fails on revert, since Eagle FPIDs keep empty nicknames
BOOST_AUTO_TEST_CASE( EagleBoardResolvesToGeneratedCache )
{
    wxString projectPath = stageProject( wxS( "eagle_fpreconcile" ) );
    PROJECT& project = Pgm().GetSettingsManager().Prj();

    wxFileName brdFn( KI_TEST::GetEeschemaTestDataDir() );
    brdFn.AppendDir( wxS( "io" ) );
    brdFn.AppendDir( wxS( "eagle" ) );
    brdFn.SetFullName( wxS( "eagle-import-testfile.brd" ) );
    BOOST_REQUIRE_MESSAGE( brdFn.FileExists(), "Missing Eagle board fixture" );

    PCB_IO_EAGLE           plugin;
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();
    board->SetProject( &project );
    plugin.LoadBoard( brdFn.GetFullPath(), board.get(), nullptr, &project );

    BOOST_REQUIRE_GT( board->Footprints().size(), 0 );

    std::vector<FOOTPRINT*>                 raw = plugin.GetImportedCachedLibraryFootprints();
    std::vector<std::unique_ptr<FOOTPRINT>> defs;

    for( FOOTPRINT* fp : raw )
        defs.emplace_back( fp );

    FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( &project );
    BOOST_REQUIRE( adapter );

    const wxString              cacheNick = wxS( "eagle_test-import-fps" );
    FOOTPRINT_IMPORT_RECONCILER reconciler( *adapter, project.GetProjectPath() );

    FOOTPRINT_IMPORT_RECONCILE_RESULT result =
            reconciler.Reconcile( board.get(), std::move( defs ), cacheNick, {} );

    // cache library published
    BOOST_CHECK_EQUAL( result.m_cacheNickname, cacheNick );
    BOOST_CHECK_GT( result.m_savedToCache, 0 );

    wxFileName prettyDir( project.GetProjectPath(), cacheNick,
                          wxString( FILEEXT::KiCadFootprintLibPathExtension ) );
    BOOST_CHECK_MESSAGE( wxDir::Exists( prettyDir.GetFullPath() ),
                         "Generated .pretty was not published" );

    // project fp-lib table gained the row
    LIBRARY_TABLE* projectTable = adapter->ProjectTable().value_or( nullptr );
    BOOST_REQUIRE( projectTable );
    BOOST_CHECK( projectTable->HasRow( cacheNick ) );

    // every board FPID resolves via the adapter
    int resolved = 0;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        wxString name = fp->GetFPID().GetUniStringLibItemName();

        if( name.IsEmpty() )
            continue;

        wxString nick = fp->GetFPID().GetUniStringLibNickname();

        BOOST_CHECK_MESSAGE( !nick.IsEmpty(),
                             wxString::Format( "Board footprint '%s' left with empty nickname",
                                               name ) );
        BOOST_CHECK_MESSAGE( adapter->FootprintExists( nick, name ),
                             wxString::Format( "FPID '%s:%s' does not resolve after reconciliation",
                                               nick, name ) );
        resolved++;
    }

    BOOST_CHECK_GT( resolved, 0 );
    BOOST_CHECK_EQUAL( result.m_unresolved, 0 );
}


// Altium footprints carry a source-.PcbLib nick not registered here, so all fall back to the cache
// and resolve through the adapter
BOOST_AUTO_TEST_CASE( AltiumBoardResolvesToGeneratedCache )
{
    stageProject( wxS( "altium_fpreconcile" ) );
    PROJECT& project = Pgm().GetSettingsManager().Prj();

    std::string dataPath =
            KI_TEST::GetPcbnewTestDataDir() + "plugins/altium/HiFive/HiFive1.B01.PcbDoc";

    PCB_IO_ALTIUM_DESIGNER plugin;
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();
    board->SetProject( &project );
    plugin.LoadBoard( dataPath, board.get(), nullptr, &project );

    BOOST_REQUIRE_GT( board->Footprints().size(), 0 );

    std::vector<FOOTPRINT*>                 raw = plugin.GetImportedCachedLibraryFootprints();
    std::vector<std::unique_ptr<FOOTPRINT>> defs;

    for( FOOTPRINT* fp : raw )
        defs.emplace_back( fp );

    FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( &project );
    BOOST_REQUIRE( adapter );

    const wxString              cacheNick = wxS( "hifive-import-fps" );
    FOOTPRINT_IMPORT_RECONCILER reconciler( *adapter, project.GetProjectPath() );

    FOOTPRINT_IMPORT_RECONCILE_RESULT result =
            reconciler.Reconcile( board.get(), std::move( defs ), cacheNick, {} );

    BOOST_CHECK_EQUAL( result.m_cacheNickname, cacheNick );
    BOOST_CHECK_EQUAL( result.m_unresolved, 0 );

    int resolved = 0;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        wxString name = fp->GetFPID().GetUniStringLibItemName();

        if( name.IsEmpty() )
            continue;

        wxString nick = fp->GetFPID().GetUniStringLibNickname();

        BOOST_CHECK_MESSAGE( adapter->FootprintExists( nick, name ),
                             wxString::Format( "FPID '%s:%s' does not resolve after reconciliation",
                                               nick, name ) );
        resolved++;
    }

    BOOST_CHECK_GT( resolved, 0 );
}


// EasyEDA Pro v3 hands definitions over the standard hook, leaving the source directory untouched
// fails on revert, since LoadBoard then publishes its own .pretty beside the archive
BOOST_AUTO_TEST_CASE( EasyEdaProV3BoardResolvesToGeneratedCache )
{
    stageProject( wxS( "easyedapro_v3_fpreconcile" ) );
    PROJECT& project = Pgm().GetSettingsManager().Prj();

    IMPORTED_BOARD sample = importSampleBoard( project, "easyedapro_v3_fpreconcile_src" );
    BOARD*         board = sample.m_board.get();

    // the importer must not have published anything of its own beside the archive
    wxFileName srcDir( wxString::FromUTF8( sample.m_sourceDir->GetPath().string() ),
                       wxEmptyString );
    wxFileName strayLib( srcDir.GetPath(),
                         EASYEDAPRO::ShortenLibName( wxS( "ProProject_LS2K0300Core_2025-11-14" ) ),
                         wxString( FILEEXT::KiCadFootprintLibPathExtension ) );
    BOOST_CHECK_MESSAGE( !wxDir::Exists( strayLib.GetFullPath() ),
                         "LoadBoard wrote a library into the source directory" );

    FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( &project );
    BOOST_REQUIRE( adapter );

    const wxString              cacheNick = wxS( "ls2k0300-import-fps" );
    FOOTPRINT_IMPORT_RECONCILER reconciler( *adapter, project.GetProjectPath() );

    FOOTPRINT_IMPORT_RECONCILE_RESULT result =
            reconciler.Reconcile( board, std::move( sample.m_definitions ), cacheNick, {} );

    // the cache lands in the project directory, not next to the source archive
    BOOST_CHECK_EQUAL( result.m_cacheNickname, cacheNick );
    BOOST_CHECK_GT( result.m_savedToCache, 0 );

    wxFileName prettyDir( project.GetProjectPath(), cacheNick,
                          wxString( FILEEXT::KiCadFootprintLibPathExtension ) );
    BOOST_CHECK_MESSAGE( wxDir::Exists( prettyDir.GetFullPath() ),
                         "Generated .pretty was not published into the project" );

    LIBRARY_TABLE* projectTable = adapter->ProjectTable().value_or( nullptr );
    BOOST_REQUIRE( projectTable );
    BOOST_CHECK( projectTable->HasRow( cacheNick ) );

    int resolved = 0;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        wxString name = fp->GetFPID().GetUniStringLibItemName();

        if( name.IsEmpty() )
            continue;

        wxString nick = fp->GetFPID().GetUniStringLibNickname();

        BOOST_CHECK_MESSAGE( adapter->FootprintExists( nick, name ),
                             wxString::Format( "FPID '%s:%s' does not resolve after reconciliation",
                                               nick, name ) );
        resolved++;
    }

    BOOST_CHECK_GT( resolved, 0 );
    BOOST_CHECK_EQUAL( result.m_unresolved, 0 );
}


// An unrelated library that happens to carry the nickname the importer emitted must not swallow
// the imported definition; without the provenance check the footprint relinks to the wrong part
BOOST_AUTO_TEST_CASE( CollidingNicknameDoesNotStealTheLink )
{
    stageProject( wxS( "fpreconcile_collide" ) );
    PROJECT& project = Pgm().GetSettingsManager().Prj();

    IMPORTED_BOARD      sample = importSampleBoard( project, "fpreconcile_collide_src" );
    COLLISION_CANDIDATE candidate = findCandidate( sample );
    BOOST_REQUIRE( candidate.m_footprint );

    FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( &project );
    BOOST_REQUIRE( adapter );

    // a padless namesake registered under the importer's nickname: same name, different part
    FOOTPRINT impostor( nullptr );
    impostor.SetFPID( LIB_ID( candidate.m_nickname, candidate.m_name ) );
    BOOST_REQUIRE( impostor.Pads().empty() );
    publishLibrary( project, *adapter, candidate.m_nickname, wxS( "impostor" ), impostor );

    const wxString              cacheNick = wxS( "collide-import-fps" );
    FOOTPRINT_IMPORT_RECONCILER reconciler( *adapter, project.GetProjectPath() );

    FOOTPRINT_IMPORT_RECONCILE_RESULT result = reconciler.Reconcile(
            sample.m_board.get(), std::move( sample.m_definitions ), cacheNick, {} );

    BOOST_CHECK_EQUAL( result.m_cacheNickname, cacheNick );

    // the imported definition is kept in the cache rather than dropped for the namesake
    BOOST_CHECK_EQUAL( candidate.m_footprint->GetFPID().GetUniStringLibNickname(), cacheNick );

    std::unique_ptr<FOOTPRINT> linked( adapter->LoadFootprint( cacheNick, candidate.m_name,
                                                               true ) );
    BOOST_REQUIRE( linked );
    BOOST_CHECK_GT( linked->Pads().size(), 0 );
}


// A project row already owning the cache nickname is a user library even when no .pretty sits at
// the generated path, so publishing must not rewrite its URI
BOOST_AUTO_TEST_CASE( ExistingUserRowIsNotRepurposed )
{
    stageProject( wxS( "fpreconcile_row" ) );
    PROJECT& project = Pgm().GetSettingsManager().Prj();

    IMPORTED_BOARD sample = importSampleBoard( project, "fpreconcile_row_src" );

    FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( &project );
    BOOST_REQUIRE( adapter );

    const wxString cacheNick = wxS( "row-import-fps" );

    // the user's row owns the nickname but points at a library of its own
    FOOTPRINT owned( nullptr );
    owned.SetFPID( LIB_ID( cacheNick, wxS( "UserPart" ) ) );
    publishLibrary( project, *adapter, cacheNick, wxS( "user-owned" ), owned );

    LIBRARY_TABLE* table = adapter->ProjectTable().value_or( nullptr );
    BOOST_REQUIRE( table );

    LIBRARY_TABLE_ROW* row = table->Row( cacheNick ).value_or( nullptr );
    BOOST_REQUIRE( row );

    const wxString uriBefore = row->URI();

    FOOTPRINT_IMPORT_RECONCILER reconciler( *adapter, project.GetProjectPath() );

    FOOTPRINT_IMPORT_RECONCILE_RESULT result = reconciler.Reconcile(
            sample.m_board.get(), std::move( sample.m_definitions ), cacheNick, {} );

    BOOST_CHECK( result.m_cacheNickname.IsEmpty() );
    BOOST_CHECK_EQUAL( row->URI(), uriBefore );

    wxFileName prettyDir( project.GetProjectPath(), cacheNick,
                          wxString( FILEEXT::KiCadFootprintLibPathExtension ) );
    BOOST_CHECK_MESSAGE( !wxDir::Exists( prettyDir.GetFullPath() ),
                         "Published a cache over a nickname the user already owns" );
}


// regression gate for a netlist of reconciled FPIDs applying via BOARD_NETLIST_UPDATER with 0 errors
// and no not-found, over the same adapter path Update PCB from Schematic uses
BOOST_AUTO_TEST_CASE( ReconciledFootprintsResolveViaNetlistUpdater )
{
    stageProject( wxS( "eagle_netlist_roundtrip" ) );
    PROJECT& project = Pgm().GetSettingsManager().Prj();

    wxFileName brdFn( KI_TEST::GetEeschemaTestDataDir() );
    brdFn.AppendDir( wxS( "io" ) );
    brdFn.AppendDir( wxS( "eagle" ) );
    brdFn.SetFullName( wxS( "eagle-import-testfile.brd" ) );
    BOOST_REQUIRE( brdFn.FileExists() );

    PCB_IO_EAGLE           plugin;
    std::unique_ptr<BOARD> imported = std::make_unique<BOARD>();
    imported->SetProject( &project );
    plugin.LoadBoard( brdFn.GetFullPath(), imported.get(), nullptr, &project );

    std::vector<FOOTPRINT*>                 raw = plugin.GetImportedCachedLibraryFootprints();
    std::vector<std::unique_ptr<FOOTPRINT>> defs;

    for( FOOTPRINT* fp : raw )
        defs.emplace_back( fp );

    FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( &project );
    BOOST_REQUIRE( adapter );

    const wxString              cacheNick = wxS( "eagle_test-import-fps" );
    FOOTPRINT_IMPORT_RECONCILER reconciler( *adapter, project.GetProjectPath() );
    reconciler.Reconcile( imported.get(), std::move( defs ), cacheNick, {} );

    // netlist of distinct reconciled FPIDs, one fresh component each
    NETLIST            netlist;
    std::set<wxString> seen;
    int                expectedComponents = 0;

    for( FOOTPRINT* fp : imported->Footprints() )
    {
        LIB_ID fpid = fp->GetFPID();

        if( fpid.GetUniStringLibItemName().IsEmpty() || !seen.insert( fpid.GetUniStringLibId() ).second )
            continue;

        wxString ref = wxString::Format( wxS( "U%d" ), ++expectedComponents );
        netlist.AddComponent(
                new COMPONENT( fpid, ref, ref, KIID_PATH(), std::vector<KIID>{ KIID() } ) );
    }

    BOOST_REQUIRE_GT( expectedComponents, 0 );

    // updater must load each footprint from the reconciled lib onto a fresh board
    std::unique_ptr<BOARD> target = std::make_unique<BOARD>();
    target->SetProject( &project );

    TOOL_MANAGER toolMgr;
    toolMgr.SetEnvironment( target.get(), nullptr, nullptr, nullptr, nullptr );
    toolMgr.RegisterTool( new KI_TEST::DUMMY_TOOL() );

    WX_STRING_REPORTER    reporter;
    BOARD_NETLIST_UPDATER updater( &toolMgr, target.get() );
    updater.SetReporter( &reporter );
    updater.SetReplaceFootprints( false );
    updater.SetDeleteUnusedFootprints( false );

    BOOST_REQUIRE( updater.UpdateNetlist( netlist ) );

    BOOST_CHECK_EQUAL( updater.GetErrorCount(), 0 );
    BOOST_CHECK_MESSAGE( !reporter.GetMessages().Lower().Contains( wxS( "not found" ) ),
                         "Netlist updater reported a footprint not found after reconciliation" );
    BOOST_CHECK_EQUAL( static_cast<int>( target->Footprints().size() ), expectedComponents );
}


BOOST_AUTO_TEST_SUITE_END()
