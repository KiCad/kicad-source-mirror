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

#include "sch_drawing_tools.h"
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
#include <sch_bus_entry.h>
#include <sch_text.h>
#include <sch_sheet.h>
#include <sch_bitmap.h>
#include <class_library.h>


// Drawing tool actions
TOOL_ACTION SCH_ACTIONS::placeSymbol( "eeschema.InteractiveDrawing.placeSymbol",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_NEW_COMPONENT ),
        _( "Add Symbol" ), _( "Add a symbol" ),
        add_component_xpm, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::placePower( "eeschema.InteractiveDrawing.placePowerPort",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_NEW_POWER ),
        _( "Add Power" ), _( "Add a power port" ),
        add_power_xpm, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::placeNoConnect( "eeschema.InteractiveDrawing.placeNoConnect",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_NOCONN_FLAG ),
        _( "Add No Connect Flag" ), _( "Add a no-connection flag" ),
        noconn_xpm, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::placeJunction( "eeschema.InteractiveDrawing.placeJunction",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_JUNCTION ),
        _( "Add Junction" ), _( "Add a junction" ),
        add_junction_xpm, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::placeBusWireEntry( "eeschema.InteractiveDrawing.placeBusWireEntry",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_WIRE_ENTRY ),
        _( "Add Wire to Bus Entry" ), _( "Add a wire entry to a bus" ),
        add_line2bus_xpm, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::placeBusBusEntry( "eeschema.InteractiveDrawing.placeBusBusEntry",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_BUS_ENTRY ),
        _( "Add Bus to Bus Entry" ), _( "Add a bus entry to a bus" ),
        add_bus2bus_xpm, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::placeLabel( "eeschema.InteractiveDrawing.placeLabel",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_LABEL ),
        _( "Add Label" ), _( "Add a net label" ),
        add_line_label_xpm, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::placeHierarchicalLabel( "eeschema.InteractiveDrawing.placeHierarchicalLabel",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_HLABEL ),
        _( "Add Hierarchical Label" ), _( "Add a hierarchical sheet label" ),
        add_hierarchical_label_xpm, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::drawSheet( "eeschema.InteractiveDrawing.drawSheet",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_HIER_SHEET ),
        _( "Add Sheet" ), _( "Add a hierarchical sheet" ),
        add_hierarchical_subsheet_xpm, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::placeSheetPin( "eeschema.InteractiveDrawing.placeSheetPin",
        AS_GLOBAL, 0, _( "Add Sheet Pin" ), _( "Add a sheet pin" ),
        add_hierar_pin_xpm, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::importSheetPin( "eeschema.InteractiveDrawing.importSheetPin",
        AS_GLOBAL, 0, _( "Import Sheet Pin" ), _( "Import a hierarchical sheet pin" ),
        import_hierarchical_label_xpm, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::placeGlobalLabel( "eeschema.InteractiveDrawing.placeGlobalLabel",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_GLABEL ),
        _( "Add Global Label" ), _( "Add a global label" ),
        add_glabel_xpm, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::placeSchematicText( "eeschema.InteractiveDrawing.placeSchematicText",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_GRAPHIC_TEXT ),
        _( "Add Text" ), _( "Add text" ),
        text_xpm, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::placeImage( "eeschema.InteractiveDrawing.placeImage",
        AS_GLOBAL, 0, _( "Add Image" ), _( "Add bitmap image" ),
        image_xpm, AF_ACTIVATE );

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

TOOL_ACTION SCH_ACTIONS::addHierLabel( "eeschema.InteractiveEditing.addHierLabel",
        AS_GLOBAL, 0, _( "Add Hierarchical Label" ), _( "Add a hierarchical label to a wire or bus" ),
        add_hierarchical_label_xpm, AF_NONE );


SCH_DRAWING_TOOLS::SCH_DRAWING_TOOLS() :
    TOOL_INTERACTIVE( "eeschema.InteractiveDrawing" ),
    m_selectionTool( nullptr ),
    m_view( nullptr ),
    m_controls( nullptr ),
    m_frame( nullptr ),
    m_menu( *this )
{
};


SCH_DRAWING_TOOLS::~SCH_DRAWING_TOOLS()
{
}


bool SCH_DRAWING_TOOLS::Init()
{
    m_frame = getEditFrame<SCH_EDIT_FRAME>();
    m_selectionTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();

    auto belowRootSheetCondition = [] ( const SELECTION& aSel ) {
        return g_CurrentSheet->Last() != g_RootSheet;
    };

    auto& ctxMenu = m_menu.GetMenu();

    //
    // Build the drawing tool menu
    //
    ctxMenu.AddItem( ACTIONS::cancelInteractive, SCH_CONDITIONS::ShowAlways, 1 );
    ctxMenu.AddItem( SCH_ACTIONS::leaveSheet, belowRootSheetCondition, 2 );

    ctxMenu.AddSeparator( SCH_CONDITIONS::ShowAlways, 1000 );
    m_menu.AddStandardSubMenus( m_frame );

    return true;
}


void SCH_DRAWING_TOOLS::Reset( RESET_REASON aReason )
{
    // Init variables used by every drawing tool
    m_view = static_cast<KIGFX::SCH_VIEW*>( getView() );
    m_controls = getViewControls();
    m_frame = getEditFrame<SCH_EDIT_FRAME>();
}


int SCH_DRAWING_TOOLS::AddJunction( const TOOL_EVENT& aEvent )
{
    m_toolMgr->RunAction( SCH_ACTIONS::clearSelection, true );

    m_frame->GetCanvas()->MoveCursorToCrossHair();
    m_frame->AddJunction( m_frame->GetCrossHairPosition() );

    return 0;
}


int SCH_DRAWING_TOOLS::AddLabel( const TOOL_EVENT& aEvent )
{
    m_toolMgr->RunAction( SCH_ACTIONS::clearSelection, true );

    int layer = LAYER_NOTES;

    if( aEvent.IsAction( &SCH_ACTIONS::addLabel ) )
        layer = LAYER_LOCLABEL;
    else if( aEvent.IsAction( &SCH_ACTIONS::addGlobalLabel ) )
        layer = LAYER_GLOBLABEL;
    else if( aEvent.IsAction( &SCH_ACTIONS::addHierLabel ) )
        layer = LAYER_HIERLABEL;

    SCH_ITEM* item = m_frame->CreateNewText( layer );
    m_frame->AddItemToScreenAndUndoList( item );

    return 0;
}


// History lists for PlaceSymbol()
static SCH_BASE_FRAME::HISTORY_LIST s_SymbolHistoryList;
static SCH_BASE_FRAME::HISTORY_LIST s_PowerHistoryList;


int SCH_DRAWING_TOOLS::PlaceSymbol( const TOOL_EVENT& aEvent )
{
    SCH_COMPONENT* component = aEvent.Parameter<SCH_COMPONENT*>();

    m_frame->SetToolID( ID_SCH_PLACE_COMPONENT, wxCURSOR_PENCIL, _( "Add Symbol" ) );

    return doPlaceComponent( component, nullptr, s_SymbolHistoryList );
}


int SCH_DRAWING_TOOLS::PlacePower( const TOOL_EVENT& aEvent )
{
    SCH_COMPONENT* component = aEvent.Parameter<SCH_COMPONENT*>();
    SCHLIB_FILTER  filter;

    filter.FilterPowerParts( true );
    m_frame->SetToolID( ID_PLACE_POWER_BUTT, wxCURSOR_PENCIL, _( "Add Power" ) );

    return doPlaceComponent( component, &filter, s_PowerHistoryList );
}


int SCH_DRAWING_TOOLS::doPlaceComponent( SCH_COMPONENT* aComponent, SCHLIB_FILTER* aFilter,
                                         SCH_BASE_FRAME::HISTORY_LIST aHistoryList )
{
    VECTOR2I cursorPos = m_controls->GetCursorPosition();
    m_controls->ShowCursor( true );

    Activate();

    // If a component was passed in get it ready for placement.
    if( aComponent )
    {
        aComponent->SetFlags( IS_NEW | IS_MOVED );

        m_toolMgr->RunAction( SCH_ACTIONS::clearSelection, true );
        m_selectionTool->AddItemToSel( aComponent );

        // Queue up a refresh event so we don't have to wait for the next mouse-moved event
        m_toolMgr->RunAction( SCH_ACTIONS::refreshPreview );
    }

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        cursorPos = m_controls->GetCursorPosition( !evt->Modifier( MD_ALT ) );

        if( TOOL_EVT_UTILS::IsCancelInteractive( evt.get() ) )
        {
            if( aComponent )
            {
                m_toolMgr->RunAction( SCH_ACTIONS::clearSelection, true );
                m_view->ClearPreview();
                delete aComponent;
                aComponent = nullptr;

                if( !evt->IsActivate() )
                    continue;
            }

            break;
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( !aComponent )
            {
                m_toolMgr->RunAction( SCH_ACTIONS::clearSelection, true );
                m_frame->SetRepeatItem( nullptr );

                // Pick the module to be placed
                m_frame->GetCanvas()->SetIgnoreMouseEvents( true );

                auto sel = m_frame->SelectCompFromLibTree( aFilter, aHistoryList, true, 1, 1,
                                                           m_frame->GetShowFootprintPreviews());

                m_frame->GetCanvas()->SetIgnoreMouseEvents( false );

                // Restore cursor after dialog
                m_frame->GetCanvas()->MoveCursorToCrossHair();

                LIB_PART* part = sel.LibId.IsValid() ? m_frame->GetLibPart( sel.LibId ) : nullptr;

                if( !part )
                    continue;

                aComponent = new SCH_COMPONENT( *part, g_CurrentSheet, sel, (wxPoint) cursorPos );
                aComponent->SetFlags( IS_NEW | IS_MOVED );

                // Be sure the link to the corresponding LIB_PART is OK:
                aComponent->Resolve( *m_frame->Prj().SchSymbolLibTable() );

                if( m_frame->GetAutoplaceFields() )
                    aComponent->AutoplaceFields( /* aScreen */ NULL, /* aManual */ false );

                m_frame->SetRepeatItem( aComponent );

                m_view->ClearPreview();
                m_view->AddToPreview( aComponent->Clone() );
                m_selectionTool->AddItemToSel( aComponent );
            }
            else
            {
                m_frame->AddItemToScreenAndUndoList( aComponent );
                aComponent = nullptr;

                m_view->ClearPreview();
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !aComponent )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu.ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else if( evt->Category() == TC_COMMAND && evt->Action() == TA_CONTEXT_MENU_CHOICE )
        {
            if( evt->GetCommandId().get() >= ID_POPUP_SCH_SELECT_UNIT_CMP
                && evt->GetCommandId().get() <= ID_POPUP_SCH_SELECT_UNIT_CMP_MAX )
            {
                int unit = evt->GetCommandId().get() - ID_POPUP_SCH_SELECT_UNIT_CMP;

                if( aComponent )
                {
                    m_frame->SelectUnit( aComponent, unit );
                    m_toolMgr->RunAction( SCH_ACTIONS::refreshPreview );
                }
            }
        }
        else if( aComponent && ( evt->IsAction( &SCH_ACTIONS::refreshPreview )
                              || evt->IsMotion() ) )
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


int SCH_DRAWING_TOOLS::PlaceImage( const TOOL_EVENT& aEvent )
{
    SCH_BITMAP* image = aEvent.Parameter<SCH_BITMAP*>();

    m_frame->SetToolID( ID_ADD_IMAGE_BUTT, wxCURSOR_PENCIL, _( "Add image" ) );

    VECTOR2I cursorPos = m_controls->GetCursorPosition();

    m_toolMgr->RunAction( SCH_ACTIONS::clearSelection, true );
    m_controls->ShowCursor( true );

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
                m_toolMgr->RunAction( SCH_ACTIONS::clearSelection, true );
                m_view->ClearPreview();
                delete image;
                image = nullptr;

                if( !evt->IsActivate() )
                    continue;
            }

            break;
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( !image )
            {
                wxFileDialog dlg( m_frame, _( "Choose Image" ), wxEmptyString, wxEmptyString,
                                  _( "Image Files " ) + wxImage::GetImageExtWildcard(), wxFD_OPEN );

                m_frame->GetCanvas()->SetIgnoreMouseEvents( true );

                if( dlg.ShowModal() != wxID_OK )
                    continue;

                m_frame->GetCanvas()->SetIgnoreMouseEvents( false );

                // Restore cursor after dialog
                m_frame->GetCanvas()->MoveCursorToCrossHair();

                wxString fullFilename = dlg.GetPath();

                if( wxFileExists( fullFilename ) )
                    image = new SCH_BITMAP( (wxPoint)cursorPos );

                if( !image || !image->ReadImageFile( fullFilename ) )
                {
                    wxMessageBox( _( "Couldn't load image from \"%s\"" ), fullFilename );
                    delete image;
                    image = nullptr;
                    continue;
                }

                image->SetFlags( IS_NEW | IS_MOVED );

                m_frame->SetRepeatItem( image );

                m_view->ClearPreview();
                m_view->AddToPreview( image->Clone() );
                m_view->RecacheAllItems();  // Bitmaps are cached in Opengl

                m_selectionTool->AddItemToSel( image );

                m_controls->SetCursorPosition( cursorPos, false );
            }
            else
            {
                m_frame->AddItemToScreenAndUndoList( image );
                image = nullptr;

                m_view->ClearPreview();
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !image )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu.ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else if( image && ( evt->IsAction( &SCH_ACTIONS::refreshPreview )
                         || evt->IsMotion() ) )
        {
            image->SetPosition( (wxPoint)cursorPos );
            m_view->ClearPreview();
            m_view->AddToPreview( image->Clone() );
            m_view->RecacheAllItems();  // Bitmaps are cached in Opengl
        }

        // Enable autopanning and cursor capture only when there is a module to be placed
        m_controls->SetAutoPan( !!image );
        m_controls->CaptureCursor( !!image );
    }

    m_frame->SetNoToolSelected();

    return 0;
}


int SCH_DRAWING_TOOLS::PlaceNoConnect( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_NOCONN_BUTT, wxCURSOR_PENCIL, _( "Add no connect" ) );
    return doSingleClickPlace( SCH_NO_CONNECT_T );
}


int SCH_DRAWING_TOOLS::PlaceJunction( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_JUNCTION_BUTT, wxCURSOR_PENCIL, _( "Add junction" ) );
    return doSingleClickPlace( SCH_JUNCTION_T );
}


