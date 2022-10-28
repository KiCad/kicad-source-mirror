/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
#include "exit_codes.h"
#include "jobs/job_export_pcb_dxf.h"
#include <kiface_base.h>
#include <layer_ids.h>
#include <wx/crt.h>

#include <macros.h>
#include <wx/tokenzr.h>

#define ARG_LAYERS "--layers"
#define ARG_INCLUDE_REFDES "--include-refdes"
#define ARG_INCLUDE_VALUE "--include-value"
#define ARG_USE_CONTOURS "--use-contours"
#define ARG_OUTPUT_UNITS "--output-units"

CLI::EXPORT_PCB_DXF_COMMAND::EXPORT_PCB_DXF_COMMAND() : EXPORT_PCB_BASE_COMMAND( "dxf" )
{
    m_argParser.add_argument( "-l", ARG_LAYERS )
            .default_value( std::string() )
            .help( "comma separated list of untranslated layer names to include such as F.Cu,B.Cu" );

    m_argParser.add_argument( "-ird", ARG_INCLUDE_REFDES )
            .help( "Include the reference designator text" )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( "-iv", ARG_INCLUDE_VALUE )
            .help( "Include the value text" )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( "-uc", ARG_USE_CONTOURS )
            .help( "Plot graphic items using their contours" )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( "-ou", ARG_OUTPUT_UNITS )
            .default_value( std::string( "in" ) )
            .help( "output file name" );
}


int CLI::EXPORT_PCB_DXF_COMMAND::Perform( KIWAY& aKiway ) const
{
    std::unique_ptr<JOB_EXPORT_PCB_DXF> dxfJob( new JOB_EXPORT_PCB_DXF( true ) );

    dxfJob->m_filename = FROM_UTF8( m_argParser.get<std::string>( ARG_INPUT ).c_str() );
    dxfJob->m_outputFile = FROM_UTF8( m_argParser.get<std::string>( ARG_OUTPUT ).c_str() );

    if( !wxFile::Exists( dxfJob->m_filename ) )
    {
        wxFprintf( stderr, _( "Board file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    dxfJob->m_plotFootprintValues = m_argParser.get<bool>( ARG_INCLUDE_VALUE );
    dxfJob->m_plotRefDes = m_argParser.get<bool>( ARG_INCLUDE_VALUE );
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

    wxString layers = FROM_UTF8( m_argParser.get<std::string>( ARG_LAYERS ).c_str() );

    LSET layerMask = convertLayerStringList( layers );

    dxfJob->m_printMaskLayer = layerMask;

    int exitCode = aKiway.ProcessJob( KIWAY::FACE_PCB, dxfJob.get() );

    return exitCode;
}
