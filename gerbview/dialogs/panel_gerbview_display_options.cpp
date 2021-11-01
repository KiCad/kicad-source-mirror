/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <vector>
#include <core/arraydim.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <gerbview_settings.h>
#include <widgets/gal_options_panel.h>
#include "panel_gerbview_display_options.h"


/// List of page sizes
static const wxChar* gerberPageSizeList[] =
{
    wxT( "GERBER" ),    // index 0: full size page selection
    wxT( "A4" ),
    wxT( "A3" ),
    wxT( "A2" ),
    wxT( "A" ),
    wxT( "B" ),
    wxT( "C" ),
};


PANEL_GERBVIEW_DISPLAY_OPTIONS::PANEL_GERBVIEW_DISPLAY_OPTIONS( wxWindow* aParent ) :
    PANEL_GERBVIEW_DISPLAY_OPTIONS_BASE( aParent, wxID_ANY )
{
    GERBVIEW_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<GERBVIEW_SETTINGS>();

    m_galOptsPanel = new GAL_OPTIONS_PANEL( this, cfg );
    m_galOptionsSizer->Add( m_galOptsPanel, 0, wxEXPAND | wxLEFT, 5 );
}


void PANEL_GERBVIEW_DISPLAY_OPTIONS::loadSettings( GERBVIEW_SETTINGS* aCfg )
{
    // Show Option Draw polygons
    m_OptDisplayPolygons->SetValue( !aCfg->m_Display.m_DisplayPolygonsFill );

    // Show Option Draw Lines. We use DisplayPcbTrackFill as Lines draw option
    m_OptDisplayLines->SetValue( !aCfg->m_Display.m_DisplayLinesFill );
    m_OptDisplayFlashedItems->SetValue( !aCfg->m_Display.m_DisplayFlashedItemsFill );
    m_OptDisplayDCodes->SetValue( aCfg->m_Appearance.show_dcodes );

    for( unsigned i = 0;  i < arrayDim( gerberPageSizeList );  ++i )
    {
        if( gerberPageSizeList[i] == aCfg->m_Appearance.page_type )
        {
            m_PageSize->SetSelection( i );
            break;
        }
    }

    m_ShowPageLimitsOpt->SetValue( aCfg->m_Display.m_DisplayPageLimits );
}


bool PANEL_GERBVIEW_DISPLAY_OPTIONS::TransferDataToWindow()
{
    m_galOptsPanel->TransferDataToWindow();

    GERBVIEW_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<GERBVIEW_SETTINGS>();

    loadSettings( cfg );

    return true;
}


bool PANEL_GERBVIEW_DISPLAY_OPTIONS::TransferDataFromWindow()
{
    GERBVIEW_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<GERBVIEW_SETTINGS>();

    m_galOptsPanel->TransferDataFromWindow();

    cfg->m_Display.m_DisplayLinesFill = !m_OptDisplayLines->GetValue();
    cfg->m_Display.m_DisplayFlashedItemsFill = !m_OptDisplayFlashedItems->GetValue();
    cfg->m_Display.m_DisplayPolygonsFill = !m_OptDisplayPolygons->GetValue();
    cfg->m_Appearance.show_dcodes = m_OptDisplayDCodes->GetValue();

    cfg->m_Appearance.page_type = gerberPageSizeList[ m_PageSize->GetSelection() ];
    cfg->m_Display.m_DisplayPageLimits = m_ShowPageLimitsOpt->GetValue();

    return true;
}


void PANEL_GERBVIEW_DISPLAY_OPTIONS::ResetPanel()
{
    GERBVIEW_SETTINGS cfg;
    cfg.Load();             // Loading without a file will init to defaults

    loadSettings( &cfg );

    m_galOptsPanel->ResetPanel( &cfg );
}


