/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2020-2023 CERN
 * Copyright (C) 2021-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <font/fontconfig.h>
#include <font/kicad_font_name.h>
#include <pgm_base.h>
#include <pcb_base_edit_frame.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <properties/property_mgr.h>
#include <properties/pg_editors.h>
#include <board_commit.h>
#include <board_connected_item.h>
#include <properties/pg_properties.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <pcb_track.h>
#include <pcb_generator.h>
#include <pad.h>
#include <settings/color_settings.h>
#include <string_utils.h>


PCB_PROPERTIES_PANEL::PCB_PROPERTIES_PANEL( wxWindow* aParent, PCB_BASE_EDIT_FRAME* aFrame ) :
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

    it = wxPGGlobalVars->m_mapEditorClasses.find( PG_RATIO_EDITOR::EDITOR_NAME );

    if( it == wxPGGlobalVars->m_mapEditorClasses.end() )
    {
        PG_RATIO_EDITOR* ratioEditor = new PG_RATIO_EDITOR();
        m_ratioEditorInstance = static_cast<PG_RATIO_EDITOR*>( wxPropertyGrid::RegisterEditorClass( ratioEditor ) );
    }
    else
    {
        m_ratioEditorInstance = static_cast<PG_RATIO_EDITOR*>( it->second );
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

    rebuildProperties( selection );

    CallAfter( [this]()
               {
                   static_cast<PCB_EDIT_FRAME*>( m_frame )->GetCanvas()->SetFocus();
               } );
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

        auto ret = new PGPROPERTY_COLORENUM( new wxPGChoices( boardLayerNames, boardLayerIDs ) );

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

    return PGPropertyFactory( aProperty, m_frame );
}


PROPERTY_BASE* PCB_PROPERTIES_PANEL::getPropertyFromEvent( const wxPropertyGridEvent& aEvent ) const
{
    PCB_SELECTION_TOOL* selectionTool = m_frame->GetToolManager()->GetTool<PCB_SELECTION_TOOL>();
    const SELECTION& selection = selectionTool->GetSelection();
    BOARD_ITEM* firstItem = static_cast<BOARD_ITEM*>( selection.Front() );

    wxCHECK_MSG( firstItem, nullptr,
                 wxT( "getPropertyFromEvent for a property with nothing selected!") );

    PROPERTY_BASE* property = m_propMgr.GetProperty( TYPE_HASH( *firstItem ),
                                                     aEvent.GetPropertyName() );
    wxCHECK_MSG( property, nullptr,
                 wxT( "getPropertyFromEvent for a property not found on the selected item!" ) );

    return property;
}


void PCB_PROPERTIES_PANEL::valueChanging( wxPropertyGridEvent& aEvent )
{
    PCB_SELECTION_TOOL* selectionTool = m_frame->GetToolManager()->GetTool<PCB_SELECTION_TOOL>();
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


void PCB_PROPERTIES_PANEL::valueChanged( wxPropertyGridEvent& aEvent )
{
    PCB_SELECTION_TOOL* selectionTool = m_frame->GetToolManager()->GetTool<PCB_SELECTION_TOOL>();
    const SELECTION& selection = selectionTool->GetSelection();

    PROPERTY_BASE* property = getPropertyFromEvent( aEvent );
    wxCHECK( property, /* void */ );

    wxVariant newValue = aEvent.GetPropertyValue();
    BOARD_COMMIT changes( m_frame );

    PROPERTY_COMMIT_HANDLER handler( &changes );

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
    wxPGChoices layersAll;
    wxPGChoices layersCu;
    wxPGChoices nets;
    wxPGChoices fonts;

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

    std::vector<std::pair<wxString, int>> netNames;
    netNames.reserve( aBoard->GetNetInfo().NetsByNetcode().size() );

    for( const auto& [ netCode, netInfo ] : aBoard->GetNetInfo().NetsByNetcode() )
        netNames.emplace_back( UnescapeString( netInfo->GetNetname() ), netCode );

    std::sort( netNames.begin(), netNames.end(), []( const auto& a, const auto& b )
    {
        return a.first.CmpNoCase( b.first ) < 0;
    } );

    for( const auto& [ netName, netCode ] : netNames )
        nets.Add( netName, netCode );

    auto netProperty = m_propMgr.GetProperty( TYPE_HASH( BOARD_CONNECTED_ITEM ), _HKI( "Net" ) );
    netProperty->SetChoices( nets );

    // Regnerate font names
    std::vector<std::string> fontNames;
    Fontconfig()->ListFonts( fontNames, std::string( Pgm().GetLanguageTag().utf8_str() ) );

    fonts.Add( KICAD_FONT_NAME, -1 );

    for( int ii = 0; ii < (int) fontNames.size(); ++ii )
        fonts.Add( wxString( fontNames[ii] ), ii );

    auto fontProperty = m_propMgr.GetProperty( TYPE_HASH( EDA_TEXT ), _HKI( "Font" ) );
    fontProperty->SetChoices( fonts );
}
