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
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <jobs/job_diff_base.h>


NLOHMANN_JSON_SERIALIZE_ENUM( JOB_DIFF_BASE::OUTPUT_FORMAT,
                              {
                                    { JOB_DIFF_BASE::OUTPUT_FORMAT::JSON, "json" },
                                    { JOB_DIFF_BASE::OUTPUT_FORMAT::TEXT, "text" },
                                    { JOB_DIFF_BASE::OUTPUT_FORMAT::PNG,  "png" },
                                    { JOB_DIFF_BASE::OUTPUT_FORMAT::SVG,  "svg" },
                              } )


JOB_DIFF_BASE::JOB_DIFF_BASE( const std::string& aType ) :
        JOB( aType, false )
{
    m_params.emplace_back( new JOB_PARAM<wxString>( "input_a", &m_inputA, m_inputA ) );
    m_params.emplace_back( new JOB_PARAM<wxString>( "input_b", &m_inputB, m_inputB ) );
    m_params.emplace_back( new JOB_PARAM<OUTPUT_FORMAT>( "format", &m_format, m_format ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "exit_code_only",
                                                &m_exitCodeOnly, m_exitCodeOnly ) );
}
