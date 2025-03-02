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

#include <jobs/job_export_pcb_ipc2581.h>
#include <jobs/job_registry.h>
#include <i18n_utility.h>
#include <wildcards_and_files_ext.h>

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_EXPORT_PCB_IPC2581::IPC2581_UNITS,
                              {
                                      { JOB_EXPORT_PCB_IPC2581::IPC2581_UNITS::INCH, "in" },
                                      { JOB_EXPORT_PCB_IPC2581::IPC2581_UNITS::MM, "mm" },
                              } )

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_EXPORT_PCB_IPC2581::IPC2581_VERSION,
                              {
                                      { JOB_EXPORT_PCB_IPC2581::IPC2581_VERSION::B, "B" },
                                      { JOB_EXPORT_PCB_IPC2581::IPC2581_VERSION::C, "C" },
                              } )

JOB_EXPORT_PCB_IPC2581::JOB_EXPORT_PCB_IPC2581() :
        JOB( "ipc2581", false ),
        m_filename(),
        m_drawingSheet(),
        m_units( IPC2581_UNITS::MM ),
        m_version( IPC2581_VERSION::C ),
        m_precision( 6 ),
        m_compress( false ),
        m_colInternalId(),
        m_colMfgPn(),
        m_colMfg(),
        m_colDistPn(),
        m_colDist()
{
    m_params.emplace_back( new JOB_PARAM<wxString>( "drawing_sheet", &m_drawingSheet, m_drawingSheet ) );
    m_params.emplace_back( new JOB_PARAM<IPC2581_UNITS>( "units", &m_units, m_units ) );
    m_params.emplace_back( new JOB_PARAM<IPC2581_VERSION>( "version", &m_version, m_version ) );
    m_params.emplace_back( new JOB_PARAM<int>( "precision", &m_precision, m_precision ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "compress", &m_compress, m_compress ) );
    m_params.emplace_back( new JOB_PARAM<wxString>( "field_bom_map.internal_id",
                                                    &m_colInternalId,
                                                    m_colInternalId ) );
    m_params.emplace_back( new JOB_PARAM<wxString>( "field_bom_map.mfg_pn",
                                                    &m_colMfgPn, m_colMfgPn ) );
    m_params.emplace_back( new JOB_PARAM<wxString>( "field_bom_map.mfg", &m_colMfg, m_colMfg ) );
    m_params.emplace_back( new JOB_PARAM<wxString>( "field_bom_map.dist_pn",
                                                    &m_colDistPn,
                                                    m_colDistPn ) );
    m_params.emplace_back( new JOB_PARAM<wxString>( "field_bom_map.dist", &m_colDist, m_colDist ) );
}


wxString JOB_EXPORT_PCB_IPC2581::GetDefaultDescription() const
{
    return _( "Export IPC-2581" );
}


wxString JOB_EXPORT_PCB_IPC2581::GetSettingsDialogTitle() const
{
    return _( "Export IPC-2581 Job Settings" );
}


void JOB_EXPORT_PCB_IPC2581::SetDefaultOutputPath( const wxString& aReferenceName )
{
    wxFileName fn = aReferenceName;

    fn.SetExt( FILEEXT::Ipc2581FileExtension );

    SetConfiguredOutputPath( fn.GetFullName() );
}

REGISTER_JOB( pcb_export_ipc2581, _HKI( "PCB: Export IPC-2581" ), KIWAY::FACE_PCB,
              JOB_EXPORT_PCB_IPC2581 );