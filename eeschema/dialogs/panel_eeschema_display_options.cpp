/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <base_screen.h>
#include <sch_view.h>
#include <sch_edit_frame.h>
#include <sch_painter.h>
#include <class_libentry.h>
#include <panel_eeschema_display_options.h>
#include <widgets/gal_options_panel.h>
#include <sch_junction.h>

PANEL_EESCHEMA_DISPLAY_OPTIONS::PANEL_EESCHEMA_DISPLAY_OPTIONS( SCH_EDIT_FRAME* aFrame,
                                                                wxWindow* aWindow ) :
        PANEL_EESCHEMA_DISPLAY_OPTIONS_BASE( aWindow ),
        m_frame( aFrame )
{
    KIGFX::GAL_DISPLAY_OPTIONS& galOptions = m_frame->GetGalDisplayOptions();
    m_galOptsPanel = new GAL_OPTIONS_PANEL( this, galOptions );

    m_galOptionsSizer->Add( m_galOptsPanel, 1, wxEXPAND, 0 );
}


bool PANEL_EESCHEMA_DISPLAY_OPTIONS::TransferDataToWindow()
{
    // Reference style one of: "A" ".A" "-A" "_A" ".1" "-1" "_1"
    int refStyleSelection;

    switch( LIB_PART::GetSubpartIdSeparator() )
    {
    default:
    case 0:   refStyleSelection = 0; break;
    case '.': refStyleSelection = LIB_PART::GetSubpartFirstId() == '1' ? 4 : 1; break;
    case '-': refStyleSelection = LIB_PART::GetSubpartFirstId() == '1' ? 5 : 2; break;
    case '_': refStyleSelection = LIB_PART::GetSubpartFirstId() == '1' ? 6 : 3; break;
    }

    m_choiceSeparatorRefId->SetSelection( refStyleSelection );

    m_busWidthCtrl->SetValue( StringFromValue( INCHES, GetDefaultBusThickness(), false, true ) );
    m_lineWidthCtrl->SetValue( StringFromValue( INCHES, GetDefaultLineThickness(), false, true ) );
    m_jctSizeCtrl->SetValue( StringFromValue( INCHES, SCH_JUNCTION::GetSymbolSize(), false, true ) );
    m_checkShowHiddenPins->SetValue( m_frame->GetShowAllPins() );
    m_checkPageLimits->SetValue( m_frame->ShowPageLimits() );

    m_galOptsPanel->TransferDataToWindow();

    return true;
}


bool PANEL_EESCHEMA_DISPLAY_OPTIONS::TransferDataFromWindow()
{
    // Reference style one of: "A" ".A" "-A" "_A" ".1" "-1" "_1"
    int firstRefId, refSeparator;

    switch( m_choiceSeparatorRefId->GetSelection() )
    {
    default:
    case 0: firstRefId = 'A'; refSeparator = 0; break;
    case 1: firstRefId = 'A'; refSeparator = '.'; break;
    case 2: firstRefId = 'A'; refSeparator = '-'; break;
    case 3: firstRefId = 'A'; refSeparator = '_'; break;
    case 4: firstRefId = '1'; refSeparator = '.'; break;
    case 5: firstRefId = '1'; refSeparator = '-'; break;
    case 6: firstRefId = '1'; refSeparator = '_'; break;
    }

    if( refSeparator != LIB_PART::GetSubpartIdSeparator() ||
        firstRefId != LIB_PART::GetSubpartFirstId() )
    {
        LIB_PART::SetSubpartIdNotation( refSeparator, firstRefId );
        m_frame->SaveProjectSettings( false );
    }

    SetDefaultBusThickness( ValueFromString( INCHES, m_busWidthCtrl->GetValue(), true ) );
    SetDefaultLineThickness( ValueFromString( INCHES, m_lineWidthCtrl->GetValue(), true ) );
    SCH_JUNCTION::SetSymbolSize( ValueFromString( INCHES, m_jctSizeCtrl->GetValue(), true ) );
    m_frame->SetShowAllPins( m_checkShowHiddenPins->GetValue() );
    m_frame->SetShowPageLimits( m_checkPageLimits->GetValue() );

    // Update canvas
    m_frame->GetRenderSettings()->m_ShowHiddenPins = m_checkShowHiddenPins->GetValue();
    m_frame->GetRenderSettings()->SetShowPageLimits( m_checkPageLimits->GetValue() );
    m_frame->GetCanvas()->GetView()->MarkDirty();
    m_frame->GetCanvas()->GetView()->UpdateAllItems( KIGFX::REPAINT );
    m_frame->GetCanvas()->Refresh();

    m_galOptsPanel->TransferDataFromWindow();

    return true;
}


