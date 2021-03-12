/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2018 Jean-Pierre Charras jp.charras at wanadoo.fr
 * Copyright (C) 2010-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialog_text_properties.h>
#include <confirm.h>
#include <widgets/unit_binder.h>
#include <board_commit.h>
#include <board.h>
#include <footprint.h>
#include <kicad_string.h>
#include <pcb_text.h>
#include <fp_text.h>
#include <pcb_edit_frame.h>
#include <pcb_layer_box_selector.h>
#include <wx/valnum.h>
#include <math/util.h>      // for KiROUND


DIALOG_TEXT_PROPERTIES::DIALOG_TEXT_PROPERTIES( PCB_BASE_EDIT_FRAME* aParent, BOARD_ITEM* aItem ) :
    DIALOG_TEXT_PROPERTIES_BASE( aParent ),
    m_Parent( aParent ),
    m_item( aItem ),
    m_edaText( nullptr ),
    m_fpText( nullptr ),
    m_pcbText( nullptr ),
    m_textWidth( aParent, m_SizeXLabel, m_SizeXCtrl, m_SizeXUnits ),
    m_textHeight( aParent, m_SizeYLabel, m_SizeYCtrl, m_SizeYUnits ),
    m_thickness( aParent, m_ThicknessLabel, m_ThicknessCtrl, m_ThicknessUnits ),
    m_posX( aParent, m_PositionXLabel, m_PositionXCtrl, m_PositionXUnits ),
    m_posY( aParent, m_PositionYLabel, m_PositionYCtrl, m_PositionYUnits ),
    m_orientation( aParent, m_OrientLabel, m_OrientCtrl, nullptr )
{
    wxString title;

    // Configure display origin transforms
    m_posX.SetCoordType( ORIGIN_TRANSFORMS::ABS_X_COORD );
    m_posY.SetCoordType( ORIGIN_TRANSFORMS::ABS_Y_COORD );

    m_MultiLineText->SetEOLMode( wxSTC_EOL_LF );

    // A hack which causes Scintilla to auto-size the text editor canvas
    // See: https://github.com/jacobslusser/ScintillaNET/issues/216
    m_MultiLineText->SetScrollWidth( 1 );
    m_MultiLineText->SetScrollWidthTracking( true );

    if( m_item->Type() == PCB_FP_TEXT_T )
    {
        title = _( "Footprint Text Properties" );

        m_fpText = (FP_TEXT*) m_item;
        m_edaText = static_cast<EDA_TEXT*>( m_fpText );

        switch( m_fpText->GetType() )
        {
        case FP_TEXT::TEXT_is_REFERENCE: m_TextLabel->SetLabel( _( "Reference:" ) ); break;
        case FP_TEXT::TEXT_is_VALUE:     m_TextLabel->SetLabel( _( "Value:" ) );     break;
        case FP_TEXT::TEXT_is_DIVERS:    m_TextLabel->SetLabel( _( "Text:" ) );      break;
        }

        SetInitialFocus( m_SingleLineText );
        m_MultiLineSizer->Show( false );
    }
    else
    {
        title = _( "Text Properties" );

        m_pcbText = (PCB_TEXT*) aItem;
        m_edaText = static_cast<EDA_TEXT*>( m_pcbText );

        SetInitialFocus( m_MultiLineText );
        m_SingleLineSizer->Show( false );

        int    size = wxNORMAL_FONT->GetPointSize();
        wxFont fixedFont( size, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL );

        for( size_t i = 0; i < wxSTC_STYLE_MAX; ++i )
            m_MultiLineText->StyleSetFont( i, fixedFont );

        // Addresses a bug in wx3.0 where styles are not correctly set
        m_MultiLineText->StyleClearAll();

        // This option makes sense only for footprint texts; texts on board are always visible.
        m_Visible->SetValue( true );
        m_Visible->Enable( false );

        m_KeepUpright->Show( false );
        m_statusLine->Show( false );
    }

    SetTitle( title );
    m_hash_key = title;

    // Configure the layers list selector.  Note that footprints are built outside the current
    // board and so we may need to show all layers if the text is on an unactivated layer.
    if( !m_Parent->GetBoard()->IsLayerEnabled( m_item->GetLayer() ) )
        m_LayerSelectionCtrl->ShowNonActivatedLayers( true );

    m_LayerSelectionCtrl->SetLayersHotkeys( false );
    m_LayerSelectionCtrl->SetBoardFrame( m_Parent );
    m_LayerSelectionCtrl->Resync();

    m_OrientValue = 0.0;
    m_orientation.SetUnits( EDA_UNITS::DEGREES );
    m_orientation.SetPrecision( 3 );

    // Set predefined rotations in combo dropdown, according to the locale floating point
    // separator notation
    double rot_list[] = { 0.0, 90.0, -90.0, 180.0 };

    for( size_t ii = 0; ii < m_OrientCtrl->GetCount() && ii < 4; ++ii )
        m_OrientCtrl->SetString( ii, wxString::Format( "%.1f", rot_list[ii] ) );

    // Set font sizes
    wxFont infoFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    infoFont.SetSymbolicSize( wxFONTSIZE_X_SMALL );
    m_statusLine->SetFont( infoFont );

    m_sdbSizerOK->SetDefault();

    // We can't set the tab order through wxWidgets due to shortcomings in their mnemonics
    // implementation on MSW
    m_tabOrder = {
            m_LayerLabel,
            m_LayerSelectionCtrl,
            m_SizeXCtrl,
            m_SizeYCtrl,
            m_ThicknessCtrl,
            m_PositionXCtrl,
            m_PositionYCtrl,
            m_Visible,
            m_Italic,
            m_JustifyChoice,
            m_OrientCtrl,
            m_Mirrored,
            m_KeepUpright,
            m_sdbSizerOK,
            m_sdbSizerCancel
    };

    // wxTextCtrls fail to generate wxEVT_CHAR events when the wxTE_MULTILINE flag is set,
    // so we have to listen to wxEVT_CHAR_HOOK events instead.
    Connect( wxEVT_CHAR_HOOK, wxKeyEventHandler( DIALOG_TEXT_PROPERTIES::OnCharHook ), NULL, this );

    finishDialogSettings();
}


