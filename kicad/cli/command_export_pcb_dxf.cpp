/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "command_export_pcb_dxf.h"
#include <cli/exit_codes.h>
#include "jobs/job_export_pcb_dxf.h"
#include <kiface_base.h>
#include <layer_ids.h>
#include <wx/crt.h>

#include <macros.h>
#include <wx/tokenzr.h>

#include <locale_io.h>

#define ARG_USE_CONTOURS "--use-contours"
#define ARG_OUTPUT_UNITS "--output-units"

CLI::EXPORT_PCB_DXF_COMMAND::EXPORT_PCB_DXF_COMMAND() : EXPORT_PCB_BASE_COMMAND( "dxf" )
{
    addLayerArg( true );

    m_argParser.add_argument( "--erd", ARG_EXCLUDE_REFDES )
            .help( UTF8STDSTR( _( "Exclude the reference designator text" ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( "--ev", ARG_EXCLUDE_VALUE )
            .help( UTF8STDSTR( _( "Exclude the value text" ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( "--uc", ARG_USE_CONTOURS )
            .help( UTF8STDSTR( _( "Plot graphic items using their contours" ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( "--ou", ARG_OUTPUT_UNITS )
            .default_value( std::string( "in" ) )
            .help( UTF8STDSTR( _( "Output units, valid options: mm, in" ) ) );
}


int CLI::EXPORT_PCB_DXF_COMMAND::doPerform( KIWAY& aKiway )
{
    int baseExit = EXPORT_PCB_BASE_COMMAND::doPerform( aKiway );
    if( baseExit != EXIT_CODES::OK )
        return baseExit;

    std::unique_ptr<JOB_EXPORT_PCB_DXF> dxfJob( new JOB_EXPORT_PCB_DXF( true ) );

    dxfJob->m_filename = FROM_UTF8( m_argParser.get<std::string>( ARG_INPUT ).c_str() );
    dxfJob->m_outputFile = FROM_UTF8( m_argParser.get<std::string>( ARG_OUTPUT ).c_str() );

    if( !wxFile::Exists( dxfJob->m_filename ) )
    {
        wxFprintf( stderr, _( "Board file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    dxfJob->m_plotFootprintValues = m_argParser.get<bool>( ARG_EXCLUDE_VALUE );
    dxfJob->m_plotRefDes = !m_argParser.get<bool>( ARG_EXCLUDE_REFDES );
    dxfJob->m_plotGraphicItemsUsingContours = m_argParser.get<bool>( ARG_USE_CONTOURS );

    wxString units = FROM_UTF8( m_argParser.get<std::string>( ARG_OUTPUT_UNITS ).c_str() );

    if( units == wxS( "mm" ) )
    {
        dxfJob->m_dxfUnits = JOB_EXPORT_PCB_DXF::DXF_UNITS::MILLIMETERS;
    }
    else if( units == wxS( "in" ) )
    {
        dxfJob->m_dxfUnits = JOB_EXPORT_PCB_DXF::DXF_UNITS::INCHES;
    }
    else
    {
        wxFprintf( stderr, _( "Invalid units specified\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    dxfJob->m_printMaskLayer = m_selectedLayers;

    LOCALE_IO dummy;    // Switch to "C" locale
    int exitCode = aKiway.ProcessJob( KIWAY::FACE_PCB, dxfJob.get() );

    return exitCode;
}
