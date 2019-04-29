/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "sch_drawing_tool.h"
#include "sch_selection_tool.h"
#include <sch_actions.h>

#include <sch_edit_frame.h>
#include <sch_view.h>
#include <class_draw_panel_gal.h>
#include <project.h>
#include <id.h>
#include <eeschema_id.h>
#include <confirm.h>
#include <view/view_group.h>
#include <view/view_controls.h>
#include <view/view.h>
#include <tool/tool_manager.h>
#include <hotkeys.h>
#include <sch_component.h>
#include <sch_no_connect.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_bus_entry.h>
#include <sch_text.h>
#include <sch_sheet.h>
#include <sch_bitmap.h>
#include <class_library.h>


// Drawing tool actions
TOOL_ACTION SCH_ACTIONS::placeSymbol( "eeschema.InteractiveDrawing.placeSymbol",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_NEW_COMPONENT ),
        _( "Add Symbol" ), _( "Add a symbol" ),
        nullptr, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::placePower( "eeschema.InteractiveDrawing.placePowerPort",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_NEW_POWER ),
        _( "Add Power" ), _( "Add a power port" ),
        nullptr, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::startWire( "eeschema.InteractiveDrawing.startWire",
        AS_GLOBAL, 0,
        _( "Begin Wire" ), _( "Start drawing a wire" ),
        add_line_xpm, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::drawWire( "eeschema.InteractiveDrawing.drawWire",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_BEGIN_WIRE ),
        _( "Add Wire" ), _( "Add a wire" ),
        nullptr, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::startBus( "eeschema.InteractiveDrawing.startBus",
        AS_GLOBAL, 0,
        _( "Begin Bus" ), _( "Start drawing a bus" ),
        add_bus_xpm, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::drawBus( "eeschema.InteractiveDrawing.drawBus",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_BEGIN_BUS ),
        _( "Add Bus" ), _( "Add a bus" ),
        nullptr, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::unfoldBus( "eeschema.InteractiveDrawing.unfoldBus",
        AS_GLOBAL, 0, "", "", NULL, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::placeNoConnect( "eeschema.InteractiveDrawing.placeNoConnect",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_NOCONN_FLAG ),
        _( "Add No Connect Flag" ), _( "Add a no-connection flag" ),
        nullptr, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::placeJunction( "eeschema.InteractiveDrawing.placeJunction",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_JUNCTION ),
        _( "Add Junction" ), _( "Add a junction" ),
        NULL, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::placeBusWireEntry( "eeschema.InteractiveDrawing.placeBusWireEntry",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_WIRE_ENTRY ),
        _( "Add Wire to Bus Entry" ), _( "Add a wire entry to a bus" ),
        NULL, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::placeBusBusEntry( "eeschema.InteractiveDrawing.placeBusBusEntry",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_BUS_ENTRY ),
        _( "Add Bus to Bus Entry" ), _( "Add a bus entry to a bus" ),
        NULL, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::placeLabel( "eeschema.InteractiveDrawing.placeLabel",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_LABEL ),
        _( "Add Label" ), _( "Add a net label" ),
        NULL, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::placeHierarchicalLabel( "eeschema.InteractiveDrawing.placeHierarchicalLabel",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_HLABEL ),
        _( "Add Hierarchical Label" ), _( "Add a hierarchical sheet label" ),
        NULL, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::drawSheet( "eeschema.InteractiveDrawing.drawSheet",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_HIER_SHEET ),
        _( "Add Sheet" ), _( "Add a hierarchical sheet" ),
        NULL, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::resizeSheet( "eeschema.InteractiveDrawing.resizeSheet",
        AS_GLOBAL, 0, _( "Resize Sheet" ), _( "Resize hierarchical sheet" ),
        NULL, AF_NONE );

TOOL_ACTION SCH_ACTIONS::placeSheetPin( "eeschema.InteractiveDrawing.placeSheetPin",
        AS_GLOBAL, 0, _( "Add Sheet Pin" ), _( "Add a sheet pin" ),
        NULL, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::importSheetPin( "eeschema.InteractiveDrawing.importSheetPin",
        AS_GLOBAL, 0, _( "Import Sheet Pin" ), _( "Import a hierarchical sheet pin" ),
        NULL, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::placeGlobalLabel( "eeschema.InteractiveDrawing.placeGlobalLabel",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_GLABEL ),
        _( "Add Global Label" ), _( "Add a global label" ),
        NULL, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::placeSchematicText( "eeschema.InteractiveDrawing.placeSchematicText",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_GRAPHIC_TEXT ),
        _( "Add Text" ), _( "Add text" ),
        NULL, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::startLines( "eeschema.InteractiveDrawing.startLines",
        AS_GLOBAL, 0, _( "Begin Lines" ), _( "Start drawing connected graphic lines" ),
        add_line_xpm, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::drawLines( "eeschema.InteractiveDrawing.drawLines",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_GRAPHIC_POLYLINE ),
        _( "Add Lines" ), _( "Add connected graphic lines" ),
        nullptr, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::placeImage( "eeschema.InteractiveDrawing.placeImage",
        AS_GLOBAL, 0, _( "Add Image" ), _( "Add bitmap image" ),
        nullptr, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::finishLineWireOrBus( "eeschema.InteractiveDrawing.finishLineWireOrBus",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_END_CURR_LINEWIREBUS ),
        _( "Finish Wire or Bus" ), _( "Complete drawing at current segment" ),
        checked_ok_xpm, AF_NONE );

TOOL_ACTION SCH_ACTIONS::finishWire( "eeschema.InteractiveDrawing.finishWire",
        AS_GLOBAL, 0, _( "Finish Wire" ), _( "Complete wire with current segment" ),
        checked_ok_xpm, AF_NONE );

TOOL_ACTION SCH_ACTIONS::finishBus( "eeschema.InteractiveDrawing.finishBus",
        AS_GLOBAL, 0, _( "Finish Bus" ), _( "Complete bus with current segment" ),
        checked_ok_xpm, AF_NONE );

TOOL_ACTION SCH_ACTIONS::finishLine( "eeschema.InteractiveDrawing.finishLine",
        AS_GLOBAL, 0, _( "Finish Lines" ), _( "Complete connected lines with current segment" ),
        checked_ok_xpm, AF_NONE );

TOOL_ACTION SCH_ACTIONS::finishSheet( "eeschema.InteractiveDrawing.finishSheet",
        AS_GLOBAL, 0, _( "Finish Sheet" ), _( "Finish drawing sheet" ),
        checked_ok_xpm, AF_NONE );

TOOL_ACTION SCH_ACTIONS::addJunction( "eeschema.InteractiveEditing.addJunction",
        AS_GLOBAL, 0, _( "Add Junction" ), _( "Add a wire or bus junction" ),
        add_junction_xpm, AF_NONE );

TOOL_ACTION SCH_ACTIONS::addLabel( "eeschema.InteractiveEditing.addLabel",
        AS_GLOBAL, 0, _( "Add Label" ), _( "Add a label to a wire or bus" ),
        add_line_label_xpm, AF_NONE );

TOOL_ACTION SCH_ACTIONS::addGlobalLabel( "eeschema.InteractiveEditing.addGlobalLabel",
        AS_GLOBAL, 0, _( "Add Global Label" ), _( "Add a global label to a wire or bus" ),
        add_glabel_xpm, AF_NONE );


SCH_DRAWING_TOOL::SCH_DRAWING_TOOL() :
    TOOL_INTERACTIVE( "eeschema.InteractiveDrawing" ),
    m_selectionTool( nullptr ),
    m_view( nullptr ),
    m_controls( nullptr ),
    m_frame( nullptr ),
    m_menu( *this )
{
    m_busUnfold = {};
};


SCH_DRAWING_TOOL::~SCH_DRAWING_TOOL()
{
}


bool SCH_DRAWING_TOOL::Init()
{
    m_frame = getEditFrame<SCH_EDIT_FRAME>();
    m_selectionTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();

    auto activeToolCondition = [ this ] ( const SELECTION& aSel ) {
        return ( m_frame->GetToolId() != ID_NO_TOOL_SELECTED );
    };

    auto idleWireCondition = [ this ] ( const SELECTION& aSel ) {
        return ( m_frame->GetToolId() == ID_WIRE_BUTT && aSel.Empty() );
    };

    auto idleBusCondition = [ this ] ( const SELECTION& aSel ) {
        return ( m_frame->GetToolId() == ID_BUS_BUTT && aSel.Empty() );
    };

    auto idleLineCondition = [ this ] ( const SELECTION& aSel ) {
        return ( m_frame->GetToolId() == ID_LINE_COMMENT_BUTT && aSel.Empty() );
    };

    auto drawingWireCondition = [ this ] ( const SELECTION& aSel ) {
        return ( m_frame->GetToolId() == ID_WIRE_BUTT
                     && aSel.GetSize() == 1
                     && dynamic_cast<SCH_LINE*>( aSel.GetItem( 0 ) ) );
    };

    auto drawingBusCondition = [ this ] ( const SELECTION& aSel ) {
        return ( m_frame->GetToolId() == ID_BUS_BUTT
                     && aSel.GetSize() == 1
                     && dynamic_cast<SCH_LINE*>( aSel.GetItem( 0 ) ) );
    };

    auto drawingLineCondition = [ this ] ( const SELECTION& aSel ) {
        return ( m_frame->GetToolId() == ID_LINE_COMMENT_BUTT
                     && aSel.GetSize() == 1
                     && dynamic_cast<SCH_LINE*>( aSel.GetItem( 0 ) ) );
    };

    auto& ctxMenu = m_menu.GetMenu();

    // cancel current tool goes in main context menu at the top if present
    ctxMenu.AddItem( ACTIONS::cancelInteractive, activeToolCondition, 1 );

    ctxMenu.AddItem( SCH_ACTIONS::startWire, idleWireCondition, 1 );
    ctxMenu.AddItem( SCH_ACTIONS::startBus, idleBusCondition, 1 );
    ctxMenu.AddItem( SCH_ACTIONS::startLines, idleLineCondition, 1 );
    ctxMenu.AddItem( SCH_ACTIONS::finishWire, drawingWireCondition, 1 );
    ctxMenu.AddItem( SCH_ACTIONS::finishBus, drawingBusCondition, 1 );
    ctxMenu.AddItem( SCH_ACTIONS::finishLine, drawingLineCondition, 1 );

    ctxMenu.AddSeparator( activeToolCondition, 1000 );
    m_menu.AddStandardSubMenus( m_frame );

    return true;
}


void SCH_DRAWING_TOOL::Reset( RESET_REASON aReason )
{
    // Init variables used by every drawing tool
    m_view = static_cast<KIGFX::SCH_VIEW*>( getView() );
    m_controls = getViewControls();
    m_frame = getEditFrame<SCH_EDIT_FRAME>();
}


int SCH_DRAWING_TOOL::AddJunction( const TOOL_EVENT& aEvent )
{
    m_toolMgr->RunAction( SCH_ACTIONS::selectionClear, true );

    m_frame->GetCanvas()->MoveCursorToCrossHair();
    m_frame->AddJunction( m_frame->GetCrossHairPosition() );

    return 0;
}


int SCH_DRAWING_TOOL::AddLabel( const TOOL_EVENT& aEvent )
{
    return doAddItem( SCH_LABEL_T );
}


int SCH_DRAWING_TOOL::AddGlobalLabel( const TOOL_EVENT& aEvent )
{
    return doAddItem( SCH_GLOBAL_LABEL_T );
}


int SCH_DRAWING_TOOL::doAddItem( KICAD_T aType )
{
    m_toolMgr->RunAction( SCH_ACTIONS::selectionClear, true );

    SCH_ITEM* item = nullptr;

    switch( aType )
    {
    case SCH_LABEL_T:        item = m_frame->CreateNewText( LAYER_LOCLABEL );   break;
    case SCH_GLOBAL_LABEL_T: item = m_frame->CreateNewText( LAYER_GLOBLABEL );  break;
    default:                 wxFAIL_MSG( "doAddItem(): unknown type" );
    }

    m_frame->AddItemToScreen( item );
    m_frame->SetNoToolSelected();

    return 0;
}


// History lists for PlaceSymbol()
static SCH_BASE_FRAME::HISTORY_LIST s_SymbolHistoryList;
static SCH_BASE_FRAME::HISTORY_LIST s_PowerHistoryList;


int SCH_DRAWING_TOOL::PlaceSymbol( const TOOL_EVENT& aEvent )
{
    SCH_COMPONENT* component = aEvent.Parameter<SCH_COMPONENT*>();

    m_frame->SetToolID( ID_SCH_PLACE_COMPONENT, wxCURSOR_PENCIL, _( "Add Symbol" ) );

    return doPlaceComponent( component, nullptr, s_SymbolHistoryList );
}


int SCH_DRAWING_TOOL::PlacePower( const TOOL_EVENT& aEvent )
{
    SCH_COMPONENT* component = aEvent.Parameter<SCH_COMPONENT*>();
    SCHLIB_FILTER  filter;

    filter.FilterPowerParts( true );
    m_frame->SetToolID( ID_PLACE_POWER_BUTT, wxCURSOR_PENCIL, _( "Add Power" ) );

    return doPlaceComponent( component, &filter, s_PowerHistoryList );
}


int SCH_DRAWING_TOOL::doPlaceComponent( SCH_COMPONENT* aComponent, SCHLIB_FILTER* aFilter,
                                        SCH_BASE_FRAME::HISTORY_LIST aHistoryList )
{
    VECTOR2I cursorPos = m_controls->GetCursorPosition();

    m_toolMgr->RunAction( SCH_ACTIONS::selectionClear, true );
    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );

    Activate();

    // Add all the drawable parts to preview
    if( aComponent )
    {
        aComponent->SetPosition( (wxPoint)cursorPos );
        m_view->ClearPreview();
        m_view->AddToPreview( aComponent->Clone() );
    }

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        cursorPos = m_controls->GetCursorPosition( !evt->Modifier( MD_ALT ) );

        if( TOOL_EVT_UTILS::IsCancelInteractive( evt.get() ) )
        {
            if( aComponent )
            {
                m_toolMgr->RunAction( SCH_ACTIONS::selectionClear, true );
                getModel<SCH_SCREEN>()->SetCurItem( nullptr );
                m_view->ClearPreview();
                m_view->ClearHiddenFlags();
                delete aComponent;
                aComponent = nullptr;
            }
            else
                break;

            if( evt->IsActivate() )  // now finish unconditionally
                break;
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( !aComponent )
            {
                // Pick the module to be placed
                m_frame->SetRepeatItem( NULL );
                m_frame->GetCanvas()->SetIgnoreMouseEvents( true );

                auto sel = m_frame->SelectComponentFromLibTree( aFilter, aHistoryList, true, 1, 1,
                                                                m_frame->GetShowFootprintPreviews() );

                // Restore cursor after dialog
                m_frame->GetCanvas()->MoveCursorToCrossHair();
                m_frame->GetCanvas()->SetIgnoreMouseEvents( false );

                LIB_PART* part = nullptr;

                if( sel.LibId.IsValid() )
                    part = m_frame->GetLibPart( sel.LibId );

                if( !part )
                    continue;

                aComponent = new SCH_COMPONENT( *part, sel.LibId, g_CurrentSheet, sel.Unit,
                                                sel.Convert, (wxPoint)cursorPos, true );

                // Be sure the link to the corresponding LIB_PART is OK:
                aComponent->Resolve( *m_frame->Prj().SchSymbolLibTable() );

                // Set any fields that have been modified
                for( auto const& i : sel.Fields )
                {
                    auto field = aComponent->GetField( i.first );

                    if( field )
                        field->SetText( i.second );
                }

                if( m_frame->GetAutoplaceFields() )
                    aComponent->AutoplaceFields( /* aScreen */ NULL, /* aManual */ false );

                aComponent->SetFlags( IS_MOVED );
                m_frame->SetRepeatItem( aComponent );
                m_frame->GetScreen()->SetCurItem( aComponent );
                m_view->ClearPreview();
                m_view->AddToPreview( aComponent->Clone() );

                m_toolMgr->RunAction( SCH_ACTIONS::selectItem, true, aComponent );

                m_controls->SetCursorPosition( cursorPos, false );
            }
            else
            {
                m_view->ClearPreview();

                m_frame->AddItemToScreen( aComponent );

                aComponent = nullptr;
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !aComponent )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu.ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else if( aComponent && ( evt->IsAction( &SCH_ACTIONS::refreshPreview ) || evt->IsMotion() ) )
        {
            aComponent->SetPosition( (wxPoint)cursorPos );
            m_view->ClearPreview();
            m_view->AddToPreview( aComponent->Clone() );
        }

        // Enable autopanning and cursor capture only when there is a module to be placed
        m_controls->SetAutoPan( !!aComponent );
        m_controls->CaptureCursor( !!aComponent );
    }

    m_frame->SetNoToolSelected();

    return 0;
}


int SCH_DRAWING_TOOL::PlaceImage( const TOOL_EVENT& aEvent )
{
    SCH_BITMAP* image = aEvent.Parameter<SCH_BITMAP*>();

    m_frame->SetToolID( ID_ADD_IMAGE_BUTT, wxCURSOR_PENCIL, _( "Add image" ) );

    VECTOR2I cursorPos = m_controls->GetCursorPosition();

    m_toolMgr->RunAction( SCH_ACTIONS::selectionClear, true );
    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );

    Activate();

    // Add all the drawable parts to preview
    if( image )
    {
        image->SetPosition( (wxPoint)cursorPos );
        m_view->ClearPreview();
        m_view->AddToPreview( image->Clone() );
    }

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        cursorPos = m_controls->GetCursorPosition( !evt->Modifier( MD_ALT ) );

        if( TOOL_EVT_UTILS::IsCancelInteractive( evt.get() ) )
        {
            if( image )
            {
                m_toolMgr->RunAction( SCH_ACTIONS::selectionClear, true );
                getModel<SCH_SCREEN>()->SetCurItem( nullptr );
                m_view->ClearPreview();
                m_view->ClearHiddenFlags();
                delete image;
                image = nullptr;
            }
            else
                break;

            if( evt->IsActivate() )  // now finish unconditionally
                break;
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( !image )
            {
                m_frame->GetCanvas()->SetIgnoreMouseEvents( true );

                wxFileDialog dlg( m_frame, _( "Choose Image" ), wxEmptyString, wxEmptyString,
                                  _( "Image Files " ) + wxImage::GetImageExtWildcard(), wxFD_OPEN );

                if( dlg.ShowModal() != wxID_OK )
                    continue;

                // Restore cursor after dialog
                m_frame->GetCanvas()->MoveCursorToCrossHair();
                m_frame->GetCanvas()->SetIgnoreMouseEvents( false );

                wxString fullFilename = dlg.GetPath();

                if( !wxFileExists( fullFilename ) )
                {
                    wxMessageBox( _( "Couldn't load image from \"%s\"" ), fullFilename );
                    continue;
                }

                image = new SCH_BITMAP( (wxPoint)cursorPos );

                if( !image->ReadImageFile( fullFilename ) )
                {
                    wxMessageBox( _( "Couldn't load image from \"%s\"" ), fullFilename );
                    image = nullptr;
                    delete image;
                    continue;
                }

                image->SetFlags( IS_MOVED );
                m_frame->SetRepeatItem( image );
                m_frame->GetScreen()->SetCurItem( image );
                m_view->ClearPreview();
                m_view->AddToPreview( image->Clone() );

                m_toolMgr->RunAction( SCH_ACTIONS::selectItem, true, aComponent );

                m_controls->SetCursorPosition( cursorPos, false );
            }
            else
            {
                m_view->ClearPreview();

                m_frame->AddItemToScreen( image );

                image = nullptr;
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !image )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu.ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else if( image && ( evt->IsAction( &SCH_ACTIONS::refreshPreview ) || evt->IsMotion() ) )
        {
            image->SetPosition( (wxPoint)cursorPos );
            m_view->ClearPreview();
            m_view->AddToPreview( image->Clone() );
        }

        // Enable autopanning and cursor capture only when there is a module to be placed
        m_controls->SetAutoPan( !!image );
        m_controls->CaptureCursor( !!image );
    }

    m_frame->SetNoToolSelected();

    return 0;
}


int SCH_DRAWING_TOOL::PlaceNoConnect( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_NOCONN_BUTT, wxCURSOR_PENCIL, _( "Add no connect" ) );
    return doSingleClickPlace( SCH_NO_CONNECT_T );
}


int SCH_DRAWING_TOOL::PlaceJunction( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_JUNCTION_BUTT, wxCURSOR_PENCIL, _( "Add junction" ) );
    return doSingleClickPlace( SCH_JUNCTION_T );
}


int SCH_DRAWING_TOOL::PlaceBusWireEntry( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_WIRETOBUS_ENTRY_BUTT, wxCURSOR_PENCIL, _( "Add wire to bus entry" ) );
    return doSingleClickPlace( SCH_BUS_WIRE_ENTRY_T );
}


int SCH_DRAWING_TOOL::PlaceBusBusEntry( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_BUSTOBUS_ENTRY_BUTT, wxCURSOR_PENCIL, _( "Add bus to bus entry" ) );
    return doSingleClickPlace( SCH_BUS_BUS_ENTRY_T );
}


