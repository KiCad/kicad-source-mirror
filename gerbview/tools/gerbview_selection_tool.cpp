/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jon Evans <jon@craftyjon.com>
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

#include <limits>
#include <functional>
using namespace std::placeholders;
#include <bitmaps.h>
#include <eda_item.h>
#include <gerber_collectors.h>
#include <gerbview_settings.h>
#include <class_draw_panel_gal.h>
#include <string_utils.h>
#include <view/view.h>
#include <view/view_group.h>
#include <gal/painter.h>
#include <tool/tool_event.h>
#include <tool/tool_manager.h>
#include "gerbview_selection_tool.h"
#include "gerbview_actions.h"


class HIGHLIGHT_MENU : public ACTION_MENU
{
public:
    HIGHLIGHT_MENU() :
        ACTION_MENU( true )
    {
        SetIcon( BITMAPS::net_highlight_schematic );
        SetTitle( _( "Highlight" ) );
    }

private:

    void update() override
    {
        bool addSeparator = false;

        Clear();

        const auto& selection = getToolManager()->GetTool<GERBVIEW_SELECTION_TOOL>()->GetSelection();

        if( selection.Size() == 1 )
        {
            auto item = static_cast<GERBER_DRAW_ITEM*>( selection[0] );
            const auto& net_attr = item->GetNetAttributes();

            if( ( net_attr.m_NetAttribType & GBR_NETLIST_METADATA::GBR_NETINFO_PAD ) ||
                ( net_attr.m_NetAttribType & GBR_NETLIST_METADATA::GBR_NETINFO_CMP ) )
            {
                auto menuEntry = Add( GERBVIEW_ACTIONS::highlightComponent );
                menuEntry->SetItemLabel( wxString::Format( _( "Highlight Items of Component '%s'" ),
                                                           net_attr.m_Cmpref ) );
                addSeparator = true;
            }

            if( ( net_attr.m_NetAttribType & GBR_NETLIST_METADATA::GBR_NETINFO_NET ) )
            {
                auto menuEntry = Add( GERBVIEW_ACTIONS::highlightNet );
                menuEntry->SetItemLabel( wxString::Format( _( "Highlight Items of Net '%s'" ),
                                                           UnescapeString( net_attr.m_Netname ) ) );
                addSeparator = true;
            }

            D_CODE* apertDescr = item->GetDcodeDescr();

            if( apertDescr && !apertDescr->m_AperFunction.IsEmpty() )
            {
                auto menuEntry = Add( GERBVIEW_ACTIONS::highlightAttribute );
                menuEntry->SetItemLabel( wxString::Format( _( "Highlight Aperture Type '%s'" ),
                                                           apertDescr->m_AperFunction ) );
                addSeparator = true;
            }

            if( apertDescr )
            {
                auto menuEntry = Add( GERBVIEW_ACTIONS::highlightDCode );
                menuEntry->SetItemLabel( wxString::Format( _( "Highlight DCode D%d" ),
                                                           apertDescr->m_Num_Dcode ) );
                addSeparator = true;
            }
        }

        if( addSeparator )
            AppendSeparator();

        Add( GERBVIEW_ACTIONS::highlightClear );
    }

    ACTION_MENU* create() const override
    {
        return new HIGHLIGHT_MENU();
    }
};


GERBVIEW_SELECTION_TOOL::GERBVIEW_SELECTION_TOOL() :
        SELECTION_TOOL( "common.InteractiveSelection" ),
        m_frame( nullptr )
{
}


GERBVIEW_SELECTION_TOOL::~GERBVIEW_SELECTION_TOOL()
{
    getView()->Remove( &m_selection );
}


bool GERBVIEW_SELECTION_TOOL::Init()
{
    std::shared_ptr<HIGHLIGHT_MENU> highlightSubMenu = std::make_shared<HIGHLIGHT_MENU>();
    highlightSubMenu->SetTool( this );
    m_menu->RegisterSubMenu( highlightSubMenu );

    m_menu->GetMenu().AddMenu( highlightSubMenu.get() );
    m_menu->GetMenu().AddSeparator( 1000 );

    getEditFrame<GERBVIEW_FRAME>()->AddStandardSubMenus( *m_menu.get() );

    return true;
}