int SCH_DRAWING_TOOLS::PlaceBusWireEntry( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_WIRETOBUS_ENTRY_BUTT, wxCURSOR_PENCIL, _( "Add wire to bus entry" ) );
    return doSingleClickPlace( SCH_BUS_WIRE_ENTRY_T );
}


int SCH_DRAWING_TOOLS::PlaceBusBusEntry( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_BUSTOBUS_ENTRY_BUTT, wxCURSOR_PENCIL, _( "Add bus to bus entry" ) );
    return doSingleClickPlace( SCH_BUS_BUS_ENTRY_T );
}


int SCH_DRAWING_TOOLS::doSingleClickPlace( KICAD_T aType )
{
    m_toolMgr->RunAction( SCH_ACTIONS::clearSelection, true );
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
                m_frame->AddItemToScreenAndUndoList( item );

                m_frame->SetRepeatItem( item );

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


int SCH_DRAWING_TOOLS::PlaceLabel( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_LABEL_BUTT, wxCURSOR_PENCIL, _( "Add net label" ) );
    return doTwoClickPlace( SCH_LABEL_T );
}


int SCH_DRAWING_TOOLS::PlaceGlobalLabel( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_GLOBALLABEL_BUTT, wxCURSOR_PENCIL, _( "Add global label" ) );
    return doTwoClickPlace( SCH_GLOBAL_LABEL_T );
}


