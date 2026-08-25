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

#include <base_units.h>
#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <jobs/job_export_pcb_pos.h>
#include <pad.h>
#include <exporters/gerber_placefile_writer.h>
#include <pcbnew_utils/board_test_utils.h>

#include <wx/ffile.h>
#include <wx/filename.h>

// Regression test for https://gitlab.com/kicad/code/kicad/-/work_items/25010
//
// The position file dialog greyed out and force-cleared the DNP and "exclude from BOM" filters
// whenever the Gerber X3 format was selected, so those footprints always landed in the exported
// file even though PLACEFILE_GERBER_WRITER honours both flags.

BOOST_AUTO_TEST_SUITE( Issue25010 )


static FOOTPRINT* addFootprint( BOARD& aBoard, const wxString& aReference )
{
    FOOTPRINT* fp = new FOOTPRINT( &aBoard );

    fp->SetReference( aReference );
    fp->SetValue( wxS( "10K" ) );
    fp->SetLayer( F_Cu );
    fp->SetAttributes( FP_SMD );

    PAD* pad = new PAD( fp );
    pad->SetNumber( wxS( "1" ) );
    pad->SetPadstackMode( PADSTACK::MODE::NORMAL );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
    pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) ) );
    pad->SetLayerSet( LSET( { F_Cu } ) );
    fp->Add( pad );

    aBoard.Add( fp, ADD_MODE::INSERT );

    return fp;
}


/**
 * The format capability table drives the dialog's checkbox enable state.  Gerber X3 cannot
 * filter on pad technology, but it can drop DNP and BOM-excluded footprints.
 */
BOOST_AUTO_TEST_CASE( GerberFormatSupportsPopulationFilters )
{
    using FORMAT = JOB_EXPORT_PCB_POS::FORMAT;
    using FILTER = JOB_EXPORT_PCB_POS::FILTER;

    BOOST_CHECK( JOB_EXPORT_PCB_POS::FormatSupportsFilter( FORMAT::GERBER, FILTER::EXCLUDE_DNP ) );
    BOOST_CHECK( JOB_EXPORT_PCB_POS::FormatSupportsFilter( FORMAT::GERBER, FILTER::EXCLUDE_BOM ) );

    BOOST_CHECK( !JOB_EXPORT_PCB_POS::FormatSupportsFilter( FORMAT::GERBER, FILTER::SMD_ONLY ) );
    BOOST_CHECK( !JOB_EXPORT_PCB_POS::FormatSupportsFilter( FORMAT::GERBER, FILTER::EXCLUDE_TH ) );

    for( FORMAT format : { FORMAT::ASCII, FORMAT::CSV } )
    {
        for( FILTER filter : { FILTER::SMD_ONLY, FILTER::EXCLUDE_TH, FILTER::EXCLUDE_DNP,
                               FILTER::EXCLUDE_BOM } )
        {
            BOOST_CHECK( JOB_EXPORT_PCB_POS::FormatSupportsFilter( format, filter ) );
        }
    }
}


/**
 * End to end check that the Gerber X3 writer drops the footprints the dialog now lets the user
 * exclude.
 */
BOOST_AUTO_TEST_CASE( GerberPlaceFileHonorsExclusions )
{
    BOARD board;

    addFootprint( board, wxS( "R1" ) );
    addFootprint( board, wxS( "R2" ) )->SetDNP( true );
    addFootprint( board, wxS( "R3" ) )->SetExcludedFromBOM( true );
    addFootprint( board, wxS( "R4" ) )->SetExcludedFromPosFiles( true );

    KI_TEST::TEMPORARY_DIRECTORY tmpDir( "kicad_qa_issue25010", "" );

    wxFileName tmpFile( tmpDir.GetPath().string(), wxS( "pnp.gbr" ) );

    PLACEFILE_GERBER_WRITER writer( &board );

    auto generate =
            [&]( bool aExcludeDNP, bool aExcludeBOM ) -> wxString
            {
                int count = writer.CreatePlaceFile( tmpFile.GetFullPath(), F_Cu, false, aExcludeDNP,
                                                    aExcludeBOM );

                BOOST_REQUIRE_GE( count, 0 );

                wxFFile  file( tmpFile.GetFullPath() );
                wxString contents;

                BOOST_REQUIRE( file.ReadAll( &contents ) );

                return contents;
            };

    wxString unfiltered = generate( false, false );

    BOOST_CHECK( unfiltered.Contains( wxS( "R1" ) ) );
    BOOST_CHECK( unfiltered.Contains( wxS( "R2" ) ) );
    BOOST_CHECK( unfiltered.Contains( wxS( "R3" ) ) );
    BOOST_CHECK( !unfiltered.Contains( wxS( "R4" ) ) );

    wxString filtered = generate( true, true );

    BOOST_CHECK( filtered.Contains( wxS( "R1" ) ) );
    BOOST_CHECK( !filtered.Contains( wxS( "R2" ) ) );
    BOOST_CHECK( !filtered.Contains( wxS( "R3" ) ) );
    BOOST_CHECK( !filtered.Contains( wxS( "R4" ) ) );
}


BOOST_AUTO_TEST_SUITE_END()
