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

#include <lset.h>
#include <project.h>
#include <project/project_local_settings.h>
#include <settings/layer_settings_utils.h>
#include <settings/json_settings_internals.h>
#include <settings/parameters.h>

const int projectLocalSettingsVersion = 5;


PROJECT_LOCAL_SETTINGS::PROJECT_LOCAL_SETTINGS( PROJECT* aProject, const wxString& aFilename ) :
        JSON_SETTINGS( aFilename, SETTINGS_LOC::PROJECT, projectLocalSettingsVersion,
                       /* aCreateIfMissing = */ true, /* aCreateIfDefault = */ false,
                       /* aWriteFile = */ true ),
        // clang-format off: suggestion is less readable.
        m_ActiveLayer( UNDEFINED_LAYER ),
        m_ContrastModeDisplay( HIGH_CONTRAST_MODE::NORMAL ),
        m_NetColorMode( NET_COLOR_MODE::RATSNEST ),
        m_AutoTrackWidth( true ),
        m_ZoneDisplayMode( ZONE_DISPLAY_MODE::SHOW_FILLED ),
        m_PrototypeZoneFill( false ),
        m_TrackOpacity( 1.0 ),
        m_ViaOpacity( 1.0 ),
        m_PadOpacity( 1.0 ),
        m_ZoneOpacity( 0.6 ),
        m_ShapeOpacity( 1.0 ),
        m_ImageOpacity( 0.6 ),
        m_PcbSelectionFilter(),
        m_GitIntegrationDisabled( false ),
        m_project( aProject ),
        m_wasMigrated( false )
