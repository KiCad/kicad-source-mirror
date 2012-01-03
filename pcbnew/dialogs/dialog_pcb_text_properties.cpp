/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2010 Jean-Pierre Charras <jean-pierre.charras@gpisa-lab.inpg.fr>
 * Copyright (C) 2010 KiCad Developers, see change_log.txt for contributors.
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

/***************************************************************************/
/* Dialog editor for text on copper and technical layers (TEXTE_PCB class) */
/***************************************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "class_drawpanel.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "drawtxt.h"
#include "confirm.h"
#include "dialog_helpers.h"

#include "class_board.h"
#include "class_pcb_text.h"

#include "dialog_pcb_text_properties.h"


/**
 *  DIALOG_PCB_TEXT_PROPERTIES, derived from DIALOG_PCB_TEXT_PROPERTIES_BASE
 *  @see dialog_dialog_pcb_text_properties_base.h and
 *  dialog_dialog_pcb_text_properties_base.cpp, automatically created by
 *  wxFormBuilder.
 */

DIALOG_PCB_TEXT_PROPERTIES::DIALOG_PCB_TEXT_PROPERTIES( PCB_EDIT_FRAME* parent,
                                                        TEXTE_PCB* passedTextPCB, wxDC* DC )
                            : DIALOG_PCB_TEXT_PROPERTIES_BASE( parent )
{
    m_Parent = parent;
    m_DC = DC;
    m_SelectedPCBText = passedTextPCB;

    MyInit();
    m_StandardSizerOK->SetDefault();
    GetSizer()->SetSizeHints( this );
    Centre();
}


/**
 * Routine for main window class to launch text properties dialog.
 */
void PCB_EDIT_FRAME::InstallTextPCBOptionsFrame( TEXTE_PCB* TextPCB, wxDC* DC )
{
    m_canvas->SetIgnoreMouseEvents( true );
    DIALOG_PCB_TEXT_PROPERTIES dlg( this, TextPCB, DC );
    dlg.ShowModal();
    m_canvas->MoveCursorToCrossHair();
    m_canvas->SetIgnoreMouseEvents( false );
}


void DIALOG_PCB_TEXT_PROPERTIES::MyInit()
{
    SetFocus();

    // Put units symbols to text labels where appropriate
    AddUnitSymbol( *m_SizeXLabel );
    AddUnitSymbol( *m_SizeYLabel );
    AddUnitSymbol( *m_ThicknessLabel );
    AddUnitSymbol( *m_PositionXLabel );
    AddUnitSymbol( *m_PositionYLabel );

    // Fill fields with current values
    *m_TextContentCtrl << m_SelectedPCBText->m_Text;

    PutValueInLocalUnits( *m_SizeXCtrl, m_SelectedPCBText->m_Size.x,
        m_Parent->GetInternalUnits() );
    PutValueInLocalUnits( *m_SizeYCtrl, m_SelectedPCBText->m_Size.y,
        m_Parent->GetInternalUnits() );
    PutValueInLocalUnits( *m_ThicknessCtrl, m_SelectedPCBText->m_Thickness,
        m_Parent->GetInternalUnits() );
    PutValueInLocalUnits( *m_PositionXCtrl, m_SelectedPCBText->m_Pos.x,
        m_Parent->GetInternalUnits() );
    PutValueInLocalUnits( *m_PositionYCtrl, m_SelectedPCBText->m_Pos.y,
        m_Parent->GetInternalUnits() );

    int enabledLayers = m_Parent->GetBoard()->GetEnabledLayers();

    for( int layer = 0; layer < NB_LAYERS;  ++layer )
    {
        if( enabledLayers & (1 << layer) )
        {
            layerList.push_back( layer );
            int itemIndex =
                m_LayerSelectionCtrl->Append( m_Parent->GetBoard()->GetLayerName( layer ) );

            if( m_SelectedPCBText->GetLayer() == layer )
                m_LayerSelectionCtrl->SetSelection( itemIndex );
        }
    }

    switch( (int) m_SelectedPCBText->GetOrientation() )
    {
    default:
        m_OrientationCtrl->SetSelection( 0 );
        break;

    case 900:
    case -2700:
        m_OrientationCtrl->SetSelection( 1 );
        break;

    case 1800:
    case -1800:
        m_OrientationCtrl->SetSelection( 2 );
        break;

    case 2700:
    case -900:
        m_OrientationCtrl->SetSelection( 3 );
        break;
    }

    if( m_SelectedPCBText->m_Mirror )
        m_DisplayCtrl->SetSelection( 1 );
    else
        m_DisplayCtrl->SetSelection( 0 );

    if( m_SelectedPCBText->m_Italic )
        m_StyleCtrl->SetSelection( 1 );
    else
        m_StyleCtrl->SetSelection( 0 );

    // Set justification
    EDA_TEXT_HJUSTIFY_T hJustify = m_SelectedPCBText->GetHorizJustify();
    m_justifyChoice->SetSelection( (int) hJustify + 1 );

    // Set focus on most important control
    m_TextContentCtrl->SetFocus();
    m_TextContentCtrl->SetSelection( -1, -1 );
}


