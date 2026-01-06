/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "sch_properties_panel.h"

#include <font/fontconfig.h>
#include <pgm_base.h>
#include <connection_graph.h>
#include <properties/pg_editors.h>
#include <properties/pg_properties.h>
#include <properties/property_mgr.h>
#include <sch_commit.h>
#include <sch_edit_frame.h>
#include <symbol_edit_frame.h>
#include <symbol_viewer_frame.h>
#include <schematic.h>
#include <sch_symbol.h>
#include <sch_field.h>
#include <template_fieldnames.h>
#include <settings/color_settings.h>
#include <string_utils.h>
#include <tool/tool_manager.h>
#include <tools/sch_selection_tool.h>
#include <set>

static const wxString MISSING_FIELD_SENTINEL = wxS( "\uE000" );

class SCH_SYMBOL_FIELD_PROPERTY : public PROPERTY_BASE
{
public:
    SCH_SYMBOL_FIELD_PROPERTY( const wxString& aName ) :
            PROPERTY_BASE( aName ),
            m_name( aName )
    {
    }

    size_t OwnerHash() const override { return TYPE_HASH( SCH_SYMBOL ); }
    size_t BaseHash() const override { return TYPE_HASH( SCH_SYMBOL ); }
    size_t TypeHash() const override { return TYPE_HASH( wxString ); }

    bool Writeable( INSPECTABLE* aObject ) const override
    {
        return PROPERTY_BASE::Writeable( aObject );
    }

    void setter( void* obj, wxAny& v ) override
    {
        wxString value;

        if( !v.GetAs( &value ) )
            return;

        SCH_SYMBOL* symbol = reinterpret_cast<SCH_SYMBOL*>( obj );
        SCH_FIELD*  field = symbol->GetField( m_name );

        wxString variantName;
        const SCH_SHEET_PATH* sheetPath = nullptr;

        if( symbol->Schematic() )
        {
            variantName = symbol->Schematic()->GetCurrentVariant();
            sheetPath = &symbol->Schematic()->CurrentSheet();
        }

        if( !field )
        {
            SCH_FIELD newField( symbol, FIELD_T::USER, m_name );
            newField.SetText( value, sheetPath, variantName );
            symbol->AddField( newField );
        }
        else
        {
            field->SetText( value, sheetPath, variantName );
        }
    }

    wxAny getter( const void* obj ) const override
    {
        const SCH_SYMBOL* symbol = reinterpret_cast<const SCH_SYMBOL*>( obj );
        const SCH_FIELD*  field = symbol->GetField( m_name );

        if( field )
        {
            wxString variantName;
            const SCH_SHEET_PATH* sheetPath = nullptr;

            if( symbol->Schematic() )
            {
                variantName = symbol->Schematic()->GetCurrentVariant();
                sheetPath = &symbol->Schematic()->CurrentSheet();
            }

            wxString text;

            if( !variantName.IsEmpty() && sheetPath )
                text = field->GetText( sheetPath, variantName );
            else
                text = field->GetText();

            return wxAny( text );
        }
        else
        {
            return wxAny( MISSING_FIELD_SENTINEL );
        }
    }

private:
    wxString m_name;
};

std::set<wxString> SCH_PROPERTIES_PANEL::m_currentFieldNames;