int SCH_DRAWING_TOOL::doSingleClickPlace( KICAD_T aType )
{
    m_toolMgr->RunAction( SCH_ACTIONS::selectionClear, true );
    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );

    Activate();

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        wxPoint cursorPos = (wxPoint)m_controls->GetCursorPosition( !evt->Modifier( MD_ALT ) );

        if( TOOL_EVT_UTILS::IsCancelInteractive( evt.get() ) )
        {
            break;
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            SCH_ITEM* item = nullptr;

            if( !m_frame->GetScreen()->GetItem( cursorPos, 0, aType ) )
            {
                switch( aType )
                {
                case SCH_NO_CONNECT_T:
                    item = new SCH_NO_CONNECT( cursorPos );
                    break;
                case SCH_JUNCTION_T:
                    item = m_frame->AddJunction( cursorPos );
                    break;
                case SCH_BUS_WIRE_ENTRY_T:
                    item = new SCH_BUS_WIRE_ENTRY( cursorPos, g_lastBusEntryShape );
                    break;
                case SCH_BUS_BUS_ENTRY_T:
                    item = new SCH_BUS_BUS_ENTRY( cursorPos, g_lastBusEntryShape );
                    break;
                default:
                    wxFAIL_MSG( "doSingleClickPlace(): unknown type" );
                }
            }

            if( item )
            {
                item->SetFlags( IS_NEW );
                m_frame->AddItemToScreen( item );
                m_frame->SaveCopyInUndoList( item, UR_NEW );

                m_frame->SetRepeatItem( item );
                m_frame->GetScreen()->SetCurItem( item );

                m_frame->SchematicCleanUp();
                m_frame->TestDanglingEnds();
                m_frame->OnModify();
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            SELECTION emptySelection;

            m_menu.ShowContextMenu( emptySelection );
        }
    }

    m_frame->SetNoToolSelected();

    return 0;
}


