/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <sch_painter.h>
#include <class_libentry.h>
#include <panel_eeschema_display_options.h>
#include <widgets/gal_options_panel.h>
#include <sch_junction.h>
#include <gr_text.h>

PANEL_EESCHEMA_DISPLAY_OPTIONS::PANEL_EESCHEMA_DISPLAY_OPTIONS( SCH_EDIT_FRAME* aFrame,
                                                                wxWindow* aWindow ) :
        PANEL_EESCHEMA_DISPLAY_OPTIONS_BASE( aWindow ),
        m_frame( aFrame )
{
    KIGFX::GAL_DISPLAY_OPTIONS& galOptions = m_frame->GetGalDisplayOptions();
    m_galOptsPanel = new GAL_OPTIONS_PANEL( this, galOptions );

    m_galOptionsSizer->Add( m_galOptsPanel, 1, wxEXPAND, 0 );

    wxFont infoFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    infoFont.SetSymbolicSize( wxFONTSIZE_SMALL );
    m_highlightColorNote->SetFont( infoFont );
}


bool PANEL_EESCHEMA_DISPLAY_OPTIONS::TransferDataToWindow()
{
    m_checkShowHiddenPins->SetValue( m_frame->GetShowAllPins() );

    int superSubFlags = ENABLE_SUBSCRIPT_MARKUP | ENABLE_SUPERSCRIPT_MARKUP;

    m_checkSuperSub->SetValue( GetTextMarkupFlags() & superSubFlags );

    m_checkPageLimits->SetValue( m_frame->ShowPageLimits() );

    m_checkSelTextBox->SetValue( GetSelectionTextAsBox() );
    m_checkSelDrawChildItems->SetValue( GetSelectionDrawChildItems() );
    m_checkSelFillShapes->SetValue( GetSelectionFillShapes() );
    m_selWidthCtrl->SetValue( Iu2Mils( GetSelectionThickness() ) );

    m_galOptsPanel->TransferDataToWindow();

    return true;
}


bool PANEL_EESCHEMA_DISPLAY_OPTIONS::TransferDataFromWindow()
{
    m_frame->SetShowAllPins( m_checkShowHiddenPins->GetValue() );
    m_frame->SetShowPageLimits( m_checkPageLimits->GetValue() );
    SetSelectionTextAsBox( m_checkSelTextBox->GetValue() );
    SetSelectionDrawChildItems( m_checkSelDrawChildItems->GetValue() );
    SetSelectionFillShapes( m_checkSelFillShapes->GetValue() );
    SetSelectionThickness( Mils2iu( m_selWidthCtrl->GetValue() ) );

    // Update canvas
    m_frame->GetRenderSettings()->m_ShowHiddenPins = m_checkShowHiddenPins->GetValue();

    int superSubFlags = ENABLE_SUBSCRIPT_MARKUP | ENABLE_SUPERSCRIPT_MARKUP;

    if( m_checkSuperSub->GetValue() )
        SetTextMarkupFlags( GetTextMarkupFlags() | superSubFlags );
    else
        SetTextMarkupFlags( GetTextMarkupFlags() & ~superSubFlags );

    m_frame->GetRenderSettings()->SetShowPageLimits( m_checkPageLimits->GetValue() );
    m_frame->GetCanvas()->GetView()->MarkDirty();
    m_frame->GetCanvas()->GetView()->UpdateAllItems( KIGFX::REPAINT );
    m_frame->GetCanvas()->Refresh();

    m_galOptsPanel->TransferDataFromWindow();

    return true;
}


