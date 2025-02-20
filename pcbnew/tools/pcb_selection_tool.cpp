/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
#include <cmath>
#include <functional>
#include <stack>
using namespace std::placeholders;

#include <advanced_config.h>
#include <macros.h>
#include <core/kicad_algo.h>
#include <board.h>
#include <board_design_settings.h>
#include <board_item.h>
#include <pcb_reference_image.h>
#include <pcb_track.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_group.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <pcb_textbox.h>
#include <pcb_table.h>
#include <pcb_tablecell.h>
#include <pcb_marker.h>
#include <pcb_generator.h>
#include <zone.h>
#include <collectors.h>
#include <dialog_filter_selection.h>
#include <dialogs/dialog_locked_items_query.h>
#include <class_draw_panel_gal.h>
#include <view/view_controls.h>
#include <preview_items/selection_area.h>
#include <gal/painter.h>
#include <router/router_tool.h>
#include <pcbnew_settings.h>
#include <tool/tool_event.h>
#include <tool/tool_manager.h>
#include <tools/tool_event_utils.h>
#include <tools/pcb_point_editor.h>
#include <tools/pcb_selection_tool.h>
#include <tools/pcb_actions.h>
#include <tools/board_inspection_tool.h>
#include <connectivity/connectivity_data.h>
#include <ratsnest/ratsnest_data.h>
#include <footprint_viewer_frame.h>
#include <wx/event.h>
#include <wx/timer.h>
#include <wx/log.h>
#include <wx/debug.h>
#include <core/profile.h>
#include <math/vector2wx.h>


struct LAYER_OPACITY_ITEM
{
    PCB_LAYER_ID      m_Layer;
    double            m_Opacity;
    const BOARD_ITEM* m_Item;;
};


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
        Add( PCB_ACTIONS::selectOnSchematic );

        Add( PCB_ACTIONS::selectUnconnected );
        Add( PCB_ACTIONS::grabUnconnected );
    }

private:
    ACTION_MENU* create() const override
    {
        return new SELECT_MENU();
    }
};


/**
 * Private implementation of firewalled private data.
 */
class PCB_SELECTION_TOOL::PRIV
{
public:
    DIALOG_FILTER_SELECTION::OPTIONS m_filterOpts;
};


PCB_SELECTION_TOOL::PCB_SELECTION_TOOL() :
        SELECTION_TOOL( "pcbnew.InteractiveSelection" ),
        m_frame( nullptr ),
        m_isFootprintEditor( false ),
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

    Disconnect( wxEVT_TIMER, wxTimerEventHandler( PCB_SELECTION_TOOL::onDisambiguationExpire ),
                nullptr, this );
}


bool PCB_SELECTION_TOOL::Init()
{
    PCB_BASE_FRAME* frame = getEditFrame<PCB_BASE_FRAME>();

    if( frame && frame->IsType( FRAME_FOOTPRINT_VIEWER ) )
    {
        frame->AddStandardSubMenus( *m_menu.get() );
        return true;
    }

    std::shared_ptr<SELECT_MENU> selectMenu = std::make_shared<SELECT_MENU>();
    selectMenu->SetTool( this );
    m_menu->RegisterSubMenu( selectMenu );

    static const std::vector<KICAD_T> tableCellTypes = { PCB_TABLECELL_T };

    auto& menu = m_menu->GetMenu();

    auto activeToolCondition =
            [ frame ] ( const SELECTION& aSel )
            {
                return !frame->ToolStackIsEmpty();
            };

    auto haveHighlight =
            [&]( const SELECTION& sel )
            {
                KIGFX::RENDER_SETTINGS* cfg = m_toolMgr->GetView()->GetPainter()->GetSettings();

                return !cfg->GetHighlightNetCodes().empty();
            };

    auto groupEnterCondition =
            SELECTION_CONDITIONS::Count( 1 ) && SELECTION_CONDITIONS::HasType( PCB_GROUP_T );

    auto inGroupCondition =
            [this] ( const SELECTION& )
            {
                return m_enteredGroup != nullptr;
            };

    auto tableCellSelection = SELECTION_CONDITIONS::MoreThan( 0 )
                                && SELECTION_CONDITIONS::OnlyTypes( tableCellTypes );

    if( frame && frame->IsType( FRAME_PCB_EDITOR ) )
    {
        menu.AddMenu( selectMenu.get(), SELECTION_CONDITIONS::NotEmpty  );
        menu.AddSeparator( 1000 );
    }

    // "Cancel" goes at the top of the context menu when a tool is active
    menu.AddItem( ACTIONS::cancelInteractive,  activeToolCondition, 1 );
    menu.AddItem( PCB_ACTIONS::groupEnter,     groupEnterCondition, 1 );
    menu.AddItem( PCB_ACTIONS::groupLeave,     inGroupCondition,    1 );
    menu.AddItem( PCB_ACTIONS::clearHighlight, haveHighlight,       1 );
    menu.AddSeparator(                         haveHighlight,       1 );

    menu.AddItem( ACTIONS::selectColumns,      tableCellSelection, 2 );
    menu.AddItem( ACTIONS::selectRows,         tableCellSelection, 2 );
    menu.AddItem( ACTIONS::selectTable,        tableCellSelection, 2 );

    menu.AddSeparator( 1 );

    if( frame )
        frame->AddStandardSubMenus( *m_menu.get() );

    m_disambiguateTimer.SetOwner( this );
    Connect( wxEVT_TIMER, wxTimerEventHandler( PCB_SELECTION_TOOL::onDisambiguationExpire ),
             nullptr, this );

    return true;
}


