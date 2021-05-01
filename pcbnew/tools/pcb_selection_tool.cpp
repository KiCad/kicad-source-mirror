/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * Copyright (C) 2018-2020 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <limits>
#include <functional>
using namespace std::placeholders;
#include <core/kicad_algo.h>
#include <board.h>
#include <board_item.h>
#include <clipper.hpp>
#include <track.h>
#include <footprint.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <pcb_marker.h>
#include <zone.h>
#include <collectors.h>
#include <dialog_find.h>
#include <dialog_filter_selection.h>
#include <dialogs/dialog_locked_items_query.h>
#include <class_draw_panel_gal.h>
#include <view/view_controls.h>
#include <preview_items/selection_area.h>
#include <painter.h>
#include <router/router_tool.h>
#include <bitmaps.h>
#include <pcbnew_settings.h>
#include <tool/tool_event.h>
#include <tool/tool_manager.h>
#include <connectivity/connectivity_data.h>
#include <footprint_viewer_frame.h>
#include <id.h>
#include "tool_event_utils.h"
#include "pcb_selection_tool.h"
#include "pcb_actions.h"


class SELECT_MENU : public ACTION_MENU
{
public:
    SELECT_MENU() :
        ACTION_MENU( true )
    {
        SetTitle( _( "Select" ) );

        Add( PCB_ACTIONS::filterSelection );

        AppendSeparator();

        Add( PCB_ACTIONS::selectConnection );
        Add( PCB_ACTIONS::selectNet );
        // This could be enabled if we have better logic for picking the target net with the mouse
        // Add( PCB_ACTIONS::deselectNet );
        Add( PCB_ACTIONS::selectSameSheet );
    }

private:
    ACTION_MENU* create() const override
    {
        return new SELECT_MENU();
    }
};


/**
 * Private implementation of firewalled private data
 */
class PCB_SELECTION_TOOL::PRIV
{
public:
    DIALOG_FILTER_SELECTION::OPTIONS m_filterOpts;
};


PCB_SELECTION_TOOL::PCB_SELECTION_TOOL() :
        PCB_TOOL_BASE( "pcbnew.InteractiveSelection" ),
        m_frame( NULL ),
        m_additive( false ),
        m_subtractive( false ),
        m_exclusive_or( false ),
        m_multiple( false ),
        m_skip_heuristics( false ),
        m_highlight_modifier( false ),
        m_nonModifiedCursor( KICURSOR::ARROW ),
        m_enteredGroup( nullptr ),
        m_priv( std::make_unique<PRIV>() )
{
    m_filter.lockedItems = false;
    m_filter.footprints  = true;
    m_filter.text        = true;
    m_filter.tracks      = true;
    m_filter.vias        = true;
    m_filter.pads        = true;
    m_filter.graphics    = true;
    m_filter.zones       = true;
    m_filter.keepouts    = true;
    m_filter.dimensions  = true;
    m_filter.otherItems  = true;
}


PCB_SELECTION_TOOL::~PCB_SELECTION_TOOL()
{
    getView()->Remove( &m_selection );
    getView()->Remove( &m_enteredGroupOverlay );
}


bool PCB_SELECTION_TOOL::Init()
{
    auto frame = getEditFrame<PCB_BASE_FRAME>();

    if( frame && ( frame->IsType( FRAME_FOOTPRINT_VIEWER )
                   || frame->IsType( FRAME_FOOTPRINT_VIEWER_MODAL ) ) )
    {
        frame->AddStandardSubMenus( m_menu );
        return true;
    }

    auto selectMenu = std::make_shared<SELECT_MENU>();
    selectMenu->SetTool( this );
    m_menu.AddSubMenu( selectMenu );

    auto& menu = m_menu.GetMenu();

    auto activeToolCondition =
            [ frame ] ( const SELECTION& aSel )
            {
                return !frame->ToolStackIsEmpty();
            };

    auto inGroupCondition =
            [this] ( const SELECTION& )
            {
                return m_enteredGroup != nullptr;
            };

    if( frame && frame->IsType( FRAME_PCB_EDITOR ) )
    {
        menu.AddMenu( selectMenu.get(), SELECTION_CONDITIONS::NotEmpty  );
        menu.AddSeparator( 1000 );
    }

    // "Cancel" goes at the top of the context menu when a tool is active
    menu.AddItem( ACTIONS::cancelInteractive, activeToolCondition, 1 );
    menu.AddItem( PCB_ACTIONS::groupLeave,    inGroupCondition,    1 );
    menu.AddSeparator( 1 );

    if( frame )
        frame->AddStandardSubMenus( m_menu );

    return true;
}


void PCB_SELECTION_TOOL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<PCB_BASE_FRAME>();

    if( m_enteredGroup )
        ExitGroup();

    if( aReason == TOOL_BASE::MODEL_RELOAD )
    {
        // Deselect any item being currently in edit, to avoid unexpected behavior
        // and remove pointers to the selected items from containers
        // without changing their properties (as they are already deleted
        // while a new board is loaded)
        ClearSelection( true );

        getView()->GetPainter()->GetSettings()->SetHighlight( false );
    }
    else
    {
        // Restore previous properties of selected items and remove them from containers
        ClearSelection( true );
    }

    // Reinsert the VIEW_GROUP, in case it was removed from the VIEW
    view()->Remove( &m_selection );
    view()->Add( &m_selection );

    view()->Remove( &m_enteredGroupOverlay );
    view()->Add( &m_enteredGroupOverlay );
}


void PCB_SELECTION_TOOL::setModifiersState( bool aShiftState, bool aCtrlState, bool aAltState )
{
    // Set the configuration of m_additive, m_subtractive, m_exclusive_or
    // from the state of modifier keys SHIFT, CTRL, ALT and the OS

    // on left click, a selection is made, depending on modifiers ALT, SHIFT, CTRL:
    // Due to the fact ALT key modifier cannot be useed freely on Winows and Linux,
    // actions are different on OSX and others OS
    // Especially, ALT key cannot be used to force showing the full selection choice
    // context menu (the menu is immediately closed on Windows )
    //
    // No modifier = select items and deselect previous selection
    // ALT (on OSX) = skip heuristic and show full selection choice
    // ALT (on others) = exclusive OR of selected items (inverse selection)
    //
    // CTRL (on OSX) = exclusive OR of selected items (inverse selection)
    // CTRL (on others) = skip heuristic and show full selection choice
    //
    // SHIFT = add selected items to the current selection
    //
    // CTRL+SHIFT (on OSX) = remove selected items to the current selection
    // CTRL+SHIFT (on others) = highlight net
    //
    // CTRL+ALT (on OSX) = highlight net
    // CTRL+ALT (on others) = do nothing (same as no modifier)
    //
    // SHIFT+ALT (on OSX) =  do nothing (same as no modifier)
    // SHIFT+ALT (on others) = remove selected items to the current selection

#ifdef __WXOSX_MAC__
    m_subtractive        = aCtrlState && aShiftState && !aAltState;
    m_additive           = aShiftState && !aCtrlState && !aAltState;
    m_exclusive_or       = aCtrlState && !aShiftState && !aAltState;
    m_skip_heuristics    = aAltState && !aShiftState && !aCtrlState;
    m_highlight_modifier = aCtrlState && aAltState && !aShiftState;

#else
    m_subtractive  = aShiftState && !aCtrlState && aAltState;
    m_additive     = aShiftState && !aCtrlState && !aAltState;
    m_exclusive_or = !aShiftState && !aCtrlState && aAltState;

    // Is the user requesting that the selection list include all possible
    // items without removing less likely selection candidates
    // Cannot use the Alt key on windows or the disambiguation context menu is immediately
    // dismissed rendering it useless.
    m_skip_heuristics = aCtrlState && !aShiftState && !aAltState;

    m_highlight_modifier = aCtrlState && aShiftState && !aAltState;
#endif
}


void PCB_SELECTION_TOOL::OnIdle( wxIdleEvent& aEvent )
{
    if( m_frame->ToolStackIsEmpty() && !m_multiple )
    {
        wxMouseState keyboardState = wxGetMouseState();

        setModifiersState( keyboardState.ShiftDown(), keyboardState.ControlDown(),
                           keyboardState.AltDown() );

        if( m_additive )
            m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ADD );
        else if( m_subtractive )
            m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::SUBTRACT );
        else if( m_exclusive_or )
            m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::XOR );
        else
            m_frame->GetCanvas()->SetCurrentCursor( m_nonModifiedCursor );
    }
}


