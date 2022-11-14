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

#include "command_export_pcb_gerber.h"
#include <cli/exit_codes.h>
#include "jobs/job_export_pcb_gerber.h"
#include <kiface_base.h>
#include <layer_ids.h>
#include <wx/crt.h>

#include <macros.h>
#include <wx/tokenzr.h>

#include <locale_io.h>

#define ARG_NO_X2 "--no-x2"
#define ARG_NO_NETLIST "--no-netlist"
#define ARG_SUBTRACT_SOLDERMASK "--subtract-soldermask"
#define ARG_DISABLE_APERTURE_MACROS "--disable-aperture-macros"
#define ARG_PRECISION "--precision"

CLI::EXPORT_PCB_GERBER_COMMAND::EXPORT_PCB_GERBER_COMMAND() : EXPORT_PCB_BASE_COMMAND( "gerber" )
{
    addLayerArg( true );

    m_argParser.add_argument( "-ird", ARG_INCLUDE_REFDES )
            .help( UTF8STDSTR( _( "Include the reference designator text" ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( "-iv", ARG_INCLUDE_VALUE )
            .help( UTF8STDSTR( _( "Include the value text" ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( "-ibt", ARG_INCLUDE_BORDER_TITLE )
            .help( UTF8STDSTR( _( "Include the border and title block" ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( ARG_NO_X2 )
            .help( UTF8STDSTR( _( "Do not use the extended X2 format" ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( ARG_NO_NETLIST )
            .help( UTF8STDSTR( _( "Do not generate netlist attributes" ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( ARG_SUBTRACT_SOLDERMASK )
            .help( UTF8STDSTR( _( "Subtract soldermask from silkscreen" ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( ARG_DISABLE_APERTURE_MACROS )
            .help( UTF8STDSTR( _( "Disable aperature macros" ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( ARG_PRECISION )
            .help( UTF8STDSTR(
                    _( "Precision of gerber coordinates, valid options: 5 or 6" ) ) )
            .default_value( 6 );
}


int CLI::EXPORT_PCB_GERBER_COMMAND::Perform( KIWAY& aKiway )
{
    int baseExit = EXPORT_PCB_BASE_COMMAND::Perform( aKiway );
    if( baseExit != EXIT_CODES::OK )
        return baseExit;

    std::unique_ptr<JOB_EXPORT_PCB_GERBER> gerberJob( new JOB_EXPORT_PCB_GERBER( true ) );

    gerberJob->m_filename = FROM_UTF8( m_argParser.get<std::string>( ARG_INPUT ).c_str() );
    gerberJob->m_outputFile = FROM_UTF8( m_argParser.get<std::string>( ARG_OUTPUT ).c_str() );

    if( !wxFile::Exists( gerberJob->m_filename ) )
    {
        wxFprintf( stderr, _( "Board file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    gerberJob->m_plotFootprintValues = m_argParser.get<bool>( ARG_INCLUDE_VALUE );
    gerberJob->m_plotRefDes = m_argParser.get<bool>( ARG_INCLUDE_VALUE );
    gerberJob->m_plotBorderTitleBlocks = m_argParser.get<bool>( ARG_INCLUDE_BORDER_TITLE );
    gerberJob->m_disableApertureMacros = m_argParser.get<bool>( ARG_DISABLE_APERTURE_MACROS );
    gerberJob->m_subtractSolderMaskFromSilk = m_argParser.get<bool>( ARG_SUBTRACT_SOLDERMASK );
    gerberJob->m_includeNetlistAttributes = !m_argParser.get<bool>( ARG_NO_NETLIST );
    gerberJob->m_useX2Format = !m_argParser.get<bool>( ARG_NO_X2 );
    gerberJob->m_precision = m_argParser.get<int>( ARG_PRECISION );

    if( gerberJob->m_precision != 5 && gerberJob->m_precision != 6 )
    {
        wxFprintf( stderr, _( "Gerber coordinate precision should be either 5 or 6\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    gerberJob->m_printMaskLayer = m_selectedLayers;

    LOCALE_IO dummy;
    int exitCode = aKiway.ProcessJob( KIWAY::FACE_PCB, gerberJob.get() );

    return exitCode;
}