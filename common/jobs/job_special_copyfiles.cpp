/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Mark Roszko <mark.roszko@gmail.com>
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

#include <jobs/job_special_copyfiles.h>
#include <jobs/job_registry.h>
#include <i18n_utility.h>

JOB_SPECIAL_COPYFILES::JOB_SPECIAL_COPYFILES() :
        JOB( "special_copyfiles", false ),
        m_source(),
        m_dest(),
        m_generateErrorOnNoCopy( false ),
        m_overwriteDest( true )
{
    m_params.emplace_back( new JOB_PARAM<wxString>( "source", &m_source, m_source ) );
    m_params.emplace_back( new JOB_PARAM<wxString>( "dest", &m_dest, m_dest ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "zero_copies_error", &m_generateErrorOnNoCopy,
                                                m_generateErrorOnNoCopy ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "overwrite", &m_overwriteDest, m_overwriteDest ) );
}


wxString JOB_SPECIAL_COPYFILES::GetDefaultDescription() const
{
    return wxString( "Copy files" );
}


REGISTER_JOB( special_copyfiles, _HKI( "Special: Copy Files" ), KIWAY::KIWAY_FACE_COUNT,
              JOB_SPECIAL_COPYFILES );