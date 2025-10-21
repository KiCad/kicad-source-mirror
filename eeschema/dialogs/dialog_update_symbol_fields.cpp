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

#include <core/kicad_algo.h>
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

    // Create the set of fields to be updated
    m_updateFields.clear();

    for( unsigned i = 0; i < m_fieldsBox->GetCount(); ++i )
    {
        if( m_fieldsBox->IsChecked( i ) )
            m_updateFields.insert( m_fieldsBox->GetString( i ) );
    }

    std::unique_ptr<LIB_SYMBOL> flattenedParent = m_symbol->GetParent().lock()->Flatten();

    bool removeExtras = m_removeExtraBox->GetValue();
    bool resetVis = m_resetFieldVisibilities->GetValue();
    bool resetEffects = m_resetFieldEffects->GetValue();
    bool resetPositions = m_resetFieldPositions->GetValue();

    std::vector<SCH_FIELD> fields;
    std::vector<SCH_FIELD> result;
    m_symbol->CopyFields( fields );

    for( SCH_FIELD& field : fields )
    {
        bool       copy = true;
        SCH_FIELD* parentField = nullptr;

        if( alg::contains( m_updateFields, field.GetName() ) )
        {
            if( field.IsMandatory() )
                parentField = flattenedParent->GetField( field.GetId() );
            else
                parentField = flattenedParent->GetField( field.GetName() );

            if( parentField )
            {
                bool resetText = parentField->GetText().IsEmpty() ? m_resetEmptyFields->GetValue()
                                                                  : m_resetFieldText->GetValue();

                if( resetText )
                    field.SetText( parentField->GetText() );

                if( resetVis )
                {
                    field.SetVisible( parentField->IsVisible() );
                    field.SetNameShown( parentField->IsNameShown() );
                }

                if( resetEffects )
                {
                    // Careful: the visible bit and position are also set by SetAttributes()
                    bool    visible = field.IsVisible();
                    VECTOR2I pos = field.GetPosition();

                    field.SetAttributes( *parentField );

                    field.SetVisible( visible );
                    field.SetPosition( pos );
                }

                if( resetPositions )
                    field.SetTextPos( parentField->GetTextPos() );
            }
            else if( removeExtras )
            {
                copy = false;
            }
        }

        if( copy )
            result.emplace_back( std::move( field ) );
    }

    std::vector<SCH_FIELD*> parentFields;

    flattenedParent->GetFields( parentFields );

    for( SCH_FIELD* parentField : parentFields )
    {
        if( !alg::contains( m_updateFields, parentField->GetName() ) )
            continue;

        if( !m_symbol->GetField( parentField->GetName() ) )
        {
            result.emplace_back( m_symbol, FIELD_T::USER );
            SCH_FIELD* newField = &result.back();

            newField->SetName( parentField->GetCanonicalName() );
            newField->SetText( parentField->GetText() );
            newField->SetAttributes( *parentField );   // Includes visible bit and position
        }
    }

    m_symbol->SetFields( result );

    commit.Push( _( "Update Symbol Fields" ) );
    m_editFrame->RebuildView();

    wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
}


