/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <base_struct.h>
#include <bitmaps.h>

#include <gerber_collectors.h>

#include <class_draw_panel_gal.h>
#include <view/view.h>
#include <view/view_controls.h>
#include <view/view_group.h>
#include <painter.h>
#include <bitmaps.h>
#include <hotkeys.h>

#include <tool/tool_event.h>
#include <tool/tool_manager.h>
#include <preview_items/bright_box.h>
#include <preview_items/ruler_item.h>
#include <preview_items/selection_area.h>

#include <gerbview_id.h>
#include <gerbview_painter.h>

#include "selection_tool.h"
#include "gerbview_actions.h"

// Selection tool actions
TOOL_ACTION GERBVIEW_ACTIONS::selectionActivate( "gerbview.InteractiveSelection",
        AS_GLOBAL, 0,
        "", "", NULL, AF_ACTIVATE ); // No description, it is not supposed to be shown anywhere

TOOL_ACTION GERBVIEW_ACTIONS::selectionCursor( "gerbview.InteractiveSelection.Cursor",
        AS_GLOBAL, 0,
        "", "" );    // No description, it is not supposed to be shown anywhere

TOOL_ACTION GERBVIEW_ACTIONS::selectItem( "gerbview.InteractiveSelection.SelectItem",
        AS_GLOBAL, 0,
        "", "" );    // No description, it is not supposed to be shown anywhere

TOOL_ACTION GERBVIEW_ACTIONS::unselectItem( "gerbview.InteractiveSelection.UnselectItem",
        AS_GLOBAL, 0,
        "", "" );    // No description, it is not supposed to be shown anywhere

TOOL_ACTION GERBVIEW_ACTIONS::selectionClear( "gerbview.InteractiveSelection.Clear",
        AS_GLOBAL, 0,
        "", "" );    // No description, it is not supposed to be shown anywhere

TOOL_ACTION GERBVIEW_ACTIONS::measureTool( "gerbview.InteractiveSelection.measureTool",
        AS_GLOBAL, MD_CTRL + MD_SHIFT + 'M',
        _( "Measure tool" ), _( "Interactively measure distance between points" ),
        nullptr, AF_ACTIVATE );


class HIGHLIGHT_MENU: public CONTEXT_MENU
{
public:
    HIGHLIGHT_MENU()
    {
        SetIcon( net_highlight_schematic_xpm );
        SetTitle( _( "Highlight..." ) );
    }

private:

    void update() override
    {
        bool addSeparator = false;

        const auto& selection = getToolManager()->GetTool<GERBVIEW_SELECTION_TOOL>()->GetSelection();

        if( selection.Size() == 1 )
        {
            auto item = static_cast<GERBER_DRAW_ITEM*>( selection[0] );
            const auto& net_attr = item->GetNetAttributes();

            if( ( net_attr.m_NetAttribType & GBR_NETLIST_METADATA::GBR_NETINFO_PAD ) ||
                ( net_attr.m_NetAttribType & GBR_NETLIST_METADATA::GBR_NETINFO_CMP ) )
            {
                auto menuEntry = Add( GERBVIEW_ACTIONS::highlightComponent );
                menuEntry->SetItemLabel( wxString::Format( _( "Highlight items of component \"%s\"" ),
                                         GetChars( net_attr.m_Cmpref ) ) );
                addSeparator = true;
            }

            if( ( net_attr.m_NetAttribType & GBR_NETLIST_METADATA::GBR_NETINFO_NET ) )
            {
                auto menuEntry = Add( GERBVIEW_ACTIONS::highlightNet );
                menuEntry->SetItemLabel( wxString::Format( _( "Highlight items of net \"%s\"" ),
                                         GetChars( net_attr.m_Netname ) ) );
                addSeparator = true;
            }

            D_CODE* apertDescr = item->GetDcodeDescr();

            if( apertDescr && !apertDescr->m_AperFunction.IsEmpty() )
            {
                auto menuEntry = Add( GERBVIEW_ACTIONS::highlightAttribute );
                menuEntry->SetItemLabel( wxString::Format( _( "Highlight aperture type \"%s\"" ),
                                         GetChars( apertDescr->m_AperFunction ) ) );
                addSeparator = true;
            }
        }

        if( addSeparator )
            AppendSeparator();

        Add( GERBVIEW_ACTIONS::highlightClear );
    }

    CONTEXT_MENU* create() const override
    {
        return new HIGHLIGHT_MENU();
    }
};