void PCB_SELECTION_TOOL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<PCB_BASE_FRAME>();
    m_isFootprintEditor = m_frame->IsType( FRAME_FOOTPRINT_EDITOR );

    if( aReason != TOOL_BASE::REDRAW )
    {
        if( m_enteredGroup )
            ExitGroup();

        // Deselect any item being currently in edit, to avoid unexpected behavior and remove
        // pointers to the selected items from containers.
        ClearSelection( true );
    }

    if( aReason == TOOL_BASE::MODEL_RELOAD )
        getView()->GetPainter()->GetSettings()->SetHighlight( false );

    // Reinsert the VIEW_GROUP, in case it was removed from the VIEW
    view()->Remove( &m_selection );
    view()->Add( &m_selection );

    view()->Remove( &m_enteredGroupOverlay );
    view()->Add( &m_enteredGroupOverlay );
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
        TRACK_DRAG_ACTION trackDragAction = TRACK_DRAG_ACTION::MOVE;

        try
        {
            trackDragAction = m_frame->GetPcbNewSettings()->m_TrackDragAction;
        }
        catch( const std::runtime_error& e )
        {
            wxFAIL_MSG( e.what() );
        }

        // on left click, a selection is made, depending on modifiers ALT, SHIFT, CTRL:
        setModifiersState( evt->Modifier( MD_SHIFT ), evt->Modifier( MD_CTRL ),
                           evt->Modifier( MD_ALT ) );

        PCB_BASE_FRAME* frame = getEditFrame<PCB_BASE_FRAME>();
        bool            brd_editor = frame && frame->IsType( FRAME_PCB_EDITOR );
        ROUTER_TOOL*    router = m_toolMgr->GetTool<ROUTER_TOOL>();

        // If the router tool is active, don't override
        if( router && router->IsToolActive() && router->RoutingInProgress() )
        {
            evt->SetPassEvent();
        }
        else if( evt->IsMouseDown( BUT_LEFT ) )
        {
            // Avoid triggering when running under other tools
            PCB_POINT_EDITOR *pt_tool = m_toolMgr->GetTool<PCB_POINT_EDITOR>();

            if( m_frame->ToolStackIsEmpty() && pt_tool && !pt_tool->HasPoint() )
            {
                m_originalCursor = m_toolMgr->GetMousePosition();
                m_disambiguateTimer.StartOnce( ADVANCED_CFG::GetCfg().m_DisambiguationMenuDelay );
            }
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            // If there is no disambiguation, this routine is still running and will
            // register a `click` event when released
            if( m_disambiguateTimer.IsRunning() )
            {
                m_disambiguateTimer.Stop();

                // Single click? Select single object
                if( m_highlight_modifier && brd_editor )
                {
                    m_toolMgr->RunAction( PCB_ACTIONS::highlightNet );
                }
                else
                {
                    m_frame->FocusOnItem( nullptr );
                    selectPoint( evt->Position() );
                }
            }

            m_canceledMenu = false;
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_disambiguateTimer.Stop();

            // Right click? if there is any object - show the context menu
            bool selectionCancelled = false;

            if( m_selection.Empty() )
            {
                selectPoint( evt->Position(), false, &selectionCancelled );
                m_selection.SetIsHover( true );
            }

            // Show selection before opening menu
            m_frame->GetCanvas()->ForceRefresh();

            if( !selectionCancelled )
            {
                m_toolMgr->VetoContextMenuMouseWarp();
                m_menu->ShowContextMenu( m_selection );
            }
        }
        else if( evt->IsDblClick( BUT_LEFT ) )
        {
            m_disambiguateTimer.Stop();

            // Double clicks make no sense in the footprint viewer
            if( frame && frame->IsType( FRAME_FOOTPRINT_VIEWER ) )
            {
                evt->SetPassEvent();
                continue;
            }

            // Double click? Display the properties window
            m_frame->FocusOnItem( nullptr );

            if( m_selection.Empty() )
                selectPoint( evt->Position() );

            if( m_selection.GetSize() == 1 && m_selection[0]->Type() == PCB_GROUP_T )
                EnterGroup();
            else
                m_toolMgr->RunAction( PCB_ACTIONS::properties );
        }
        else if( evt->IsDblClick( BUT_MIDDLE ) )
        {
            // Middle double click?  Do zoom to fit or zoom to objects
            if( evt->Modifier( MD_CTRL ) ) // Is CTRL key down?
                m_toolMgr->RunAction( ACTIONS::zoomFitObjects );
            else
                m_toolMgr->RunAction( ACTIONS::zoomFitScreen );
        }
        else if( evt->Action() == TA_MOUSE_WHEEL )
        {
            int field = -1;

            if( evt->Modifier() == ( MD_SHIFT | MD_ALT ) )
                field = 0;
            else if( evt->Modifier() == ( MD_CTRL | MD_ALT ) )
                field = 1;
            // any more?

            if( field >= 0 )
            {
                const int          delta = evt->Parameter<int>();
                ACTIONS::INCREMENT incParams{
                    delta > 0 ? 1 : -1,
                    field,
                };

                m_toolMgr->RunAction( ACTIONS::increment, incParams );
            }
        }
        else if( evt->IsDrag( BUT_LEFT ) )
        {
            m_disambiguateTimer.Stop();

            // Is another tool already moving a new object?  Don't allow a drag start
            if( !m_selection.Empty() && m_selection[0]->HasFlag( IS_NEW | IS_MOVING ) )
            {
                evt->SetPassEvent();
                continue;
            }

            // Drag with LMB? Select multiple objects (or at least draw a selection box)
            // or drag them
            m_frame->FocusOnItem( nullptr );
            m_toolMgr->ProcessEvent( EVENTS::InhibitSelectionEditing );

            GENERAL_COLLECTORS_GUIDE guide = getCollectorsGuide();
            GENERAL_COLLECTOR        collector;

            if( m_isFootprintEditor )
            {
                if( board()->GetFirstFootprint() )
                {
                    collector.Collect( board()->GetFirstFootprint(), { PCB_TABLECELL_T },
                                       evt->DragOrigin(), guide );
                }
            }
            else
            {
                collector.Collect( board(), { PCB_TABLECELL_T }, evt->DragOrigin(), guide );
            }

            if( collector.GetCount() )
            {
                if( m_selection.GetSize() == 1 && dynamic_cast<PCB_TABLE*>( m_selection.GetItem( 0 ) ) )
                {
                    m_toolMgr->RunAction( PCB_ACTIONS::move );
                }
                else
                {
                    selectTableCells( static_cast<PCB_TABLE*>( collector[0]->GetParent() ) );
                }
            }
            else if( hasModifier() || dragAction == MOUSE_DRAG_ACTION::SELECT )
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
                            int accuracy = aCollector.GetGuide()->Accuracy();
                            std::set<EDA_ITEM*> remove;

                            for( EDA_ITEM* item : aCollector )
                            {
                                if( item->Type() == PCB_ZONE_T )
                                {
                                    ZONE* zone = static_cast<ZONE*>( item );

                                    if( !zone->HitTestForCorner( aWhere, accuracy * 2 )
                                            && !zone->HitTestForEdge( aWhere, accuracy ) )
                                    {
                                        remove.insert( zone );
                                    }
                                }
                            }

                            for( EDA_ITEM* item : remove )
                                aCollector.Remove( item );
                        };

                // See if we can drag before falling back to selectMultiple()
                bool doDrag = false;

                if( evt->HasPosition() )
                {
                    if( m_selection.Empty()
                        && selectPoint( evt->DragOrigin(), false, nullptr, zoneFilledAreaFilter ) )
                    {
                        m_selection.SetIsHover( true );
                        doDrag = true;
                    }
                    // Check if dragging has started within any of selected items bounding box.
                    else if( selectionContains( evt->DragOrigin() ) )
                    {
                        doDrag = true;
                    }
                }

                if( doDrag )
                {
                    bool isTracks = m_selection.GetSize() > 0
                            && m_selection.OnlyContains( { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T } );

                    if( isTracks && trackDragAction == TRACK_DRAG_ACTION::DRAG )
                        m_toolMgr->RunAction( PCB_ACTIONS::drag45Degree );
                    else if( isTracks && trackDragAction == TRACK_DRAG_ACTION::DRAG_FREE_ANGLE )
                        m_toolMgr->RunAction( PCB_ACTIONS::dragFreeAngle );
                    else
                        m_toolMgr->RunAction( PCB_ACTIONS::move );
                }
                else
                {
                    // Otherwise drag a selection box
                    selectMultiple();
                }
            }
        }
        else if( evt->IsCancel() )
        {
            m_disambiguateTimer.Stop();
            m_frame->FocusOnItem( nullptr );

            if( !GetSelection().Empty() )
            {
                ClearSelection();
            }
            else if( evt->FirstResponder() == this && evt->GetCommandId() == (int) WXK_ESCAPE )
            {
                if( m_enteredGroup )
                {
                    ExitGroup();
                }
                else
                {
                    BOARD_INSPECTION_TOOL* controller = m_toolMgr->GetTool<BOARD_INSPECTION_TOOL>();

                    try
                    {
                        if( controller && m_frame->GetPcbNewSettings()->m_ESCClearsNetHighlight )
                            controller->ClearHighlight( *evt );
                    }
                    catch( const std::runtime_error& e )
                    {
                        wxCHECK_MSG( false, 0, e.what() );
                    }
                }
            }
        }
        else
        {
            evt->SetPassEvent();
        }


        if( m_frame->ToolStackIsEmpty() )
        {
            // move cursor prediction
            if( !hasModifier()
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
    m_disambiguateTimer.Stop();

    return 0;
}


void PCB_SELECTION_TOOL::EnterGroup()
{
    wxCHECK_RET( m_selection.GetSize() == 1 && m_selection[0]->Type() == PCB_GROUP_T,
                 wxT( "EnterGroup called when selection is not a single group" ) );
    PCB_GROUP* aGroup = static_cast<PCB_GROUP*>( m_selection[0] );

    if( m_enteredGroup != nullptr )
        ExitGroup();

    ClearSelection();
    m_enteredGroup = aGroup;
    m_enteredGroup->SetFlags( ENTERED );
    m_enteredGroup->RunOnChildren( [&]( BOARD_ITEM* titem )
                                   {
                                       select( titem );
                                   } );

    m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    view()->Hide( m_enteredGroup, true );
    m_enteredGroupOverlay.Add( m_enteredGroup );
    view()->Update( &m_enteredGroupOverlay );
}


void PCB_SELECTION_TOOL::ExitGroup( bool aSelectGroup )
{
    // Only continue if there is a group entered
    if( m_enteredGroup == nullptr )
        return;

    m_enteredGroup->ClearFlags( ENTERED );
    view()->Hide( m_enteredGroup, false );
    ClearSelection();

    if( aSelectGroup )
    {
        select( m_enteredGroup );
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
    }

    m_enteredGroupOverlay.Clear();
    m_enteredGroup = nullptr;
    view()->Update( &m_enteredGroupOverlay );
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
        m_toolMgr->RunAction( PCB_ACTIONS::selectionCursor, aClientFilter );
        m_selection.ClearReferencePoint();
    }

    if( aClientFilter )
    {
        enum DISPOSITION { BEFORE = 1, AFTER, BOTH };

        std::map<EDA_ITEM*, DISPOSITION> itemDispositions;
        GENERAL_COLLECTORS_GUIDE         guide = getCollectorsGuide();
        GENERAL_COLLECTOR                collector;

        collector.SetGuide( &guide );

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
        // recursive highlighting of that element by the group.

        for( std::pair<EDA_ITEM* const, DISPOSITION> itemDisposition : itemDispositions )
        {
            EDA_ITEM*   item = itemDisposition.first;
            DISPOSITION disposition = itemDisposition.second;

            if( disposition == BEFORE )
                unhighlight( item, SELECTED, &m_selection );
        }

        for( std::pair<EDA_ITEM* const, DISPOSITION> itemDisposition : itemDispositions )
        {
            EDA_ITEM*   item = itemDisposition.first;
            DISPOSITION disposition = itemDisposition.second;

            // Note that we must re-highlight even previously-highlighted items
            // (ie: disposition BOTH) in case we removed any of their children.
            if( disposition == AFTER || disposition == BOTH )
                highlight( item, SELECTED, &m_selection );
        }

        m_frame->GetCanvas()->ForceRefresh();
    }

    if( aConfirmLockedItems )
    {
        std::vector<BOARD_ITEM*> lockedItems;

        for( EDA_ITEM* item : m_selection )
        {
            BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( item );
            bool        lockedDescendant = false;

            boardItem->RunOnDescendants(
                    [&]( BOARD_ITEM* curr_item )
                    {
                        if( curr_item->IsLocked() )
                            lockedDescendant = true;
                    } );

            if( boardItem->IsLocked() || lockedDescendant )
                lockedItems.push_back( boardItem );
        }

        PCBNEW_SETTINGS* settings = m_frame->GetPcbNewSettings();
        if( !lockedItems.empty() && !settings->m_LockingOptions.m_sessionSkipPrompts )
        {
            DIALOG_LOCKED_ITEMS_QUERY dlg( frame(), lockedItems.size(),
                                           settings->m_LockingOptions );

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
    guide.SetIgnoreFPTextOnBack( !board()->IsElementVisible( LAYER_FP_TEXT ) );
    guide.SetIgnoreFPTextOnFront( !board()->IsElementVisible( LAYER_FP_TEXT ) );
    guide.SetIgnoreFootprintsOnBack( !board()->IsElementVisible( LAYER_FOOTPRINTS_BK ) );
    guide.SetIgnoreFootprintsOnFront( !board()->IsElementVisible( LAYER_FOOTPRINTS_FR ) );
    guide.SetIgnorePadsOnBack( padsDisabled );
    guide.SetIgnorePadsOnFront( padsDisabled );
    guide.SetIgnoreThroughHolePads( padsDisabled );
    guide.SetIgnoreFPValues( !board()->IsElementVisible( LAYER_FP_VALUES ) );
    guide.SetIgnoreFPReferences( !board()->IsElementVisible( LAYER_FP_REFERENCES ) );
    guide.SetIgnoreThroughVias( ! board()->IsElementVisible( LAYER_VIAS ) );
    guide.SetIgnoreBlindBuriedVias( ! board()->IsElementVisible( LAYER_VIAS ) );
    guide.SetIgnoreMicroVias( ! board()->IsElementVisible( LAYER_VIAS ) );
    guide.SetIgnoreTracks( ! board()->IsElementVisible( LAYER_TRACKS ) );

    return guide;
}


bool PCB_SELECTION_TOOL::ctrlClickHighlights()
{
    return m_frame && m_frame->GetPcbNewSettings()->m_CtrlClickHighlight && !m_isFootprintEditor;
}


bool PCB_SELECTION_TOOL::selectPoint( const VECTOR2I& aWhere, bool aOnDrag,
                                      bool* aSelectionCancelledFlag,
                                      CLIENT_SELECTION_FILTER aClientFilter )
{
    GENERAL_COLLECTORS_GUIDE   guide = getCollectorsGuide();
    GENERAL_COLLECTOR          collector;
    const PCB_DISPLAY_OPTIONS& displayOpts = m_frame->GetDisplayOptions();

    guide.SetIgnoreZoneFills( displayOpts.m_ZoneDisplayMode != ZONE_DISPLAY_MODE::SHOW_FILLED );

    if( m_enteredGroup && !m_enteredGroup->GetBoundingBox().Contains( aWhere ) )
        ExitGroup();

    collector.Collect( board(), m_isFootprintEditor ? GENERAL_COLLECTOR::FootprintItems
                                                    : GENERAL_COLLECTOR::AllBoardItems,
                       aWhere, guide );

    // Remove unselectable items
    for( int i = collector.GetCount() - 1; i >= 0; --i )
    {
        if( !Selectable( collector[ i ] ) || ( aOnDrag && collector[i]->IsLocked() ) )
            collector.Remove( i );
    }

    m_selection.ClearReferencePoint();

    // Apply the stateful filter (remove items disabled by the Selection Filter)
    FilterCollectedItems( collector, false );

    // Allow the client to do tool- or action-specific filtering to see if we can get down
    // to a single item
    if( aClientFilter )
        aClientFilter( aWhere, collector, this );

    FilterCollectorForHierarchy( collector, false );

    FilterCollectorForFootprints( collector, aWhere );

    // For subtracting, we only want items that are selected
    if( m_subtractive )
    {
        for( int i = collector.GetCount() - 1; i >= 0; --i )
        {
            if( !collector[i]->IsSelected() )
                collector.Remove( i );
        }
    }

    // Apply some ugly heuristics to avoid disambiguation menus whenever possible
    if( collector.GetCount() > 1 && !m_skip_heuristics )
    {
        try
        {
            GuessSelectionCandidates( collector, aWhere );
        }
        catch( const std::exception& exc )
        {
            wxLogWarning( wxS( "Exception \"%s\" occurred attempting to guess selection "
                               "candidates." ), exc.what() );
            return false;
        }
    }

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

    int  addedCount = 0;
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
                addedCount++;
            }
        }
    }

    if( addedCount == 1 )
    {
        m_toolMgr->ProcessEvent( EVENTS::PointSelectedEvent );
        return true;
    }
    else if( addedCount > 1 )
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
        selectPoint( getViewControls()->GetCursorPosition( false ), false, nullptr, aClientFilter );
    }

    return !m_selection.Empty();
}


// Some navigation actions are allowed in selectMultiple
const TOOL_ACTION* allowedActions[] = { &ACTIONS::panUp,          &ACTIONS::panDown,
                                        &ACTIONS::panLeft,        &ACTIONS::panRight,
                                        &ACTIONS::cursorUp,       &ACTIONS::cursorDown,
                                        &ACTIONS::cursorLeft,     &ACTIONS::cursorRight,
                                        &ACTIONS::cursorUpFast,   &ACTIONS::cursorDownFast,
                                        &ACTIONS::cursorLeftFast, &ACTIONS::cursorRightFast,
                                        &ACTIONS::zoomIn,         &ACTIONS::zoomOut,
                                        &ACTIONS::zoomInCenter,   &ACTIONS::zoomOutCenter,
                                        &ACTIONS::zoomCenter,     &ACTIONS::zoomFitScreen,
                                        &ACTIONS::zoomFitObjects, nullptr };


bool PCB_SELECTION_TOOL::selectTableCells( PCB_TABLE* aTable )
{
    bool cancelled = false;     // Was the tool canceled while it was running?
    m_multiple = true;          // Multiple selection mode is active

    for( PCB_TABLECELL* cell : aTable->GetCells() )
    {
        if( cell->IsSelected() )
            cell->SetFlags( CANDIDATE );
        else
            cell->ClearFlags( CANDIDATE );
    }

    auto wasSelected =
            []( EDA_ITEM* aItem )
            {
                return ( aItem->GetFlags() & CANDIDATE ) > 0;
            };

    while( TOOL_EVENT* evt = Wait() )
    {
        if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            cancelled = true;
            break;
        }
        else if( evt->IsDrag( BUT_LEFT ) )
        {
            getViewControls()->SetAutoPan( true );

            BOX2I selectionRect( evt->DragOrigin(), evt->Position() - evt->DragOrigin() );
            selectionRect.Normalize();

            for( PCB_TABLECELL* cell : aTable->GetCells() )
            {
                bool doSelect = false;

                if( cell->HitTest( selectionRect, false ) )
                {
                    if( m_subtractive )
                        doSelect = false;
                    else if( m_exclusive_or )
                        doSelect = !wasSelected( cell );
                    else
                        doSelect = true;
                }
                else if( wasSelected( cell ) )
                {
                    doSelect = m_additive || m_subtractive || m_exclusive_or;
                }

                if( doSelect && !cell->IsSelected() )
                    select( cell );
                else if( !doSelect && cell->IsSelected() )
                    unselect( cell );
            }
        }
        else if( evt->IsMouseUp( BUT_LEFT ) )
        {
            m_selection.SetIsHover( false );

            bool anyAdded = false;
            bool anySubtracted = false;

            for( PCB_TABLECELL* cell : aTable->GetCells() )
            {
                if( cell->IsSelected() && !wasSelected( cell ) )
                    anyAdded = true;
                else if( wasSelected( cell ) && !cell->IsSelected() )
                    anySubtracted = true;
            }

            // Inform other potentially interested tools
            if( anyAdded )
                m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

            if( anySubtracted )
                m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );

            break;  // Stop waiting for events
        }
        else
        {
            // Allow some actions for navigation
            for( int i = 0; allowedActions[i]; ++i )
            {
                if( evt->IsAction( allowedActions[i] ) )
                {
                    evt->SetPassEvent();
                    break;
                }
            }
        }
    }

    getViewControls()->SetAutoPan( false );

    m_multiple = false;         // Multiple selection mode is inactive

    if( !cancelled )
        m_selection.ClearReferencePoint();

    return cancelled;
}


