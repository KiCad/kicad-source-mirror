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

#include "command_pcb_export_gerbers.h"
#include <cli/exit_codes.h>
#include "jobs/job_export_pcb_gerbers.h"
#include <kiface_base.h>
#include <string_utils.h>

#define ARG_USE_BOARD_PLOT_PARAMS "--board-plot-params"


CLI::PCB_EXPORT_GERBERS_COMMAND::PCB_EXPORT_GERBERS_COMMAND() :
        PCB_EXPORT_GERBER_COMMAND( "gerbers", true )
{
    m_argParser.add_description( UTF8STDSTR( _( "Plot multiple Gerbers for a PCB, including the "
                                                "ability to use stored board plot settings" ) ) );

    m_argParser.add_argument( ARG_USE_BOARD_PLOT_PARAMS )
            .help( UTF8STDSTR( _( "Use the Gerber plot settings already configured in the "
                                  "board file" ) ) )
            .flag();
}


int CLI::PCB_EXPORT_GERBERS_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_EXPORT_PCB_GERBERS> gerberJob( new JOB_EXPORT_PCB_GERBERS() );

    int exitCode = populateJob( gerberJob.get() );

    if( exitCode != EXIT_CODES::OK )
        return exitCode;

    gerberJob->m_argLayers = From_UTF8( m_argParser.get<std::string>( ARG_LAYERS ).c_str() );
    gerberJob->m_argCommonLayers = From_UTF8( m_argParser.get<std::string>( ARG_COMMON_LAYERS ).c_str() );
    gerberJob->m_useBoardPlotParams = m_argParser.get<bool>( ARG_USE_BOARD_PLOT_PARAMS );

    return aKiway.ProcessJob( KIWAY::FACE_PCB, gerberJob.get() );
}
