/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <regex>

#include <class_board.h>
#include <gal/gal_display_options.h>
#include <layers_id_colors_and_visibility.h>
#include <panel_pcbnew_color_settings.h>
#include <pcbnew_settings.h>
#include <pcb_edit_frame.h>
#include <pcb_layer_widget.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>


PANEL_PCBNEW_COLOR_SETTINGS::PANEL_PCBNEW_COLOR_SETTINGS( PCB_EDIT_FRAME* aFrame,
                                                          wxWindow* aParent )
        : PANEL_COLOR_SETTINGS( aParent ),
          m_frame( aFrame ),
          m_page( nullptr ),
          m_titleBlock( nullptr ),
          m_ws( nullptr )
{
     // Currently this only applies to eeschema
    m_optOverrideColors->Hide();

    m_colorNamespace = "board";

    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();

    PCBNEW_SETTINGS* app_settings = mgr.GetAppSettings<PCBNEW_SETTINGS>();
    COLOR_SETTINGS*  current      = mgr.GetColorSettings( app_settings->m_ColorTheme );

    // Store the current settings before reloading below
    current->Store();
    mgr.SaveColorSettings( current, "board" );

    m_optOverrideColors->SetValue( current->GetOverrideSchItemColors() );

    m_currentSettings = new COLOR_SETTINGS( *current );

    mgr.ReloadColorSettings();
    createThemeList( app_settings->m_ColorTheme );

    for( int id = F_Cu; id < PCB_LAYER_ID_COUNT; id++ )
        m_validLayers.push_back( id );

    for( int id = GAL_LAYER_ID_START; id < GAL_LAYER_ID_END; id++ )
    {
        if( id == LAYER_VIAS || id == LAYER_GRID_AXES || id == LAYER_PADS_PLATEDHOLES
                || id == LAYER_VIAS_HOLES )
        {
            continue;
        }

        m_validLayers.push_back( id );
    }

    m_backgroundLayer = LAYER_PCB_BACKGROUND;

    m_colorsMainSizer->Insert( 0, 10, 0, 0, wxEXPAND, 5 );

    createSwatches();
}


PANEL_PCBNEW_COLOR_SETTINGS::~PANEL_PCBNEW_COLOR_SETTINGS()
{
    delete m_page;
    delete m_titleBlock;
}


bool PANEL_PCBNEW_COLOR_SETTINGS::TransferDataFromWindow()
{
    m_currentSettings->SetOverrideSchItemColors( m_optOverrideColors->GetValue() );

    if( !saveCurrentTheme( true ) )
        return false;

    m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings()->LoadColors( m_currentSettings );

    SETTINGS_MANAGER& settingsMgr = Pgm().GetSettingsManager();
    PCBNEW_SETTINGS* app_settings = settingsMgr.GetAppSettings<PCBNEW_SETTINGS>();
    app_settings->m_ColorTheme = m_currentSettings->GetFilename();

    m_frame->GetLayerManager()->SyncLayerColors();

    return true;
}


bool PANEL_PCBNEW_COLOR_SETTINGS::TransferDataToWindow()
{
    return true;
}


void PANEL_PCBNEW_COLOR_SETTINGS::createSwatches()
{
    std::vector<int> layers;

    for( GAL_LAYER_ID i = GAL_LAYER_ID_START; i < GAL_LAYER_ID_END; ++i )
    {
        if( m_currentSettings->GetColor( i ) != COLOR4D::UNSPECIFIED )
            layers.push_back( i );
    }

    std::sort( layers.begin(), layers.end(),
               []( int a, int b )
               {
                   return LayerName( a ) < LayerName( b );
               } );

    // Don't sort board layers by name
    for( int i = PCBNEW_LAYER_ID_START; i < PCB_LAYER_ID_COUNT; ++i )
        layers.insert( layers.begin() + i, i );

    BOARD* board = m_frame->GetBoard();

    for( int layer : layers )
    {
        wxString name = LayerName( layer );

        if( board && layer >= PCBNEW_LAYER_ID_START && layer < PCB_LAYER_ID_COUNT )
            name = board->GetLayerName( static_cast<PCB_LAYER_ID>( layer ) );

        createSwatch( layer, name );
    }

    // Give a minimal width to m_colorsListWindow, in order to always having
    // a full row shown
    int min_width = m_colorsGridSizer->GetMinSize().x;
    const int margin = 20;  // A margin around the sizer
    m_colorsListWindow->SetMinSize( wxSize( min_width + margin, -1 ) );
}
