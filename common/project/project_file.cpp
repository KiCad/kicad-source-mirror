/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <config_params.h>
#include <project.h>
#include <project/net_settings.h>
#include <settings/json_settings_internals.h>
#include <project/project_file.h>
#include <settings/common_settings.h>
#include <settings/parameters.h>
#include <wildcards_and_files_ext.h>
#include <wx/config.h>
#include <wx/log.h>


///! Update the schema version whenever a migration is required
const int projectFileSchemaVersion = 1;


PROJECT_FILE::PROJECT_FILE( const wxString& aFullPath ) :
        JSON_SETTINGS( aFullPath, SETTINGS_LOC::PROJECT, projectFileSchemaVersion ),
        m_ErcSettings( nullptr ),
        m_SchematicSettings( nullptr ),
        m_BoardSettings(),
        m_sheets(),
        m_boards(),
        m_project( nullptr )
{
    // Keep old files around
    m_deleteLegacyAfterMigration = false;

    m_params.emplace_back( new PARAM_LIST<FILE_INFO_PAIR>( "sheets", &m_sheets, {} ) );

    m_params.emplace_back( new PARAM_LIST<FILE_INFO_PAIR>( "boards", &m_boards, {} ) );

    m_params.emplace_back( new PARAM_WXSTRING_MAP( "text_variables", &m_TextVars, {} ) );

    m_params.emplace_back(
            new PARAM_LIST<wxString>( "libraries.pinned_symbol_libs", &m_PinnedSymbolLibs, {} ) );

    m_params.emplace_back( new PARAM_LIST<wxString>(
            "libraries.pinned_footprint_libs", &m_PinnedFootprintLibs, {} ) );

    m_params.emplace_back(
            new PARAM_PATH_LIST( "cvpcb.equivalence_files", &m_EquivalenceFiles, {} ) );

    m_params.emplace_back(
            new PARAM_PATH( "pcbnew.page_layout_descr_file", &m_BoardDrawingSheetFile, "" ) );

    m_params.emplace_back(
            new PARAM_PATH( "pcbnew.last_paths.netlist", &m_PcbLastPath[LAST_PATH_NETLIST], "" ) );

    m_params.emplace_back(
            new PARAM_PATH( "pcbnew.last_paths.step", &m_PcbLastPath[LAST_PATH_STEP], "" ) );

    m_params.emplace_back(
            new PARAM_PATH( "pcbnew.last_paths.idf", &m_PcbLastPath[LAST_PATH_IDF], "" ) );

    m_params.emplace_back(
            new PARAM_PATH( "pcbnew.last_paths.vrml", &m_PcbLastPath[LAST_PATH_VRML], "" ) );

    m_params.emplace_back( new PARAM_PATH(
            "pcbnew.last_paths.specctra_dsn", &m_PcbLastPath[LAST_PATH_SPECCTRADSN], "" ) );

    m_params.emplace_back(
            new PARAM_PATH( "pcbnew.last_paths.gencad", &m_PcbLastPath[LAST_PATH_GENCAD], "" ) );

    m_params.emplace_back( new PARAM<wxString>( "schematic.legacy_lib_dir", &m_LegacyLibDir, "" ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "schematic.legacy_lib_list",
            [&]() -> nlohmann::json
            {
                nlohmann::json ret = nlohmann::json::array();

                for( const wxString& libName : m_LegacyLibNames )
                    ret.push_back( libName );

                return ret;
            },
            [&]( const nlohmann::json& aJson )
            {
                if( aJson.empty() || !aJson.is_array() )
                    return;

                m_LegacyLibNames.clear();

                for( const nlohmann::json& entry : aJson )
                    m_LegacyLibNames.push_back( entry.get<wxString>() );
            }, {} ) );

    m_NetSettings = std::make_shared<NET_SETTINGS>( this, "net_settings" );

    m_params.emplace_back( new PARAM_LAYER_PRESET( "board.layer_presets", &m_LayerPresets ) );
}