int SCH_DRAWING_TOOL::PlaceLabel( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_LABEL_BUTT, wxCURSOR_PENCIL, _( "Add net label" ) );
    return doTwoClickPlace( SCH_LABEL_T );
}


int SCH_DRAWING_TOOL::PlaceGlobalLabel( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_GLOBALLABEL_BUTT, wxCURSOR_PENCIL, _( "Add global label" ) );
    return doTwoClickPlace( SCH_GLOBAL_LABEL_T );
}


int SCH_DRAWING_TOOL::PlaceHierarchicalLabel( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_HIERLABEL_BUTT, wxCURSOR_PENCIL, _( "Add hierarchical label" ) );
    return doTwoClickPlace( SCH_HIER_LABEL_T );
}


int SCH_DRAWING_TOOL::PlaceSheetPin( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_SHEET_PIN_BUTT, wxCURSOR_PENCIL, _( "Add sheet pins" ) );
    return doTwoClickPlace( SCH_SHEET_PIN_T );
}


int SCH_DRAWING_TOOL::ImportSheetPin( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_IMPORT_HLABEL_BUTT, wxCURSOR_PENCIL, _( "Import sheet pins" ) );
    return doTwoClickPlace( SCH_SHEET_PIN_T );
}


int SCH_DRAWING_TOOL::PlaceSchematicText( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_TEXT_COMMENT_BUTT, wxCURSOR_PENCIL, _( "Add text" ) );
    return doTwoClickPlace( SCH_TEXT_T );
}