SCH_PROPERTIES_PANEL::SCH_PROPERTIES_PANEL( wxWindow* aParent, SCH_BASE_FRAME* aFrame ) :
        PROPERTIES_PANEL( aParent, aFrame ),
        m_frame( aFrame ),
        m_propMgr( PROPERTY_MANAGER::Instance() )
{
    m_propMgr.Rebuild();
    bool found = false;

    wxASSERT( wxPGGlobalVars );

    wxString editorKey = PG_UNIT_EDITOR::BuildEditorName( m_frame );

    auto it = wxPGGlobalVars->m_mapEditorClasses.find( editorKey );

    if( it != wxPGGlobalVars->m_mapEditorClasses.end() )
    {
        m_unitEditorInstance = static_cast<PG_UNIT_EDITOR*>( it->second );
        m_unitEditorInstance->UpdateFrame( m_frame );
        found = true;
    }

    if( !found )
    {
        PG_UNIT_EDITOR* new_editor = new PG_UNIT_EDITOR( m_frame );
        m_unitEditorInstance = static_cast<PG_UNIT_EDITOR*>( wxPropertyGrid::RegisterEditorClass( new_editor ) );
    }

    it = wxPGGlobalVars->m_mapEditorClasses.find( PG_CHECKBOX_EDITOR::EDITOR_NAME );

    if( it == wxPGGlobalVars->m_mapEditorClasses.end() )
    {
        PG_CHECKBOX_EDITOR* cbEditor = new PG_CHECKBOX_EDITOR();
        m_checkboxEditorInstance = static_cast<PG_CHECKBOX_EDITOR*>( wxPropertyGrid::RegisterEditorClass( cbEditor ) );
    }
    else
    {
        m_checkboxEditorInstance = static_cast<PG_CHECKBOX_EDITOR*>( it->second );
    }

    it = wxPGGlobalVars->m_mapEditorClasses.find( PG_COLOR_EDITOR::EDITOR_NAME );

    if( it == wxPGGlobalVars->m_mapEditorClasses.end() )
    {
        PG_COLOR_EDITOR* colorEditor = new PG_COLOR_EDITOR();
        m_colorEditorInstance = static_cast<PG_COLOR_EDITOR*>( wxPropertyGrid::RegisterEditorClass( colorEditor ) );
    }
    else
    {
        m_colorEditorInstance = static_cast<PG_COLOR_EDITOR*>( it->second );
    }

    it = wxPGGlobalVars->m_mapEditorClasses.find( PG_FPID_EDITOR::BuildEditorName( m_frame ) );

    if( it != wxPGGlobalVars->m_mapEditorClasses.end() )
    {
        m_fpEditorInstance = static_cast<PG_FPID_EDITOR*>( it->second );
        m_fpEditorInstance->UpdateFrame( m_frame );
    }
    else
    {
        PG_FPID_EDITOR* fpEditor = new PG_FPID_EDITOR( m_frame );
        m_fpEditorInstance = static_cast<PG_FPID_EDITOR*>( wxPropertyGrid::RegisterEditorClass( fpEditor ) );
    }

    it = wxPGGlobalVars->m_mapEditorClasses.find( PG_URL_EDITOR::BuildEditorName( m_frame ) );

    if( it != wxPGGlobalVars->m_mapEditorClasses.end() )
    {
        m_urlEditorInstance = static_cast<PG_URL_EDITOR*>( it->second );
        m_urlEditorInstance->UpdateFrame( m_frame );
    }
    else
    {
        PG_URL_EDITOR* urlEditor = new PG_URL_EDITOR( m_frame );
        m_urlEditorInstance = static_cast<PG_URL_EDITOR*>( wxPropertyGrid::RegisterEditorClass( urlEditor ) );
    }
}



SCH_PROPERTIES_PANEL::~SCH_PROPERTIES_PANEL()
{
    m_unitEditorInstance->UpdateFrame( nullptr );
    m_fpEditorInstance->UpdateFrame( nullptr );
    m_urlEditorInstance->UpdateFrame( nullptr );
}


const SELECTION& SCH_PROPERTIES_PANEL::getSelection( SELECTION& aFallbackSelection )
{
    SCH_SELECTION_TOOL* selectionTool = m_frame->GetToolManager()->GetTool<SCH_SELECTION_TOOL>();
    const SELECTION& selection = selectionTool->GetSelection();

    if( selection.Empty() && m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR ) )
    {
        SYMBOL_EDIT_FRAME* symbolFrame = static_cast<SYMBOL_EDIT_FRAME*>( m_frame );

        if( symbolFrame->GetCurSymbol() )
        {
            aFallbackSelection.Clear();
            aFallbackSelection.Add( symbolFrame->GetCurSymbol() );
            return aFallbackSelection;
        }
    }

    return selection;
}


EDA_ITEM* SCH_PROPERTIES_PANEL::getFrontItem()
{
    SELECTION fallbackSelection;
    const SELECTION& selection = getSelection( fallbackSelection );

    return selection.Empty() ? nullptr : selection.Front();
}


void SCH_PROPERTIES_PANEL::UpdateData()
{
    SELECTION fallbackSelection;
    const SELECTION& selection = getSelection( fallbackSelection );

    // Will actually just be updatePropertyValues() if selection hasn't changed
    rebuildProperties( selection );
}


