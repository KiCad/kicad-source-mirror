/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2001 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2004-2023 KiCad Developers, see change_log.txt for contributors.
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
#include <widgets/color_swatch.h>
#include <symbol_edit_frame.h>
#include <lib_text.h>
#include <settings/settings_manager.h>
#include <dialog_lib_text_properties.h>
#include <symbol_editor/symbol_editor_settings.h>
#include <tools/symbol_editor_drawing_tools.h>
#include <scintilla_tricks.h>
#include "confirm.h"

DIALOG_LIB_TEXT_PROPERTIES::DIALOG_LIB_TEXT_PROPERTIES( SYMBOL_EDIT_FRAME* aParent,
                                                        LIB_TEXT* aText ) :
        DIALOG_LIB_TEXT_PROPERTIES_BASE( aParent ),
        m_parent( aParent ),
        m_graphicText( aText ),
        m_textSize( aParent, m_textSizeLabel, m_textSizeCtrl, m_textSizeUnits, true )
{
    COLOR_SETTINGS* colorSettings = m_parent->GetColorSettings();
    COLOR4D         schematicBackground = colorSettings->GetColor( LAYER_SCHEMATIC_BACKGROUND );

    m_scintillaTricks = new SCINTILLA_TRICKS( m_StyledTextCtrl, wxT( "{}" ), false,
            [this]()
            {
                wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
            } );

    SetInitialFocus( m_StyledTextCtrl );

    m_separator1->SetIsSeparator();

    m_horizontal->SetIsRadioButton();
    m_horizontal->SetBitmap( KiBitmap( BITMAPS::text_horizontal ) );
    m_vertical->SetIsRadioButton();
    m_vertical->SetBitmap( KiBitmap( BITMAPS::text_vertical ) );

    m_separator2->SetIsSeparator();

    m_bold->SetIsCheckButton();
    m_bold->SetBitmap( KiBitmap( BITMAPS::text_bold ) );
    m_italic->SetIsCheckButton();
    m_italic->SetBitmap( KiBitmap( BITMAPS::text_italic ) );

    m_separator3->SetIsSeparator();

    m_hAlignLeft->SetIsRadioButton();
    m_hAlignLeft->SetBitmap( KiBitmap( BITMAPS::text_align_left ) );
    m_hAlignCenter->SetIsRadioButton();
    m_hAlignCenter->SetBitmap( KiBitmap( BITMAPS::text_align_center ) );
    m_hAlignRight->SetIsRadioButton();
    m_hAlignRight->SetBitmap( KiBitmap( BITMAPS::text_align_right ) );

    m_separator4->SetIsSeparator();

    m_vAlignTop->SetIsRadioButton();
    m_vAlignTop->SetBitmap( KiBitmap( BITMAPS::text_valign_top ) );
    m_vAlignCenter->SetIsRadioButton();
    m_vAlignCenter->SetBitmap( KiBitmap( BITMAPS::text_valign_center ) );
    m_vAlignBottom->SetIsRadioButton();
    m_vAlignBottom->SetBitmap( KiBitmap( BITMAPS::text_valign_bottom ) );

    m_separator5->SetIsSeparator();

    m_textColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );
    m_textColorSwatch->SetSwatchBackground( schematicBackground );

    m_horizontal->Bind( wxEVT_BUTTON, &DIALOG_LIB_TEXT_PROPERTIES::onOrientButton, this );
    m_vertical->Bind( wxEVT_BUTTON, &DIALOG_LIB_TEXT_PROPERTIES::onOrientButton, this );

    m_hAlignLeft->Bind( wxEVT_BUTTON, &DIALOG_LIB_TEXT_PROPERTIES::onHAlignButton, this );
    m_hAlignCenter->Bind( wxEVT_BUTTON, &DIALOG_LIB_TEXT_PROPERTIES::onHAlignButton, this );
    m_hAlignRight->Bind( wxEVT_BUTTON, &DIALOG_LIB_TEXT_PROPERTIES::onHAlignButton, this );

    m_vAlignTop->Bind( wxEVT_BUTTON, &DIALOG_LIB_TEXT_PROPERTIES::onVAlignButton, this );
    m_vAlignCenter->Bind( wxEVT_BUTTON, &DIALOG_LIB_TEXT_PROPERTIES::onVAlignButton, this );
    m_vAlignBottom->Bind( wxEVT_BUTTON, &DIALOG_LIB_TEXT_PROPERTIES::onVAlignButton, this );

    SetupStandardButtons();

    if( !aParent->IsSymbolEditable() || aParent->IsSymbolAlias() )
    {
        m_sdbSizerButtonsCancel->SetDefault();
        m_sdbSizerButtonsOK->SetLabel( _( "Read Only" ) );
        m_sdbSizerButtonsOK->Enable( false );
    }

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


