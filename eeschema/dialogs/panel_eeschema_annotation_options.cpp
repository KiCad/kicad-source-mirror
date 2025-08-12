/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mike Williams <mike@mikebwilliams.com>
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
#include <refdes_tracker.h>
#include <settings/settings_manager.h>
#include <settings/color_settings.h>
#include <schematic.h>
#include <schematic_settings.h>
#include <sch_edit_frame.h>
#include "panel_eeschema_annotation_options.h"


PANEL_EESCHEMA_ANNOTATION_OPTIONS::PANEL_EESCHEMA_ANNOTATION_OPTIONS(
        wxWindow* aWindow, EDA_BASE_FRAME* schSettingsProvider ) :
        PANEL_EESCHEMA_ANNOTATION_OPTIONS_BASE( aWindow ),
        m_schSettingsProvider( schSettingsProvider )
{
    annotate_down_right_bitmap->SetBitmap( KiBitmapBundle( BITMAPS::annotate_down_right ) );
    annotate_right_down_bitmap->SetBitmap( KiBitmapBundle( BITMAPS::annotate_right_down ) );
}


void PANEL_EESCHEMA_ANNOTATION_OPTIONS::loadEEschemaSettings( SCHEMATIC_SETTINGS* aCfg )
{
    int annotateStartNum = aCfg->m_AnnotateStartNum;

    switch( aCfg->m_AnnotateSortOrder )
    {
    default:
    case SORT_BY_X_POSITION: m_rbSortBy_X_Position->SetValue( true ); break;
    case SORT_BY_Y_POSITION: m_rbSortBy_Y_Position->SetValue( true ); break;
    }

    switch( aCfg->m_AnnotateMethod )
    {
    default:
    case INCREMENTAL_BY_REF:  m_rbFirstFree->SetValue( true );  break;
    case SHEET_NUMBER_X_100:  m_rbSheetX100->SetValue( true );  break;
    case SHEET_NUMBER_X_1000: m_rbSheetX1000->SetValue( true ); break;
    }

    m_textNumberAfter->SetValue( wxString::Format( wxT( "%d" ), annotateStartNum ) );

    if( aCfg->m_SubpartFirstId == 'A' )
    {
        switch( aCfg->m_SubpartIdSeparator )
        {
        default:
        case 0: m_choiceSeparatorRefId->SetSelection( 0 ); break;
        case '.': m_choiceSeparatorRefId->SetSelection( 1 ); break;
        case '-': m_choiceSeparatorRefId->SetSelection( 2 ); break;
        case '_': m_choiceSeparatorRefId->SetSelection( 3 ); break;
        }
    }
    else
    {
        switch( aCfg->m_SubpartIdSeparator )
        {
        default:
        case '.': m_choiceSeparatorRefId->SetSelection( 4 ); break;
        case '-': m_choiceSeparatorRefId->SetSelection( 5 ); break;
        case '_': m_choiceSeparatorRefId->SetSelection( 6 ); break;
        }
    }

    m_checkReuseRefdes->SetValue( aCfg->m_refDesTracker->GetReuseRefDes() );
}


bool PANEL_EESCHEMA_ANNOTATION_OPTIONS::TransferDataToWindow()
{
    if( SCH_EDIT_FRAME* schFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_schSettingsProvider ) )
        loadEEschemaSettings( &schFrame->Schematic().Settings() );

    return true;
}


bool PANEL_EESCHEMA_ANNOTATION_OPTIONS::TransferDataFromWindow()
{
    if( SCH_EDIT_FRAME* schFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_schSettingsProvider ) )
    {
        SCHEMATIC_SETTINGS& projSettings = schFrame->Schematic().Settings();

        projSettings.m_AnnotateSortOrder = m_rbSortBy_Y_Position->GetValue() ?
                ANNOTATE_ORDER_T::SORT_BY_Y_POSITION : ANNOTATE_ORDER_T::SORT_BY_X_POSITION;

        if( m_rbSheetX100->GetValue() )
            projSettings.m_AnnotateMethod = ANNOTATE_ALGO_T::SHEET_NUMBER_X_100;
        else if( m_rbSheetX1000->GetValue() )
            projSettings.m_AnnotateMethod = ANNOTATE_ALGO_T::SHEET_NUMBER_X_1000;
        else
            projSettings.m_AnnotateMethod = ANNOTATE_ALGO_T::INCREMENTAL_BY_REF;

        projSettings.m_AnnotateStartNum = EDA_UNIT_UTILS::UI::ValueFromString( m_textNumberAfter->GetValue() );
        projSettings.m_refDesTracker->SetReuseRefDes( m_checkReuseRefdes->GetValue() );

        switch( m_choiceSeparatorRefId->GetSelection() )
        {
        default:
        case 0: projSettings.m_SubpartFirstId = 'A'; projSettings.m_SubpartIdSeparator = 0;   break;
        case 1: projSettings.m_SubpartFirstId = 'A'; projSettings.m_SubpartIdSeparator = '.'; break;
        case 2: projSettings.m_SubpartFirstId = 'A'; projSettings.m_SubpartIdSeparator = '-'; break;
        case 3: projSettings.m_SubpartFirstId = 'A'; projSettings.m_SubpartIdSeparator = '_'; break;
        case 4: projSettings.m_SubpartFirstId = '1'; projSettings.m_SubpartIdSeparator = '.'; break;
        case 5: projSettings.m_SubpartFirstId = '1'; projSettings.m_SubpartIdSeparator = '-'; break;
        case 6: projSettings.m_SubpartFirstId = '1'; projSettings.m_SubpartIdSeparator = '_'; break;
        }

    }

    return true;
}


void PANEL_EESCHEMA_ANNOTATION_OPTIONS::ResetPanel()
{
    SCHEMATIC_SETTINGS cfg( nullptr, "" );
    loadEEschemaSettings( &cfg );
}


void PANEL_EESCHEMA_ANNOTATION_OPTIONS::ImportSettingsFrom( SCHEMATIC_SETTINGS& aSettings )
{
    loadEEschemaSettings( &aSettings );
}