int PCB_SELECTION_TOOL::Main( const TOOL_EVENT& aEvent )
{
    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        MOUSE_DRAG_ACTION dragAction = m_frame->GetDragAction();
        TRACK_DRAG_ACTION trackDragAction = m_frame->Settings().m_TrackDragAction;

        // on left click, a selection is made, depending on modifiers ALT, SHIFT, CTRL:
        setModifiersState( evt->Modifier( MD_SHIFT ), evt->Modifier( MD_CTRL ),
                           evt->Modifier( MD_ALT ) );

        bool modifier_enabled = m_subtractive || m_additive || m_exclusive_or;
        PCB_BASE_FRAME* frame = getEditFrame<PCB_BASE_FRAME>();
        bool brd_editor = frame && frame->IsType( FRAME_PCB_EDITOR );
        ROUTER_TOOL* router = m_toolMgr->GetTool<ROUTER_TOOL>();

        // If the router tool is active, don't override
        if( router && router->IsToolActive() )
        {
            evt->SetPassEvent();
        }
        // Single click? Select single object
        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( m_highlight_modifier && brd_editor )
                m_toolMgr->RunAction( PCB_ACTIONS::highlightNet, true );
            else
            {
                m_frame->FocusOnItem( nullptr );
                selectPoint( evt->Position() );
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Right click? if there is any object - show the context menu
            bool selectionCancelled = false;

            if( m_selection.Empty() )
            {
                selectPoint( evt->Position(), false, &selectionCancelled );
                m_selection.SetIsHover( true );
            }

            if( !selectionCancelled )
                m_menu.ShowContextMenu( m_selection );
        }
        else if( evt->IsDblClick( BUT_LEFT ) )
        {
            // Double click? Display the properties window
            m_frame->FocusOnItem( nullptr );

            if( m_selection.Empty() )
                selectPoint( evt->Position() );

            if( m_selection.GetSize() == 1 && m_selection[0]->Type() == PCB_GROUP_T )
            {
                EnterGroup();
            }
            else
            {
                m_toolMgr->RunAction( PCB_ACTIONS::properties, true );
            }
        }
        else if( evt->IsDblClick( BUT_MIDDLE ) )
        {
            // Middle double click?  Do zoom to fit or zoom to objects
            if( evt->Modifier( MD_CTRL ) ) // Is CTRL key down?
                m_toolMgr->RunAction( ACTIONS::zoomFitObjects, true );
            else
                m_toolMgr->RunAction( ACTIONS::zoomFitScreen, true );
        }
        else if( evt->IsDrag( BUT_LEFT ) )
        {
            // Is another tool already moving a new object?  Don't allow a drag start
            if( !m_selection.Empty() && m_selection[0]->HasFlag( IS_NEW | IS_MOVED ) )
            {
                evt->SetPassEvent();
                continue;
            }

            // Drag with LMB? Select multiple objects (or at least draw a selection box)
            // or drag them
            m_frame->FocusOnItem( nullptr );
            m_toolMgr->ProcessEvent( EVENTS::InhibitSelectionEditing );

            if( modifier_enabled || dragAction == MOUSE_DRAG_ACTION::SELECT )
            {
                selectMultiple();
            }
            else if( m_selection.Empty() && dragAction != MOUSE_DRAG_ACTION::DRAG_ANY )
            {
                selectMultiple();
            }
            else
            {
                // Don't allow starting a drag from a zone filled area that isn't already selected
                auto zoneFilledAreaFilter =
                        []( const VECTOR2I& aWhere, GENERAL_COLLECTOR& aCollector,
                            PCB_SELECTION_TOOL* aTool )
                        {
                            wxPoint location = wxPoint( aWhere );
                            int     accuracy = KiROUND( 5 * aCollector.GetGuide()->OnePixelInIU() );
                            std::set<EDA_ITEM*> remove;

                            for( EDA_ITEM* item : aCollector )
                            {
                                if( item->Type() == PCB_ZONE_T || item->Type() == PCB_FP_ZONE_T )
                                {
                                    ZONE* zone = static_cast<ZONE*>( item );

                                    if( !zone->HitTestForCorner( location, accuracy * 2 ) &&
                                        !zone->HitTestForEdge( location, accuracy ) )
                                        remove.insert( zone );
                                }
                            }

                            for( EDA_ITEM* item : remove )
                                aCollector.Remove( item );
                        };

                // Selection is empty? try to start dragging the item under the point where drag
                // started
                if( m_selection.Empty() && selectCursor( false, zoneFilledAreaFilter ) )
                    m_selection.SetIsHover( true );

                // Check if dragging has started within any of selected items bounding box.
                // We verify "HasPosition()" first to protect against edge case involving
                // moving off menus that causes problems (issue #5250)
                if( evt->HasPosition() && selectionContains( evt->Position() ) )
                {
                    // Yes -> run the move tool and wait till it finishes
                    TRACK* track = dynamic_cast<TRACK*>( m_selection.GetItem( 0 ) );

                    // If there is only item in the selection and it's a track, then we need to route it
                    bool doRouting = ( track && ( 1 == m_selection.GetSize() ) );

                    if( doRouting && trackDragAction == TRACK_DRAG_ACTION::DRAG )
                        m_toolMgr->RunAction( PCB_ACTIONS::drag45Degree, true );
                    else if( doRouting && trackDragAction == TRACK_DRAG_ACTION::DRAG_FREE_ANGLE )
                        m_toolMgr->RunAction( PCB_ACTIONS::dragFreeAngle, true );
                    else
                        m_toolMgr->RunAction( PCB_ACTIONS::move, true );
                }
                else
                {
                    // No -> drag a selection box
                    selectMultiple();
                }
            }
        }
        else if( evt->IsCancel() )
        {
            m_frame->FocusOnItem( nullptr );

            if( m_enteredGroup )
                ExitGroup();

            ClearSelection();

            if( evt->FirstResponder() == this )
                m_toolMgr->RunAction( PCB_ACTIONS::clearHighlight );
        }
        else
        {
            evt->SetPassEvent();
        }


        if( m_frame->ToolStackIsEmpty() )
        {
            // move cursor prediction
            if( !modifier_enabled
                    && dragAction == MOUSE_DRAG_ACTION::DRAG_SELECTED
                    && !m_selection.Empty()
                    && evt->HasPosition()
                    && selectionContains( evt->Position() ) )
            {
                m_nonModifiedCursor = KICURSOR::MOVING;
            }
            else
            {
                m_nonModifiedCursor = KICURSOR::ARROW;
            }
        }
    }

    // Shutting down; clear the selection
    m_selection.Clear();

    return 0;
}


void PCB_SELECTION_TOOL::EnterGroup()
{
    wxCHECK_RET( m_selection.GetSize() == 1 && m_selection[0]->Type() == PCB_GROUP_T,
                 "EnterGroup called when selection is not a single group" );
    PCB_GROUP* aGroup = static_cast<PCB_GROUP*>( m_selection[0] );

    if( m_enteredGroup != NULL )
        ExitGroup();

    ClearSelection();
    m_enteredGroup = aGroup;
    m_enteredGroup->SetFlags( ENTERED );
    m_enteredGroup->RunOnChildren( [&]( BOARD_ITEM* titem )
                                   {
                                       select( titem );
                                   } );

    m_enteredGroupOverlay.Add( m_enteredGroup );
}


void PCB_SELECTION_TOOL::ExitGroup( bool aSelectGroup )
{
    // Only continue if there is a group entered
    if( m_enteredGroup == nullptr )
        return;

    m_enteredGroup->ClearFlags( ENTERED );
    ClearSelection();

    if( aSelectGroup )
        select( m_enteredGroup );

    m_enteredGroupOverlay.Clear();
    m_enteredGroup = nullptr;
}


PCB_SELECTION& PCB_SELECTION_TOOL::GetSelection()
{
    return m_selection;
}


PCB_SELECTION& PCB_SELECTION_TOOL::RequestSelection( CLIENT_SELECTION_FILTER aClientFilter,
                                                     bool aConfirmLockedItems )
{
    bool selectionEmpty = m_selection.Empty();
    m_selection.SetIsHover( selectionEmpty );

    if( selectionEmpty )
    {
        m_toolMgr->RunAction( PCB_ACTIONS::selectionCursor, true, aClientFilter );
        m_selection.ClearReferencePoint();
    }

    if( aClientFilter )
    {
        enum DISPOSITION { BEFORE = 1, AFTER, BOTH };

        std::map<EDA_ITEM*, DISPOSITION> itemDispositions;
        GENERAL_COLLECTOR                collector;

        for( EDA_ITEM* item : m_selection )
        {
            collector.Append( item );
            itemDispositions[ item ] = BEFORE;
        }

        aClientFilter( VECTOR2I(), collector, this );

        for( EDA_ITEM* item : collector )
        {
            if( itemDispositions.count( item ) )
                itemDispositions[ item ] = BOTH;
            else
                itemDispositions[ item ] = AFTER;
        }

        // Unhighlight the BEFORE items before highlighting the AFTER items.
        // This is so that in the case of groups, if aClientFilter replaces a selection
        // with the enclosing group, the unhighlight of the element doesn't undo the
        // recursive highlighting of that elemetn by the group.

        for( std::pair<EDA_ITEM* const, DISPOSITION> itemDisposition : itemDispositions )
        {
            BOARD_ITEM* item = static_cast<BOARD_ITEM*>( itemDisposition.first );
            DISPOSITION disposition = itemDisposition.second;

            if( disposition == BEFORE )
            {
                unhighlight( item, SELECTED, &m_selection );
            }
        }

        for( std::pair<EDA_ITEM* const, DISPOSITION> itemDisposition : itemDispositions )
        {
            BOARD_ITEM* item = static_cast<BOARD_ITEM*>( itemDisposition.first );
            DISPOSITION disposition = itemDisposition.second;

            if( disposition == AFTER )
            {
                highlight( item, SELECTED, &m_selection );
            }
            else if( disposition == BOTH )
            {
                // nothing to do
            }
        }

        m_frame->GetCanvas()->ForceRefresh();
    }

    if( aConfirmLockedItems )
    {
        std::vector<BOARD_ITEM*> lockedItems;

        for( EDA_ITEM* item : m_selection )
        {
            BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( item );

            if( boardItem->Type() == PCB_GROUP_T )
            {
                PCB_GROUP* group = static_cast<PCB_GROUP*>( boardItem );
                bool       lockedDescendant = false;

                group->RunOnDescendants(
                        [&lockedDescendant]( BOARD_ITEM* child )
                        {
                            if( child->IsLocked() )
                                lockedDescendant = true;
                        } );

                if( lockedDescendant )
                    lockedItems.push_back( group );
            }
            else if( boardItem->IsLocked() )
            {
                lockedItems.push_back( boardItem );
            }
        }

        if( !lockedItems.empty() )
        {
            DIALOG_LOCKED_ITEMS_QUERY dlg( frame(), lockedItems.size() );

            switch( dlg.ShowModal() )
            {
            case wxID_OK:
                // remove locked items from selection
                for( BOARD_ITEM* item : lockedItems )
                    unselect( item );

                break;

            case wxID_CANCEL:
                // cancel operation
                ClearSelection();
                break;

            case wxID_APPLY:
                // continue with operation with current selection
                break;
            }
        }
    }

    return m_selection;
}