void SCH_PROPERTIES_PANEL::AfterCommit()
{
    SELECTION fallbackSelection;
    const SELECTION& selection = getSelection( fallbackSelection );

    rebuildProperties( selection );
}


void SCH_PROPERTIES_PANEL::rebuildProperties( const SELECTION& aSelection )
{
    m_currentFieldNames.clear();

    for( EDA_ITEM* item : aSelection )
    {
        if( item->Type() != SCH_SYMBOL_T )
            continue;

        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        for( const SCH_FIELD& field : symbol->GetFields() )
        {
            if( field.IsPrivate() )
                continue;

            m_currentFieldNames.insert( field.GetCanonicalName() );
        }
    }

    const wxString groupFields = _HKI( "Fields" );

    for( const wxString& name : m_currentFieldNames )
    {
        if( !m_propMgr.GetProperty( TYPE_HASH( SCH_SYMBOL ), name ) )
        {
            m_propMgr.AddProperty( new SCH_SYMBOL_FIELD_PROPERTY( name ), groupFields )
                    .SetAvailableFunc( [name]( INSPECTABLE* )
                                       {
                                           return SCH_PROPERTIES_PANEL::m_currentFieldNames.count( name );
                                       } );
        }
    }

    PROPERTIES_PANEL::rebuildProperties( aSelection );
}


wxPGProperty* SCH_PROPERTIES_PANEL::createPGProperty( const PROPERTY_BASE* aProperty ) const
{
    wxPGProperty* prop = PGPropertyFactory( aProperty, m_frame );

    if( auto colorProp = dynamic_cast<PGPROPERTY_COLOR4D*>( prop ) )
    {
        COLOR4D bg = m_frame->GetColorSettings()->GetColor( LAYER_SCHEMATIC_BACKGROUND );
        colorProp->SetBackgroundColor( bg );
    }

    if( aProperty->Name() == GetCanonicalFieldName( FIELD_T::FOOTPRINT ) )
        prop->SetEditor( PG_FPID_EDITOR::BuildEditorName( m_frame ) );
    else if( aProperty->Name() == GetCanonicalFieldName( FIELD_T::DATASHEET ) )
        prop->SetEditor( PG_URL_EDITOR::BuildEditorName( m_frame ) );

    return prop;
}


PROPERTY_BASE* SCH_PROPERTIES_PANEL::getPropertyFromEvent( const wxPropertyGridEvent& aEvent ) const
{
    EDA_ITEM* item = const_cast<SCH_PROPERTIES_PANEL*>( this )->getFrontItem();

    if( !item || !item->IsSCH_ITEM() )
        return nullptr;

    SCH_ITEM* firstItem = static_cast<SCH_ITEM*>( item );

    wxCHECK_MSG( firstItem, nullptr,
                 wxT( "getPropertyFromEvent for a property with nothing selected!") );

    PROPERTY_BASE* property = m_propMgr.GetProperty( TYPE_HASH( *firstItem ),
                                                     aEvent.GetPropertyName() );
    wxCHECK_MSG( property, nullptr,
                 wxT( "getPropertyFromEvent for a property not found on the selected item!" ) );

    return property;
}


void SCH_PROPERTIES_PANEL::valueChanging( wxPropertyGridEvent& aEvent )
{
    if( m_SuppressGridChangeEvents )
        return;

    EDA_ITEM* frontItem = getFrontItem();

    if( !frontItem )
        return;

    if( PROPERTY_BASE* property = getPropertyFromEvent( aEvent ) )
    {
        wxVariant newValue = aEvent.GetPropertyValue();

        if( VALIDATOR_RESULT validationFailure = property->Validate( newValue.GetAny(), frontItem ) )
        {
            wxString errorMsg = wxString::Format( wxS( "%s: %s" ), wxGetTranslation( property->Name() ),
                                                  validationFailure->get()->Format( m_frame ) );
            m_frame->ShowInfoBarError( errorMsg );
            aEvent.Veto();
            return;
        }

        aEvent.Skip();
    }
}


