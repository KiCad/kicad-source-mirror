/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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


PANEL_PCBNEW_DISPLAY_ORIGIN::PANEL_PCBNEW_DISPLAY_ORIGIN( wxWindow* aParent,
                                                          APP_SETTINGS_BASE* aCfg,
                                                          FRAME_T aFrameType ) :
        PANEL_PCBNEW_DISPLAY_ORIGIN_BASE( aParent ),
        m_cfg( aCfg ),
        m_frameType( aFrameType )
{
    m_DisplayOrigin->Show( m_frameType == FRAME_PCB_EDITOR );
}


void PANEL_PCBNEW_DISPLAY_ORIGIN::loadSettings( APP_SETTINGS_BASE* aCfg )
{
    if( m_frameType == FRAME_FOOTPRINT_EDITOR )
    {
        FOOTPRINT_EDITOR_SETTINGS* cfg = static_cast<FOOTPRINT_EDITOR_SETTINGS*>( aCfg );

        m_XAxisDirection->SetSelection( cfg->m_DisplayInvertXAxis ? 1 : 0 );
        m_YAxisDirection->SetSelection( cfg->m_DisplayInvertYAxis ? 0 : 1 );
    }
    else
    {
        PCBNEW_SETTINGS* cfg = static_cast<PCBNEW_SETTINGS*>( aCfg );
        int              origin = 0;

        switch( cfg->m_Display.m_DisplayOrigin )
        {
        case PCB_DISPLAY_ORIGIN::PCB_ORIGIN_PAGE: origin = 0; break;
        case PCB_DISPLAY_ORIGIN::PCB_ORIGIN_AUX:  origin = 1; break;
        case PCB_DISPLAY_ORIGIN::PCB_ORIGIN_GRID: origin = 2; break;
        }

        m_DisplayOrigin->SetSelection( origin );
        m_XAxisDirection->SetSelection( cfg->m_Display.m_DisplayInvertXAxis ? 1 : 0 );
        m_YAxisDirection->SetSelection( cfg->m_Display.m_DisplayInvertYAxis ? 0 : 1 );
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
        FOOTPRINT_EDITOR_SETTINGS* cfg = static_cast<FOOTPRINT_EDITOR_SETTINGS*>( m_cfg );

        cfg->m_DisplayInvertXAxis = m_XAxisDirection->GetSelection() != 0;
        cfg->m_DisplayInvertYAxis = m_YAxisDirection->GetSelection() == 0;
    }
    else
    {
        PCBNEW_SETTINGS* cfg = static_cast<PCBNEW_SETTINGS*>( m_cfg );

        switch( m_DisplayOrigin->GetSelection() )
        {
        case 0: cfg->m_Display.m_DisplayOrigin = PCB_DISPLAY_ORIGIN::PCB_ORIGIN_PAGE; break;
        case 1: cfg->m_Display.m_DisplayOrigin = PCB_DISPLAY_ORIGIN::PCB_ORIGIN_AUX;  break;
        case 2: cfg->m_Display.m_DisplayOrigin = PCB_DISPLAY_ORIGIN::PCB_ORIGIN_GRID; break;
        }

        cfg->m_Display.m_DisplayInvertXAxis = m_XAxisDirection->GetSelection() != 0;
        cfg->m_Display.m_DisplayInvertYAxis = m_YAxisDirection->GetSelection() == 0;
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


