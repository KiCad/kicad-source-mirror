/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <widgets/bitmap_button.h>
#include <widgets/font_choice.h>
#include <confirm.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <board.h>
#include <footprint.h>
#include <pcb_textbox.h>
#include <pcb_tablecell.h>
#include <pcb_table.h>
#include <project.h>
#include <pcb_edit_frame.h>
#include <pcb_layer_box_selector.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <scintilla_tricks.h>
#include "dialog_tablecell_properties.h"

DIALOG_TABLECELL_PROPERTIES::DIALOG_TABLECELL_PROPERTIES( PCB_BASE_EDIT_FRAME* aFrame,
                                                          std::vector<PCB_TABLECELL*> aCells ) :
        DIALOG_TABLECELL_PROPERTIES_BASE( aFrame ),
        m_frame( aFrame ),
        m_table( nullptr ),
        m_cells( std::move( aCells ) ),
        m_textHeight( aFrame, m_SizeYLabel, m_SizeYCtrl, m_SizeYUnits ),
        m_textWidth( aFrame, m_SizeXLabel, m_SizeXCtrl, m_SizeXUnits ),
        m_textThickness( aFrame, m_ThicknessLabel, m_ThicknessCtrl, m_ThicknessUnits ),
        m_marginLeft( aFrame, nullptr, m_marginLeftCtrl, nullptr ),
        m_marginTop( aFrame, nullptr, m_marginTopCtrl, m_marginTopUnits ),
        m_marginRight( aFrame, nullptr, m_marginRightCtrl, nullptr ),
        m_marginBottom( aFrame, nullptr, m_marginBottomCtrl, nullptr ),
        m_returnValue( TABLECELL_PROPS_CANCEL )
{
    wxASSERT( m_cells.size() > 0 && m_cells[0] );

    m_table = static_cast<PCB_TABLE*>( m_cells[0]->GetParent() );

    m_hAlignLeft->SetIsRadioButton();
    m_hAlignLeft->SetBitmap( KiBitmapBundle( BITMAPS::text_align_left ) );
    m_hAlignCenter->SetIsRadioButton();
    m_hAlignCenter->SetBitmap( KiBitmapBundle( BITMAPS::text_align_center ) );
    m_hAlignRight->SetIsRadioButton();
    m_hAlignRight->SetBitmap( KiBitmapBundle( BITMAPS::text_align_right ) );

    m_vAlignTop->SetIsRadioButton();
    m_vAlignTop->SetBitmap( KiBitmapBundle( BITMAPS::text_valign_top ) );
    m_vAlignCenter->SetIsRadioButton();
    m_vAlignCenter->SetBitmap( KiBitmapBundle( BITMAPS::text_valign_center ) );
    m_vAlignBottom->SetIsRadioButton();
    m_vAlignBottom->SetBitmap( KiBitmapBundle( BITMAPS::text_valign_bottom ) );

    SetupStandardButtons();
    Layout();

    m_hAlignLeft->Bind( wxEVT_BUTTON, &DIALOG_TABLECELL_PROPERTIES::onHAlignButton, this );
    m_hAlignCenter->Bind( wxEVT_BUTTON, &DIALOG_TABLECELL_PROPERTIES::onHAlignButton, this );
    m_hAlignRight->Bind( wxEVT_BUTTON, &DIALOG_TABLECELL_PROPERTIES::onHAlignButton, this );
    m_vAlignTop->Bind( wxEVT_BUTTON, &DIALOG_TABLECELL_PROPERTIES::onVAlignButton, this );
    m_vAlignCenter->Bind( wxEVT_BUTTON, &DIALOG_TABLECELL_PROPERTIES::onVAlignButton, this );
    m_vAlignBottom->Bind( wxEVT_BUTTON, &DIALOG_TABLECELL_PROPERTIES::onVAlignButton, this );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


bool DIALOG_TABLECELL_PROPERTIES::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    bool              firstCell = true;
    GR_TEXT_H_ALIGN_T hAlign = GR_TEXT_H_ALIGN_INDETERMINATE;
    GR_TEXT_V_ALIGN_T vAlign = GR_TEXT_V_ALIGN_INDETERMINATE;

    for( PCB_TABLECELL* cell : m_cells )
    {
        if( firstCell )
        {
            m_fontCtrl->SetFontSelection( cell->GetFont() );
            m_textWidth.SetValue( cell->GetTextWidth() );
            m_textHeight.SetValue( cell->GetTextHeight() );
            m_textThickness.SetValue( cell->GetTextThickness() );

            m_bold->Set3StateValue( cell->IsBold() ? wxCHK_CHECKED : wxCHK_UNCHECKED );
            m_italic->Set3StateValue( cell->IsItalic() ? wxCHK_CHECKED : wxCHK_UNCHECKED );

            hAlign = cell->GetHorizJustify();
            vAlign = cell->GetVertJustify();

            m_marginLeft.SetValue( cell->GetMarginLeft() );
            m_marginTop.SetValue( cell->GetMarginTop() );
            m_marginRight.SetValue( cell->GetMarginRight() );
            m_marginBottom.SetValue( cell->GetMarginBottom() );

            firstCell = false;
        }
        else
        {
            if( cell->GetFont() != m_fontCtrl->GetFontSelection( cell->IsBold(), cell->IsItalic() ) )
                m_fontCtrl->SetSelection( -1 );

            if( cell->GetTextWidth() != m_textWidth.GetValue() )
                m_textWidth.SetValue( INDETERMINATE_STATE );

            if( cell->GetTextHeight() != m_textHeight.GetValue() )
                m_textHeight.SetValue( INDETERMINATE_STATE );

            if( cell->GetTextThickness() != m_textThickness.GetValue() )
                m_textThickness.SetValue( INDETERMINATE_STATE );

            wxCheckBoxState bold = cell->IsBold() ? wxCHK_CHECKED : wxCHK_UNCHECKED;

            if( bold != m_bold->Get3StateValue() )
                m_bold->Set3StateValue( wxCHK_UNDETERMINED );

            wxCheckBoxState italic = cell->IsItalic() ? wxCHK_CHECKED : wxCHK_UNCHECKED;

            if( italic != m_italic->Get3StateValue() )
                m_italic->Set3StateValue( wxCHK_UNDETERMINED );

            if( cell->GetHorizJustify() != hAlign )
                hAlign = GR_TEXT_H_ALIGN_INDETERMINATE;

            if( cell->GetVertJustify() != vAlign )
                vAlign = GR_TEXT_V_ALIGN_INDETERMINATE;

            if( cell->GetMarginLeft() != m_marginLeft.GetIntValue() )
                m_marginLeft.SetValue( INDETERMINATE_STATE );

            if( cell->GetMarginTop() != m_marginTop.GetIntValue() )
                m_marginTop.SetValue( INDETERMINATE_STATE );

            if( cell->GetMarginRight() != m_marginRight.GetIntValue() )
                m_marginRight.SetValue( INDETERMINATE_STATE );

            if( cell->GetMarginBottom() != m_marginBottom.GetIntValue() )
                m_marginBottom.SetValue( INDETERMINATE_STATE );
        }

        switch( hAlign )
        {
        case GR_TEXT_H_ALIGN_LEFT:          m_hAlignLeft->Check();   break;
        case GR_TEXT_H_ALIGN_CENTER:        m_hAlignCenter->Check(); break;
        case GR_TEXT_H_ALIGN_RIGHT:         m_hAlignRight->Check();  break;
        case GR_TEXT_H_ALIGN_INDETERMINATE:                          break;
        }

        switch( vAlign )
        {
        case GR_TEXT_V_ALIGN_TOP:           m_vAlignTop->Check();    break;
        case GR_TEXT_V_ALIGN_CENTER:        m_vAlignCenter->Check(); break;
        case GR_TEXT_V_ALIGN_BOTTOM:        m_vAlignBottom->Check(); break;
        case GR_TEXT_V_ALIGN_INDETERMINATE:                          break;
        }
    }

    return true;
}