// clang-format on: suggestion is less readable.
{
    // Keep old files around
    m_deleteLegacyAfterMigration = false;

    m_params.emplace_back( new PARAM_LAMBDA<std::string>( "board.visible_layers",
            [&]() -> std::string
            {
                return m_VisibleLayers.FmtHex();
            },
            [&]( const std::string& aString )
            {
                m_VisibleLayers.ParseHex( aString );
            },
            LSET::AllLayersMask().FmtHex() ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "board.visible_items",
            [&]() -> nlohmann::json
            {
                nlohmann::json ret = nlohmann::json::array();

                for( GAL_LAYER_ID l : m_VisibleItems.Seq() )
                {
                    if( std::optional<VISIBILITY_LAYER> vl = VisibilityLayerFromRenderLayer( l ) )
                        ret.push_back( VisibilityLayerToString( *vl ) );
                }

                // Explicit marker to tell apart a wiped-out array from the user hiding everything
                if( ret.empty() )
                    ret.push_back( "none" );

                return ret;
            },
            [&]( const nlohmann::json& aVal )
            {
                if( !aVal.is_array() || aVal.empty() )
                {
                    m_VisibleItems |= UserVisbilityLayers();
                    return;
                }

                m_VisibleItems &= ~UserVisbilityLayers();
                GAL_SET visible;
                bool none = false;

                for( const nlohmann::json& entry : aVal )
                {
                    try
                    {
                        std::string vs = entry.get<std::string>();

                        if( std::optional<GAL_LAYER_ID> l = RenderLayerFromVisbilityString( vs ) )
                            visible.set( *l );
                        else if( vs == "none" )
                            none = true;
                    }
                    catch( ... )
                    {
                        // Unknown entry (possibly the settings file was re-saved by an old version
                        // of kicad that used numeric entries, or is a future format)
                    }
                }

                // Restore corrupted state
                if( !visible.any() && !none )
                    m_VisibleItems |= UserVisbilityLayers();
                else
                    m_VisibleItems |= UserVisbilityLayers() & visible;
            },
            {} ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "board.selection_filter",
            [&]() -> nlohmann::json
            {
                nlohmann::json ret;

                ret["lockedItems"] = m_PcbSelectionFilter.lockedItems;
                ret["footprints"]  = m_PcbSelectionFilter.footprints;
                ret["text"]        = m_PcbSelectionFilter.text;
                ret["tracks"]      = m_PcbSelectionFilter.tracks;
                ret["vias"]        = m_PcbSelectionFilter.vias;
                ret["pads"]        = m_PcbSelectionFilter.pads;
                ret["graphics"]    = m_PcbSelectionFilter.graphics;
                ret["zones"]       = m_PcbSelectionFilter.zones;
                ret["keepouts"]    = m_PcbSelectionFilter.keepouts;
                ret["dimensions"]  = m_PcbSelectionFilter.dimensions;
                ret["otherItems"]  = m_PcbSelectionFilter.otherItems;

                return ret;
            },
            [&]( const nlohmann::json& aVal )
            {
                if( aVal.empty() || !aVal.is_object() )
                    return;

                SetIfPresent( aVal, "lockedItems", m_PcbSelectionFilter.lockedItems );
                SetIfPresent( aVal, "footprints", m_PcbSelectionFilter.footprints );
                SetIfPresent( aVal, "text", m_PcbSelectionFilter.text );
                SetIfPresent( aVal, "tracks", m_PcbSelectionFilter.tracks );
                SetIfPresent( aVal, "vias", m_PcbSelectionFilter.vias );
                SetIfPresent( aVal, "pads", m_PcbSelectionFilter.pads );
                SetIfPresent( aVal, "graphics", m_PcbSelectionFilter.graphics );
                SetIfPresent( aVal, "zones", m_PcbSelectionFilter.zones );
                SetIfPresent( aVal, "keepouts", m_PcbSelectionFilter.keepouts );
                SetIfPresent( aVal, "dimensions", m_PcbSelectionFilter.dimensions );
                SetIfPresent( aVal, "otherItems", m_PcbSelectionFilter.otherItems );
            },
            {
                { "lockedItems", false },
                { "footprints", true },
                { "text", true },
                { "tracks", true },
                { "vias", true },
                { "pads", true },
                { "graphics", true },
                { "zones", true },
                { "keepouts", true },
                { "dimensions", true },
                { "otherItems", true }
            } ) );

    m_params.emplace_back( new PARAM_ENUM<PCB_LAYER_ID>( "board.active_layer",
                           &m_ActiveLayer, F_Cu, PCBNEW_LAYER_ID_START, F_Fab ) );

    m_params.emplace_back( new PARAM<wxString>( "board.active_layer_preset",
                           &m_ActiveLayerPreset, "" ) );

    m_params.emplace_back( new PARAM_ENUM<HIGH_CONTRAST_MODE>( "board.high_contrast_mode",
                           &m_ContrastModeDisplay, HIGH_CONTRAST_MODE::NORMAL,
                           HIGH_CONTRAST_MODE::NORMAL, HIGH_CONTRAST_MODE::HIDDEN ) );

    m_params.emplace_back( new PARAM<double>( "board.opacity.tracks", &m_TrackOpacity, 1.0 ) );
    m_params.emplace_back( new PARAM<double>( "board.opacity.vias", &m_ViaOpacity, 1.0 ) );
    m_params.emplace_back( new PARAM<double>( "board.opacity.pads", &m_PadOpacity, 1.0 ) );
    m_params.emplace_back( new PARAM<double>( "board.opacity.zones", &m_ZoneOpacity, 0.6 ) );
    m_params.emplace_back( new PARAM<double>( "board.opacity.images", &m_ImageOpacity, 0.6 ) );
    m_params.emplace_back( new PARAM<double>( "board.opacity.shapes", &m_ShapeOpacity, 1.0 ) );

    m_params.emplace_back( new PARAM_LIST<wxString>( "board.hidden_nets", &m_HiddenNets, {} ) );

    m_params.emplace_back( new PARAM_SET<wxString>( "board.hidden_netclasses",
                                                     &m_HiddenNetclasses, {} ) );

    m_params.emplace_back( new PARAM_ENUM<NET_COLOR_MODE>( "board.net_color_mode",
                           &m_NetColorMode, NET_COLOR_MODE::RATSNEST, NET_COLOR_MODE::OFF,
                           NET_COLOR_MODE::ALL ) );

    m_params.emplace_back( new PARAM<bool>( "board.auto_track_width",
                           &m_AutoTrackWidth, true ) );

    m_params.emplace_back( new PARAM_ENUM<ZONE_DISPLAY_MODE>( "board.zone_display_mode",
                           &m_ZoneDisplayMode,
                           ZONE_DISPLAY_MODE::SHOW_FILLED, ZONE_DISPLAY_MODE::SHOW_FILLED,
                           ZONE_DISPLAY_MODE::SHOW_TRIANGULATION ) );

    m_params.emplace_back( new PARAM<bool>( "board.prototype_zone_fills", &m_PrototypeZoneFill, false ) );

    m_params.emplace_back( new PARAM<wxString>( "git.repo_username", &m_GitRepoUsername, "" ) );

    m_params.emplace_back( new PARAM<wxString>( "git.repo_type", &m_GitRepoType, "" ) );

    m_params.emplace_back( new PARAM<wxString>( "git.ssh_key", &m_GitSSHKey, "" ) );

    m_params.emplace_back( new PARAM<bool>( "git.integration_disabled", &m_GitIntegrationDisabled, false ) );

    m_params.emplace_back( new PARAM<wxString>( "net_inspector_panel.filter_text",
                                                &m_NetInspectorPanel.filter_text, "" ) );
    m_params.emplace_back( new PARAM<bool>( "net_inspector_panel.filter_by_net_name",
                                            &m_NetInspectorPanel.filter_by_net_name, true ) );
    m_params.emplace_back( new PARAM<bool>( "net_inspector_panel.filter_by_netclass",
                                            &m_NetInspectorPanel.filter_by_netclass, true ) );
    m_params.emplace_back( new PARAM<bool>( "net_inspector_panel.group_by_netclass",
                                            &m_NetInspectorPanel.group_by_netclass, false ) );
    m_params.emplace_back( new PARAM<bool>( "net_inspector_panel.group_by_constraint",
                                            &m_NetInspectorPanel.group_by_constraint, false ) );
    m_params.emplace_back( new PARAM_LIST<wxString>( "net_inspector_panel.custom_group_rules",
                                                     &m_NetInspectorPanel.custom_group_rules,
                                                     {} ) );
    m_params.emplace_back( new PARAM<bool>( "net_inspector_panel.show_zero_pad_nets",
                                            &m_NetInspectorPanel.show_zero_pad_nets, false ) );
    m_params.emplace_back( new PARAM<bool>( "net_inspector_panel.show_unconnected_nets",
                                            &m_NetInspectorPanel.show_unconnected_nets, false ) );
    m_params.emplace_back( new PARAM<bool>( "net_inspector_panel.show_time_domain_details",
                                            &m_NetInspectorPanel.show_time_domain_details, false ) );
    m_params.emplace_back( new PARAM<int>( "net_inspector_panel.sorting_column",
                                           &m_NetInspectorPanel.sorting_column, -1 ) );
    m_params.emplace_back( new PARAM<bool>( "net_inspector_panel.sort_ascending",
                                            &m_NetInspectorPanel.sort_order_asc, true ) );
    m_params.emplace_back( new PARAM_LIST<int>( "net_inspector_panel.col_order",
                                                &m_NetInspectorPanel.col_order, {} ) );
    m_params.emplace_back( new PARAM_LIST<int>( "net_inspector_panel.col_widths",
                                                &m_NetInspectorPanel.col_widths, {} ) );
    m_params.emplace_back( new PARAM_LIST<bool>( "net_inspector_panel.col_hidden",
                                                 &m_NetInspectorPanel.col_hidden, {} ) );
    m_params.emplace_back( new PARAM_LIST<wxString>( "net_inspector_panel.expanded_rows",
                                                     &m_NetInspectorPanel.expanded_rows, {} ) );

    m_params.emplace_back( new PARAM_LIST<wxString>( "open_jobsets", &m_OpenJobSets, {} ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "project.files",
            [&]() -> nlohmann::json
            {
                nlohmann::json ret = nlohmann::json::array();

                for( PROJECT_FILE_STATE& fileState : m_files )
                {
                    nlohmann::json file;
                    file["name"] = fileState.fileName;
                    file["open"] = fileState.open;

                    nlohmann::json window;
                    window["maximized"] = fileState.window.maximized;
                    window["size_x"]    = fileState.window.size_x;
                    window["size_y"]    = fileState.window.size_y;
                    window["pos_x"]     = fileState.window.pos_x;
                    window["pos_y"]     = fileState.window.pos_y;
                    window["display"]   = fileState.window.display;

                    file["window"] = window;

                    ret.push_back( file );
                }

                return ret;
            },
            [&]( const nlohmann::json& aVal )
            {
                if( !aVal.is_array() || aVal.empty() )
                    return;

                m_files.clear();

                for( const nlohmann::json& file : aVal )
                {
                    PROJECT_FILE_STATE fileState;

                    try
                    {
                        SetIfPresent( file, "name", fileState.fileName );
                        SetIfPresent( file, "open", fileState.open );
                        SetIfPresent( file, "window.size_x", fileState.window.size_x );
                        SetIfPresent( file, "window.size_y", fileState.window.size_y );
                        SetIfPresent( file, "window.pos_x", fileState.window.pos_x );
                        SetIfPresent( file, "window.pos_y", fileState.window.pos_y );
                        SetIfPresent( file, "window.maximized", fileState.window.maximized );
                        SetIfPresent( file, "window.display", fileState.window.display );

                        m_files.push_back( fileState );
                    }
                    catch( ... )
                    {
                        // Non-integer or out of range entry in the array; ignore
                    }
                }

            },
            {
            } ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "schematic.selection_filter",
            [&]() -> nlohmann::json
            {
                nlohmann::json ret;

                ret["lockedItems"] = m_SchSelectionFilter.lockedItems;
                ret["symbols"]     = m_SchSelectionFilter.symbols;
                ret["text"]        = m_SchSelectionFilter.text;
                ret["wires"]       = m_SchSelectionFilter.wires;
                ret["labels"]      = m_SchSelectionFilter.labels;
                ret["pins"]        = m_SchSelectionFilter.pins;
                ret["graphics"]    = m_SchSelectionFilter.graphics;
                ret["images"]      = m_SchSelectionFilter.images;
                ret["ruleAreas"]   = m_SchSelectionFilter.ruleAreas;
                ret["otherItems"]  = m_SchSelectionFilter.otherItems;

                return ret;
            },
            [&]( const nlohmann::json& aVal )
            {
                if( aVal.empty() || !aVal.is_object() )
                    return;

                SetIfPresent( aVal, "lockedItems", m_SchSelectionFilter.lockedItems );
                SetIfPresent( aVal, "symbols", m_SchSelectionFilter.symbols );
                SetIfPresent( aVal, "text", m_SchSelectionFilter.text );
                SetIfPresent( aVal, "wires", m_SchSelectionFilter.wires );
                SetIfPresent( aVal, "labels", m_SchSelectionFilter.labels );
                SetIfPresent( aVal, "pins", m_SchSelectionFilter.pins );
                SetIfPresent( aVal, "graphics", m_SchSelectionFilter.graphics );
                SetIfPresent( aVal, "images", m_SchSelectionFilter.images );
                SetIfPresent( aVal, "ruleAreas", m_SchSelectionFilter.ruleAreas );
                SetIfPresent( aVal, "otherItems", m_SchSelectionFilter.otherItems );
            },
            {
                { "lockedItems", false },
                { "symbols", true },
                { "text", true },
                { "wires", true },
                { "labels", true },
                { "pins", true },
                { "graphics", true },
                { "images", true },
                { "ruleAreas", true },
                { "otherItems", true }
            } ) );

    m_params.emplace_back( new PARAM_LIST<wxString>( "schematic.hierarchy_collapsed",
                                                    &m_SchHierarchyCollapsed, {} ) );

    registerMigration( 1, 2,
            [&]()
            {
                /**
                 * Schema version 1 to 2:
                 * LAYER_PADS and LAYER_ZONES added to visibility controls
                 */

                std::string ptr( "board.visible_items" );

                if( Contains( ptr ) )
                {
                    if( At( ptr ).is_array() )
                    {
                        At( ptr ).push_back( LAYER_PADS - GAL_LAYER_ID_START );
                        At( ptr ).push_back( LAYER_ZONES - GAL_LAYER_ID_START );
                    }
                    else
                    {
                        At( "board" ).erase( "visible_items" );
                    }

                    m_wasMigrated = true;
                }

                return true;
            } );

    registerMigration( 2, 3,
            [&]()
            {
                /**
                 * Schema version 2 to 3:
                 * Fix issue with object visibility not migrating from legacy, which required
                 * remapping of GAL_LAYER_ID to match the legacy bitmask ordering.
                 */

                /// Stores a mapping from old to new enum offset
                const std::map<int, int> offsets = {
                        { 22, 34 },    // LAYER_PAD_HOLEWALLS
                        { 23, 22 },    // LAYER_VIA_HOLES
                        { 24, 35 },    // LAYER_VIA_HOLEWALLS
                        { 25, 23 },    // LAYER_DRC_ERROR
                        { 26, 36 },    // LAYER_DRC_WARNING
                        { 27, 37 },    // LAYER_DRC_EXCLUSION
                        { 28, 38 },    // LAYER_MARKER_SHADOWS
                        { 29, 24 },    // LAYER_DRAWINGSHEET
                        { 30, 25 },    // LAYER_GP_OVERLAY
                        { 31, 26 },    // LAYER_SELECT_OVERLAY
                        { 32, 27 },    // LAYER_PCB_BACKGROUND
                        { 33, 28 },    // LAYER_CURSOR
                        { 34, 29 },    // LAYER_AUX_ITEM
                        { 35, 30 },    // LAYER_DRAW_BITMAPS
                        { 39, 32 },    // LAYER_PADS
                        { 40, 33 },    // LAYER_ZONES
                    };

                std::string ptr( "board.visible_items" );

                if( Contains( ptr ) && At( ptr ).is_array() )
                {
                    nlohmann::json visible = nlohmann::json::array();

                    for( const nlohmann::json& val : At( ptr ) )
                    {
                        try
                        {
                            int layer = val.get<int>();

                            if( offsets.count( layer ) )
                                visible.push_back( offsets.at( layer ) );
                            else
                                visible.push_back( layer );
                        }
                        catch( ... )
                        {
                            // skip invalid value
                        }
                    }

                    At( "board" )["visible_items"] = visible;
                    m_wasMigrated = true;
                }

                return true;
            } );

    registerMigration( 3, 4,
            [&]()
            {
                // Schema version 3 to 4: LAYER_FILLED_SHAPES added to visibility controls

                std::string ptr( "board.visible_items" );

                if( Contains( ptr ) )
                {
                    if( At( ptr ).is_array() && !At( ptr ).empty() )
                        At( ptr ).push_back( LAYER_FILLED_SHAPES - GAL_LAYER_ID_START );
                    else
                        At( "board" ).erase( "visible_items" );

                    m_wasMigrated = true;
                }

                return true;
            } );

    registerMigration( 4, 5,
            [&]()
            {
                // Schema version 5: use named render layers

                std::string ptr( "board.visible_items" );

                if( Contains( ptr ) && At( ptr ).is_array() )
                {
                    std::vector<std::string> newLayers;

                    for( nlohmann::json& entry : At( ptr ) )
                    {
                        if( !entry.is_number_integer() )
                            continue;

                        if( std::optional<VISIBILITY_LAYER> vl =
                            VisibilityLayerFromRenderLayer( GAL_LAYER_ID_START + entry.get<int>() ) )
                        {
                            newLayers.emplace_back( VisibilityLayerToString( *vl ) );
                        }
                    }

                    At( ptr ) = newLayers;
                    m_wasMigrated = true;
                }

                return true;
            } );
}


