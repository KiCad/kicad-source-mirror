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

#include "pcb_properties_panel.h"

#include <pcb_edit_frame.h>
#include <tool/tool_manager.h>
#include <tools/pcb_selection_tool.h>
#include <properties/property_mgr.h>
#include <properties/pg_editors.h>
#include <board_commit.h>
#include <board_connected_item.h>
#include <properties/pg_properties.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <pcb_track.h>
#include <settings/color_settings.h>
#include <string_utils.h>


PCB_PROPERTIES_PANEL::PCB_PROPERTIES_PANEL( wxWindow* aParent, PCB_EDIT_FRAME* aFrame ) :
        PROPERTIES_PANEL( aParent, aFrame ),
        m_frame( aFrame ),
        m_propMgr( PROPERTY_MANAGER::Instance() )
{
    m_propMgr.Rebuild();
    bool found = false;

    wxASSERT( wxPGGlobalVars );

    auto it = wxPGGlobalVars->m_mapEditorClasses.find( PG_UNIT_EDITOR::EDITOR_NAME );

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
}



PCB_PROPERTIES_PANEL::~PCB_PROPERTIES_PANEL()
{
    m_unitEditorInstance->UpdateFrame( nullptr );
}


void PCB_PROPERTIES_PANEL::UpdateData()
{
    PCB_SELECTION_TOOL* selectionTool = m_frame->GetToolManager()->GetTool<PCB_SELECTION_TOOL>();
    const SELECTION& selection = selectionTool->GetSelection();

    // TODO perhaps it could be called less often? use PROPERTIES_TOOL and catch MODEL_RELOAD?
    updateLists( static_cast<PCB_EDIT_FRAME*>( m_frame )->GetBoard() );

    // Will actually just be updatePropertyValues() if selection hasn't changed
    rebuildProperties( selection );
}


void PCB_PROPERTIES_PANEL::AfterCommit()
{
    PCB_SELECTION_TOOL* selectionTool = m_frame->GetToolManager()->GetTool<PCB_SELECTION_TOOL>();
    const SELECTION& selection = selectionTool->GetSelection();

    updatePropertyValues( selection );

    CallAfter( [&]()
               {
                   static_cast<PCB_EDIT_FRAME*>( m_frame )->GetCanvas()->SetFocus();
               } );
}


void PCB_PROPERTIES_PANEL::updatePropertyValues( const SELECTION& aSelection )
{
    // TODO: Refactor to reduce duplication with PROPERTIES_PANEL::rebuildProperties
    BOARD_ITEM* firstItem = static_cast<BOARD_ITEM*>( aSelection.Front() );

    for( wxPropertyGridIterator it = m_grid->GetIterator(); !it.AtEnd(); it.Next() )
    {
        wxPGProperty* pgProp = it.GetProperty();

        PROPERTY_BASE* property = m_propMgr.GetProperty( TYPE_HASH( *firstItem ),
                                                         pgProp->GetName() );
        wxCHECK2( property, continue );

        bool writeable = true;
        bool different = false;
        wxVariant commonVal;

        for( EDA_ITEM* edaItem : aSelection )
        {
            writeable &= property->Writeable( edaItem );

            wxVariant value = commonVal;

            if( getItemValue( edaItem, property, value ) )
            {
                // Null value indicates different property values between items
                if( !different && !commonVal.IsNull() && value != commonVal )
                {
                    different = true;
                    commonVal.MakeNull();
                }
                else if( !different )
                {
                    commonVal = value;
                }
            }
        }

        pgProp->SetValue( commonVal );
        pgProp->Enable( writeable );
    }
}


