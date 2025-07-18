/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <pcbnew_settings.h>
#include <footprint_editor_settings.h>
#include <panel_pcbnew_display_origin.h>


PANEL_PCBNEW_DISPLAY_ORIGIN::PANEL_PCBNEW_DISPLAY_ORIGIN( wxWindow* aParent, APP_SETTINGS_BASE* aCfg,
                                                          FRAME_T aFrameType ) :
        PANEL_PCBNEW_DISPLAY_ORIGIN_BASE( aParent ),
        m_cfg( aCfg ),
        m_frameType( aFrameType )
{
    m_displayOrigin->Show( m_frameType == FRAME_PCB_EDITOR );
}


void PANEL_PCBNEW_DISPLAY_ORIGIN::loadSettings( APP_SETTINGS_BASE* aCfg )
{
    if( m_frameType == FRAME_FOOTPRINT_EDITOR )
    {
        FOOTPRINT_EDITOR_SETTINGS* cfg = GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" );

        if( cfg && cfg->m_DisplayInvertXAxis )
            m_xIncreasesLeft->SetValue( true );
        else
            m_xIncreasesRight->SetValue( true );

        if( cfg && cfg->m_DisplayInvertYAxis )
            m_yIncreasesUp->SetValue( true );
        else
            m_yIncreasesDown->SetValue( true );
    }
    else
    {
        PCBNEW_SETTINGS* cfg = GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" );

        if( cfg && cfg->m_Display.m_DisplayOrigin == PCB_DISPLAY_ORIGIN::PCB_ORIGIN_PAGE )
            m_pageOrigin->SetValue( true );
        else if( cfg && cfg->m_Display.m_DisplayOrigin == PCB_DISPLAY_ORIGIN::PCB_ORIGIN_GRID )
            m_gridOrigin->SetValue( true );
        else
            m_drillPlaceOrigin->SetValue( true );

        if( cfg &&cfg->m_Display.m_DisplayInvertXAxis )
            m_xIncreasesLeft->SetValue( true );
        else
            m_xIncreasesRight->SetValue( true );

        if( cfg && cfg->m_Display.m_DisplayInvertYAxis )
            m_yIncreasesUp->SetValue( true );
        else
            m_yIncreasesDown->SetValue( true );
    }
}


bool PANEL_PCBNEW_DISPLAY_ORIGIN::TransferDataToWindow()
{
    loadSettings( m_cfg );

    return true;
}


bool PANEL_PCBNEW_DISPLAY_ORIGIN::TransferDataFromWindow()
{
    if( m_frameType == FRAME_FOOTPRINT_EDITOR )
    {
        if( FOOTPRINT_EDITOR_SETTINGS* cfg = GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" ) )
        {
        cfg->m_DisplayInvertXAxis = m_xIncreasesLeft->GetValue();
        cfg->m_DisplayInvertYAxis = m_yIncreasesUp->GetValue();
        }
    }
    else
    {
        if( PCBNEW_SETTINGS* cfg = GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" ) )
        {
            if( m_pageOrigin->GetValue() )
                cfg->m_Display.m_DisplayOrigin = PCB_DISPLAY_ORIGIN::PCB_ORIGIN_PAGE;
            else if( m_gridOrigin->GetValue() )
                cfg->m_Display.m_DisplayOrigin = PCB_DISPLAY_ORIGIN::PCB_ORIGIN_GRID;
            else
                cfg->m_Display.m_DisplayOrigin = PCB_DISPLAY_ORIGIN::PCB_ORIGIN_AUX;

            cfg->m_Display.m_DisplayInvertXAxis = m_xIncreasesLeft->GetValue();
            cfg->m_Display.m_DisplayInvertYAxis = m_yIncreasesUp->GetValue();
        }
    }

    return true;
}


void PANEL_PCBNEW_DISPLAY_ORIGIN::ResetPanel()
{
    if( m_frameType == FRAME_FOOTPRINT_EDITOR )
    {
        FOOTPRINT_EDITOR_SETTINGS cfg;
        cfg.Load();                     // Loading without a file will init to defaults

        loadSettings( &cfg );
    }
    else
    {
        PCBNEW_SETTINGS cfg;
        cfg.Load();                     // Loading without a file will init to defaults

        loadSettings( &cfg );
    }
}