DIALOG_LIB_TEXT_PROPERTIES::~DIALOG_LIB_TEXT_PROPERTIES()
{
    delete m_scintillaTricks;
};


bool DIALOG_LIB_TEXT_PROPERTIES::TransferDataToWindow()
{
    wxCHECK( m_CommonUnit, false );

    LIB_SYMBOL* symbol = nullptr;

    if( m_graphicText )
    {
        symbol = m_graphicText->GetParent();

        wxCHECK( symbol, false );

        m_textSize.SetValue( m_graphicText->GetTextWidth() );
        m_StyledTextCtrl->SetValue( m_graphicText->GetText() );
        m_StyledTextCtrl->EmptyUndoBuffer();

        m_fontCtrl->SetFontSelection( m_graphicText->GetFont() );
        m_textColorSwatch->SetSwatchColor( m_graphicText->GetTextColor(), false );

        m_italic->Check( m_graphicText->IsItalic() );
        m_bold->Check( m_graphicText->IsBold() );

        m_privateCheckbox->SetValue( m_graphicText->IsPrivate() );
        m_CommonUnit->SetValue(
                symbol && symbol->GetUnitCount() > 1 && m_graphicText->GetUnit() == 0 );
        m_CommonConvert->SetValue( m_graphicText->GetConvert() == 0 );

        if( m_graphicText->GetTextAngle().IsHorizontal() )
            m_horizontal->Check();
        else
            m_vertical->Check();

        switch ( m_graphicText->GetHorizJustify() )
        {
        case GR_TEXT_H_ALIGN_LEFT:   m_hAlignLeft->Check( true );   break;
        case GR_TEXT_H_ALIGN_CENTER: m_hAlignCenter->Check( true ); break;
        case GR_TEXT_H_ALIGN_RIGHT:  m_hAlignRight->Check( true );  break;
        }

        switch ( m_graphicText->GetVertJustify() )
        {
        case GR_TEXT_V_ALIGN_TOP:    m_vAlignTop->Check( true );    break;
        case GR_TEXT_V_ALIGN_CENTER: m_vAlignCenter->Check( true ); break;
        case GR_TEXT_V_ALIGN_BOTTOM: m_vAlignBottom->Check( true ); break;
        }
    }
    else
    {
        SYMBOL_EDITOR_SETTINGS* cfg = m_parent->GetSettings();
        auto* tools = m_parent->GetToolManager()->GetTool<SYMBOL_EDITOR_DRAWING_TOOLS>();
        symbol = m_parent->GetCurSymbol();

        wxCHECK( cfg && symbol && tools, false );

        m_textSize.SetValue( schIUScale.MilsToIU( cfg->m_Defaults.text_size ) );

        m_CommonUnit->SetValue( symbol->GetUnitCount() > 1 && !tools->GetDrawSpecificUnit() );
        m_CommonConvert->SetValue( !tools->GetDrawSpecificConvert() );

        if( tools->GetLastTextAngle().IsHorizontal() )
            m_horizontal->Check();
        else
            m_vertical->Check();
    }

    m_CommonUnit->Enable( symbol && symbol->GetUnitCount() > 1 );

    return true;
}


