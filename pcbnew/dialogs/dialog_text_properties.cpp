/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2018 Jean-Pierre Charras jp.charras at wanadoo.fr
 * Copyright (C) 2010-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <pcbnew.h>
#include <pcb_edit_frame.h>
#include <draw_graphic_text.h>
#include <confirm.h>
#include <wx/valnum.h>
#include <pcb_layer_box_selector.h>
#include <board_commit.h>
#include <widgets/unit_binder.h>
#include <class_board.h>
#include <class_pcb_text.h>
#include <class_text_mod.h>
#include <class_module.h>
#include <class_dimension.h>

#include <dialog_text_properties.h>


/**
 *  DIALOG_PCB_TEXT_PROPERTIES, derived from DIALOG_PCB_TEXT_PROPERTIES_BASE
 *  @see dialog_dialog_pcb_text_properties_base.h and
 *  dialog_dialog_pcb_text_properties_base.cpp, automatically created by
 *  wxFormBuilder.
 */

DIALOG_TEXT_PROPERTIES::DIALOG_TEXT_PROPERTIES( PCB_BASE_EDIT_FRAME* aParent, BOARD_ITEM* aItem,
                                                wxDC* aDC ) :
    DIALOG_TEXT_PROPERTIES_BASE( aParent ),
    m_Parent( aParent ), m_DC( aDC ), m_item( aItem ),
    m_textWidth( aParent, m_SizeXLabel, m_SizeXCtrl, m_SizeXUnits, true, TEXTS_MIN_SIZE ),
    m_textHeight( aParent, m_SizeYLabel, m_SizeYCtrl, m_SizeYUnits, true, TEXTS_MIN_SIZE ),
    m_thickness( aParent, m_ThicknessLabel, m_ThicknessCtrl, m_ThicknessUnits, true, 0 ),
    m_posX( aParent, m_PositionXLabel, m_PositionXCtrl, m_PositionXUnits ),
    m_posY( aParent, m_PositionYLabel, m_PositionYCtrl, m_PositionYUnits ),
    m_OrientValidator( 1, &m_OrientValue )
{
    wxString title, label;
    bool     multiLine = false;

    if( m_item->Type() == PCB_DIMENSION_T )
    {
        title = _( "Dimension Text Properties" );

        DIMENSION* dimension = (DIMENSION*) m_item;
        m_edaText = &dimension->Text();
        m_pcbText = &dimension->Text();

        label = _( "Dimension text:" );

        m_KeepUpright->Show( false );
    }
    else if( m_item->Type() == PCB_MODULE_TEXT_T )
    {
        title = _( "Footprint Text Properties" );

        m_modText = (TEXTE_MODULE*) m_item;
        m_edaText = static_cast<EDA_TEXT*>( m_modText );

        switch( m_modText->GetType() )
        {
        case TEXTE_MODULE::TEXT_is_REFERENCE: label = _( "Reference:" ); break;
        case TEXTE_MODULE::TEXT_is_VALUE:     label = _( "Value:" );     break;
        case TEXTE_MODULE::TEXT_is_DIVERS:    label = _( "Text:" );      break;
        }
    }
    else
    {
        title = _( "Text Properties" );

        m_pcbText = (TEXTE_PCB*) aItem;
        m_edaText = static_cast<EDA_TEXT*>( m_pcbText );
        multiLine = true;

        m_KeepUpright->Show( false );
    }

    SetTitle( title );
    m_hash_key = title;

    m_TextLabel->SetLabel( label );
    m_SingleLineSizer->Show( !multiLine );
    m_MultiLineSizer->Show( multiLine );
    SetInitialFocus( multiLine ? m_MultiLineText : m_SingleLineText );

    // Configure the layers list selector.  Note that footprints are built outside the current
    // board and so we may need to show all layers if the text is on an unactivated layer.
    if( !m_Parent->GetBoard()->IsLayerEnabled( m_item->GetLayer() ) )
        m_LayerSelectionCtrl->ShowNonActivatedLayers( true );

    m_LayerSelectionCtrl->SetLayersHotkeys( false );
    m_LayerSelectionCtrl->SetNotAllowedLayerSet( LSET::ForbiddenTextLayers() );
    m_LayerSelectionCtrl->SetBoardFrame( m_Parent );
    m_LayerSelectionCtrl->Resync();

    m_OrientValue = 0.0;
    m_OrientValidator.SetRange( -360.0, 360.0 );
    m_OrientCtrl->SetValidator( m_OrientValidator );
    m_OrientValidator.SetWindow( m_OrientCtrl );

    // Set font sizes
    wxFont infoFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    infoFont.SetSymbolicSize( wxFONTSIZE_X_SMALL );
    m_statusLine1->SetFont( infoFont );
    m_statusLine2->SetFont( infoFont );

    m_sdbSizerOK->SetDefault();

    // wxTextCtrls fail to generate wxEVT_CHAR events when the wxTE_MULTILINE flag is set,
    // so we have to listen to wxEVT_CHAR_HOOK events instead.
    m_MultiLineText->Connect( wxEVT_CHAR_HOOK, wxKeyEventHandler( DIALOG_TEXT_PROPERTIES::OnCharHook ), NULL, this );

    FinishDialogSettings();
}