GERBVIEW_SELECTION_TOOL::GERBVIEW_SELECTION_TOOL() :
        TOOL_INTERACTIVE( "gerbview.InteractiveSelection" ),
        m_frame( NULL ), m_additive( false ), m_subtractive( false ),
        m_multiple( false ),
        m_menu( *this )
{
    // these members are initialized to avoid warnings about non initialized vars
    m_preliminary = true;
}


GERBVIEW_SELECTION_TOOL::~GERBVIEW_SELECTION_TOOL()
{
    getView()->Remove( &m_selection );
}


bool GERBVIEW_SELECTION_TOOL::Init()
{
    auto selectMenu = std::make_shared<HIGHLIGHT_MENU>();
    selectMenu->SetTool( this );
    m_menu.AddSubMenu( selectMenu );

    auto& menu = m_menu.GetMenu();

    menu.AddMenu( selectMenu.get(), false );
    menu.AddSeparator( SELECTION_CONDITIONS::ShowAlways, 1000 );

    m_menu.AddStandardSubMenus( *getEditFrame<GERBVIEW_FRAME>() );

    return true;
}


void GERBVIEW_SELECTION_TOOL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<GERBVIEW_FRAME>();
    m_preliminary = true;

    if( aReason == TOOL_BASE::MODEL_RELOAD )
    {
        // Remove pointers to the selected items from containers
        // without changing their properties (as they are already deleted
        // while a new file is loaded)
        m_selection.Clear();
        getView()->GetPainter()->GetSettings()->SetHighlight( false );
    }
    else
        // Restore previous properties of selected items and remove them from containers
        clearSelection();

    // Reinsert the VIEW_GROUP, in case it was removed from the VIEW
    getView()->Remove( &m_selection );
    getView()->Add( &m_selection );
}


int GERBVIEW_SELECTION_TOOL::Main( const TOOL_EVENT& aEvent )
{
    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        // This is kind of hacky: activate RMB drag on any event.
        // There doesn't seem to be any other good way to tell when another tool
        // is canceled and control returns to the selection tool, except by the
        // fact that the selection tool starts to get events again.
        if( m_frame->GetToolId() == ID_NO_TOOL_SELECTED)
        {
            getViewControls()->SetAdditionalPanButtons( false, true );
        }

        // Disable RMB pan for other tools; they can re-enable if desired
        if( evt->IsActivate() )
        {
            getViewControls()->SetAdditionalPanButtons( false, false );
        }

        // single click? Select single object
        if( evt->IsClick( BUT_LEFT ) )
        {
            if( !m_additive )
                clearSelection();

            selectPoint( evt->Position() );
        }

        // right click? if there is any object - show the context menu
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            if( m_selection.Empty() )
            {
                selectPoint( evt->Position() );
                m_selection.SetIsHover( true );
            }

            m_menu.ShowContextMenu( m_selection );
        }

        else if( evt->IsCancel() || evt->Action() == TA_UNDO_REDO_PRE )
        {
            clearSelection();
        }

        else if( evt->Action() == TA_CONTEXT_MENU_CLOSED )
        {
            m_menu.CloseContextMenu( evt );
        }
    }

    // This tool is supposed to be active forever
    assert( false );

    return 0;
}


SELECTION& GERBVIEW_SELECTION_TOOL::GetSelection()
{
    return m_selection;
}


SELECTION& GERBVIEW_SELECTION_TOOL::RequestSelection( int aFlags )
{
    if( m_selection.Empty() )
    {
        m_toolMgr->RunAction( GERBVIEW_ACTIONS::selectionCursor, true, 0 );
        m_selection.SetIsHover( true );
    }

    // Be careful with iterators: items can be removed from list
    // that invalidate iterators.
    for( unsigned ii = 0; ii < m_selection.GetSize(); ii++ )
    {
        EDA_ITEM* item = m_selection[ii];

        if( ( aFlags & SELECTION_EDITABLE ) && item->Type() == PCB_MARKER_T )
        {
            unselect( static_cast<EDA_ITEM *>( item ) );
        }
    }

    return m_selection;
}


void GERBVIEW_SELECTION_TOOL::toggleSelection( EDA_ITEM* aItem )
{
    if( aItem->IsSelected() )
    {
        unselect( aItem );

        // Inform other potentially interested tools
        m_toolMgr->ProcessEvent( UnselectedEvent );
    }
    else
    {
        if( !m_additive )
            clearSelection();

        // Prevent selection of invisible or inactive items
        if( selectable( aItem ) )
        {
            select( aItem );

            // Inform other potentially interested tools
            m_toolMgr->ProcessEvent( SelectedEvent );
        }
    }

    m_frame->GetGalCanvas()->ForceRefresh();
}


