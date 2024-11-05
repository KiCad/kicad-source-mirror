/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <jobs/job_export_sch_pythonbom.h>
#include <jobs/job_registry.h>
#include <i18n_utility.h>


JOB_EXPORT_SCH_PYTHONBOM::JOB_EXPORT_SCH_PYTHONBOM() :
        JOB( "pythonbom", false ),
        m_filename()
{
}


wxString JOB_EXPORT_SCH_PYTHONBOM::GetDescription()
{
    return wxString::Format( _( "Schematic PythonBOM export" ) );
}


REGISTER_JOB( sch_export_pythonbom, _HKI( "Schematic: Export PythonBOM" ), KIWAY::FACE_SCH,
              JOB_EXPORT_SCH_PYTHONBOM );