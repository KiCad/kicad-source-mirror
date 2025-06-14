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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <pgm_base.h>
#include <layer_ids.h>
#include <panel_gerbview_color_settings.h>
#include <settings/settings_manager.h>
#include <gerbview_settings.h>
#include <gerbview_frame.h>
#include <widgets/gerbview_layer_widget.h>

#include <wx/log.h>
PANEL_GERBVIEW_COLOR_SETTINGS::PANEL_GERBVIEW_COLOR_SETTINGS( wxWindow* aParent ) :
        PANEL_COLOR_SETTINGS( aParent )
{
    m_colorNamespace = "gerbview";

    GERBVIEW_SETTINGS* cfg     = GetAppSettings<GERBVIEW_SETTINGS>( "gerbview" );
    COLOR_SETTINGS*    current = ::GetColorSettings( cfg ? cfg->m_ColorTheme : DEFAULT_THEME );

    // Colors can also be modified from the LayersManager, so collect last settings if exist
    // (They can be no yet saved on disk)
    if( GERBVIEW_FRAME* frame = dynamic_cast<GERBVIEW_FRAME*>( wxWindow::FindWindowByName( GERBVIEW_FRAME_NAME ) ) )
        frame->m_LayersManager->CollectCurrentColorSettings( current );

    // Saved theme doesn't exist?  Reset to default
    if( cfg && current->GetFilename() != cfg->m_ColorTheme )
        cfg->m_ColorTheme = current->GetFilename();

    createThemeList( cfg ? cfg->m_ColorTheme : DEFAULT_THEME );

    // Currently this only applies to eeschema
    m_optOverrideColors->Hide();

    m_currentSettings = new COLOR_SETTINGS( *current );

    for( int i = GERBVIEW_LAYER_ID_START; i < GERBVIEW_LAYER_ID_START + GERBER_DRAWLAYERS_COUNT; i++ )
        m_validLayers.push_back( i );

    for( int i = LAYER_DCODES; i < GERBVIEW_LAYER_ID_END; i++ )
        m_validLayers.push_back( i );

    m_backgroundLayer = LAYER_GERBVIEW_BACKGROUND;
}


PANEL_GERBVIEW_COLOR_SETTINGS::~PANEL_GERBVIEW_COLOR_SETTINGS()
{
    delete m_currentSettings;
}


bool PANEL_GERBVIEW_COLOR_SETTINGS::TransferDataFromWindow()
{
    if( GERBVIEW_SETTINGS* cfg = GetAppSettings<GERBVIEW_SETTINGS>( "gerbview" ) )
        cfg->m_ColorTheme = m_currentSettings->GetFilename();

    return true;
}


bool PANEL_GERBVIEW_COLOR_SETTINGS::TransferDataToWindow()
{
    return true;
}


void PANEL_GERBVIEW_COLOR_SETTINGS::createSwatches()
{
    wxString layerName;

    for( int layer : m_validLayers )
    {
        switch( layer )
        {
        case LAYER_DCODES:                layerName = _( "DCodes" );           break;
        case LAYER_NEGATIVE_OBJECTS:      layerName = _( "Negative Objects" ); break;
        case LAYER_GERBVIEW_GRID:         layerName = _( "Grid" );             break;
        case LAYER_GERBVIEW_AXES:         layerName = _( "Axes" );             break;
        case LAYER_GERBVIEW_DRAWINGSHEET: layerName = _( "Drawing Sheet" );    break;
        case LAYER_GERBVIEW_PAGE_LIMITS:  layerName = _( "Page Limits" );      break;
        case LAYER_GERBVIEW_BACKGROUND:   layerName = _( "Background" );       break;

        default:
            layerName = wxString::Format( _( "Graphic Layer %d" ), layer + 1 - GERBVIEW_LAYER_ID_START );
            break;
        }

        createSwatch( layer, layerName );
    }

    Layout();
}