wxPGProperty* PCB_PROPERTIES_PANEL::createPGProperty( const PROPERTY_BASE* aProperty ) const
{
    if( aProperty->TypeHash() == TYPE_HASH( PCB_LAYER_ID ) )
    {
        wxASSERT( aProperty->HasChoices() );

        const wxPGChoices& canonicalLayers = aProperty->Choices();
        wxArrayString      boardLayerNames;
        wxArrayInt         boardLayerIDs;

        for( int ii = 0; ii < (int) aProperty->Choices().GetCount(); ++ii )
        {
            int layer = canonicalLayers.GetValue( ii );

            boardLayerNames.push_back( m_frame->GetBoard()->GetLayerName( ToLAYER_ID( layer ) ) );
            boardLayerIDs.push_back( canonicalLayers.GetValue( ii ) );
        }

        auto ret = new PGPROPERTY_COLORENUM( wxPG_LABEL, wxPG_LABEL,
                                             new wxPGChoices( boardLayerNames, boardLayerIDs ) );

        ret->SetColorFunc(
                [&]( int aValue ) -> wxColour
                {
                    return m_frame->GetColorSettings()->GetColor( ToLAYER_ID( aValue ) ).ToColour();
                } );

        ret->SetLabel( wxGetTranslation( aProperty->Name() ) );
        ret->SetName( aProperty->Name() );
        ret->SetHelpString( wxGetTranslation( aProperty->Name() ) );
        ret->SetClientData( const_cast<PROPERTY_BASE*>( aProperty ) );

        return ret;
    }

    return PGPropertyFactory( aProperty );
}


void PCB_PROPERTIES_PANEL::valueChanged( wxPropertyGridEvent& aEvent )
{
    PCB_SELECTION_TOOL* selectionTool = m_frame->GetToolManager()->GetTool<PCB_SELECTION_TOOL>();
    const SELECTION& selection = selectionTool->GetSelection();
    BOARD_ITEM* firstItem = static_cast<BOARD_ITEM*>( selection.Front() );

    wxCHECK_MSG( firstItem, /* void */,
                 wxT( "valueChanged for a property with nothing selected!") );

    PROPERTY_BASE* property = m_propMgr.GetProperty( TYPE_HASH( *firstItem ),
                                                     aEvent.GetPropertyName() );
    wxCHECK_MSG( property, /* void */,
                 wxT( "valueChanged for a property not found on the selected item!" ) );

    wxVariant newValue = aEvent.GetPropertyValue();
    BOARD_COMMIT changes( m_frame );

    for( EDA_ITEM* edaItem : selection )
    {
        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( edaItem );
        changes.Modify( item );
        item->Set( property, newValue );
    }

    changes.Push( _( "Change property" ) );
    m_frame->Refresh();

    // Perform grid updates as necessary based on value change
    AfterCommit();
}


void PCB_PROPERTIES_PANEL::updateLists( const BOARD* aBoard )
{
    wxPGChoices layersAll, layersCu, nets;

    // Regenerate all layers
    for( LSEQ seq = aBoard->GetEnabledLayers().UIOrder(); seq; ++seq )
        layersAll.Add( LSET::Name( *seq ), *seq );

    for( LSEQ seq = LSET( aBoard->GetEnabledLayers() & LSET::AllCuMask() ).UIOrder(); seq; ++seq )
        layersCu.Add( LSET::Name( *seq ), *seq );

    m_propMgr.GetProperty( TYPE_HASH( BOARD_ITEM ), _HKI( "Layer" ) )->SetChoices( layersAll );
    m_propMgr.GetProperty( TYPE_HASH( PCB_SHAPE ), _HKI( "Layer" ) )->SetChoices( layersAll );

    // Copper only properties
    m_propMgr.GetProperty( TYPE_HASH( BOARD_CONNECTED_ITEM ),
                           _HKI( "Layer" ) )->SetChoices( layersCu );
    m_propMgr.GetProperty( TYPE_HASH( PCB_VIA ), _HKI( "Layer Top" ) )->SetChoices( layersCu );
    m_propMgr.GetProperty( TYPE_HASH( PCB_VIA ), _HKI( "Layer Bottom" ) )->SetChoices( layersCu );

    // Regenerate nets
    for( const auto& [ netCode, netInfo ] : aBoard->GetNetInfo().NetsByNetcode() )
        nets.Add( UnescapeString( netInfo->GetNetname() ), netCode );

    auto netProperty = m_propMgr.GetProperty( TYPE_HASH( BOARD_CONNECTED_ITEM ), _HKI( "Net" ) );
    netProperty->SetChoices( nets );
}