int SCH_DRAWING_TOOL::doTwoClickPlace( KICAD_T aType )
{
    VECTOR2I  cursorPos = m_controls->GetCursorPosition();
    SCH_ITEM* item = nullptr;

    m_toolMgr->RunAction( SCH_ACTIONS::selectionClear, true );
    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );

    Activate();

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        cursorPos = m_controls->GetCursorPosition( !evt->Modifier( MD_ALT ) );

        if( TOOL_EVT_UTILS::IsCancelInteractive( evt.get() ) )
        {
            if( item )
            {
                m_toolMgr->RunAction( SCH_ACTIONS::selectionClear, true );
                getModel<SCH_SCREEN>()->SetCurItem( nullptr );
                m_view->ClearPreview();
                m_view->ClearHiddenFlags();
                delete item;
                item = nullptr;
            }
            else
                break;

            if( evt->IsActivate() )  // now finish unconditionally
                break;
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            // First click creates...
            if( !item )
            {
                m_frame->SetRepeatItem( NULL );
                m_frame->GetCanvas()->SetIgnoreMouseEvents( true );

                switch( aType )
                {
                case SCH_LABEL_T:
                    item = m_frame->CreateNewText( LAYER_LOCLABEL );
                    break;
                case SCH_HIER_LABEL_T:
                    item = m_frame->CreateNewText( LAYER_HIERLABEL );
                    break;
                case SCH_GLOBAL_LABEL_T:
                    item = m_frame->CreateNewText( LAYER_GLOBLABEL );
                    break;
                case SCH_TEXT_T:
                    item = m_frame->CreateNewText( LAYER_NOTES );
                    break;
                case SCH_SHEET_PIN_T:
                    item = m_selectionTool->SelectPoint( cursorPos, SCH_COLLECTOR::SheetsAndSheetLabels );

                    if( item )
                    {
                        if( m_frame->GetToolId() == ID_IMPORT_HLABEL_BUTT )
                            item = m_frame->ImportSheetPin( (SCH_SHEET*) item );
                        else
                            item = m_frame->CreateSheetPin( (SCH_SHEET*) item );
                    }
                    break;
                default:
                    wxFAIL_MSG( "doTwoClickPlace(): unknown type" );
                }

                // Restore cursor after dialog
                m_frame->GetCanvas()->MoveCursorToCrossHair();
                m_frame->GetCanvas()->SetIgnoreMouseEvents( false );

                if( item )
                {
                    m_toolMgr->RunAction( SCH_ACTIONS::selectItem, true, item );

                    item->SetFlags( IS_MOVED );
                    m_view->ClearPreview();
                    m_view->AddToPreview( item->Clone() );
                }

                m_controls->SetCursorPosition( cursorPos, false );
            }

            // ... and second click places:
            else
            {
                m_view->ClearPreview();

                m_frame->AddItemToScreen( item );

                item = nullptr;
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !item )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu.ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else if( TOOL_EVT_UTILS::IsSelectionEvent( evt.get() ) )
        {
            // This happens if our text was replaced out from under us by CovertTextType()
            SELECTION& selection = m_selectionTool->GetSelection();

            if( selection.GetSize() == 1 )
            {
                item = (SCH_ITEM*) selection.GetItem( 0 );
                m_view->ClearPreview();
                m_view->AddToPreview( item->Clone() );
            }
            else
                item = nullptr;
        }
        else if( item && ( evt->IsAction( &SCH_ACTIONS::refreshPreview ) || evt->IsMotion() ) )
        {
            item->SetPosition( (wxPoint)cursorPos );
            m_view->ClearPreview();
            m_view->AddToPreview( item->Clone() );
        }

        // Enable autopanning and cursor capture only when there is a module to be placed
        m_controls->SetAutoPan( !!item );
        m_controls->CaptureCursor( !!item );
    }

    m_frame->SetNoToolSelected();

    return 0;
}