void SCH_PROPERTIES_PANEL::valueChanged( wxPropertyGridEvent& aEvent )
{
    if( m_SuppressGridChangeEvents )
        return;

    SELECTION fallbackSelection;
    const SELECTION& selection = getSelection( fallbackSelection );

    wxCHECK( getPropertyFromEvent( aEvent ), /* void */ );

    wxVariant   newValue = aEvent.GetPropertyValue();
    SCH_COMMIT  changes( m_frame );
    SCH_SCREEN* screen = m_frame->GetScreen();

    PROPERTY_COMMIT_HANDLER handler( &changes );

    for( EDA_ITEM* edaItem : selection )
    {
        if( !edaItem->IsSCH_ITEM() )
            continue;

        SCH_ITEM* item = static_cast<SCH_ITEM*>( edaItem );
        PROPERTY_BASE* property = m_propMgr.GetProperty( TYPE_HASH( *item ), aEvent.GetPropertyName() );
        wxCHECK2( property, continue );

        // Editing reference text in the schematic must go through the parent symbol in order to handle
        // symbol instance data properly.
        if( item->Type() == SCH_FIELD_T && static_cast<SCH_FIELD*>( item )->GetId() == FIELD_T::REFERENCE
                && m_frame->IsType( FRAME_SCH )
                && property->Name() == wxT( "Text" ) )
        {
            SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( item->GetParentSymbol() );
            wxCHECK2( symbol, continue );

            changes.Modify( symbol, screen, RECURSE_MODE::NO_RECURSE );
            symbol->SetRefProp( newValue.GetString() );
            symbol->SyncOtherUnits( symbol->Schematic()->CurrentSheet(), changes, property );
            continue;
        }

        // Editing field text in the schematic when a variant is active must use variant-aware
        // SetText to properly store the value as a variant override.
        if( item->Type() == SCH_FIELD_T
                && m_frame->IsType( FRAME_SCH )
                && property->Name() == wxT( "Text" ) )
        {
            SCH_FIELD* field = static_cast<SCH_FIELD*>( item );
            SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( item->GetParentSymbol() );

            if( symbol && symbol->Schematic() )
            {
                wxString variantName = symbol->Schematic()->GetCurrentVariant();

                if( !variantName.IsEmpty() )
                {
                    changes.Modify( symbol, screen, RECURSE_MODE::NO_RECURSE );
                    field->SetText( newValue.GetString(), &symbol->Schematic()->CurrentSheet(),
                                    variantName );
                    symbol->SyncOtherUnits( symbol->Schematic()->CurrentSheet(), changes, property );
                    continue;
                }
            }
        }

        if( item->Type() == SCH_TABLECELL_T )
            changes.Modify( item->GetParent(), screen, RECURSE_MODE::NO_RECURSE );
        else
            changes.Modify( item, screen, RECURSE_MODE::NO_RECURSE );

        item->Set( property, newValue );

        if( SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( item ) )
            symbol->SyncOtherUnits( symbol->Schematic()->CurrentSheet(), changes, property );
    }

    changes.Push( _( "Edit Properties" ) );

    // Force a repaint of the items whose properties were changed
    // This is necessary to update field displays in the schematic view
    for( EDA_ITEM* edaItem : selection )
        m_frame->UpdateItem( edaItem );

    // Perform grid updates as necessary based on value change
    AfterCommit();

    aEvent.Skip();
}


void SCH_PROPERTIES_PANEL::OnLanguageChanged( wxCommandEvent& aEvent )
{
    PROPERTIES_PANEL::OnLanguageChanged( aEvent );

    aEvent.Skip();
}


bool SCH_PROPERTIES_PANEL::getItemValue( EDA_ITEM* aItem, PROPERTY_BASE* aProperty, wxVariant& aValue )
{
    // For SCH_FIELD "Text" property, return the variant-aware value when a variant is active
    if( aItem->Type() == SCH_FIELD_T
            && m_frame->IsType( FRAME_SCH )
            && aProperty->Name() == wxT( "Text" ) )
    {
        SCH_FIELD* field = static_cast<SCH_FIELD*>( aItem );
        SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( field->GetParentSymbol() );

        if( symbol && symbol->Schematic() )
        {
            wxString variantName = symbol->Schematic()->GetCurrentVariant();

            if( !variantName.IsEmpty() )
            {
                wxString text = field->GetText( &symbol->Schematic()->CurrentSheet(), variantName );
                aValue = wxVariant( text );
                return true;
            }
        }
    }

    return PROPERTIES_PANEL::getItemValue( aItem, aProperty, aValue );
}


