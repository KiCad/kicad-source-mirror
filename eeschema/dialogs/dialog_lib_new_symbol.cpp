/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2016-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <default_values.h>
#include <dialog_lib_new_symbol.h>
#include <eda_draw_frame.h>
#include <sch_validators.h>
#include <template_fieldnames.h>

DIALOG_LIB_NEW_SYMBOL::DIALOG_LIB_NEW_SYMBOL( EDA_DRAW_FRAME* aParent,
                                              const wxArrayString* aRootSymbolNames ) :
    DIALOG_LIB_NEW_SYMBOL_BASE( dynamic_cast<wxWindow*>( aParent ) ),
    m_pinTextPosition( aParent, m_staticPinTextPositionLabel, m_textPinTextPosition,
                       m_staticPinTextPositionUnits, true )
{
    if( aRootSymbolNames && aRootSymbolNames->GetCount() )
    {
        wxArrayString escapedNames;

        for( const wxString& name : *aRootSymbolNames )
            escapedNames.Add( UnescapeString( name ) );

        m_comboInheritanceSelect->Append( escapedNames );
    }

    m_textName->SetValidator( SCH_FIELD_VALIDATOR( true, VALUE_FIELD ) );
    m_textReference->SetValidator( SCH_FIELD_VALIDATOR( true, REFERENCE_FIELD ) );

    m_pinTextPosition.SetValue( Mils2iu( DEFAULT_PIN_NAME_OFFSET ) );

    // initial focus should be on first editable field.
    m_textName->SetFocus();

    // What happens when user presses "Enter"? OK button!  OK?
    m_sdbSizerOK->SetDefault();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


void DIALOG_LIB_NEW_SYMBOL::OnParentSymbolSelect( wxCommandEvent& aEvent )
{
    syncControls( !m_comboInheritanceSelect->GetValue().IsEmpty() );
}


void DIALOG_LIB_NEW_SYMBOL::syncControls( bool aIsDerivedPart )
{
    m_staticTextDes->Enable( !aIsDerivedPart );
    m_textReference->Enable( !aIsDerivedPart );
    m_staticTextUnits->Enable( !aIsDerivedPart );
    m_spinPartCount->Enable( !aIsDerivedPart );
    m_checkLockItems->Enable( !aIsDerivedPart );
    m_checkHasConversion->Enable( !aIsDerivedPart );
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

