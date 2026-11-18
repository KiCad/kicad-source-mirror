/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 Krishna Swaroop <krishna.swaroop@pixxel.co.in>
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <jobs/job_export_pcb_hyperlynx.h>
#include <jobs/job_registry.h>
#include <i18n_utility.h>
#include <wildcards_and_files_ext.h>


JOB_EXPORT_PCB_HYPERLYNX::JOB_EXPORT_PCB_HYPERLYNX() :
        JOB( "hyperlynx", false ),
        m_filename()
{
}


wxString JOB_EXPORT_PCB_HYPERLYNX::GetDefaultDescription() const
{
    return _( "Export Hyperlynx" );
}


wxString JOB_EXPORT_PCB_HYPERLYNX::GetSettingsDialogTitle() const
{
    return _( "Export Hyperlynx Job Settings" );
}


void JOB_EXPORT_PCB_HYPERLYNX::SetDefaultOutputPath( const wxString& aReferenceName )
{
    wxFileName fn = aReferenceName;

    fn.SetExt( FILEEXT::HyperlynxFileExtension );

    SetConfiguredOutputPath( fn.GetFullName() );
}


REGISTER_JOB( pcb_export_hyperlynx, _HKI( "PCB: Export Hyperlynx" ), KIWAY::FACE_PCB, JOB_EXPORT_PCB_HYPERLYNX );
