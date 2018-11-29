/**
 * @file dialog_edit_one_field.cpp
 * @brief dialog to editing a field ( not a graphic text) in current component.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2016 Wayne Stambaugh, stambaughw@gmail.com
 * Copyright (C) 2004-2018 KiCad Developers, see change_log.txt for contributors.
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
#include <kiway.h>
#include <confirm.h>
#include <kicad_string.h>
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
    DIALOG_LIB_EDIT_TEXT_BASE( aParent ),
    m_posX( aParent, m_xPosLabel, m_xPosCtrl, m_xPosUnits, true ),
    m_posY( aParent, m_yPosLabel, m_yPosCtrl, m_yPosUnits, true ),
    m_textSize( aParent, m_textSizeLabel, m_textSizeCtrl, m_textSizeUnits, true )
{
    SetTitle( aTitle );

    // The field ID and power status are Initialized in the derived object's ctor.
    m_fieldId = VALUE;
    m_isPower = false;

    m_text = aTextItem->GetText();
    m_isItalic = aTextItem->IsItalic();
    m_isBold = aTextItem->IsBold();
    m_position = aTextItem->GetTextPos();
    m_size = aTextItem->GetTextWidth();
    m_isVertical = ( aTextItem->GetTextAngle() == TEXT_ANGLE_VERT );
    m_verticalJustification = aTextItem->GetVertJustify() + 1;
    m_horizontalJustification = aTextItem->GetHorizJustify() + 1;
    m_isVisible = aTextItem->IsVisible();
}


void DIALOG_EDIT_ONE_FIELD::init()
{
    SetInitialFocus( m_TextValue );
    SCH_BASE_FRAME* parent = GetParent();
    bool libedit = parent->IsType( FRAME_SCH_LIB_EDITOR );
    m_TextValue->SetValidator( SCH_FIELD_VALIDATOR( libedit, m_fieldId, &m_text ) );

    // Disable options for graphic text editing which are not needed for fields.
    m_CommonConvert->Show( false );
    m_CommonUnit->Show( false );

    // Show the footprint selection dialog if this is the footprint field.
    m_TextValueSelectButton->Show( m_fieldId == FOOTPRINT );

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
    wxString fpid = m_TextValue->GetValue();

    KIWAY_PLAYER* frame = Kiway().Player( FRAME_PCB_MODULE_VIEWER_MODAL, true );

    if( frame->ShowModal( &fpid, this ) )
    {
        m_TextValue->SetValue( fpid );
    }

    frame->Destroy();
}


void DIALOG_EDIT_ONE_FIELD::OnSetFocusText( wxFocusEvent& event )
{
    if( m_fieldId == REFERENCE )
        SelectReferenceNumber( static_cast<wxTextEntry*>( m_TextValue ) );
    else
        m_TextValue->SetSelection( -1, -1 );

    event.Skip();
}


bool DIALOG_EDIT_ONE_FIELD::TransferDataToWindow()
{
    m_TextValue->SetValue( UnescapeString( m_text ) );

    m_posX.SetValue( m_position.x );
    m_posY.SetValue( m_position.y );
    m_textSize.SetValue( m_size );
    m_orientChoice->SetSelection( m_isVertical ? 1 : 0 );
    m_hAlignChoice->SetSelection( m_horizontalJustification );
    m_vAlignChoice->SetSelection( m_verticalJustification );
    m_visible->SetValue( m_isVisible );
    m_italic->SetValue( m_isItalic );
    m_bold->SetValue( m_isBold );

    return true;
}


bool DIALOG_EDIT_ONE_FIELD::TransferDataFromWindow()
{
    m_text = EscapeString( m_TextValue->GetValue() );

    if( m_fieldId == REFERENCE )
    {
        // Test if the reference string is valid:
        if( !SCH_COMPONENT::IsReferenceStringValid( m_text ) )
        {
            DisplayError( this, _( "Illegal reference field value!" ) );
            return false;
        }
    }
    else if( m_fieldId == VALUE )
    {
        if( m_text.IsEmpty() )
        {
            DisplayError( this, _( "Value may not be empty." ) );
            return false;
        }
    }

    m_isVertical = m_orientChoice->GetSelection() == 1;
    m_position = wxPoint( m_posX.GetValue(), m_posY.GetValue() );
    m_size = m_textSize.GetValue();
    m_horizontalJustification = m_hAlignChoice->GetSelection();
    m_verticalJustification = m_vAlignChoice->GetSelection();
    m_isVisible = m_visible->GetValue();
    m_isItalic = m_italic->GetValue();
    m_isBold = m_bold->GetValue();

    return true;
}


void DIALOG_EDIT_ONE_FIELD::updateText( EDA_TEXT* aText )
{
    aText->SetTextPos( m_position );
    aText->SetTextSize( wxSize( m_size, m_size ) );
    aText->SetVisible( m_isVisible );
    aText->SetTextAngle( m_isVertical ? TEXT_ANGLE_VERT : TEXT_ANGLE_HORIZ );
    aText->SetItalic( m_isItalic );
    aText->SetBold( m_isBold );
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

    wxASSERT_MSG( component && component->Type() == SCH_COMPONENT_T,
                  wxT( "Invalid schematic field parent item." ) );

    // The library symbol may have been removed so using SCH_COMPONENT::GetPartRef() here
    // could result in a segfault.  If the library symbol is no longer available, the
    // schematic fields can still edit so set the power symbol flag to false.  This may not
    // be entirely accurate if the power library is missing but it's better then a segfault.
    const LIB_PART* part = GetParent()->GetLibPart( component->GetLibId(), true );

    m_isPower = ( part ) ? part->IsPower() : false;

    init();
}


void DIALOG_SCH_EDIT_ONE_FIELD::UpdateField( SCH_FIELD* aField, SCH_SHEET_PATH* aSheetPath )
{
    if( aField->GetId() == REFERENCE )
    {
        wxASSERT( aSheetPath  );

        SCH_COMPONENT* component = dynamic_cast< SCH_COMPONENT* >( aField->GetParent() );

        wxASSERT( component  );

        if( component )
            component->SetRef( aSheetPath, m_text );
    }

    bool positioningModified = false;

    if( aField->GetTextPos() != m_position )
        positioningModified = true;

    if( ( aField->GetTextAngle() == TEXT_ANGLE_VERT ) != m_isVertical )
        positioningModified = true;

    if( aField->GetHorizJustify() != IntToEdaTextHorizJustify( m_horizontalJustification - 1 ) )
        positioningModified = true;

    if( aField->GetVertJustify() != IntToEdaTextVertJustify( m_verticalJustification - 1 ) )
        positioningModified = true;

    aField->SetText( m_text );
    updateText( aField );

    if( positioningModified )
    {
        auto component = static_cast< SCH_COMPONENT* >( aField->GetParent() );
        component->ClearFieldsAutoplaced();
    }
}
