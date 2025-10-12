/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Julie Vairai <j.vairai@hexa-h.com>
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

#include "command_pcb_export_gencad.h"
#include <cli/exit_codes.h>
#include "jobs/job_export_pcb_gencad.h"
#include <kiface_base.h>
#include <string_utils.h>
#include <wx/crt.h>


CLI::PCB_EXPORT_GENCAD_COMMAND::PCB_EXPORT_GENCAD_COMMAND() :
        PCB_EXPORT_BASE_COMMAND( "gencad", false, true )
{
    m_argParser.add_description( UTF8STDSTR( _( "Export the PCB in Gencad format" ) ) );

    addDefineArg();

    m_argParser.add_argument( "-f", ARG_FLIP_BOTTOM_PADS )
            .help( UTF8STDSTR( _( "Flip bottom footprint padstacks" ) ) )
            .flag();

    m_argParser.add_argument( "", ARG_UNIQUE_PINS )
            .help( UTF8STDSTR( _( "Generate unique pin names" ) ) )
            .flag();

    m_argParser.add_argument( "", ARG_UNIQUE_FOOTPRINTS )
            .help( UTF8STDSTR( _( "Generate a new shape for each footprint instance (do not reuse shapes)" ) ) )
            .flag();

    m_argParser.add_argument( "", ARG_USE_DRILL_ORIGIN )
            .help( UTF8STDSTR( _( "Use drill/place file origin as origin" ) ) )
            .flag();

    m_argParser.add_argument( "", ARG_STORE_ORIGIN_COORD )
            .help( UTF8STDSTR( _( "Save the origin coordinates in the file" ) ) )
            .flag();
}


int CLI::PCB_EXPORT_GENCAD_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_EXPORT_PCB_GENCAD> gencadJob( new JOB_EXPORT_PCB_GENCAD() );

    gencadJob->m_filename = m_argInput;
    gencadJob->SetConfiguredOutputPath( m_argOutput );
    gencadJob->SetVarOverrides( m_argDefineVars );

    gencadJob->m_flipBottomPads = m_argParser.get<bool>( ARG_FLIP_BOTTOM_PADS );
    gencadJob->m_useUniquePins = m_argParser.get<bool>( ARG_UNIQUE_PINS );
    gencadJob->m_useIndividualShapes = m_argParser.get<bool>( ARG_UNIQUE_FOOTPRINTS );
    gencadJob->m_useDrillOrigin = m_argParser.get<bool>( ARG_USE_DRILL_ORIGIN );
    gencadJob->m_storeOriginCoords = m_argParser.get<bool>( ARG_STORE_ORIGIN_COORD );

    if( !wxFile::Exists( gencadJob->m_filename ) )
    {
        wxFprintf( stderr, _( "Board file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    return aKiway.ProcessJob( KIWAY::FACE_PCB, gencadJob.get() );
}
