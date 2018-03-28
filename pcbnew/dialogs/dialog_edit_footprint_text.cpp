/**
 * @file dialog_edit_module_text.cpp
 * @brief dialog editor for texts (fields) in footprints.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras
 * Copyright (C) 2013 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <draw_graphic_text.h>
#include <confirm.h>
#include <pcb_base_frame.h>
#include <base_units.h>
#include <wx/numformatter.h>
#include <board_commit.h>
#include <widgets/text_ctrl_eval.h>

#include <class_module.h>
#include <class_text_mod.h>
#include <class_board.h>

#include <pcb_layer_box_selector.h>
#include <dialog_edit_footprint_text.h>


extern wxPoint MoveVector;  // Move vector for move edge, imported from edtxtmod.cpp


void PCB_BASE_FRAME::InstallTextModOptionsFrame( TEXTE_MODULE* TextMod, wxDC* DC )
{
    m_canvas->SetIgnoreMouseEvents( true );
    DIALOG_EDIT_FPTEXT dialog( this, this, TextMod, DC );
    dialog.ShowModal();
    m_canvas->SetIgnoreMouseEvents( false );
}


DIALOG_EDIT_FPTEXT::DIALOG_EDIT_FPTEXT( wxWindow* aCaller, PCB_BASE_FRAME* aFrame,
                                        TEXTE_MODULE* aTextMod, wxDC* aDC ) :
    DIALOG_EDIT_FPTEXT_BASE( aCaller ),
    m_frame( aFrame ), m_dc( aDC ), m_text( aTextMod ),
    m_textWidth( aFrame, m_widthLabel, m_widthCtrl, m_widthUnits, true, TEXTS_MIN_SIZE ),
    m_textHeight( aFrame, m_heightLabel, m_heightCtrl, m_heightUnits, true, TEXTS_MIN_SIZE ),
    m_thickness( aFrame, m_thicknessLabel, m_thicknessCtrl, m_thicknessUnits, true, 0 ),
    m_posX( aFrame, m_posXLabel, m_posXCtrl, m_posXUnits ),
    m_posY( aFrame, m_posYLabel, m_posYCtrl, m_posYUnits ),
    m_OrientValidator( 1, &m_OrientValue )
{
    if( m_text )
        m_module = (MODULE*) m_text->GetParent();

    m_OrientValue = 0;

    switch( m_text->GetType() )
    {
    case TEXTE_MODULE::TEXT_is_REFERENCE: m_TextDataTitle->SetLabel( _( "Reference:" ) ); break;
    case TEXTE_MODULE::TEXT_is_VALUE:     m_TextDataTitle->SetLabel( _( "Value:" ) );     break;
    case TEXTE_MODULE::TEXT_is_DIVERS:    m_TextDataTitle->SetLabel( _( "Text:" ) );      break;
    }

    // Configure the layers list selector.  Note that footprints are built outside the current
    // board and so we may need to show all layers if the text is on an unactivated layer.
    if( !m_frame->GetBoard()->IsLayerEnabled( m_text->GetLayer() ) )
        m_LayerSelectionCtrl->ShowNonActivatedLayers( true );

    m_LayerSelectionCtrl->SetLayersHotkeys( false );
    m_LayerSelectionCtrl->SetNotAllowedLayerSet( LSET::ForbiddenTextLayers() );
    m_LayerSelectionCtrl->SetBoardFrame( m_frame );
    m_LayerSelectionCtrl->Resync();

    m_OrientValidator.SetRange( -180.0, 180.0 );
    m_OrientValueCtrl->SetValidator( m_OrientValidator );
    m_OrientValidator.SetWindow( m_OrientValueCtrl );

    m_sdbSizerOK->SetDefault();
    SetInitialFocus( m_Name );

    Layout();
    FinishDialogSettings();
}


bool DIALOG_EDIT_FPTEXT::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    wxString msg1, msg2;

    if( m_module )
    {
        msg1.Printf( _("Footprint %s (%s),"), m_module->GetReference(), m_module->GetValue() );
        msg2.Printf( _("orientation %.1f deg"), m_module->GetOrientation() / 10.0 );
    }

    m_statusLine1->SetLabel( msg1 );
    m_statusLine2->SetLabel( msg2 );

    m_Name->SetValue( m_text->GetText() );
    m_textWidth.SetValue( m_text->GetTextWidth() );
    m_textHeight.SetValue( m_text->GetTextHeight() );
    m_thickness.SetValue(  m_text->GetThickness() );
    m_posX.SetValue( m_text->GetPos0().x );
    m_posY.SetValue( m_text->GetPos0().y );

    m_Show->SetValue( m_text->IsVisible() );
    m_Italic->SetValue(  m_text->IsItalic() );

    m_OrientValue = m_text->GetTextAngle() / 10.0;
    m_Orient0->SetValue( m_OrientValue == 0.0 );
    m_Orient90->SetValue( m_OrientValue == 90.0 || m_OrientValue == -270.0 );
    m_Orient270->SetValue( m_OrientValue == 270.0 || m_OrientValue == -90.0 );
    m_Orient180->SetValue( m_OrientValue == 180.0 || m_OrientValue == -180.0 );

    m_OrientOther->SetValue( !m_Orient0->GetValue() && !m_Orient90->GetValue()
                             && !m_Orient270->GetValue() && !m_Orient180->GetValue() );
    m_OrientValidator.TransferToWindow();

    m_unlock->SetValue( m_text->IsUnlocked() );

    if( m_LayerSelectionCtrl->SetLayerSelection( m_text->GetLayer() ) < 0 )
    {
        wxString layerName = m_frame->GetBoard()->GetLayerName( ToLAYER_ID( m_text->GetLayer() ) );
        DisplayError( this, wxString::Format( _( "Text not allowed on %s." ), layerName ) );
        m_LayerSelectionCtrl->SetLayerSelection( F_SilkS );
    }

    return true;
}


bool DIALOG_EDIT_FPTEXT::TransferDataFromWindow()
{
    BOARD_COMMIT commit( m_frame );

    if( !Validate() || !DIALOG_EDIT_FPTEXT_BASE::TransferDataFromWindow() )
        return false;

    if( !m_textWidth.Validate( true ) || !m_textHeight.Validate( true ) )
        return false;

    if( m_module )
        commit.Modify( m_text );

#ifndef USE_WX_OVERLAY
    if( m_dc )     // Erase old text on screen
    {
        m_text->Draw( m_frame->GetCanvas(), m_dc, GR_XOR,
                      m_text->IsMoving() ? MoveVector : wxPoint( 0, 0 ) );
    }
#endif

    m_text->SetText( m_Name->GetValue() );
    m_text->SetItalic( m_Italic->GetValue() );
    m_text->SetVisible( m_Show->GetValue() );

    m_text->SetPos0( wxPoint( m_posX.GetValue(), m_posY.GetValue() ) );
    m_text->SetTextSize( wxSize( m_textWidth.GetValue(), m_textHeight.GetValue() ) );
    m_text->SetThickness( Clamp_Text_PenSize( m_thickness.GetValue(), m_text->GetTextSize() ) );

    m_text->SetTextAngle( KiROUND( m_OrientValue * 10.0 ) );

    m_text->SetDrawCoord();

    m_text->SetUnlocked( m_unlock->GetValue() );

    m_text->SetLayer( ToLAYER_ID( m_LayerSelectionCtrl->GetLayerSelection() ) );
    m_text->SetMirrored( IsBackLayer( m_text->GetLayer() ) );

#ifndef USE_WX_OVERLAY
    if( m_dc )     // Display new text
    {
        m_text->Draw( m_frame->GetCanvas(), m_dc, GR_XOR,
                      m_text->IsMoving() ? MoveVector : wxPoint( 0, 0 ) );
    }
#else
    m_frame->Refresh();
#endif

    commit.Push( _( "Modify module text" ) );

    if( m_module )
        m_module->SetLastEditTime();

    return true;
}


void DIALOG_EDIT_FPTEXT::ModuleOrientEvent( wxCommandEvent&  )
{
    if( m_Orient0->GetValue() )
        m_OrientValue = 0.0;
    else if( m_Orient90->GetValue() )
        m_OrientValue = 90.0;
    else if( m_Orient270->GetValue() )
        m_OrientValue = 270.0;
    else if( m_Orient180->GetValue() )
        m_OrientValue = 180.0;

    m_OrientValidator.TransferToWindow();
}


void DIALOG_EDIT_FPTEXT::OnOtherOrientation( wxKeyEvent& aEvent )
{
    m_OrientOther->SetValue( true );

    aEvent.Skip();
}


