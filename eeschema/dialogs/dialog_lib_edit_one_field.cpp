/**
 * @file dialog_lib_edit_one_field.cpp
 * @brief dialog to editing  fields ( not graphic texts) in components.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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

#include <general.h>
#include <libeditframe.h>
#include <sch_component.h>
#include <template_fieldnames.h>
#include <class_libentry.h>
#include <lib_field.h>

#include <dialog_lib_edit_one_field.h>


DIALOG_LIB_EDIT_ONE_FIELD::DIALOG_LIB_EDIT_ONE_FIELD( LIB_EDIT_FRAME* aParent,
                                                      const wxString& aTitle,
                                                      LIB_FIELD* aField ) :
    DIALOG_LIB_EDIT_TEXT_BASE( aParent )
{
    m_parent = aParent;
    m_field = aField;
    SetTitle( aTitle );
    initDlg();

    GetSizer()->SetSizeHints(this);
    Centre();
}


void DIALOG_LIB_EDIT_ONE_FIELD::initDlg( )
{
    wxString msg;

    m_TextValue->SetFocus();

    // Disable options for graphic text edition, not existing in fields
    m_CommonConvert->Show(false);
    m_CommonUnit->Show(false);

    if ( m_field )
    {
        msg = ReturnStringFromValue( g_UserUnit, m_field->m_Size.x,
                                     m_parent->GetInternalUnits() );
        m_TextSize->SetValue( msg );
        m_TextValue->SetValue( m_field->m_Text );

        if( m_field->GetOrientation() == TEXT_ORIENT_VERT )
            m_Orient->SetValue( true );

        if( m_field->GetOrientation() == TEXT_ORIENT_VERT )
            m_Orient->SetValue( true );

        m_Invisible->SetValue( m_field->IsVisible() ? false : true);

        int shape = 0;
        if( m_field->m_Italic )
            shape = 1;
        if( m_field->m_Bold )
            shape |= 2;

        m_TextShapeOpt->SetSelection( shape );

        switch ( m_field->m_HJustify )
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

        switch ( m_field->m_VJustify )
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
    }

    msg = m_TextSizeText->GetLabel() + ReturnUnitSymbol();
    m_TextSizeText->SetLabel( msg );

    m_sdbSizerButtonsOK->SetDefault();
}


void DIALOG_LIB_EDIT_ONE_FIELD::OnCancelClick( wxCommandEvent& event )
{
    EndModal(wxID_CANCEL);
}


/* Updates the different parameters for the component being edited */
void DIALOG_LIB_EDIT_ONE_FIELD::OnOkClick( wxCommandEvent& event )
{
    EndModal(wxID_OK);
}

wxString DIALOG_LIB_EDIT_ONE_FIELD::GetTextField()
{
    wxString line = m_TextValue->GetValue();
    line.Replace( wxT( " " ), wxT( "_" ) );
    return line;
};

void DIALOG_LIB_EDIT_ONE_FIELD::TransfertDataToField()
{

    int orientation = m_Orient->GetValue() ? TEXT_ORIENT_VERT : TEXT_ORIENT_HORIZ;
    wxString msg = m_TextSize->GetValue();
    int textSize = ReturnValueFromString( g_UserUnit, msg, m_parent->GetInternalUnits() );

    if( m_field )
    {
        m_field->SetText( GetTextField() );

        m_field->m_Size.x = m_field->m_Size.y = textSize;
        m_field->m_Orient = orientation;

        if( m_Invisible->GetValue() )
            m_field->m_Attributs |= TEXT_NO_VISIBLE;
        else
            m_field->m_Attributs &= ~TEXT_NO_VISIBLE;

        if( ( m_TextShapeOpt->GetSelection() & 1 ) != 0 )
            m_field->m_Italic = true;
        else
            m_field->m_Italic = false;

        if( ( m_TextShapeOpt->GetSelection() & 2 ) != 0 )
            m_field->m_Bold = true;
        else
            m_field->m_Bold = false;

        switch( m_TextHJustificationOpt->GetSelection() )
        {
        case 0:
            m_field->m_HJustify = GR_TEXT_HJUSTIFY_LEFT;
            break;

        case 1:
            m_field->m_HJustify = GR_TEXT_HJUSTIFY_CENTER;
            break;

        case 2:
            m_field->m_HJustify = GR_TEXT_HJUSTIFY_RIGHT;
            break;
        }

        switch( m_TextVJustificationOpt->GetSelection() )
        {
        case 0:
            m_field->m_VJustify = GR_TEXT_VJUSTIFY_BOTTOM;
            break;

        case 1:
            m_field->m_VJustify = GR_TEXT_VJUSTIFY_CENTER;
            break;

        case 2:
            m_field->m_VJustify = GR_TEXT_VJUSTIFY_TOP;
            break;
        }
    }
}