void DIALOG_TABLECELL_PROPERTIES::onHAlignButton( wxCommandEvent& aEvent )
{
    for( BITMAP_BUTTON* btn : { m_hAlignLeft, m_hAlignCenter, m_hAlignRight } )
    {
        if( btn->IsChecked() && btn != aEvent.GetEventObject() )
            btn->Check( false );
    }
}


void DIALOG_TABLECELL_PROPERTIES::onVAlignButton( wxCommandEvent& aEvent )
{
    for( BITMAP_BUTTON* btn : { m_vAlignTop, m_vAlignCenter, m_vAlignBottom } )
    {
        if( btn->IsChecked() && btn != aEvent.GetEventObject() )
            btn->Check( false );
    }
}


bool DIALOG_TABLECELL_PROPERTIES::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    BOARD_COMMIT commit( m_frame );
    commit.Modify( m_table );

    // If no other command in progress, prepare undo command
    // (for a command in progress, will be made later, at the completion of command)
    bool pushCommit = ( m_table->GetEditFlags() == 0 );

    // Set IN_EDIT flag to force undo/redo/abort proper operation and avoid new calls to
    // SaveCopyInUndoList for the same text if is moved, and then rotated, edited, etc....
    if( !pushCommit )
        m_table->SetFlags( IN_EDIT );

    for( PCB_TABLECELL* cell : m_cells )
    {
        if( m_bold->Get3StateValue() == wxCHK_CHECKED )
            cell->SetBold( true );
        else if( m_bold->Get3StateValue() == wxCHK_UNCHECKED )
            cell->SetBold( false );

        if( m_italic->Get3StateValue() == wxCHK_CHECKED )
            cell->SetItalic( true );
        else if( m_italic->Get3StateValue() == wxCHK_UNCHECKED )
            cell->SetItalic( false );

        if( m_fontCtrl->HaveFontSelection() )
            cell->SetFont( m_fontCtrl->GetFontSelection( cell->IsBold(), cell->IsItalic() ) );

        if( !m_textWidth.IsIndeterminate() )
            cell->SetTextWidth( m_textWidth.GetIntValue() );

        if( !m_textHeight.IsIndeterminate() )
            cell->SetTextHeight( m_textHeight.GetIntValue() );

        if( !m_textThickness.IsIndeterminate() )
            cell->SetTextThickness( m_textThickness.GetIntValue() );

        if( m_hAlignLeft->IsChecked() )
            cell->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        else if( m_hAlignRight->IsChecked() )
            cell->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        else if( m_hAlignCenter->IsChecked() )
            cell->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );

        if( m_vAlignTop->IsChecked() )
            cell->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
        else if( m_vAlignBottom->IsChecked() )
            cell->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
        else if( m_vAlignCenter->IsChecked() )
            cell->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );

        if( !m_marginLeft.IsIndeterminate() )
            cell->SetMarginLeft( m_marginLeft.GetIntValue() );

        if( !m_marginTop.IsIndeterminate() )
            cell->SetMarginTop( m_marginTop.GetIntValue() );

        if( !m_marginRight.IsIndeterminate() )
            cell->SetMarginRight( m_marginRight.GetIntValue() );

        if( !m_marginBottom.IsIndeterminate() )
            cell->SetMarginBottom( m_marginBottom.GetIntValue() );
    }

    if( !commit.Empty() )
        commit.Push( _( "Edit Table Cell Properties" ), SKIP_CONNECTIVITY );

    m_returnValue = TABLECELL_PROPS_OK;
    return true;
}


void DIALOG_TABLECELL_PROPERTIES::onEditTable( wxCommandEvent& aEvent )
{
    if( TransferDataFromWindow() )
    {
        m_returnValue = TABLECELL_PROPS_EDIT_TABLE;
        Close();
    }
}