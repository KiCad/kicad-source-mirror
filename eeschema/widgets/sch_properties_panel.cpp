/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright (C) 2021-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <connection_graph.h>
#include <properties/pg_editors.h>
#include <properties/pg_properties.h>
#include <properties/property_mgr.h>
#include <sch_commit.h>
#include <sch_edit_frame.h>
#include <schematic.h>
#include <settings/color_settings.h>
#include <string_utils.h>
#include <tool/tool_manager.h>
#include <tools/ee_selection_tool.h>


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
}



SCH_PROPERTIES_PANEL::~SCH_PROPERTIES_PANEL()
{
    m_unitEditorInstance->UpdateFrame( nullptr );
}


void SCH_PROPERTIES_PANEL::UpdateData()
{
    EE_SELECTION_TOOL* selectionTool = m_frame->GetToolManager()->GetTool<EE_SELECTION_TOOL>();
    const SELECTION& selection = selectionTool->GetSelection();

    // TODO perhaps it could be called less often? use PROPERTIES_TOOL and catch MODEL_RELOAD?
    if( SCH_EDIT_FRAME* schFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame ) )
        updateLists( schFrame->Schematic() );

    // Will actually just be updatePropertyValues() if selection hasn't changed
    rebuildProperties( selection );
}


void SCH_PROPERTIES_PANEL::AfterCommit()
{
    EE_SELECTION_TOOL* selectionTool = m_frame->GetToolManager()->GetTool<EE_SELECTION_TOOL>();
    const SELECTION& selection = selectionTool->GetSelection();

    rebuildProperties( selection );

    CallAfter( [&]()
               {
                   m_frame->GetCanvas()->SetFocus();
               } );
}


wxPGProperty* SCH_PROPERTIES_PANEL::createPGProperty( const PROPERTY_BASE* aProperty ) const
{
    wxPGProperty* prop = PGPropertyFactory( aProperty, m_frame );

    if( auto colorProp = dynamic_cast<PGPROPERTY_COLOR4D*>( prop ) )
    {
        COLOR4D bg = m_frame->GetColorSettings()->GetColor( LAYER_SCHEMATIC_BACKGROUND );
        colorProp->SetBackgroundColor( bg );
    }

    return prop;
}


PROPERTY_BASE* SCH_PROPERTIES_PANEL::getPropertyFromEvent( const wxPropertyGridEvent& aEvent ) const
{
    EE_SELECTION_TOOL* selectionTool = m_frame->GetToolManager()->GetTool<EE_SELECTION_TOOL>();
    const SELECTION& selection = selectionTool->GetSelection();
    SCH_ITEM* firstItem = static_cast<SCH_ITEM*>( selection.Front() );

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
    EE_SELECTION_TOOL* selectionTool = m_frame->GetToolManager()->GetTool<EE_SELECTION_TOOL>();
    const SELECTION& selection = selectionTool->GetSelection();
    EDA_ITEM* item = selection.Front();

    PROPERTY_BASE* property = getPropertyFromEvent( aEvent );
    wxCHECK( property, /* void */ );
    wxCHECK( item, /* void */ );

    wxVariant newValue = aEvent.GetPropertyValue();

    if( VALIDATOR_RESULT validationFailure = property->Validate( newValue.GetAny(), item ) )
    {
        wxString errorMsg = wxString::Format( wxS( "%s: %s" ), wxGetTranslation( property->Name() ),
                                              validationFailure->get()->Format( m_frame ) );
        m_frame->ShowInfoBarError( errorMsg );
        aEvent.Veto();
        return;
    }
}


void SCH_PROPERTIES_PANEL::valueChanged( wxPropertyGridEvent& aEvent )
{
    EE_SELECTION_TOOL* selectionTool = m_frame->GetToolManager()->GetTool<EE_SELECTION_TOOL>();
    const SELECTION& selection = selectionTool->GetSelection();

    PROPERTY_BASE* property = getPropertyFromEvent( aEvent );
    wxCHECK( property, /* void */ );

    wxVariant newValue = aEvent.GetPropertyValue();
    SCH_COMMIT changes( m_frame );
    SCH_SCREEN* screen = m_frame->GetScreen();

    for( EDA_ITEM* edaItem : selection )
    {
        SCH_ITEM* item = static_cast<SCH_ITEM*>( edaItem );
        changes.Modify( item, screen );
        item->Set( property, newValue );
    }

    changes.Push( _( "Change property" ) );
    m_frame->Refresh();

    // Perform grid updates as necessary based on value change
    AfterCommit();
}


void SCH_PROPERTIES_PANEL::updateLists( const SCHEMATIC& aSchematic )
{
    // No lists yet
}