void GERBVIEW_SELECTION_TOOL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<GERBVIEW_FRAME>();

    if( aReason == TOOL_BASE::MODEL_RELOAD || aReason == RESET_REASON::SHUTDOWN )
    {
        // Remove pointers to the selected items from containers
        // without changing their properties (as they are already deleted
        // while a new file is loaded)
        m_selection.Clear();
        getView()->GetPainter()->GetSettings()->SetHighlight( false );
    }
    else
    {
        // Restore previous properties of selected items and remove them from containers
        clearSelection();
    }

    // Reinsert the VIEW_GROUP, in case it was removed from the VIEW
    getView()->Remove( &m_selection );
    getView()->Add( &m_selection );
}


int GERBVIEW_SELECTION_TOOL::Main( const TOOL_EVENT& aEvent )
{
    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        if( m_frame->ToolStackIsEmpty() )
            m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );

        // on left click, a selection is made, depending on modifiers ALT, SHIFT, CTRL:
        setModifiersState( evt->Modifier( MD_SHIFT ), evt->Modifier( MD_CTRL ),
                           evt->Modifier( MD_ALT ) );

        // single click? Select single object
        if( evt->IsClick( BUT_LEFT ) )
        {
            selectPoint( evt->Position() );
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // right click? if there is any object - show the context menu
            if( m_selection.Empty() )
            {
                selectPoint( evt->Position() );
                m_selection.SetIsHover( true );
            }

            // Show selection before opening menu
            m_frame->GetCanvas()->ForceRefresh();

            m_menu->ShowContextMenu( m_selection );
        }
        else if( evt->IsDblClick( BUT_MIDDLE ) )
        {
            // Middle double click?  Do zoom to fit
            m_toolMgr->RunAction( ACTIONS::zoomFitScreen );
        }
        else if( evt->IsCancel() || evt->Action() == TA_UNDO_REDO_PRE )
        {
            clearSelection();
        }
        else
        {
            evt->SetPassEvent();
        }
    }

    return 0;
}


GERBVIEW_SELECTION& GERBVIEW_SELECTION_TOOL::GetSelection()
{
    return m_selection;
}


bool GERBVIEW_SELECTION_TOOL::selectPoint( const VECTOR2I& aWhere )
{
    EDA_ITEM*        item = nullptr;
    GERBER_COLLECTOR collector;
    EDA_ITEM*        model = getModel<EDA_ITEM>();

    collector.Collect( model, { GERBER_LAYOUT_T, GERBER_IMAGE_T, GERBER_DRAW_ITEM_T }, aWhere );

    // Remove unselectable items
    for( int i = collector.GetCount() - 1; i >= 0; --i )
    {
        if( !selectable( collector[i] ) )
            collector.Remove( i );
    }

    if( collector.GetCount() > 1 )
    {
        doSelectionMenu( &collector );

        if( collector.m_MenuCancelled )
            return false;
    }

    if( !m_additive && !m_subtractive && !m_exclusive_or )
        clearSelection();

    if( collector.GetCount() == 1 )
    {
        item = collector[ 0 ];

        if( m_subtractive || ( m_exclusive_or && item->IsSelected() ) )
        {
            unselect( item );
            m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );
            return false;
        }
        else
        {
            select( item );
            m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
            return true;
        }
    }

    return false;
}


void GERBVIEW_SELECTION_TOOL::setTransitions()
{
    Go( &GERBVIEW_SELECTION_TOOL::UpdateMenu,     ACTIONS::updateMenu.MakeEvent() );
    Go( &GERBVIEW_SELECTION_TOOL::Main,           ACTIONS::selectionActivate.MakeEvent() );
    Go( &GERBVIEW_SELECTION_TOOL::ClearSelection, ACTIONS::selectionClear.MakeEvent() );
    Go( &GERBVIEW_SELECTION_TOOL::SelectItem,     ACTIONS::selectItem.MakeEvent() );
    Go( &GERBVIEW_SELECTION_TOOL::UnselectItem,   ACTIONS::unselectItem.MakeEvent() );
}