int SCH_DRAWING_TOOL::StartWire( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_WIRE_BUTT, wxCURSOR_PENCIL, _( "Add wire" ) );

    m_frame->GetCanvas()->MoveCursorToCrossHair();
    SCH_LINE* segment = startSegments( LAYER_WIRE, m_frame->GetCrossHairPosition() );
    return doDrawSegments( LAYER_WIRE, segment );
}


int SCH_DRAWING_TOOL::DrawWire( const TOOL_EVENT& aEvent )
{
    if( m_frame->GetToolId() == ID_WIRE_BUTT )
        return StartWire( aEvent );
    else
    {
        m_frame->SetToolID( ID_WIRE_BUTT, wxCURSOR_PENCIL, _( "Add wire" ) );
        return doDrawSegments( LAYER_WIRE, nullptr );
    }
}


int SCH_DRAWING_TOOL::StartBus( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_BUS_BUTT, wxCURSOR_PENCIL, _( "Add bus" ) );

    m_frame->GetCanvas()->MoveCursorToCrossHair();
    SCH_LINE* segment = startSegments( LAYER_BUS, m_frame->GetCrossHairPosition() );
    return doDrawSegments( LAYER_BUS, segment );
}


int SCH_DRAWING_TOOL::DrawBus( const TOOL_EVENT& aEvent )
{
    if( m_frame->GetToolId() == ID_BUS_BUTT )
        return StartBus( aEvent );
    else
    {
        m_frame->SetToolID( ID_BUS_BUTT, wxCURSOR_PENCIL, _( "Add bus" ) );
        return doDrawSegments( LAYER_BUS, nullptr );
    }
}


int SCH_DRAWING_TOOL::UnfoldBus( const TOOL_EVENT& aEvent )
{
    wxString net = *aEvent.Parameter<wxString*>();
    wxPoint  pos = m_frame->GetCrossHairPosition();

    /**
     * Unfolding a bus consists of the following user inputs:
     * 1) User selects a bus to unfold (see AddMenusForBus())
     *    We land in this event handler.
     *
     * 2) User clicks to set the net label location (handled by BeginSegment())
     *    Before this first click, the posture of the bus entry  follows the
     *    mouse cursor in X and Y (handled by DrawSegment())
     *
     * 3) User is now in normal wiring mode and can exit in any normal way.
     */

    wxASSERT( !m_busUnfold.in_progress );

    m_busUnfold.entry = new SCH_BUS_WIRE_ENTRY( pos, '\\' );
    m_busUnfold.entry->SetParent( m_frame->GetScreen() );
    m_frame->AddToScreen( m_busUnfold.entry );

    m_busUnfold.label = new SCH_LABEL( m_busUnfold.entry->m_End(), net );
    m_busUnfold.label->SetTextSize( wxSize( GetDefaultTextSize(), GetDefaultTextSize() ) );
    m_busUnfold.label->SetLabelSpinStyle( 0 );
    m_busUnfold.label->SetParent( m_frame->GetScreen() );

    m_busUnfold.in_progress = true;
    m_busUnfold.origin = pos;
    m_busUnfold.net_name = net;

    m_frame->SetToolID( ID_WIRE_BUTT, wxCURSOR_PENCIL, _( "Add wire" ) );

    m_frame->SetCrossHairPosition( m_busUnfold.entry->m_End() );
    SCH_LINE* segment = startSegments( LAYER_WIRE, m_busUnfold.entry->m_End() );
    return doDrawSegments( LAYER_WIRE, segment );
}


int SCH_DRAWING_TOOL::StartLines( const TOOL_EVENT& aEvent)
{
    m_frame->SetToolID( ID_LINE_COMMENT_BUTT, wxCURSOR_PENCIL, _( "Add lines" ) );

    m_frame->GetCanvas()->MoveCursorToCrossHair();
    SCH_LINE* segment = startSegments( LAYER_NOTES, m_frame->GetCrossHairPosition() );
    return doDrawSegments( LAYER_BUS, segment );
}


int SCH_DRAWING_TOOL::DrawLines( const TOOL_EVENT& aEvent)
{
    if( m_frame->GetToolId() == ID_LINE_COMMENT_BUTT )
        return StartLines( aEvent );
    else
    {
        m_frame->SetToolID( ID_LINE_COMMENT_BUTT, wxCURSOR_PENCIL, _( "Add lines" ) );
        return doDrawSegments( LAYER_NOTES, nullptr );
    }
}


// Storage for the line segments while drawing
static DLIST<SCH_LINE> s_wires;


/**
 * In a contiguous list of wires, remove wires that backtrack over the previous
 * wire. Example:
 *
 * Wire is added:
 * ---------------------------------------->
 *
 * A second wire backtracks over it:
 * -------------------<====================>
 *
 * RemoveBacktracks is called:
 * ------------------->
 */
static void RemoveBacktracks( DLIST<SCH_LINE>& aWires )
{
    SCH_LINE* next = nullptr;
    std::vector<SCH_LINE*> last_lines;

    for( SCH_LINE* line = aWires.GetFirst(); line; line = next )
    {
        next = line->Next();

        if( line->IsNull() )
        {
            delete s_wires.Remove( line );
            continue;
        }

        if( !last_lines.empty() )
        {
            SCH_LINE* last_line = last_lines[last_lines.size() - 1];
            bool contiguous = ( last_line->GetEndPoint() == line->GetStartPoint() );
            bool backtracks = IsPointOnSegment( last_line->GetStartPoint(),
                                                last_line->GetEndPoint(), line->GetEndPoint() );
            bool total_backtrack = ( last_line->GetStartPoint() == line->GetEndPoint() );

            if( contiguous && backtracks )
            {
                if( total_backtrack )
                {
                    delete s_wires.Remove( last_line );
                    delete s_wires.Remove( line );
                    last_lines.pop_back();
                }
                else
                {
                    last_line->SetEndPoint( line->GetEndPoint() );
                    delete s_wires.Remove( line );
                }
            }
            else
            {
                last_lines.push_back( line );
            }
        }
        else
        {
            last_lines.push_back( line );
        }
    }
}


/**
 * A helper function to find any sheet pins at the specified position.
 */
static const SCH_SHEET_PIN* getSheetPin( SCH_SCREEN* aScreen, const wxPoint& aPosition )
{
    for( SCH_ITEM* item = aScreen->GetDrawItems(); item; item = item->Next() )
    {
        if( item->Type() == SCH_SHEET_T )
        {
            SCH_SHEET* sheet = (SCH_SHEET*) item;

            for( const SCH_SHEET_PIN& pin : sheet->GetPins() )
            {
                if( pin.GetPosition() == aPosition )
                    return &pin;
            }
        }
    }

    return nullptr;
}


