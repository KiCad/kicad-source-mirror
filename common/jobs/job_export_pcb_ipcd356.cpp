/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2025 Connor Goss <connor.goss@acroname.com>
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

#include <jobs/job_export_pcb_ipcd356.h>
#include <jobs/job_registry.h>
#include <i18n_utility.h>
#include <wildcards_and_files_ext.h>

JOB_EXPORT_PCB_IPCD356::JOB_EXPORT_PCB_IPCD356() :
        JOB( "ipcd356", false ),
        m_filename()
{
}


wxString JOB_EXPORT_PCB_IPCD356::GetDefaultDescription() const
{
    return _( "Export IPC-D-356" );
}


wxString JOB_EXPORT_PCB_IPCD356::GetSettingsDialogTitle() const
{
    return _( "Export IPC-D-356 Job Settings" );
}


void JOB_EXPORT_PCB_IPCD356::SetDefaultOutputPath( const wxString& aReferenceName )
{
    wxFileName fn = aReferenceName;

    fn.SetExt( FILEEXT::IpcD356FileExtension );

    SetConfiguredOutputPath( fn.GetFullName() );
}


REGISTER_JOB( pcb_export_ipcd356, _HKI( "PCB: Export IPC-D-356" ), KIWAY::FACE_PCB,
              JOB_EXPORT_PCB_IPCD356 );
