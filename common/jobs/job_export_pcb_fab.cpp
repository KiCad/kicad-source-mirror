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

#include <magic_enum.hpp>


NLOHMANN_JSON_SERIALIZE_ENUM( JOB_EXPORT_PCB_FAB::UNITS,
                              {
                                      { JOB_EXPORT_PCB_FAB::UNITS::INCH, "in" },
                                      { JOB_EXPORT_PCB_FAB::UNITS::MM, "mm" },
                              } )


// The null entry comes first so that an unknown token loads as COUNT
NLOHMANN_JSON_SERIALIZE_ENUM( JOB_EXPORT_PCB_FAB::DATA_SET,
                              {
                                      { JOB_EXPORT_PCB_FAB::DATA_SET::COUNT, nullptr },
                                      { JOB_EXPORT_PCB_FAB::DATA_SET::USERDEF, "userdef" },
                                      { JOB_EXPORT_PCB_FAB::DATA_SET::BOM, "bom" },
                                      { JOB_EXPORT_PCB_FAB::DATA_SET::STACKUP, "stackup" },
                                      { JOB_EXPORT_PCB_FAB::DATA_SET::FABRICATION, "fabrication" },
                                      { JOB_EXPORT_PCB_FAB::DATA_SET::ASSEMBLY, "assembly" },
                                      { JOB_EXPORT_PCB_FAB::DATA_SET::TEST, "test" },
                                      { JOB_EXPORT_PCB_FAB::DATA_SET::STENCIL, "stencil" },
                                      { JOB_EXPORT_PCB_FAB::DATA_SET::DFX, "dfx" },
                              } )

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_EXPORT_PCB_FAB::NET_NAMES,
                              {
                                      { JOB_EXPORT_PCB_FAB::NET_NAMES::INCLUDE, "include" },
                                      { JOB_EXPORT_PCB_FAB::NET_NAMES::ANONYMIZE, "anonymize" },
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
    m_params.emplace_back( new JOB_PARAM<DATA_SET>( "data_set", &m_dataSet, m_dataSet ) );
    m_params.emplace_back( new JOB_PARAM<NET_NAMES>( "net_names", &m_netNames, m_netNames ) );
}


JOB_EXPORT_PCB_FAB::DATA_SET JOB_EXPORT_PCB_FAB::DataSetFromToken( const wxString& aToken )
{
    if( aToken.IsEmpty() )
        return DATA_SET::USERDEF;

    // The magic_enum comparison folds ASCII only, so tr_TR cannot break FABRICATION
    return magic_enum::enum_cast<DATA_SET>( std::string( aToken.ToUTF8() ), magic_enum::case_insensitive )
            .value_or( DATA_SET::COUNT );
}


std::string JOB_EXPORT_PCB_FAB::DataSetToken( DATA_SET aDataSet )
{
    nlohmann::json token = aDataSet;

    // COUNT has no token, and the CLI asks from a static constructor where a throw aborts
    return token.is_string() ? token.get<std::string>() : std::string();
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

    // Older files hold an empty key when nothing was chosen, so only the flag marks an empty choice
    wxString sections = aJson.value( "sections", wxString() );

    if( !sections.IsEmpty() || aJson.value( "custom_sections", false ) )
        m_sections = sections;
    else
        m_sections.reset();
}


void JOB_EXPORT_PCB_FAB::ToJson( nlohmann::json& aJson ) const
{
    JOB::ToJson( aJson );
    aJson["variant"] = m_variantNames.empty() ? wxString() : m_variantNames.front();
    aJson["sections"] = m_sections.value_or( wxString() );
    aJson["custom_sections"] = m_sections && m_sections->IsEmpty();
}