/**
 * Function ComputeBreakPoint
 * computes the middle coordinate for 2 segments from the start point to \a aPosition
 * with the segments kept in the horizontal or vertical axis only.
 *
 * @param aSegment A pointer to a #SCH_LINE object containing the first line break point
 *                 to compute.
 * @param aPosition A reference to a wxPoint object containing the coordinates of the
 *                  position used to calculate the line break point.
 */
static void computeBreakPoint( SCH_SCREEN* aScreen, SCH_LINE* aSegment, wxPoint& aPosition )
{
    wxCHECK_RET( aSegment != nullptr, wxT( "Cannot compute break point of NULL line segment." ) );

    SCH_LINE* nextSegment = aSegment->Next();

    wxPoint midPoint;
    int iDx = aSegment->GetEndPoint().x - aSegment->GetStartPoint().x;
    int iDy = aSegment->GetEndPoint().y - aSegment->GetStartPoint().y;

    const SCH_SHEET_PIN* connectedPin = getSheetPin( aScreen, aSegment->GetStartPoint() );
    auto force = connectedPin ? connectedPin->GetEdge() : SCH_SHEET_PIN::SHEET_UNDEFINED_SIDE;

    if( force == SCH_SHEET_PIN::SHEET_LEFT_SIDE || force == SCH_SHEET_PIN::SHEET_RIGHT_SIDE )
    {
        if( aPosition.x == connectedPin->GetPosition().x )  // push outside sheet boundary
        {
            int direction = ( force == SCH_SHEET_PIN::SHEET_LEFT_SIDE ) ? -1 : 1;
            aPosition.x += int( aScreen->GetGridSize().x * direction );
        }

        midPoint.x = aPosition.x;
        midPoint.y = aSegment->GetStartPoint().y;     // force horizontal
    }
    else if( iDy != 0 )    // keep the first segment orientation (vertical)
    {
        midPoint.x = aSegment->GetStartPoint().x;
        midPoint.y = aPosition.y;
    }
    else if( iDx != 0 )    // keep the first segment orientation (horizontal)
    {
        midPoint.x = aPosition.x;
        midPoint.y = aSegment->GetStartPoint().y;
    }
    else
    {
        if( std::abs( aPosition.x - aSegment->GetStartPoint().x ) <
            std::abs( aPosition.y - aSegment->GetStartPoint().y ) )
        {
            midPoint.x = aSegment->GetStartPoint().x;
            midPoint.y = aPosition.y;
        }
        else
        {
            midPoint.x = aPosition.x;
            midPoint.y = aSegment->GetStartPoint().y;
        }
    }

    aSegment->SetEndPoint( midPoint );
    nextSegment->SetStartPoint( midPoint );
    nextSegment->SetEndPoint( aPosition );
}


int SCH_DRAWING_TOOL::doDrawSegments( int aType, SCH_LINE* aSegment )
{
    bool forceHV = m_frame->GetForceHVLines();
    SCH_SCREEN* screen = m_frame->GetScreen();

    m_toolMgr->RunAction( SCH_ACTIONS::selectionClear, true );
    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );

    Activate();

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        wxPoint cursorPos = (wxPoint)m_controls->GetCursorPosition( !evt->Modifier( MD_ALT ) );

        if( TOOL_EVT_UTILS::IsCancelInteractive( evt.get() ) )
        {
            if( aSegment || m_busUnfold.in_progress )
            {
                aSegment = nullptr;
                s_wires.DeleteAll();

                if( m_busUnfold.entry )
                    m_frame->RemoveFromScreen( m_busUnfold.entry );

                if( m_busUnfold.label && m_busUnfold.label_placed )
                    m_frame->RemoveFromScreen( m_busUnfold.label );

                delete m_busUnfold.entry;
                delete m_busUnfold.label;
                m_busUnfold = {};

                m_view->ClearPreview();
                m_view->ShowPreview( false );
                m_view->ClearHiddenFlags();

                // Clear flags used in edit functions.
                screen->ClearDrawingState();
                screen->SetCurItem( nullptr );

                if( !evt->IsActivate() )
                    continue;
            }

            if( evt->IsAction( &SCH_ACTIONS::drawWire ) && aType == LAYER_WIRE )
                ; // don't cancel tool; we're going to re-enter
            else if( evt->IsAction( &SCH_ACTIONS::drawBus ) && aType == LAYER_BUS )
                ; // don't cancel tool; we're going to re-enter
            else
                m_frame->SetNoToolSelected();

            break;
        }
        else if( evt->IsAction( &SCH_ACTIONS::finishLineWireOrBus )
                     || evt->IsAction( &SCH_ACTIONS::finishWire )
                     || evt->IsAction( &SCH_ACTIONS::finishBus )
                     || evt->IsAction( &SCH_ACTIONS::finishLine ) )
        {
            if( aSegment || m_busUnfold.in_progress )
            {
                finishSegments();
                aSegment = nullptr;
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !aSegment )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu.ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else if( evt->IsClick( BUT_LEFT ) || ( aSegment && evt->IsDblClick( BUT_LEFT ) ) )
        {
            // First click when unfolding places the label and wire-to-bus entry
            if( m_busUnfold.in_progress && !m_busUnfold.label_placed )
            {
                wxASSERT( aType == LAYER_WIRE );

                m_frame->AddToScreen( m_busUnfold.label );
                m_busUnfold.label_placed = true;
            }

            if( !aSegment )
            {
                aSegment = startSegments( aType, cursorPos );
            }
            // Create a new segment if we're out of previously-created ones
            else if( !aSegment->IsNull() || ( forceHV && !aSegment->Back()->IsNull() ) )
            {
                // Terminate the command if the end point is on a pin, junction, or another
                // wire or bus.
                if( !m_busUnfold.in_progress
                        && screen->IsTerminalPoint( cursorPos, aSegment->GetLayer() ) )
                {
                    finishSegments();
                    aSegment = nullptr;
                }
                else
                {
                    aSegment->SetEndPoint( cursorPos );
                    aSegment->ClearFlags( IS_NEW );
                    aSegment->SetFlags( SELECTED );

                    // Create a new segment, and chain it after the current new segment.
                    aSegment = new SCH_LINE( *aSegment );
                    aSegment->SetFlags( IS_NEW );
                    aSegment->SetStartPoint( cursorPos );
                    s_wires.PushBack( aSegment );
                    screen->SetCurItem( aSegment );
                }
            }

            if( evt->IsDblClick( BUT_LEFT ) )
            {
                finishSegments();
                aSegment = nullptr;
            }
        }
        else if( evt->IsMotion() )
        {
            m_view->ClearPreview();

            // Update the bus unfold posture based on the mouse movement
            if( m_busUnfold.in_progress && !m_busUnfold.label_placed )
            {
                wxPoint cursor_delta = cursorPos - m_busUnfold.origin;
                SCH_BUS_WIRE_ENTRY* entry = m_busUnfold.entry;

                bool offset = ( cursor_delta.x < 0 );
                char shape = ( offset ? ( ( cursor_delta.y >= 0 ) ? '/' : '\\' )
                                      : ( ( cursor_delta.y >= 0 ) ? '\\' : '/' ) );

                // Erase and redraw if necessary
                if( shape != entry->GetBusEntryShape() || offset != m_busUnfold.offset )
                {
                    entry->SetBusEntryShape( shape );
                    wxPoint entry_pos = m_busUnfold.origin;

                    if( offset )
                        entry_pos -= entry->GetSize();

                    entry->SetPosition( entry_pos );
                    m_busUnfold.offset = offset;

                    m_frame->RefreshItem( entry );

                    wxPoint wire_start = offset ? entry->GetPosition() : entry->m_End();
                    s_wires.begin()->SetStartPoint( wire_start );
                }

                // Update the label "ghost" position
                m_busUnfold.label->SetPosition( cursorPos );
                m_view->AddToPreview( m_busUnfold.label->Clone() );
            }

            if( aSegment )
            {
                // Coerce the line to vertical or horizontal if necessary
                if( forceHV )
                    computeBreakPoint( screen, aSegment->Back(), cursorPos );
                else
                    aSegment->SetEndPoint( cursorPos );
            }

            for( auto seg = s_wires.begin(); seg; seg = seg->Next() )
            {
                if( !seg->IsNull() )  // Add to preview if segment length != 0
                    m_view->AddToPreview( seg->Clone() );
            }
        }

        // Enable autopanning and cursor capture only when there is a segment to be placed
        m_controls->SetAutoPan( !!aSegment );
        m_controls->CaptureCursor( !!aSegment );
    }

    return 0;
}


