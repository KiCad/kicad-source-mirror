/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Julie Vairai <j.vairai@hexa-h.com>
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <layer_ids.h>
#include <string_utils.h>
#include <wx/crt.h>

#include <macros.h>
#include <wx/tokenzr.h>

#include <locale_io.h>


CLI::PCB_EXPORT_GENCAD_COMMAND::PCB_EXPORT_GENCAD_COMMAND() : PCB_EXPORT_BASE_COMMAND( "gencad" )
{
    m_argParser.add_description( UTF8STDSTR( _( "Generate Gencad from a list of layers" ) ) );

    addLayerArg( true );
    addDrawingSheetArg();
    addDefineArg();

    m_argParser.add_argument( "-f", ARG_FLIP_BOTTOM_PADS )
            .default_value( false )
            .help( UTF8STDSTR( _( "Flip bottom footprint padstacks" ) ) )
            .flag();

    m_argParser.add_argument( "", ARG_UNIQUE_PINS )
            .default_value( false )
            .help( UTF8STDSTR( _( "Generate unique pin names" ) ) )
            .flag();

    m_argParser.add_argument( "", ARG_UNIQUE_FOOTPRINTS )
            .default_value( false )
            .help( UTF8STDSTR( _(
                    "Generate a new shape for each footprint instance (do not reuse shapes)" ) ) )
            .flag();

    m_argParser.add_argument( "", ARG_USE_DRILL_ORIGIN )
            .default_value( false )
            .help( UTF8STDSTR( _( "Use drill/place file origin as origin" ) ) )
            .flag();

    m_argParser.add_argument( "", ARG_STORE_ORIGIN_COORD )
            .default_value( false )
            .help( UTF8STDSTR( _( "Save the origin coordinates in the file" ) ) )
            .flag();
}


int CLI::PCB_EXPORT_GENCAD_COMMAND::doPerform( KIWAY& aKiway )
{
    int baseExit = PCB_EXPORT_BASE_COMMAND::doPerform( aKiway );

    if( baseExit != EXIT_CODES::OK )
        return baseExit;

    std::unique_ptr<JOB_EXPORT_PCB_GENCAD> gencadJob( new JOB_EXPORT_PCB_GENCAD() );

    gencadJob->m_filename = m_argInput;
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

    LOCALE_IO dummy; // Switch to "C" locale
    int       exitCode = aKiway.ProcessJob( KIWAY::FACE_PCB, gencadJob.get() );

    return exitCode;
}