bool GERBVIEW_SELECTION_TOOL::selectPoint( const VECTOR2I& aWhere, bool aOnDrag )
{
    EDA_ITEM* item = NULL;
    GERBER_COLLECTOR collector;
    EDA_ITEM* model = getModel<EDA_ITEM>();

    collector.Collect( model, GERBER_COLLECTOR::AllItems, wxPoint( aWhere.x, aWhere.y ) );

    bool anyCollected = collector.GetCount() != 0;

    // Remove unselectable items
    for( int i = collector.GetCount() - 1; i >= 0; --i )
    {
        if( !selectable( collector[i] ) )
            collector.Remove( i );
    }

    switch( collector.GetCount() )
    {
    case 0:
        if( !m_additive && anyCollected )
            clearSelection();

        return false;

    case 1:
        toggleSelection( collector[0] );

        return true;

    default:
        // Let's see if there is still disambiguation in selection..
        if( collector.GetCount() == 1 )
        {
            toggleSelection( collector[0] );

            return true;
        }
        else if( collector.GetCount() > 1 )
        {
            if( aOnDrag )
                Wait( TOOL_EVENT( TC_ANY, TA_MOUSE_UP, BUT_LEFT ) );

            item = disambiguationMenu( &collector );

            if( item )
            {
                toggleSelection( item );

                return true;
            }
        }
        break;
    }

    return false;
}


bool GERBVIEW_SELECTION_TOOL::selectCursor( bool aSelectAlways )
{
    if( aSelectAlways || m_selection.Empty() )
    {
        clearSelection();
        selectPoint( getViewControls()->GetCursorPosition( false ) );
    }

    return !m_selection.Empty();
}


bool GERBVIEW_SELECTION_TOOL::selectMultiple()
{
    bool cancelled = false;     // Was the tool cancelled while it was running?
    m_multiple = true;          // Multiple selection mode is active
    KIGFX::VIEW* view = getView();
    getViewControls()->SetAutoPan( true );

    KIGFX::PREVIEW::SELECTION_AREA area;
    view->Add( &area );

    while( OPT_TOOL_EVENT evt = Wait() )
    {
        if( evt->IsCancel() )
        {
            cancelled = true;
            break;
        }

        if( evt->IsDrag( BUT_LEFT ) )
        {

            // Start drawing a selection box
            area.SetOrigin( evt->DragOrigin() );
            area.SetEnd( evt->Position() );
            area.SetAdditive( m_additive );
            area.SetSubtractive( m_subtractive );

            view->SetVisible( &area, true );
            view->Update( &area );
        }

        if( evt->IsMouseUp( BUT_LEFT ) )
        {
            // End drawing the selection box
            view->SetVisible( &area, false );

            // Mark items within the selection box as selected
            std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> selectedItems;

            // Filter the view items based on the selection box
            BOX2I selectionBox = area.ViewBBox();
            view->Query( selectionBox, selectedItems );         // Get the list of selected items

            std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR>::iterator it, it_end;

            int width = area.GetEnd().x - area.GetOrigin().x;
            int height = area.GetEnd().y - area.GetOrigin().y;

            // Construct an EDA_RECT to determine EDA_ITEM selection
            EDA_RECT selectionRect( wxPoint( area.GetOrigin().x, area.GetOrigin().y ),
                                    wxSize( width, height ) );

            selectionRect.Normalize();

            for( it = selectedItems.begin(), it_end = selectedItems.end(); it != it_end; ++it )
            {
                auto item = static_cast<GERBER_DRAW_ITEM*>( it->first );

                if( !item || !selectable( item ) )
                    continue;

                /* Selection mode depends on direction of drag-selection:
                 * Left > Right : Select objects that are fully enclosed by selection
                 * Right > Left : Select objects that are crossed by selection
                 */

                if( width >= 0 )
                {
                    if( selectionBox.Contains( item->ViewBBox() ) )
                    {
                        if( m_subtractive )
                            unselect( item );
                        else
                            select( item );
                    }
                }
                else
                {
                    if( item->HitTest( selectionRect ) )
                    {
                        if( m_subtractive )
                            unselect( item );
                        else
                            select( item );
                    }

                }
            }

            if( m_selection.Size() == 1 )
                m_frame->SetCurItem( static_cast<GERBER_DRAW_ITEM*>( m_selection.Front() ) );
            else
                m_frame->SetCurItem( NULL );

            // Inform other potentially interested tools
            if( !m_selection.Empty() )
                m_toolMgr->ProcessEvent( SelectedEvent );

            break;  // Stop waiting for events
        }
    }

    // Stop drawing the selection box
    view->Remove( &area );
    m_multiple = false;         // Multiple selection mode is inactive
    getViewControls()->SetAutoPan( false );

    return cancelled;
}


