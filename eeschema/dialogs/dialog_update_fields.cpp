/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
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
}


bool DIALOG_UPDATE_FIELDS::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    if( m_components.empty() )
        return true;        // nothing to process


    // Create the set of fields to be updated
    m_fields.clear();

    for( unsigned i = 0; i < m_fieldsBox->GetCount(); ++i )
    {
        if( m_fieldsBox->IsChecked( i ) )
            m_fields.insert( m_fieldsBox->GetString( i ) );
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

    m_frame->OnModify();

    return true;
}


bool DIALOG_UPDATE_FIELDS::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    // Collect all field names from library parts of components that are going to be updated
    {
        for( auto component : m_components )
        {
            const auto part = component->GetPartRef().lock();

            if( !part )
                continue;

            const auto& drawItems = part->GetDrawItems();

            for( auto it = drawItems.begin( LIB_FIELD_T ); it != drawItems.end( LIB_FIELD_T ); ++it )
            {
                const LIB_FIELD* field = static_cast<const LIB_FIELD*>( &( *it ) );
                m_fields.insert( field->GetName( false ) );
            }
        }
    }

    // Update the listbox widget
    m_fieldsBox->Clear();

    for( const auto& field : m_fields )
    {
        int idx = m_fieldsBox->Append( field );

        if( field != "Reference" && field != "Value" )
            m_fieldsBox->Check( idx, true );
    }

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();

    return true;
}


void DIALOG_UPDATE_FIELDS::updateFields( SCH_COMPONENT* aComponent )
{
    std::vector<SCH_FIELD*> oldFields;
    SCH_FIELDS newFields;

    PART_SPTR libPart = aComponent->GetPartRef().lock();

    if( libPart == nullptr )    // the symbol is not found in lib: cannot update fields
        return;

    aComponent->GetFields( oldFields, false );

    for( auto compField : oldFields )
    {
        // If requested, transfer only fields that occur also in the original library part
        if( !m_removeExtraBox->IsChecked() || libPart->FindField( compField->GetName() ) )
            newFields.push_back( *compField );
    }

    // Update the requested field values
    for( const auto& partField : m_fields )
    {
        LIB_FIELD* libField = libPart->FindField( partField );

        if( !libField )
            continue;

        SCH_FIELD* field = nullptr;
        auto it = std::find_if( newFields.begin(), newFields.end(), [&] ( const SCH_FIELD& f )
                { return f.GetName() == partField; } );

        if( it != newFields.end() )
        {
            field = &*it;
        }
        else
        {
            // Missing field, it has to be added to the component
            SCH_FIELD f( wxPoint( 0, 0 ), newFields.size(), aComponent, partField );
            newFields.push_back( f );
            field = &newFields.back();
        }

        // If the library field is empty an update would clear an existing entry.
        // Check if this is the desired behavior.
        auto newText = libField->GetText();
        if( !newText.empty() || !m_omitEmpty->IsChecked() )
        {
           field->SetText( newText );
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