bool PROJECT_FILE::MigrateFromLegacy( wxConfigBase* aCfg )
{
    bool     ret = true;
    wxString str;
    long     index = 0;

    std::set<wxString> group_blacklist;

    // Legacy files don't store board info; they assume board matches project name
    // We will leave m_boards empty here so it can be populated with other code

    // First handle migration of data that will be stored locally in this object

    auto loadPinnedLibs =
            [&]( const std::string& aDest )
            {
                int      libIndex = 1;
                wxString libKey   = wxT( "PinnedItems" );
                libKey << libIndex;

                nlohmann::json libs = nlohmann::json::array();

                while( aCfg->Read( libKey, &str ) )
                {
                    libs.push_back( str );

                    aCfg->DeleteEntry( libKey, true );

                    libKey = wxT( "PinnedItems" );
                    libKey << ++libIndex;
                }

                Set( aDest, libs );
            };

    aCfg->SetPath( wxT( "/LibeditFrame" ) );
    loadPinnedLibs( "libraries.pinned_symbol_libs" );

    aCfg->SetPath( wxT( "/ModEditFrame" ) );
    loadPinnedLibs( "libraries.pinned_footprint_libs" );

    aCfg->SetPath( wxT( "/cvpcb/equfiles" ) );

    {
        int      eqIdx = 1;
        wxString eqKey = wxT( "EquName" );
        eqKey << eqIdx;

        nlohmann::json eqs = nlohmann::json::array();

        while( aCfg->Read( eqKey, &str ) )
        {
            eqs.push_back( str );

            eqKey = wxT( "EquName" );
            eqKey << ++eqIdx;
        }

        Set( "cvpcb.equivalence_files", eqs );
    }

    // All CvPcb params that we want to keep have been migrated above
    group_blacklist.insert( wxT( "/cvpcb" ) );

    aCfg->SetPath( wxT( "/eeschema" ) );
    fromLegacyString( aCfg, "LibDir", "schematic.legacy_lib_dir" );

    aCfg->SetPath( wxT( "/eeschema/libraries" ) );

    {
        int      libIdx = 1;
        wxString libKey = wxT( "LibName" );
        libKey << libIdx;

        nlohmann::json libs = nlohmann::json::array();

        while( aCfg->Read( libKey, &str ) )
        {
            libs.push_back( str );

            libKey = wxT( "LibName" );
            libKey << ++libIdx;
        }

        Set( "schematic.legacy_lib_list", libs );
    }

    group_blacklist.insert( wxT( "/eeschema" ) );

    aCfg->SetPath( wxT( "/text_variables" ) );

    {
        int      txtIdx = 1;
        wxString txtKey;
        txtKey << txtIdx;

        nlohmann::json vars = nlohmann::json();

        while( aCfg->Read( txtKey, &str ) )
        {
            wxArrayString tokens = wxSplit( str, ':' );

            if( tokens.size() == 2 )
                vars[ tokens[0].ToStdString() ] = tokens[1];

            txtKey.clear();
            txtKey << ++txtIdx;
        }

        Set( "text_variables", vars );
    }

    group_blacklist.insert( wxT( "/text_variables" ) );

    aCfg->SetPath( wxT( "/schematic_editor" ) );

    fromLegacyString( aCfg, "PageLayoutDescrFile",     "schematic.page_layout_descr_file" );
    fromLegacyString( aCfg, "PlotDirectoryName",       "schematic.plot_directory" );
    fromLegacyString( aCfg, "NetFmtName",              "schematic.net_format_name" );
    fromLegacy<bool>( aCfg, "SpiceAjustPassiveValues", "schematic.spice_adjust_passive_values" );
    fromLegacy<int>(  aCfg, "SubpartIdSeparator",      "schematic.subpart_id_separator" );
    fromLegacy<int>(  aCfg, "SubpartFirstId",          "schematic.subpart_first_id" );

    fromLegacy<int>( aCfg, "LineThickness",         "schematic.drawing.default_line_thickness" );
    fromLegacy<int>( aCfg, "WireThickness",         "schematic.drawing.default_wire_thickness" );
    fromLegacy<int>( aCfg, "BusThickness",          "schematic.drawing.default_bus_thickness" );
    fromLegacy<int>( aCfg, "LabSize",               "schematic.drawing.default_text_size" );
    fromLegacy<int>( aCfg, "PinSymbolSize",         "schematic.drawing.pin_symbol_size" );
    fromLegacy<int>( aCfg, "JunctionSize",          "schematic.drawing.default_junction_size" );

    fromLegacyString(   aCfg, "FieldNameTemplates", "schematic.drawing.field_names" );

    if( !fromLegacy<double>( aCfg, "TextOffsetRatio", "schematic.drawing.text_offset_ratio" ) )
    {
        // Use the spacing of Eeschema V5
        Set( "schematic.drawing.text_offset_ratio", 0.08 );
        Set( "schematic.drawing.label_size_ratio", 0.25 );
    }

    // All schematic_editor keys we keep are migrated above
    group_blacklist.insert( wxT( "/schematic_editor" ) );

    aCfg->SetPath( wxT( "/pcbnew" ) );

    fromLegacyString( aCfg, "PageLayoutDescrFile",       "pcbnew.page_layout_descr_file" );
    fromLegacyString( aCfg, "LastNetListRead",           "pcbnew.last_paths.netlist" );
    fromLegacyString( aCfg, "LastSTEPExportPath",        "pcbnew.last_paths.step" );
    fromLegacyString( aCfg, "LastIDFExportPath",         "pcbnew.last_paths.idf" );
    fromLegacyString( aCfg, "LastVRMLExportPath",        "pcbnew.last_paths.vmrl" );
    fromLegacyString( aCfg, "LastSpecctraDSNExportPath", "pcbnew.last_paths.specctra_dsn" );
    fromLegacyString( aCfg, "LastGenCADExportPath",      "pcbnew.last_paths.gencad" );

    std::string bp = "board.design_settings.";

    {
        int      idx = 1;
        wxString key = wxT( "DRCExclusion" );
        key << idx;

        nlohmann::json exclusions = nlohmann::json::array();

        while( aCfg->Read( key, &str ) )
        {
            exclusions.push_back( str );

            key = wxT( "DRCExclusion" );
            key << ++idx;
        }

        Set( bp + "drc_exclusions", exclusions );
    }

    fromLegacy<bool>( aCfg,   "AllowMicroVias",  bp + "rules.allow_microvias" );
    fromLegacy<bool>( aCfg,   "AllowBlindVias",  bp + "rules.allow_blind_buried_vias" );
    fromLegacy<double>( aCfg, "MinClearance",    bp + "rules.min_clearance" );
    fromLegacy<double>( aCfg, "MinTrackWidth",   bp + "rules.min_track_width" );
    fromLegacy<double>( aCfg, "MinViaAnnulus",   bp + "rules.min_via_annulus" );
    fromLegacy<double>( aCfg, "MinViaDiameter",  bp + "rules.min_via_diameter" );

    if( !fromLegacy<double>( aCfg, "MinThroughDrill", bp + "rules.min_through_hole_diameter" ) )
        fromLegacy<double>( aCfg, "MinViaDrill", bp + "rules.min_through_hole_diameter" );

    fromLegacy<double>( aCfg, "MinMicroViaDiameter",  bp + "rules.min_microvia_diameter" );
    fromLegacy<double>( aCfg, "MinMicroViaDrill",     bp + "rules.min_microvia_drill" );
    fromLegacy<double>( aCfg, "MinHoleToHole",        bp + "rules.min_hole_to_hole" );
    fromLegacy<double>( aCfg, "CopperEdgeClearance",  bp + "rules.min_copper_edge_clearance" );
    fromLegacy<double>( aCfg, "SolderMaskClearance",  bp + "rules.solder_mask_clearance" );
    fromLegacy<double>( aCfg, "SolderMaskMinWidth",   bp + "rules.solder_mask_min_width" );
    fromLegacy<double>( aCfg, "SolderPasteClearance", bp + "rules.solder_paste_clearance" );
    fromLegacy<double>( aCfg, "SolderPasteRatio",     bp + "rules.solder_paste_margin_ratio" );

    if( !fromLegacy<double>( aCfg, "SilkLineWidth", bp + "defaults.silk_line_width" ) )
        fromLegacy<double>( aCfg, "ModuleOutlineThickness", bp + "defaults.silk_line_width" );

    if( !fromLegacy<double>( aCfg, "SilkTextSizeV", bp + "defaults.silk_text_size_v" ) )
        fromLegacy<double>( aCfg, "ModuleTextSizeV", bp + "defaults.silk_text_size_v" );

    if( !fromLegacy<double>( aCfg, "SilkTextSizeH", bp + "defaults.silk_text_size_h" ) )
        fromLegacy<double>( aCfg, "ModuleTextSizeH", bp + "defaults.silk_text_size_h" );

    if( !fromLegacy<double>( aCfg, "SilkTextSizeThickness", bp + "defaults.silk_text_thickness" ) )
        fromLegacy<double>( aCfg, "ModuleTextSizeThickness", bp + "defaults.silk_text_thickness" );

    fromLegacy<bool>( aCfg, "SilkTextItalic",   bp + "defaults.silk_text_italic" );
    fromLegacy<bool>( aCfg, "SilkTextUpright",  bp + "defaults.silk_text_upright" );

    if( !fromLegacy<double>( aCfg, "CopperLineWidth", bp + "defaults.copper_line_width" ) )
        fromLegacy<double>( aCfg, "DrawSegmentWidth", bp + "defaults.copper_line_width" );

    if( !fromLegacy<double>( aCfg, "CopperTextSizeV", bp + "defaults.copper_text_size_v" ) )
        fromLegacy<double>( aCfg, "PcbTextSizeV", bp + "defaults.copper_text_size_v" );

    if( !fromLegacy<double>( aCfg, "CopperTextSizeH", bp + "defaults.copper_text_size_h" ) )
        fromLegacy<double>( aCfg, "PcbTextSizeH", bp + "defaults.copper_text_size_h" );

    if( !fromLegacy<double>( aCfg, "CopperTextThickness", bp + "defaults.copper_text_thickness" ) )
        fromLegacy<double>( aCfg, "PcbTextThickness", bp + "defaults.copper_text_thickness" );

    fromLegacy<bool>( aCfg, "CopperTextItalic",   bp + "defaults.copper_text_italic" );
    fromLegacy<bool>( aCfg, "CopperTextUpright",  bp + "defaults.copper_text_upright" );

    if( !fromLegacy<double>( aCfg, "EdgeCutLineWidth", bp + "defaults.board_outline_line_width" ) )
        fromLegacy<double>( aCfg, "BoardOutlineThickness", bp + "defaults.board_outline_line_width" );

    fromLegacy<double>( aCfg, "CourtyardLineWidth",   bp + "defaults.courtyard_line_width" );

    fromLegacy<double>( aCfg, "FabLineWidth",         bp + "defaults.fab_line_width" );
    fromLegacy<double>( aCfg, "FabTextSizeV",         bp + "defaults.fab_text_size_v" );
    fromLegacy<double>( aCfg, "FabTextSizeH",         bp + "defaults.fab_text_size_h" );
    fromLegacy<double>( aCfg, "FabTextSizeThickness", bp + "defaults.fab_text_thickness" );
    fromLegacy<bool>(   aCfg, "FabTextItalic",        bp + "defaults.fab_text_italic" );
    fromLegacy<bool>(   aCfg, "FabTextUpright",       bp + "defaults.fab_text_upright" );

    if( !fromLegacy<double>( aCfg, "OthersLineWidth", bp + "defaults.other_line_width" ) )
        fromLegacy<double>( aCfg, "ModuleOutlineThickness", bp + "defaults.other_line_width" );

    fromLegacy<double>( aCfg, "OthersTextSizeV",         bp + "defaults.other_text_size_v" );
    fromLegacy<double>( aCfg, "OthersTextSizeH",         bp + "defaults.other_text_size_h" );
    fromLegacy<double>( aCfg, "OthersTextSizeThickness", bp + "defaults.other_text_thickness" );
    fromLegacy<bool>(   aCfg, "OthersTextItalic",        bp + "defaults.other_text_italic" );
    fromLegacy<bool>(   aCfg, "OthersTextUpright",       bp + "defaults.other_text_upright" );

    fromLegacy<int>( aCfg, "DimensionUnits",     bp + "defaults.dimension_units" );
    fromLegacy<int>( aCfg, "DimensionPrecision", bp + "defaults.dimension_precision" );

    std::string sev = bp + "rule_severities";

    fromLegacy<bool>( aCfg, "RequireCourtyardDefinitions", sev + "legacy_no_courtyard_defined" );

    fromLegacy<bool>( aCfg, "ProhibitOverlappingCourtyards", sev + "legacy_courtyards_overlap" );

    {
        int      idx     = 1;
        wxString keyBase = "TrackWidth";
        wxString key     = keyBase;
        double   val;

        nlohmann::json widths = nlohmann::json::array();

        key << idx;

        while( aCfg->Read( key, &val ) )
        {
            widths.push_back( val );
            key = keyBase;
            key << ++idx;
        }

        Set( bp + "track_widths", widths );
    }

    {
        int      idx     = 1;
        wxString keyBase = "ViaDiameter";
        wxString key     = keyBase;
        double   diameter;
        double   drill   = 1.0;

        nlohmann::json vias = nlohmann::json::array();

        key << idx;

        while( aCfg->Read( key, &diameter ) )
        {
            key = "ViaDrill";
            aCfg->Read( key << idx, &drill );

            nlohmann::json via = { { "diameter", diameter }, { "drill", drill } };
            vias.push_back( via );

            key = keyBase;
            key << ++idx;
        }

        Set( bp + "via_dimensions", vias );
    }

    {
        int      idx     = 1;
        wxString keyBase = "dPairWidth";
        wxString key     = keyBase;
        double   width;
        double   gap     = 1.0;
        double   via_gap = 1.0;

        nlohmann::json pairs = nlohmann::json::array();

        key << idx;

        while( aCfg->Read( key, &width ) )
        {
            key = "dPairGap";
            aCfg->Read( key << idx, &gap );

            key = "dPairViaGap";
            aCfg->Read( key << idx, &via_gap );

            nlohmann::json pair = { { "width", width }, { "gap", gap }, { "via_gap", via_gap } };
            pairs.push_back( pair );

            key = keyBase;
            key << ++idx;
        }

        Set( bp + "diff_pair_dimensions",  pairs );
    }

    group_blacklist.insert( wxT( "/pcbnew" ) );

    // General group is unused these days, we can throw it away
    group_blacklist.insert( wxT( "/general" ) );

    // Next load sheet names and put all other legacy data in the legacy dict
    aCfg->SetPath( wxT( "/" ) );

    auto loadSheetNames =
            [&]() -> bool
            {
                int            sheet = 1;
                wxString       entry;
                nlohmann::json arr   = nlohmann::json::array();

                wxLogTrace( traceSettings, "Migrating sheet names" );

                aCfg->SetPath( wxT( "/sheetnames" ) );

                while( aCfg->Read( wxString::Format( "%d", sheet++ ), &entry ) )
                {
                    wxArrayString tokens = wxSplit( entry, ':' );

                    if( tokens.size() == 2 )
                    {
                        wxLogTrace( traceSettings, "%d: %s = %s", sheet, tokens[0], tokens[1] );
                        arr.push_back( nlohmann::json::array( { tokens[0], tokens[1] } ) );
                    }
                }

                Set( "sheets", arr );

                aCfg->SetPath( "/" );

                // TODO: any reason we want to fail on this?
                return true;
            };

    std::vector<wxString> groups;

    groups.emplace_back( "" );

    auto loadLegacyPairs =
            [&]( const std::string& aGroup ) -> bool
            {
                wxLogTrace( traceSettings, "Migrating group %s", aGroup );
                bool     success = true;
                wxString keyStr;
                wxString val;

                index = 0;

                while( aCfg->GetNextEntry( keyStr, index ) )
                {
                    if( !aCfg->Read( keyStr, &val ) )
                        continue;

                    std::string key( keyStr.ToUTF8() );

                    wxLogTrace( traceSettings, "    %s = %s", key, val );

                    try
                    {
                        Set( "legacy." + aGroup + "." + key, val );
                    }
                    catch( ... )
                    {
                        success = false;
                    }
                }

                return success;
            };

    for( size_t i = 0; i < groups.size(); i++ )
    {
        aCfg->SetPath( groups[i] );

        if( groups[i] == wxT( "/sheetnames" ) )
        {
            ret |= loadSheetNames();
            continue;
        }

        aCfg->DeleteEntry( wxT( "last_client" ), true );
        aCfg->DeleteEntry( wxT( "update" ), true );
        aCfg->DeleteEntry( wxT( "version" ), true );

        ret &= loadLegacyPairs( groups[i].ToStdString() );

        index = 0;

        while( aCfg->GetNextGroup( str, index ) )
        {
            wxString group = groups[i] + "/" + str;

            if( !group_blacklist.count( group ) )
                groups.emplace_back( group );
        }

        aCfg->SetPath( "/" );
    }

    return ret;
}