const GENERAL_COLLECTORS_GUIDE PCB_SELECTION_TOOL::getCollectorsGuide() const
{
    GENERAL_COLLECTORS_GUIDE guide( board()->GetVisibleLayers(),
                                    (PCB_LAYER_ID) view()->GetTopLayer(), view() );

    bool padsDisabled = !board()->IsElementVisible( LAYER_PADS );

    // account for the globals
    guide.SetIgnoreMTextsMarkedNoShow( ! board()->IsElementVisible( LAYER_MOD_TEXT_INVISIBLE ) );
    guide.SetIgnoreMTextsOnBack( ! board()->IsElementVisible( LAYER_MOD_TEXT_BK ) );
    guide.SetIgnoreMTextsOnFront( ! board()->IsElementVisible( LAYER_MOD_TEXT_FR ) );
    guide.SetIgnoreModulesOnBack( ! board()->IsElementVisible( LAYER_MOD_BK ) );
    guide.SetIgnoreModulesOnFront( ! board()->IsElementVisible( LAYER_MOD_FR ) );
    guide.SetIgnorePadsOnBack( padsDisabled || ! board()->IsElementVisible( LAYER_PAD_BK ) );
    guide.SetIgnorePadsOnFront( padsDisabled || ! board()->IsElementVisible( LAYER_PAD_FR ) );
    guide.SetIgnoreThroughHolePads( padsDisabled || ! board()->IsElementVisible( LAYER_PADS_TH ) );
    guide.SetIgnoreModulesVals( ! board()->IsElementVisible( LAYER_MOD_VALUES ) );
    guide.SetIgnoreModulesRefs( ! board()->IsElementVisible( LAYER_MOD_REFERENCES ) );
    guide.SetIgnoreThroughVias( ! board()->IsElementVisible( LAYER_VIAS ) );
    guide.SetIgnoreBlindBuriedVias( ! board()->IsElementVisible( LAYER_VIAS ) );
    guide.SetIgnoreMicroVias( ! board()->IsElementVisible( LAYER_VIAS ) );
    guide.SetIgnoreTracks( ! board()->IsElementVisible( LAYER_TRACKS ) );

    return guide;
}


bool PCB_SELECTION_TOOL::selectPoint( const VECTOR2I& aWhere, bool aOnDrag,
                                      bool* aSelectionCancelledFlag,
                                      CLIENT_SELECTION_FILTER aClientFilter )
{
    GENERAL_COLLECTORS_GUIDE   guide = getCollectorsGuide();
    GENERAL_COLLECTOR          collector;
    const PCB_DISPLAY_OPTIONS& displayOpts = m_frame->GetDisplayOptions();

    guide.SetIgnoreZoneFills( displayOpts.m_ZoneDisplayMode != ZONE_DISPLAY_MODE::SHOW_FILLED );

    if( m_enteredGroup && !m_enteredGroup->GetBoundingBox().Contains( (wxPoint) aWhere ) )
        ExitGroup();

    collector.Collect( board(), m_isFootprintEditor ? GENERAL_COLLECTOR::FootprintItems
                                                    : GENERAL_COLLECTOR::AllBoardItems,
                       (wxPoint) aWhere, guide );

    // Remove unselectable items
    for( int i = collector.GetCount() - 1; i >= 0; --i )
    {
        if( !Selectable( collector[ i ] ) || ( aOnDrag && collector[i]->IsLocked() ) )
            collector.Remove( i );
    }

    m_selection.ClearReferencePoint();

    // Allow the client to do tool- or action-specific filtering to see if we can get down
    // to a single item
    if( aClientFilter )
        aClientFilter( aWhere, collector, this );

    // Apply the stateful filter
    FilterCollectedItems( collector );

    FilterCollectorForHierarchy( collector, false );

    // Apply some ugly heuristics to avoid disambiguation menus whenever possible
    if( collector.GetCount() > 1 && !m_skip_heuristics )
        GuessSelectionCandidates( collector, aWhere );

    // If still more than one item we're going to have to ask the user.
    if( collector.GetCount() > 1 )
    {
        if( aOnDrag )
            Wait( TOOL_EVENT( TC_ANY, TA_MOUSE_UP, BUT_LEFT ) );

        if( !doSelectionMenu( &collector ) )
        {
            if( aSelectionCancelledFlag )
                *aSelectionCancelledFlag = true;

            return false;
        }
    }

    bool anyAdded      = false;
    bool anySubtracted = false;

    if( !m_additive && !m_subtractive && !m_exclusive_or )
    {
        if( m_selection.GetSize() > 0 )
        {
            ClearSelection( true /*quiet mode*/ );
            anySubtracted = true;
        }
    }

    if( collector.GetCount() > 0 )
    {
        for( int i = 0; i < collector.GetCount(); ++i )
        {
            if( m_subtractive || ( m_exclusive_or && collector[i]->IsSelected() ) )
            {
                unselect( collector[i] );
                anySubtracted = true;
            }
            else
            {
                select( collector[i] );
                anyAdded = true;
            }
        }
    }

    if( anyAdded )
    {
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
        return true;
    }
    else if( anySubtracted )
    {
        m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );
        return true;
    }

    return false;
}


bool PCB_SELECTION_TOOL::selectCursor( bool aForceSelect, CLIENT_SELECTION_FILTER aClientFilter )
{
    if( aForceSelect || m_selection.Empty() )
    {
        ClearSelection( true /*quiet mode*/ );
        selectPoint( getViewControls()->GetCursorPosition( false ), false, NULL, aClientFilter );
    }

    return !m_selection.Empty();
}


bool PCB_SELECTION_TOOL::selectMultiple()
{
    bool cancelled = false;     // Was the tool cancelled while it was running?
    m_multiple = true;          // Multiple selection mode is active
    KIGFX::VIEW* view = getView();

    KIGFX::PREVIEW::SELECTION_AREA area;
    view->Add( &area );

    bool anyAdded = false;
    bool anySubtracted = false;

    while( TOOL_EVENT* evt = Wait() )
    {
        int width = area.GetEnd().x - area.GetOrigin().x;

        /* Selection mode depends on direction of drag-selection:
             * Left > Right : Select objects that are fully enclosed by selection
             * Right > Left : Select objects that are crossed by selection
             */
        bool windowSelection = width >= 0 ? true : false;

        if( view->IsMirroredX() )
            windowSelection = !windowSelection;

        m_frame->GetCanvas()->SetCurrentCursor( windowSelection ? KICURSOR::SELECT_WINDOW
                                                                : KICURSOR::SELECT_LASSO );

        if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            cancelled = true;
            break;
        }

        if( evt->IsDrag( BUT_LEFT ) )
        {
            if( !m_additive && !m_subtractive && !m_exclusive_or )
            {
                if( m_selection.GetSize() > 0 )
                {
                    anySubtracted = true;
                    ClearSelection( true /*quiet mode*/ );
                }
            }

            // Start drawing a selection box
            area.SetOrigin( evt->DragOrigin() );
            area.SetEnd( evt->Position() );
            area.SetAdditive( m_additive );
            area.SetSubtractive( m_subtractive );
            area.SetExclusiveOr( m_exclusive_or );

            view->SetVisible( &area, true );
            view->Update( &area );
            getViewControls()->SetAutoPan( true );
        }

        if( evt->IsMouseUp( BUT_LEFT ) )
        {
            getViewControls()->SetAutoPan( false );

            // End drawing the selection box
            view->SetVisible( &area, false );

            std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> candidates;
            BOX2I selectionBox = area.ViewBBox();
            view->Query( selectionBox, candidates );    // Get the list of nearby items

            int height = area.GetEnd().y - area.GetOrigin().y;

            // Construct an EDA_RECT to determine BOARD_ITEM selection
            EDA_RECT selectionRect( (wxPoint) area.GetOrigin(), wxSize( width, height ) );

            selectionRect.Normalize();

            GENERAL_COLLECTOR collector;

            for( auto it = candidates.begin(), it_end = candidates.end(); it != it_end; ++it )
            {
                BOARD_ITEM* item = static_cast<BOARD_ITEM*>( it->first );

                if( item && Selectable( item ) && item->HitTest( selectionRect, windowSelection ) )
                    collector.Append( item );
            }

            // Apply the stateful filter
            FilterCollectedItems( collector );

            FilterCollectorForHierarchy( collector, true );

            for( EDA_ITEM* i : collector )
            {
                BOARD_ITEM* item = static_cast<BOARD_ITEM*>( i );

                if( m_subtractive || ( m_exclusive_or && item->IsSelected() ) )
                {
                    unselect( item );
                    anySubtracted = true;
                }
                else
                {
                    select( item );
                    anyAdded = true;
                }
            }

            m_selection.SetIsHover( false );

            // Inform other potentially interested tools
            if( anyAdded )
                m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
            else if( anySubtracted )
                m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );

            break;  // Stop waiting for events
        }
    }

    getViewControls()->SetAutoPan( false );

    // Stop drawing the selection box
    view->Remove( &area );
    m_multiple = false;         // Multiple selection mode is inactive

    if( !cancelled )
        m_selection.ClearReferencePoint();

    m_toolMgr->ProcessEvent( EVENTS::UninhibitSelectionEditing );

    return cancelled;
}


int PCB_SELECTION_TOOL::CursorSelection( const TOOL_EVENT& aEvent )
{
    CLIENT_SELECTION_FILTER aClientFilter = aEvent.Parameter<CLIENT_SELECTION_FILTER>();

    selectCursor( false, aClientFilter );

    return 0;
}


int PCB_SELECTION_TOOL::ClearSelection( const TOOL_EVENT& aEvent )
{
    ClearSelection();

    return 0;
}


