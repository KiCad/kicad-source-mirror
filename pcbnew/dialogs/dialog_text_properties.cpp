/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2018 Jean-Pierre Charras jp.charras at wanadoo.fr
 * Copyright (C) 2010-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <gr_text.h>
#include <fctsys.h>
#include <widgets/tab_traversal.h>
#include <widgets/unit_binder.h>
#include <board_commit.h>
#include <class_board.h>
#include <class_module.h>
#include <class_pcb_text.h>
#include <class_text_mod.h>
#include <pcb_edit_frame.h>
#include <pcb_layer_box_selector.h>
#include <pcbnew.h>
#include <wx/valnum.h>
#include <math/util.h>      // for KiROUND


DIALOG_TEXT_PROPERTIES::DIALOG_TEXT_PROPERTIES( PCB_BASE_EDIT_FRAME* aParent, BOARD_ITEM* aItem ) :
    DIALOG_TEXT_PROPERTIES_BASE( aParent ),
    m_Parent( aParent ),
    m_item( aItem ),
    m_edaText( nullptr ),
    m_modText( nullptr ),
    m_pcbText( nullptr ),
    m_textWidth( aParent, m_SizeXLabel, m_SizeXCtrl, m_SizeXUnits, true ),
    m_textHeight( aParent, m_SizeYLabel, m_SizeYCtrl, m_SizeYUnits, true ),
    m_thickness( aParent, m_ThicknessLabel, m_ThicknessCtrl, m_ThicknessUnits, true ),
    m_posX( aParent, m_PositionXLabel, m_PositionXCtrl, m_PositionXUnits ),
    m_posY( aParent, m_PositionYLabel, m_PositionYCtrl, m_PositionYUnits ),
    m_linesThickness( aParent, m_LineThicknessLabel, m_LineThicknessCtrl,
                      m_LineThicknessUnits, true ),
    m_OrientValidator( 1, &m_OrientValue )
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

    if( m_item->Type() == PCB_MODULE_TEXT_T )
    {
        title = _( "Footprint Text Properties" );

        m_modText = (TEXTE_MODULE*) m_item;
        m_edaText = static_cast<EDA_TEXT*>( m_modText );

        switch( m_modText->GetType() )
        {
        case TEXTE_MODULE::TEXT_is_REFERENCE: m_TextLabel->SetLabel( _( "Reference:" ) ); break;
        case TEXTE_MODULE::TEXT_is_VALUE:     m_TextLabel->SetLabel( _( "Value:" ) );     break;
        case TEXTE_MODULE::TEXT_is_DIVERS:    m_TextLabel->SetLabel( _( "Text:" ) );      break;
        }

        SetInitialFocus( m_SingleLineText );
        m_MultiLineSizer->Show( false );
    }
    else
    {
        title = _( "Text Properties" );

        m_pcbText = (TEXTE_PCB*) aItem;
        m_edaText = static_cast<EDA_TEXT*>( m_pcbText );

        SetInitialFocus( m_MultiLineText );
        m_SingleLineSizer->Show( false );

        // This option make sense only for footprint texts,
        // Texts on board are always visible:
        m_Visible->SetValue( true );
        m_Visible->Show( false );

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
    m_LayerSelectionCtrl->SetNotAllowedLayerSet( LSET::ForbiddenTextLayers() );
    m_LayerSelectionCtrl->SetBoardFrame( m_Parent );
    m_LayerSelectionCtrl->Resync();

    m_OrientValue = 0.0;
    m_OrientValidator.SetRange( -360.0, 360.0 );
    m_OrientCtrl->SetValidator( m_OrientValidator );
    m_OrientValidator.SetWindow( m_OrientCtrl );

    // Handle decimal separators in combo dropdown
    for( size_t i = 0; i < m_OrientCtrl->GetCount(); ++i )
    {
        wxString item = m_OrientCtrl->GetString( i );
        item.Replace( '.', localeconv()->decimal_point[0] );
        m_OrientCtrl->SetString( i, item );
    }

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
            m_LineThicknessCtrl,
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

    FinishDialogSettings();
}


DIALOG_TEXT_PROPERTIES::~DIALOG_TEXT_PROPERTIES()
{
    Disconnect( wxEVT_CHAR_HOOK, wxKeyEventHandler( DIALOG_TEXT_PROPERTIES::OnCharHook ), NULL, this );
}


/**
 * Routine for main window class to launch text properties dialog.
 */
void PCB_BASE_EDIT_FRAME::InstallTextOptionsFrame( BOARD_ITEM* aText )
{
    DIALOG_TEXT_PROPERTIES dlg( this, aText );
    dlg.ShowModal();
}


