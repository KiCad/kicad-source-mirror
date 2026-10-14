/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <jobs/job_export_pcb_specctra_dsn.h>
#include <jobs/job_registry.h>
#include <i18n_utility.h>


JOB_EXPORT_PCB_SPECCTRA_DSN::JOB_EXPORT_PCB_SPECCTRA_DSN() : JOB( "specctra_dsn", false )
{
}


wxString JOB_EXPORT_PCB_SPECCTRA_DSN::GetDefaultDescription() const
{
    return _( "Export Specctra DSN File" );
}


wxString JOB_EXPORT_PCB_SPECCTRA_DSN::GetSettingsDialogTitle() const
{
    return _( "Export Specctra DSN Job Settings" );
}


REGISTER_JOB( pcb_export_specctra_dsn, _HKI( "PCB: Export Specctra DSN" ), KIWAY::FACE_PCB,
              JOB_EXPORT_PCB_SPECCTRA_DSN );
