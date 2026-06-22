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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/21427
 *
 * Netlist net-point coordinates must use the same coordinate system as
 * the copper features. Previously the netlist subtracted the board aux
 * origin, which shifted all net points when the aux origin was non-zero.
 */

#include <boost/test/unit_test.hpp>

#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <base_units.h>
#include <core/utf8.h>

#include <pcbnew/pcb_io/odbpp/pcb_io_odbpp.h>

#include <wx/dir.h>
#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/utils.h>
#include <wx/regex.h>


namespace
{

wxFileName MakeTempDir( const wxString& aPrefix )
{
    wxFileName tempDir(
            wxFileName::CreateTempFileName( wxString::Format( wxT( "kicad-%s-" ), aPrefix ) ) );

    BOOST_REQUIRE( tempDir.FileExists() );
    BOOST_REQUIRE( wxRemoveFile( tempDir.GetFullPath() ) );
    BOOST_REQUIRE( tempDir.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );

    return tempDir;
}


// Save and restore the static ODB++ exporter formatting state so this regression
// test does not change formatting defaults for later tests in the shared binary.
struct ODB_EXPORT_STATE_GUARD
{
    ODB_EXPORT_STATE_GUARD() :
            m_scale( PCB_IO_ODBPP::m_scale ),
            m_symbolScale( PCB_IO_ODBPP::m_symbolScale ),
            m_sigfig( PCB_IO_ODBPP::m_sigfig ),
            m_unitsStr( PCB_IO_ODBPP::m_unitsStr )
    {
    }

    ~ODB_EXPORT_STATE_GUARD()
    {
        PCB_IO_ODBPP::m_scale = m_scale;
        PCB_IO_ODBPP::m_symbolScale = m_symbolScale;
        PCB_IO_ODBPP::m_sigfig = m_sigfig;
        PCB_IO_ODBPP::m_unitsStr = m_unitsStr;
    }

    double      m_scale;
    double      m_symbolScale;
    int         m_sigfig;
    std::string m_unitsStr;
};


wxString ReadFile( const wxFileName& aPath )
{
    wxFFile stream( aPath.GetFullPath(), wxT( "rb" ) );
    wxString contents;
    BOOST_REQUIRE( stream.ReadAll( &contents ) );
    stream.Close();
    return contents;
}

} // anonymous namespace


BOOST_AUTO_TEST_CASE( ODBNetlistCoordinatesMatchFeatures )
{
    wxFileName tempDir = MakeTempDir( wxT( "odb-netlist" ) );

    ODB_EXPORT_STATE_GUARD odbExportStateGuard;

    BOARD board;
    board.SetCopperLayerCount( 2 );

    // Set a non-zero aux origin to trigger the bug
    VECTOR2I auxOrigin( pcbIUScale.mmToIU( 50.0 ), pcbIUScale.mmToIU( 30.0 ) );
    board.GetDesignSettings().SetAuxOrigin( auxOrigin );

    // Add a footprint with a pad at a known position
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 100.0 ), pcbIUScale.mmToIU( 80.0 ) ) );
    fp->SetReference( wxT( "U1" ) );

    PAD* pad = new PAD( fp );
    pad->SetAttribute( PAD_ATTRIB::SMD );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
    pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( pcbIUScale.mmToIU( 1.0 ), pcbIUScale.mmToIU( 0.5 ) ) );
    pad->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 100.0 ), pcbIUScale.mmToIU( 80.0 ) ) );
    pad->SetLayerSet( LSET( { F_Cu, F_Mask, F_Paste } ) );
    pad->SetNumber( wxT( "1" ) );

    NETINFO_ITEM* net1 = new NETINFO_ITEM( &board, wxT( "TestNet" ), 1 );
    board.Add( net1 );
    pad->SetNet( net1 );

    fp->Add( pad );
    board.Add( fp );

    // Add a via at a known position
    PCB_VIA* via = new PCB_VIA( &board );
    via->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 120.0 ), pcbIUScale.mmToIU( 60.0 ) ) );
    via->SetLayerPair( F_Cu, B_Cu );
    via->SetDrill( pcbIUScale.mmToIU( 0.3 ) );
    via->SetWidth( pcbIUScale.mmToIU( 0.6 ) );
    via->SetNet( net1 );
    board.Add( via );

    // Export ODB++
    wxFileName odbRoot( tempDir.GetFullPath(), wxEmptyString );
    odbRoot.AppendDir( wxT( "odb_out" ) );
    BOOST_REQUIRE( odbRoot.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );

    PCB_IO_ODBPP odbExporter;
    std::map<std::string, UTF8> props;
    props["units"] = "mm";
    props["sigfig"] = "6";
    BOOST_REQUIRE_NO_THROW( odbExporter.SaveBoard( odbRoot.GetFullPath(), &board, &props ) );

    // Read the netlist file
    wxFileName netlistFile( odbRoot.GetFullPath(), wxEmptyString );
    netlistFile.AppendDir( wxT( "steps" ) );
    netlistFile.AppendDir( wxT( "pcb" ) );
    netlistFile.AppendDir( wxT( "netlists" ) );
    netlistFile.AppendDir( wxT( "cadnet" ) );
    netlistFile.SetFullName( wxT( "netlist" ) );
    BOOST_REQUIRE( netlistFile.FileExists() );

    wxString netlistContent = ReadFile( netlistFile );

    // Read a features file that has the toeprint/component position to compare
    wxFileName compFile( odbRoot.GetFullPath(), wxEmptyString );
    compFile.AppendDir( wxT( "steps" ) );
    compFile.AppendDir( wxT( "pcb" ) );
    compFile.AppendDir( wxT( "layers" ) );
    compFile.AppendDir( wxT( "comp_+_top" ) );
    compFile.SetFullName( wxT( "components" ) );
    BOOST_REQUIRE( compFile.FileExists() );

    wxString compContent = ReadFile( compFile );

    // The pad is at board position (100, 80) mm.
    // ODB::AddXY applies m_scale and negates Y, giving (100.0, -80.0) in mm.
    // The toeprint (TOP record) should have x=100.0, y=-80.0 if in mm.
    // The netlist net-point should have the same coordinates.

    // Verify the toeprint has the pad position (component file uses ODB::AddXY)
    BOOST_CHECK_MESSAGE( compContent.Contains( wxT( "100" ) ),
                         "Component file should contain the pad X coordinate" );

    // The critical check: the netlist should NOT be offset by the aux origin.
    // With the bug, x would be (100 - 50) = 50 and y would be (30 - 80) = -50 in mm.
    // With the fix, x should be 100 and y should be -80 in mm.
    // We look for "100" and "-80" appearing in the netlist net-point data.
    BOOST_CHECK_MESSAGE( !netlistContent.Contains( wxT( "50.0" ) ),
                         "Netlist should not contain aux-origin-shifted X coordinate 50.0" );
    BOOST_CHECK_MESSAGE( netlistContent.Contains( wxT( "100.0" ) ),
                         "Netlist should contain the raw pad X coordinate 100.0" );

    // Clean up
    wxFileName::Rmdir( odbRoot.GetFullPath(), wxPATH_RMDIR_RECURSIVE );
    wxFileName::Rmdir( tempDir.GetFullPath(), wxPATH_RMDIR_RECURSIVE );
}