int GERBVIEW_SELECTION_TOOL::ClearSelection( const TOOL_EVENT& aEvent )
{
    clearSelection();

    return 0;
}


int GERBVIEW_SELECTION_TOOL::SelectItems( const TOOL_EVENT& aEvent )
{
    std::vector<EDA_ITEM*>* items = aEvent.Parameter<std::vector<EDA_ITEM*>*>();

    if( items )
    {
        // Perform individual selection of each item before processing the event.
        for( EDA_ITEM* item : *items )
            select( item );

        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
    }

    return 0;
}


int GERBVIEW_SELECTION_TOOL::SelectItem( const TOOL_EVENT& aEvent )
{
    // Check if there is an item to be selected
    EDA_ITEM* item = aEvent.Parameter<EDA_ITEM*>();

    if( item )
    {
        select( item );

        // Inform other potentially interested tools
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
    }

    return 0;
}


int GERBVIEW_SELECTION_TOOL::UnselectItems( const TOOL_EVENT& aEvent )
{
    std::vector<EDA_ITEM*>* items = aEvent.Parameter<std::vector<EDA_ITEM*>*>();

    if( items )
    {
        // Perform individual unselection of each item before processing the event
        for( EDA_ITEM* item : *items )
            unselect( item );

        m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );
    }

    return 0;
}


int GERBVIEW_SELECTION_TOOL::UnselectItem( const TOOL_EVENT& aEvent )
{
    // Check if there is an item to be selected
    EDA_ITEM* item = aEvent.Parameter<EDA_ITEM*>();

    if( item )
    {
        unselect( item );

        // Inform other potentially interested tools
        m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );
    }

    return 0;
}


void GERBVIEW_SELECTION_TOOL::clearSelection()
{
    if( m_selection.Empty() )
        return;

    for( EDA_ITEM* item : m_selection )
        unselectVisually( item );

    m_selection.Clear();

    // Inform other potentially interested tools
    m_toolMgr->ProcessEvent( EVENTS::ClearedEvent );
}


bool GERBVIEW_SELECTION_TOOL::selectable( const EDA_ITEM* aItem ) const
{
    GERBVIEW_FRAME*         frame = getEditFrame<GERBVIEW_FRAME>();
    const GERBER_DRAW_ITEM* item = static_cast<const GERBER_DRAW_ITEM*>( aItem );
    int                     layer = item->GetLayer();

    if( !frame->gvconfig()->m_Appearance.show_negative_objects && item->GetLayerPolarity() )
        return false;

    // We do not want to select items that are in the background
    if( frame->gvconfig()->m_Display.m_HighContrastMode && layer != frame->GetActiveLayer() )
        return false;

    return frame->IsLayerVisible( layer );
}


void GERBVIEW_SELECTION_TOOL::select( EDA_ITEM* aItem )
{
    if( aItem->IsSelected() )
        return;

    m_selection.Add( aItem );
    getView()->Add( &m_selection, std::numeric_limits<int>::max() );
    selectVisually( aItem );
}


void GERBVIEW_SELECTION_TOOL::unselect( EDA_ITEM* aItem )
{
    if( !aItem->IsSelected() )
        return;

    unselectVisually( aItem );
    m_selection.Remove( aItem );

    if( m_selection.Empty() )
        getView()->Remove( &m_selection );
}


void GERBVIEW_SELECTION_TOOL::selectVisually( EDA_ITEM* aItem )
{
    // Move the item's layer to the front
    int layer = static_cast<GERBER_DRAW_ITEM*>( aItem )->GetLayer();
    m_frame->SetActiveLayer( layer, true );

    // Hide the original item, so it is shown only on overlay
    aItem->SetSelected();
    getView()->Hide( aItem, true );

    getView()->Update( &m_selection );
}


void GERBVIEW_SELECTION_TOOL::unselectVisually( EDA_ITEM* aItem )
{
    // Restore original item visibility
    aItem->ClearSelected();
    getView()->Hide( aItem, false );
    getView()->Update( aItem, KIGFX::ALL );

    getView()->Update( &m_selection );
}