DIALOG_TEXT_PROPERTIES::~DIALOG_TEXT_PROPERTIES()
{
    m_MultiLineText->Disconnect( wxEVT_CHAR_HOOK, wxKeyEventHandler( DIALOG_TEXT_PROPERTIES::OnCharHook ), NULL, this );
}


/**
 * Routine for main window class to launch text properties dialog.
 */
void PCB_BASE_EDIT_FRAME::InstallTextOptionsFrame( BOARD_ITEM* aText, wxDC* aDC )
{
    m_canvas->SetIgnoreMouseEvents( true );
#ifndef __WXMAC__
    DIALOG_TEXT_PROPERTIES dlg( this, aText, aDC );
#else
    // Avoid "writes" in the dialog, creates errors with WxOverlay and NSView
    // Raising an Exception - Fixes #891347
    DIALOG_TEXT_PROPERTIES dlg( this, aText, NULL );
#endif
    dlg.ShowModal();
    m_canvas->MoveCursorToCrossHair();
    m_canvas->SetIgnoreMouseEvents( false );
}


void DIALOG_TEXT_PROPERTIES::OnCharHook( wxKeyEvent& aEvent )
{
    if( aEvent.GetKeyCode() == WXK_TAB )
    {
        int flags = 0;

        if( !aEvent.ShiftDown() )
            flags |= wxNavigationKeyEvent::IsForward;

        if( aEvent.ControlDown() )
            flags |= wxNavigationKeyEvent::WinChange;

        NavigateIn( flags );
    }
    else if( aEvent.GetKeyCode() == WXK_RETURN && aEvent.ShiftDown() )
    {
        TransferDataFromWindow();
        EndModal( wxID_OK );
    }
    else
    {
        aEvent.Skip();
    }
}


bool DIALOG_TEXT_PROPERTIES::TransferDataToWindow()
{
    wxString      msg1, msg2;

    if( m_SingleLineText->IsShown() )
    {
        m_SingleLineText->SetValue( m_edaText->GetText() );
        m_SingleLineText->SetSelection( -1, -1 );
    }
    else
    {
        m_MultiLineText->SetValue( m_edaText->GetText() );
        m_MultiLineText->SetSelection( -1, -1 );
    }

    if( m_item->Type() == PCB_MODULE_TEXT_T )
    {
        MODULE* module = dynamic_cast<MODULE*>( m_modText->GetParent() );

        if( module )
        {
            wxString side = module->IsFlipped() ? _( "back side (mirrored)" ) : _( "front side" );
            msg1.Printf( _("Footprint %s (%s),"), module->GetReference(), module->GetValue() );
            msg2.Printf( _("%s, rotated %.1f deg"), side, module->GetOrientation() / 10.0 );
        }
    }

    m_statusLine1->SetLabel( msg1 );
    m_statusLine2->SetLabel( msg2 );

    m_LayerSelectionCtrl->SetLayerSelection( m_item->GetLayer() );

    m_textWidth.SetValue( m_edaText->GetTextSize().x );
    m_textHeight.SetValue( m_edaText->GetTextSize().y );
    m_thickness.SetValue( m_edaText->GetThickness() );
    m_posX.SetValue( m_edaText->GetTextPos().x );
    m_posY.SetValue( m_edaText->GetTextPos().y );

    m_Visible->SetValue( m_edaText->IsVisible() );
    m_Italic->SetValue( m_edaText->IsItalic() );
    EDA_TEXT_HJUSTIFY_T hJustify = m_edaText->GetHorizJustify();
    m_JustifyChoice->SetSelection( (int) hJustify + 1 );
    m_OrientValue = m_edaText->GetTextAngleDegrees();
    m_Mirrored->SetValue( m_edaText->IsMirrored() );

    if( m_modText )
        m_KeepUpright->SetValue( m_modText->IsKeepUpright() );

    return DIALOG_TEXT_PROPERTIES_BASE::TransferDataToWindow();
}


