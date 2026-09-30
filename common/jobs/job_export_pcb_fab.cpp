/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <jobs/job_export_pcb_fab.h>


NLOHMANN_JSON_SERIALIZE_ENUM( JOB_EXPORT_PCB_FAB::UNITS,
                              {
                                      { JOB_EXPORT_PCB_FAB::UNITS::INCH, "in" },
                                      { JOB_EXPORT_PCB_FAB::UNITS::MM, "mm" },
                              } )


JOB_EXPORT_PCB_FAB::JOB_EXPORT_PCB_FAB( const std::string& aType ) : JOB( aType, false )
{
    m_params.emplace_back( new JOB_PARAM<wxString>( "drawing_sheet", &m_drawingSheet, m_drawingSheet ) );
    m_params.emplace_back( new JOB_PARAM<UNITS>( "units", &m_units, m_units ) );
    m_params.emplace_back( new JOB_PARAM<int>( "precision", &m_precision, m_precision ) );
    m_params.emplace_back( new JOB_PARAM_LIST<wxString>( "variant_names", &m_variantNames, m_variantNames ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "check_zones", &m_checkZonesBeforeExport,
                                                 m_checkZonesBeforeExport ) );
    m_params.emplace_back( new JOB_PARAM<wxString>( "field_bom_map.mfg_pn", &m_colMfgPn,
                                                     m_colMfgPn ) );
}


void JOB_EXPORT_PCB_FAB::FromJson( const nlohmann::json& aJson )
{
    JOB::FromJson( aJson );

    if( !aJson.contains( "variant_names" ) )
    {
        wxString legacy = aJson.value( "variant", wxString() );

        if( !legacy.IsEmpty() )
        {
            m_variantNames = { legacy };
        }
    }
}


void JOB_EXPORT_PCB_FAB::ToJson( nlohmann::json& aJson ) const
{
    JOB::ToJson( aJson );
    aJson["variant"] = m_variantNames.empty() ? wxString() : m_variantNames.front();
}