int PCB_SELECTION_TOOL::SelectItems( const TOOL_EVENT& aEvent )
{
    std::vector<BOARD_ITEM*>* items = aEvent.Parameter<std::vector<BOARD_ITEM*>*>();

    if( items )
    {
        // Perform individual selection of each item before processing the event.
        for( BOARD_ITEM* item : *items )
            select( item );

        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
    }

    return 0;
}


int PCB_SELECTION_TOOL::SelectItem( const TOOL_EVENT& aEvent )
{
    AddItemToSel( aEvent.Parameter<BOARD_ITEM*>() );
    return 0;
}


int PCB_SELECTION_TOOL::SelectAll( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW* view = getView();

    // hold all visible items
    std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> selectedItems;

    // Filter the view items based on the selection box
    BOX2I selectionBox;

    // Intermediate step to allow filtering against hierarchy
    GENERAL_COLLECTOR collection;

    selectionBox.SetMaximum();
    view->Query( selectionBox, selectedItems );         // Get the list of selected items

    for( const KIGFX::VIEW::LAYER_ITEM_PAIR& item_pair : selectedItems )
    {
        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( item_pair.first );

        if( !item || !Selectable( item ) || !itemPassesFilter( item ) )
            continue;

        collection.Append( item );
    }

    FilterCollectorForHierarchy( collection, true );

    for( EDA_ITEM* item : collection )
        select( static_cast<BOARD_ITEM*>( item ) );

    m_frame->GetCanvas()->ForceRefresh();

    return 0;
}


void PCB_SELECTION_TOOL::AddItemToSel( BOARD_ITEM* aItem, bool aQuietMode )
{
    if( aItem )
    {
        select( aItem );

        // Inform other potentially interested tools
        if( !aQuietMode )
            m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
    }
}


int PCB_SELECTION_TOOL::UnselectItems( const TOOL_EVENT& aEvent )
{
    std::vector<BOARD_ITEM*>* items = aEvent.Parameter<std::vector<BOARD_ITEM*>*>();

    if( items )
    {
        // Perform individual unselection of each item before processing the event
        for( auto item : *items )
            unselect( item );

        m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );
    }

    return 0;
}


int PCB_SELECTION_TOOL::UnselectItem( const TOOL_EVENT& aEvent )
{
    RemoveItemFromSel( aEvent.Parameter<BOARD_ITEM*>() );
    return 0;
}


void PCB_SELECTION_TOOL::RemoveItemFromSel( BOARD_ITEM* aItem, bool aQuietMode )
{
    if( aItem )
    {
        unselect( aItem );

        // Inform other potentially interested tools
        m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );
    }
}


void PCB_SELECTION_TOOL::BrightenItem( BOARD_ITEM* aItem )
{
    highlight( aItem, BRIGHTENED );
}


void PCB_SELECTION_TOOL::UnbrightenItem( BOARD_ITEM* aItem )
{
    unhighlight( aItem, BRIGHTENED );
}


void connectedItemFilter( const VECTOR2I&, GENERAL_COLLECTOR& aCollector,
                          PCB_SELECTION_TOOL* sTool )
{
    // Narrow the collection down to a single BOARD_CONNECTED_ITEM for each represented net.
    // All other items types are removed.
    std::set<int> representedNets;

    for( int i = aCollector.GetCount() - 1; i >= 0; i-- )
    {
        BOARD_CONNECTED_ITEM* item = dynamic_cast<BOARD_CONNECTED_ITEM*>( aCollector[i] );
        if( !item )
            aCollector.Remove( i );
        else if ( representedNets.count( item->GetNetCode() ) )
            aCollector.Remove( i );
        else
            representedNets.insert( item->GetNetCode() );
    }
}


int PCB_SELECTION_TOOL::expandConnection( const TOOL_EVENT& aEvent )
{
    unsigned initialCount = 0;

    for( auto item : m_selection.GetItems() )
    {
        if( dynamic_cast<BOARD_CONNECTED_ITEM*>( item ) )
            initialCount++;
    }

    if( initialCount == 0 )
        selectCursor( true, connectedItemFilter );

    for( STOP_CONDITION stopCondition : { STOP_AT_JUNCTION, STOP_AT_PAD, STOP_NEVER } )
    {
        // copy the selection, since we're going to iterate and modify
        std::deque<EDA_ITEM*> selectedItems = m_selection.GetItems();

        for( EDA_ITEM* item : selectedItems )
            item->ClearTempFlags();

        for( EDA_ITEM* item : selectedItems )
        {
            TRACK* trackItem = dynamic_cast<TRACK*>( item );

            // Track items marked SKIP_STRUCT have already been visited
            if( trackItem && !( trackItem->GetFlags() & SKIP_STRUCT ) )
                selectConnectedTracks( *trackItem, stopCondition );
        }

        if( m_selection.GetItems().size() > initialCount )
            break;
    }

    // Inform other potentially interested tools
    if( m_selection.Size() > 0 )
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    return 0;
}


void PCB_SELECTION_TOOL::selectConnectedTracks( BOARD_CONNECTED_ITEM& aStartItem,
                                                STOP_CONDITION aStopCondition )
{
    constexpr KICAD_T types[] = { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T, PCB_PAD_T, EOT };

    auto connectivity = board()->GetConnectivity();
    auto connectedItems = connectivity->GetConnectedItems( &aStartItem, types, true );

    std::map<wxPoint, std::vector<TRACK*>> trackMap;
    std::map<wxPoint, VIA*>                viaMap;
    std::map<wxPoint, PAD*>                padMap;

    // Build maps of connected items
    for( BOARD_CONNECTED_ITEM* item : connectedItems )
    {
        switch( item->Type() )
        {
        case PCB_ARC_T:
        case PCB_TRACE_T:
        {
            TRACK* track = static_cast<TRACK*>( item );
            trackMap[ track->GetStart() ].push_back( track );
            trackMap[ track->GetEnd() ].push_back( track );
        }
            break;

        case PCB_VIA_T:
        {
            VIA* via = static_cast<VIA*>( item );
            viaMap[ via->GetStart() ] = via;
        }
            break;

        case PCB_PAD_T:
        {
            PAD* pad = static_cast<PAD*>( item );
            padMap[ pad->GetPosition() ] = pad;
        }
            break;

        default:
            break;
        }

        item->SetState( SKIP_STRUCT, false );
    }

    std::vector<wxPoint> activePts;

    // Set up the initial active points
    switch( aStartItem.Type() )
    {
    case PCB_ARC_T:
    case PCB_TRACE_T:
        activePts.push_back( static_cast<TRACK*>( &aStartItem )->GetStart() );
        activePts.push_back( static_cast<TRACK*>( &aStartItem )->GetEnd() );
        break;

    case PCB_VIA_T:
        activePts.push_back( static_cast<TRACK*>( &aStartItem )->GetStart() );
        break;

    case PCB_PAD_T:
        activePts.push_back( aStartItem.GetPosition() );
        break;

    default:
        break;
    }

    bool expand = true;

    // Iterative push from all active points
    while( expand )
    {
        expand = false;

        for( int i = activePts.size() - 1; i >= 0; --i )
        {
            wxPoint pt = activePts[i];
            size_t  pt_count = trackMap[ pt ].size() + viaMap.count( pt );

            if( pt_count > 2 && aStopCondition == STOP_AT_JUNCTION )
            {
                activePts.erase( activePts.begin() + i );
                continue;
            }

            if( padMap.count( pt ) && aStopCondition != STOP_NEVER )
            {
                activePts.erase( activePts.begin() + i );
                continue;
            }

            for( TRACK* track : trackMap[ pt ] )
            {
                if( track->GetState( SKIP_STRUCT ) )
                    continue;

                track->SetState( SKIP_STRUCT, true );
                select( track );

                if( track->GetStart() == pt )
                    activePts.push_back( track->GetEnd() );
                else
                    activePts.push_back( track->GetStart() );

                expand = true;
            }

            if( viaMap.count( pt ) && !viaMap[ pt ]->IsSelected() && aStopCondition != STOP_AT_JUNCTION )
                select( viaMap[ pt ] );

            activePts.erase( activePts.begin() + i );
        }
    }
}


void PCB_SELECTION_TOOL::selectAllItemsOnNet( int aNetCode, bool aSelect )
{
    constexpr KICAD_T types[] = { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T, EOT };
    auto connectivity = board()->GetConnectivity();

    for( BOARD_CONNECTED_ITEM* item : connectivity->GetNetItems( aNetCode, types ) )
        if( itemPassesFilter( item ) )
            aSelect ? select( item ) : unselect( item );
}


int PCB_SELECTION_TOOL::selectNet( const TOOL_EVENT& aEvent )
{
    bool select = aEvent.IsAction( &PCB_ACTIONS::selectNet );

    // If we've been passed an argument, just select that netcode1
    int netcode = aEvent.Parameter<intptr_t>();

    if( netcode > 0 )
    {
        selectAllItemsOnNet( netcode, select );
        return 0;
    }

    if( !selectCursor() )
        return 0;

    // copy the selection, since we're going to iterate and modify
    auto selection = m_selection.GetItems();

    for( EDA_ITEM* i : selection )
    {
        BOARD_CONNECTED_ITEM* connItem = dynamic_cast<BOARD_CONNECTED_ITEM*>( i );

        if( connItem )
            selectAllItemsOnNet( connItem->GetNetCode(), select );
    }

    // Inform other potentially interested tools
    if( m_selection.Size() > 0 )
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    return 0;
}