void GERBVIEW_SELECTION_TOOL::setTransitions()
{
    Go( &GERBVIEW_SELECTION_TOOL::Main,             GERBVIEW_ACTIONS::selectionActivate.MakeEvent() );
    Go( &GERBVIEW_SELECTION_TOOL::CursorSelection,  GERBVIEW_ACTIONS::selectionCursor.MakeEvent() );
    Go( &GERBVIEW_SELECTION_TOOL::ClearSelection,   GERBVIEW_ACTIONS::selectionClear.MakeEvent() );
    Go( &GERBVIEW_SELECTION_TOOL::SelectItem,       GERBVIEW_ACTIONS::selectItem.MakeEvent() );
    Go( &GERBVIEW_SELECTION_TOOL::UnselectItem,     GERBVIEW_ACTIONS::unselectItem.MakeEvent() );
    Go( &GERBVIEW_SELECTION_TOOL::MeasureTool,      GERBVIEW_ACTIONS::measureTool.MakeEvent() );
}


int GERBVIEW_SELECTION_TOOL::CursorSelection( const TOOL_EVENT& aEvent )
{
    if( m_selection.Empty() )                        // Try to find an item that could be modified
    {
        selectCursor( true );

        clearSelection();
        return 0;
    }

    return 0;
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
        // Perform individual selection of each item
        // before processing the event.
        for( auto item : *items )
        {
            select( item );
        }

        m_toolMgr->ProcessEvent( SelectedEvent );
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
        m_toolMgr->ProcessEvent( SelectedEvent );
    }

    return 0;
}


int GERBVIEW_SELECTION_TOOL::UnselectItems( const TOOL_EVENT& aEvent )
{
    std::vector<EDA_ITEM*>* items = aEvent.Parameter<std::vector<EDA_ITEM*>*>();

    if( items )
    {
        // Perform individual unselection of each item
        // before processing the event
        for( auto item : *items )
        {
            unselect( item );
        }

        m_toolMgr->ProcessEvent( UnselectedEvent );
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
        m_toolMgr->ProcessEvent( UnselectedEvent );
    }

    return 0;
}


void GERBVIEW_SELECTION_TOOL::clearSelection()
{
    if( m_selection.Empty() )
        return;

    for( auto item : m_selection )
        unselectVisually( static_cast<EDA_ITEM*>( item ) );

    m_selection.Clear();

    m_frame->SetCurItem( NULL );

    // Inform other potentially interested tools
    m_toolMgr->ProcessEvent( ClearedEvent );
}


void GERBVIEW_SELECTION_TOOL::zoomFitSelection( void )
{
    //Should recalculate the view to zoom in on the selection
    auto selectionBox = m_selection.ViewBBox();
    auto canvas = m_frame->GetGalCanvas();
    auto view = getView();

    VECTOR2D screenSize = view->ToWorld( canvas->GetClientSize(), false );

    if( !( selectionBox.GetWidth() == 0 ) || !( selectionBox.GetHeight() == 0 ) )
    {
        VECTOR2D vsize = selectionBox.GetSize();
        double scale = view->GetScale() / std::max( fabs( vsize.x / screenSize.x ),
                fabs( vsize.y / screenSize.y ) );
        view->SetScale( scale );
        view->SetCenter( selectionBox.Centre() );
        view->Add( &m_selection );
    }

    m_frame->GetGalCanvas()->ForceRefresh();
}


