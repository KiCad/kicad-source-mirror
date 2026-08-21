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

#include "job_export_pcb_stackup.h"

#include <jobs/job_registry.h>
#include <wildcards_and_files_ext.h>
#include <wx/filename.h>

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_EXPORT_PCB_STACKUP::OUTPUT_FORMAT,
                              {
                                      { JOB_EXPORT_PCB_STACKUP::OUTPUT_FORMAT::CSV, "csv" },
                                      { JOB_EXPORT_PCB_STACKUP::OUTPUT_FORMAT::JSON, "json" },
                              } )

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_EXPORT_PCB_STACKUP::UNITS, {
                                                                     { JOB_EXPORT_PCB_STACKUP::UNITS::INCH, "in" },
                                                                     { JOB_EXPORT_PCB_STACKUP::UNITS::MM, "mm" },
                                                             } )

JOB_EXPORT_PCB_STACKUP::JOB_EXPORT_PCB_STACKUP() :
        JOB( "stackup", false ),
        m_filename(),
        m_format( OUTPUT_FORMAT::CSV ),
        m_units( UNITS::MM ),
        m_includeColor( true ),
        m_includeMaterial( true ),
        m_includeThickness( true ),
        m_includeEpsilonR( true ),
        m_includeLossTangent( true ),
        m_includeFinish( true ),
        m_includeBoardOptions( true )
{
    m_params.emplace_back( new JOB_PARAM<OUTPUT_FORMAT>( "output_format", &m_format, m_format ) );
    m_params.emplace_back( new JOB_PARAM<UNITS>( "units", &m_units, m_units ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "include_color", &m_includeColor, m_includeColor ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "include_material", &m_includeMaterial, m_includeMaterial ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "include_thickness", &m_includeThickness, m_includeThickness ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "include_epsilon_r", &m_includeEpsilonR, m_includeEpsilonR ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "include_loss_tangent", &m_includeLossTangent, m_includeLossTangent ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "include_finish", &m_includeFinish, m_includeFinish ) );
    m_params.emplace_back(
            new JOB_PARAM<bool>( "include_board_options", &m_includeBoardOptions, m_includeBoardOptions ) );
}


wxString JOB_EXPORT_PCB_STACKUP::GetDefaultDescription() const
{
    return _( "Export board stackup table" );
}


wxString JOB_EXPORT_PCB_STACKUP::GetSettingsDialogTitle() const
{
    return _( "Export Board Stackup Job Settings" );
}


void JOB_EXPORT_PCB_STACKUP::SetDefaultOutputPath( const wxString& aReferenceName )
{
    wxFileName fn( aReferenceName );

    fn.SetName( fn.GetName() + wxS( "_stackup" ) );

    if( m_format == OUTPUT_FORMAT::JSON )
        fn.SetExt( FILEEXT::JsonFileExtension );
    else
        fn.SetExt( FILEEXT::CsvFileExtension );

    SetWorkingOutputPath( fn.GetFullName() );
}


REGISTER_JOB( pcb_export_stackup, _( "PCB: Export Stackup Table" ), KIWAY::FACE_PCB, JOB_EXPORT_PCB_STACKUP );
