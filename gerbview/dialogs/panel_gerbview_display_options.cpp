/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
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

#include <vector>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <gerbview_settings.h>
#include <dialogs/panel_gal_options.h>
#include "panel_gerbview_display_options.h"


PANEL_GERBVIEW_DISPLAY_OPTIONS::PANEL_GERBVIEW_DISPLAY_OPTIONS( wxWindow* aParent ) :
     PANEL_GERBVIEW_DISPLAY_OPTIONS_BASE( aParent, wxID_ANY )
{
    GERBVIEW_SETTINGS* cfg = GetAppSettings<GERBVIEW_SETTINGS>( "gerbview" );

    m_galOptsPanel = new PANEL_GAL_OPTIONS( this, cfg );
    m_galOptionsSizer->Add( m_galOptsPanel, 0, wxEXPAND|wxRIGHT, 15 );
}


void PANEL_GERBVIEW_DISPLAY_OPTIONS::loadSettings( GERBVIEW_SETTINGS* aCfg )
{
    // Show Option Draw polygons
    m_OptDisplayPolygons->SetValue( !aCfg->m_Display.m_DisplayPolygonsFill );

    // Show Option Draw Lines. We use DisplayPcbTrackFill as Lines draw option
    m_OptDisplayLines->SetValue( !aCfg->m_Display.m_DisplayLinesFill );
    m_OptDisplayFlashedItems->SetValue( !aCfg->m_Display.m_DisplayFlashedItemsFill );
    m_OptDisplayDCodes->SetValue( aCfg->m_Appearance.show_dcodes );
    m_spOpacityCtrl->SetValue( aCfg->m_Display.m_OpacityModeAlphaValue );

    if( aCfg->m_Appearance.page_type == wxT( "A4" ) )
        m_pageSizeA4->SetValue( true );
    else if( aCfg->m_Appearance.page_type == wxT( "A3" ) )
        m_pageSizeA3->SetValue( true );
    else if( aCfg->m_Appearance.page_type == wxT( "A2" ) )
        m_pageSizeA2->SetValue( true );
    else if( aCfg->m_Appearance.page_type == wxT( "A" ) )
        m_pageSizeA->SetValue( true );
    else if( aCfg->m_Appearance.page_type == wxT( "B" ) )
        m_pageSizeB->SetValue( true );
    else if( aCfg->m_Appearance.page_type == wxT( "C" ) )
        m_pageSizeC->SetValue( true );
    else
        m_pageSizeFull->SetValue( true );

    m_ShowPageLimitsOpt->SetValue( aCfg->m_Display.m_DisplayPageLimits );
}


bool PANEL_GERBVIEW_DISPLAY_OPTIONS::TransferDataToWindow()
{
    m_galOptsPanel->TransferDataToWindow();

    loadSettings( GetAppSettings<GERBVIEW_SETTINGS>( "gerbview" ) );

    return true;
}


bool PANEL_GERBVIEW_DISPLAY_OPTIONS::TransferDataFromWindow()
{
    m_galOptsPanel->TransferDataFromWindow();

    if( GERBVIEW_SETTINGS* cfg = GetAppSettings<GERBVIEW_SETTINGS>( "gerbview" ) )
    {
        cfg->m_Display.m_DisplayLinesFill = !m_OptDisplayLines->GetValue();
        cfg->m_Display.m_DisplayFlashedItemsFill = !m_OptDisplayFlashedItems->GetValue();
        cfg->m_Display.m_DisplayPolygonsFill = !m_OptDisplayPolygons->GetValue();
        cfg->m_Appearance.show_dcodes = m_OptDisplayDCodes->GetValue();

        // clang-format off
        if(      m_pageSizeA4->GetValue() ) cfg->m_Appearance.page_type = wxT( "A4" );
        else if( m_pageSizeA3->GetValue() ) cfg->m_Appearance.page_type = wxT( "A3" );
        else if( m_pageSizeA2->GetValue() ) cfg->m_Appearance.page_type = wxT( "A2" );
        else if( m_pageSizeA->GetValue() )  cfg->m_Appearance.page_type = wxT( "A" );
        else if( m_pageSizeB->GetValue() )  cfg->m_Appearance.page_type = wxT( "B" );
        else if( m_pageSizeC->GetValue() )  cfg->m_Appearance.page_type = wxT( "C" );
        else                                cfg->m_Appearance.page_type = wxT( "GERBER" );
        // clang-format on

        cfg->m_Display.m_DisplayPageLimits = m_ShowPageLimitsOpt->GetValue();
        cfg->m_Display.m_OpacityModeAlphaValue = m_spOpacityCtrl->GetValue();
    }

    return true;
}


void PANEL_GERBVIEW_DISPLAY_OPTIONS::ResetPanel()
{
    GERBVIEW_SETTINGS cfg;
    cfg.Load();             // Loading without a file will init to defaults

    loadSettings( &cfg );

    m_galOptsPanel->ResetPanel( &cfg );
}


