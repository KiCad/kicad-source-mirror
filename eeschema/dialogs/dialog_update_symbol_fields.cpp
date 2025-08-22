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


bool g_removeExtraLibFields      = false;
bool g_resetEmptyLibFields       = false;
bool g_resetLibFieldText         = true;
bool g_resetLibFieldVisibilities = true;
bool g_resetLibFieldEffects      = true;
bool g_resetLibFieldPositions    = true;


DIALOG_UPDATE_SYMBOL_FIELDS::DIALOG_UPDATE_SYMBOL_FIELDS( SYMBOL_EDIT_FRAME* aParent,
                                                          LIB_SYMBOL* aSymbol ) :
        DIALOG_UPDATE_SYMBOL_FIELDS_BASE( aParent ),
        m_editFrame( aParent ),
        m_symbol( aSymbol)
{
    wxASSERT( aParent );
    wxASSERT( aSymbol );

    m_parentSymbolReadOnly->SetValue( UnescapeString( m_symbol->GetParent().lock()->GetName() ) );

    for( int i = 0; i < MANDATORY_FIELD_COUNT; ++i )
    {
        m_fieldsBox->Append( GetDefaultFieldName( i, DO_TRANSLATE ) );
        m_fieldsBox->Check( i, true );
    }

    updateFieldsList();

    m_removeExtraBox->SetValue( g_removeExtraLibFields );
    m_resetEmptyFields->SetValue( g_resetEmptyLibFields );
    m_resetFieldText->SetValue( g_resetLibFieldText );
    m_resetFieldVisibilities->SetValue( g_resetLibFieldVisibilities );
    m_resetFieldEffects->SetValue( g_resetLibFieldEffects );
    m_resetFieldPositions->SetValue( g_resetLibFieldPositions );

    SetupStandardButtons();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


DIALOG_UPDATE_SYMBOL_FIELDS::~DIALOG_UPDATE_SYMBOL_FIELDS()
{
    g_removeExtraLibFields = m_removeExtraBox->GetValue();
    g_resetEmptyLibFields = m_resetEmptyFields->GetValue();
    g_resetLibFieldText = m_resetFieldText->GetValue();
    g_resetLibFieldVisibilities = m_resetFieldVisibilities->GetValue();
    g_resetLibFieldEffects = m_resetFieldEffects->GetValue();
    g_resetLibFieldPositions = m_resetFieldPositions->GetValue();
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

    libFields.clear();

    // Update the listbox widget
    for( unsigned i = m_fieldsBox->GetCount() - 1; i >= MANDATORY_FIELD_COUNT; --i )
        m_fieldsBox->Delete( i );

    for( const wxString& fieldName : fieldNames )
        m_fieldsBox->Append( fieldName );

    for( unsigned i = MANDATORY_FIELD_COUNT; i < m_fieldsBox->GetCount(); ++i )
        m_fieldsBox->Check( i, true );
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
            parentField = flattenedParent->FindField( field.GetName() );

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
    int                     idx = result.size();

    flattenedParent->GetFields( parentFields );

    for( SCH_FIELD* parentField : parentFields )
    {
        if( !alg::contains( m_updateFields, parentField->GetName() ) )
            continue;

        if( !m_symbol->FindField( parentField->GetName() ) )
        {
            result.emplace_back( m_symbol, idx++ );
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


