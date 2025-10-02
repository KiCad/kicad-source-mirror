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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "job_export_pcb_stats.h"

#include <jobs/job_registry.h>
#include <wildcards_and_files_ext.h>
#include <wx/filename.h>

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_EXPORT_PCB_STATS::UNITS, {
                                                                   { JOB_EXPORT_PCB_STATS::UNITS::INCH, "in" },
                                                                   { JOB_EXPORT_PCB_STATS::UNITS::MM, "mm" },
                                                           } )

JOB_EXPORT_PCB_STATS::JOB_EXPORT_PCB_STATS() :
        JOB( "stats", false ),
        m_filename(),
        m_format( OUTPUT_FORMAT::REPORT ),
        m_units( UNITS::MM ),
        m_excludeFootprintsWithoutPads( false ),
        m_subtractHolesFromBoardArea( false ),
        m_subtractHolesFromCopperAreas( false )
{
    m_params.emplace_back( new JOB_PARAM<OUTPUT_FORMAT>( "output_format", &m_format, m_format ) );
    m_params.emplace_back( new JOB_PARAM<UNITS>( "units", &m_units, m_units ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "exclude_footprints_without_pads", &m_excludeFootprintsWithoutPads,
                                                m_excludeFootprintsWithoutPads ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "subtract_holes_from_board", &m_subtractHolesFromBoardArea,
                                                m_subtractHolesFromBoardArea ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "subtract_holes_from_copper", &m_subtractHolesFromCopperAreas,
                                                m_subtractHolesFromCopperAreas ) );
}


wxString JOB_EXPORT_PCB_STATS::GetDefaultDescription() const
{
    return _( "Export board statistics" );
}


wxString JOB_EXPORT_PCB_STATS::GetSettingsDialogTitle() const
{
    return _( "Export Board Statistics Job Settings" );
}


void JOB_EXPORT_PCB_STATS::SetDefaultOutputPath( const wxString& aReferenceName )
{
    wxFileName fn( aReferenceName );

    // We have other report file types (ERC/DRC) and JSON outputs so make sure we default
    // to something that won't overwrite them.
    fn.SetName( fn.GetName() + wxS( "_statistics" ) );

    if( m_format == OUTPUT_FORMAT::JSON )
        fn.SetExt( FILEEXT::JsonFileExtension );
    else
        fn.SetExt( FILEEXT::ReportFileExtension );

    SetWorkingOutputPath( fn.GetFullName() );
}


REGISTER_JOB( pcb_export_stats, _( "PCB: Export Statistics" ), KIWAY::FACE_PCB, JOB_EXPORT_PCB_STATS );
