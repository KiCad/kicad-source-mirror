/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <bitmaps.h>
#include <widgets/ui_common.h>
#include <collector.h>
#include <tool/tool_manager.h>
#include <tool/actions.h>
#include <tool/selection_tool.h>
#include <view/view.h>
#include <eda_draw_frame.h>
#include <eda_item.h>


SELECTION_TOOL::SELECTION_TOOL( const std::string& aName ) :
    TOOL_INTERACTIVE( aName ),
    m_additive( false ),
    m_subtractive( false ),
    m_exclusive_or( false ),
    m_multiple( false ),
    m_skip_heuristics( false ),
    m_highlight_modifier( false ),
    m_drag_additive( false ),
    m_drag_subtractive( false ),
    m_canceledMenu( false )
{
}

void SELECTION_TOOL::setModifiersState( bool aShiftState, bool aCtrlState, bool aAltState )
{
    // Set the configuration of m_additive, m_subtractive, m_exclusive_or from the state of
    // modifier keys SHIFT and CTRL

    // ALT key cannot be used on MSW because of a conflict with the system menu

    m_subtractive        = aCtrlState && aShiftState;
    m_additive           = !aCtrlState && aShiftState;

    if( ctrlClickHighlights() )
    {
        m_exclusive_or = false;
        m_highlight_modifier = aCtrlState && !aShiftState;
    }
    else
    {
        m_exclusive_or = aCtrlState && !aShiftState;
        m_highlight_modifier = false;
    }

    // Drag is more forgiving and allows either Ctrl+Drag or Shift+Drag to add to the selection
    // Note, however that we cannot provide disambiguation at the same time as the box selection
    m_drag_additive      = ( aCtrlState || aShiftState ) && !aAltState;
    m_drag_subtractive   = aCtrlState && aShiftState && !aAltState;

    // While the ALT key has some conflicts under MSW (and some flavors of Linux WMs), it remains
    // useful for users who only use tap-click rather than holding the button.  We disable it for
    // windows because it flashes the disambiguation menu without showing data
#ifndef __WINDOWS__
    m_skip_heuristics = aAltState;
#else
    m_skip_heuristics = false;
#endif

}


bool SELECTION_TOOL::hasModifier()
{
    return m_subtractive || m_additive || m_exclusive_or;
}


int SELECTION_TOOL::UpdateMenu( const TOOL_EVENT& aEvent )
{
    ACTION_MENU*      actionMenu = aEvent.Parameter<ACTION_MENU*>();
    CONDITIONAL_MENU* conditionalMenu = dynamic_cast<CONDITIONAL_MENU*>( actionMenu );

    if( conditionalMenu )
        conditionalMenu->Evaluate( selection() );

    if( actionMenu )
        actionMenu->UpdateAll();

    return 0;
}


int SELECTION_TOOL::AddItemToSel( const TOOL_EVENT& aEvent )
{
    AddItemToSel( aEvent.Parameter<EDA_ITEM*>() );
    selection().SetIsHover( false );
    return 0;
}


void SELECTION_TOOL::AddItemToSel( EDA_ITEM* aItem, bool aQuietMode )
{
    if( aItem )
    {
        select( aItem );

        // Inform other potentially interested tools
        if( !aQuietMode )
            m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
    }
}


int SELECTION_TOOL::ReselectItem( const TOOL_EVENT& aEvent )
{
    RemoveItemFromSel( aEvent.Parameter<EDA_ITEM*>() );
    selection().SetIsHover( false );

    AddItemToSel( aEvent.Parameter<EDA_ITEM*>() );
    selection().SetIsHover( false );

    return 0;
}


int SELECTION_TOOL::AddItemsToSel( const TOOL_EVENT& aEvent )
{
    AddItemsToSel( aEvent.Parameter<EDA_ITEMS*>(), false );
    selection().SetIsHover( false );
    return 0;
}