DIALOG_TEXT_PROPERTIES::~DIALOG_TEXT_PROPERTIES()
{
    Disconnect( wxEVT_CHAR_HOOK, wxKeyEventHandler( DIALOG_TEXT_PROPERTIES::OnCharHook ), NULL, this );
}


// Launch the text properties dialog in quasi modal mode.
void PCB_BASE_EDIT_FRAME::ShowTextPropertiesDialog( BOARD_ITEM* aText )
{
    DIALOG_TEXT_PROPERTIES dlg( this, aText );
    dlg.ShowQuasiModal();
}


void DIALOG_TEXT_PROPERTIES::OnCharHook( wxKeyEvent& aEvent )
{
    if( aEvent.GetKeyCode() == WXK_RETURN && aEvent.ShiftDown() )
    {
        if( TransferDataFromWindow() )
        {
            // Do not use EndModal to close the dialog that can be opened
            // in quasi modal mode
            SetReturnCode( wxID_OK );
            Close();
        }
    }
    else if( m_MultiLineText->IsShown() && m_MultiLineText->HasFocus() )
    {
        if( aEvent.GetKeyCode() == WXK_TAB && !aEvent.ControlDown() )
        {
            m_MultiLineText->Tab();
        }
        else if( IsCtrl( 'Z', aEvent ) )
        {
            m_MultiLineText->Undo();
        }
        else if( IsShiftCtrl( 'Z', aEvent ) || IsCtrl( 'Y', aEvent ) )
        {
            m_MultiLineText->Redo();
        }
        else if( IsCtrl( 'X', aEvent ) )
        {
            m_MultiLineText->Cut();
        }
        else if( IsCtrl( 'C', aEvent ) )
        {
            m_MultiLineText->Copy();
        }
        else if( IsCtrl( 'V', aEvent ) )
        {
            m_MultiLineText->Paste();
        }
        else if( IsCtrl( 'A', aEvent ) )
        {
            m_MultiLineText->SelectAll();
        }
        else
        {
            aEvent.Skip();
        }
    }
    else
    {
        aEvent.Skip();
    }
}


