/**
 * @file dialog_edit_module_text.cpp
 * @brief dialog editor for texts (fields) in footprints.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras
 * Copyright (C) 2013 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2008-2013 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <macros.h>
#include <gr_basic.h>
#include <common.h>
#include <class_drawpanel.h>
#include <pcbnew.h>
#include <drawtxt.h>
#include <confirm.h>
#include <wxBasePcbFrame.h>
#include <base_units.h>

#include <class_module.h>
#include <class_text_mod.h>
#include <class_board.h>

#include <dialog_edit_module_text.h>


extern wxPoint MoveVector;  // Move vector for move edge, imported from edtxtmod.cpp


void PCB_BASE_FRAME::InstallTextModOptionsFrame( TEXTE_MODULE* TextMod, wxDC* DC )
{
    m_canvas->SetIgnoreMouseEvents( true );
    DialogEditModuleText dialog( this, TextMod, DC );
    dialog.ShowModal();
    m_canvas->SetIgnoreMouseEvents( false );
}


DialogEditModuleText::DialogEditModuleText( PCB_BASE_FRAME* aParent,
                                            TEXTE_MODULE* aTextMod, wxDC* aDC ) :
    DialogEditModuleText_base( aParent )

{
    m_parent = aParent;
    m_dc     = aDC;
    m_module = NULL;
    m_currentText = aTextMod;

    if( m_currentText )
        m_module = (MODULE*) m_currentText->GetParent();

    initDlg();

    m_sdbSizer1OK->SetDefault();

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );

    Centre();
}


void DialogEditModuleText::OnCancelClick( wxCommandEvent& event )
{
   EndModal( 0 );
}


void DialogEditModuleText::initDlg( )
{
    SetFocus();

    wxString msg;

    if( m_module )
    {
        wxString format = m_ModuleInfoText->GetLabel();
        msg.Printf( format,
                    GetChars( m_module->GetReference() ),
                    GetChars( m_module->GetValue() ),
                    m_module->GetOrientation() / 10.0 );
    }
    else
    {
        msg.Empty();
    }

    m_ModuleInfoText->SetLabel( msg );

    // Create a list of not allowed layers.
    // could be slightly dependent of the type of footprint text.
    LSET forbiddenLayers( LSET::AllCuMask() );
    forbiddenLayers.set( Edge_Cuts ).set( Margin ).set( F_Paste ).set( B_Paste ).set( F_Mask ).set( B_Mask );

    switch( m_currentText->GetType() )
    {
    case TEXTE_MODULE::TEXT_is_VALUE:
        m_TextDataTitle->SetLabel( _( "Value:" ) );
        break;

    case TEXTE_MODULE::TEXT_is_DIVERS:
        m_TextDataTitle->SetLabel( _( "Text:" ) );
        break;

    case TEXTE_MODULE::TEXT_is_REFERENCE:
        m_TextDataTitle->SetLabel( _( "Reference:" ) );
        break;
    }

    m_Name->SetValue( m_currentText->GetText() );

    m_Style->SetSelection( m_currentText->IsItalic() ? 1 : 0 );

    AddUnitSymbol( *m_SizeXTitle );
    PutValueInLocalUnits( *m_TxtSizeCtrlX, m_currentText->GetSize().x );

    AddUnitSymbol( *m_SizeYTitle );
    PutValueInLocalUnits( *m_TxtSizeCtrlY, m_currentText->GetSize().y );

    AddUnitSymbol( *m_PosXTitle );
    PutValueInLocalUnits( *m_TxtPosCtrlX, m_currentText->GetPos0().x );

    AddUnitSymbol( *m_PosYTitle );
    PutValueInLocalUnits( *m_TxtPosCtrlY, m_currentText->GetPos0().y );

    AddUnitSymbol( *m_WidthTitle );
    PutValueInLocalUnits( *m_TxtWidthCtlr, m_currentText->GetThickness() );

    double text_orient = m_currentText->GetOrientation();
    NORMALIZE_ANGLE_90( text_orient );

    if( (text_orient != 0) )
        m_Orient->SetSelection( 1 );

    if( !m_currentText->IsVisible() )
        m_Show->SetSelection( 1 );

    // Configure the layers list selector
    if( !m_parent->GetBoard()->IsLayerEnabled( m_currentText->GetLayer() ) )
        // Footprints are built outside the current board, so items cann be
        // on a not activated layer, therefore show it if happens.
        m_LayerSelectionCtrl->ShowNonActivatedLayers( true );

    m_LayerSelectionCtrl->SetLayersHotkeys( false );
    m_LayerSelectionCtrl->SetLayerSet( forbiddenLayers );
    m_LayerSelectionCtrl->SetBoardFrame( m_parent );
    m_LayerSelectionCtrl->Resync();

    if( m_LayerSelectionCtrl->SetLayerSelection( m_currentText->GetLayer() ) < 0 )
    {
        wxMessageBox( _( "This item has an illegal layer id.\n"
                        "Now, forced on the front silk screen layer. Please, fix it" ) );
        m_LayerSelectionCtrl->SetLayerSelection( F_SilkS );
    }
}


void DialogEditModuleText::OnOkClick( wxCommandEvent& event )
{
    wxString msg;

    if( m_module )
        m_parent->SaveCopyInUndoList( m_module, UR_CHANGED );

#ifndef USE_WX_OVERLAY
    if( m_dc )     //Erase old text on screen
    {
        m_currentText->Draw( m_parent->GetCanvas(), m_dc, GR_XOR,
                             (m_currentText->IsMoving()) ? MoveVector : wxPoint( 0, 0 ) );
    }
#endif

    m_currentText->SetText( m_Name->GetValue() );
    m_currentText->SetItalic( m_Style->GetSelection() == 1 );

    wxPoint tmp;

    msg = m_TxtPosCtrlX->GetValue();
    tmp.x = ValueFromString( g_UserUnit, msg );

    msg = m_TxtPosCtrlY->GetValue();
    tmp.y = ValueFromString( g_UserUnit, msg );

    m_currentText->SetPos0( tmp );

    wxSize textSize( wxSize( ValueFromString( g_UserUnit, m_TxtSizeCtrlX->GetValue() ),
                             ValueFromString( g_UserUnit, m_TxtSizeCtrlY->GetValue() ) ) );

    // Test for a reasonable size:
    if( textSize.x < TEXTS_MIN_SIZE )
        textSize.x = TEXTS_MIN_SIZE;

    if( textSize.y < TEXTS_MIN_SIZE )
        textSize.y = TEXTS_MIN_SIZE;

    m_currentText->SetSize( textSize ),

    msg = m_TxtWidthCtlr->GetValue();
    int width = ValueFromString( g_UserUnit, msg );

    // Test for a reasonable width:
    if( width <= 1 )
        width = 1;

    int maxthickness = Clamp_Text_PenSize(width, m_currentText->GetSize() );

    if( width > maxthickness )
    {
        DisplayError( NULL,
                      _( "The text thickness is too large for the text size. It will be clamped" ) );
        width = maxthickness;
    }

    m_currentText->SetThickness( width );

    m_currentText->SetVisible( m_Show->GetSelection() == 0 );

    int text_orient = (m_Orient->GetSelection() == 0) ? 0 : 900;
    m_currentText->SetOrientation( text_orient );

    m_currentText->SetDrawCoord();

    LAYER_NUM layer = m_LayerSelectionCtrl->GetLayerSelection();
    m_currentText->SetLayer( ToLAYER_ID( layer ) );
    m_currentText->SetMirrored( IsBackLayer( m_currentText->GetLayer() ) );

#ifndef USE_WX_OVERLAY
    if( m_dc )     // Display new text
    {
        m_currentText->Draw( m_parent->GetCanvas(), m_dc, GR_XOR,
                             (m_currentText->IsMoving()) ? MoveVector : wxPoint( 0, 0 ) );
    }
#else
    m_parent->Refresh();
#endif

    m_parent->OnModify();

    if( m_module )
        m_module->SetLastEditTime();

    EndModal( 1 );
}
