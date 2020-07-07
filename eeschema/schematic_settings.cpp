/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * @author Jon Evans <jon@craftyjon.com>
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

#include <base_screen.h>
#include <class_libentry.h>
#include <convert_to_biu.h>
#include <default_values.h>
#include <eda_text.h>
#include <eeschema_settings.h>
#include <kiface_i.h>
#include <schematic_settings.h>
#include <settings/parameters.h>


const int schSettingsSchemaVersion = 0;


SCHEMATIC_SETTINGS::SCHEMATIC_SETTINGS( JSON_SETTINGS* aParent, const std::string& aPath ) :
        NESTED_SETTINGS( "schematic", schSettingsSchemaVersion, aParent, aPath ),
        m_DefaultLineWidth( DEFAULT_LINE_THICKNESS * IU_PER_MILS ),
        m_DefaultWireThickness( DEFAULT_WIRE_THICKNESS * IU_PER_MILS ),
        m_DefaultBusThickness( DEFAULT_BUS_THICKNESS * IU_PER_MILS ),
        m_DefaultTextSize( DEFAULT_TEXT_SIZE * IU_PER_MILS ),
        m_TextOffsetRatio( 0.08 ),
        m_PinSymbolSize( DEFAULT_TEXT_SIZE * IU_PER_MILS / 2 ),
        m_JunctionSize( DEFAULT_JUNCTION_DIAM * IU_PER_MILS ),
        m_TemplateFieldNames( nullptr )
{
    EESCHEMA_SETTINGS* appSettings = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );

    int defaultLineThickness =
            appSettings ? appSettings->m_Drawing.default_line_thickness : DEFAULT_LINE_THICKNESS;
    int defaultWireThickness =
            appSettings ? appSettings->m_Drawing.default_wire_thickness : DEFAULT_WIRE_THICKNESS;
    int defaultBusThickness =
            appSettings ? appSettings->m_Drawing.default_bus_thickness : DEFAULT_BUS_THICKNESS;
    int defaultTextSize =
            appSettings ? appSettings->m_Drawing.default_text_size : DEFAULT_TEXT_SIZE;
    int defaultPinSymbolSize =
            appSettings ? appSettings->m_Drawing.pin_symbol_size : DEFAULT_TEXT_SIZE / 2;
    int defaultJunctionSize =
            appSettings ? appSettings->m_Drawing.default_junction_size : DEFAULT_JUNCTION_DIAM / 2;

    m_params.emplace_back( new PARAM_SCALED<int>( "drawing.default_line_thickness",
            &m_DefaultLineWidth, Mils2iu( defaultLineThickness ),
            Mils2iu( 5 ), Mils2iu( 1000 ), 1 / IU_PER_MILS ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "drawing.default_wire_thickness",
            &m_DefaultWireThickness, Mils2iu( defaultWireThickness ),
            Mils2iu( 5 ), Mils2iu( 1000 ), 1 / IU_PER_MILS ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "drawing.default_bus_thickness",
            &m_DefaultBusThickness, Mils2iu( defaultBusThickness ),
            Mils2iu( 5 ), Mils2iu( 1000 ), 1 / IU_PER_MILS ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "drawing.default_text_size", &m_DefaultTextSize,
            Mils2iu( defaultTextSize ), Mils2iu( 5 ), Mils2iu( 1000 ),
            1 / IU_PER_MILS ) );

    m_params.emplace_back( new PARAM<double>( "drawing.text_offset_ratio", &m_TextOffsetRatio,
            (double) TXT_MARGIN / DEFAULT_SIZE_TEXT, -200.0, 200.0 ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "drawing.pin_symbol_size", &m_PinSymbolSize,
            Mils2iu( defaultPinSymbolSize ), Mils2iu( 5 ), Mils2iu( 1000 ),
            1 / IU_PER_MILS ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "drawing.default_junction_size", &m_JunctionSize,
            Mils2iu( defaultJunctionSize ), Mils2iu( 5 ), Mils2iu( 1000 ), 1 / IU_PER_MILS ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "drawing.field_names",
            [&]() -> nlohmann::json
            {
                nlohmann::json ret = nlohmann::json::array();

                if( !m_TemplateFieldNames )
                    return ret;

                for( const TEMPLATE_FIELDNAME& field :
                        m_TemplateFieldNames->GetTemplateFieldNames( false ) )
                {
                    ret.push_back( nlohmann::json( {
                                { "name",    field.m_Name },
                                { "visible", field.m_Visible },
                                { "url",     field.m_URL }
                            } ) );
                }

                return ret;
            },
            [&]( const nlohmann::json& aJson )
            {
                if( !m_TemplateFieldNames || aJson.empty() || !aJson.is_array() )
                    return;

                m_TemplateFieldNames->DeleteAllFieldNameTemplates( false );

                for( const nlohmann::json& entry : aJson )
                {
                    if( !entry.contains( "name" ) || !entry.contains( "url" )
                            || !entry.contains( "visible" ) )
                        continue;

                    TEMPLATE_FIELDNAME field( entry["name"].get<wxString>() );
                    field.m_URL     = entry["url"].get<bool>();
                    field.m_Visible = entry["visible"].get<bool>();
                    m_TemplateFieldNames->AddTemplateFieldName( field, false );
                }
            }, {} ) );

    // TOOD(JE) get rid of this static
    m_params.emplace_back( new PARAM<wxString>(
            "page_layout_descr_file", &BASE_SCREEN::m_PageLayoutDescrFileName, "" ) );

    m_params.emplace_back( new PARAM<wxString>( "plot_directory", &m_PlotDirectoryName, "" ) );

    m_params.emplace_back( new PARAM<wxString>( "net_format_name", &m_NetFormatName, "" ) );

    m_params.emplace_back(
            new PARAM<bool>( "spice_adjust_passive_values", &m_SpiceAdjustPassiveValues, false ) );

    // TODO(JE) should we keep these LIB_PART:: things around?
    m_params.emplace_back( new PARAM<int>(
            "subpart_id_separator", LIB_PART::SubpartIdSeparatorPtr(), 0, 0, 126 ) );

    m_params.emplace_back(
            new PARAM<int>( "subpart_first_id", LIB_PART::SubpartFirstIdPtr(), 'A', '1', 'z' ) );
}


SCHEMATIC_SETTINGS::~SCHEMATIC_SETTINGS()
{
    if( m_parent )
    {
        m_parent->ReleaseNestedSettings( this );
        m_parent = nullptr;
    }
}