bool PROJECT_LOCAL_SETTINGS::MigrateFromLegacy( wxConfigBase* aLegacyConfig )
{
    /**
     * The normal legacy migration code won't be used for this because the only legacy
     * information stored here was stored in board files, so we do that migration when loading
     * the board.
     */
    return true;
}


bool PROJECT_LOCAL_SETTINGS::SaveToFile( const wxString& aDirectory, bool aForce )
{
    wxASSERT( m_project );

    Set( "meta.filename",
         m_project->GetProjectName() + "." + FILEEXT::ProjectLocalSettingsFileExtension );

    // Even if parameters were not modified, we should resave after migration
    bool force = aForce || m_wasMigrated;

    // If we're actually going ahead and doing the save, the flag that keeps code from doing the
    // save should be cleared at this point.
    m_wasMigrated = false;

    return JSON_SETTINGS::SaveToFile( aDirectory, force );
}


bool PROJECT_LOCAL_SETTINGS::SaveAs( const wxString& aDirectory, const wxString& aFile )
{
    Set( "meta.filename", aFile + "." + FILEEXT::ProjectLocalSettingsFileExtension );
    SetFilename( aFile );

    // If we're actually going ahead and doing the save, the flag that keeps code from doing the
    // save should be cleared at this point.
    m_wasMigrated = false;

    return JSON_SETTINGS::SaveToFile( aDirectory, true );
}