void DIALOG_TEXT_PROPERTIES::OnSetFocusText( wxFocusEvent& event )
{
#ifdef __WXGTK__
    // Force an update of the text control before setting the text selection
    // This is needed because GTK seems to ignore the selection on first update
    //
    // Note that we can't do this on OSX as it tends to provoke Apple's
    // "[NSAlert runModal] may not be invoked inside of transaction begin/commit pair"
    // bug.  See: https://bugs.launchpad.net/kicad/+bug/1837225
    if( m_fpText->GetType() == FP_TEXT::TEXT_is_REFERENCE )
        m_SingleLineText->Update();
#endif

    if( m_fpText->GetType() == FP_TEXT::TEXT_is_REFERENCE )
        KIUI::SelectReferenceNumber( static_cast<wxTextEntry*>( m_SingleLineText ) );
    else
        m_SingleLineText->SetSelection( -1, -1 );

    event.Skip();
}


bool DIALOG_TEXT_PROPERTIES::TransferDataToWindow()
{
    if( m_SingleLineText->IsShown() )
    {
        m_SingleLineText->SetValue( m_edaText->GetText() );

        if( m_fpText && m_fpText->GetType() == FP_TEXT::TEXT_is_REFERENCE )
            KIUI::SelectReferenceNumber( static_cast<wxTextEntry*>( m_SingleLineText ) );
        else
            m_SingleLineText->SetSelection( -1, -1 );
    }
    else if( m_MultiLineText->IsShown() )
    {
        BOARD*   board = m_Parent->GetBoard();
        wxString converted = board->ConvertKIIDsToCrossReferences( m_edaText->GetText() );

        m_MultiLineText->SetValue( converted );
        m_MultiLineText->SetSelection( -1, -1 );
    }

    if( m_item->Type() == PCB_FP_TEXT_T && m_fpText )
    {
        FOOTPRINT* footprint = dynamic_cast<FOOTPRINT*>( m_fpText->GetParent() );
        wxString   msg;

        if( footprint )
        {
            msg.Printf( _( "Footprint %s (%s), %s, rotated %.1f deg"),
                        footprint->GetReference(),
                        footprint->GetValue(),
                        footprint->IsFlipped() ? _( "back side (mirrored)" ) : _( "front side" ),
                        footprint->GetOrientation() / 10.0 );
        }

        m_statusLine->SetLabel( msg );
    }
    else
    {
        m_statusLine->Show( false );
    }

    m_cbLocked->SetValue( m_item->IsLocked() );

    m_LayerSelectionCtrl->SetLayerSelection( m_item->GetLayer() );

    m_textWidth.SetValue( m_edaText->GetTextSize().x );
    m_textHeight.SetValue( m_edaText->GetTextSize().y );
    m_thickness.SetValue( m_edaText->GetTextThickness() );
    m_posX.SetValue( m_edaText->GetTextPos().x );
    m_posY.SetValue( m_edaText->GetTextPos().y );

    m_Visible->SetValue( m_edaText->IsVisible() );
    m_Italic->SetValue( m_edaText->IsItalic() );
    EDA_TEXT_HJUSTIFY_T hJustify = m_edaText->GetHorizJustify();
    m_JustifyChoice->SetSelection( (int) hJustify + 1 );
    m_OrientValue = m_edaText->GetTextAngle();
    m_orientation.SetDoubleValue( m_OrientValue );
    m_Mirrored->SetValue( m_edaText->IsMirrored() );

    if( m_fpText )
        m_KeepUpright->SetValue( m_fpText->IsKeepUpright() );

    return DIALOG_TEXT_PROPERTIES_BASE::TransferDataToWindow();
}


