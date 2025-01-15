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

#include "command_sch_export_pythonbom.h"
#include <cli/exit_codes.h>
#include "jobs/job_export_sch_pythonbom.h"
#include <kiface_base.h>
#include <layer_ids.h>
#include <wx/crt.h>

#include <macros.h>


CLI::SCH_EXPORT_PYTHONBOM_COMMAND::SCH_EXPORT_PYTHONBOM_COMMAND() :
        COMMAND( "python-bom" )
{
    addCommonArgs( true, true, false, false );

    m_argParser.add_description( UTF8STDSTR( _( "Export the legacy BOM XML format used in the "
                                                "schematic editor with Python scripts" ) ) );
}


int CLI::SCH_EXPORT_PYTHONBOM_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_EXPORT_SCH_PYTHONBOM> bomJob =
            std::make_unique<JOB_EXPORT_SCH_PYTHONBOM>();

    bomJob->m_filename = m_argInput;
    bomJob->SetConfiguredOutputPath( m_argOutput );

    if( !wxFile::Exists( bomJob->m_filename ) )
    {
        wxFprintf( stderr, _( "Schematic file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }


    int exitCode = aKiway.ProcessJob( KIWAY::FACE_SCH, bomJob.get() );

    return exitCode;
}