void PCB_SELECTION_TOOL::selectAllItemsOnSheet( wxString& aSheetPath )
{
    std::list<FOOTPRINT*> footprintList;

    // store all footprints that are on that sheet path
    for( FOOTPRINT* footprint : board()->Footprints() )
    {
        if( footprint == nullptr )
            continue;

        wxString footprint_path = footprint->GetPath().AsString().BeforeLast('/');

        if( aSheetPath.IsEmpty() )
            aSheetPath += '/';

        if( footprint_path == aSheetPath )
            footprintList.push_back( footprint );
    }

    //Generate a list of all pads, and of all nets they belong to.
    std::list<int>  netcodeList;
    std::list<PAD*> padList;

    for( FOOTPRINT* footprint : footprintList )
    {
        for( PAD* pad : footprint->Pads() )
        {
            if( pad->IsConnected() )
            {
                netcodeList.push_back( pad->GetNetCode() );
                padList.push_back( pad );
            }
        }
    }
    // remove all duplicates
    netcodeList.sort();
    netcodeList.unique();

    for( PAD* pad : padList )
        selectConnectedTracks( *pad, STOP_NEVER );

    // now we need to find all footprints that are connected to each of these nets then we need
    // to determine if these footprints are in the list of footprints belonging to this sheet
    std::list<int> removeCodeList;
    constexpr KICAD_T padType[] = { PCB_PAD_T, EOT };

    for( int netCode : netcodeList )
    {
        for( BOARD_CONNECTED_ITEM* mitem : board()->GetConnectivity()->GetNetItems( netCode, padType ) )
        {
            if( mitem->Type() == PCB_PAD_T && !alg::contains( footprintList, mitem->GetParent() ) )
            {
                // if we cannot find the footprint of the pad in the footprintList then we can
                // assume that that footprint is not located in the same schematic, therefore
                // invalidate this netcode.
                removeCodeList.push_back( netCode );
                break;
            }
        }
    }

    // remove all duplicates
    removeCodeList.sort();
    removeCodeList.unique();

    for( int removeCode : removeCodeList )
    {
        netcodeList.remove( removeCode );
    }

    std::list<BOARD_CONNECTED_ITEM*> localConnectionList;
    constexpr KICAD_T trackViaType[] = { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T, EOT };

    for( int netCode : netcodeList )
    {
        for( BOARD_CONNECTED_ITEM* item : board()->GetConnectivity()->GetNetItems( netCode, trackViaType ) )
            localConnectionList.push_back( item );
    }

    for( BOARD_ITEM* i : footprintList )
    {
        if( i != NULL )
            select( i );
    }

    for( BOARD_CONNECTED_ITEM* i : localConnectionList )
    {
        if( i != NULL )
            select( i );
    }
}


void PCB_SELECTION_TOOL::zoomFitSelection()
{
    //Should recalculate the view to zoom in on the selection
    auto selectionBox = m_selection.GetBoundingBox();
    auto view = getView();

    VECTOR2D screenSize = view->ToWorld( m_frame->GetCanvas()->GetClientSize(), false );
    screenSize.x = std::max( 10.0, screenSize.x );
    screenSize.y = std::max( 10.0, screenSize.y );

    if( selectionBox.GetWidth() != 0  || selectionBox.GetHeight() != 0 )
    {
        VECTOR2D vsize = selectionBox.GetSize();
        double scale = view->GetScale() / std::max( fabs( vsize.x / screenSize.x ),
                fabs( vsize.y / screenSize.y ) );
        view->SetScale( scale );
        view->SetCenter( selectionBox.Centre() );
        view->Add( &m_selection );
    }

    m_frame->GetCanvas()->ForceRefresh();
}


int PCB_SELECTION_TOOL::selectSheetContents( const TOOL_EVENT& aEvent )
{
    ClearSelection( true /*quiet mode*/ );
    wxString sheetPath = *aEvent.Parameter<wxString*>();

    selectAllItemsOnSheet( sheetPath );

    zoomFitSelection();

    if( m_selection.Size() > 0 )
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    return 0;
}


int PCB_SELECTION_TOOL::selectSameSheet( const TOOL_EVENT& aEvent )
{
    if( !selectCursor( true ) )
        return 0;

    // this function currently only supports footprints since they are only
    // on one sheet.
    auto item = m_selection.Front();

    if( !item )
        return 0;

    if( item->Type() != PCB_FOOTPRINT_T )
        return 0;

    FOOTPRINT* footprint = dynamic_cast<FOOTPRINT*>( item );

    if( footprint->GetPath().empty() )
        return 0;

    ClearSelection( true /*quiet mode*/ );

    // get the sheet path only.
    wxString sheetPath = footprint->GetPath().AsString().BeforeLast( '/' );

    if( sheetPath.IsEmpty() )
        sheetPath += '/';

    selectAllItemsOnSheet( sheetPath );

    // Inform other potentially interested tools
    if( m_selection.Size() > 0 )
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    return 0;
}


void PCB_SELECTION_TOOL::findCallback( BOARD_ITEM* aItem )
{
    bool cleared = false;

    if( m_selection.GetSize() > 0 )
    {
        // Don't fire an event now; most of the time it will be redundant as we're about to
        // fire a SelectedEvent.
        cleared = true;
        ClearSelection( true /*quiet mode*/ );
    }

    if( aItem )
    {
        select( aItem );
        m_frame->FocusOnLocation( aItem->GetPosition() );

        // Inform other potentially interested tools
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
    }
    else if( cleared )
    {
        m_toolMgr->ProcessEvent( EVENTS::ClearedEvent );
    }

    m_frame->GetCanvas()->ForceRefresh();
}


int PCB_SELECTION_TOOL::find( const TOOL_EVENT& aEvent )
{
    DIALOG_FIND dlg( m_frame );
    dlg.SetCallback( std::bind( &PCB_SELECTION_TOOL::findCallback, this, _1 ) );
    dlg.ShowModal();

    return 0;
}


/**
 * Function itemIsIncludedByFilter()
 *
 * Determine if an item is included by the filter specified
 *
 * @return true if aItem should be selected by this filter (i..e not filtered out)
 */
static bool itemIsIncludedByFilter( const BOARD_ITEM& aItem, const BOARD& aBoard,
                                    const DIALOG_FILTER_SELECTION::OPTIONS& aFilterOptions )
{
    bool include = true;
    const PCB_LAYER_ID layer = aItem.GetLayer();

    // if the item needs to be checked against the options
    if( include )
    {
        switch( aItem.Type() )
        {
        case PCB_FOOTPRINT_T:
        {
            const FOOTPRINT& footprint = static_cast<const FOOTPRINT&>( aItem );

            include = aFilterOptions.includeModules;

            if( include && !aFilterOptions.includeLockedModules )
                include = !footprint.IsLocked();

            break;
        }
        case PCB_TRACE_T:
        case PCB_ARC_T:
            include = aFilterOptions.includeTracks;
            break;

        case PCB_VIA_T:
            include = aFilterOptions.includeVias;
            break;

        case PCB_FP_ZONE_T:
        case PCB_ZONE_T:
            include = aFilterOptions.includeZones;
            break;

        case PCB_SHAPE_T:
        case PCB_TARGET_T:
        case PCB_DIM_ALIGNED_T:
        case PCB_DIM_CENTER_T:
        case PCB_DIM_ORTHOGONAL_T:
        case PCB_DIM_LEADER_T:
            if( layer == Edge_Cuts )
                include = aFilterOptions.includeBoardOutlineLayer;
            else
                include = aFilterOptions.includeItemsOnTechLayers;
            break;

        case PCB_FP_TEXT_T:
        case PCB_TEXT_T:
            include = aFilterOptions.includePcbTexts;
            break;

        default:
            // no filtering, just select it
            break;
        }
    }

    return include;
}


int PCB_SELECTION_TOOL::filterSelection( const TOOL_EVENT& aEvent )
{
    const BOARD&                      board = *getModel<BOARD>();
    DIALOG_FILTER_SELECTION::OPTIONS& opts = m_priv->m_filterOpts;
    DIALOG_FILTER_SELECTION           dlg( m_frame, opts );

    const int cmd = dlg.ShowModal();

    if( cmd != wxID_OK )
        return 0;

    // copy current selection
    std::deque<EDA_ITEM*> selection = m_selection.GetItems();

    ClearSelection( true /*quiet mode*/ );

    // re-select items from the saved selection according to the dialog options
    for( EDA_ITEM* i : selection )
    {
        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( i );
        bool        include = itemIsIncludedByFilter( *item, board, opts );

        if( include )
            select( item );
    }

    m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    return 0;
}


void PCB_SELECTION_TOOL::FilterCollectedItems( GENERAL_COLLECTOR& aCollector )
{
    if( aCollector.GetCount() == 0 )
        return;

    std::set<BOARD_ITEM*> rejected;

    for( EDA_ITEM* i : aCollector )
    {
        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( i );

        if( !itemPassesFilter( item ) )
            rejected.insert( item );
    }

    for( BOARD_ITEM* item : rejected )
        aCollector.Remove( item );
}


bool PCB_SELECTION_TOOL::itemPassesFilter( BOARD_ITEM* aItem )
{
    if( aItem->IsLocked() && !m_filter.lockedItems && aItem->Type() != PCB_PAD_T )
        return false;

    switch( aItem->Type() )
    {
    case PCB_FOOTPRINT_T:
        if( !m_filter.footprints )
            return false;

        break;

    case PCB_PAD_T:
        if( !m_filter.pads )
            return false;

        break;

    case PCB_TRACE_T:
    case PCB_ARC_T:
        if( !m_filter.tracks )
            return false;

        break;

    case PCB_VIA_T:
        if( !m_filter.vias )
            return false;

        break;

    case PCB_FP_ZONE_T:
    case PCB_ZONE_T:
    {
        ZONE* zone = static_cast<ZONE*>( aItem );

        if( ( !m_filter.zones && !zone->GetIsRuleArea() )
            || ( !m_filter.keepouts && zone->GetIsRuleArea() ) )
        {
            return false;
        }
    }
        break;

    case PCB_FP_SHAPE_T:
    case PCB_SHAPE_T:
    case PCB_TARGET_T:
        if( !m_filter.graphics )
            return false;

        break;

    case PCB_FP_TEXT_T:
    case PCB_TEXT_T:
        if( !m_filter.text )
            return false;

        break;

    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_ORTHOGONAL_T:
    case PCB_DIM_LEADER_T:
        if( !m_filter.dimensions )
            return false;

        break;

    default:
        if( !m_filter.otherItems )
            return false;
    }

    return true;
}