void DIALOG_LIB_TEXT_PROPERTIES::onOrientButton( wxCommandEvent& aEvent )
{
    for( BITMAP_BUTTON* btn : { m_horizontal, m_vertical } )
    {
        if( btn->IsChecked() && btn != aEvent.GetEventObject() )
            btn->Check( false );
    }
}


void DIALOG_LIB_TEXT_PROPERTIES::onHAlignButton( wxCommandEvent& aEvent )
{
    for( BITMAP_BUTTON* btn : { m_hAlignLeft, m_hAlignCenter, m_hAlignRight } )
    {
        if( btn->IsChecked() && btn != aEvent.GetEventObject() )
            btn->Check( false );
    }
}


void DIALOG_LIB_TEXT_PROPERTIES::onVAlignButton( wxCommandEvent& aEvent )
{
    for( BITMAP_BUTTON* btn : { m_vAlignTop, m_vAlignCenter, m_vAlignBottom } )
    {
        if( btn->IsChecked() && btn != aEvent.GetEventObject() )
            btn->Check( false );
    }
}


bool DIALOG_LIB_TEXT_PROPERTIES::TransferDataFromWindow()
{
    if( m_graphicText )
    {
        if( m_StyledTextCtrl->GetValue().IsEmpty() )
        {
            // Other text items do not have defined extents, and so will disappear if empty
            DisplayError( this, _( "Text can not be empty." ) );
            return false;
        }
        else
        {
            m_graphicText->SetText( m_StyledTextCtrl->GetValue() );
        }

        if( m_fontCtrl->HaveFontSelection() )
        {
            m_graphicText->SetFont( m_fontCtrl->GetFontSelection( m_bold->IsChecked(),
                                                                  m_italic->IsChecked() ) );
        }

        if( m_textSize.GetValue() != m_graphicText->GetTextWidth() )
            m_graphicText->SetTextSize( VECTOR2I( m_textSize.GetValue(), m_textSize.GetValue() ) );

        if( m_horizontal->IsChecked() )
            m_graphicText->SetTextAngle( ANGLE_HORIZONTAL );
        else
            m_graphicText->SetTextAngle( ANGLE_VERTICAL );

        m_graphicText->SetPrivate( m_privateCheckbox->GetValue() );

        if( !m_CommonUnit->GetValue() )
            m_graphicText->SetUnit( m_parent->GetUnit() );
        else
            m_graphicText->SetUnit( 0 );

        if( !m_CommonConvert->GetValue() )
            m_graphicText->SetConvert( m_parent->GetConvert() );
        else
            m_graphicText->SetConvert( 0 );

        m_graphicText->SetItalic( m_italic->IsChecked() );
        m_graphicText->SetBold( m_bold->IsChecked() );
        m_graphicText->SetTextColor( m_textColorSwatch->GetSwatchColor() );

        if( m_hAlignLeft->IsChecked() )
            m_graphicText->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        else if( m_hAlignCenter->IsChecked() )
            m_graphicText->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        else
            m_graphicText->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );

        if( m_vAlignTop->IsChecked() )
            m_graphicText->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
        else if( m_vAlignCenter->IsChecked() )
            m_graphicText->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
        else
            m_graphicText->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );

        // Record settings used for next time:
        auto* tools = m_parent->GetToolManager()->GetTool<SYMBOL_EDITOR_DRAWING_TOOLS>();
        tools->SetLastTextAngle( m_graphicText->GetTextAngle() );
        tools->SetDrawSpecificConvert( !m_CommonConvert->GetValue() );
        tools->SetDrawSpecificUnit( !m_CommonUnit->GetValue() );
    }

    m_parent->SetMsgPanel( m_graphicText );

    return true;
}


void DIALOG_LIB_TEXT_PROPERTIES::onMultiLineTCLostFocus( wxFocusEvent& event )
{
    if( m_scintillaTricks )
        m_scintillaTricks->CancelAutocomplete();

    event.Skip();
}
