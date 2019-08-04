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
#include <class_dimension.h>
#include <class_module.h>
#include <class_pcb_text.h>
#include <class_text_mod.h>
#include <pcb_edit_frame.h>
#include <pcb_layer_box_selector.h>
#include <pcbnew.h>
#include <wx/valnum.h>


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
    m_OrientValidator( 1, &m_OrientValue )
{
    wxString title;

    if( m_item->Type() == PCB_DIMENSION_T )
    {
        title = _( "Dimension Text Properties" );

        DIMENSION* dimension = (DIMENSION*) m_item;
        m_edaText = &dimension->Text();
        m_pcbText = &dimension->Text();

        SetInitialFocus( m_DimensionText );
        m_SingleLineSizer->Show( false );
        m_MultiLineSizer->Show( false );

        m_KeepUpright->Show( false );
        m_statusLine->Show( false );
    }
    else if( m_item->Type() == PCB_MODULE_TEXT_T )
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
        m_DimensionTextSizer->Show( false );
    }
    else
    {
        title = _( "Text Properties" );

        m_pcbText = (TEXTE_PCB*) aItem;
        m_edaText = static_cast<EDA_TEXT*>( m_pcbText );

        SetInitialFocus( m_MultiLineText );
        m_SingleLineSizer->Show( false );
        m_DimensionTextSizer->Show( false );

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
    infoFont.SetSymbolicSize( wxFONTSIZE_SMALL );
    m_statusLine->SetFont( infoFont );

    m_sdbSizerOK->SetDefault();

    // Tab down the left side first
    KIUI::SetControlsTabOrder( {
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
    } );

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
    if( aEvent.GetKeyCode() == WXK_TAB )
    {
        int flags = 0;

        if( !aEvent.ShiftDown() )
            flags |= wxNavigationKeyEvent::IsForward;

        if( aEvent.ControlDown() )
            flags |= wxNavigationKeyEvent::WinChange;

        NavigateIn( flags );
    }
    else if( aEvent.GetKeyCode() == WXK_RETURN )
    {
        // Allow typing returns into a multi-line text
        if( m_MultiLineText->IsShown() && m_MultiLineText->HasFocus() && !aEvent.ShiftDown() )
            aEvent.Skip();
        else
        {
            if( TransferDataFromWindow() )
                EndModal( wxID_OK );
        }
    }
    else
    {
        aEvent.Skip();
    }
}


void DIALOG_TEXT_PROPERTIES::OnDimensionTextChange( wxCommandEvent& event )
{
    EDA_UNITS_T units = UNSCALED_UNITS;
    bool useMils;

    FetchUnitsFromString( m_DimensionText->GetValue(), units, useMils );

    if( units != UNSCALED_UNITS )
        m_DimensionUnitsOpt->SetSelection( units == MILLIMETRES ? 2 : useMils ? 1 : 0 );
}


void DIALOG_TEXT_PROPERTIES::OnDimensionUnitsChange( wxCommandEvent& event )
{
    DIMENSION* dimension = (DIMENSION*) m_item;
    EDA_UNITS_T units;
    bool useMils;

    // Get default units in case dimension text doesn't contain units.
    dimension->GetUnits( units, useMils );

    double value = ValueFromString( units, m_DimensionText->GetValue(), useMils );

    switch( event.GetSelection() )
    {
    case 0: units = INCHES;      useMils = false; break;
    case 1: units = INCHES;      useMils = true;  break;
    case 2: units = MILLIMETRES; useMils = false; break;
    default: break;
    }

    m_DimensionText->SetValue( StringFromValue( units, value, true, useMils ) );
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
        m_MultiLineText->SetValue( m_edaText->GetText() );
        m_MultiLineText->SetSelection( -1, -1 );
    }
    else if (m_DimensionText->IsShown() )
    {
        m_DimensionText->SetValue( m_edaText->GetText() );
        m_DimensionText->SetSelection( -1, -1 );

        DIMENSION* dimension = (DIMENSION*) m_item;
        EDA_UNITS_T units;
        bool useMils;
        dimension->GetUnits( units, useMils );

        m_DimensionUnitsOpt->SetSelection( units == MILLIMETRES ? 2 : useMils ? 1 : 0 );
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
            m_edaText->SetText( m_MultiLineText->GetValue() );
    }
    else if( m_DimensionText->IsShown() )
    {
        if( !m_DimensionText->GetValue().IsEmpty() )
            m_edaText->SetText( m_DimensionText->GetValue() );

        DIMENSION* dimension = (DIMENSION*) m_item;

        switch( m_DimensionUnitsOpt->GetSelection() )
        {
        case 0: dimension->SetUnits( INCHES, false );      break;
        case 1: dimension->SetUnits( INCHES, true );       break;
        case 2: dimension->SetUnits( MILLIMETRES, false ); break;
        default: break;
        }
    }

    m_item->SetLayer( ToLAYER_ID( m_LayerSelectionCtrl->GetLayerSelection() ) );

    m_edaText->SetTextSize( wxSize( m_textWidth.GetValue(), m_textHeight.GetValue() ) );
    m_edaText->SetThickness( m_thickness.GetValue() );
    m_edaText->SetTextPos( wxPoint( m_posX.GetValue(), m_posY.GetValue() ) );

    if( m_modText )
        m_modText->SetLocalCoord();

    // Test for acceptable values for thickness and size and clamp if fails
    int maxthickness = Clamp_Text_PenSize( m_edaText->GetThickness(), m_edaText->GetTextSize() );

    if( m_edaText->GetThickness() > maxthickness )
    {
        DisplayError( this, _( "The text thickness is too large for the text size.\n"
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

    m_Parent->Refresh();

    if( pushCommit )
        commit.Push( _( "Change text properties" ) );

    return true;
}
