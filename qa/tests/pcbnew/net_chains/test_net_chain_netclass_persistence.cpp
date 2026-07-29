/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

/*
 * Regression test for https://gitlab.com/kicad/code/kicad/-/work_items/25065
 *
 * A netclass assigned by a net chain resolved on the board right after Update PCB from
 * Schematic, but was gone after save + reload: the chain-to-netclass override lived only in the
 * schematic and the transient netlist, so nothing on the board side could re-derive the per-net
 * assignments.
 */

#include <board.h>
#include <board_design_settings.h>
#include <netinfo.h>
#include <netlist_reader/board_netlist_updater.h>
#include <netlist_reader/pcb_netlist.h>
#include <project.h>
#include <project/net_settings.h>
#include <project/project_file.h>
#include <settings/settings_manager.h>

#include <pcbnew_utils/board_file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>


BOOST_AUTO_TEST_SUITE( NetChainNetclassPersistence )


namespace
{

// Chain memberships and netclass overrides as the issue's schematic declares them; the board's
// (net_chains ...) block carries the same memberships.
const wxString CHAIN_1 = wxS( "CHAIN_1" );
const wxString CHAIN_2 = wxS( "CHAIN_2" );
const wxString CHAIN_1_NETCLASS = wxS( "CHAIN_NC_1" );
const wxString CHAIN_2_NETCLASS = wxS( "CHAIN_NC_2" );

const std::vector<wxString> CHAIN_1_NETS = { wxS( "Net-(R1-Pad1)" ), wxS( "Net-(R1-Pad2)" ),
                                             wxS( "Net-(R2-Pad2)" ) };
const std::vector<wxString> CHAIN_2_NETS = { wxS( "Net-(R3-Pad1)" ), wxS( "Net-(R3-Pad2)" ),
                                             wxS( "Net-(R4-Pad2)" ) };

// Assigned by a schematic directive label rather than a chain, so it round-tripped even before
// the fix.  Serves as the positive control.
const wxString LABEL_NET = wxS( "Net-(R5-Pad1)" );
const wxString LABEL_NETCLASS = wxS( "NC_3" );


struct FIXTURE
{
    FIXTURE()
    {
        // The project is saved and reloaded repeatedly, so work on copies rather than the
        // checked-in data.
        wxString srcDir = wxString::FromUTF8( KI_TEST::GetPcbnewTestDataDir() ) + wxS( "issue25065/" );

        m_dir.AssignDir( wxStandardPaths::Get().GetTempDir() );
        m_dir.AppendDir( wxS( "kicad-qa-issue25065" ) );
        m_dir.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );
        BOOST_REQUIRE( m_dir.DirExists() );

        m_boardPath = m_dir.GetPathWithSep() + wxS( "issue25065.kicad_pcb" );
        m_projectPath = m_dir.GetPathWithSep() + wxS( "issue25065.kicad_pro" );

        BOOST_REQUIRE( wxCopyFile( srcDir + wxS( "issue25065.kicad_pcb" ), m_boardPath ) );
        BOOST_REQUIRE( wxCopyFile( srcDir + wxS( "issue25065.kicad_pro" ), m_projectPath ) );
    }

    ~FIXTURE()
    {
        closeBoard();
        m_dir.Rmdir( wxPATH_RMDIR_RECURSIVE );
    }

    /// Load the project and board from the temp copies, as opening the board editor would.
    void OpenBoard()
    {
        BOOST_REQUIRE( m_manager.LoadProject( m_projectPath ) );

        m_board = KI_TEST::ReadBoardFromFileOrStream( m_boardPath.ToStdString() );
        BOOST_REQUIRE( m_board );

        m_board->SetProject( &m_manager.Prj() );
        m_board->BuildListOfNets();
    }

    /// Save the project and reopen it from disk, as quitting and restarting KiCad would.
    void SaveAndReopenBoard()
    {
        BOOST_REQUIRE( m_manager.SaveProject() );

        closeBoard();
        OpenBoard();
    }

    /// Mirror the netlist chain data onto the board the way Update PCB from Schematic does.
    void UpdateFromNetlist( const NETLIST& aNetlist )
    {
        BOARD_NETLIST_UPDATER::ApplyChainAssignments( m_board.get(), aNetlist, nullptr, false );
        BOARD_NETLIST_UPDATER::ApplyChainNetclasses( m_board.get(), aNetlist );
        m_board->SynchronizeNetsAndNetClasses( true );
    }

    NET_SETTINGS& NetSettings() const { return *m_board->GetDesignSettings().m_NetSettings; }

    /// The netclass the board resolved for a net, as the status bar reports it.
    wxString ResolvedNetclass( const wxString& aNetname ) const
    {
        NETINFO_ITEM* net = m_board->FindNet( aNetname );

        return net ? net->GetNetClass()->GetHumanReadableName() : wxString( wxS( "<no such net>" ) );
    }

    bool ResolvesTo( const wxString& aNetname, const wxString& aNetclass ) const
    {
        NETINFO_ITEM* net = m_board->FindNet( aNetname );
        BOOST_REQUIRE_MESSAGE( net, "net " << aNetname << " missing from the board" );

        return net->GetNetClass()->ContainsNetclassWithName( aNetclass );
    }

