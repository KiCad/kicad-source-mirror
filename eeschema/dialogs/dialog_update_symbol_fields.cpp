/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <dialog_update_symbol_fields.h>
#include <lib_symbol.h>
#include <symbol_edit_frame.h>
#include <sch_commit.h>
#include <template_fieldnames.h>
#include <string_utils.h>


DIALOG_UPDATE_SYMBOL_FIELDS::DIALOG_UPDATE_SYMBOL_FIELDS( SYMBOL_EDIT_FRAME* aParent, LIB_SYMBOL* aSymbol ) :
        DIALOG_UPDATE_SYMBOL_FIELDS_BASE( aParent ),
        m_editFrame( aParent ),
        m_symbol( aSymbol)
{
    wxASSERT( aParent );
    wxASSERT( aSymbol );

    if( std::shared_ptr<LIB_SYMBOL> parent = m_symbol->GetParent().lock() )
        m_parentSymbolReadOnly->SetValue( UnescapeString( parent->GetName() ) );

    for( FIELD_T fieldId : MANDATORY_FIELDS )
    {
        m_mandatoryFieldListIndexes[fieldId] = m_fieldsBox->GetCount();
        m_fieldsBox->Append( GetDefaultFieldName( fieldId, DO_TRANSLATE ) );
        m_fieldsBox->Check( m_fieldsBox->GetCount() - 1, true );
    }

    updateFieldsList();

    SetupStandardButtons();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


void DIALOG_UPDATE_SYMBOL_FIELDS::updateFieldsList()
{
    // Load non-mandatory fields from the parent part
    std::vector<SCH_FIELD*>      libFields;
    std::set<wxString>           fieldNames;
    std::unique_ptr<LIB_SYMBOL>  flattenedParent = m_symbol->GetParent().lock()->Flatten();

    flattenedParent->GetFields( libFields );

    for( SCH_FIELD* libField : libFields )
    {
        if( !libField->IsMandatory() )
            fieldNames.insert( libField->GetName() );
    }

    libFields.clear();  // flattenedPart is about to go out of scope...

    // Load non-mandatory fields from the editor symbol
    m_symbol->GetFields( libFields );

    for( SCH_FIELD* libField : libFields )
    {
        if( !libField->IsMandatory() )
            fieldNames.insert( libField->GetName() );
    }

    auto isMandatoryField =
            [&]( int listbox_idx )
            {
                for( FIELD_T fieldId : MANDATORY_FIELDS )
                {
                    if( m_mandatoryFieldListIndexes[fieldId] == listbox_idx )
                        return true;
                }

                return false;
            };

    libFields.clear();

    // Update the listbox widget
    for( int i = (int) m_fieldsBox->GetCount() - 1; i >= 0; --i )
    {
        if( !isMandatoryField( i ) )
            m_fieldsBox->Delete( i );
    }

    for( const wxString& fieldName : fieldNames )
        m_fieldsBox->Append( fieldName );

    for( int i = 0; i < (int) m_fieldsBox->GetCount(); ++i )
    {
        if( !isMandatoryField( i ) )
            m_fieldsBox->Check( i, true );
    }
}


void DIALOG_UPDATE_SYMBOL_FIELDS::checkAll( bool aCheck )
{
    for( unsigned i = 0; i < m_fieldsBox->GetCount(); ++i )
        m_fieldsBox->Check( i, aCheck );
}


void DIALOG_UPDATE_SYMBOL_FIELDS::onOkButtonClicked( wxCommandEvent& aEvent )
{
    wxBusyCursor dummy;
    SCH_COMMIT   commit( m_editFrame );

    commit.Modify( m_symbol, m_editFrame->GetScreen() );

    LIB_FIELD_SYNC_OPTIONS options;

    // The dialog drives the field set explicitly; an empty selection updates nothing.
    options.m_updateAllFields = false;

    for( unsigned i = 0; i < m_fieldsBox->GetCount(); ++i )
    {
        if( m_fieldsBox->IsChecked( i ) )
            options.m_updateFields.insert( m_fieldsBox->GetString( i ) );
    }

    // Mandatory rows show translated names, but SyncFieldsFromParent() matches on canonical
    // names, so add those too or a non-English UI would never update mandatory fields.
    for( FIELD_T fieldId : MANDATORY_FIELDS )
    {
        if( m_fieldsBox->IsChecked( m_mandatoryFieldListIndexes[fieldId] ) )
            options.m_updateFields.insert( GetCanonicalFieldName( fieldId ) );
    }

    options.m_removeExtraFields = m_removeExtraBox->GetValue();
    options.m_resetVisibility   = m_resetFieldVisibilities->GetValue();
    options.m_resetEffects      = m_resetFieldEffects->GetValue();
    options.m_resetPositions    = m_resetFieldPositions->GetValue();
    options.m_resetText         = m_resetFieldText->GetValue();
    options.m_resetEmptyText    = m_resetEmptyFields->GetValue();

    m_symbol->SyncFieldsFromParent( options );

    commit.Push( _( "Update Symbol Fields" ) );
    m_editFrame->RebuildView();

    wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
}


