/**
 * @file dialog_edit_one_field.cpp
 * @brief dialog to editing a field ( not a graphic text) in current component.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2004-2012 KiCad Developers, see change_log.txt for contributors.
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
#include <common.h>
#include <base_units.h>
#include <kiway.h>

#include <general.h>
#include <sch_base_frame.h>
#include <sch_component.h>
#include <template_fieldnames.h>
#include <class_libentry.h>
#include <lib_field.h>
#include <sch_component.h>
#include <template_fieldnames.h>

#include <dialog_edit_one_field.h>


void DIALOG_EDIT_ONE_FIELD::initDlg_base()
{
    wxString msg;

    m_TextValue->SetFocus();

    // Disable options for graphic text edition, not existing in fields
    m_CommonConvert->Show( false );
    m_CommonUnit->Show( false );

    msg = StringFromValue( g_UserUnit, m_textsize );
    m_TextSize->SetValue( msg );

    if( m_textorient == TEXT_ORIENT_VERT )
        m_Orient->SetValue( true );

    m_Invisible->SetValue( m_text_invisible );
    m_TextShapeOpt->SetSelection( m_textshape );
    SetPowerWarning( false );

    switch ( m_textHjustify )
    {
    case GR_TEXT_HJUSTIFY_LEFT:
        m_TextHJustificationOpt->SetSelection( 0 );
        break;

    case GR_TEXT_HJUSTIFY_CENTER:
        m_TextHJustificationOpt->SetSelection( 1 );
        break;

    case GR_TEXT_HJUSTIFY_RIGHT:
        m_TextHJustificationOpt->SetSelection( 2 );
        break;
    }

    switch ( m_textVjustify )
    {
    case GR_TEXT_VJUSTIFY_BOTTOM:
        m_TextVJustificationOpt->SetSelection( 0 );
        break;

    case GR_TEXT_VJUSTIFY_CENTER:
        m_TextVJustificationOpt->SetSelection( 1 );
        break;

    case GR_TEXT_VJUSTIFY_TOP:
        m_TextVJustificationOpt->SetSelection( 2 );
        break;
    }

    msg = m_TextSizeText->GetLabel() + ReturnUnitSymbol();
    m_TextSizeText->SetLabel( msg );

    m_sdbSizerButtonsOK->SetDefault();

}

void DIALOG_EDIT_ONE_FIELD::SetPowerWarning( bool aWarn )
{
    m_PowerComponentValues->Show( aWarn );
    m_TextValue->Enable( ! aWarn );
    Fit();
}

void DIALOG_EDIT_ONE_FIELD::OnTextValueSelectButtonClick( wxCommandEvent& aEvent )
{
    // pick a footprint using the footprint picker.
    wxString fpid;

    KIWAY_PLAYER* frame = Kiway().Player( FRAME_PCB_MODULE_VIEWER_MODAL, true );

    if( frame->ShowModal( &fpid, this ) )
    {
        // DBG( printf( "%s: %s\n", __func__, TO_UTF8( fpid ) ); )
        m_TextValue->SetValue( fpid );
    }

    frame->Destroy();
}


void DIALOG_LIB_EDIT_ONE_FIELD::initDlg()
{
    m_textsize = m_field->GetSize().x;
    m_TextValue->SetValue( m_field->GetText() );
    m_textorient = m_field->GetOrientation();
    m_text_invisible = m_field->IsVisible() ? false : true;

    m_textshape = 0;

    if( m_field->IsItalic() )
        m_textshape = 1;

    if( m_field->IsBold() )
        m_textshape |= 2;

    m_textHjustify = m_field->GetHorizJustify();
    m_textVjustify = m_field->GetVertJustify();

    if( m_field->GetId() == FOOTPRINT )
    {
        m_TextValueSelectButton->Show();
        m_TextValueSelectButton->Enable();
    }
    else
    {
        m_TextValueSelectButton->Hide();
        m_TextValueSelectButton->Disable();
    }

    initDlg_base();
}


wxString DIALOG_LIB_EDIT_ONE_FIELD::GetTextField()
{
    wxString line = m_TextValue->GetValue();
    // Spaces are not allowed in fields, so replace them by '_'
    line.Replace( wxT( " " ), wxT( "_" ) );
    return line;
}


void DIALOG_EDIT_ONE_FIELD::TransfertDataToField( bool aIncludeText )
{
    // This method doesn't transfer text anyway.
    (void) aIncludeText;

    m_textorient = m_Orient->GetValue() ? TEXT_ORIENT_VERT : TEXT_ORIENT_HORIZ;
    wxString msg = m_TextSize->GetValue();
    m_textsize = ValueFromString( g_UserUnit, msg );

    switch( m_TextHJustificationOpt->GetSelection() )
    {
    case 0:
        m_textHjustify = GR_TEXT_HJUSTIFY_LEFT;
        break;

    case 1:
        m_textHjustify = GR_TEXT_HJUSTIFY_CENTER;
        break;

    case 2:
        m_textHjustify = GR_TEXT_HJUSTIFY_RIGHT;
        break;
    }

    switch( m_TextVJustificationOpt->GetSelection() )
    {
    case 0:
        m_textVjustify = GR_TEXT_VJUSTIFY_BOTTOM;
        break;

    case 1:
        m_textVjustify = GR_TEXT_VJUSTIFY_CENTER;
        break;

    case 2:
        m_textVjustify = GR_TEXT_VJUSTIFY_TOP;
        break;
    }
}


void DIALOG_LIB_EDIT_ONE_FIELD::TransfertDataToField( bool aIncludeText )
{
    DIALOG_EDIT_ONE_FIELD::TransfertDataToField( aIncludeText );

    if( aIncludeText )
        m_field->SetText( GetTextField() );

    m_field->SetSize( wxSize( m_textsize, m_textsize ) );
    m_field->SetOrientation( m_textorient );
    m_field->SetVisible( !m_Invisible->GetValue() );
    m_field->SetItalic( ( m_TextShapeOpt->GetSelection() & 1 ) != 0 );
    m_field->SetBold( ( m_TextShapeOpt->GetSelection() & 2 ) != 0 );
    m_field->SetHorizJustify( m_textHjustify );
    m_field->SetVertJustify( m_textVjustify );
}


void DIALOG_SCH_EDIT_ONE_FIELD::initDlg()
{
    m_textsize = m_field->GetSize().x;
    m_TextValue->SetValue( m_field->GetText() );
    m_textorient = m_field->GetOrientation();
    m_text_invisible = m_field->IsVisible() ? false : true;

    m_textshape = 0;

    if( m_field->IsItalic() )
        m_textshape = 1;

    if( m_field->IsBold() )
        m_textshape |= 2;

    m_textHjustify = m_field->GetHorizJustify();
    m_textVjustify = m_field->GetVertJustify();

    if( m_field->GetId() == FOOTPRINT )
    {
        m_TextValueSelectButton->Show();
        m_TextValueSelectButton->Enable();
    }
    else
    {
        m_TextValueSelectButton->Hide();
        m_TextValueSelectButton->Disable();
    }

    initDlg_base();
}


wxString DIALOG_SCH_EDIT_ONE_FIELD::GetTextField()
{
    wxString line = m_TextValue->GetValue();
    line.Trim( true );
    line.Trim( false );
    return line;
};


void DIALOG_SCH_EDIT_ONE_FIELD::TransfertDataToField( bool aIncludeText )
{
    DIALOG_EDIT_ONE_FIELD::TransfertDataToField( aIncludeText );

    if( aIncludeText )
        m_field->SetText( GetTextField() );

    m_field->SetSize( wxSize( m_textsize, m_textsize ) );
    m_field->SetOrientation( m_textorient );
    m_field->SetVisible( !m_Invisible->GetValue() );

    m_field->SetItalic( ( m_TextShapeOpt->GetSelection() & 1 ) != 0 );
    m_field->SetBold( ( m_TextShapeOpt->GetSelection() & 2 ) != 0 );
    m_field->SetHorizJustify( m_textHjustify );
    m_field->SetVertJustify( m_textVjustify );
}
