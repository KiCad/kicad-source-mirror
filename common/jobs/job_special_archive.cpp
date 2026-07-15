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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <jobs/job_special_archive.h>
#include <jobs/job_registry.h>
#include <i18n_utility.h>

JOB_SPECIAL_ARCHIVE::JOB_SPECIAL_ARCHIVE() :
        JOB( "special_archive", false ),
        m_includeExtraFiles( true )
{
    m_params.emplace_back( new JOB_PARAM<bool>( "include_extra_files", &m_includeExtraFiles, m_includeExtraFiles ) );
}


wxString JOB_SPECIAL_ARCHIVE::GetDefaultDescription() const
{
    return wxString( "Archive project" );
}


REGISTER_JOB( special_archive, _HKI( "Special: Archive Project" ), KIWAY::KIWAY_FACE_COUNT, JOB_SPECIAL_ARCHIVE );