void PCB_SELECTION_TOOL::ClearSelection( bool aQuietMode )
{
    if( m_selection.Empty() )
        return;

    while( m_selection.GetSize() )
        unhighlight( static_cast<BOARD_ITEM*>( m_selection.Front() ), SELECTED, &m_selection );

    view()->Update( &m_selection );

    m_selection.SetIsHover( false );
    m_selection.ClearReferencePoint();

    // Inform other potentially interested tools
    if( !aQuietMode )
    {
        m_toolMgr->ProcessEvent( EVENTS::ClearedEvent );
        m_toolMgr->RunAction( PCB_ACTIONS::hideDynamicRatsnest, true );
    }
}


void PCB_SELECTION_TOOL::RebuildSelection()
{
    m_selection.Clear();

    bool enteredGroupFound = false;

    INSPECTOR_FUNC inspector =
            [&]( EDA_ITEM* item, void* testData )
            {
                if( item->IsSelected() )
                {
                    EDA_ITEM* parent = item->GetParent();

                    // Let selected parents handle their children.
                    if( parent && parent->IsSelected() )
                        return SEARCH_RESULT::CONTINUE;

                    highlight( (BOARD_ITEM*) item, SELECTED, &m_selection );
                }

                if( item == m_enteredGroup )
                {
                    item->SetFlags( ENTERED );
                    enteredGroupFound = true;
                }
                else
                {
                    item->ClearFlags( ENTERED );
                }

                return SEARCH_RESULT::CONTINUE;
            };

    board()->Visit( inspector, nullptr, m_isFootprintEditor ? GENERAL_COLLECTOR::FootprintItems
                                                            : GENERAL_COLLECTOR::AllBoardItems );

    if( !enteredGroupFound )
    {
        m_enteredGroupOverlay.Clear();
        m_enteredGroup = nullptr;
    }
}


int PCB_SELECTION_TOOL::SelectionMenu( const TOOL_EVENT& aEvent )
{
    GENERAL_COLLECTOR* collector = aEvent.Parameter<GENERAL_COLLECTOR*>();

    doSelectionMenu( collector );

    return 0;
}


bool PCB_SELECTION_TOOL::doSelectionMenu( GENERAL_COLLECTOR* aCollector )
{
    BOARD_ITEM*   current = nullptr;
    PCB_SELECTION highlightGroup;
    bool          selectAll = false;
    bool          expandSelection = false;

    highlightGroup.SetLayer( LAYER_SELECT_OVERLAY );
    getView()->Add( &highlightGroup );

    do
    {
        /// The user has requested the full, non-limited list of selection items
        if( expandSelection )
            aCollector->Combine();

        expandSelection = false;

        int         limit = std::min( 9, aCollector->GetCount() );
        ACTION_MENU menu( true );

        for( int i = 0; i < limit; ++i )
        {
            wxString    text;
            BOARD_ITEM* item = ( *aCollector )[i];
            text             = item->GetSelectMenuText( m_frame->GetUserUnits() );

            wxString menuText = wxString::Format( "&%d. %s\t%d", i + 1, text, i + 1 );
            menu.Add( menuText, i + 1, item->GetMenuImage() );
        }

        menu.AppendSeparator();
        menu.Add( _( "Select &All\tA" ), limit + 1, BITMAPS::INVALID_BITMAP );

        if( !expandSelection && aCollector->HasAdditionalItems() )
            menu.Add( _( "&Expand Selection\tE" ), limit + 2, BITMAPS::INVALID_BITMAP );

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
                    unhighlight( current, BRIGHTENED, &highlightGroup );

                int id = *evt->GetCommandId();

                // User has pointed an item, so show it in a different way
                if( id > 0 && id <= limit )
                {
                    current = ( *aCollector )[id - 1];
                    highlight( current, BRIGHTENED, &highlightGroup );
                }
                else
                    current = nullptr;

                // User has pointed on the "Select All" option
                if( id == limit + 1 )
                {
                    for( int i = 0; i < aCollector->GetCount(); ++i )
                        highlight( ( *aCollector )[i], BRIGHTENED, &highlightGroup );
                    selectAll = true;
                }
                else
                    selectAll = false;
            }
            else if( evt->Action() == TA_CHOICE_MENU_CHOICE )
            {
                if( selectAll )
                {
                    for( int i = 0; i < aCollector->GetCount(); ++i )
                        unhighlight( ( *aCollector )[i], BRIGHTENED, &highlightGroup );
                }
                else if( current )
                    unhighlight( current, BRIGHTENED, &highlightGroup );

                OPT<int> id = evt->GetCommandId();

                // User has selected the "Select All" option
                if( id == limit + 1 )
                {
                    selectAll = true;
                    current   = nullptr;
                }
                else if( id == limit + 2 )
                {
                    expandSelection = true;
                    selectAll       = false;
                    current         = nullptr;
                }
                // User has selected an item, so this one will be returned
                else if( id && ( *id > 0 ) && ( *id <= limit ) )
                {
                    selectAll = false;
                    current   = ( *aCollector )[*id - 1];
                }
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
        }
    } while( expandSelection );

    getView()->Remove( &highlightGroup );

    if( selectAll )
        return true;
    else if( current )
    {
        aCollector->Empty();
        aCollector->Append( current );
        return true;
    }

    return false;
}


bool PCB_SELECTION_TOOL::Selectable( const BOARD_ITEM* aItem, bool checkVisibilityOnly ) const
{
    const RENDER_SETTINGS* settings = getView()->GetPainter()->GetSettings();

    if( settings->GetHighContrast() )
    {
        std::set<unsigned int> activeLayers = settings->GetHighContrastLayers();
        bool                   onActiveLayer = false;

        for( unsigned int layer : activeLayers )
        {
            // NOTE: Only checking the regular layers (not GAL meta-layers)
            if( layer < PCB_LAYER_ID_COUNT && aItem->IsOnLayer( ToLAYER_ID( layer ) ) )
            {
                onActiveLayer = true;
                break;
            }
        }

        if( !onActiveLayer ) // We do not want to select items that are in the background
            return false;
    }

    if( aItem->Type() == PCB_FOOTPRINT_T )
    {
        // In footprint editor, we do not want to select the footprint itself.
        if( m_isFootprintEditor )
            return false;

        // Allow selection of footprints if some part of the footprint is visible.

        const FOOTPRINT* footprint = static_cast<const FOOTPRINT*>( aItem );

        for( const BOARD_ITEM* item : footprint->GraphicalItems() )
        {
            if( Selectable( item, true ) )
                return true;
        }

        for( const PAD* pad : footprint->Pads() )
        {
            if( Selectable( pad, true ) )
                return true;
        }

        for( const ZONE* zone : footprint->Zones() )
        {
            if( Selectable( zone, true ) )
                return true;
        }

        return false;
    }
    else if( aItem->Type() == PCB_GROUP_T )
    {
        PCB_GROUP* group = const_cast<PCB_GROUP*>( static_cast<const PCB_GROUP*>( aItem ) );

        // Similar to logic for footprint, a group is selectable if any of its members are.
        // (This recurses.)
        for( BOARD_ITEM* item : group->GetItems() )
        {
            if( Selectable( item, true ) )
                return true;
        }

        return false;
    }

    const ZONE* zone = nullptr;
    const VIA*  via = nullptr;
    const PAD*  pad = nullptr;

    switch( aItem->Type() )
    {
    case PCB_ZONE_T:
    case PCB_FP_ZONE_T:
        if( !board()->IsElementVisible( LAYER_ZONES ) )
            return false;

        zone = static_cast<const ZONE*>( aItem );

        // A footprint zone is only selectable within the footprint editor
        if( zone->GetParent()
                && zone->GetParent()->Type() == PCB_FOOTPRINT_T
                && !m_isFootprintEditor
                && !checkVisibilityOnly )
        {
            return false;
        }

        // zones can exist on multiple layers!
        if( !( zone->GetLayerSet() & board()->GetVisibleLayers() ).any() )
            return false;

        break;

    case PCB_TRACE_T:
    case PCB_ARC_T:
        if( !board()->IsElementVisible( LAYER_TRACKS ) )
            return false;

        if( m_isFootprintEditor )
        {
            if( !view()->IsLayerVisible( aItem->GetLayer() ) )
                return false;
        }
        else
        {
            if( !board()->IsLayerVisible( aItem->GetLayer() ) )
                return false;
        }

        break;

    case PCB_VIA_T:
        if( !board()->IsElementVisible( LAYER_VIAS ) )
            return false;

        via = static_cast<const VIA*>( aItem );

        // For vias it is enough if only one of its layers is visible
        if( !( board()->GetVisibleLayers() & via->GetLayerSet() ).any() )
            return false;

        break;

    case PCB_FP_TEXT_T:
        if( m_isFootprintEditor )
        {
            if( !view()->IsLayerVisible( aItem->GetLayer() ) )
                return false;
        }
        else
        {
            if( !view()->IsVisible( aItem ) )
                return false;

            if( !board()->IsLayerVisible( aItem->GetLayer() ) )
                return false;
        }

        break;

    case PCB_FP_SHAPE_T:
        if( m_isFootprintEditor )
        {
            if( !view()->IsLayerVisible( aItem->GetLayer() ) )
                return false;
        }
        else
        {
            // Footprint shape selections are only allowed in footprint editor mode.
            if( !checkVisibilityOnly )
                return false;

            if( !board()->IsLayerVisible( aItem->GetLayer() ) )
                return false;
        }

        break;

    case PCB_PAD_T:
        // Multiple selection is only allowed in footprint editor mode.  In pcbnew, you have to
        // select footprint subparts one by one, rather than with a drag selection.  This is so
        // you can pick up items under an (unlocked) footprint without also moving the
        // footprint's sub-parts.
        if( !m_isFootprintEditor && !checkVisibilityOnly )
        {
            if( m_multiple )
                return false;
        }

        pad = static_cast<const PAD*>( aItem );

        if( pad->GetAttribute() == PAD_ATTRIB::PTH || pad->GetAttribute() == PAD_ATTRIB::NPTH )
        {
            // Check render mode (from the Items tab) first
            if( !board()->IsElementVisible( LAYER_PADS_TH ) )
                return false;

            // A pad's hole is visible on every layer the pad is visible on plus many layers the
            // pad is not visible on -- so we only need to check for any visible hole layers.
            if( !( board()->GetVisibleLayers() & LSET::PhysicalLayersMask() ).any() )
                return false;
        }
        else
        {
            // Check render mode (from the Items tab) first
            if( pad->IsOnLayer( F_Cu ) && !board()->IsElementVisible( LAYER_PAD_FR ) )
                return false;
            else if( pad->IsOnLayer( B_Cu ) && !board()->IsElementVisible( LAYER_PAD_BK ) )
                return false;

            if( !( pad->GetLayerSet() & board()->GetVisibleLayers() ).any() )
                return false;
        }

        break;

    // These are not selectable
    case PCB_NETINFO_T:
    case NOT_USED:
    case TYPE_NOT_INIT:
        return false;

    default:    // Suppress warnings
        break;
    }

    return aItem->ViewGetLOD( aItem->GetLayer(), view() ) < view()->GetScale();
}


