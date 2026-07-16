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
#include <libraries/library_manager.h>
#include <libraries/library_table.h>
#include <netlist_reader/board_netlist_updater.h>
#include <netlist_reader/pcb_netlist.h>
#include <pcb_io/altium/pcb_io_altium_designer.h>
#include <pcb_io/eagle/pcb_io_eagle.h>
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