SCH_LINE* SCH_DRAWING_TOOL::startSegments( int aType, const wxPoint& aPos )
{
    SCH_LINE* segment = nullptr;
    bool      forceHV = m_frame->GetForceHVLines();

    switch( aType )
    {
    default:         segment = new SCH_LINE( aPos, LAYER_NOTES ); break;
    case LAYER_WIRE: segment = new SCH_LINE( aPos, LAYER_WIRE );  break;
    case LAYER_BUS:  segment = new SCH_LINE( aPos, LAYER_BUS );   break;
    }

    segment->SetFlags( IS_NEW );
    s_wires.PushBack( segment );
    m_frame->GetScreen()->SetCurItem( segment );

    // We need 2 segments to go from a given start pin to an end point when the
    // horizontal and vertical lines only switch is on.
    if( forceHV )
    {
        segment = new SCH_LINE( *segment );
        segment->SetFlags( IS_NEW );
        s_wires.PushBack( segment );
        m_frame->GetScreen()->SetCurItem( segment );
    }

    return segment;
}


void SCH_DRAWING_TOOL::finishSegments()
{
    PICKED_ITEMS_LIST itemList;

    // Remove segments backtracking over others
    RemoveBacktracks( s_wires );

    // Collect the possible connection points for the new lines
    std::vector< wxPoint > connections;
    std::vector< wxPoint > new_ends;
    m_frame->GetSchematicConnections( connections );

    // Check each new segment for possible junctions and add/split if needed
    for( SCH_LINE* wire = s_wires.GetFirst(); wire; wire = wire->Next() )
    {
        if( wire->GetFlags() & SKIP_STRUCT )
            continue;

        wire->GetConnectionPoints( new_ends );

        for( auto i : connections )
        {
            if( IsPointOnSegment( wire->GetStartPoint(), wire->GetEndPoint(), i ) )
                new_ends.push_back( i );
        }
        itemList.PushItem( ITEM_PICKER( wire, UR_NEW ) );
    }

    if( m_busUnfold.in_progress && m_busUnfold.label_placed )
    {
        wxASSERT( m_busUnfold.entry && m_busUnfold.label );

        itemList.PushItem( ITEM_PICKER( m_busUnfold.entry, UR_NEW ) );
        itemList.PushItem( ITEM_PICKER( m_busUnfold.label, UR_NEW ) );
    }

    // Get the last non-null wire (this is the last created segment).
    m_frame->SetRepeatItem( s_wires.GetLast() );

    // Add the new wires
    while( s_wires.GetFirst() )
        m_frame->AddToScreen( s_wires.PopFront() );

    m_view->ClearPreview();
    m_view->ShowPreview( false );
    m_view->ClearHiddenFlags();

    m_controls->CaptureCursor( false );
    m_controls->SetAutoPan( false );

    m_frame->SaveCopyInUndoList( itemList, UR_NEW );

    // Correct and remove segments that need to be merged.
    m_frame->SchematicCleanUp( true );

    for( auto item = m_frame->GetScreen()->GetDrawItems(); item; item = item->Next() )
    {
        if( item->Type() != SCH_COMPONENT_T )
            continue;

        std::vector< wxPoint > pts;
        item->GetConnectionPoints( pts );

        if( pts.size() > 2 )
            continue;

        for( auto i = pts.begin(); i != pts.end(); i++ )
        {
            for( auto j = i + 1; j != pts.end(); j++ )
                m_frame->TrimWire( *i, *j, true );
        }
    }

    for( auto i : new_ends )
    {
        if( m_frame->GetScreen()->IsJunctionNeeded( i, true ) )
            m_frame->AddJunction( i, true, false );
    }

    if( m_busUnfold.in_progress )
        m_busUnfold = {};

    m_frame->TestDanglingEnds();

    m_frame->GetScreen()->ClearDrawingState();
    m_frame->GetScreen()->SetCurItem( nullptr );
    m_frame->OnModify();
}


int SCH_DRAWING_TOOL::DrawSheet( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_SHEET_SYMBOL_BUTT, wxCURSOR_PENCIL, _( "Add sheet" ) );
    return doDrawSheet( nullptr );
}


int SCH_DRAWING_TOOL::ResizeSheet( const TOOL_EVENT& aEvent )
{
    SCH_SHEET* sheet = dynamic_cast<SCH_SHEET*>( m_frame->GetScreen()->GetCurItem() );

    if( sheet )
    {
        m_frame->SetToolID( ID_POPUP_SCH_RESIZE_SHEET, wxCURSOR_PENCIL, _( "Resize sheet" ) );
        doDrawSheet( sheet );
    }
    else
        wxFAIL_MSG( "No sheet to resize" );

    return 0;
}