bool DIALOG_TEXT_PROPERTIES::TransferDataFromWindow()
{
    if( !DIALOG_TEXT_PROPERTIES_BASE::TransferDataFromWindow() )
        return false;

    if( !m_textWidth.Validate( TEXTS_MIN_SIZE, TEXTS_MAX_SIZE )
        || !m_textHeight.Validate( TEXTS_MIN_SIZE, TEXTS_MAX_SIZE ) )
    {
        return false;
    }

    BOARD_COMMIT commit( m_Parent );
    commit.Modify( m_item );

    // If no other command in progress, prepare undo command
    // (for a command in progress, will be made later, at the completion of command)
    bool pushCommit = ( m_item->GetEditFlags() == 0 );

    // Set IN_EDIT flag to force undo/redo/abort proper operation and avoid new calls to
    // SaveCopyInUndoList for the same text if is moved, and then rotated, edited, etc....
    if( !pushCommit )
        m_item->SetFlags( IN_EDIT );

    // Set the new text content
    if( m_SingleLineText->IsShown() )
    {
        if( !m_SingleLineText->GetValue().IsEmpty() )
            m_edaText->SetText( m_SingleLineText->GetValue() );
    }
    else if( m_MultiLineText->IsShown() )
    {
        if( !m_MultiLineText->GetValue().IsEmpty() )
        {
            BOARD*   board = m_Parent->GetBoard();
            wxString txt = board->ConvertCrossReferencesToKIIDs( m_MultiLineText->GetValue() );

#ifdef __WINDOWS__
            // On Windows, a new line is coded as \r\n.  We use only \n in kicad files and in
            // drawing routines so strip the \r char.
            txt.Replace( "\r", "" );
#endif
            m_edaText->SetText( EscapeString( txt, CTX_QUOTED_STR ) );
        }
    }

    m_item->SetLocked( m_cbLocked->GetValue() );

    m_item->SetLayer( ToLAYER_ID( m_LayerSelectionCtrl->GetLayerSelection() ) );

    m_edaText->SetTextSize( wxSize( m_textWidth.GetValue(), m_textHeight.GetValue() ) );
    m_edaText->SetTextThickness( m_thickness.GetValue() );
    m_edaText->SetTextPos( wxPoint( m_posX.GetValue(), m_posY.GetValue() ) );

    if( m_fpText )
        m_fpText->SetLocalCoord();

    // Test for acceptable values for thickness and size and clamp if fails
    int maxPenWidth = Clamp_Text_PenSize( m_edaText->GetTextThickness(), m_edaText->GetTextSize() );

    if( m_edaText->GetTextThickness() > maxPenWidth )
    {
        DisplayError( this, _( "The text thickness is too large for the text size.\n"
                               "It will be clamped." ) );
        m_edaText->SetTextThickness( maxPenWidth );
    }

    m_edaText->SetVisible( m_Visible->GetValue() );
    m_edaText->SetItalic( m_Italic->GetValue() );
    m_OrientValue = m_orientation.GetDoubleValue();
    m_edaText->SetTextAngle( m_OrientValue );
    m_edaText->SetMirrored( m_Mirrored->GetValue() );

    if( m_fpText )
        m_fpText->SetKeepUpright( m_KeepUpright->GetValue() );

    switch( m_JustifyChoice->GetSelection() )
    {
    case 0: m_edaText->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );   break;
    case 1: m_edaText->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER ); break;
    case 2: m_edaText->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );  break;
    default: break;
    }

    if( pushCommit )
        commit.Push( _( "Change text properties" ) );

    return true;
}
