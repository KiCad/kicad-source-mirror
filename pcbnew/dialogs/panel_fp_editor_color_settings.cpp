/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
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

#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <footprint_editor_settings.h>
#include <layer_ids.h>
#include <panel_fp_editor_color_settings.h>


PANEL_FP_EDITOR_COLOR_SETTINGS::PANEL_FP_EDITOR_COLOR_SETTINGS( wxWindow* aParent ) :
        PANEL_COLOR_SETTINGS( aParent )
{
     // Currently this only applies to eeschema
    m_optOverrideColors->Hide();

    m_colorNamespace = "board";

    FOOTPRINT_EDITOR_SETTINGS* cfg = GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" );
    COLOR_SETTINGS*            current = ::GetColorSettings( cfg ? cfg->m_ColorTheme : DEFAULT_THEME );

    // Store the current settings before reloading below
    current->Store();
    Pgm().GetSettingsManager().SaveColorSettings( current, "board" );

    m_optOverrideColors->SetValue( current->GetOverrideSchItemColors() );

    m_currentSettings = new COLOR_SETTINGS( *current );

    Pgm().GetSettingsManager().ReloadColorSettings();
    createThemeList( cfg ? cfg->m_ColorTheme : DEFAULT_THEME );

    m_validLayers.push_back( F_Cu );
    m_validLayers.push_back( In1_Cu );  // "Internal Layers"
    m_validLayers.push_back( B_Cu );

    for( int id = GAL_LAYER_ID_START; id < GAL_LAYER_ID_END; id++ )
    {
        if( id == LAYER_VIAS
         || id == LAYER_VIA_HOLES
         || id == LAYER_VIA_HOLEWALLS
         || id == LAYER_PAD_PLATEDHOLES
         || id == LAYER_PAD_HOLEWALLS )
        {
            continue;
        }

        m_validLayers.push_back( id );
    }

    m_backgroundLayer = LAYER_PCB_BACKGROUND;
}


PANEL_FP_EDITOR_COLOR_SETTINGS::~PANEL_FP_EDITOR_COLOR_SETTINGS()
{
    delete m_currentSettings;
}


bool PANEL_FP_EDITOR_COLOR_SETTINGS::TransferDataFromWindow()
{
    if( FOOTPRINT_EDITOR_SETTINGS* cfg = GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" ) )
        cfg->m_ColorTheme = m_currentSettings->GetFilename();

    return true;
}


bool PANEL_FP_EDITOR_COLOR_SETTINGS::TransferDataToWindow()
{
    return true;
}


void PANEL_FP_EDITOR_COLOR_SETTINGS::createSwatches()
{
    std::vector<GAL_LAYER_ID> galLayers;

    // Sort the gal layers by name
    for( int i : m_validLayers )
    {
        if( i >= GAL_LAYER_ID_START && m_currentSettings->GetColor( i ) != COLOR4D::UNSPECIFIED )
            galLayers.push_back( (GAL_LAYER_ID) i );
    }

    std::sort( galLayers.begin(), galLayers.end(),
               []( int a, int b )
               {
                   return LayerName( a ) < LayerName( b );
               } );

    createSwatch( F_Cu, LayerName( F_Cu ) );
    createSwatch( In1_Cu, _( "Internal Layers" ) );
    createSwatch( B_Cu, LayerName( B_Cu ) );

    for( GAL_LAYER_ID layer : galLayers )
        createSwatch( layer, LayerName( layer ) );

    Layout();
}