bool DIALOG_TEXT_PROPERTIES::TransferDataFromWindow()
{
    if( !DIALOG_TEXT_PROPERTIES_BASE::TransferDataFromWindow() )
        return false;

    // Test for acceptable layer.
    // Incorrect layer can happen for old boards, having texts on edge cut layer for instance
    if( m_LayerSelectionCtrl->GetLayerSelection() < 0 )
    {
        wxMessageBox( _( "No layer selected, Please select the text layer" ) );
        return false;
    }

    if( !m_textWidth.Validate( true ) || !m_textHeight.Validate( true ) )
        return false;

    if( !m_thickness.Validate( true ) )
        return false;

    BOARD_COMMIT commit( m_Parent );
    commit.Modify( m_item );

    // If no other command in progress, prepare undo command
    // (for a command in progress, will be made later, at the completion of command)
    bool pushCommit = ( m_item->GetFlags() == 0 );

    /* set flag in edit to force undo/redo/abort proper operation,
     * and avoid new calls to SaveCopyInUndoList for the same text
     * this can occurs when a text is moved, and then rotated, edited ..
    */
    if( m_item->GetFlags() != 0 )
        m_item->SetFlags( IN_EDIT );

#ifndef USE_WX_OVERLAY
    // Erase old text on screen if context is available
    if( m_DC )
    {
        m_item->Draw( m_Parent->GetCanvas(), m_DC, GR_XOR );
    }
#endif

    // Set the new text content
    if( m_SingleLineText->IsShown() && !m_SingleLineText->GetValue().IsEmpty() )
        m_edaText->SetText( m_SingleLineText->GetValue() );
    else if( m_MultiLineText->IsShown() && !m_MultiLineText->GetValue().IsEmpty() )
        m_edaText->SetText( m_MultiLineText->GetValue() );

    m_item->SetLayer( ToLAYER_ID( m_LayerSelectionCtrl->GetLayerSelection() ) );

    m_edaText->SetTextSize( wxSize( m_textWidth.GetValue(), m_textHeight.GetValue() ) );
    m_edaText->SetThickness( m_thickness.GetValue() );
    m_edaText->SetTextPos( wxPoint( m_posX.GetValue(), m_posY.GetValue() ) );

    // Test for acceptable values for thickness and size and clamp if fails
    int maxthickness = Clamp_Text_PenSize( m_edaText->GetThickness(), m_edaText->GetTextSize() );

    if( m_edaText->GetThickness() > maxthickness )
    {
        DisplayError( NULL, _( "The text thickness is too large for the text size.\n"
                               "It will be clamped." ) );
        m_edaText->SetThickness( maxthickness );
    }

    m_edaText->SetVisible( m_Visible->GetValue() );
    m_edaText->SetItalic( m_Italic->GetValue() );
    m_edaText->SetTextAngle( KiROUND( m_OrientValue * 10.0 ) );
    m_edaText->SetMirrored( m_Mirrored->GetValue() );

    if( m_modText )
        m_modText->SetKeepUpright( m_KeepUpright->GetValue() );

    switch( m_JustifyChoice->GetSelection() )
    {
    case 0: m_edaText->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );   break;
    case 1: m_edaText->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER ); break;
    case 2: m_edaText->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );  break;
    default: break;
    }

#ifndef USE_WX_OVERLAY
    // Finally, display new text if there is a context to do so
    if( m_DC )
    {
        m_item->Draw( m_Parent->GetCanvas(), m_DC, GR_OR );
    }
#else
    m_Parent->Refresh();
#endif

    if( pushCommit )
        commit.Push( _( "Change text properties" ) );

    return true;
}