void SELECTION_TOOL::AddItemsToSel( EDA_ITEMS* aList, bool aQuietMode )
{
    if( aList )
    {
        for( EDA_ITEM* item : *aList )
            select( item );

        // Inform other potentially interested tools
        if( !aQuietMode )
            m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
    }
}


int SELECTION_TOOL::RemoveItemFromSel( const TOOL_EVENT& aEvent )
{
    RemoveItemFromSel( aEvent.Parameter<EDA_ITEM*>() );
    selection().SetIsHover( false );
    return 0;
}


void SELECTION_TOOL::RemoveItemFromSel( EDA_ITEM* aItem, bool aQuietMode )
{
    if( aItem )
    {
        unselect( aItem );

        // Inform other potentially interested tools
        if( !aQuietMode )
            m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );
    }
}


int SELECTION_TOOL::RemoveItemsFromSel( const TOOL_EVENT& aEvent )
{
    RemoveItemsFromSel( aEvent.Parameter<EDA_ITEMS*>(), false );
    selection().SetIsHover( false );
    return 0;
}


void SELECTION_TOOL::RemoveItemsFromSel( EDA_ITEMS* aList, bool aQuietMode )
{
    if( aList )
    {
        for( EDA_ITEM* item : *aList )
            unselect( item );

        // Inform other potentially interested tools
        if( !aQuietMode )
            m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );
    }
}


void SELECTION_TOOL::RemoveItemsFromSel( std::vector<KIID>* aList, bool aQuietMode )
{
    EDA_ITEMS removeItems;

    for( EDA_ITEM* item : selection() )
    {
        if( alg::contains( *aList, item->m_Uuid ) )
            removeItems.push_back( item );
    }

    RemoveItemsFromSel( &removeItems, aQuietMode );
}


void SELECTION_TOOL::BrightenItem( EDA_ITEM* aItem )
{
    highlight( aItem, BRIGHTENED );
}


void SELECTION_TOOL::UnbrightenItem( EDA_ITEM* aItem )
{
    unhighlight( aItem, BRIGHTENED );
}


void SELECTION_TOOL::onDisambiguationExpire( wxTimerEvent& aEvent )
{
    // If there is a multiple selection then it's more likely that we're seeing a paused drag
    // than a long-click.
    if( selection().GetSize() >= 2 && !hasModifier() )
        return;

    // If another tool has since started running then we don't want to interrupt
    if( !getEditFrame<EDA_DRAW_FRAME>()->ToolStackIsEmpty() )
        return;

    m_toolMgr->ProcessEvent( EVENTS::DisambiguatePoint );
}


int SELECTION_TOOL::SelectionMenu( const TOOL_EVENT& aEvent )
{
    COLLECTOR* collector = aEvent.Parameter<COLLECTOR*>();

    if( !doSelectionMenu( collector ) )
        collector->m_MenuCancelled = true;

    return 0;
}


