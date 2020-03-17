/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <sch_edit_frame.h>
#include <class_libentry.h>
#include <panel_setup_formatting.h>
#include <sch_junction.h>
#include <gr_text.h>

PANEL_SETUP_FORMATTING::PANEL_SETUP_FORMATTING( wxWindow* aWindow, SCH_EDIT_FRAME* aFrame  ) :
        PANEL_SETUP_FORMATTING_BASE( aWindow ),
        m_frame( aFrame ),
        m_textSize( aFrame, m_textSizeLabel, m_textSizeCtrl, m_textSizeUnits, true ),
        m_busWidth( aFrame, m_busWidthLabel, m_busWidthCtrl, m_busWidthUnits, true ),
        m_wireWidth( aFrame, m_wireWidthLabel, m_wireWidthCtrl, m_wireWidthUnits, true ),
        m_junctionSize( aFrame, m_jctSizeLabel, m_jctSizeCtrl, m_jctSizeUnits, true )
{
    wxFont infoFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    infoFont.SetSymbolicSize( wxFONTSIZE_SMALL );
    m_superSubHint->SetFont( infoFont );
}


bool PANEL_SETUP_FORMATTING::TransferDataToWindow()
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

    m_textSize.SetUnits( EDA_UNITS::INCHES, true );
    m_busWidth.SetUnits( EDA_UNITS::INCHES, true );
    m_wireWidth.SetUnits( EDA_UNITS::INCHES, true );
    m_junctionSize.SetUnits( EDA_UNITS::INCHES, true );

    m_textSize.SetValue( GetDefaultTextSize() );
    m_busWidth.SetValue( GetDefaultBusThickness() );
    m_wireWidth.SetValue( GetDefaultWireThickness() );
    m_junctionSize.SetValue( SCH_JUNCTION::g_SymbolSize );

    m_textOffsetRatioCtrl->SetValue( wxString::Format( "%f", GetTextOffsetRatio() * 100.0 ) );

    int superSubFlags = ENABLE_SUBSCRIPT_MARKUP | ENABLE_SUPERSCRIPT_MARKUP;
    m_checkSuperSub->SetValue( GetTextMarkupFlags() & superSubFlags );

    return true;
}


bool PANEL_SETUP_FORMATTING::TransferDataFromWindow()
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
        m_frame->SaveProjectSettings();
    }

    SetDefaultTextSize( m_textSize.GetValue() );
    SetDefaultBusThickness( m_busWidth.GetValue() );
    SetDefaultWireThickness( m_wireWidth.GetValue() );
    SCH_JUNCTION::g_SymbolSize = m_junctionSize.GetValue();

    double dtmp = 0.0;
    wxString msg = m_textOffsetRatioCtrl->GetValue();
    msg.ToDouble( &dtmp );
    SetTextOffsetRatio( dtmp / 100.0 );

    int superSubFlags = ENABLE_SUBSCRIPT_MARKUP | ENABLE_SUPERSCRIPT_MARKUP;

    if( m_checkSuperSub->GetValue() )
        SetTextMarkupFlags( GetTextMarkupFlags() | superSubFlags );
    else
        SetTextMarkupFlags( GetTextMarkupFlags() & ~superSubFlags );

    m_frame->GetCanvas()->GetView()->MarkDirty();
    m_frame->GetCanvas()->GetView()->UpdateAllItems( KIGFX::REPAINT );
    m_frame->GetCanvas()->Refresh();

    return true;
}


