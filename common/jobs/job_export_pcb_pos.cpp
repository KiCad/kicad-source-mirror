/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
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

#include <jobs/job_export_pcb_pos.h>
#include <jobs/job_registry.h>
#include <i18n_utility.h>
#include <wildcards_and_files_ext.h>

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_EXPORT_PCB_POS::FORMAT,
                              {
                                      { JOB_EXPORT_PCB_POS::FORMAT::ASCII, "ascii" },
                                      { JOB_EXPORT_PCB_POS::FORMAT::CSV, "csv" },
                                      { JOB_EXPORT_PCB_POS::FORMAT::GERBER, "gerber" },
                              } )

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_EXPORT_PCB_POS::SIDE,
                              {
                                      { JOB_EXPORT_PCB_POS::SIDE::FRONT, "front" },
                                      { JOB_EXPORT_PCB_POS::SIDE::BACK, "back" },
                                      { JOB_EXPORT_PCB_POS::SIDE::BOTH, "both" },
                              } )

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_EXPORT_PCB_POS::UNITS,
                              {
                                      { JOB_EXPORT_PCB_POS::UNITS::INCH, "in" },
                                      { JOB_EXPORT_PCB_POS::UNITS::MM, "mm" },
                              } )

JOB_EXPORT_PCB_POS::JOB_EXPORT_PCB_POS() :
        JOB( "pos", false ),
        m_filename(),
        m_useDrillPlaceFileOrigin( true ),
        m_smdOnly( false ),
        m_excludeFootprintsWithTh( false ),
        m_excludeDNP( false ),
        m_excludeBOM( false ),
        m_negateBottomX( false ),
        m_singleFile( false ),
        m_nakedFilename( false ),
        m_side( SIDE::BOTH ),
        m_units( UNITS::MM ),
        m_format( FORMAT::ASCII ),
        m_gerberBoardEdge( true )
{
    m_params.emplace_back( new JOB_PARAM<bool>( "use_drill_place_file_origin",
                                                &m_useDrillPlaceFileOrigin,
                                                m_useDrillPlaceFileOrigin ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "smd_only",
                                                &m_smdOnly,
                                                m_smdOnly ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "exclude_footprints_with_th",
                                                &m_excludeFootprintsWithTh,
                                                m_excludeFootprintsWithTh ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "exclude_dnp",
                                                &m_excludeDNP,
                                                m_excludeDNP ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "exclude_bom",
                                                &m_excludeBOM,
                                                m_excludeBOM ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "negate_bottom_x",
                                                &m_negateBottomX,
                                                m_negateBottomX ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "single_file",
                                                &m_singleFile,
                                                m_singleFile ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "gerber_board_edge",
                                                &m_gerberBoardEdge,
                                                m_gerberBoardEdge ) );

    m_params.emplace_back( new JOB_PARAM<SIDE>( "side", &m_side, m_side ) );
    m_params.emplace_back( new JOB_PARAM<UNITS>( "units", &m_units, m_units ) );
    m_params.emplace_back( new JOB_PARAM<FORMAT>( "format", &m_format, m_format ) );
}


wxString JOB_EXPORT_PCB_POS::GetDefaultDescription() const
{
    return _( "Export position data" );
}


wxString JOB_EXPORT_PCB_POS::GetSettingsDialogTitle() const
{
    return _( "Export Position Data Job Settings" );
}


void JOB_EXPORT_PCB_POS::SetDefaultOutputPath( const wxString& aReferenceName )
{
    wxFileName fn = aReferenceName;

    if( m_format == JOB_EXPORT_PCB_POS::FORMAT::ASCII )
        fn.SetExt( FILEEXT::FootprintPlaceFileExtension );
    else if( m_format == JOB_EXPORT_PCB_POS::FORMAT::CSV )
        fn.SetExt( FILEEXT::CsvFileExtension );
    else if( m_format == JOB_EXPORT_PCB_POS::FORMAT::GERBER )
        fn.SetExt( FILEEXT::GerberFileExtension );

    SetConfiguredOutputPath( fn.GetFullName() );
}

REGISTER_JOB( pcb_export_pos, _HKI( "PCB: Export Position Data" ), KIWAY::FACE_PCB, JOB_EXPORT_PCB_POS );