int SCH_DRAWING_TOOLS::PlaceHierarchicalLabel( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_HIERLABEL_BUTT, wxCURSOR_PENCIL, _( "Add hierarchical label" ) );
    return doTwoClickPlace( SCH_HIER_LABEL_T );
}


int SCH_DRAWING_TOOLS::PlaceSheetPin( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_SHEET_PIN_BUTT, wxCURSOR_PENCIL, _( "Add sheet pins" ) );
    return doTwoClickPlace( SCH_SHEET_PIN_T );
}


int SCH_DRAWING_TOOLS::ImportSheetPin( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_IMPORT_HLABEL_BUTT, wxCURSOR_PENCIL, _( "Import sheet pins" ) );
    return doTwoClickPlace( SCH_SHEET_PIN_T );
}


int SCH_DRAWING_TOOLS::PlaceSchematicText( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_TEXT_COMMENT_BUTT, wxCURSOR_PENCIL, _( "Add text" ) );
    return doTwoClickPlace( SCH_TEXT_T );
}


int SCH_DRAWING_TOOLS::doTwoClickPlace( KICAD_T aType )
{
    VECTOR2I  cursorPos = m_controls->GetCursorPosition();
    EDA_ITEM* item = nullptr;

    m_toolMgr->RunAction( SCH_ACTIONS::clearSelection, true );
    m_controls->ShowCursor( true );

    Activate();

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        cursorPos = m_controls->GetCursorPosition( !evt->Modifier( MD_ALT ) );

        if( TOOL_EVT_UTILS::IsCancelInteractive( evt.get() ) )
        {
            if( item )
            {
                m_toolMgr->RunAction( SCH_ACTIONS::clearSelection, true );
                m_view->ClearPreview();
                delete item;
                item = nullptr;

                if( !evt->IsActivate() )
                    continue;
            }

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

                m_frame->GetCanvas()->SetIgnoreMouseEvents( false );

                // Restore cursor after dialog
                m_frame->GetCanvas()->MoveCursorToCrossHair();

                if( item )
                {
                    item->SetFlags( IS_NEW | IS_MOVED );
                    m_view->ClearPreview();
                    m_view->AddToPreview( item->Clone() );
                    m_selectionTool->AddItemToSel( item );
                }

                m_controls->SetCursorPosition( cursorPos, false );
            }

            // ... and second click places:
            else
            {
                m_frame->AddItemToScreenAndUndoList( (SCH_ITEM*) item );
                item = nullptr;

                m_view->ClearPreview();
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
            // This happens if our text was replaced out from under us by ConvertTextType()
            SELECTION& selection = m_selectionTool->GetSelection();

            if( selection.GetSize() == 1 )
            {
                item = (SCH_ITEM*) selection.Front();
                m_view->ClearPreview();
                m_view->AddToPreview( item->Clone() );
            }
            else
                item = nullptr;
        }
        else if( item && ( evt->IsAction( &SCH_ACTIONS::refreshPreview )
                        || evt->IsMotion() ) )
        {
            static_cast<SCH_ITEM*>( item )->SetPosition( (wxPoint)cursorPos );
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


int SCH_DRAWING_TOOLS::DrawSheet( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_SHEET_SYMBOL_BUTT, wxCURSOR_PENCIL, _( "Add sheet" ) );
    m_toolMgr->RunAction( SCH_ACTIONS::clearSelection, true );
    m_controls->ShowCursor( true );

    SCH_SHEET* sheet = nullptr;

    Activate();

    // Main loop: keep receiving events
    while( auto evt = Wait() )
    {
        VECTOR2I cursorPos = m_controls->GetCursorPosition( !evt->Modifier( MD_ALT ) );

        if( TOOL_EVT_UTILS::IsCancelInteractive( evt.get() ) )
        {
            m_toolMgr->RunAction( SCH_ACTIONS::clearSelection, true );
            m_view->ClearPreview();

            if( sheet )
            {
                delete sheet;
                sheet = nullptr;

                if( !evt->IsActivate() )
                    continue;
            }

            break;
        }

        else if( evt->IsClick( BUT_LEFT ) && !sheet )
        {
            sheet = new SCH_SHEET( (wxPoint) cursorPos );
            sheet->SetFlags( IS_NEW | IS_RESIZED );
            sheet->SetTimeStamp( GetNewTimeStamp() );
            sheet->SetParent( m_frame->GetScreen() );
            sheet->SetScreen( NULL );
            sizeSheet( sheet, cursorPos );

            m_frame->SetRepeatItem( nullptr );

            m_selectionTool->AddItemToSel( sheet );
            m_view->ClearPreview();
            m_view->AddToPreview( sheet->Clone() );
        }

        else if( sheet && ( evt->IsClick( BUT_LEFT )
                         || evt->IsAction( &SCH_ACTIONS::finishSheet ) ) )
        {
            m_view->ClearPreview();

            if( m_frame->EditSheet( (SCH_SHEET*)sheet, g_CurrentSheet, nullptr ) )
                m_frame->AddItemToScreenAndUndoList( sheet );
            else
                delete sheet;

            sheet = nullptr;
        }

        else if( sheet && ( evt->IsAction( &SCH_ACTIONS::refreshPreview )
                         || evt->IsMotion() ) )
        {
            sizeSheet( sheet, cursorPos );
            m_view->ClearPreview();
            m_view->AddToPreview( sheet->Clone() );
        }

        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !sheet )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu.ShowContextMenu( m_selectionTool->GetSelection() );
        }

        // Enable autopanning and cursor capture only when there is a sheet to be placed
        m_controls->SetAutoPan( !!sheet );
        m_controls->CaptureCursor( !!sheet );
    }

    m_frame->SetNoToolSelected();

    return 0;
}


