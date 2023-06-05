/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mike Williams <mike@mikebwilliams.com>
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <settings/color_settings.h>
#include <eeschema_settings.h>
#include <schematic.h>
#include <schematic_settings.h>
#include <sch_edit_frame.h>
#include "panel_eeschema_annotation_options.h"


PANEL_EESCHEMA_ANNOTATION_OPTIONS::PANEL_EESCHEMA_ANNOTATION_OPTIONS(
        wxWindow* aWindow, EDA_BASE_FRAME* schSettingsProvider ) :
        PANEL_EESCHEMA_ANNOTATION_OPTIONS_BASE( aWindow ),
        m_schSettingsProvider( schSettingsProvider )
{
    annotate_down_right_bitmap->SetBitmap( KiBitmap( BITMAPS::annotate_down_right ) );
    annotate_right_down_bitmap->SetBitmap( KiBitmap( BITMAPS::annotate_right_down ) );
}


void PANEL_EESCHEMA_ANNOTATION_OPTIONS::loadEEschemaSettings( EESCHEMA_SETTINGS* aCfg )
{
    m_checkAutoAnnotate->SetValue( aCfg->m_AnnotatePanel.automatic );

    switch( aCfg->m_AnnotatePanel.sort_order )
    {
    default:
    case 0: m_rbSortBy_X_Position->SetValue( true ); break;
    case 1: m_rbSortBy_Y_Position->SetValue( true ); break;
    }

    switch( aCfg->m_AnnotatePanel.method )
    {
    default:
    case 0: m_rbFirstFree->SetValue( true );  break;
    case 1: m_rbSheetX100->SetValue( true );  break;
    case 2: m_rbSheetX1000->SetValue( true ); break;
    }


    int annotateStartNum = 0; // Default "start after" value for annotation

    // See if we can get a "start after" value from the project settings
    SCH_EDIT_FRAME* schFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_schSettingsProvider );

    if( schFrame )
    {
        SCHEMATIC_SETTINGS& projSettings = schFrame->Schematic().Settings();
        annotateStartNum = projSettings.m_AnnotateStartNum;
    }

    m_textNumberAfter->SetValue( wxString::Format( wxT( "%d" ), annotateStartNum ) );
}


bool PANEL_EESCHEMA_ANNOTATION_OPTIONS::TransferDataToWindow()
{
    SETTINGS_MANAGER&  mgr = Pgm().GetSettingsManager();
    EESCHEMA_SETTINGS* cfg = mgr.GetAppSettings<EESCHEMA_SETTINGS>();

    loadEEschemaSettings( cfg );

    return true;
}


bool PANEL_EESCHEMA_ANNOTATION_OPTIONS::TransferDataFromWindow()
{
    SETTINGS_MANAGER&  mgr = Pgm().GetSettingsManager();
    EESCHEMA_SETTINGS* cfg = mgr.GetAppSettings<EESCHEMA_SETTINGS>();

    cfg->m_AnnotatePanel.automatic = m_checkAutoAnnotate->GetValue();

    cfg->m_AnnotatePanel.sort_order = m_rbSortBy_Y_Position->GetValue()
                                              ? ANNOTATE_ORDER_T::SORT_BY_Y_POSITION
                                              : ANNOTATE_ORDER_T::SORT_BY_X_POSITION;

    if( m_rbSheetX100->GetValue() )
        cfg->m_AnnotatePanel.method = ANNOTATE_ALGO_T::SHEET_NUMBER_X_100;
    else if( m_rbSheetX1000->GetValue() )
        cfg->m_AnnotatePanel.method = ANNOTATE_ALGO_T::SHEET_NUMBER_X_1000;
    else
        cfg->m_AnnotatePanel.method = ANNOTATE_ALGO_T::INCREMENTAL_BY_REF;


    SCH_EDIT_FRAME* schFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_schSettingsProvider );

    if( schFrame )
    {
        SCHEMATIC_SETTINGS& projSettings = schFrame->Schematic().Settings();
        projSettings.m_AnnotateStartNum = EDA_UNIT_UTILS::UI::ValueFromString( m_textNumberAfter->GetValue() );
    }

    return true;
}


void PANEL_EESCHEMA_ANNOTATION_OPTIONS::ResetPanel()
{
    EESCHEMA_SETTINGS cfg;
    cfg.Load(); // Loading without a file will init to defaults

    loadEEschemaSettings( &cfg );
}
