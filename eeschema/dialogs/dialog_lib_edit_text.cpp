/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2001 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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
#include <sch_draw_panel.h>
#include <lib_edit_frame.h>
#include <class_libentry.h>
#include <lib_text.h>

#include <dialog_lib_edit_text.h>


DIALOG_LIB_EDIT_TEXT::DIALOG_LIB_EDIT_TEXT( LIB_EDIT_FRAME* aParent, LIB_TEXT* aText ) :
    DIALOG_LIB_EDIT_TEXT_BASE( aParent ),
    m_posX( aParent, m_xPosLabel, m_xPosCtrl, m_xPosUnits, true ),
    m_posY( aParent, m_yPosLabel, m_yPosCtrl, m_yPosUnits, true ),
    m_textSize( aParent, m_textSizeLabel, m_textSizeCtrl, m_textSizeUnits, true )
{
    m_parent = aParent;
    m_graphicText = aText;

    // Disable options for fieldedit, not existing in  graphic text
    m_visible->Show( false );
    m_TextValueSelectButton->Hide();
    m_PowerComponentValues->Show( false );

    wxFont infoFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    infoFont.SetSymbolicSize( wxFONTSIZE_SMALL );
    m_PowerComponentValues->SetFont( infoFont );

    SetInitialFocus( m_TextValue );

    m_sdbSizerButtonsOK->SetDefault();

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}


bool DIALOG_LIB_EDIT_TEXT::TransferDataToWindow()
{
    if( m_graphicText )
    {
        m_posX.SetValue( m_graphicText->GetPosition().x );
        m_posY.SetValue( m_graphicText->GetPosition().y );
        m_textSize.SetValue( m_graphicText->GetTextWidth() );
        m_TextValue->SetValue( m_graphicText->GetText() );

        m_italic->SetValue( m_graphicText->IsItalic() );
        m_bold->SetValue( m_graphicText->IsBold() );
        m_CommonUnit->SetValue( m_graphicText->GetUnit() == 0 );
        m_CommonConvert->SetValue( m_graphicText->GetConvert() == 0 );
        m_orientChoice->SetSelection( m_graphicText->GetTextAngle() == TEXT_ANGLE_HORIZ ? 0 : 1 );

        switch ( m_graphicText->GetHorizJustify() )
        {
        case GR_TEXT_HJUSTIFY_LEFT:   m_hAlignChoice->SetSelection( 0 ); break;
        case GR_TEXT_HJUSTIFY_CENTER: m_hAlignChoice->SetSelection( 1 ); break;
        case GR_TEXT_HJUSTIFY_RIGHT:  m_hAlignChoice->SetSelection( 2 ); break;
        }

        switch ( m_graphicText->GetVertJustify() )
        {
        case GR_TEXT_VJUSTIFY_TOP:    m_vAlignChoice->SetSelection( 0 ); break;
        case GR_TEXT_VJUSTIFY_CENTER: m_vAlignChoice->SetSelection( 1 ); break;
        case GR_TEXT_VJUSTIFY_BOTTOM: m_vAlignChoice->SetSelection( 2 ); break;
        }
    }
    else
    {
        m_textSize.SetValue( m_parent->g_LastTextSize );

        m_CommonUnit->SetValue( !m_parent->m_DrawSpecificUnit );
        m_CommonConvert->SetValue( !m_parent->m_DrawSpecificConvert );
        m_orientChoice->SetSelection( m_graphicText->GetTextAngle() == TEXT_ANGLE_HORIZ ? 0 : 1 );
    }

    return true;
}


bool DIALOG_LIB_EDIT_TEXT::TransferDataFromWindow()
{
    m_parent->g_LastTextAngle = m_orientChoice->GetSelection() ? TEXT_ANGLE_VERT
                                                                    : TEXT_ANGLE_HORIZ;
    m_parent->g_LastTextSize = m_textSize.GetValue();
    m_parent->m_DrawSpecificConvert = !m_CommonConvert->GetValue();
    m_parent->m_DrawSpecificUnit = !m_CommonUnit->GetValue();

    if( m_graphicText )
    {
        if( m_TextValue->GetValue().IsEmpty() )
            m_graphicText->SetText( wxT( "[null]" ) );
        else
            m_graphicText->SetText( m_TextValue->GetValue() );

        m_graphicText->SetPosition( wxPoint( m_posX.GetValue(), m_posY.GetValue() ) );
        m_graphicText->SetTextSize( wxSize( m_parent->g_LastTextSize, m_parent->g_LastTextSize ) );
        m_graphicText->SetTextAngle( m_parent->g_LastTextAngle );

        if( m_parent->m_DrawSpecificUnit )
            m_graphicText->SetUnit( m_parent->GetUnit() );
        else
            m_graphicText->SetUnit( 0 );

        if( m_parent->m_DrawSpecificConvert )
            m_graphicText->SetConvert( m_parent->GetConvert() );
        else
            m_graphicText->SetConvert( 0 );

        m_graphicText->SetItalic( m_italic->GetValue() );
        m_graphicText->SetBold( m_bold->GetValue() );

        switch( m_hAlignChoice->GetSelection() )
        {
        case 0: m_graphicText->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );   break;
        case 1: m_graphicText->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER ); break;
        case 2: m_graphicText->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );  break;
        }

        switch( m_vAlignChoice->GetSelection() )
        {
        case 0: m_graphicText->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );    break;
        case 1: m_graphicText->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER ); break;
        case 2: m_graphicText->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM ); break;
        }
    }

    m_parent->SetMsgPanel( m_graphicText );

    return true;
}
