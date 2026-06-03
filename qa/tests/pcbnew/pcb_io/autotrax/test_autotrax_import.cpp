/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file test_autotrax_import.cpp
 * Test suite for import of Protel Autotrax / Easytrax (.PCB) layout files.
 *
 * Sample files are real, openly-available boards from the PRONOM file-format
 * research corpus (glepore70/pronom-research, sample_files/p/pcb2), which the
 * UK National Archives published as reference samples for the Autotrax/Easytrax
 * format. Every file carries the "PCB FILE 4" Autotrax header.
 */

#include <pcbnew_utils/board_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcbnew/pcb_io/autotrax/pcb_io_autotrax.h>

#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <pcb_shape.h>

#include <wx/filename.h>


struct AUTOTRAX_IMPORT_FIXTURE
{
    AUTOTRAX_IMPORT_FIXTURE() {}

    PCB_IO_AUTOTRAX m_plugin;

    std::string path( const std::string& aName )
    {
        return KI_TEST::GetPcbnewTestDataDir() + "plugins/autotrax/" + aName;
    }

    bool haveSample( const std::string& aName ) { return wxFileName::FileExists( path( aName ) ); }
};


BOOST_FIXTURE_TEST_SUITE( AutotraxImport, AUTOTRAX_IMPORT_FIXTURE )


/// CanReadBoard must accept a real Autotrax file by sniffing the magic header,
/// not just the (gEDA-shared) .PCB extension.
BOOST_AUTO_TEST_CASE( SniffRecognizesPcb )
{
    if( !haveSample( "PRJ.PCB" ) )
    {
        BOOST_TEST_MESSAGE( "no real autotrax sample available; load test skipped" );
        return;
    }

    BOOST_CHECK( m_plugin.CanReadBoard( path( "PRJ.PCB" ) ) );
}


/// The simplest sample is a board of through-hole components: it must yield
/// footprints with pads and some free tracks.
BOOST_AUTO_TEST_CASE( SimpleBoardStructure )
{
    if( !haveSample( "PRJ.PCB" ) )
    {
        BOOST_TEST_MESSAGE( "no real autotrax sample available; load test skipped" );
        return;
    }

    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    BOOST_REQUIRE_NO_THROW( m_plugin.LoadBoard( path( "PRJ.PCB" ), board.get(), nullptr ) );
    BOOST_REQUIRE( board );

    BOOST_CHECK_GT( board->Footprints().size(), 0 );

    int pads = 0;

    for( FOOTPRINT* fp : board->Footprints() )
        pads += fp->Pads().size();

    BOOST_CHECK_GT( pads, 0 );
    BOOST_CHECK_GT( board->Tracks().size(), 0 );
}


/// The richest sample exercises every record family (FT/FA/FV/FP/FS plus
/// component CT/CA/CV/CP). Verify the importer produces a non-trivial board
/// with copper tracks, vias and footprints.
BOOST_AUTO_TEST_CASE( RichBoardStructure )
{
    if( !haveSample( "PRJ12.PCB" ) )
    {
        BOOST_TEST_MESSAGE( "no real autotrax sample available; load test skipped" );
        return;
    }

    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    BOOST_REQUIRE_NO_THROW( m_plugin.LoadBoard( path( "PRJ12.PCB" ), board.get(), nullptr ) );
    BOOST_REQUIRE( board );

    int tracks = 0;
    int vias = 0;

    for( PCB_TRACK* t : board->Tracks() )
    {
        if( t->Type() == PCB_VIA_T )
            vias++;
        else if( t->Type() == PCB_TRACE_T )
            tracks++;
    }

    BOOST_CHECK_GT( tracks, 0 );
    BOOST_CHECK_GT( vias, 0 );
    BOOST_CHECK_GT( board->Footprints().size(), 0 );

    int pads = 0;

    for( FOOTPRINT* fp : board->Footprints() )
        pads += fp->Pads().size();

    BOOST_CHECK_GT( pads, 0 );
}


/// Every sample must load without throwing and produce a positive board extent;
/// none should crash the Y-axis flip or layer mapping.
BOOST_AUTO_TEST_CASE( AllSamplesLoad )
{
    const std::vector<std::string> samples = { "PRJ.PCB", "PRJ2.PCB", "PRJ10.PCB", "PRJ12.PCB" };

    bool any = false;

    for( const std::string& name : samples )
    {
        if( !haveSample( name ) )
            continue;

        any = true;

        std::unique_ptr<BOARD> board = std::make_unique<BOARD>();
        BOOST_REQUIRE_NO_THROW( m_plugin.LoadBoard( path( name ), board.get(), nullptr ) );
        BOOST_REQUIRE( board );

        size_t items = board->Tracks().size() + board->Footprints().size() + board->Drawings().size();
        BOOST_CHECK_GT( items, 0 );
    }

    if( !any )
        BOOST_TEST_MESSAGE( "no real autotrax sample available; load test skipped" );
}


BOOST_AUTO_TEST_SUITE_END()
