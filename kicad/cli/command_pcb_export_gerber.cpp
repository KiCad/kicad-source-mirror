/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "command_pcb_export_gerber.h"
#include <cli/exit_codes.h>
#include "jobs/job_export_pcb_gerber.h"
#include <kiface_base.h>
#include <layer_ids.h>
#include <wx/crt.h>

#include <macros.h>
#include <wx/tokenzr.h>

#include <locale_io.h>



CLI::PCB_EXPORT_GERBER_COMMAND::PCB_EXPORT_GERBER_COMMAND( const std::string& aName ) :
        PCB_EXPORT_BASE_COMMAND( aName )
{
    addLayerArg( true );
    addDrawingSheetArg();
    addDefineArg();

    m_argParser.add_description( UTF8STDSTR( _( "Plot given layers to a single Gerber file" ) ) );

    m_argParser.add_argument( "--erd", ARG_EXCLUDE_REFDES )
            .help( UTF8STDSTR( _( "Exclude the reference designator text" ) ) )
            .flag();

    m_argParser.add_argument( "--ev", ARG_EXCLUDE_VALUE )
            .help( UTF8STDSTR( _( "Exclude the value text" ) ) )
            .flag();

    m_argParser.add_argument( "--ibt", ARG_INCLUDE_BORDER_TITLE )
            .help( UTF8STDSTR( _( "Include the border and title block" ) ) )
            .flag();

    m_argParser.add_argument( ARG_NO_X2 )
            .help( UTF8STDSTR( _( "Do not use the extended X2 format" ) ) )
            .flag();

    m_argParser.add_argument( ARG_NO_NETLIST )
            .help( UTF8STDSTR( _( "Do not generate netlist attributes" ) ) )
            .flag();

    m_argParser.add_argument( ARG_SUBTRACT_SOLDERMASK )
            .help( UTF8STDSTR( _( "Subtract soldermask from silkscreen" ) ) )
            .flag();

    m_argParser.add_argument( ARG_DISABLE_APERTURE_MACROS )
            .help( UTF8STDSTR( _( "Disable aperture macros" ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( ARG_USE_DRILL_FILE_ORIGIN )
            .help( UTF8STDSTR( _( "Use drill/place file origin" ) ) )
            .flag();

    m_argParser.add_argument( ARG_PRECISION )
            .help( UTF8STDSTR( _( "Precision of Gerber coordinates, valid options: 5 or 6" ) ) )
            .scan<'i', int>()
            .default_value( 6 )
            .metavar( "PRECISION" );

    m_argParser.add_argument( ARG_NO_PROTEL_EXTENSION )
            .help( UTF8STDSTR( _( "Use KiCad Gerber file extension" ) ) )
            .flag();
}


CLI::PCB_EXPORT_GERBER_COMMAND::PCB_EXPORT_GERBER_COMMAND() : PCB_EXPORT_GERBER_COMMAND( "gerber" )
{
}


int CLI::PCB_EXPORT_GERBER_COMMAND::populateJob( JOB_EXPORT_PCB_GERBER* aJob )
{
    aJob->m_filename = m_argInput;
    aJob->SetConfiguredOutputPath( m_argOutput );
    aJob->m_drawingSheet = m_argDrawingSheet;
    aJob->SetVarOverrides( m_argDefineVars );

    aJob->m_plotFootprintValues = !m_argParser.get<bool>( ARG_EXCLUDE_VALUE );
    aJob->m_plotRefDes = !m_argParser.get<bool>( ARG_EXCLUDE_REFDES );
    aJob->m_plotDrawingSheet = m_argParser.get<bool>( ARG_INCLUDE_BORDER_TITLE );
    aJob->m_disableApertureMacros = m_argParser.get<bool>( ARG_DISABLE_APERTURE_MACROS );
    aJob->m_subtractSolderMaskFromSilk = m_argParser.get<bool>( ARG_SUBTRACT_SOLDERMASK );
    aJob->m_includeNetlistAttributes = !m_argParser.get<bool>( ARG_NO_NETLIST );
    aJob->m_useX2Format = !m_argParser.get<bool>( ARG_NO_X2 );
    aJob->m_useDrillOrigin = m_argParser.get<bool>( ARG_USE_DRILL_FILE_ORIGIN );
    aJob->m_useProtelFileExtension = !m_argParser.get<bool>( ARG_NO_PROTEL_EXTENSION );
    aJob->m_precision = m_argParser.get<int>( ARG_PRECISION );
    aJob->m_printMaskLayer = m_selectedLayers;

    if( !wxFile::Exists( aJob->m_filename ) )
    {
        wxFprintf( stderr, _( "Board file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    if( aJob->m_precision != 5 && aJob->m_precision != 6 )
    {
        wxFprintf( stderr, _( "Gerber coordinate precision should be either 5 or 6\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    return EXIT_CODES::OK;
}


int CLI::PCB_EXPORT_GERBER_COMMAND::doPerform( KIWAY& aKiway )
{
    wxFprintf( stdout, wxT( "\033[33;1m%s\033[0m\n" ),
               _( "This command is deprecated as of KiCad 9.0, please use \"gerbers\" instead\n" ) );

    int exitCode = PCB_EXPORT_BASE_COMMAND::doPerform( aKiway );

    if( exitCode != EXIT_CODES::OK )
        return exitCode;

    std::unique_ptr<JOB_EXPORT_PCB_GERBER> gerberJob( new JOB_EXPORT_PCB_GERBER() );

    exitCode = populateJob( gerberJob.get() );

    if( exitCode != EXIT_CODES::OK )
        return exitCode;

    LOCALE_IO dummy;
    exitCode = aKiway.ProcessJob( KIWAY::FACE_PCB, gerberJob.get() );

    return exitCode;
}
