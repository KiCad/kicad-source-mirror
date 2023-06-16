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

#include "command_export_pcb_gerbers.h"
#include <cli/exit_codes.h>
#include "jobs/job_export_pcb_gerbers.h"
#include <kiface_base.h>
#include <layer_ids.h>
#include <wx/crt.h>

#include <macros.h>
#include <wx/tokenzr.h>

#include <locale_io.h>

#define ARG_COMMON_LAYERS "--common-layers"
#define ARG_USE_BOARD_PLOT_PARAMS "--board-plot-params"


CLI::EXPORT_PCB_GERBERS_COMMAND::EXPORT_PCB_GERBERS_COMMAND() :
        EXPORT_PCB_GERBER_COMMAND( "gerbers" )
{
    m_requireLayers = false;

    m_argParser.add_argument( "--cl", ARG_COMMON_LAYERS )
            .default_value( std::string() )
            .help( UTF8STDSTR(
                    _( "Layers to include on each plot, comma separated list of untranslated layer names to include such as "
                       "F.Cu,B.Cu" ) ) );

    m_argParser.add_argument( ARG_USE_BOARD_PLOT_PARAMS )
            .help( UTF8STDSTR( _( "Use the gerber plot settings already configured in the board file" ) ) )
            .implicit_value( true )
            .default_value( false );
}


int CLI::EXPORT_PCB_GERBERS_COMMAND::doPerform( KIWAY& aKiway )
{
    int exitCode = EXPORT_PCB_BASE_COMMAND::doPerform( aKiway );

    if( exitCode != EXIT_CODES::OK )
        return exitCode;

    std::unique_ptr<JOB_EXPORT_PCB_GERBERS> gerberJob( new JOB_EXPORT_PCB_GERBERS( true ) );

    exitCode = populateJob( gerberJob.get() );

    if( exitCode != EXIT_CODES::OK )
        return exitCode;

    wxString layers = FROM_UTF8( m_argParser.get<std::string>( ARG_COMMON_LAYERS ).c_str() );
    gerberJob->m_layersIncludeOnAll =
            convertLayerStringList( layers, gerberJob->m_layersIncludeOnAllSet );
    gerberJob->m_useBoardPlotParams = m_argParser.get<bool>( ARG_USE_BOARD_PLOT_PARAMS );

    LOCALE_IO dummy;
    exitCode = aKiway.ProcessJob( KIWAY::FACE_PCB, gerberJob.get() );

    return exitCode;
}