bool SELECTION_TOOL::doSelectionMenu( COLLECTOR* aCollector )
{
    UNITS_PROVIDER* unitsProvider = getEditFrame<EDA_DRAW_FRAME>();
    EDA_ITEM*       current = nullptr;
    SELECTION       highlightGroup;
    bool            selectAll = false;
    bool            showMoreChoices = false;

    highlightGroup.SetLayer( LAYER_SELECT_OVERLAY );
    getView()->Add( &highlightGroup );

    do
    {
        /// This must be the second time through the loop, and the user has requested the full,
        /// non-limited list of selection items
        if( showMoreChoices )
        {
            aCollector->Combine();

            // prime event loop so we don't have to wait around for a mouse-moved
            m_toolMgr->PrimeTool( { 0, 0 } );

            showMoreChoices = false;
        }

        int         limit = std::min( 100, aCollector->GetCount() );
        ACTION_MENU menu( true );

        for( int i = 0; i < limit; ++i )
        {
            EDA_ITEM* item = ( *aCollector )[i];
            wxString  menuText;

            if( i < 9 )
            {
#ifdef __WXMAC__
                menuText = wxString::Format( "%s\t%d",
                                             item->GetItemDescription( unitsProvider, false ),
                                             i + 1 );
#else
                menuText = wxString::Format( "&%d  %s\t%d",
                                             i + 1,
                                             item->GetItemDescription( unitsProvider, false ),
                                             i + 1 );
#endif
            }
            else
            {
                menuText = item->GetItemDescription( unitsProvider, false );
            }

            menu.Add( menuText, i + 1, item->GetMenuImage() );
        }

        menu.AppendSeparator();
        menu.Add( _( "Select &All\tA" ), limit + 1, BITMAPS::INVALID_BITMAP );

        if( !showMoreChoices && aCollector->HasAdditionalItems() )
            menu.Add( _( "Show &More Choices...\tM" ), limit + 2, BITMAPS::INVALID_BITMAP );

        if( aCollector->m_MenuTitle.Length() )
        {
            menu.SetTitle( aCollector->m_MenuTitle );
            menu.SetIcon( BITMAPS::info );
            menu.DisplayTitle( true );
        }
        else
        {
            menu.DisplayTitle( false );
        }

        SetContextMenu( &menu, CMENU_NOW );

        while( TOOL_EVENT* evt = Wait() )
        {
            if( evt->Action() == TA_CHOICE_MENU_UPDATE )
            {
                if( selectAll )
                {
                    for( int i = 0; i < aCollector->GetCount(); ++i )
                        unhighlight( ( *aCollector )[i], BRIGHTENED, &highlightGroup );
                }
                else if( current )
                {
                    unhighlight( current, BRIGHTENED, &highlightGroup );
                }

                int id = *evt->GetCommandId();

                // User has pointed an item, so show it in a different way
                if( id > 0 && id <= limit )
                {
                    current = ( *aCollector )[id - 1];
                    highlight( current, BRIGHTENED, &highlightGroup );
                }
                else
                {
                    current = nullptr;
                }

                // User has pointed on the "Select All" option
                if( id == limit + 1 )
                {
                    for( int i = 0; i < aCollector->GetCount(); ++i )
                        highlight( ( *aCollector )[i], BRIGHTENED, &highlightGroup );

                    selectAll = true;
                }
                else
                {
                    selectAll = false;
                }
            }
            else if( evt->Action() == TA_CHOICE_MENU_CHOICE )
            {
                if( selectAll )
                {
                    for( int i = 0; i < aCollector->GetCount(); ++i )
                        unhighlight( ( *aCollector )[i], BRIGHTENED, &highlightGroup );
                }
                else if( current )
                {
                    unhighlight( current, BRIGHTENED, &highlightGroup );
                }

                std::optional<int> id = evt->GetCommandId();

                // User has selected the "Select All" option
                if( id == limit + 1 )
                {
                    selectAll = true;
                    current   = nullptr;
                }
                // User has selected the "Expand Selection" option
                else if( id == limit + 2 )
                {
                    selectAll       = false;
                    current         = nullptr;
                    showMoreChoices = true;
                }
                // User has selected an item, so this one will be returned
                else if( id && ( *id > 0 ) && ( *id <= limit ) )
                {
                    selectAll = false;
                    current   = ( *aCollector )[*id - 1];
                }
                // User has cancelled the menu (either by <esc> or clicking out of it)
                else
                {
                    selectAll = false;
                    current   = nullptr;
                }
            }
            else if( evt->Action() == TA_CHOICE_MENU_CLOSED )
            {
                break;
            }

            getView()->UpdateItems();
            getEditFrame<EDA_DRAW_FRAME>()->GetCanvas()->Refresh();
        }
    }
    while( showMoreChoices );

    getView()->Remove( &highlightGroup );

    if( selectAll )
    {
        return true;
    }
    else if( current )
    {
        aCollector->Empty();
        aCollector->Append( current );
        return true;
    }

    return false;
}