int SCH_DRAWING_TOOL::doDrawSheet( SCH_SHEET *aSheet )
{
    m_toolMgr->RunAction( SCH_ACTIONS::selectionClear, true );
    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );

    if( aSheet )
    {
        m_frame->SetUndoItem( aSheet );
        aSheet->SetFlags( IS_RESIZED );

        m_view->Hide( aSheet );
        m_view->AddToPreview( aSheet->Clone() );

        m_controls->SetCursorPosition( aSheet->GetResizePosition() );
        m_controls->SetCrossHairCursorPosition( m_controls->GetCursorPosition() );
    }

    Activate();

    // Main loop: keep receiving events
    while( auto evt = Wait() )
    {
        wxPoint cursorPos = (wxPoint)m_controls->GetCursorPosition( !evt->Modifier( MD_ALT ) );

        if( TOOL_EVT_UTILS::IsCancelInteractive( evt.get() ) )
        {
            m_view->ClearPreview();

            if( m_frame->GetToolId() == ID_POPUP_SCH_RESIZE_SHEET )
            {
                if( dynamic_cast<SCH_SHEET*>( m_frame->GetUndoItem() ) )
                {
                    delete aSheet;
                    aSheet = (SCH_SHEET*)m_frame->GetUndoItem();
                    m_frame->SetUndoItem( nullptr );
                }

                m_frame->AddToScreen( aSheet );
                aSheet->ClearFlags();
                m_view->Hide( aSheet, false );

                m_frame->GetScreen()->SetCurItem( nullptr );
                break;  // resize sheet is a single-shot command, not a reusable tool
            }
            else if( aSheet )
            {
                delete aSheet;
                aSheet = nullptr;
                m_frame->GetScreen()->SetCurItem( nullptr );
            }
            else
                break;

            if( evt->IsActivate() )
                break;      // exit unconditionally
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( m_frame->GetToolId() == ID_POPUP_SCH_RESIZE_SHEET )
            {
                m_frame->SaveUndoItemInUndoList( aSheet );
                m_frame->OnModify();

                m_view->ClearPreview();
                m_view->Hide( aSheet, false );
                m_frame->RefreshItem( aSheet );

                m_frame->GetScreen()->SetCurItem( nullptr );
                break;  // resize sheet is a single-shot command; when we're done we're done
            }
            else if( !aSheet )
            {
                aSheet = new SCH_SHEET( cursorPos );

                aSheet->SetFlags( IS_NEW | IS_RESIZED );
                aSheet->SetTimeStamp( GetNewTimeStamp() );
                aSheet->SetParent( m_frame->GetScreen() );
                aSheet->SetScreen( NULL );

                m_frame->SetRepeatItem( nullptr );
                m_frame->GetScreen()->SetCurItem( aSheet );
            }
            else
            {
                m_frame->AddItemToScreen( aSheet );
                aSheet = nullptr;

                m_view->ClearPreview();
                m_frame->GetScreen()->SetCurItem( nullptr );
            }
        }
        else if( evt->IsAction( &SCH_ACTIONS::finishSheet ) )
        {
            if( aSheet )
            {
                m_frame->AddItemToScreen( aSheet );
                aSheet = nullptr;

                m_view->ClearPreview();
                m_frame->GetScreen()->SetCurItem( nullptr );
            }
        }
        else if( evt->IsMotion() )
        {
            m_view->ClearPreview();

            if( aSheet )
            {
                wxPoint pos = aSheet->GetPosition();
                wxPoint size = cursorPos - pos;

                // If the sheet doesn't have any pins, clamp the minimum size to the defaults.
                size.x = std::max( size.x, MIN_SHEET_WIDTH );
                size.y = std::max( size.y, MIN_SHEET_HEIGHT );

                if( aSheet->HasPins() )
                {
                    int gridSizeX = KiROUND( m_frame->GetScreen()->GetGridSize().x );
                    int gridSizeY = KiROUND( m_frame->GetScreen()->GetGridSize().y );

                    // Use the pin positions to clamp the minimum width and height.
                    size.x = std::max( size.x, aSheet->GetMinWidth() + gridSizeX );
                    size.y = std::max( size.y, aSheet->GetMinHeight() + gridSizeY );
                }

                wxPoint grid = m_frame->GetNearestGridPosition( pos + size );
                aSheet->Resize( wxSize( grid.x - pos.x, grid.y - pos.y ) );

                m_view->AddToPreview( aSheet->Clone() );
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !aSheet )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu.ShowContextMenu( m_selectionTool->GetSelection() );
        }

        // Enable autopanning and cursor capture only when there is a sheet to be placed
        m_controls->SetAutoPan( !!aSheet );
        m_controls->CaptureCursor( !!aSheet );
    }

    m_frame->SetNoToolSelected();

    return 0;
}


void SCH_DRAWING_TOOL::setTransitions()
{
    Go( &SCH_DRAWING_TOOL::PlaceSymbol,           SCH_ACTIONS::placeSymbol.MakeEvent() );
    Go( &SCH_DRAWING_TOOL::PlacePower,            SCH_ACTIONS::placePower.MakeEvent() );
    Go( &SCH_DRAWING_TOOL::DrawWire,              SCH_ACTIONS::drawWire.MakeEvent() );
    Go( &SCH_DRAWING_TOOL::DrawBus,               SCH_ACTIONS::drawBus.MakeEvent() );
    Go( &SCH_DRAWING_TOOL::UnfoldBus,             SCH_ACTIONS::unfoldBus.MakeEvent() );
    Go( &SCH_DRAWING_TOOL::PlaceNoConnect,        SCH_ACTIONS::placeNoConnect.MakeEvent() );
    Go( &SCH_DRAWING_TOOL::PlaceJunction,         SCH_ACTIONS::placeJunction.MakeEvent() );
    Go( &SCH_DRAWING_TOOL::PlaceBusWireEntry,     SCH_ACTIONS::placeBusWireEntry.MakeEvent() );
    Go( &SCH_DRAWING_TOOL::PlaceBusBusEntry,      SCH_ACTIONS::placeBusBusEntry.MakeEvent() );
    Go( &SCH_DRAWING_TOOL::PlaceLabel,            SCH_ACTIONS::placeLabel.MakeEvent() );
    Go( &SCH_DRAWING_TOOL::PlaceHierarchicalLabel,SCH_ACTIONS::placeHierarchicalLabel.MakeEvent() );
    Go( &SCH_DRAWING_TOOL::PlaceGlobalLabel,      SCH_ACTIONS::placeGlobalLabel.MakeEvent() );
    Go( &SCH_DRAWING_TOOL::DrawSheet,             SCH_ACTIONS::drawSheet.MakeEvent() );
    Go( &SCH_DRAWING_TOOL::ResizeSheet,           SCH_ACTIONS::resizeSheet.MakeEvent() );
    Go( &SCH_DRAWING_TOOL::PlaceSheetPin,         SCH_ACTIONS::placeSheetPin.MakeEvent() );
    Go( &SCH_DRAWING_TOOL::ImportSheetPin,        SCH_ACTIONS::importSheetPin.MakeEvent() );
    Go( &SCH_DRAWING_TOOL::PlaceSchematicText,    SCH_ACTIONS::placeSchematicText.MakeEvent() );
    Go( &SCH_DRAWING_TOOL::DrawLines,             SCH_ACTIONS::drawLines.MakeEvent() );
    Go( &SCH_DRAWING_TOOL::PlaceImage,            SCH_ACTIONS::placeImage.MakeEvent() );

    Go( &SCH_DRAWING_TOOL::StartWire,             SCH_ACTIONS::startWire.MakeEvent() );
    Go( &SCH_DRAWING_TOOL::StartBus,              SCH_ACTIONS::startBus.MakeEvent() );
    Go( &SCH_DRAWING_TOOL::StartLines,            SCH_ACTIONS::startLines.MakeEvent() );
    Go( &SCH_DRAWING_TOOL::AddJunction,           SCH_ACTIONS::addJunction.MakeEvent() );
    Go( &SCH_DRAWING_TOOL::AddLabel,              SCH_ACTIONS::addLabel.MakeEvent() );
    Go( &SCH_DRAWING_TOOL::AddGlobalLabel,        SCH_ACTIONS::addGlobalLabel.MakeEvent() );
}
