/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <lib_symbol.h>
#include <default_values.h>
#include <eeschema_settings.h>
#include <macros.h>
#include <pgm_base.h>
#include <refdes_tracker.h>
#include <schematic_settings.h>
#include <settings/json_settings_internals.h>
#include <settings/parameters.h>
#include <settings/settings_manager.h>
#include <settings/bom_settings.h>
#include <sim/spice_settings.h>


const int schSettingsSchemaVersion = 1;


SCHEMATIC_SETTINGS::SCHEMATIC_SETTINGS( JSON_SETTINGS* aParent, const std::string& aPath ) :
        NESTED_SETTINGS( "schematic", schSettingsSchemaVersion, aParent, aPath, false ),
        m_DefaultLineWidth( DEFAULT_LINE_WIDTH_MILS * schIUScale.IU_PER_MILS ),
        m_DefaultTextSize( DEFAULT_TEXT_SIZE * schIUScale.IU_PER_MILS ),
        m_LabelSizeRatio( DEFAULT_LABEL_SIZE_RATIO ),
        m_TextOffsetRatio( DEFAULT_TEXT_OFFSET_RATIO ),
        m_PinSymbolSize( DEFAULT_TEXT_SIZE * schIUScale.IU_PER_MILS / 2 ),
        m_JunctionSizeChoice( 3 ),
        m_JunctionSize( DEFAULT_JUNCTION_DIAM * schIUScale.IU_PER_MILS ),
        m_HopOverSizeChoice( 0 ),
        m_HopOverScale( 0.0 ),
        m_ConnectionGridSize( DEFAULT_CONNECTION_GRID_MILS * schIUScale.IU_PER_MILS ),
        m_AnnotateStartNum( 0 ),
        m_AnnotateSortOrder( 0 ),
        m_AnnotateMethod( 0 ),
        m_IntersheetRefsShow( false ),
        m_IntersheetRefsListOwnPage( true ),
        m_IntersheetRefsFormatShort( false ),
        m_IntersheetRefsPrefix( DEFAULT_IREF_PREFIX ),
        m_IntersheetRefsSuffix( DEFAULT_IREF_SUFFIX ),
        m_DashedLineDashRatio( 12.0 ),
        m_DashedLineGapRatio( 3.0 ),
        m_OPO_VPrecision( 3 ),
        m_OPO_VRange( wxS( "~V" ) ),
        m_OPO_IPrecision( 3 ),
        m_OPO_IRange( wxS( "~A" ) ),
        m_MaxError( ARC_LOW_DEF_MM * schIUScale.IU_PER_MM ),
        m_NgspiceSettings( nullptr ),
        m_refDesTracker( nullptr )
{
    EESCHEMA_SETTINGS* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" );

    int defaultLineThickness = cfg ? cfg->m_Drawing.default_line_thickness : DEFAULT_LINE_WIDTH_MILS;
    int defaultTextSize = cfg ? cfg->m_Drawing.default_text_size : DEFAULT_TEXT_SIZE;
    int defaultPinSymbolSize = cfg ? cfg->m_Drawing.pin_symbol_size : DEFAULT_TEXT_SIZE / 2;
    int defaultJunctionSizeChoice = cfg ? cfg->m_Drawing.junction_size_choice : 3;
    int defaultHopOverSizeChoice = cfg ? cfg->m_Drawing.hop_over_size_choice : 0;
    bool defaultIntersheetsRefShow = cfg ? cfg->m_Drawing.intersheets_ref_show : false;
    bool defaultIntersheetsRefOwnPage = cfg ? cfg->m_Drawing.intersheets_ref_own_page : true;
    bool defaultIntersheetsRefFormatShort = cfg ? cfg->m_Drawing.intersheets_ref_short : false;
    wxString defaultIntersheetsRefPrefix = cfg ? cfg->m_Drawing.intersheets_ref_prefix
                                               : wxString( wxS( DEFAULT_IREF_PREFIX ) );
    wxString defaultIntersheetsRefSuffix = cfg ? cfg->m_Drawing.intersheets_ref_suffix
                                               : wxString( wxS( DEFAULT_IREF_SUFFIX ) );

    m_params.emplace_back( new PARAM<bool>( "drawing.intersheets_ref_show",
            &m_IntersheetRefsShow, defaultIntersheetsRefShow ) );

    m_params.emplace_back( new PARAM<bool>( "drawing.intersheets_ref_own_page",
            &m_IntersheetRefsListOwnPage, defaultIntersheetsRefOwnPage ) );

    m_params.emplace_back( new PARAM<bool>( "drawing.intersheets_ref_short",
            &m_IntersheetRefsFormatShort, defaultIntersheetsRefFormatShort ) );

    m_params.emplace_back( new PARAM<wxString>( "drawing.intersheets_ref_prefix",
            &m_IntersheetRefsPrefix, defaultIntersheetsRefPrefix ) );

    m_params.emplace_back( new PARAM<wxString>( "drawing.intersheets_ref_suffix",
            &m_IntersheetRefsSuffix, defaultIntersheetsRefSuffix ) );

    m_params.emplace_back( new PARAM<double>( "drawing.dashed_lines_dash_length_ratio",
            &m_DashedLineDashRatio, 12.0 ) );   // Default from ISO 128-2

    m_params.emplace_back( new PARAM<double>( "drawing.dashed_lines_gap_length_ratio",
            &m_DashedLineGapRatio, 3.0 ) );     // Default from ISO 128-2

    m_params.emplace_back( new PARAM<int>( "drawing.operating_point_overlay_v_precision",
            &m_OPO_VPrecision, 3 ) );

    m_params.emplace_back( new PARAM<wxString>( "drawing.operating_point_overlay_v_range",
            &m_OPO_VRange, wxS( "~V" ) ) );

    m_params.emplace_back( new PARAM<int>( "drawing.operating_point_overlay_i_precision",
            &m_OPO_IPrecision, 3 ) );

    m_params.emplace_back( new PARAM<wxString>( "drawing.operating_point_overlay_i_range",
            &m_OPO_IRange, wxS( "~A" ) ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "drawing.default_line_thickness",
            &m_DefaultLineWidth, schIUScale.MilsToIU( defaultLineThickness ),
            schIUScale.MilsToIU( 5 ), schIUScale.MilsToIU( 1000 ), 1 / schIUScale.IU_PER_MILS ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "drawing.default_text_size",
            &m_DefaultTextSize, schIUScale.MilsToIU( defaultTextSize ),
            schIUScale.MilsToIU( 5 ), schIUScale.MilsToIU( 1000 ), 1 / schIUScale.IU_PER_MILS ) );

    m_params.emplace_back( new PARAM<double>( "drawing.text_offset_ratio",
            &m_TextOffsetRatio, DEFAULT_TEXT_OFFSET_RATIO, 0.0, 2.0 ) );

    m_params.emplace_back( new PARAM<double>( "drawing.label_size_ratio",
            &m_LabelSizeRatio, DEFAULT_LABEL_SIZE_RATIO, 0.0, 2.0 ) );

    m_params.emplace_back( new PARAM<double>( "drawing.overbar_offset_ratio",
            &m_FontMetrics.m_OverbarHeight, m_FontMetrics.m_OverbarHeight ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "drawing.pin_symbol_size",
            &m_PinSymbolSize, schIUScale.MilsToIU( defaultPinSymbolSize ),
            schIUScale.MilsToIU( 0 ), schIUScale.MilsToIU( 1000 ), 1 / schIUScale.IU_PER_MILS ) );

    m_params.emplace_back( new PARAM_SCALED<int>( "connection_grid_size",
            &m_ConnectionGridSize, schIUScale.MilsToIU( DEFAULT_CONNECTION_GRID_MILS ),
            schIUScale.MilsToIU( MIN_CONNECTION_GRID_MILS ), schIUScale.MilsToIU( 10000 ),
            1 / schIUScale.IU_PER_MILS ) );

    // m_JunctionSize is only a run-time cache of the calculated size.  Do not save it.

    // User choice for junction dot size ( e.g. none = 0, smallest = 1, small = 2, etc )
    m_params.emplace_back( new PARAM<int>( "drawing.junction_size_choice",
            &m_JunctionSizeChoice, defaultJunctionSizeChoice ) );

    m_params.emplace_back( new PARAM<int>( "drawing.hop_over_size_choice",
            &m_HopOverSizeChoice, defaultHopOverSizeChoice ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "drawing.field_names",
            [&]() -> nlohmann::json
            {
                nlohmann::json ret = nlohmann::json::array();

                for( const TEMPLATE_FIELDNAME& field :
                        m_TemplateFieldNames.GetTemplateFieldNames( false ) )
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
                if( !aJson.empty() && aJson.is_array() )
                {
                    m_TemplateFieldNames.DeleteAllFieldNameTemplates( false );

                    for( const nlohmann::json& entry : aJson )
                    {
                        if( !entry.contains( "name" ) || !entry.contains( "url" )
                                || !entry.contains( "visible" ) )
                        {
                            continue;
                        }

                        TEMPLATE_FIELDNAME field( entry["name"].get<wxString>() );
                        field.m_URL     = entry["url"].get<bool>();
                        field.m_Visible = entry["visible"].get<bool>();
                        m_TemplateFieldNames.AddTemplateFieldName( field, false );
                    }
                }

                // Read global fieldname templates
                if( EESCHEMA_SETTINGS* curr_cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) )
                {
                    if( !curr_cfg->m_Drawing.field_names.IsEmpty() )
                        m_TemplateFieldNames.AddTemplateFieldNames( curr_cfg->m_Drawing.field_names );
                }
            }, {} ) );

    m_params.emplace_back( new PARAM<wxString>( "bom_export_filename",
            &m_BomExportFileName, "${PROJECTNAME}.csv" ) );

    m_params.emplace_back(
            new PARAM<BOM_PRESET>( "bom_settings", &m_BomSettings, BOM_PRESET::DefaultEditing() ) );
    m_params.emplace_back( new PARAM_LIST<BOM_PRESET>( "bom_presets",
            &m_BomPresets, {} ) );

    m_params.emplace_back( new PARAM<BOM_FMT_PRESET>( "bom_fmt_settings",
            &m_BomFmtSettings, BOM_FMT_PRESET::CSV() ) );
    m_params.emplace_back( new PARAM_LIST<BOM_FMT_PRESET>( "bom_fmt_presets",
            &m_BomFmtPresets, {} ) );

    m_params.emplace_back( new PARAM<wxString>( "page_layout_descr_file",
            &m_SchDrawingSheetFileName, "" ) );

    m_params.emplace_back( new PARAM<wxString>( "plot_directory",
            &m_PlotDirectoryName, "" ) );

    // TODO(JE) should we keep these LIB_SYMBOL:: things around?
    m_params.emplace_back( new PARAM<int>( "subpart_id_separator",
            &m_SubpartIdSeparator, 0, 0, 126 ) );

    m_params.emplace_back( new PARAM<int>( "subpart_first_id",
            &m_SubpartFirstId, 'A', '1', 'z' ) );

    m_params.emplace_back( new PARAM<int>( "annotate_start_num",
            &m_AnnotateStartNum, 0 ) );

    m_params.emplace_back( new PARAM<int>( "annotation.sort_order",
            &m_AnnotateSortOrder, 0, 0, 1 ) );

    m_params.emplace_back( new PARAM<int>( "annotation.method",
            &m_AnnotateMethod, 0, 0, 2 ) );

    m_NgspiceSettings = std::make_shared<NGSPICE_SETTINGS>( this, "ngspice" );

    m_params.emplace_back( new PARAM_LAMBDA<bool>( "reuse_designators",
            [&]() -> bool
            {
                return m_refDesTracker ? m_refDesTracker->GetReuseRefDes() : false;
            },
            [&]( bool aReuse )
            {
                if( !m_refDesTracker )
                    m_refDesTracker = std::make_shared<REFDES_TRACKER>();

                m_refDesTracker->SetReuseRefDes( aReuse );
            }, true ) );

    m_params.emplace_back( new PARAM_LAMBDA<std::string>( "used_designators",
            [&]() -> std::string
            {
                if( m_refDesTracker )
                    return m_refDesTracker->Serialize();

                return std::string();
            },
            [&]( const std::string& aData )
            {
                if( !m_refDesTracker )
                    m_refDesTracker = std::make_shared<REFDES_TRACKER>();

                m_refDesTracker->Deserialize( aData );
            }, {} ) );

    registerMigration( 0, 1,
            [&]() -> bool
            {
                std::optional<double> tor = Get<double>( "drawing.text_offset_ratio" );

                if( tor )
                    Set( "drawing.label_size_ratio", *tor );

                return true;
            } );
}


SCHEMATIC_SETTINGS::~SCHEMATIC_SETTINGS()
{
    ReleaseNestedSettings( m_NgspiceSettings.get() );
    m_NgspiceSettings.reset();

    if( m_parent )
    {
        m_parent->ReleaseNestedSettings( this );
        m_parent = nullptr;
    }
}


wxString SCHEMATIC_SETTINGS::SubReference( int aUnit, bool aAddSeparator ) const
{
    wxString subRef;

    if( aUnit < 1 )
        return subRef;

    if( m_SubpartIdSeparator != 0 && aAddSeparator )
        subRef << wxChar( m_SubpartIdSeparator );

    if( m_SubpartFirstId >= '0' && m_SubpartFirstId <= '9' )
        subRef << aUnit;
    else
        subRef << LIB_SYMBOL::LetterSubReference( aUnit, m_SubpartFirstId );

    return subRef;
}