void SCH_DRAWING_TOOLS::sizeSheet( SCH_SHEET* aSheet, VECTOR2I aPos )
{
    wxPoint pos = aSheet->GetPosition();
    wxPoint size = (wxPoint) aPos - pos;

    size.x = std::max( size.x, MIN_SHEET_WIDTH );
    size.y = std::max( size.y, MIN_SHEET_HEIGHT );

    wxPoint grid = m_frame->GetNearestGridPosition( pos + size );
    aSheet->Resize( wxSize( grid.x - pos.x, grid.y - pos.y ) );
}


void SCH_DRAWING_TOOLS::setTransitions()
{
    Go( &SCH_DRAWING_TOOLS::PlaceSymbol,           SCH_ACTIONS::placeSymbol.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::PlacePower,            SCH_ACTIONS::placePower.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::PlaceNoConnect,        SCH_ACTIONS::placeNoConnect.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::PlaceJunction,         SCH_ACTIONS::placeJunction.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::PlaceBusWireEntry,     SCH_ACTIONS::placeBusWireEntry.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::PlaceBusBusEntry,      SCH_ACTIONS::placeBusBusEntry.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::PlaceLabel,            SCH_ACTIONS::placeLabel.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::PlaceHierarchicalLabel,SCH_ACTIONS::placeHierarchicalLabel.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::PlaceGlobalLabel,      SCH_ACTIONS::placeGlobalLabel.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::DrawSheet,             SCH_ACTIONS::drawSheet.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::PlaceSheetPin,         SCH_ACTIONS::placeSheetPin.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::ImportSheetPin,        SCH_ACTIONS::importSheetPin.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::PlaceSchematicText,    SCH_ACTIONS::placeSchematicText.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::PlaceImage,            SCH_ACTIONS::placeImage.MakeEvent() );

    Go( &SCH_DRAWING_TOOLS::AddJunction,           SCH_ACTIONS::addJunction.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::AddLabel,              SCH_ACTIONS::addLabel.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::AddLabel,              SCH_ACTIONS::addGlobalLabel.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::AddLabel,              SCH_ACTIONS::addHierLabel.MakeEvent() );
}
