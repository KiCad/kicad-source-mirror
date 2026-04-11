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

#include <jobs/job_registry.h>
#include <jobs/job_gerber_diff.h>
#include <i18n_utility.h>


NLOHMANN_JSON_SERIALIZE_ENUM( JOB_GERBER_DIFF::OUTPUT_FORMAT, { { JOB_GERBER_DIFF::OUTPUT_FORMAT::PNG, "png" },
                                                                { JOB_GERBER_DIFF::OUTPUT_FORMAT::TEXT, "text" },
                                                                { JOB_GERBER_DIFF::OUTPUT_FORMAT::JSON, "json" } } )


JOB_GERBER_DIFF::JOB_GERBER_DIFF() :
        JOB( "gerber_diff", false )
{
    m_params.emplace_back( new JOB_PARAM<OUTPUT_FORMAT>( "output_format", &m_outputFormat, m_outputFormat ) );
    m_params.emplace_back( new JOB_PARAM<int>( "dpi", &m_dpi, m_dpi ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "antialias", &m_antialias, m_antialias ) );
    m_params.emplace_back(
            new JOB_PARAM<bool>( "transparent_background", &m_transparentBackground, m_transparentBackground ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "exit_code_only", &m_exitCodeOnly, m_exitCodeOnly ) );
    m_params.emplace_back( new JOB_PARAM<int>( "tolerance", &m_tolerance, m_tolerance ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "no_align", &m_noAlign, m_noAlign ) );
}


wxString JOB_GERBER_DIFF::GetDefaultDescription() const
{
    return _( "Gerber Diff" );
}


wxString JOB_GERBER_DIFF::GetSettingsDialogTitle() const
{
    return _( "Gerber Diff Job Settings" );
}


REGISTER_JOB( gerber_diff, _HKI( "Gerber: Diff" ), KIWAY::FACE_GERBVIEW, JOB_GERBER_DIFF );