EDA_ITEM* GERBVIEW_SELECTION_TOOL::disambiguationMenu( GERBER_COLLECTOR* aCollector )
{
    EDA_ITEM* current = NULL;
    KIGFX::VIEW_GROUP highlightGroup;
    CONTEXT_MENU menu;

    highlightGroup.SetLayer( LAYER_GP_OVERLAY );
    getView()->Add( &highlightGroup );

    int limit = std::min( 10, aCollector->GetCount() );

    for( int i = 0; i < limit; ++i )
    {
        wxString text;
        EDA_ITEM* item = ( *aCollector )[i];
        text = item->GetSelectMenuText();
        menu.Add( text, i + 1 );
    }

    menu.SetTitle( _( "Clarify selection" ) );
    menu.DisplayTitle( true );
    SetContextMenu( &menu, CMENU_NOW );

    while( OPT_TOOL_EVENT evt = Wait() )
    {
        if( evt->Action() == TA_CONTEXT_MENU_UPDATE )
        {
            if( current )
            {
                current->ClearBrightened();
                getView()->Hide( current, false );
                highlightGroup.Remove( current );
                getView()->MarkTargetDirty( KIGFX::TARGET_OVERLAY );
            }

            int id = *evt->GetCommandId();

            // User has pointed an item, so show it in a different way
            if( id > 0 && id <= limit )
            {
                current = ( *aCollector )[id - 1];
                current->SetBrightened();
                getView()->Hide( current, true );
                highlightGroup.Add( current );
                getView()->MarkTargetDirty( KIGFX::TARGET_OVERLAY );
            }
            else
            {
                current = NULL;
            }
        }
        else if( evt->Action() == TA_CONTEXT_MENU_CHOICE )
        {
            OPT<int> id = evt->GetCommandId();

            // User has selected an item, so this one will be returned
            if( id && ( *id > 0 ) )
                current = ( *aCollector )[*id - 1];
            else
                current = NULL;

            break;
        }
    }

    if( current && current->IsBrightened() )
    {
        current->ClearBrightened();
        getView()->Hide( current, false );
        getView()->MarkTargetDirty( KIGFX::TARGET_OVERLAY );
    }

    getView()->Remove( &highlightGroup );

    return current;
}


bool GERBVIEW_SELECTION_TOOL::selectable( const EDA_ITEM* aItem ) const
{
    auto item = static_cast<const GERBER_DRAW_ITEM*>( aItem );

    if( item->GetLayerPolarity() )
    {
        // Don't allow selection of invisible negative items
        auto rs = static_cast<KIGFX::GERBVIEW_RENDER_SETTINGS*>( getView()->GetPainter()->GetSettings() );
        if( !rs->IsShowNegativeItems() )
            return false;
    }

    return getEditFrame<GERBVIEW_FRAME>()->IsLayerVisible( item->GetLayer() );
}


void GERBVIEW_SELECTION_TOOL::select( EDA_ITEM* aItem )
{
    if( aItem->IsSelected() )
    {
        return;
    }

    m_selection.Add( aItem );
    getView()->Add( &m_selection );
    selectVisually( aItem );

    if( m_selection.Size() == 1 )
    {
        // Set as the current item, so the information about selection is displayed
        m_frame->SetCurItem( static_cast<GERBER_DRAW_ITEM*>( aItem ), true );
    }
    else if( m_selection.Size() == 2 )  // Check only for 2, so it will not be
    {                                   // called for every next selected item
        // If multiple items are selected, do not show the information about the selected item
        m_frame->SetCurItem( NULL, true );
    }
}


