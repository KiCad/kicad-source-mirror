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

#include <jobs/job_export_pcb_odb.h>
#include <jobs/job_registry.h>
#include <i18n_utility.h>
#include <wildcards_and_files_ext.h>

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_EXPORT_PCB_ODB::ODB_UNITS,
                              {
                                      { JOB_EXPORT_PCB_ODB::ODB_UNITS::INCH, "in" },
                                      { JOB_EXPORT_PCB_ODB::ODB_UNITS::MM, "mm" },
                              } )

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_EXPORT_PCB_ODB::ODB_COMPRESSION,
                              {
                                      { JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::NONE, "none" },
                                      { JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::ZIP, "zip" },
                                      { JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::TGZ, "tgz" },
                              } )


JOB_EXPORT_PCB_ODB::JOB_EXPORT_PCB_ODB() :
        JOB( "odb", false ),
        m_filename(),
        m_drawingSheet(),
        m_units( ODB_UNITS::MM ),
        m_precision( 4 ),
        m_compressionMode( ODB_COMPRESSION::ZIP )
{
    m_params.emplace_back( new JOB_PARAM<wxString>( "drawing_sheet", &m_drawingSheet, m_drawingSheet ) );
    m_params.emplace_back( new JOB_PARAM<ODB_UNITS>( "units", &m_units, m_units ) );
    m_params.emplace_back( new JOB_PARAM<int>( "precision", &m_precision, m_precision ) );
    m_params.emplace_back( new JOB_PARAM<ODB_COMPRESSION>( "compression", &m_compressionMode,
                                                           m_compressionMode ) );
}


wxString JOB_EXPORT_PCB_ODB::GetDefaultDescription() const
{
    return _( "Export ODB++" );
}


wxString JOB_EXPORT_PCB_ODB::GetSettingsDialogTitle() const
{
    return _( "Export ODB++ Job Settings" );
}


void JOB_EXPORT_PCB_ODB::SetDefaultOutputPath( const wxString& aReferenceName )
{
    wxFileName fn = aReferenceName;

    fn.SetExt( "zip" );

    SetConfiguredOutputPath( fn.GetFullName() );
}

REGISTER_JOB( pcb_export_odb, _HKI( "PCB: Export ODB++" ), KIWAY::FACE_PCB,
              JOB_EXPORT_PCB_ODB );