void DIALOG_PCB_TEXT_PROPERTIES::OnClose( wxCloseEvent& event )
{
    EndModal( 0 );
}


void DIALOG_PCB_TEXT_PROPERTIES::OnCancelClick( wxCommandEvent& event )
{
    EndModal( wxID_CANCEL );
}


void DIALOG_PCB_TEXT_PROPERTIES::OnOkClick( wxCommandEvent& event )
{
    wxPoint newPosition;
    wxSize  newSize;

    // If no other command in progress, prepare undo command
    // (for a command in progress, will be made later, at the completion of command)
    if( m_SelectedPCBText->GetFlags() == 0 )
        m_Parent->SaveCopyInUndoList( m_SelectedPCBText, UR_CHANGED );

    /* set flag in edit to force undo/redo/abort proper operation,
     * and avoid new calls to SaveCopyInUndoList for the same text
     * this can occurs when a text is moved, and then rotated, edited ..
    */
    if( m_SelectedPCBText->GetFlags() != 0 )
        m_SelectedPCBText->SetFlags( IN_EDIT );

    // Erase old text on screen if context is available
    if( m_DC )
    {
        m_SelectedPCBText->Draw( m_Parent->GetCanvas(), m_DC, GR_XOR );
    }

    // Set the new text content
    if( !m_TextContentCtrl->GetValue().IsEmpty() )
    {
        m_SelectedPCBText->m_Text = m_TextContentCtrl->GetValue();
    }

    // Set PCB Text position
    newPosition.x = ReturnValueFromString( g_UserUnit, m_PositionXCtrl->GetValue(),
                                           m_Parent->GetInternalUnits() );
    newPosition.y = ReturnValueFromString( g_UserUnit, m_PositionYCtrl->GetValue(),
                                           m_Parent->GetInternalUnits() );
    m_SelectedPCBText->m_Pos  = newPosition;

    // Check constraints and set PCB Text size
    newSize.x = ReturnValueFromString( g_UserUnit, m_SizeXCtrl->GetValue(),
                                       m_Parent->GetInternalUnits() );
    newSize.y = ReturnValueFromString( g_UserUnit, m_SizeYCtrl->GetValue(),
                                       m_Parent->GetInternalUnits() );

    if( newSize.x < TEXTS_MIN_SIZE )
        newSize.x = TEXTS_MIN_SIZE;

    if( newSize.y < TEXTS_MIN_SIZE )
        newSize.y = TEXTS_MIN_SIZE;

    if( newSize.x > TEXTS_MAX_WIDTH )
        newSize.x = TEXTS_MAX_WIDTH;

    if( newSize.y > TEXTS_MAX_WIDTH )
        newSize.y = TEXTS_MAX_WIDTH;

    m_SelectedPCBText->m_Size = newSize;

    // Set the new thickness
    m_SelectedPCBText->m_Thickness = ReturnValueFromString( g_UserUnit,
                                                            m_ThicknessCtrl->GetValue(),
                                                            m_Parent->GetInternalUnits() );

    // Test for acceptable values for thickness and size and clamp if fails
    int maxthickness = Clamp_Text_PenSize( m_SelectedPCBText->m_Thickness,
                                           m_SelectedPCBText->m_Size  );

    if( m_SelectedPCBText->m_Thickness > maxthickness )
    {
        DisplayError( NULL,
                      _( "The text thickness is too large for the text size. It will be clamped" ) );
        m_SelectedPCBText->m_Thickness = maxthickness;
    }

    // Set the layer on which the PCB text is laying
    m_SelectedPCBText->SetLayer( layerList[m_LayerSelectionCtrl->GetSelection()] );

    // Set whether the PCB text is mirrored (faced down from layer face perspective)
    m_SelectedPCBText->m_Mirror = (m_DisplayCtrl->GetSelection() == 1) ? true : false;

    // Set the text orientation
    m_SelectedPCBText->m_Orient = m_OrientationCtrl->GetSelection() * 900;

    // Set whether the PCB text is slanted (it is not italics, as italics has additional curves in style)
    m_SelectedPCBText->m_Italic = m_StyleCtrl->GetSelection() ? 1 : 0;

    // Set justification
    switch( m_justifyChoice->GetSelection() )
    {
    case 0:
        m_SelectedPCBText->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        break;
    case 1:
        m_SelectedPCBText->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
        break;
    case 2:
        m_SelectedPCBText->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        break;
    default:
        break;
    }

    // Finally, display new text if there is a context to do so
    if( m_DC )
    {
        m_SelectedPCBText->Draw( m_Parent->GetCanvas(), m_DC, GR_OR );
    }

    m_Parent->OnModify();
    EndModal( 1 );
}
