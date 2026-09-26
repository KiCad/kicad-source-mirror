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

#include <jobs/job_export_pcb_idf.h>
#include <jobs/job_registry.h>
#include <i18n_utility.h>
#include <wildcards_and_files_ext.h>
#include <wx/filename.h>

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_EXPORT_PCB_IDF::UNITS,
                              {
                                  { JOB_EXPORT_PCB_IDF::UNITS::MM,   "mm" },
                                  { JOB_EXPORT_PCB_IDF::UNITS::MILS, "mils" },
                              } )

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_EXPORT_PCB_IDF::COORD_ORIGIN,
                              {
                                  { JOB_EXPORT_PCB_IDF::COORD_ORIGIN::DRILL,  "drill" },
                                  { JOB_EXPORT_PCB_IDF::COORD_ORIGIN::GRID,   "grid" },
                                  { JOB_EXPORT_PCB_IDF::COORD_ORIGIN::CENTER, "center" },
                                  { JOB_EXPORT_PCB_IDF::COORD_ORIGIN::USER,   "user" },
                              } )

JOB_EXPORT_PCB_IDF::JOB_EXPORT_PCB_IDF() :
        JOB( "idf", false ),
        m_filename(),
        m_units( UNITS::MM ),
        m_originMode( COORD_ORIGIN::CENTER ),
        m_includeUnspecified( true ),
        m_includeDNP( true ),
        m_calculateHeightFromModels( true )
{
    m_params.emplace_back( new JOB_PARAM<UNITS>( "units", &m_units, m_units ) );
    m_params.emplace_back( new JOB_PARAM<COORD_ORIGIN>( "origin", &m_originMode, m_originMode ) );
    m_params.emplace_back( new JOB_PARAM<double>( "user_origin.x", &m_userOrigin.x, m_userOrigin.x ) );
    m_params.emplace_back( new JOB_PARAM<double>( "user_origin.y", &m_userOrigin.y, m_userOrigin.y ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "include_unspecified", &m_includeUnspecified, m_includeUnspecified ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "include_dnp", &m_includeDNP, m_includeDNP ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "calculate_height_from_models", &m_calculateHeightFromModels,
                                                m_calculateHeightFromModels ) );
}


wxString JOB_EXPORT_PCB_IDF::GetDefaultDescription() const
{
    return _( "Export IDFv3" );
}


wxString JOB_EXPORT_PCB_IDF::GetSettingsDialogTitle() const
{
    return _( "Export IDFv3 Job Settings" );
}


void JOB_EXPORT_PCB_IDF::SetDefaultOutputPath( const wxString& aReferenceName )
{
    wxFileName fn = aReferenceName;

    fn.SetExt( FILEEXT::IdfV3BoardFileExtension );

    SetConfiguredOutputPath( fn.GetFullName() );
}


REGISTER_JOB( pcb_export_idf, _HKI( "PCB: Export IDFv3" ), KIWAY::FACE_PCB, JOB_EXPORT_PCB_IDF );