void PCB_SELECTION_TOOL::select( BOARD_ITEM* aItem )
{
    if( aItem->IsSelected() )
        return;

    if( aItem->Type() == PCB_PAD_T )
    {
        FOOTPRINT* footprint = static_cast<FOOTPRINT*>( aItem->GetParent() );

        if( m_selection.Contains( footprint ) )
            return;
    }

    highlight( aItem, SELECTED, &m_selection );
}


void PCB_SELECTION_TOOL::unselect( BOARD_ITEM* aItem )
{
    unhighlight( aItem, SELECTED, &m_selection );
}


void PCB_SELECTION_TOOL::highlight( BOARD_ITEM* aItem, int aMode, PCB_SELECTION* aGroup )
{
    if( aGroup )
        aGroup->Add( aItem );

    highlightInternal( aItem, aMode, aGroup != nullptr );
    view()->Update( aItem, KIGFX::REPAINT );

    // Many selections are very temporal and updating the display each time just
    // creates noise.
    if( aMode == BRIGHTENED )
        getView()->MarkTargetDirty( KIGFX::TARGET_OVERLAY );
}


void PCB_SELECTION_TOOL::highlightInternal( BOARD_ITEM* aItem, int aMode, bool aUsingOverlay )
{
    if( aMode == SELECTED )
        aItem->SetSelected();
    else if( aMode == BRIGHTENED )
        aItem->SetBrightened();

    if( aUsingOverlay )
        view()->Hide( aItem, true );    // Hide the original item, so it is shown only on overlay

    if( aItem->Type() == PCB_FOOTPRINT_T )
    {
        static_cast<FOOTPRINT*>( aItem )->RunOnChildren(
                [&]( BOARD_ITEM* aChild )
                {
                    highlightInternal( aChild, aMode, aUsingOverlay );
                } );
    }
    else if( aItem->Type() == PCB_GROUP_T )
    {
        static_cast<PCB_GROUP*>( aItem )->RunOnChildren(
                [&]( BOARD_ITEM* aChild )
                {
                    highlightInternal( aChild, aMode, aUsingOverlay );
                } );
    }
}


void PCB_SELECTION_TOOL::unhighlight( BOARD_ITEM* aItem, int aMode, PCB_SELECTION* aGroup )
{
    if( aGroup )
        aGroup->Remove( aItem );

    unhighlightInternal( aItem, aMode, aGroup != nullptr );
    view()->Update( aItem, KIGFX::REPAINT );

    // Many selections are very temporal and updating the display each time just
    // creates noise.
    if( aMode == BRIGHTENED )
        getView()->MarkTargetDirty( KIGFX::TARGET_OVERLAY );
}


void PCB_SELECTION_TOOL::unhighlightInternal( BOARD_ITEM* aItem, int aMode, bool aUsingOverlay )
{
    if( aMode == SELECTED )
        aItem->ClearSelected();
    else if( aMode == BRIGHTENED )
        aItem->ClearBrightened();

    if( aUsingOverlay )
        view()->Hide( aItem, false );   // // Restore original item visibility

    if( aItem->Type() == PCB_FOOTPRINT_T )
    {
        static_cast<FOOTPRINT*>( aItem )->RunOnChildren(
                [&]( BOARD_ITEM* aChild )
                {
                    unhighlightInternal( aChild, aMode, aUsingOverlay );
                } );
    }
    else if( aItem->Type() == PCB_GROUP_T )
    {
        static_cast<PCB_GROUP*>( aItem )->RunOnChildren(
                [&]( BOARD_ITEM* aChild )
                {
                    unhighlightInternal( aChild, aMode, aUsingOverlay );
                } );
    }
}


bool PCB_SELECTION_TOOL::selectionContains( const VECTOR2I& aPoint ) const
{
    GENERAL_COLLECTORS_GUIDE   guide = getCollectorsGuide();
    GENERAL_COLLECTOR          collector;

    // Since we're just double-checking, we want a considerably sloppier check than the initial
    // selection (for which most tools use 5 pixels).  So we increase this to an effective 20
    // pixels by artificially inflating the value of a pixel by 4X.
    guide.SetOnePixelInIU( guide.OnePixelInIU() * 4 );

    collector.Collect( board(), m_isFootprintEditor ? GENERAL_COLLECTOR::FootprintItems
                                                    : GENERAL_COLLECTOR::AllBoardItems,
                       (wxPoint) aPoint, guide );

    for( int i = collector.GetCount() - 1; i >= 0; --i )
    {
        BOARD_ITEM* item = collector[i];

        if( item->IsSelected() && item->HitTest( (wxPoint) aPoint, 5 * guide.OnePixelInIU() ) )
            return true;
    }

    return false;
}


int PCB_SELECTION_TOOL::hitTestDistance( const wxPoint& aWhere, BOARD_ITEM* aItem,
                                         int aMaxDistance ) const
{
    BOX2D viewportD = getView()->GetViewport();
    BOX2I viewport( VECTOR2I( viewportD.GetPosition() ), VECTOR2I( viewportD.GetSize() ) );
    int   distance = INT_MAX;
    SEG   loc( aWhere, aWhere );

    switch( aItem->Type() )
    {
    case PCB_TEXT_T:
    {
        PCB_TEXT* text = static_cast<PCB_TEXT*>( aItem );
        text->GetEffectiveTextShape()->Collide( loc, aMaxDistance, &distance );
    }
        break;

    case PCB_FP_TEXT_T:
    {
        FP_TEXT* text = static_cast<FP_TEXT*>( aItem );
        text->GetEffectiveTextShape()->Collide( loc, aMaxDistance, &distance );
    }
        break;

    case PCB_ZONE_T:
    {
        ZONE* zone = static_cast<ZONE*>( aItem );

        // Zone borders are very specific
        if( zone->HitTestForEdge( aWhere, aMaxDistance / 2 ) )
            distance = 0;
        else if( zone->HitTestForEdge( aWhere, aMaxDistance ) )
            distance = aMaxDistance / 2;
        else
            aItem->GetEffectiveShape()->Collide( loc, aMaxDistance, &distance );
    }
        break;

    case PCB_FOOTPRINT_T:
    {
        FOOTPRINT* footprint = static_cast<FOOTPRINT*>( aItem );
        EDA_RECT   bbox = footprint->GetBoundingBox( false, false );

        footprint->GetBoundingHull().Collide( loc, aMaxDistance, &distance );

        // Consider footprints larger than the viewport only as a last resort
        if( bbox.GetHeight() > viewport.GetHeight() || bbox.GetWidth() > viewport.GetWidth() )
            distance = INT_MAX / 2;
    }
        break;

    case PCB_MARKER_T:
    {
        PCB_MARKER*      marker = static_cast<PCB_MARKER*>( aItem );
        SHAPE_LINE_CHAIN polygon;

        marker->ShapeToPolygon( polygon );
        polygon.Move( marker->GetPos() );
        polygon.Collide( loc, aMaxDistance, &distance );
    }
        break;

    case PCB_GROUP_T:
    {
        PCB_GROUP* group = static_cast<PCB_GROUP*>( aItem );

        for( BOARD_ITEM* member : group->GetItems() )
            distance = std::min( distance, hitTestDistance( aWhere, member, aMaxDistance ) );
    }
        break;

    default:
        aItem->GetEffectiveShape()->Collide( loc, aMaxDistance, &distance );
        break;
    }

    return distance;
}