bool PROJECT_FILE::SaveToFile( const wxString& aDirectory, bool aForce )
{
    wxASSERT( m_project );

    Set( "meta.filename", m_project->GetProjectName() + "." + ProjectFileExtension );

    return JSON_SETTINGS::SaveToFile( aDirectory, aForce );
}


bool PROJECT_FILE::SaveAs( const wxString& aDirectory, const wxString& aFile )
{
    Set( "meta.filename", aFile + "." + ProjectFileExtension );
    SetFilename( aFile );

    // While performing Save As, we have already checked that we can write to the directory
    // so don't carry the previous flag
    SetReadOnly( false );
    return JSON_SETTINGS::SaveToFile( aDirectory, true );
}


wxString PROJECT_FILE::getFileExt() const
{
    return ProjectFileExtension;
}


wxString PROJECT_FILE::getLegacyFileExt() const
{
    return LegacyProjectFileExtension;
}


void to_json( nlohmann::json& aJson, const FILE_INFO_PAIR& aPair )
{
    aJson = nlohmann::json::array( { aPair.first.AsString().ToUTF8(), aPair.second.ToUTF8() } );
}


void from_json( const nlohmann::json& aJson, FILE_INFO_PAIR& aPair )
{
    wxCHECK( aJson.is_array() && aJson.size() == 2, /* void */ );
    aPair.first  = KIID( wxString( aJson[0].get<std::string>().c_str(), wxConvUTF8 ) );
    aPair.second = wxString( aJson[1].get<std::string>().c_str(), wxConvUTF8 );
}