void GERBVIEW_SELECTION_TOOL::unselect( EDA_ITEM* aItem )
{
    if( !aItem->IsSelected() )
        return;

    unselectVisually( aItem );
    m_selection.Remove( aItem );

    if( m_selection.Empty() )
    {
        m_frame->SetCurItem( NULL );
        getView()->Remove( &m_selection );
    }
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


bool GERBVIEW_SELECTION_TOOL::selectionContains( const VECTOR2I& aPoint ) const
{
    const unsigned GRIP_MARGIN = 20;
    VECTOR2D margin = getView()->ToWorld( VECTOR2D( GRIP_MARGIN, GRIP_MARGIN ), false );

    // Check if the point is located within any of the currently selected items bounding boxes
    for( auto item : m_selection )
    {
        BOX2I itemBox = item->ViewBBox();
        itemBox.Inflate( margin.x, margin.y );    // Give some margin for gripping an item

        if( itemBox.Contains( aPoint ) )
            return true;
    }

    return false;
}


int GERBVIEW_SELECTION_TOOL::MeasureTool( const TOOL_EVENT& aEvent )
{
    auto& view = *getView();
    auto& controls = *getViewControls();

    Activate();
    getEditFrame<GERBVIEW_FRAME>()->SetToolID( ID_TB_MEASUREMENT_TOOL,
                        wxCURSOR_PENCIL, _( "Measure distance between two points" ) );

    KIGFX::PREVIEW::TWO_POINT_GEOMETRY_MANAGER twoPtMgr;
    KIGFX::PREVIEW::RULER_ITEM ruler( twoPtMgr );

    view.Add( &ruler );
    view.SetVisible( &ruler, false );

    bool originSet = false;

    controls.ShowCursor( true );
    controls.SetSnapping( true );
    getViewControls()->SetAdditionalPanButtons( false, true );

    while( auto evt = Wait() )
    {
        const VECTOR2I cursorPos = controls.GetCursorPosition();

        if( evt->IsCancel() || evt->IsActivate() )
        {
            break;
        }

        // click or drag starts
        else if( !originSet &&
                ( evt->IsDrag( BUT_LEFT ) || evt->IsClick( BUT_LEFT ) ) )
        {
            if( !evt->IsDrag( BUT_LEFT ) )
            {
                twoPtMgr.SetOrigin( cursorPos );
                twoPtMgr.SetEnd( cursorPos );
            }

            controls.CaptureCursor( true );
            controls.SetAutoPan( true );

            originSet = true;
        }

        else if( !originSet && evt->IsMotion() )
        {
            // make sure the origin is set before a drag starts
            // otherwise you can miss a step
            twoPtMgr.SetOrigin( cursorPos );
            twoPtMgr.SetEnd( cursorPos );
        }

        // second click or mouse up after drag ends
        else if( originSet &&
                ( evt->IsClick( BUT_LEFT ) || evt->IsMouseUp( BUT_LEFT ) ) )
        {
            originSet = false;

            controls.SetAutoPan( false );
            controls.CaptureCursor( false );

            view.SetVisible( &ruler, false );
        }

        // move or drag when origin set updates rules
        else if( originSet &&
                ( evt->IsMotion() || evt->IsDrag( BUT_LEFT ) ) )
        {
            twoPtMgr.SetAngleSnap( evt->Modifier( MD_CTRL ) );
            twoPtMgr.SetEnd( cursorPos );

            view.SetVisible( &ruler, true );
            view.Update( &ruler, KIGFX::GEOMETRY );
        }

        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( m_selection );
        }
    }

    view.SetVisible( &ruler, false );
    view.Remove( &ruler );
    getViewControls()->SetAdditionalPanButtons( false, false );

    getEditFrame<GERBVIEW_FRAME>()->SetToolID( ID_NO_TOOL_SELECTED, wxCURSOR_DEFAULT, wxEmptyString );

    return 0;
}


VECTOR2I SELECTION::GetCenter() const
{
    VECTOR2I centre;

    if( Size() == 1 )
    {
        centre = static_cast<GERBER_DRAW_ITEM*>( Front() )->GetPosition();
    }
    else
    {
        EDA_RECT bbox = Front()->GetBoundingBox();
        auto i = m_items.begin();
        ++i;

        for( ; i != m_items.end(); ++i )
        {
            bbox.Merge( (*i)->GetBoundingBox() );
        }

        centre = bbox.Centre();
    }

    return centre;
}


const BOX2I SELECTION::ViewBBox() const
{
    EDA_RECT eda_bbox;

    if( Size() == 1 )
    {
        eda_bbox = Front()->GetBoundingBox();
    }
    else if( Size() > 1 )
    {
        eda_bbox = Front()->GetBoundingBox();
        auto i = m_items.begin();
        ++i;

        for( ; i != m_items.end(); ++i )
        {
            eda_bbox.Merge( (*i)->GetBoundingBox() );
        }
    }

    return BOX2I( eda_bbox.GetOrigin(), eda_bbox.GetSize() );
}


const KIGFX::VIEW_GROUP::ITEMS SELECTION::updateDrawList() const
{
    std::vector<VIEW_ITEM*> items;

    for( auto item : m_items )
        items.push_back( item );

    return items;
}



const TOOL_EVENT GERBVIEW_SELECTION_TOOL::SelectedEvent( TC_MESSAGE, TA_ACTION, "gerbview.InteractiveSelection.selected" );
const TOOL_EVENT GERBVIEW_SELECTION_TOOL::UnselectedEvent( TC_MESSAGE, TA_ACTION, "gerbview.InteractiveSelection.unselected" );
const TOOL_EVENT GERBVIEW_SELECTION_TOOL::ClearedEvent( TC_MESSAGE, TA_ACTION, "gerbview.InteractiveSelection.cleared" );
