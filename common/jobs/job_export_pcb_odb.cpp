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

#include <jobs/job_export_pcb_odb.h>
#include <jobs/job_registry.h>
#include <i18n_utility.h>
#include <wildcards_and_files_ext.h>
#include <lset.h>
#include <json_conversions.h>


void nlohmann::adl_serializer<ODB_LAYER_OVERRIDE>::from_json( const json& aJson, ODB_LAYER_OVERRIDE& aOverride )
{
    aOverride = {};

    if( !aJson.is_object() || !aJson.contains( "layer" ) || !aJson["layer"].is_string() )
        return;

    if( ( aJson.contains( "include" ) && !aJson["include"].is_boolean() )
        || ( aJson.contains( "odb_name" ) && !aJson["odb_name"].is_string() )
        || ( aJson.contains( "odb_type" ) && !aJson["odb_type"].is_string() ) )
    {
        return;
    }

    wxString layerName = aJson["layer"].get<wxString>();
    aOverride.m_layer = static_cast<PCB_LAYER_ID>( LSET::NameToLayer( layerName ) );

    if( aJson.contains( "include" ) )
        aOverride.m_include = aJson["include"].get<bool>();

    if( aJson.contains( "odb_name" ) )
        aOverride.m_odbName = aJson["odb_name"].get<wxString>();

    if( aJson.contains( "odb_type" ) )
        aOverride.m_odbType = aJson["odb_type"].get<wxString>();
}


void nlohmann::adl_serializer<ODB_LAYER_OVERRIDE>::to_json( json& aJson, const ODB_LAYER_OVERRIDE& aOverride )
{
    aJson = { { "layer", LSET::Name( aOverride.m_layer ) }, { "include", aOverride.m_include } };

    if( !aOverride.m_odbName.IsEmpty() )
        aJson["odb_name"] = aOverride.m_odbName;

    if( !aOverride.m_odbType.IsEmpty() )
        aJson["odb_type"] = aOverride.m_odbType;
}


NLOHMANN_JSON_SERIALIZE_ENUM( JOB_EXPORT_PCB_ODB::ODB_COMPRESSION,
                              {
                                      { JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::NONE, "none" },
                                      { JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::ZIP, "zip" },
                                      { JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::TGZ, "tgz" },
                              } )

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_EXPORT_PCB_ODB::VARIANT_PACKAGING,
                              {
                                      { JOB_EXPORT_PCB_ODB::VARIANT_PACKAGING::SEPARATE, "separate" },
                                      { JOB_EXPORT_PCB_ODB::VARIANT_PACKAGING::COMBINED, "combined" },
                              } )

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_EXPORT_PCB_ODB::ORIGIN,
                              {
                                      { JOB_EXPORT_PCB_ODB::ORIGIN::ABSOLUTE_COORDS, "absolute" },
                                      { JOB_EXPORT_PCB_ODB::ORIGIN::AUX, "aux" },
                                      { JOB_EXPORT_PCB_ODB::ORIGIN::GRID, "grid" },
                              } )


JOB_EXPORT_PCB_ODB::JOB_EXPORT_PCB_ODB() :
        JOB_EXPORT_PCB_FAB( "odb" ),
        m_compressionMode( ODB_COMPRESSION::ZIP )
{
    m_params.emplace_back( new JOB_PARAM<ODB_COMPRESSION>( "compression", &m_compressionMode, m_compressionMode ) );
    m_params.emplace_back(
            new JOB_PARAM<VARIANT_PACKAGING>( "variant_packaging", &m_variantPackaging, m_variantPackaging ) );
    m_params.emplace_back( new JOB_PARAM<ORIGIN>( "origin", &m_origin, m_origin ) );
    m_params.emplace_back( new JOB_PARAM<wxString>( "product_name", &m_productName, m_productName ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "board_metadata", &m_boardMetadata, m_boardMetadata ) );
    m_params.emplace_back( new JOB_PARAM_LIST<ODB_LAYER_OVERRIDE>( "layers", &m_layerOverrides, m_layerOverrides ) );
}


bool JOB_EXPORT_PCB_ODB::SupportsDataSet( DATA_SET aDataSet ) const
{
    switch( aDataSet )
    {
    case DATA_SET::USERDEF:
    case DATA_SET::STACKUP:
    case DATA_SET::FABRICATION:
    case DATA_SET::ASSEMBLY:
    case DATA_SET::TEST:
        return true;

    // An ODB++ product has no BOM-only or stencil-only form
    default:
        return false;
    }
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
