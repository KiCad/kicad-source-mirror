/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "dialog_update_fields.h"

#include <sch_edit_frame.h>
#include <sch_component.h>
#include <class_libentry.h>
#include <algorithm>


int InvokeDialogUpdateFields( SCH_EDIT_FRAME* aCaller,
        const list<SCH_COMPONENT*> aComponents, bool aCreateUndoEntry )
{
    DIALOG_UPDATE_FIELDS dlg( aCaller, aComponents, aCreateUndoEntry );
    return dlg.ShowQuasiModal();
}


DIALOG_UPDATE_FIELDS::DIALOG_UPDATE_FIELDS( SCH_EDIT_FRAME* aParent,
        const list<SCH_COMPONENT*>& aComponents, bool aCreateUndoEntry )
    : DIALOG_UPDATE_FIELDS_BASE( aParent ), m_frame( aParent ),
    m_components( aComponents ), m_createUndo( aCreateUndoEntry )
{
    m_sdbSizerOK->SetDefault();
}


bool DIALOG_UPDATE_FIELDS::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    if( m_components.empty() )
        return true;        // nothing to process


    // Create the set of fields to be updated
    m_updateFields.clear();

    for( unsigned i = 0; i < m_fieldsBox->GetCount(); ++i )
    {
        if( m_fieldsBox->IsChecked( i ) )
            m_updateFields.insert( m_fieldsBox->GetString( i ) );
    }


    // Undo buffer entry
    if( m_createUndo )
    {
        PICKED_ITEMS_LIST itemsList;

        for( auto component : m_components )
            itemsList.PushItem( ITEM_PICKER( component, UR_CHANGED ) );

        m_frame->SaveCopyInUndoList( itemsList, UR_CHANGED );
    }


    // Do it!
    for( auto component : m_components )
        updateFields( component );

    m_frame->SyncView();
    m_frame->GetCanvas()->Refresh();
    m_frame->OnModify();

    return true;
}


bool DIALOG_UPDATE_FIELDS::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() || !m_components.size() )
        return false;

    // Collect all user field names from library parts of components that are going to be updated
    {
        for( auto component : m_components )
        {
            const std::unique_ptr< LIB_PART >&  part = component->GetPartRef();

            if( !part )
                continue;

            const auto& drawItems = part->GetDrawItems();

            for( auto it = drawItems.begin( LIB_FIELD_T ); it != drawItems.end( LIB_FIELD_T ); ++it )
            {
                const LIB_FIELD* field = static_cast<const LIB_FIELD*>( &( *it ) );

                if( field->GetId() >= MANDATORY_FIELDS )
                    m_updateFields.insert( field->GetName() );
            }
        }
    }

    // Update the listbox widget
    m_fieldsBox->Clear();

    for( int i = 0; i < MANDATORY_FIELDS; ++i )
    {
        m_fieldsBox->Append( m_components.front()->GetField( i )->GetName() );

        if( i != REFERENCE && i != VALUE )
            m_fieldsBox->Check( i, true );
    }

    for( const wxString& fieldName : m_updateFields )
    {
        int idx = m_fieldsBox->Append( fieldName );
        m_fieldsBox->Check( idx, true );
    }

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();

    return true;
}


void DIALOG_UPDATE_FIELDS::updateFields( SCH_COMPONENT* aComponent )
{
    SCH_FIELDS newFields;

    std::unique_ptr< LIB_PART >& libPart = aComponent->GetPartRef();

    if( !libPart )    // the symbol is not found in lib: cannot update fields
        return;

    LIB_PART* alias = m_frame->GetLibPart( aComponent->GetLibId() );

    for( const SCH_FIELD& existingField : aComponent->GetFields() )
    {
        if( existingField.GetId() >= 0 && existingField.GetId() < MANDATORY_FIELDS )
        {
            newFields.push_back( existingField );
            continue;
        }

        // If requested, transfer only fields that occur also in the original library part
        if( m_removeExtraBox->IsChecked() && !libPart->FindField( existingField.GetName() ) )
            continue;

        newFields.push_back( existingField );
    }

    // Update the requested fields
    for( const wxString& fieldName : m_updateFields )
    {
        LIB_FIELD* libField = libPart->FindField( fieldName );

        if( !libField )
            continue;

        auto it = std::find_if( newFields.begin(), newFields.end(),
                                [&] ( const SCH_FIELD& f )
                                {
                                    return f.GetName() == fieldName;
                                } );

        if( it != newFields.end() )
        {
            SCH_FIELD* newField = &*it;
            wxString   fieldValue = libField->GetText();

            if( alias )
            {
                if( fieldName == TEMPLATE_FIELDNAME::GetDefaultFieldName( VALUE ) )
                    fieldValue = alias->GetName();
                else if( fieldName == TEMPLATE_FIELDNAME::GetDefaultFieldName( DATASHEET ) )
                    fieldValue = alias->GetDatasheetField().GetText();
            }

            if( fieldValue.IsEmpty() )
            {
                // If the library field is empty an update would clear an existing entry.
                // Check if this is the desired behavior.
                if( m_resetEmpty->IsChecked() )
                    newField->SetText( wxEmptyString );
            }
            else
            {
                newField->SetText( fieldValue );
            }

            if( m_resetVisibility->IsChecked() )
            {
                newField->SetVisible( libField->IsVisible() );
            }

            if( m_resetPosition->IsChecked() )
            {
                newField->SetTextAngle( libField->GetTextAngle() );

                // Schematic fields are schematic-relative; symbol editor fields are component-relative
                if( m_createUndo )
                    newField->SetTextPos( libField->GetTextPos() + aComponent->GetPosition() );
                else
                    newField->SetTextPos( libField->GetTextPos() );
            }

            if( m_resetSizeAndStyle->IsChecked() )
            {
                newField->SetHorizJustify( libField->GetHorizJustify() );
                newField->SetVertJustify( libField->GetVertJustify() );
                newField->SetTextSize( libField->GetTextSize() );
                newField->SetItalic( libField->IsItalic() );
                newField->SetBold( libField->IsBold() );
            }
        }
        else
        {
            // Missing field, it has to be added to the component
            SCH_FIELD newField( wxPoint( 0, 0 ), newFields.size(), aComponent, fieldName );

            newField.ImportValues( *libField );
            newField.SetText( libField->GetText() );

            // Schematic fields are schematic-relative; symbol editor fields are component-relative
            if( m_createUndo )
                newField.SetTextPos( libField->GetTextPos() + aComponent->GetPosition() );
            else
                newField.SetTextPos( libField->GetTextPos() );

            newFields.push_back( newField );
        }
    }

    // Apply changes & clean-up
    aComponent->SetFields( newFields );
}


void DIALOG_UPDATE_FIELDS::checkAll( bool aCheck )
{
    for( unsigned i = 0; i < m_fieldsBox->GetCount(); ++i )
        m_fieldsBox->Check( i, aCheck );
}
