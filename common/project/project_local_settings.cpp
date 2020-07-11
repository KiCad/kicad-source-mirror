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

#include <project.h>
#include <project/project_local_settings.h>
#include <settings/parameters.h>

const int projectLocalSettingsVersion = 1;


PROJECT_LOCAL_SETTINGS::PROJECT_LOCAL_SETTINGS( const std::string& aFilename ) :
        JSON_SETTINGS( aFilename, SETTINGS_LOC::PROJECT, projectLocalSettingsVersion,
                       /* aCreateIfMissing = */ true, /* aCreateIfDefault = */ false,
                       /* aWriteFile = */ true ),
        m_project( nullptr ),
        m_SelectionFilter()
{
    m_params.emplace_back( new PARAM_LAMBDA<std::string>( "board.visible_layers",
            [&]() -> std::string
            {
                return m_VisibleLayers.FmtHex();
            },
            [&]( const std::string& aString )
            {
                m_VisibleLayers.ParseHex( aString.c_str(), aString.size() );
            },
            LSET::AllLayersMask().FmtHex() ) );

    static GAL_SET defaultVisible;
    defaultVisible.set().reset( GAL_LAYER_INDEX( LAYER_MOD_TEXT_INVISIBLE ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "board.visible_items",
            [&]() -> nlohmann::json
            {
                nlohmann::json ret = nlohmann::json::array();

                for( size_t i = 0; i < m_VisibleItems.size(); i++ )
                    if( m_VisibleItems.test( i ) )
                        ret.push_back( i );

                return ret;
            },
            [&]( const nlohmann::json& aVal )
            {
                if( !aVal.is_array() || aVal.empty() )
                {
                    m_VisibleItems = defaultVisible;
                    return;
                }

                m_VisibleItems.reset();

                for( const nlohmann::json& entry : aVal )
                {
                    try
                    {
                        int i = entry.get<int>();
                        m_VisibleItems.set( i );
                    }
                    catch( ... )
                    {
                        // Non-integer or out of range entry in the array; ignore
                    }
                }
            },
            {} ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "board.selection_filter",
            [&]() -> nlohmann::json
            {
                nlohmann::json ret;

                ret["lockedItems"] = m_SelectionFilter.lockedItems;
                ret["footprints"]  = m_SelectionFilter.footprints;
                ret["text"]        = m_SelectionFilter.text;
                ret["tracks"]      = m_SelectionFilter.tracks;
                ret["vias"]        = m_SelectionFilter.vias;
                ret["pads"]        = m_SelectionFilter.pads;
                ret["graphics"]    = m_SelectionFilter.graphics;
                ret["zones"]       = m_SelectionFilter.zones;
                ret["keepouts"]    = m_SelectionFilter.keepouts;
                ret["dimensions"]  = m_SelectionFilter.dimensions;
                ret["otherItems"]  = m_SelectionFilter.otherItems;

                return ret;
            },
            [&]( const nlohmann::json& aVal )
            {
                if( aVal.empty() || !aVal.is_object() )
                    return;

                auto setIfPresent =
                        [&aVal]( const std::string& aKey, bool& aTarget )
                        {
                            if( aVal.contains( aKey ) && aVal.at( aKey ).is_boolean() )
                                aTarget = aVal.at( aKey ).get<bool>();
                        };

                setIfPresent( "lockedItems", m_SelectionFilter.lockedItems );
                setIfPresent( "footprints", m_SelectionFilter.footprints );
                setIfPresent( "text", m_SelectionFilter.text );
                setIfPresent( "tracks", m_SelectionFilter.tracks );
                setIfPresent( "vias", m_SelectionFilter.vias );
                setIfPresent( "pads", m_SelectionFilter.pads );
                setIfPresent( "graphics", m_SelectionFilter.graphics );
                setIfPresent( "zones", m_SelectionFilter.zones );
                setIfPresent( "keepouts", m_SelectionFilter.keepouts );
                setIfPresent( "dimensions", m_SelectionFilter.dimensions );
                setIfPresent( "otherItems", m_SelectionFilter.otherItems );
            },
            {
                { "lockedItems", true },
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

    m_params.emplace_back( new PARAM_ENUM<PCB_LAYER_ID>(
            "board.active_layer", &m_ActiveLayer, F_Cu, PCBNEW_LAYER_ID_START, F_Fab ) );

    m_params.emplace_back( new PARAM<wxString>( "board.active_layer_preset",
            &m_ActiveLayerPreset, "" ) );

    m_params.emplace_back( new PARAM_ENUM<HIGH_CONTRAST_MODE>( "board.high_contrast_mode",
            &m_ContrastModeDisplay, HIGH_CONTRAST_MODE::NORMAL, HIGH_CONTRAST_MODE::NORMAL,
            HIGH_CONTRAST_MODE::HIDDEN ) );

    m_params.emplace_back( new PARAM_LIST<wxString>( "board.hidden_nets", &m_HiddenNets, {} ) );
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


bool PROJECT_LOCAL_SETTINGS::SaveToFile( const std::string& aDirectory, bool aForce )
{
    wxASSERT( m_project );

    ( *this )[PointerFromString( "meta.filename" )] =
            m_project->GetProjectName() + "." + ProjectLocalSettingsFileExtension;

    return JSON_SETTINGS::SaveToFile( aDirectory, aForce );
}