const PROJECT_FILE_STATE* PROJECT_LOCAL_SETTINGS::GetFileState( const wxString& aFileName )
{
    auto it = std::find_if( m_files.begin(), m_files.end(),
                            [&aFileName]( const PROJECT_FILE_STATE &a )
                            {
                                return a.fileName == aFileName;
                            } );

    if( it != m_files.end() )
    {
        return &( *it );
    }

    return nullptr;
}


void PROJECT_LOCAL_SETTINGS::SaveFileState( const wxString& aFileName,
                                            const WINDOW_SETTINGS* aWindowCfg, bool aOpen )
{
    auto it = std::find_if( m_files.begin(), m_files.end(),
                            [&aFileName]( const PROJECT_FILE_STATE& a )
                            {
                                return a.fileName == aFileName;
                            } );

    if( it == m_files.end() )
    {
        PROJECT_FILE_STATE fileState;
        fileState.fileName = aFileName;
        fileState.open = false;
        fileState.window.maximized = false;
        fileState.window.size_x = -1;
        fileState.window.size_y = -1;
        fileState.window.pos_x = -1;
        fileState.window.pos_y = -1;
        fileState.window.display = 0;

        m_files.push_back( fileState );

        it = m_files.end() - 1;
    }

    ( *it ).window = aWindowCfg->state;
    ( *it ).open   = aOpen;
}


void PROJECT_LOCAL_SETTINGS::ClearFileState()
{
    m_files.clear();
}