bool PCB_SELECTION_TOOL::selectMultiple()
{
    bool cancelled = false;     // Was the tool canceled while it was running?
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
        bool greedySelection = width >= 0 ? false : true;

        if( view->IsMirroredX() )
            greedySelection = !greedySelection;

        m_frame->GetCanvas()->SetCurrentCursor( !greedySelection ? KICURSOR::SELECT_WINDOW
                                                                 : KICURSOR::SELECT_LASSO );

        if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            cancelled = true;
            break;
        }

        if( evt->IsDrag( BUT_LEFT ) )
        {
            if( !m_drag_additive && !m_drag_subtractive )
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
            area.SetAdditive( m_drag_additive );
            area.SetSubtractive( m_drag_subtractive );
            area.SetExclusiveOr( false );

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

            // Construct a BOX2I to determine BOARD_ITEM selection
            BOX2I selectionRect( area.GetOrigin(), VECTOR2I( width, height ) );

            selectionRect.Normalize();

            GENERAL_COLLECTOR collector;
            GENERAL_COLLECTOR padsCollector;
            std::set<BOARD_ITEM*> group_items;

            for( PCB_GROUP* group : board()->Groups() )
            {
                // The currently entered group does not get limited
                if( m_enteredGroup == group )
                    continue;

                std::unordered_set<BOARD_ITEM*>& newset = group->GetItems();

                // If we are not greedy and have selected the whole group, add just one item
                // to allow it to be promoted to the group later
                if( !greedySelection && selectionRect.Contains( group->GetBoundingBox() )
                        && newset.size() )
                {
                    for( BOARD_ITEM* group_item : newset )
                    {
                        if( Selectable( group_item ) )
                            collector.Append( *newset.begin() );
                    }
                }

                for( BOARD_ITEM* group_item : newset )
                    group_items.emplace( group_item );
            }

            for( const KIGFX::VIEW::LAYER_ITEM_PAIR& candidate : candidates )
            {
                BOARD_ITEM* item = static_cast<BOARD_ITEM*>( candidate.first );

                if( item && Selectable( item ) && item->HitTest( selectionRect, !greedySelection )
                        && ( greedySelection || !group_items.count( item ) ) )
                {
                    if( item->Type() == PCB_PAD_T && !m_isFootprintEditor )
                        padsCollector.Append( item );
                    else
                        collector.Append( item );
                }
            }

            // Apply the stateful filter
            FilterCollectedItems( collector, true );

            FilterCollectorForHierarchy( collector, true );

            // If we selected nothing but pads, allow them to be selected
            if( collector.GetCount() == 0 )
            {
                collector = padsCollector;
                FilterCollectedItems( collector, true );
                FilterCollectorForHierarchy( collector, true );
            }

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

        // Allow some actions for navigation
        for( int i = 0; allowedActions[i]; ++i )
        {
            if( evt->IsAction( allowedActions[i] ) )
            {
                evt->SetPassEvent();
                break;
            }
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


int PCB_SELECTION_TOOL::disambiguateCursor( const TOOL_EVENT& aEvent )
{
    wxMouseState keyboardState = wxGetMouseState();

    setModifiersState( keyboardState.ShiftDown(), keyboardState.ControlDown(),
                       keyboardState.AltDown() );

    m_skip_heuristics = true;
    selectPoint( m_originalCursor, false, &m_canceledMenu );
    m_skip_heuristics = false;

    return 0;
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


int PCB_SELECTION_TOOL::SelectAll( const TOOL_EVENT& aEvent )
{
    GENERAL_COLLECTOR collection;
    BOX2I             selectionBox;

    selectionBox.SetMaximum();

    getView()->Query( selectionBox,
            [&]( KIGFX::VIEW_ITEM* viewItem ) -> bool
            {
                BOARD_ITEM* item = static_cast<BOARD_ITEM*>( viewItem );

                if( !item || !Selectable( item ) || !itemPassesFilter( item, true ) )
                    return true;

                collection.Append( item );
                return true;
            } );

    FilterCollectorForHierarchy( collection, true );

    for( EDA_ITEM* item : collection )
        select( item );

    m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    m_frame->GetCanvas()->ForceRefresh();

    return 0;
}


int PCB_SELECTION_TOOL::UnselectAll( const TOOL_EVENT& aEvent )
{
    BOX2I selectionBox;

    selectionBox.SetMaximum();

    getView()->Query( selectionBox,
            [&]( KIGFX::VIEW_ITEM* viewItem ) -> bool
            {
                BOARD_ITEM* item = static_cast<BOARD_ITEM*>( viewItem );

                if( !item || !Selectable( item ) )
                    return true;

                unselect( item );
                return true;
            } );

    m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );

    m_frame->GetCanvas()->ForceRefresh();

    return 0;
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


int PCB_SELECTION_TOOL::unrouteSelected( const TOOL_EVENT& aEvent )
{
    std::deque<EDA_ITEM*> selectedItems = m_selection.GetItems();

    // Get all footprints and pads
    std::vector<BOARD_CONNECTED_ITEM*> toUnroute;

    for( EDA_ITEM* item : selectedItems )
    {
        if( item->Type() == PCB_FOOTPRINT_T )
        {
            for( PAD* pad : static_cast<FOOTPRINT*>( item )->Pads() )
                toUnroute.push_back( pad );
        }
        else if( BOARD_CONNECTED_ITEM::ClassOf( item ) )
        {
            toUnroute.push_back( static_cast<BOARD_CONNECTED_ITEM*>( item ) );
        }
    }

    // Clear selection so we don't delete our footprints/pads
    ClearSelection( true );

    // Get the tracks on our list of pads, then delete them
    selectAllConnectedTracks( toUnroute, STOP_CONDITION::STOP_AT_PAD );
    m_toolMgr->RunAction( ACTIONS::doDelete );

    // Reselect our footprint/pads as they were in our original selection
    for( EDA_ITEM* item : selectedItems )
    {
        if( item->Type() == PCB_FOOTPRINT_T || item->Type() == PCB_PAD_T )
            select( item );
    }

    return 0;
}


int PCB_SELECTION_TOOL::expandConnection( const TOOL_EVENT& aEvent )
{
    // expandConnection will get called no matter whether the user selected a connected item or a
    // non-connected shape (graphic on a non-copper layer). The algorithm for expanding to connected
    // items is different from graphics, so they need to be handled separately.
    unsigned initialCount = 0;

    for( const EDA_ITEM* item : m_selection.GetItems() )
    {
        if( item->Type() == PCB_FOOTPRINT_T
            || item->Type() == PCB_GENERATOR_T
            || ( static_cast<const BOARD_ITEM*>( item )->IsConnected() ) )
        {
            initialCount++;
        }
    }

    if( initialCount == 0 )
    {
        // First, process any graphic shapes we have
        std::vector<PCB_SHAPE*> startShapes;

        for( EDA_ITEM* item : m_selection.GetItems() )
        {
            if( isExpandableGraphicShape( item ) )
                startShapes.push_back( static_cast<PCB_SHAPE*>( item ) );
        }

        // If no non-copper shapes; fall back to looking for connected items
        if( !startShapes.empty() )
            selectAllConnectedShapes( startShapes );
        else
            selectCursor( true, connectedItemFilter );
    }

    m_frame->SetStatusText( _( "Select/Expand Connection..." ) );

    for( STOP_CONDITION stopCondition : { STOP_AT_JUNCTION, STOP_AT_PAD, STOP_NEVER } )
    {
        std::deque<EDA_ITEM*> selectedItems = m_selection.GetItems();

        for( EDA_ITEM* item : selectedItems )
            item->ClearTempFlags();

        std::vector<BOARD_CONNECTED_ITEM*> startItems;

        for( EDA_ITEM* item : selectedItems )
        {
            if( item->Type() == PCB_FOOTPRINT_T )
            {
                FOOTPRINT* footprint = static_cast<FOOTPRINT*>( item );

                for( PAD* pad : footprint->Pads() )
                    startItems.push_back( pad );
            }
            else if( item->Type() == PCB_GENERATOR_T )
            {
                for( BOARD_ITEM* generatedItem : static_cast<PCB_GENERATOR*>( item )->GetItems() )
                {
                    if( BOARD_CONNECTED_ITEM::ClassOf( generatedItem ) )
                        startItems.push_back( static_cast<BOARD_CONNECTED_ITEM*>( generatedItem ) );
                }
            }
            else if( BOARD_CONNECTED_ITEM::ClassOf( item ) )
            {
                startItems.push_back( static_cast<BOARD_CONNECTED_ITEM*>( item ) );
            }
        }

        selectAllConnectedTracks( startItems, stopCondition );

        if( m_selection.GetItems().size() > initialCount )
            break;
    }

    m_frame->SetStatusText( wxEmptyString );

    // Inform other potentially interested tools
    m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    return 0;
}


void PCB_SELECTION_TOOL::selectAllConnectedTracks(
        const std::vector<BOARD_CONNECTED_ITEM*>& aStartItems, STOP_CONDITION aStopCondition )
{
    const LSET        allCuMask = LSET::AllCuMask();

    PROF_TIMER refreshTimer;
    double     refreshIntervalMs = 500; // Refresh display with this interval to indicate progress
    int        lastSelectionSize = (int) m_selection.GetSize();

    auto connectivity = board()->GetConnectivity();

    std::map<VECTOR2I, std::vector<PCB_TRACK*>> trackMap;
    std::map<VECTOR2I, PCB_VIA*>                viaMap;
    std::map<VECTOR2I, PAD*>                    padMap;
    std::map<VECTOR2I, std::vector<PCB_SHAPE*>> shapeMap;
    std::set<PAD*>                              startPadSet;
    std::vector<BOARD_CONNECTED_ITEM*>          cleanupItems;
    std::vector<std::pair<VECTOR2I, LSET>>      activePts;

    for( BOARD_CONNECTED_ITEM* startItem : aStartItems )
    {
        // Register starting pads
        if( startItem->Type() == PCB_PAD_T )
            startPadSet.insert( static_cast<PAD*>( startItem ) );

        // Select any starting track items
        if( startItem->IsType( { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T } ) )
            select( startItem );
    }

    for( BOARD_CONNECTED_ITEM* startItem : aStartItems )
    {
        if( startItem->HasFlag( SKIP_STRUCT ) ) // Skip already visited items
            continue;

        auto connectedItems = connectivity->GetConnectedItems( startItem,
                { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T, PCB_PAD_T, PCB_SHAPE_T }, true );

        // Build maps of connected items
        for( BOARD_CONNECTED_ITEM* item : connectedItems )
        {
            switch( item->Type() )
            {
            case PCB_ARC_T:
            case PCB_TRACE_T:
            {
                PCB_TRACK* track = static_cast<PCB_TRACK*>( item );
                trackMap[track->GetStart()].push_back( track );
                trackMap[track->GetEnd()].push_back( track );
                break;
            }

            case PCB_VIA_T:
            {
                PCB_VIA* via = static_cast<PCB_VIA*>( item );
                viaMap[via->GetStart()] = via;
                break;
            }

            case PCB_PAD_T:
            {
                PAD* pad = static_cast<PAD*>( item );
                padMap[pad->GetPosition()] = pad;
                break;
            }

            case PCB_SHAPE_T:
            {
                PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( item );

                for( const auto& point : shape->GetConnectionPoints() )
                    shapeMap[point].push_back( shape );

                break;
            }

            default:
                break;
            }
        }

        // Set up the initial active points
        switch( startItem->Type() )
        {
        case PCB_ARC_T:
        case PCB_TRACE_T:
        {
            PCB_TRACK* track = static_cast<PCB_TRACK*>( startItem );

            activePts.push_back( { track->GetStart(), track->GetLayerSet() } );
            activePts.push_back( { track->GetEnd(), track->GetLayerSet() } );
            break;
        }

        case PCB_VIA_T:
            activePts.push_back( { startItem->GetPosition(), startItem->GetLayerSet() } );
            break;

        case PCB_PAD_T:
            activePts.push_back( { startItem->GetPosition(), startItem->GetLayerSet() } );
            break;

        case PCB_SHAPE_T:
        {
            PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( startItem );

            for( const auto& point : shape->GetConnectionPoints() )
                activePts.push_back( { point, startItem->GetLayerSet() } );

            break;
        }

        default:
            break;
        }

        bool expand = true;
        int  failSafe = 0;

        // Iterative push from all active points
        while( expand && failSafe++ < 100000 )
        {
            expand = false;

            for( int i = (int) activePts.size() - 1; i >= 0; --i )
            {
                VECTOR2I pt = activePts[i].first;
                LSET     layerSetCu = activePts[i].second & allCuMask;

                auto viaIt = viaMap.find( pt );
                auto padIt = padMap.find( pt );

                bool gotVia = ( viaIt != viaMap.end() )
                              && ( layerSetCu & ( viaIt->second->GetLayerSet() ) ).any();

                bool gotPad = ( padIt != padMap.end() )
                              && ( layerSetCu & ( padIt->second->GetLayerSet() ) ).any();

                bool gotNonStartPad =
                        gotPad && ( startPadSet.find( padIt->second ) == startPadSet.end() );

                if( aStopCondition == STOP_AT_JUNCTION )
                {
                    size_t pt_count = 0;

                    for( PCB_TRACK* track : trackMap[pt] )
                    {
                        if( track->GetStart() != track->GetEnd()
                                && layerSetCu.Contains( track->GetLayer() ) )
                        {
                            pt_count++;
                        }
                    }

                    if( pt_count > 2 || gotVia || gotNonStartPad )
                    {
                        activePts.erase( activePts.begin() + i );
                        continue;
                    }
                }
                else if( aStopCondition == STOP_AT_PAD )
                {
                    if( gotNonStartPad )
                    {
                        activePts.erase( activePts.begin() + i );
                        continue;
                    }
                }

                if( gotPad )
                {
                    PAD* pad = padIt->second;

                    if( !pad->HasFlag( SKIP_STRUCT ) )
                    {
                        pad->SetFlags( SKIP_STRUCT );
                        cleanupItems.push_back( pad );

                        activePts.push_back( { pad->GetPosition(), pad->GetLayerSet() } );
                        expand = true;
                    }
                }

                for( PCB_TRACK* track : trackMap[pt] )
                {
                    if( !layerSetCu.Contains( track->GetLayer() ) )
                        continue;

                    if( !track->IsSelected() )
                        select( track );

                    if( !track->HasFlag( SKIP_STRUCT ) )
                    {
                        track->SetFlags( SKIP_STRUCT );
                        cleanupItems.push_back( track );

                        if( track->GetStart() == pt )
                            activePts.push_back( { track->GetEnd(), track->GetLayerSet() } );
                        else
                            activePts.push_back( { track->GetStart(), track->GetLayerSet() } );

                        expand = true;
                    }
                }

                for( PCB_SHAPE* shape : shapeMap[pt] )
                {
                    if( !layerSetCu.Contains( shape->GetLayer() ) )
                        continue;

                    if( !shape->IsSelected() )
                        select( shape );

                    if( !shape->HasFlag( SKIP_STRUCT ) )
                    {
                        shape->SetFlags( SKIP_STRUCT );
                        cleanupItems.push_back( shape );

                        for( const VECTOR2I& newPoint : shape->GetConnectionPoints() )
                        {
                            if( newPoint == pt )
                                continue;

                            activePts.push_back( { newPoint, shape->GetLayerSet() } );
                        }

                        expand = true;
                    }
                }

                if( viaMap.count( pt ) )
                {
                    PCB_VIA* via = viaMap[pt];

                    if( !via->IsSelected() )
                        select( via );

                    if( !via->HasFlag( SKIP_STRUCT ) )
                    {
                        via->SetFlags( SKIP_STRUCT );
                        cleanupItems.push_back( via );

                        activePts.push_back( { via->GetPosition(), via->GetLayerSet() } );
                        expand = true;
                    }
                }

                activePts.erase( activePts.begin() + i );
            }

            // Refresh display for the feel of progress
            if( refreshTimer.msecs() >= refreshIntervalMs )
            {
                if( m_selection.Size() != lastSelectionSize )
                {
                    m_frame->GetCanvas()->ForceRefresh();
                    lastSelectionSize = m_selection.Size();
                }

                refreshTimer.Start();
            }
        }
    }

    std::set<EDA_ITEM*> toDeselect;
    std::set<EDA_ITEM*> toSelect;

    // Promote generated members to their PCB_GENERATOR parents
    for( EDA_ITEM* item : m_selection )
    {
        if( !item->IsBOARD_ITEM() )
            continue;

        BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( item );
        PCB_GROUP*  parent = boardItem->GetParentGroup();

        if( parent && parent->Type() == PCB_GENERATOR_T )
        {
            toDeselect.insert( item );

            if( !parent->IsSelected() )
                toSelect.insert( parent );
        }
    }

    for( EDA_ITEM* item : toDeselect )
        unselect( item );

    for( EDA_ITEM* item : toSelect )
        select( item );

    for( BOARD_CONNECTED_ITEM* item : cleanupItems )
        item->ClearFlags( SKIP_STRUCT );
}


bool PCB_SELECTION_TOOL::isExpandableGraphicShape( const EDA_ITEM* aItem ) const
{
    if( aItem->Type() == PCB_SHAPE_T )
    {
        const PCB_SHAPE* shape = static_cast<const PCB_SHAPE*>( aItem );

        switch( shape->GetShape() )
        {
        case SHAPE_T::SEGMENT:
        case SHAPE_T::ARC:
        case SHAPE_T::BEZIER:
            return !shape->IsOnCopperLayer();

        case SHAPE_T::POLY:
            return !shape->IsOnCopperLayer() && !shape->IsClosed();

        default:
            return false;
        }
    }

    return false;
}


void PCB_SELECTION_TOOL::selectAllConnectedShapes( const std::vector<PCB_SHAPE*>& aStartItems )
{
    std::stack<PCB_SHAPE*> toSearch;
    std::set<PCB_SHAPE*> toCleanup;

    for( PCB_SHAPE* startItem : aStartItems )
        toSearch.push( startItem );

    GENERAL_COLLECTOR        collector;
    GENERAL_COLLECTORS_GUIDE guide = getCollectorsGuide();

    auto searchPoint = [&]( const VECTOR2I& aWhere )
    {
        collector.Collect( board(), { PCB_SHAPE_T }, aWhere, guide );

        for( EDA_ITEM* item : collector )
        {
            if( isExpandableGraphicShape( item ) )
                toSearch.push( static_cast<PCB_SHAPE*>( item ) );
        }
    };

    while( !toSearch.empty() )
    {
        PCB_SHAPE* shape = toSearch.top();
        toSearch.pop();

        if( shape->HasFlag( SKIP_STRUCT ) )
            continue;

        select( shape );
        shape->SetFlags( SKIP_STRUCT );
        toCleanup.insert( shape );

        guide.SetLayerVisibleBits( shape->GetLayerSet() );

        searchPoint( shape->GetStart() );
        searchPoint( shape->GetEnd() );
    }

    for( PCB_SHAPE* shape : toCleanup )
        shape->ClearFlags( SKIP_STRUCT );
}


int PCB_SELECTION_TOOL::selectUnconnected( const TOOL_EVENT& aEvent )
{
    // Get all pads
    std::vector<PAD*> pads;

    for( EDA_ITEM* item : m_selection.GetItems() )
    {
        if( item->Type() == PCB_FOOTPRINT_T )
        {
            for( PAD* pad : static_cast<FOOTPRINT*>( item )->Pads() )
                pads.push_back( pad );
        }
        else if( item->Type() == PCB_PAD_T )
        {
            pads.push_back( static_cast<PAD*>( item ) );
        }
    }

    // Select every footprint on the end of the ratsnest for each pad in our selection
    std::shared_ptr<CONNECTIVITY_DATA> conn = board()->GetConnectivity();

    for( PAD* pad : pads )
    {
        for( const CN_EDGE& edge : conn->GetRatsnestForPad( pad ) )
        {
            wxCHECK2( edge.GetSourceNode() && !edge.GetSourceNode()->Dirty(), continue );
            wxCHECK2( edge.GetTargetNode() && !edge.GetTargetNode()->Dirty(), continue );

            BOARD_CONNECTED_ITEM* sourceParent = edge.GetSourceNode()->Parent();
            BOARD_CONNECTED_ITEM* targetParent = edge.GetTargetNode()->Parent();

            if( sourceParent == pad )
            {
                if( targetParent->Type() == PCB_PAD_T )
                    select( static_cast<PAD*>( targetParent )->GetParent() );
            }
            else if( targetParent == pad )
            {
                if( sourceParent->Type() == PCB_PAD_T )
                    select( static_cast<PAD*>( sourceParent )->GetParent() );
            }
        }
    }

    return 0;
}


int PCB_SELECTION_TOOL::grabUnconnected( const TOOL_EVENT& aEvent )
{
    PCB_SELECTION originalSelection = m_selection;

    // Get all pads
    std::vector<PAD*> pads;

    for( EDA_ITEM* item : m_selection.GetItems() )
    {
        if( item->Type() == PCB_FOOTPRINT_T )
        {
            for( PAD* pad : static_cast<FOOTPRINT*>( item )->Pads() )
                pads.push_back( pad );
        }
        else if( item->Type() == PCB_PAD_T )
        {
            pads.push_back( static_cast<PAD*>( item ) );
        }
    }

    ClearSelection();

    // Select every footprint on the end of the ratsnest for each pad in our selection
    std::shared_ptr<CONNECTIVITY_DATA> conn = board()->GetConnectivity();

    for( PAD* pad : pads )
    {
        const std::vector<CN_EDGE> edges = conn->GetRatsnestForPad( pad );

        // Need to have something unconnected to grab
        if( edges.size() == 0 )
            continue;

        double     currentDistance = DBL_MAX;
        FOOTPRINT* nearest = nullptr;

        // Check every ratsnest line for the nearest one
        for( const CN_EDGE& edge : edges )
        {
            if( edge.GetSourceNode()->Parent()->GetParentFootprint()
                == edge.GetTargetNode()->Parent()->GetParentFootprint() )
            {
                continue; // This edge is a loop on the same footprint
            }

            // Figure out if we are the source or the target node on the ratnest
            const CN_ANCHOR* other = edge.GetSourceNode()->Parent() == pad ? edge.GetTargetNode().get()
                                                                           : edge.GetSourceNode().get();

            wxCHECK2( other && !other->Dirty(), continue );

            // We only want to grab footprints, so the ratnest has to point to a pad
            if( other->Parent()->Type() != PCB_PAD_T )
                continue;

            if( edge.GetLength() < currentDistance )
            {
                currentDistance = edge.GetLength();
                nearest = other->Parent()->GetParentFootprint();
            }
        }

        if( nearest != nullptr )
            select( nearest );
    }

    m_toolMgr->RunAction( PCB_ACTIONS::moveIndividually );

    return 0;
}


void PCB_SELECTION_TOOL::SelectAllItemsOnNet( int aNetCode, bool aSelect )
{
    std::shared_ptr<CONNECTIVITY_DATA> conn = board()->GetConnectivity();

    for( BOARD_ITEM* item : conn->GetNetItems( aNetCode, { PCB_TRACE_T,
                                                           PCB_ARC_T,
                                                           PCB_VIA_T,
                                                           PCB_SHAPE_T } ) )
    {
        if( itemPassesFilter( item, true ) )
            aSelect ? select( item ) : unselect( item );
    }
}


int PCB_SELECTION_TOOL::selectNet( const TOOL_EVENT& aEvent )
{
    bool select = aEvent.IsAction( &PCB_ACTIONS::selectNet );

    // If we've been passed an argument, just select that netcode1
    int netcode = aEvent.Parameter<int>();

    if( netcode > 0 )
    {
        SelectAllItemsOnNet( netcode, select );

        // Inform other potentially interested tools
        if( m_selection.Size() > 0 )
            m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
        else
            m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );

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
            SelectAllItemsOnNet( connItem->GetNetCode(), select );
    }

    // Inform other potentially interested tools
    if( m_selection.Size() > 0 )
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
    else
        m_toolMgr->ProcessEvent( EVENTS::UnselectedEvent );

    return 0;
}


void PCB_SELECTION_TOOL::selectAllItemsOnSheet( wxString& aSheetPath )
{
    std::vector<BOARD_ITEM*> footprints;

    // store all footprints that are on that sheet path
    for( FOOTPRINT* footprint : board()->Footprints() )
    {
        if( footprint == nullptr )
            continue;

        wxString footprint_path = footprint->GetPath().AsString().BeforeLast( '/' );

        if( footprint_path.IsEmpty() )
            footprint_path += '/';

        if( footprint_path == aSheetPath )
            footprints.push_back( footprint );
    }

    for( BOARD_ITEM* i : footprints )
    {
        if( i != nullptr )
            select( i );
    }

    selectConnections( footprints );
}


void PCB_SELECTION_TOOL::selectConnections( const std::vector<BOARD_ITEM*>& aItems )
{
    // Generate a list of all pads, and of all nets they belong to.
    std::list<int>                     netcodeList;
    std::vector<BOARD_CONNECTED_ITEM*> padList;

    for( BOARD_ITEM* item : aItems )
    {
        switch( item->Type() )
        {
        case PCB_FOOTPRINT_T:
        {
            for( PAD* pad : static_cast<FOOTPRINT*>( item )->Pads() )
            {
                if( pad->IsConnected() )
                {
                    netcodeList.push_back( pad->GetNetCode() );
                    padList.push_back( pad );
                }
            }

            break;
        }

        case PCB_PAD_T:
        {
            PAD* pad = static_cast<PAD*>( item );

            if( pad->IsConnected() )
            {
                netcodeList.push_back( pad->GetNetCode() );
                padList.push_back( pad );
            }

            break;
        }

        default:
            break;
        }
    }

    // Sort for binary search
    std::sort( padList.begin(), padList.end() );

    // remove all duplicates
    netcodeList.sort();
    netcodeList.unique();

    selectAllConnectedTracks( padList, STOP_AT_PAD );

    // now we need to find all footprints that are connected to each of these nets then we need
    // to determine if these footprints are in the list of footprints
    std::vector<int>                   removeCodeList;
    std::shared_ptr<CONNECTIVITY_DATA> conn = board()->GetConnectivity();

    for( int netCode : netcodeList )
    {
        for( BOARD_CONNECTED_ITEM* pad : conn->GetNetItems( netCode, { PCB_PAD_T } ) )
        {
            if( !std::binary_search( padList.begin(), padList.end(), pad ) )
            {
                // if we cannot find the pad in the padList then we can assume that that pad
                // should not be used, therefore invalidate this netcode.
                removeCodeList.push_back( netCode );
                break;
            }
        }
    }

    for( int removeCode : removeCodeList )
        netcodeList.remove( removeCode );

    std::unordered_set<BOARD_ITEM*> localConnectionList;

    for( int netCode : netcodeList )
    {
        for( BOARD_ITEM* item : conn->GetNetItems( netCode, { PCB_TRACE_T,
                                                              PCB_ARC_T,
                                                              PCB_VIA_T,
                                                              PCB_SHAPE_T } ) )
        {
            localConnectionList.insert( item );
        }
    }

    for( BOARD_ITEM* item : localConnectionList )
        select( item );
}


int PCB_SELECTION_TOOL::syncSelection( const TOOL_EVENT& aEvent )
{
    std::vector<BOARD_ITEM*>* items = aEvent.Parameter<std::vector<BOARD_ITEM*>*>();

    if( items )
        doSyncSelection( *items, false );

    return 0;
}


int PCB_SELECTION_TOOL::syncSelectionWithNets( const TOOL_EVENT& aEvent )
{
    std::vector<BOARD_ITEM*>* items = aEvent.Parameter<std::vector<BOARD_ITEM*>*>();

    if( items )
        doSyncSelection( *items, true );

    return 0;
}


void PCB_SELECTION_TOOL::doSyncSelection( const std::vector<BOARD_ITEM*>& aItems, bool aWithNets )
{
    ClearSelection( true /*quiet mode*/ );

    // Perform individual selection of each item before processing the event.
    for( BOARD_ITEM* item : aItems )
        select( item );

    if( aWithNets )
        selectConnections( aItems );

    BOX2I bbox = m_selection.GetBoundingBox();

    if( bbox.GetWidth() != 0 && bbox.GetHeight() != 0 )
    {
        if( m_frame->GetPcbNewSettings()->m_CrossProbing.center_on_items )
        {
            if( m_frame->GetPcbNewSettings()->m_CrossProbing.zoom_to_fit )
                ZoomFitCrossProbeBBox( bbox );

            m_frame->FocusOnLocation( bbox.Centre() );
        }
    }

    view()->UpdateAllLayersColor();

    m_frame->GetCanvas()->ForceRefresh();

    if( m_selection.Size() > 0 )
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
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
    // this function currently only supports footprints since they are only on one sheet.
    EDA_ITEM* item = m_selection.Front();

    if( !item )
        return 0;

    if( item->Type() != PCB_FOOTPRINT_T )
        return 0;

    FOOTPRINT* footprint = dynamic_cast<FOOTPRINT*>( item );

    if( !footprint || footprint->GetPath().empty() )
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


void PCB_SELECTION_TOOL::zoomFitSelection()
{
    // Should recalculate the view to zoom in on the selection.
    BOX2I        selectionBox = m_selection.GetBoundingBox();
    KIGFX::VIEW* view = getView();

    VECTOR2D     screenSize = view->ToWorld( ToVECTOR2D( m_frame->GetCanvas()->GetClientSize() ),
                                             false );
    screenSize.x = std::max( 10.0, screenSize.x );
    screenSize.y = std::max( 10.0, screenSize.y );

    if( selectionBox.GetWidth() != 0 || selectionBox.GetHeight() != 0 )
    {
        VECTOR2D vsize = selectionBox.GetSize();
        double   scale = view->GetScale() / std::max( fabs( vsize.x / screenSize.x ),
                                                      fabs( vsize.y / screenSize.y ) );
        view->SetScale( scale );
        view->SetCenter( selectionBox.Centre() );
        view->Add( &m_selection );
    }

    m_frame->GetCanvas()->ForceRefresh();
}


void PCB_SELECTION_TOOL::ZoomFitCrossProbeBBox( const BOX2I& aBBox )
{
    // Should recalculate the view to zoom in on the bbox.
    KIGFX::VIEW* view = getView();

    if( aBBox.GetWidth() == 0 )
        return;

    BOX2I bbox = aBBox;
    bbox.Normalize();

    //#define DEFAULT_PCBNEW_CODE // Un-comment for normal full zoom KiCad algorithm
#ifdef DEFAULT_PCBNEW_CODE
    auto bbSize = bbox.Inflate( bbox.GetWidth() * 0.2f ).GetSize();
    auto screenSize = view->ToWorld( GetCanvas()->GetClientSize(), false );

    // The "fabs" on x ensures the right answer when the view is flipped
    screenSize.x = std::max( 10.0, fabs( screenSize.x ) );
    screenSize.y = std::max( 10.0, screenSize.y );
    double ratio = std::max( fabs( bbSize.x / screenSize.x ), fabs( bbSize.y / screenSize.y ) );

    // Try not to zoom on every cross-probe; it gets very noisy
    if( crossProbingSettings.zoom_to_fit && ( ratio < 0.5 || ratio > 1.0 ) )
        view->SetScale( view->GetScale() / ratio );
#endif // DEFAULT_PCBNEW_CODE

#ifndef DEFAULT_PCBNEW_CODE // Do the scaled zoom
    auto bbSize = bbox.Inflate( KiROUND( bbox.GetWidth() * 0.2 ) ).GetSize();
    VECTOR2D screenSize = view->ToWorld( ToVECTOR2D( m_frame->GetCanvas()->GetClientSize() ),
                                         false );

    // This code tries to come up with a zoom factor that doesn't simply zoom in
    // to the cross probed component, but instead shows a reasonable amount of the
    // circuit around it to provide context.  This reduces or eliminates the need
    // to manually change the zoom because it's too close.

    // Using the default text height as a constant to compare against, use the
    // height of the bounding box of visible items for a footprint to figure out
    // if this is a big footprint (like a processor) or a small footprint (like a resistor).
    // This ratio is not useful by itself as a scaling factor.  It must be "bent" to
    // provide good scaling at varying component sizes.  Bigger components need less
    // scaling than small ones.
    double currTextHeight = pcbIUScale.mmToIU( DEFAULT_TEXT_SIZE );

    double compRatio = bbSize.y / currTextHeight; // Ratio of component to text height

    // This will end up as the scaling factor we apply to "ratio".
    double compRatioBent = 1.0;

    // This is similar to the original KiCad code that scaled the zoom to make sure
    // components were visible on screen.  It's simply a ratio of screen size to
    // component size, and its job is to zoom in to make the component fullscreen.
    // Earlier in the code the component BBox is given a 20% margin to add some
    // breathing room. We compare the height of this enlarged component bbox to the
    // default text height.  If a component will end up with the sides clipped, we
    // adjust later to make sure it fits on screen.
    //
    // The "fabs" on x ensures the right answer when the view is flipped
    screenSize.x = std::max( 10.0, fabs( screenSize.x ) );
    screenSize.y = std::max( 10.0, screenSize.y );
    double ratio = std::max( -1.0, fabs( bbSize.y / screenSize.y ) );

    // Original KiCad code for how much to scale the zoom
    double kicadRatio = std::max( fabs( bbSize.x / screenSize.x ),
                                  fabs( bbSize.y / screenSize.y ) );

    // LUT to scale zoom ratio to provide reasonable schematic context.  Must work
    // with footprints of varying sizes (e.g. 0402 package and 200 pin BGA).
    // "first" is used as the input and "second" as the output
    //
    // "first" = compRatio (footprint height / default text height)
    // "second" = Amount to scale ratio by
    std::vector<std::pair<double, double>> lut {
        { 1, 8 },
        { 1.5, 5 },
        { 3, 3 },
        { 4.5, 2.5 },
        { 8, 2.0 },
        { 12, 1.7 },
        { 16, 1.5 },
        { 24, 1.3 },
        { 32, 1.0 },
    };


    std::vector<std::pair<double, double>>::iterator it;

    compRatioBent = lut.back().second; // Large component default

    if( compRatio >= lut.front().first )
    {
        // Use LUT to do linear interpolation of "compRatio" within "first", then
        // use that result to linearly interpolate "second" which gives the scaling
        // factor needed.

        for( it = lut.begin(); it < lut.end() - 1; it++ )
        {
            if( it->first <= compRatio && next( it )->first >= compRatio )
            {
                double diffx = compRatio - it->first;
                double diffn = next( it )->first - it->first;

                compRatioBent = it->second + ( next( it )->second - it->second ) * diffx / diffn;
                break; // We have our interpolated value
            }
        }
    }
    else
    {
        compRatioBent = lut.front().second; // Small component default
    }

    // If the width of the part we're probing is bigger than what the screen width will be
    // after the zoom, then punt and use the KiCad zoom algorithm since it guarantees the
    // part's width will be encompassed within the screen.  This will apply to parts that
    // are much wider than they are tall.

    if( bbSize.x > screenSize.x * ratio * compRatioBent )
    {
        // Use standard KiCad zoom algorithm for parts too wide to fit screen/
        ratio = kicadRatio;
        compRatioBent = 1.0; // Reset so we don't modify the "KiCad" ratio
        wxLogTrace( "CROSS_PROBE_SCALE",
                    "Part TOO WIDE for screen.  Using normal KiCad zoom ratio: %1.5f", ratio );
    }

    // Now that "compRatioBent" holds our final scaling factor we apply it to the original
    // fullscreen zoom ratio to arrive at the final ratio itself.
    ratio *= compRatioBent;

    bool alwaysZoom = false; // DEBUG - allows us to minimize zooming or not

    // Try not to zoom on every cross-probe; it gets very noisy
    if( ( ratio < 0.5 || ratio > 1.0 ) || alwaysZoom )
        view->SetScale( view->GetScale() / ratio );
#endif // ifndef DEFAULT_PCBNEW_CODE
}


void PCB_SELECTION_TOOL::FindItem( BOARD_ITEM* aItem )
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
        switch( aItem->Type() )
        {
        case PCB_NETINFO_T:
        {
            int netCode = static_cast<NETINFO_ITEM*>( aItem )->GetNetCode();

            if( netCode > 0 )
            {
                SelectAllItemsOnNet( netCode, true );
                m_frame->FocusOnLocation( aItem->GetCenter() );
            }
            break;
        }

        default:
            select( aItem );
            m_frame->FocusOnLocation( aItem->GetPosition() );
        }

        // If the item has a bounding box, then zoom out if needed
        if( aItem->GetBoundingBox().GetHeight() > 0 && aItem->GetBoundingBox().GetWidth() > 0 )
        {
            // This adds some margin
            double marginFactor = 2;

            KIGFX::PCB_VIEW* pcbView = canvas()->GetView();
            BOX2D            screenBox = pcbView->GetViewport();
            VECTOR2D         screenSize = screenBox.GetSize();
            BOX2I            screenRect = BOX2ISafe( screenBox.GetOrigin(), screenSize / marginFactor );

            if( !screenRect.Contains( aItem->GetBoundingBox() ) )
            {
                double scaleX = screenSize.x /
                                static_cast<double>( aItem->GetBoundingBox().GetWidth() );
                double scaleY = screenSize.y /
                                static_cast<double>( aItem->GetBoundingBox().GetHeight() );

                scaleX /= marginFactor;
                scaleY /= marginFactor;

                double scale = scaleX > scaleY ? scaleY : scaleX;

                if( scale < 1 ) // Don't zoom in, only zoom out
                {
                    pcbView->SetScale( pcbView->GetScale() * ( scale ) );

                    //Let's refocus because there is an algorithm to avoid dialogs in there.
                    m_frame->FocusOnLocation( aItem->GetCenter() );
                }
            }
        }
        // Inform other potentially interested tools
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
    }
    else if( cleared )
    {
        m_toolMgr->ProcessEvent( EVENTS::ClearedEvent );
    }

    m_frame->GetCanvas()->ForceRefresh();
}


/**
 * Determine if an item is included by the filter specified.
 *
 * @return true if aItem should be selected by this filter (i..e not filtered out)
 */
static bool itemIsIncludedByFilter( const BOARD_ITEM& aItem, const BOARD& aBoard,
                                    const DIALOG_FILTER_SELECTION::OPTIONS& aFilterOptions )
{
    switch( aItem.Type() )
    {
    case PCB_FOOTPRINT_T:
    {
        const FOOTPRINT& footprint = static_cast<const FOOTPRINT&>( aItem );

        return aFilterOptions.includeModules
                    && ( aFilterOptions.includeLockedModules || !footprint.IsLocked() );
    }

    case PCB_TRACE_T:
    case PCB_ARC_T:
        return aFilterOptions.includeTracks;

    case PCB_VIA_T:
        return aFilterOptions.includeVias;

    case PCB_ZONE_T:
        return aFilterOptions.includeZones;

    case PCB_SHAPE_T:
    case PCB_TARGET_T:
    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_RADIAL_T:
    case PCB_DIM_ORTHOGONAL_T:
    case PCB_DIM_LEADER_T:
        if( aItem.GetLayer() == Edge_Cuts )
            return aFilterOptions.includeBoardOutlineLayer;
        else
            return aFilterOptions.includeItemsOnTechLayers;

    case PCB_FIELD_T:
    case PCB_TEXT_T:
    case PCB_TEXTBOX_T:
    case PCB_TABLE_T:
    case PCB_TABLECELL_T:
        return aFilterOptions.includePcbTexts;

    default:
        // Filter dialog is inclusive, not exclusive.  If it's not included, then it doesn't
        // get selected.
        return false;
    }
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


void PCB_SELECTION_TOOL::FilterCollectedItems( GENERAL_COLLECTOR& aCollector, bool aMultiSelect )
{
    if( aCollector.GetCount() == 0 )
        return;

    std::set<BOARD_ITEM*> rejected;

    for( EDA_ITEM* i : aCollector )
    {
        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( i );

        if( !itemPassesFilter( item, aMultiSelect ) )
            rejected.insert( item );
    }

    for( BOARD_ITEM* item : rejected )
        aCollector.Remove( item );
}


bool PCB_SELECTION_TOOL::itemPassesFilter( BOARD_ITEM* aItem, bool aMultiSelect )
{
    if( !m_filter.lockedItems )
    {
        if( aItem->IsLocked() || ( aItem->GetParent() && aItem->GetParent()->IsLocked() ) )
        {
            if( aItem->Type() == PCB_PAD_T && !aMultiSelect )
            {
                // allow a single pad to be selected -- there are a lot of operations that
                // require this so we allow this one inconsistency
            }
            else
            {
                return false;
            }
        }
    }

    if( !aItem )
        return false;

    KICAD_T itemType = aItem->Type();

    if( itemType == PCB_GENERATOR_T )
    {
        if( static_cast<PCB_GENERATOR*>( aItem )->GetItems().empty() )
        {
            if( !m_filter.otherItems )
                return false;
        }
        else
        {
            itemType = ( *static_cast<PCB_GENERATOR*>( aItem )->GetItems().begin() )->Type();
        }
    }

    switch( itemType )
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

    case PCB_ZONE_T:
    {
        ZONE* zone = static_cast<ZONE*>( aItem );

        if( ( !m_filter.zones && !zone->GetIsRuleArea() )
            || ( !m_filter.keepouts && zone->GetIsRuleArea() ) )
        {
            return false;
        }

        // m_SolderMaskBridges zone is a special zone, only used to showsolder mask briges
        // after running DRC. it is not really a board item.
        // Never select it or delete by a Commit.
        if( zone == m_frame->GetBoard()->m_SolderMaskBridges )
            return false;

        break;
    }

    case PCB_SHAPE_T:
    case PCB_TARGET_T:
        if( !m_filter.graphics )
            return false;

        break;

    case PCB_REFERENCE_IMAGE_T:
        if( !m_filter.graphics )
            return false;

        // a reference image living in a footprint must not be selected inside the board editor
        if( !m_isFootprintEditor && aItem->GetParentFootprint() )
            return false;

        break;

    case PCB_FIELD_T:
    case PCB_TEXT_T:
    case PCB_TEXTBOX_T:
    case PCB_TABLE_T:
    case PCB_TABLECELL_T:
        if( !m_filter.text )
            return false;

        break;

    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_RADIAL_T:
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
        unhighlight( m_selection.Front(), SELECTED, &m_selection );

    view()->Update( &m_selection );

    m_selection.SetIsHover( false );
    m_selection.ClearReferencePoint();

    // Inform other potentially interested tools
    if( !aQuietMode )
    {
        m_toolMgr->ProcessEvent( EVENTS::ClearedEvent );
        m_toolMgr->RunAction( PCB_ACTIONS::hideLocalRatsnest );
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
                        return INSPECT_RESULT::CONTINUE;

                    highlight( item, SELECTED, &m_selection );
                }

                if( item->Type() == PCB_GROUP_T )
                {
                    if( item == m_enteredGroup )
                    {
                        item->SetFlags( ENTERED );
                        enteredGroupFound = true;
                    }
                    else
                    {
                        item->ClearFlags( ENTERED );
                    }
                }

                return INSPECT_RESULT::CONTINUE;
            };

    board()->Visit( inspector, nullptr, m_isFootprintEditor ? GENERAL_COLLECTOR::FootprintItems
                                                            : GENERAL_COLLECTOR::AllBoardItems );

    if( !enteredGroupFound )
    {
        m_enteredGroupOverlay.Clear();
        m_enteredGroup = nullptr;
    }
}


bool PCB_SELECTION_TOOL::Selectable( const BOARD_ITEM* aItem, bool checkVisibilityOnly ) const
{
    const RENDER_SETTINGS* settings = getView()->GetPainter()->GetSettings();
    const PCB_DISPLAY_OPTIONS& options = frame()->GetDisplayOptions();

    auto visibleLayers =
            [&]()
            {
                if( m_isFootprintEditor )
                {
                    LSET set;

                    for( PCB_LAYER_ID layer : LSET::AllLayersMask().Seq() )
                        set.set( layer, view()->IsLayerVisible( layer ) );

                    return set;
                }
                else
                {
                    return board()->GetVisibleLayers();
                }
            };

    auto layerVisible =
            [&]( PCB_LAYER_ID aLayer )
            {
                if( m_isFootprintEditor )
                    return view()->IsLayerVisible( aLayer );
                else
                    return board()->IsLayerVisible( aLayer );
            };

    if( settings->GetHighContrast() )
    {
        const std::set<int> activeLayers = settings->GetHighContrastLayers();
        bool                onActiveLayer = false;

        for( int layer : activeLayers )
        {
            // NOTE: Only checking the regular layers (not GAL meta-layers)
            if( layer < PCB_LAYER_ID_COUNT && aItem->IsOnLayer( ToLAYER_ID( layer ) ) )
            {
                onActiveLayer = true;
                break;
            }
        }

        if( !onActiveLayer && aItem->Type() != PCB_MARKER_T )
        {
            // We do not want to select items that are in the background
            return false;
        }
    }

    if( aItem->Type() == PCB_FOOTPRINT_T )
    {
        const FOOTPRINT* footprint = static_cast<const FOOTPRINT*>( aItem );

        // In footprint editor, we do not want to select the footprint itself.
        if( m_isFootprintEditor )
            return false;

        // Allow selection of footprints if some part of the footprint is visible.
        if( footprint->GetSide() != UNDEFINED_LAYER && !m_skip_heuristics )
        {
            LSET boardSide = footprint->IsFlipped() ? LSET::BackMask() : LSET::FrontMask();

            if( !( visibleLayers() & boardSide ).any() )
                return false;
        }

        // If the footprint has no items except the reference and value fields, include the
        // footprint in the selections.
        if( footprint->GraphicalItems().empty()
                && footprint->Pads().empty()
                && footprint->Zones().empty() )
        {
            return true;
        }

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

    if( aItem->GetParentGroup() && aItem->GetParentGroup()->Type() == PCB_GENERATOR_T )
        return false;

    const ZONE*     zone = nullptr;
    const PCB_VIA*  via = nullptr;
    const PAD*      pad = nullptr;
    const PCB_TEXT* text = nullptr;
    const PCB_FIELD* field = nullptr;

    // Most footprint children can only be selected in the footprint editor.
    if( aItem->GetParentFootprint() && !m_isFootprintEditor && !checkVisibilityOnly )
    {
        if( aItem->Type() != PCB_FIELD_T && aItem->Type() != PCB_PAD_T
            && aItem->Type() != PCB_TEXT_T )
        {
            return false;
        }
    }

    switch( aItem->Type() )
    {
    case PCB_ZONE_T:
        if( !board()->IsElementVisible( LAYER_ZONES ) || ( options.m_ZoneOpacity == 0.00 ) )
            return false;

        zone = static_cast<const ZONE*>( aItem );

        // A teardrop is modelled as a property of a via, pad or the board (for track-to-track
        // teardrops).  The underlying zone is only an implementation detail.
        if( zone->IsTeardropArea() && !board()->LegacyTeardrops() )
            return false;

        // zones can exist on multiple layers!
        if( !( zone->GetLayerSet() & visibleLayers() ).any() )
            return false;

        break;

    case PCB_TRACE_T:
    case PCB_ARC_T:
        if( !board()->IsElementVisible( LAYER_TRACKS ) || ( options.m_TrackOpacity == 0.00 ) )
            return false;

        if( !layerVisible( aItem->GetLayer() ) )
            return false;

        break;

    case PCB_VIA_T:
        if( !board()->IsElementVisible( LAYER_VIAS ) || ( options.m_ViaOpacity == 0.00 ) )
            return false;

        via = static_cast<const PCB_VIA*>( aItem );

        // For vias it is enough if only one of its layers is visible
        if( !( visibleLayers() & via->GetLayerSet() ).any() )
            return false;

        break;

    case PCB_FIELD_T:
        field = static_cast<const PCB_FIELD*>( aItem );

        if( !field->IsVisible() )
            return false;

        if( field->IsReference() && !view()->IsLayerVisible( LAYER_FP_REFERENCES ) )
            return false;

        if( field->IsValue() && !view()->IsLayerVisible( LAYER_FP_VALUES ) )
            return false;

        // Handle all other fields with normal text visibility controls
        KI_FALLTHROUGH;
    case PCB_TEXT_T:
        text = static_cast<const PCB_TEXT*>( aItem );

        if( !layerVisible( text->GetLayer() ) )
            return false;

        // Apply the LOD visibility test as well
        if( !view()->IsVisible( text ) )
            return false;

        if( aItem->GetParentFootprint() )
        {
            int controlLayer = LAYER_FP_TEXT;

            if( text->GetText() == wxT( "${REFERENCE}" ) )
                controlLayer = LAYER_FP_REFERENCES;
            else if( text->GetText() == wxT( "${VALUE}" ) )
                controlLayer = LAYER_FP_VALUES;

            if( !view()->IsLayerVisible( controlLayer ) )
                return false;
        }

        break;

    case PCB_REFERENCE_IMAGE_T:
        if( options.m_ImageOpacity == 0.00 )
            return false;

        // Bitmap images on board are hidden if LAYER_DRAW_BITMAPS is not visible
        if( !view()->IsLayerVisible( LAYER_DRAW_BITMAPS ) )
            return false;

        KI_FALLTHROUGH;

    case PCB_SHAPE_T:
        // Note: LAYER_SHAPES does not control the visibility of a PCB_SHAPE_T, only
        // the opacity of filled areas
        // The visibility is managed by the item layer
        if( options.m_FilledShapeOpacity == 0.0 )
            return false;

        KI_FALLTHROUGH;

    case PCB_TEXTBOX_T:
    case PCB_TABLE_T:
    case PCB_TABLECELL_T:
        if( !layerVisible( aItem->GetLayer() ) )
            return false;

        if( aItem->Type() == PCB_TABLECELL_T )
        {
            const PCB_TABLECELL* cell = static_cast<const PCB_TABLECELL*>( aItem );

            if( cell->GetRowSpan() == 0 || cell->GetColSpan() == 0 )
                return false;
        }

        break;

    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_LEADER_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_RADIAL_T:
    case PCB_DIM_ORTHOGONAL_T:
        if( !layerVisible( aItem->GetLayer() ) )
            return false;

        break;

    case PCB_PAD_T:
        if( options.m_PadOpacity == 0.00 )
            return false;

        pad = static_cast<const PAD*>( aItem );

        if( pad->GetAttribute() == PAD_ATTRIB::PTH || pad->GetAttribute() == PAD_ATTRIB::NPTH )
        {
            // A pad's hole is visible on every layer the pad is visible on plus many layers the
            // pad is not visible on -- so we only need to check for any visible hole layers.
            if( !( visibleLayers() & LSET::PhysicalLayersMask() ).any() )
                return false;
        }
        else
        {
            if( !( pad->GetLayerSet() & visibleLayers() ).any() )
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

    return true;
}


void PCB_SELECTION_TOOL::select( EDA_ITEM* aItem )
{
    if( !aItem || aItem->IsSelected() )
        return;

    if( aItem->Type() == PCB_PAD_T )
    {
        FOOTPRINT* footprint = static_cast<FOOTPRINT*>( aItem->GetParent() );

        if( m_selection.Contains( footprint ) )
            return;
    }

    if( m_enteredGroup &&
        !PCB_GROUP::WithinScope( static_cast<BOARD_ITEM*>( aItem ), m_enteredGroup,
                                 m_isFootprintEditor ) )
    {
        ExitGroup();
    }

    highlight( aItem, SELECTED, &m_selection );
}


void PCB_SELECTION_TOOL::unselect( EDA_ITEM* aItem )
{
    unhighlight( aItem, SELECTED, &m_selection );
}


void PCB_SELECTION_TOOL::highlight( EDA_ITEM* aItem, int aMode, SELECTION* aGroup )
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


void PCB_SELECTION_TOOL::highlightInternal( EDA_ITEM* aItem, int aMode, bool aUsingOverlay )
{
    if( aMode == SELECTED )
        aItem->SetSelected();
    else if( aMode == BRIGHTENED )
        aItem->SetBrightened();

    if( aUsingOverlay && aMode != BRIGHTENED )
        view()->Hide( aItem, true );    // Hide the original item, so it is shown only on overlay

    if( aItem->IsBOARD_ITEM() )
    {
        BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( aItem );
        boardItem->RunOnDescendants( std::bind( &PCB_SELECTION_TOOL::highlightInternal, this, _1,
                                                aMode, aUsingOverlay ) );
    }
}


void PCB_SELECTION_TOOL::unhighlight( EDA_ITEM* aItem, int aMode, SELECTION* aGroup )
{
    if( aGroup )
        aGroup->Remove( aItem );

    unhighlightInternal( aItem, aMode, aGroup != nullptr );
    view()->Update( aItem, KIGFX::REPAINT );

    // Many selections are very temporal and updating the display each time just creates noise.
    if( aMode == BRIGHTENED )
        getView()->MarkTargetDirty( KIGFX::TARGET_OVERLAY );
}


void PCB_SELECTION_TOOL::unhighlightInternal( EDA_ITEM* aItem, int aMode, bool aUsingOverlay )
{
    if( aMode == SELECTED )
        aItem->ClearSelected();
    else if( aMode == BRIGHTENED )
        aItem->ClearBrightened();

    if( aUsingOverlay && aMode != BRIGHTENED )
    {
        view()->Hide( aItem, false );   // Restore original item visibility...
        view()->Update( aItem );        // ... and make sure it's redrawn un-selected
    }

    if( aItem->IsBOARD_ITEM() )
    {
        BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( aItem );
        boardItem->RunOnDescendants( std::bind( &PCB_SELECTION_TOOL::unhighlightInternal, this, _1,
                                                aMode, aUsingOverlay ) );
    }
}


bool PCB_SELECTION_TOOL::selectionContains( const VECTOR2I& aPoint ) const
{
    const unsigned GRIP_MARGIN = 20;
    int            margin = KiROUND( getView()->ToWorld( GRIP_MARGIN ) );

    // Check if the point is located close to any of the currently selected items
    for( EDA_ITEM* item : m_selection )
    {
        BOX2I itemBox = item->ViewBBox();
        itemBox.Inflate( margin ); // Give some margin for gripping an item

        if( itemBox.Contains( aPoint ) )
        {
            if( item->HitTest( aPoint, margin ) )
                return true;

            bool found = false;

            if( PCB_GROUP* group = dynamic_cast<PCB_GROUP*>( item ) )
            {
                group->RunOnDescendants(
                        [&]( BOARD_ITEM* aItem )
                        {
                            if( aItem->HitTest( aPoint, margin ) )
                                found = true;
                        } );
            }

            if( found )
                return true;
        }
    }

    return false;
}


int PCB_SELECTION_TOOL::hitTestDistance( const VECTOR2I& aWhere, BOARD_ITEM* aItem,
                                         int aMaxDistance ) const
{
    BOX2D viewportD = getView()->GetViewport();
    BOX2I viewport = BOX2ISafe( viewportD );
    int   distance = INT_MAX;
    SEG   loc( aWhere, aWhere );

    switch( aItem->Type() )
    {
    case PCB_FIELD_T:
    case PCB_TEXT_T:
    {
        PCB_TEXT* text = static_cast<PCB_TEXT*>( aItem );

        // Add a bit of slop to text-shapes
        if( text->GetEffectiveTextShape()->Collide( loc, aMaxDistance, &distance ) )
            distance = std::clamp( distance - ( aMaxDistance / 2 ), 0, distance );

        break;
    }

    case PCB_TEXTBOX_T:
    case PCB_TABLECELL_T:
    {
        PCB_TEXTBOX* textbox = static_cast<PCB_TEXTBOX*>( aItem );

        // Add a bit of slop to text-shapes
        if( textbox->GetEffectiveTextShape()->Collide( loc, aMaxDistance, &distance ) )
            distance = std::clamp( distance - ( aMaxDistance / 2 ), 0, distance );

        break;
    }

    case PCB_TABLE_T:
    {
        PCB_TABLE* table = static_cast<PCB_TABLE*>( aItem );

        for( PCB_TABLECELL* cell : table->GetCells() )
        {
            // Add a bit of slop to text-shapes
            if( cell->GetEffectiveTextShape()->Collide( loc, aMaxDistance, &distance ) )
                distance = std::clamp( distance - ( aMaxDistance / 2 ), 0, distance );
        }

        break;
    }

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

        break;
    }

    case PCB_FOOTPRINT_T:
    {
        FOOTPRINT* footprint = static_cast<FOOTPRINT*>( aItem );
        BOX2I      bbox = footprint->GetBoundingBox( false );

        try
        {
            footprint->GetBoundingHull().Collide( loc, aMaxDistance, &distance );
        }
        catch( const std::exception& e )
        {
            wxFAIL_MSG( wxString::Format( wxT( "Clipper exception occurred: %s" ), e.what() ) );
        }

        // Consider footprints larger than the viewport only as a last resort
        if( bbox.GetHeight() > viewport.GetHeight() || bbox.GetWidth() > viewport.GetWidth() )
            distance = INT_MAX / 2;

        break;
    }

    case PCB_MARKER_T:
    {
        PCB_MARKER*      marker = static_cast<PCB_MARKER*>( aItem );
        SHAPE_LINE_CHAIN polygon;

        marker->ShapeToPolygon( polygon );
        polygon.Move( marker->GetPos() );
        polygon.Collide( loc, aMaxDistance, &distance );
        break;
    }

    case PCB_GROUP_T:
    case PCB_GENERATOR_T:
    {
        PCB_GROUP* group = static_cast<PCB_GROUP*>( aItem );

        for( BOARD_ITEM* member : group->GetItems() )
            distance = std::min( distance, hitTestDistance( aWhere, member, aMaxDistance ) );

        break;
    }

    case PCB_PAD_T:
    {
        static_cast<PAD*>( aItem )->Padstack().ForEachUniqueLayer(
            [&]( PCB_LAYER_ID aLayer )
            {
                int layerDistance = INT_MAX;
                aItem->GetEffectiveShape( aLayer )->Collide( loc, aMaxDistance, &layerDistance );
                distance = std::min( distance, layerDistance );
            } );

        break;
    }

    default:
        aItem->GetEffectiveShape()->Collide( loc, aMaxDistance, &distance );
        break;
    }

    return distance;
}


void PCB_SELECTION_TOOL::pruneObscuredSelectionCandidates( GENERAL_COLLECTOR& aCollector ) const
{
    wxCHECK( m_frame, /* void */ );

    if( aCollector.GetCount() < 2 )
        return;

    const RENDER_SETTINGS* settings = getView()->GetPainter()->GetSettings();

    wxCHECK( settings, /* void */ );

    PCB_LAYER_ID activeLayer = m_frame->GetActiveLayer();
    LSET visibleLayers = m_frame->GetBoard()->GetVisibleLayers();
    LSET enabledLayers = m_frame->GetBoard()->GetEnabledLayers();
    LSEQ enabledLayerStack = enabledLayers.SeqStackupTop2Bottom( activeLayer );

    wxCHECK( !enabledLayerStack.empty(), /* void */ );

    auto isCopperPourKeepoutZone = []( const BOARD_ITEM* aItem ) -> bool
                                   {
                                       if( aItem->Type() == PCB_ZONE_T )
                                       {
                                           const ZONE* zone = static_cast<const ZONE*>( aItem );

                                           wxCHECK( zone, false );

                                           if( zone->GetIsRuleArea()
                                             && zone->GetDoNotAllowCopperPour() )
                                               return true;
                                       }

                                       return false;
                                   };

    std::vector<LAYER_OPACITY_ITEM> opacityStackup;

    for( int i = 0; i < aCollector.GetCount(); i++ )
    {
        const BOARD_ITEM* item = aCollector[i];

        LSET itemLayers = item->GetLayerSet() & enabledLayers & visibleLayers;
        LSEQ itemLayerSeq = itemLayers.Seq( enabledLayerStack );

        for( PCB_LAYER_ID layer : itemLayerSeq )
        {
            COLOR4D color = settings->GetColor( item, layer );

            if( color.a == 0 )
                continue;

            LAYER_OPACITY_ITEM opacityItem;

            opacityItem.m_Layer = layer;
            opacityItem.m_Opacity = color.a;
            opacityItem.m_Item = item;

            if( isCopperPourKeepoutZone( item ) )
                opacityItem.m_Opacity = 0.0;

            opacityStackup.emplace_back( opacityItem );
        }
    }

    std::sort( opacityStackup.begin(), opacityStackup.end(),
               [&]( const LAYER_OPACITY_ITEM& aLhs, const LAYER_OPACITY_ITEM& aRhs ) -> bool
               {
                   int retv = enabledLayerStack.TestLayers( aLhs.m_Layer, aRhs.m_Layer );

                   if( retv )
                       return retv > 0;

                   return aLhs.m_Opacity > aRhs.m_Opacity;
               } );

    std::set<const BOARD_ITEM*> visibleItems;
    std::set<const BOARD_ITEM*> itemsToRemove;
    double minAlphaLimit = ADVANCED_CFG::GetCfg().m_PcbSelectionVisibilityRatio;
    double currentStackupOpacity = 0.0;
    PCB_LAYER_ID lastVisibleLayer = PCB_LAYER_ID::UNDEFINED_LAYER;

    for( const LAYER_OPACITY_ITEM& opacityItem : opacityStackup )
    {
        if( lastVisibleLayer == PCB_LAYER_ID::UNDEFINED_LAYER )
        {
            currentStackupOpacity = opacityItem.m_Opacity;
            lastVisibleLayer = opacityItem.m_Layer;
            visibleItems.emplace( opacityItem.m_Item );
            continue;
        }

        // Objects to ignore and fallback to the old selection behavior.
        auto ignoreItem = [&]()
                          {
                              const BOARD_ITEM* item = opacityItem.m_Item;

                              wxCHECK( item, false );

                              // Check items that span multiple layers for visibility.
                              if( visibleItems.count( item ) )
                                  return true;

                              // Don't prune child items of a footprint that is already visible.
                              if( item->GetParent()
                                && ( item->GetParent()->Type() == PCB_FOOTPRINT_T )
                                && visibleItems.count( item->GetParent() ) )
                                  return true;

                              // Keepout zones are transparent but for some reason,
                              // PCB_PAINTER::GetColor() returns the color of the zone it
                              // prevents from filling.
                              if( isCopperPourKeepoutZone( item ) )
                                  return true;

                              return false;
                          };

        // Everything on the currently selected layer is visible;
        if( opacityItem.m_Layer == enabledLayerStack[0] )
        {
            visibleItems.emplace( opacityItem.m_Item );
        }
        else
        {
            double itemVisibility = opacityItem.m_Opacity * ( 1.0 - currentStackupOpacity );

            if( ( itemVisibility <= minAlphaLimit ) && !ignoreItem() )
                itemsToRemove.emplace( opacityItem.m_Item );
            else
                visibleItems.emplace( opacityItem.m_Item );
        }

        if( opacityItem.m_Layer != lastVisibleLayer )
        {
            currentStackupOpacity += opacityItem.m_Opacity * ( 1.0 - currentStackupOpacity );
            currentStackupOpacity = std::min( currentStackupOpacity, 1.0 );
            lastVisibleLayer = opacityItem.m_Layer;
        }
    }

    for( const BOARD_ITEM* itemToRemove : itemsToRemove )
    {
        wxCHECK( aCollector.GetCount() > 1, /* void */ );
        aCollector.Remove( itemToRemove );
    }
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
    static const LSET silkLayers( { B_SilkS, F_SilkS } );
    static const LSET courtyardLayers( { B_CrtYd, F_CrtYd } );
    static std::vector<KICAD_T> singleLayerSilkTypes = { PCB_FIELD_T,
                                                         PCB_TEXT_T, PCB_TEXTBOX_T,
                                                         PCB_TABLE_T, PCB_TABLECELL_T,
                                                         PCB_SHAPE_T };

    if( ADVANCED_CFG::GetCfg().m_PcbSelectionVisibilityRatio != 1.0 )
        pruneObscuredSelectionCandidates( aCollector );

    if( aCollector.GetCount() == 1 )
        return;

    std::set<BOARD_ITEM*>  preferred;
    std::set<BOARD_ITEM*>  rejected;
    VECTOR2I               where( aWhere.x, aWhere.y );
    const RENDER_SETTINGS* settings = getView()->GetPainter()->GetSettings();
    PCB_LAYER_ID           activeLayer = m_frame->GetActiveLayer();

    // If a silk layer is in front, we assume the user is working with silk and give preferential
    // treatment to single-layer items on *either* silk layer.
    if( silkLayers[activeLayer] )
    {
        for( int i = 0; i < aCollector.GetCount(); ++i )
        {
            BOARD_ITEM* item = aCollector[i];

            if( item->IsType( singleLayerSilkTypes ) && silkLayers[ item->GetLayer() ] )
                preferred.insert( item );
        }
    }
    // Similarly, if a courtyard layer is in front, we assume the user is positioning footprints
    // and give preferential treatment to footprints on *both* top and bottom.
    else if( courtyardLayers[activeLayer] && settings->GetHighContrast() )
    {
        for( int i = 0; i < aCollector.GetCount(); ++i )
        {
            BOARD_ITEM* item = aCollector[i];

            if( item->Type() == PCB_FOOTPRINT_T )
                preferred.insert( item );
        }
    }

    if( preferred.size() > 0 )
    {
        aCollector.Empty();

        for( BOARD_ITEM* item : preferred )
            aCollector.Append( item );

        if( preferred.size() == 1 )
            return;
    }

    // Prefer exact hits to sloppy ones
    constexpr int MAX_SLOP = 5;

    int singlePixel = KiROUND( aCollector.GetGuide()->OnePixelInIU() );
    int maxSlop = KiROUND( MAX_SLOP * aCollector.GetGuide()->OnePixelInIU() );
    int minSlop = INT_MAX;

    std::map<BOARD_ITEM*, int> itemsBySloppiness;

    for( int i = 0; i < aCollector.GetCount(); ++i )
    {
        BOARD_ITEM* item = aCollector[i];
        int         itemSlop = hitTestDistance( where, item, maxSlop );

        itemsBySloppiness[ item ] = itemSlop;

        if( itemSlop < minSlop )
            minSlop = itemSlop;
    }

    // Prune sloppier items
    if( minSlop < INT_MAX )
    {
        for( std::pair<BOARD_ITEM*, int> pair : itemsBySloppiness )
        {
            if( pair.second > minSlop + singlePixel )
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
        double      area = 0.0;

        if( item->Type() == PCB_ZONE_T
                && static_cast<ZONE*>( item )->HitTestForEdge( where, maxSlop / 2 ) )
        {
            // Zone borders are very specific, so make them "small"
            area = (double) SEG::Square( singlePixel ) * MAX_SLOP;
        }
        else if( item->Type() == PCB_VIA_T )
        {
            // Vias rarely hide other things, and we don't want them deferring to short track
            // segments underneath them -- so artificially reduce their size from πr² to r².
            area = (double) SEG::Square( static_cast<PCB_VIA*>( item )->GetDrill() / 2 );
        }
        else if( item->Type() == PCB_REFERENCE_IMAGE_T )
        {
            BOX2I box = item->GetBoundingBox();
            area = (double) box.GetWidth() * box.GetHeight();
        }
        else
        {
            try
            {
                area = FOOTPRINT::GetCoverageArea( item, aCollector );
            }
            catch( const std::exception& e )
            {
                wxFAIL_MSG( wxString::Format( wxT( "Clipper exception occurred: %s" ), e.what() ) );
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
        {
            for( BOARD_ITEM* item : rejected )
                aCollector.Transfer( item );
        }
    }
}


void PCB_SELECTION_TOOL::FilterCollectorForHierarchy( GENERAL_COLLECTOR& aCollector,
                                                      bool aMultiselect ) const
{
    std::unordered_set<BOARD_ITEM*> toAdd;

    // Set CANDIDATE on all parents which are included in the GENERAL_COLLECTOR.  This
    // algorithm is O(3n), whereas checking for the parent inclusion could potentially be O(n^2).
    for( int j = 0; j < aCollector.GetCount(); j++ )
    {
        if( aCollector[j]->GetParent() )
            aCollector[j]->GetParent()->ClearFlags( CANDIDATE );
    }

    if( aMultiselect )
    {
        for( int j = 0; j < aCollector.GetCount(); j++ )
            aCollector[j]->SetFlags( CANDIDATE );
    }

    for( int j = 0; j < aCollector.GetCount(); )
    {
        BOARD_ITEM* item = aCollector[j];
        BOARD_ITEM* parent = item->GetParent();
        BOARD_ITEM* start = item;

        if( !m_isFootprintEditor && parent && parent->Type() == PCB_FOOTPRINT_T )
            start = parent;

        // If a group is entered, disallow selections of objects outside the group.
        if( m_enteredGroup && !PCB_GROUP::WithinScope( item, m_enteredGroup, m_isFootprintEditor ) )
        {
            aCollector.Remove( item );
            continue;
        }

        // If any element is a member of a group, replace those elements with the top containing
        // group.
        if( PCB_GROUP* top = PCB_GROUP::TopLevelGroup( start, m_enteredGroup,
                                                       m_isFootprintEditor ) )
        {
            if( top != item )
            {
                toAdd.insert( top );
                top->SetFlags(CANDIDATE );

                aCollector.Remove( item );
                continue;
            }
        }

        // Footprints are a bit easier as they can't be nested.
        if( parent && ( parent->GetFlags() & CANDIDATE ) )
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


void PCB_SELECTION_TOOL::FilterCollectorForTableCells( GENERAL_COLLECTOR& aCollector ) const
{
    std::set<BOARD_ITEM*> to_add;

    // Iterate from the back so we don't have to worry about removals.
    for( int i = (int) aCollector.GetCount() - 1; i >= 0; --i )
    {
        BOARD_ITEM* item = aCollector[i];

        if( item->Type() == PCB_TABLECELL_T )
        {
            if( !aCollector.HasItem( item->GetParent() ) )
                to_add.insert( item->GetParent() );

            aCollector.Remove( item );
        }
    }

    for( BOARD_ITEM* item : to_add )
        aCollector.Append( item );
}


void PCB_SELECTION_TOOL::FilterCollectorForFreePads( GENERAL_COLLECTOR& aCollector,
                                                     bool aForcePromotion ) const
{
    std::set<BOARD_ITEM*> to_add;

    // Iterate from the back so we don't have to worry about removals.
    for( int i = aCollector.GetCount() - 1; i >= 0; --i )
    {
        BOARD_ITEM* item = aCollector[i];

        if( !m_isFootprintEditor && item->Type() == PCB_PAD_T
            && ( !frame()->GetPcbNewSettings()->m_AllowFreePads || aForcePromotion ) )
        {
            if( !aCollector.HasItem( item->GetParent() ) )
                to_add.insert( item->GetParent() );

            aCollector.Remove( item );
        }
    }

    for( BOARD_ITEM* item : to_add )
        aCollector.Append( item );
}


void PCB_SELECTION_TOOL::FilterCollectorForMarkers( GENERAL_COLLECTOR& aCollector ) const
{
    // Iterate from the back so we don't have to worry about removals.
    for( int i = aCollector.GetCount() - 1; i >= 0; --i )
    {
        BOARD_ITEM* item = aCollector[i];

        if( item->Type() == PCB_MARKER_T )
            aCollector.Remove( item );
    }
}


void PCB_SELECTION_TOOL::FilterCollectorForFootprints( GENERAL_COLLECTOR& aCollector,
                                                       const VECTOR2I& aWhere ) const
{
    const RENDER_SETTINGS* settings = getView()->GetPainter()->GetSettings();
    BOX2D viewport = getView()->GetViewport();
    BOX2I extents = BOX2ISafe( viewport );

    bool need_direct_hit = false;
    FOOTPRINT* single_fp = nullptr;

    // If the designer is not modifying the existing selection AND we already have
    // a selection, then we only want to select items that are directly under the cursor.
    // This prevents us from being unable to clear the selection when zoomed into a footprint
    if( !m_additive && !m_subtractive && !m_exclusive_or && m_selection.GetSize() > 0 )
    {
        need_direct_hit = true;

        for( EDA_ITEM* item : m_selection )
        {
            FOOTPRINT* fp = nullptr;

            if( item->Type() != PCB_FOOTPRINT_T )
                fp = static_cast<BOARD_ITEM*>( item )->GetParentFootprint();
            else
                fp = static_cast<FOOTPRINT*>( item );

            // If the selection contains items that are not footprints, then don't restrict
            // whether we deselect the item or not.
            if( !fp )
            {
                single_fp = nullptr;
                break;
            }
            else if( !single_fp )
            {
                single_fp = fp;
            }
            // If the selection contains items from multiple footprints, then don't restrict
            // whether we deselect the item or not.
            else if( single_fp != fp )
            {
                single_fp = nullptr;
                break;
            }
        }
    }

    auto visibleLayers =
            [&]()
            {
                if( m_isFootprintEditor )
                {
                    LSET set;

                    for( PCB_LAYER_ID layer : LSET::AllLayersMask().Seq() )
                        set.set( layer, view()->IsLayerVisible( layer ) );

                    return set;
                }
                else
                {
                    return board()->GetVisibleLayers();
                }
            };

    LSET layers = visibleLayers();

    if( settings->GetHighContrast() )
    {
        layers.reset();

        const std::set<int> activeLayers = settings->GetHighContrastLayers();

        for( int layer : activeLayers )
        {
            if( layer >= 0 && layer < PCB_LAYER_ID_COUNT )
                layers.set( layer );
        }
    }

    // Iterate from the back so we don't have to worry about removals.
    for( int i = aCollector.GetCount() - 1; i >= 0; --i )
    {
        BOARD_ITEM* item = aCollector[i];
        FOOTPRINT* fp = dyn_cast<FOOTPRINT*>( item );

        if( !fp )
            continue;

        // Make footprints not difficult to select in high-contrast modes.
        if( layers[fp->GetLayer()] )
            continue;

        BOX2I bbox = fp->GetLayerBoundingBox( layers );

        // If the point clicked is not inside the visible bounding box, we can also remove it.
        if( !bbox.Contains( aWhere) )
            aCollector.Remove( item );

        bool has_hit = false;

        for( PCB_LAYER_ID layer : layers.Seq() )
        {
            if( fp->HitTestOnLayer( extents, false, layer ) )
            {
                has_hit = true;
                break;
            }
        }

        // If the point is outside of the visible bounding box, we can remove it.
        if( !has_hit )
        {
            aCollector.Remove( item );
        }
        // Do not require a direct hit on this fp if the existing selection only contains
        // this fp's items.  This allows you to have a selection of pads from a single
        // footprint and still click in the center of the footprint to select it.
        else if( single_fp )
        {
            if( fp == single_fp )
                continue;
        }
        else if( need_direct_hit )
        {
            has_hit = false;

            for( PCB_LAYER_ID layer : layers.Seq() )
            {
                if( fp->HitTestOnLayer( aWhere, layer ) )
                {
                    has_hit = true;
                    break;
                }
            }

            if( !has_hit )
                aCollector.Remove( item );
        }
    }
}


int PCB_SELECTION_TOOL::updateSelection( const TOOL_EVENT& aEvent )
{
    getView()->Update( &m_selection );
    getView()->Update( &m_enteredGroupOverlay );

    return 0;
}


int PCB_SELECTION_TOOL::SelectColumns( const TOOL_EVENT& aEvent )
{
    std::set<std::pair<PCB_TABLE*, int>> columns;
    bool                                 added = false;

    for( EDA_ITEM* item : m_selection )
    {
        if( PCB_TABLECELL* cell = dynamic_cast<PCB_TABLECELL*>( item ) )
        {
            PCB_TABLE* table = static_cast<PCB_TABLE*>( cell->GetParent() );
            columns.insert( std::make_pair( table, cell->GetColumn() ) );
        }
    }

    for( auto& [ table, col ] : columns )
    {
        for( int row = 0; row < table->GetRowCount(); ++row )
        {
            PCB_TABLECELL* cell = table->GetCell( row, col );

            if( !cell->IsSelected() )
            {
                select( table->GetCell( row, col ) );
                added = true;
            }
        }
    }

    if( added )
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    return 0;
}


int PCB_SELECTION_TOOL::SelectRows( const TOOL_EVENT& aEvent )
{
    std::set<std::pair<PCB_TABLE*, int>> rows;
    bool                                 added = false;

    for( EDA_ITEM* item : m_selection )
    {
        if( PCB_TABLECELL* cell = dynamic_cast<PCB_TABLECELL*>( item ) )
        {
            PCB_TABLE* table = static_cast<PCB_TABLE*>( cell->GetParent() );
            rows.insert( std::make_pair( table, cell->GetRow() ) );
        }
    }

    for( auto& [ table, row ] : rows )
    {
        for( int col = 0; col < table->GetColCount(); ++col )
        {
            PCB_TABLECELL* cell = table->GetCell( row, col );

            if( !cell->IsSelected() )
            {
                select( table->GetCell( row, col ) );
                added = true;
            }
        }
    }

    if( added )
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    return 0;
}


int PCB_SELECTION_TOOL::SelectTable( const TOOL_EVENT& aEvent )
{
    std::set<PCB_TABLE*> tables;
    bool                 added = false;

    for( EDA_ITEM* item : m_selection )
    {
        if( PCB_TABLECELL* cell = dynamic_cast<PCB_TABLECELL*>( item ) )
            tables.insert( static_cast<PCB_TABLE*>( cell->GetParent() ) );
    }

    ClearSelection();

    for( PCB_TABLE* table : tables )
    {
        if( !table->IsSelected() )
        {
            select( table );
            added = true;
        }
    }

    if( added )
        m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    return 0;
}


void PCB_SELECTION_TOOL::setTransitions()
{
    Go( &PCB_SELECTION_TOOL::UpdateMenu,          ACTIONS::updateMenu.MakeEvent() );

    Go( &PCB_SELECTION_TOOL::Main,                PCB_ACTIONS::selectionActivate.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::CursorSelection,     PCB_ACTIONS::selectionCursor.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::ClearSelection,      PCB_ACTIONS::selectionClear.MakeEvent() );

    Go( &PCB_SELECTION_TOOL::AddItemToSel,        PCB_ACTIONS::selectItem.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::AddItemsToSel,       PCB_ACTIONS::selectItems.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::RemoveItemFromSel,   PCB_ACTIONS::unselectItem.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::RemoveItemsFromSel,  PCB_ACTIONS::unselectItems.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::ReselectItem,        PCB_ACTIONS::reselectItem.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::SelectionMenu,       PCB_ACTIONS::selectionMenu.MakeEvent() );

    Go( &PCB_SELECTION_TOOL::filterSelection,     PCB_ACTIONS::filterSelection.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::expandConnection,    PCB_ACTIONS::selectConnection.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::unrouteSelected,     PCB_ACTIONS::unrouteSelected.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::selectNet,           PCB_ACTIONS::selectNet.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::selectNet,           PCB_ACTIONS::deselectNet.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::selectUnconnected,   PCB_ACTIONS::selectUnconnected.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::grabUnconnected,     PCB_ACTIONS::grabUnconnected.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::syncSelection,       PCB_ACTIONS::syncSelection.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::syncSelectionWithNets,
        PCB_ACTIONS::syncSelectionWithNets.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::selectSameSheet,     PCB_ACTIONS::selectSameSheet.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::selectSheetContents,
        PCB_ACTIONS::selectOnSheetFromEeschema.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::updateSelection,     EVENTS::SelectedItemsModified );
    Go( &PCB_SELECTION_TOOL::updateSelection,     EVENTS::SelectedItemsMoved );
    Go( &PCB_SELECTION_TOOL::SelectColumns,       ACTIONS::selectColumns.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::SelectRows,          ACTIONS::selectRows.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::SelectTable,         ACTIONS::selectTable.MakeEvent() );

    Go( &PCB_SELECTION_TOOL::SelectAll,           ACTIONS::selectAll.MakeEvent() );
    Go( &PCB_SELECTION_TOOL::UnselectAll,         ACTIONS::unselectAll.MakeEvent() );

    Go( &PCB_SELECTION_TOOL::disambiguateCursor,  EVENTS::DisambiguatePoint );
}