    void closeBoard()
    {
        if( m_board )
        {
            m_board->SetProject( nullptr );
            m_board.reset();
        }

        m_manager.UnloadProject( &m_manager.Prj(), false );
    }

    SETTINGS_MANAGER       m_manager;
    std::unique_ptr<BOARD> m_board;
    wxFileName             m_dir;
    wxString               m_boardPath;
    wxString               m_projectPath;
};


/// Add a chain, its netclass override and its member nets, as the schematic exporter would.
/// Fills in place because NETLIST holds a boost::ptr_vector and is neither copyable nor movable.
void AddChain( NETLIST& aNetlist, const wxString& aChain, const wxString& aNetclass,
               const std::vector<wxString>& aNets )
{
    aNetlist.SetSignalNetClass( aChain, aNetclass );

    for( const wxString& net : aNets )
        aNetlist.SetNetChainFor( net, aChain );
}

} // namespace


// The whole point of the issue: the chain netclass must survive a save and reload with no
// intervening Update PCB from Schematic.
BOOST_FIXTURE_TEST_CASE( ChainNetclassSurvivesReload, FIXTURE )
{
    OpenBoard();

    // The netclass a directive label assigns is persisted per-net and resolves on a bare load;
    // if this control fails the board or project copy is wrong, not the chain handling.
    BOOST_REQUIRE_MESSAGE( ResolvesTo( LABEL_NET, LABEL_NETCLASS ),
                           LABEL_NET << " resolved to " << ResolvedNetclass( LABEL_NET ) );

    NETLIST netlist;
    AddChain( netlist, CHAIN_1, CHAIN_1_NETCLASS, CHAIN_1_NETS );
    AddChain( netlist, CHAIN_2, CHAIN_2_NETCLASS, CHAIN_2_NETS );

    UpdateFromNetlist( netlist );

    for( const wxString& net : CHAIN_1_NETS )
    {
        BOOST_REQUIRE_MESSAGE( ResolvesTo( net, CHAIN_1_NETCLASS ),
                               net << " resolved to " << ResolvedNetclass( net )
                                   << " straight after the netlist update" );
    }

    SaveAndReopenBoard();

    for( const wxString& net : CHAIN_1_NETS )
    {
        BOOST_CHECK_MESSAGE( ResolvesTo( net, CHAIN_1_NETCLASS ),
                             net << " resolved to " << ResolvedNetclass( net ) << " after reload" );
    }

    for( const wxString& net : CHAIN_2_NETS )
    {
        BOOST_CHECK_MESSAGE( ResolvesTo( net, CHAIN_2_NETCLASS ),
                             net << " resolved to " << ResolvedNetclass( net ) << " after reload" );
    }

    BOOST_CHECK_MESSAGE( ResolvesTo( LABEL_NET, LABEL_NETCLASS ),
                         LABEL_NET << " resolved to " << ResolvedNetclass( LABEL_NET )
                                   << " after reload" );

    // Nets outside any chain must not pick one up.
    BOOST_CHECK( !ResolvesTo( wxS( "Net-(R6-Pad2)" ), CHAIN_1_NETCLASS ) );
}


// Persisting the override must not resurrect it: a chain dropped from the schematic stops
// applying its netclass, both in the session and across the next reload.
BOOST_FIXTURE_TEST_CASE( RemovedChainNetclassIsDropped, FIXTURE )
{
    OpenBoard();

    NETLIST netlist;
    AddChain( netlist, CHAIN_1, CHAIN_1_NETCLASS, CHAIN_1_NETS );
    AddChain( netlist, CHAIN_2, CHAIN_2_NETCLASS, CHAIN_2_NETS );

    UpdateFromNetlist( netlist );
    SaveAndReopenBoard();
    BOOST_REQUIRE( ResolvesTo( CHAIN_1_NETS[0], CHAIN_1_NETCLASS ) );

    NETLIST withoutChain1;
    AddChain( withoutChain1, CHAIN_2, CHAIN_2_NETCLASS, CHAIN_2_NETS );

    UpdateFromNetlist( withoutChain1 );

    BOOST_CHECK( NetSettings().GetNetChainNetClass( CHAIN_1 ).IsEmpty() );
    BOOST_CHECK_MESSAGE( !ResolvesTo( CHAIN_1_NETS[0], CHAIN_1_NETCLASS ),
                         CHAIN_1_NETS[0] << " still resolved to "
                                         << ResolvedNetclass( CHAIN_1_NETS[0] ) );
    BOOST_CHECK( ResolvesTo( CHAIN_2_NETS[0], CHAIN_2_NETCLASS ) );

    // The board file still lists CHAIN_1's membership, so a stale project-side override would
    // reappear here.
    SaveAndReopenBoard();

    BOOST_CHECK( !ResolvesTo( CHAIN_1_NETS[0], CHAIN_1_NETCLASS ) );
    BOOST_CHECK( ResolvesTo( CHAIN_2_NETS[0], CHAIN_2_NETCLASS ) );
}


BOOST_AUTO_TEST_SUITE_END()