void DIALOG_TEXT_PROPERTIES::OnCharHook( wxKeyEvent& aEvent )
{
    if( aEvent.GetKeyCode() == WXK_RETURN && aEvent.ShiftDown() )
    {
        if( TransferDataFromWindow() )
            EndModal( wxID_OK );
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


wxString DIALOG_TEXT_PROPERTIES::convertKIIDsToReferences( const wxString& aSource )
{
    wxString newbuf;
    size_t   sourceLen = aSource.length();

    for( size_t i = 0; i < sourceLen; ++i )
    {
        if( aSource[i] == '$' && i + 1 < sourceLen && aSource[i+1] == '{' )
        {
            wxString token;
            bool     isCrossRef = false;

            for( i = i + 2; i < sourceLen; ++i )
            {
                if( aSource[i] == '}' )
                    break;

                if( aSource[i] == ':' )
                    isCrossRef = true;

                token.append( aSource[i] );
            }

            if( isCrossRef )
            {
                wxString      remainder;
                wxString      ref = token.BeforeFirst( ':', &remainder );
                BOARD_ITEM*   refItem = m_Parent->GetBoard()->GetItem( KIID( ref ) );

                if( refItem && refItem->Type() == PCB_MODULE_T )
                    token = static_cast<MODULE*>( refItem )->GetReference() + ":" + remainder;
            }

            newbuf.append( "${" + token + "}" );
        }
        else
        {
            newbuf.append( aSource[i] );
        }
    }

    return newbuf;
}


wxString DIALOG_TEXT_PROPERTIES::convertReferencesToKIIDs( const wxString& aSource )
{
    wxString newbuf;
    size_t   sourceLen = aSource.length();

    for( size_t i = 0; i < sourceLen; ++i )
    {
        if( aSource[i] == '$' && i + 1 < sourceLen && aSource[i+1] == '{' )
        {
            wxString token;
            bool     isCrossRef = false;

            for( i = i + 2; i < sourceLen; ++i )
            {
                if( aSource[i] == '}' )
                    break;

                if( aSource[i] == ':' )
                    isCrossRef = true;

                token.append( aSource[i] );
            }

            if( isCrossRef )
            {
                wxString remainder;
                wxString ref = token.BeforeFirst( ':', &remainder );

                for( MODULE* mod : m_Parent->GetBoard()->Modules() )
                {
                    if( mod->GetReference().CmpNoCase( ref ) == 0 )
                    {
                        wxString test( remainder );

                        if( mod->ResolveTextVar( &test ) )
                            token = mod->m_Uuid.AsString() + ":" + remainder;

                        break;
                    }
                }
            }

            newbuf.append( "${" + token + "}" );
        }
        else
        {
            newbuf.append( aSource[i] );
        }
    }

    return newbuf;
}


bool DIALOG_TEXT_PROPERTIES::TransferDataToWindow()
{
    if( m_SingleLineText->IsShown() )
    {
        m_SingleLineText->SetValue( m_edaText->GetText() );

        if( m_modText && m_modText->GetType() == TEXTE_MODULE::TEXT_is_REFERENCE )
            SelectReferenceNumber( static_cast<wxTextEntry*>( m_SingleLineText ) );
        else
            m_SingleLineText->SetSelection( -1, -1 );
    }
    else if( m_MultiLineText->IsShown() )
    {
        m_MultiLineText->SetValue( convertKIIDsToReferences( m_edaText->GetText() ) );
        m_MultiLineText->SetSelection( -1, -1 );
    }

    if( m_item->Type() == PCB_MODULE_TEXT_T && m_modText )
    {
        MODULE*  module = dynamic_cast<MODULE*>( m_modText->GetParent() );
        wxString msg;

        if( module )
        {
            msg.Printf( _("Footprint %s (%s), %s, rotated %.1f deg"),
                        module->GetReference(),
                        module->GetValue(),
                        module->IsFlipped() ? _( "back side (mirrored)" ) : _( "front side" ),
                        module->GetOrientation() / 10.0 );
        }

        m_statusLine->SetLabel( msg );
    }
    else
    {
        m_statusLine->Show( false );
    }

    if( m_LayerSelectionCtrl->SetLayerSelection( m_item->GetLayer() ) < 0 )
    {
        wxMessageBox( _( "This item was on a non-existing or forbidden layer.\n"
                         "It has been moved to the first allowed layer." ) );
        m_LayerSelectionCtrl->SetSelection( 0 );
    }

    m_textWidth.SetValue( m_edaText->GetTextSize().x );
    m_textHeight.SetValue( m_edaText->GetTextSize().y );
    m_thickness.SetValue( m_edaText->GetTextThickness() );
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

    if( !m_textWidth.Validate( TEXTS_MIN_SIZE, TEXTS_MAX_SIZE )
        || !m_textHeight.Validate( TEXTS_MIN_SIZE, TEXTS_MAX_SIZE ) )
        return false;

    BOARD_COMMIT commit( m_Parent );
    commit.Modify( m_item );

    // If no other command in progress, prepare undo command
    // (for a command in progress, will be made later, at the completion of command)
    bool pushCommit = ( m_item->GetEditFlags() == 0 );

    /* set flag in edit to force undo/redo/abort proper operation,
     * and avoid new calls to SaveCopyInUndoList for the same text
     * this can occurs when a text is moved, and then rotated, edited ..
    */
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
            wxString txt = convertReferencesToKIIDs( m_MultiLineText->GetValue() );

            // On Windows, a new line is coded as \r\n.
            // We use only \n in kicad files and in drawing routines.
            // so strip the \r char
#ifdef __WINDOWS__
            txt.Replace( "\r", "" );
#endif
            m_edaText->SetText( txt );
        }
    }

    m_item->SetLayer( ToLAYER_ID( m_LayerSelectionCtrl->GetLayerSelection() ) );

    m_edaText->SetTextSize( wxSize( m_textWidth.GetValue(), m_textHeight.GetValue() ) );
    m_edaText->SetTextThickness( m_thickness.GetValue() );
    m_edaText->SetTextPos( wxPoint( m_posX.GetValue(), m_posY.GetValue() ) );

    if( m_modText )
        m_modText->SetLocalCoord();

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

    m_Parent->Refresh();

    if( pushCommit )
        commit.Push( _( "Change text properties" ) );

    return true;
}
