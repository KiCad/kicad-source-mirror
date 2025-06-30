/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "dialog_lib_new_symbol.h"

#include <default_values.h>
#include <eda_draw_frame.h>
#include <sch_validators.h>
#include <template_fieldnames.h>


static wxString getDerivativeName( const wxString& aParentName )
{
    return wxString::Format( "%s_1", aParentName );
}


DIALOG_LIB_NEW_SYMBOL::DIALOG_LIB_NEW_SYMBOL( EDA_DRAW_FRAME*      aParent,
                                              const wxArrayString& aSymbolNames,
                                              const wxString&      aInheritFromSymbolName,
                                              std::function<bool( wxString newName )> aValidator ) :
        DIALOG_LIB_NEW_SYMBOL_BASE( dynamic_cast<wxWindow*>( aParent ) ),
        m_pinTextPosition( aParent, m_staticPinTextPositionLabel, m_textPinTextPosition,
                           m_staticPinTextPositionUnits, true ),
        m_validator( std::move( aValidator ) ),
        m_nameIsDefaulted( true )
{
    if( aSymbolNames.GetCount() )
    {
        wxArrayString escapedNames;

        for( const wxString& name : aSymbolNames )
            escapedNames.Add( UnescapeString( name ) );

        m_comboInheritanceSelect->SetStringList( escapedNames );

        if( !aInheritFromSymbolName.IsEmpty() )
            m_comboInheritanceSelect->SetSelectedString( aInheritFromSymbolName );
    }

    // Trigger the event handler to show/hide the info bar message.
    wxCommandEvent dummyEvent;
    onParentSymbolSelect( dummyEvent );

    m_textName->SetValidator( FIELD_VALIDATOR( VALUE_FIELD ) );
    m_textReference->SetValidator( FIELD_VALIDATOR( REFERENCE_FIELD ) );

    if( !aInheritFromSymbolName.IsEmpty() )
    {
        m_textName->ChangeValue( getDerivativeName( aInheritFromSymbolName ) );
        m_nameIsDefaulted = true;
    }

    m_pinTextPosition.SetValue( schIUScale.MilsToIU( DEFAULT_PIN_NAME_OFFSET ) );

    m_comboInheritanceSelect->Connect(
            FILTERED_ITEM_SELECTED,
            wxCommandEventHandler( DIALOG_LIB_NEW_SYMBOL::onParentSymbolSelect ), nullptr, this );

    m_textName->Bind( wxEVT_TEXT,
                      [this]( wxCommandEvent& aEvent )
                      {
                          m_nameIsDefaulted = false;
                      } );

    // initial focus should be on first editable field.
    m_textName->SetFocus();

    SetupStandardButtons();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


DIALOG_LIB_NEW_SYMBOL::~DIALOG_LIB_NEW_SYMBOL()
{
    m_comboInheritanceSelect->Disconnect(
            FILTERED_ITEM_SELECTED,
            wxCommandEventHandler( DIALOG_LIB_NEW_SYMBOL::onParentSymbolSelect ), nullptr, this );
}


bool DIALOG_LIB_NEW_SYMBOL::TransferDataFromWindow()
{
    return m_validator( GetName() );
}


void DIALOG_LIB_NEW_SYMBOL::onParentSymbolSelect( wxCommandEvent& aEvent )
{
    const wxString parent = m_comboInheritanceSelect->GetValue();

    if( !parent.IsEmpty() )
    {
        m_infoBar->RemoveAllButtons();
        m_infoBar->ShowMessage( wxString::Format( _( "Deriving from symbol '%s'." ), parent ),
                                wxICON_INFORMATION );
    }
    else
    {
        m_infoBar->Dismiss();
    }

    if( m_textName->IsEmpty() || m_nameIsDefaulted )
    {
        m_textName->SetValue( getDerivativeName( parent ) );
        m_textName->SetInsertionPointEnd();
        m_nameIsDefaulted = true;
    }

    syncControls( !parent.IsEmpty() );
}


void DIALOG_LIB_NEW_SYMBOL::syncControls( bool aIsDerivedPart )
{
    m_staticTextDes->Enable( !aIsDerivedPart );
    m_textReference->Enable( !aIsDerivedPart );
    m_staticTextUnits->Enable( !aIsDerivedPart );
    m_spinPartCount->Enable( !aIsDerivedPart );
    m_checkUnitsInterchangeable->Enable( !aIsDerivedPart );
    m_checkHasAlternateBodyStyle->Enable( !aIsDerivedPart );
    m_checkIsPowerSymbol->Enable( !aIsDerivedPart );
    m_excludeFromBomCheckBox->Enable( !aIsDerivedPart );
    m_excludeFromBoardCheckBox->Enable( !aIsDerivedPart );
    m_staticPinTextPositionLabel->Enable( !aIsDerivedPart );
    m_textPinTextPosition->Enable( !aIsDerivedPart );
    m_staticPinTextPositionUnits->Enable( !aIsDerivedPart );
    m_checkShowPinNumber->Enable( !aIsDerivedPart );
    m_checkShowPinName->Enable( !aIsDerivedPart );
    m_checkShowPinNameInside->Enable( !aIsDerivedPart );
}


void DIALOG_LIB_NEW_SYMBOL::onPowerCheckBox( wxCommandEvent& aEvent )
{
    if( m_checkIsPowerSymbol->IsChecked() )
    {
        m_excludeFromBomCheckBox->SetValue( true );
        m_excludeFromBoardCheckBox->SetValue( true );
        m_excludeFromBomCheckBox->Enable( false );
        m_excludeFromBoardCheckBox->Enable( false );
    }
    else
    {
        m_excludeFromBomCheckBox->Enable( true );
        m_excludeFromBoardCheckBox->Enable( true );
    }
}

