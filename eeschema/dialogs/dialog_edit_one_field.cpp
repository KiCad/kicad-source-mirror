/**
 * @file dialog_edit_one_field.cpp
 * @brief dialog to editing a field ( not a graphic text) in current component.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2016 Wayne Stambaugh, stambaughw@gmail.com
 * Copyright (C) 2004-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <confirm.h>

#include <general.h>
#include <sch_base_frame.h>
#include <sch_component.h>
#include <class_libentry.h>
#include <lib_field.h>
#include <sch_component.h>
#include <template_fieldnames.h>
#include <class_library.h>
#include <sch_validators.h>

#include <dialog_edit_one_field.h>


// These should probably moved into some other file as helpers.
EDA_TEXT_HJUSTIFY_T IntToEdaTextHorizJustify( int aHorizJustify )
{
    wxASSERT( aHorizJustify >= GR_TEXT_HJUSTIFY_LEFT && aHorizJustify <= GR_TEXT_HJUSTIFY_RIGHT );

    if( aHorizJustify > GR_TEXT_HJUSTIFY_RIGHT )
        return GR_TEXT_HJUSTIFY_RIGHT;

    if( aHorizJustify < GR_TEXT_HJUSTIFY_LEFT )
        return GR_TEXT_HJUSTIFY_LEFT;

    return (EDA_TEXT_HJUSTIFY_T) aHorizJustify;
}


EDA_TEXT_VJUSTIFY_T IntToEdaTextVertJustify( int aVertJustify )
{
    wxASSERT( aVertJustify >= GR_TEXT_VJUSTIFY_TOP && aVertJustify <= GR_TEXT_VJUSTIFY_BOTTOM );

    if( aVertJustify > GR_TEXT_VJUSTIFY_BOTTOM )
        return GR_TEXT_VJUSTIFY_BOTTOM;

    if( aVertJustify < GR_TEXT_VJUSTIFY_TOP )
        return GR_TEXT_VJUSTIFY_TOP;

    return (EDA_TEXT_VJUSTIFY_T) aVertJustify;
}


DIALOG_EDIT_ONE_FIELD::DIALOG_EDIT_ONE_FIELD( SCH_BASE_FRAME* aParent, const wxString& aTitle,
                                              const EDA_TEXT* aTextItem ) :
    DIALOG_LIB_EDIT_TEXT_BASE( aParent )
{
    SetTitle( aTitle );

    // The field ID and power status are Initialized in the derived object's ctor.
    m_fieldId = VALUE;
    m_isPower = false;

    m_text = aTextItem->GetText();
    m_style = aTextItem->IsItalic() ? 1 : 0;
    m_style += aTextItem->IsBold() ? 2 : 0;
    m_size = aTextItem->GetTextWidth();
    m_orientation = ( aTextItem->GetTextAngle() == TEXT_ANGLE_VERT );
    m_verticalJustification = aTextItem->GetVertJustify() + 1;
    m_horizontalJustification = aTextItem->GetHorizJustify() + 1;
    m_isVisible = aTextItem->IsVisible();
}


void DIALOG_EDIT_ONE_FIELD::init()
{
    wxString msg;

    m_TextValue->SetFocus();
    SCH_BASE_FRAME* parent = static_cast<SCH_BASE_FRAME*>( GetParent() );
    m_TextValue->SetValidator( SCH_FIELD_VALIDATOR(
                                    parent->IsType( FRAME_SCH_LIB_EDITOR ),
                                    m_fieldId, &m_text ) );

    // Disable options for graphic text editing which are not needed for fields.
    m_CommonConvert->Show( false );
    m_CommonUnit->Show( false );

    // Show the footprint selection dialog if this is the footprint field.
    if( m_fieldId == FOOTPRINT )
    {
        m_TextValueSelectButton->Show();
        m_TextValueSelectButton->Enable();
    }
    else
    {
        m_TextValueSelectButton->Hide();
        m_TextValueSelectButton->Disable();
    }

    msg = m_TextSizeText->GetLabel() + ReturnUnitSymbol();
    m_TextSizeText->SetLabel( msg );


    // Value fields of power components cannot be modified. This will grey out
    // the text box and display an explanation.
    if( m_fieldId == VALUE && m_isPower )
    {
        m_PowerComponentValues->Show( true );
        m_TextValue->Enable( false );
    }
    else
    {
        m_PowerComponentValues->Show( false );
        m_TextValue->Enable( true );
    }

    m_sdbSizerButtonsOK->SetDefault();

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}


void DIALOG_EDIT_ONE_FIELD::OnTextValueSelectButtonClick( wxCommandEvent& aEvent )
{
    // pick a footprint using the footprint picker.
    wxString fpid;

    KIWAY_PLAYER* frame = Kiway().Player( FRAME_PCB_MODULE_VIEWER_MODAL, true );

    if( frame->ShowModal( &fpid, this ) )
    {
        m_TextValue->SetValue( fpid );
    }

    frame->Destroy();
}


bool DIALOG_EDIT_ONE_FIELD::TransferDataToWindow()
{
    wxLogDebug( "In DIALOG_EDIT_ONE_FIELD::TransferDataToWindow()" );

    m_TextValue->SetValue( m_text );

    if( m_fieldId == REFERENCE )
    {
        if( m_text.find_first_of( '?' ) != m_text.npos )
        {
            m_TextValue->SetSelection( m_text.find_first_of( '?' ),
                                       m_text.find_last_of( '?' ) + 1 );
        }
        else
        {
            wxString num = m_text;

            while( !num.IsEmpty() && ( !isdigit( num.Last() ) ||
                                       !isdigit( num.GetChar( 0 ) ) ) )
            {
                if( !isdigit( num.Last() ) )
                    num.RemoveLast();
                if( !isdigit( num.GetChar ( 0 ) ) )
                    num = num.Right( num.Length() - 1);
            }

            m_TextValue->SetSelection( m_text.Find( num ),
                                       m_text.Find( num ) + num.Length() );

            if( num.IsEmpty() )
                m_TextValue->SetSelection( -1, -1 );
        }
    }
    else
    {
        m_TextValue->SetSelection( -1, -1 );
    }

    m_Orient->SetValue( m_orientation );
    m_TextSize->SetValue( StringFromValue( g_UserUnit, m_size ) );
    m_TextHJustificationOpt->SetSelection( m_horizontalJustification );
    m_TextVJustificationOpt->SetSelection( m_verticalJustification );
    m_Invisible->SetValue( !m_isVisible );
    m_TextShapeOpt->SetSelection( m_style );

    return true;
}


bool DIALOG_EDIT_ONE_FIELD::TransferDataFromWindow()
{
    wxLogDebug( "In DIALOG_EDIT_ONE_FIELD::TransferDataFromWindow()" );

    m_text = m_TextValue->GetValue();

    // There are lots of specific tests required to validate field text.
    if( m_fieldId == REFERENCE )
    {
        // Test if the reference string is valid:
        if( !SCH_COMPONENT::IsReferenceStringValid( m_text ) )
        {
            DisplayError( this, _( "Illegal reference field value!" ) );
            return false;
        }
    }

    m_orientation = m_Orient->GetValue();
    m_size = ValueFromString( g_UserUnit, m_TextSize->GetValue() );
    m_horizontalJustification = m_TextHJustificationOpt->GetSelection();
    m_verticalJustification = m_TextVJustificationOpt->GetSelection();
    m_isVisible = !m_Invisible->GetValue();
    m_style = m_TextShapeOpt->GetSelection();

    return true;
}


void DIALOG_EDIT_ONE_FIELD::updateText( EDA_TEXT* aText )
{
    aText->SetTextSize( wxSize( m_size, m_size ) );
    aText->SetVisible( m_isVisible );
    aText->SetTextAngle( m_orientation ? TEXT_ANGLE_VERT : TEXT_ANGLE_HORIZ );
    aText->SetItalic( (m_style & 1) != 0 );
    aText->SetBold( (m_style & 2) != 0 );
    aText->SetHorizJustify( IntToEdaTextHorizJustify( m_horizontalJustification - 1 ) );
    aText->SetVertJustify( IntToEdaTextVertJustify( m_verticalJustification - 1 ) );
}


DIALOG_LIB_EDIT_ONE_FIELD::DIALOG_LIB_EDIT_ONE_FIELD( SCH_BASE_FRAME* aParent,
                                                      const wxString& aTitle,
                                                      const LIB_FIELD* aField ) :
    DIALOG_EDIT_ONE_FIELD( aParent, aTitle, dynamic_cast< const EDA_TEXT* >( aField ) )
{
    m_fieldId = aField->GetId();

    // When in the library editor, power components can be renamed.
    m_isPower = false;
    init();
}


DIALOG_SCH_EDIT_ONE_FIELD::DIALOG_SCH_EDIT_ONE_FIELD( SCH_BASE_FRAME* aParent,
                                                      const wxString& aTitle,
                                                      const SCH_FIELD* aField ) :
    DIALOG_EDIT_ONE_FIELD( aParent, aTitle, dynamic_cast< const EDA_TEXT* >( aField ) )
{
    m_fieldId = aField->GetId();

    const SCH_COMPONENT* component = (SCH_COMPONENT*) aField->GetParent();

    wxASSERT_MSG( component != NULL && component->Type() == SCH_COMPONENT_T,
                  wxT( "Invalid schematic field parent item." ) );

    const LIB_PART* part = GetParent()->Prj().SchLibs()->FindLibPart( component->GetLibId() );

    wxASSERT_MSG( part, wxT( "Library part for component <" ) +
                  FROM_UTF8( component->GetLibId().Format() ) +
                  wxT( "> could not be found." ) );

    m_isPower = part->IsPower();

    init();
}


void DIALOG_SCH_EDIT_ONE_FIELD::UpdateField( SCH_FIELD* aField, SCH_SHEET_PATH* aSheetPath )
{
    wxASSERT( aField != NULL || aField->Type() != SCH_FIELD_T );

    if( aField->GetId() == REFERENCE )
    {
        wxASSERT( aSheetPath != NULL );

        SCH_COMPONENT* component = dynamic_cast< SCH_COMPONENT* >( aField->GetParent() );

        wxASSERT( component != NULL );

        if( component != NULL )
            component->SetRef( aSheetPath, m_text );
    }

    aField->SetText( m_text );
    updateText( aField );
}