// The general idea here is that if the user clicks directly on a small item inside a larger
// one, then they want the small item.  The quintessential case of this is clicking on a pad
// within a footprint, but we also apply it for text within a footprint, footprints within
// larger footprints, and vias within either larger pads or longer tracks.
//
// These "guesses" presume there is area within the larger item to click in to select it.  If
// an item is mostly covered by smaller items within it, then the guesses are inappropriate as
// there might not be any area left to click to select the larger item.  In this case we must
// leave the items in the collector and bring up a Selection Clarification menu.
//
// We currently check for pads and text mostly covering a footprint, but we don't check for
// smaller footprints mostly covering a larger footprint.
//
void PCB_SELECTION_TOOL::GuessSelectionCandidates( GENERAL_COLLECTOR& aCollector,
                                                   const VECTOR2I& aWhere ) const
{
    std::set<BOARD_ITEM*> preferred;
    std::set<BOARD_ITEM*> rejected;
    wxPoint               where( aWhere.x, aWhere.y );

    PCB_LAYER_ID activeLayer = (PCB_LAYER_ID) view()->GetTopLayer();
    LSET         silkLayers( 2, B_SilkS, F_SilkS );

    if( silkLayers[activeLayer] )
    {
        for( int i = 0; i < aCollector.GetCount(); ++i )
        {
            BOARD_ITEM* item = aCollector[i];
            KICAD_T type = item->Type();

            if( ( type == PCB_FP_TEXT_T || type == PCB_TEXT_T || type == PCB_SHAPE_T )
                    && silkLayers[item->GetLayer()] )
            {
                preferred.insert( item );
            }
        }

        if( preferred.size() > 0 )
        {
            aCollector.Empty();

            for( BOARD_ITEM* item : preferred )
                aCollector.Append( item );

            return;
        }
    }

    // Prefer exact hits to sloppy ones

    constexpr int MAX_SLOP = 5;

    int pixel = (int) aCollector.GetGuide()->OnePixelInIU();
    int minSlop = INT_MAX;

    std::map<BOARD_ITEM*, int> itemsBySloppiness;

    for( int i = 0; i < aCollector.GetCount(); ++i )
    {
        BOARD_ITEM* item = aCollector[i];
        int         itemSlop = hitTestDistance( where, item, MAX_SLOP * pixel );

        itemsBySloppiness[ item ] = itemSlop;

        if( itemSlop < minSlop )
            minSlop = itemSlop;
    }

    // Prune sloppier items

    if( minSlop < INT_MAX )
    {
        for( std::pair<BOARD_ITEM*, int> pair : itemsBySloppiness )
        {
            if( pair.second > minSlop + pixel )
                aCollector.Transfer( pair.first );
        }
    }

    // If the user clicked on a small item within a much larger one then it's pretty clear
    // they're trying to select the smaller one.

    constexpr double sizeRatio = 1.5;

    std::vector<std::pair<BOARD_ITEM*, double>> itemsByArea;

    for( int i = 0; i < aCollector.GetCount(); ++i )
    {
        BOARD_ITEM* item = aCollector[i];
        double      area;

        if( ( item->Type() == PCB_ZONE_T || item->Type() == PCB_FP_ZONE_T )
                && static_cast<ZONE*>( item )->HitTestForEdge( where, MAX_SLOP * pixel / 2 ) )
        {
            // Zone borders are very specific, so make them "small"
            area = MAX_SLOP * SEG::Square( pixel );
        }
        else if( item->Type() == PCB_VIA_T )
        {
            // Vias rarely hide other things, and we don't want them deferring to short track
            // segments -- so make them artificially small by dropping the π from πr².
            area = SEG::Square( static_cast<VIA*>( item )->GetDrill() / 2 );
        }
        else
        {
            try
            {
                area = FOOTPRINT::GetCoverageArea( item, aCollector );
            }
            catch( const ClipperLib::clipperException& e )
            {
                wxLogError( "A clipper exception %s was detected.", e.what() );
            }
        }

        itemsByArea.emplace_back( item, area );
    }

    std::sort( itemsByArea.begin(), itemsByArea.end(),
               []( const std::pair<BOARD_ITEM*, double>& lhs,
                   const std::pair<BOARD_ITEM*, double>& rhs ) -> bool
               {
                   return lhs.second < rhs.second;
               } );

    bool rejecting = false;

    for( int i = 1; i < (int) itemsByArea.size(); ++i )
    {
        if( itemsByArea[i].second > itemsByArea[i-1].second * sizeRatio )
            rejecting = true;

        if( rejecting )
            rejected.insert( itemsByArea[i].first );
    }

    // Special case: if a footprint is completely covered with other features then there's no
    // way to select it -- so we need to leave it in the list for user disambiguation.

    constexpr double maxCoverRatio = 0.70;

    for( int i = 0; i < aCollector.GetCount(); ++i )
    {
        if( FOOTPRINT* footprint = dynamic_cast<FOOTPRINT*>( aCollector[i] ) )
        {
            if( footprint->CoverageRatio( aCollector ) > maxCoverRatio )
                rejected.erase( footprint );
        }
    }

    // Hopefully we've now got what the user wanted.

    if( (unsigned) aCollector.GetCount() > rejected.size() )  // do not remove everything
    {
        for( BOARD_ITEM* item : rejected )
            aCollector.Transfer( item );
    }

    // Finally, what we are left with is a set of items of similar coverage area.  We now reject
    // any that are not on the active layer, to reduce the number of disambiguation menus shown.
    // If the user wants to force-disambiguate, they can either switch layers or use the modifier
    // key to force the menu.
    if( aCollector.GetCount() > 1 )
    {
        bool haveItemOnActive = false;
        rejected.clear();

        for( int i = 0; i < aCollector.GetCount(); ++i )
        {
            if( !aCollector[i]->IsOnLayer( activeLayer ) )
                rejected.insert( aCollector[i] );
            else
                haveItemOnActive = true;
        }

        if( haveItemOnActive )
            for( BOARD_ITEM* item : rejected )
                aCollector.Transfer( item );
    }
}


void PCB_SELECTION_TOOL::FilterCollectorForHierarchy( GENERAL_COLLECTOR& aCollector,
                                                      bool aMultiselect ) const
{
    std::unordered_set<BOARD_ITEM*> toAdd;

    // Set TEMP_SELECTED on all parents which are included in the GENERAL_COLLECTOR.  This
    // algorithm is O3n, whereas checking for the parent inclusion could potentially be On^2.

    for( int j = 0; j < aCollector.GetCount(); j++ )
    {
        if( aCollector[j]->GetParent() )
            aCollector[j]->GetParent()->ClearFlags( TEMP_SELECTED );
    }

    if( aMultiselect )
    {
        for( int j = 0; j < aCollector.GetCount(); j++ )
            aCollector[j]->SetFlags( TEMP_SELECTED );
    }

    for( int j = 0; j < aCollector.GetCount(); )
    {
        BOARD_ITEM* item = aCollector[j];
        BOARD_ITEM* parent = item->GetParent();
        BOARD_ITEM* start = item;

        if( !m_isFootprintEditor && parent && parent->Type() == PCB_FOOTPRINT_T )
            start = parent;

        // If any element is a member of a group, replace those elements with the top containing
        // group.
        PCB_GROUP*  aTop = PCB_GROUP::TopLevelGroup( start, m_enteredGroup, m_isFootprintEditor );

        if( aTop )
        {
            if( aTop != item )
            {
                toAdd.insert( aTop );
                aTop->SetFlags( TEMP_SELECTED );

                aCollector.Remove( item );
                continue;
            }
        }
        else if( m_enteredGroup
                    && !PCB_GROUP::WithinScope( item, m_enteredGroup, m_isFootprintEditor ) )
        {
            // If a group is entered, disallow selections of objects outside the group.
            aCollector.Remove( item );
            continue;
        }

        // Footprints are a bit easier as they can't be nested.
        if( parent && parent->GetFlags() & TEMP_SELECTED )
        {
            // Remove children of selected items
            aCollector.Remove( item );
            continue;
        }

        ++j;
    }

    for( BOARD_ITEM* item : toAdd )
    {
        if( !aCollector.HasItem( item ) )
            aCollector.Append( item );
    }
}


int PCB_SELECTION_TOOL::updateSelection( const TOOL_EVENT& aEvent )
{
    getView()->Update( &m_selection );
    getView()->Update( &m_enteredGroupOverlay );

    return 0;
}


int PCB_SELECTION_TOOL::UpdateMenu( const TOOL_EVENT& aEvent )
{
    ACTION_MENU*      actionMenu = aEvent.Parameter<ACTION_MENU*>();
    CONDITIONAL_MENU* conditionalMenu = dynamic_cast<CONDITIONAL_MENU*>( actionMenu );

    if( conditionalMenu )
        conditionalMenu->Evaluate( m_selection );

    if( actionMenu )
        actionMenu->UpdateAll();

    return 0;
}


void PCB_SELECTION_TOOL::setTransitions()
{
    Go( &PCB_SELECTION_TOOL::UpdateMenu,          ACTIONS::updateMenu.MakeEvent() );

    Go( &PCB_SELECTION_TOOL::Main,                PCB_ACTIONS::selectionActivate.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::CursorSelection,     PCB_ACTIONS::selectionCursor.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::ClearSelection,      PCB_ACTIONS::selectionClear.MakeEvent() );

    Go( &PCB_SELECTION_TOOL::SelectItem,          PCB_ACTIONS::selectItem.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::SelectItems,         PCB_ACTIONS::selectItems.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::UnselectItem,        PCB_ACTIONS::unselectItem.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::UnselectItems,       PCB_ACTIONS::unselectItems.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::SelectionMenu,       PCB_ACTIONS::selectionMenu.MakeEvent() );

    Go( &PCB_SELECTION_TOOL::find,                ACTIONS::find.MakeEvent() );

    Go( &PCB_SELECTION_TOOL::filterSelection,     PCB_ACTIONS::filterSelection.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::expandConnection,    PCB_ACTIONS::selectConnection.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::selectNet,           PCB_ACTIONS::selectNet.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::selectNet,           PCB_ACTIONS::deselectNet.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::selectSameSheet,     PCB_ACTIONS::selectSameSheet.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::selectSheetContents, PCB_ACTIONS::selectOnSheetFromEeschema.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::updateSelection,     EVENTS::SelectedItemsModified );
    Go( &PCB_SELECTION_TOOL::updateSelection,     EVENTS::SelectedItemsMoved );

    Go( &PCB_SELECTION_TOOL::SelectAll,           ACTIONS::selectAll.MakeEvent() );
}
