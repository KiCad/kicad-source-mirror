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
#include "ee_selection_tool.h"
#include <ee_actions.h>

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
#include <sch_component.h>
#include <sch_no_connect.h>
#include <sch_line.h>
#include <sch_junction.h>
#include <sch_bus_entry.h>
#include <sch_text.h>
#include <sch_sheet.h>
#include <sch_bitmap.h>
#include <class_library.h>


// Drawing tool actions
TOOL_ACTION EE_ACTIONS::placeSymbol( "eeschema.InteractiveDrawing.placeSymbol",
        AS_GLOBAL, 
        'A', LEGACY_HK_NAME( "Add Symbol" ),
        _( "Add Symbol" ), _( "Add a symbol" ),
        add_component_xpm, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::placePower( "eeschema.InteractiveDrawing.placePowerPort",
        AS_GLOBAL, 
        'P', LEGACY_HK_NAME( "Add Power" ),
        _( "Add Power" ), _( "Add a power port" ),
        add_power_xpm, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::placeNoConnect( "eeschema.InteractiveDrawing.placeNoConnect",
        AS_GLOBAL, 
        'Q', LEGACY_HK_NAME( "Add No Connect Flag" ),
        _( "Add No Connect Flag" ), _( "Add a no-connection flag" ),
        noconn_xpm, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::placeJunction( "eeschema.InteractiveDrawing.placeJunction",
        AS_GLOBAL, 
        'J', LEGACY_HK_NAME( "Add Junction" ),
        _( "Add Junction" ), _( "Add a junction" ),
        add_junction_xpm, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::placeBusWireEntry( "eeschema.InteractiveDrawing.placeBusWireEntry",
        AS_GLOBAL, 
        'Z', LEGACY_HK_NAME( "Add Wire Entry" ),
        _( "Add Wire to Bus Entry" ), _( "Add a wire entry to a bus" ),
        add_line2bus_xpm, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::placeBusBusEntry( "eeschema.InteractiveDrawing.placeBusBusEntry",
        AS_GLOBAL, 
        '/', LEGACY_HK_NAME( "Add Bus Entry" ),
        _( "Add Bus to Bus Entry" ), _( "Add a bus entry to a bus" ),
        add_bus2bus_xpm, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::placeLabel( "eeschema.InteractiveDrawing.placeLabel",
        AS_GLOBAL, 
        'L', LEGACY_HK_NAME( "Add Label" ),
        _( "Add Label" ), _( "Add a net label" ),
        add_line_label_xpm, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::placeHierarchicalLabel( "eeschema.InteractiveDrawing.placeHierarchicalLabel",
        AS_GLOBAL, 
        'H', LEGACY_HK_NAME( "Add Hierarchical Label" ),
        _( "Add Hierarchical Label" ), _( "Add a hierarchical sheet label" ),
        add_hierarchical_label_xpm, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::drawSheet( "eeschema.InteractiveDrawing.drawSheet",
        AS_GLOBAL, 
        'S', LEGACY_HK_NAME( "Add Sheet" ),
        _( "Add Sheet" ), _( "Add a hierarchical sheet" ),
        add_hierarchical_subsheet_xpm, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::placeSheetPin( "eeschema.InteractiveDrawing.placeSheetPin",
        AS_GLOBAL, 0, "",
        _( "Add Sheet Pin" ), _( "Add a sheet pin" ),
        add_hierar_pin_xpm, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::importSheetPin( "eeschema.InteractiveDrawing.importSheetPin",
        AS_GLOBAL, 0, "",
        _( "Import Sheet Pin" ), _( "Import a hierarchical sheet pin" ),
        import_hierarchical_label_xpm, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::placeGlobalLabel( "eeschema.InteractiveDrawing.placeGlobalLabel",
        AS_GLOBAL, 
        MD_CTRL + 'H', LEGACY_HK_NAME( "Add Global Label" ),
        _( "Add Global Label" ), _( "Add a global label" ),
        add_glabel_xpm, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::placeSchematicText( "eeschema.InteractiveDrawing.placeSchematicText",
        AS_GLOBAL, 
        'T', LEGACY_HK_NAME( "Add Graphic Text" ),
        _( "Add Text" ), _( "Add text" ),
        text_xpm, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::placeImage( "eeschema.InteractiveDrawing.placeImage",
        AS_GLOBAL, 0, "",
        _( "Add Image" ), _( "Add bitmap image" ),
        image_xpm, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::finishSheet( "eeschema.InteractiveDrawing.finishSheet",
        AS_GLOBAL, 0, "",
        _( "Finish Sheet" ), _( "Finish drawing sheet" ),
        checked_ok_xpm, AF_NONE );

TOOL_ACTION EE_ACTIONS::addJunction( "eeschema.InteractiveEditing.addJunction",
        AS_GLOBAL, 0, "",
        _( "Add Junction" ), _( "Add a wire or bus junction" ),
        add_junction_xpm, AF_NONE );

TOOL_ACTION EE_ACTIONS::addLabel( "eeschema.InteractiveEditing.addLabel",
        AS_GLOBAL, 0, "",
        _( "Add Label" ), _( "Add a label to a wire or bus" ),
        add_line_label_xpm, AF_NONE );

TOOL_ACTION EE_ACTIONS::addGlobalLabel( "eeschema.InteractiveEditing.addGlobalLabel",
        AS_GLOBAL, 0, "",
        _( "Add Global Label" ), _( "Add a global label to a wire or bus" ),
        add_glabel_xpm, AF_NONE );

TOOL_ACTION EE_ACTIONS::addHierLabel( "eeschema.InteractiveEditing.addHierLabel",
        AS_GLOBAL, 0, "",
        _( "Add Hierarchical Label" ), _( "Add a hierarchical label to a wire or bus" ),
        add_hierarchical_label_xpm, AF_NONE );

TOOL_ACTION EE_ACTIONS::addSheetPin( "eeschema.InteractiveEditing.addSheetPin",
        AS_GLOBAL, 0, "",
        _( "Add Sheet Pin" ), _( "Add a sheet pin to the selected sheet" ),
        add_hierarchical_label_xpm, AF_NONE );

TOOL_ACTION EE_ACTIONS::addImportedSheetPin( "eeschema.InteractiveEditing.addImportedSheetPin",
        AS_GLOBAL, 0, "",
        _( "Add Imported Sheet Pin" ), _( "Add an imported sheet pin" ),
        add_hierarchical_label_xpm, AF_NONE );


SCH_DRAWING_TOOLS::SCH_DRAWING_TOOLS() :
    EE_TOOL_BASE<SCH_EDIT_FRAME>( "eeschema.InteractiveDrawing" )
{
}


SCH_DRAWING_TOOLS::~SCH_DRAWING_TOOLS()
{
}


bool SCH_DRAWING_TOOLS::Init()
{
    EE_TOOL_BASE::Init();

    auto belowRootSheetCondition = [] ( const SELECTION& aSel ) {
        return g_CurrentSheet->Last() != g_RootSheet;
    };

    auto& ctxMenu = m_menu.GetMenu();
    ctxMenu.AddItem( EE_ACTIONS::leaveSheet, belowRootSheetCondition, 2 );

    return true;
}


int SCH_DRAWING_TOOLS::AddJunction( const TOOL_EVENT& aEvent )
{
    EE_SELECTION& selection = m_selectionTool->GetSelection();
    SCH_LINE*     wire = dynamic_cast<SCH_LINE*>( selection.Front() );

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

    if( wire )
    {
        SEG seg( wire->GetStartPoint(), wire->GetEndPoint() );
        VECTOR2I nearest = seg.NearestPoint( m_frame->GetCrossHairPosition() );
        m_frame->SetCrossHairPosition( (wxPoint) nearest, false );
    }

    getViewControls()->WarpCursor( m_frame->GetCrossHairPosition(), true );
    SCH_JUNCTION* junction = m_frame->AddJunction( m_frame->GetCrossHairPosition() );
    m_selectionTool->AddItemToSel( junction );

    return 0;
}


int SCH_DRAWING_TOOLS::AddLabel( const TOOL_EVENT& aEvent )
{
    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

    int layer = LAYER_NOTES;

    if( aEvent.IsAction( &EE_ACTIONS::addLabel ) )
        layer = LAYER_LOCLABEL;
    else if( aEvent.IsAction( &EE_ACTIONS::addGlobalLabel ) )
        layer = LAYER_GLOBLABEL;
    else if( aEvent.IsAction( &EE_ACTIONS::addHierLabel ) )
        layer = LAYER_HIERLABEL;

    SCH_ITEM* item = m_frame->CreateNewText( layer );
    m_frame->AddItemToScreenAndUndoList( item );
    m_selectionTool->AddItemToSel( item );

    return 0;
}


int SCH_DRAWING_TOOLS::AddSheetPin( const TOOL_EVENT& aEvent )
{
    EE_SELECTION&  selection = m_selectionTool->GetSelection();
    SCH_SHEET*     sheet = dynamic_cast<SCH_SHEET*>( selection.Front() );
    SCH_HIERLABEL* label = nullptr;

    if( !sheet )
        return 0;

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

    if( aEvent.IsAction( &EE_ACTIONS::addImportedSheetPin ) )
    {
        label = m_frame->ImportHierLabel( sheet );

        if( !label )
        {
            m_statusPopup.reset( new STATUS_TEXT_POPUP( m_frame ) );
            m_statusPopup->SetText( _( "No new hierarchical labels found." ) );
            m_statusPopup->Move( wxGetMousePosition() + wxPoint( 20, 20 ) );
            m_statusPopup->PopupFor( 2000 );
            return 0;
        }
    }

    SCH_SHEET_PIN* pin = m_frame->CreateSheetPin( sheet, label );
    m_frame->AddItemToScreenAndUndoList( pin );
    m_selectionTool->AddItemToSel( pin );

    return 0;
}


// History lists for PlaceSymbol()
static SCH_BASE_FRAME::HISTORY_LIST s_SymbolHistoryList;
static SCH_BASE_FRAME::HISTORY_LIST s_PowerHistoryList;


int SCH_DRAWING_TOOLS::PlaceSymbol( const TOOL_EVENT& aEvent )
{
    SCH_COMPONENT* component = aEvent.Parameter<SCH_COMPONENT*>();

    m_frame->SetToolID( ID_PLACE_SYMBOL_TOOL, wxCURSOR_PENCIL, _( "Add Symbol" ) );

    return doPlaceComponent( component, nullptr, s_SymbolHistoryList );
}


int SCH_DRAWING_TOOLS::PlacePower( const TOOL_EVENT& aEvent )
{
    SCH_COMPONENT* component = aEvent.Parameter<SCH_COMPONENT*>();
    SCHLIB_FILTER  filter;

    filter.FilterPowerParts( true );
    m_frame->SetToolID( ID_PLACE_POWER_TOOL, wxCURSOR_PENCIL, _( "Add Power" ) );

    return doPlaceComponent( component, &filter, s_PowerHistoryList );
}


int SCH_DRAWING_TOOLS::doPlaceComponent( SCH_COMPONENT* aComponent, SCHLIB_FILTER* aFilter,
                                         SCH_BASE_FRAME::HISTORY_LIST& aHistoryList )
{
    VECTOR2I cursorPos = getViewControls()->GetCursorPosition();
    getViewControls()->ShowCursor( true );

    Activate();

    // If a component was passed in get it ready for placement.
    if( aComponent )
    {
        aComponent->SetFlags( IS_NEW | IS_MOVED );

        m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
        m_selectionTool->AddItemToSel( aComponent );

        // Queue up a refresh event so we don't have to wait for the next mouse-moved event
        m_toolMgr->RunAction( EE_ACTIONS::refreshPreview );
    }

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        cursorPos = getViewControls()->GetCursorPosition( !evt->Modifier( MD_ALT ) );

        if( TOOL_EVT_UTILS::IsCancelInteractive( evt.get() ) )
        {
            if( aComponent )
            {
                m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
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
                m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

                // Pick the module to be placed
                auto sel = m_frame->SelectCompFromLibTree( aFilter, aHistoryList, true, 1, 1,
                                                           m_frame->GetShowFootprintPreviews());

                // Restore cursor after dialog
                getViewControls()->WarpCursor( m_frame->GetCrossHairPosition(), true );

                LIB_PART* part = sel.LibId.IsValid() ? m_frame->GetLibPart( sel.LibId ) : nullptr;

                if( !part )
                    continue;

                aComponent = new SCH_COMPONENT( *part, g_CurrentSheet, sel, (wxPoint) cursorPos );
                aComponent->SetFlags( IS_NEW | IS_MOVED );

                // Be sure the link to the corresponding LIB_PART is OK:
                aComponent->Resolve( *m_frame->Prj().SchSymbolLibTable() );

                if( m_frame->GetAutoplaceFields() )
                    aComponent->AutoplaceFields( /* aScreen */ NULL, /* aManual */ false );

                m_frame->SaveCopyForRepeatItem( aComponent );

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
                    m_toolMgr->RunAction( EE_ACTIONS::refreshPreview );
                }
            }
        }
        else if( aComponent && ( evt->IsAction( &EE_ACTIONS::refreshPreview )
                              || evt->IsMotion() ) )
        {
            aComponent->SetPosition( (wxPoint)cursorPos );
            m_view->ClearPreview();
            m_view->AddToPreview( aComponent->Clone() );
        }

        // Enable autopanning and cursor capture only when there is a module to be placed
        getViewControls()->SetAutoPan( !!aComponent );
        getViewControls()->CaptureCursor( !!aComponent );
    }

    m_frame->SetNoToolSelected();

    return 0;
}


int SCH_DRAWING_TOOLS::PlaceImage( const TOOL_EVENT& aEvent )
{
    SCH_BITMAP* image = aEvent.Parameter<SCH_BITMAP*>();

    m_frame->SetToolID( ID_PLACE_IMAGE_TOOL, wxCURSOR_PENCIL, _( "Add image" ) );

    VECTOR2I cursorPos = getViewControls()->GetCursorPosition();

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
    getViewControls()->ShowCursor( true );

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
        cursorPos = getViewControls()->GetCursorPosition( !evt->Modifier( MD_ALT ) );

        if( TOOL_EVT_UTILS::IsCancelInteractive( evt.get() ) )
        {
            if( image )
            {
                m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
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
                m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
                wxFileDialog dlg( m_frame, _( "Choose Image" ), wxEmptyString, wxEmptyString,
                                  _( "Image Files " ) + wxImage::GetImageExtWildcard(), wxFD_OPEN );

                if( dlg.ShowModal() != wxID_OK )
                    continue;

                // Restore cursor after dialog
                getViewControls()->WarpCursor( m_frame->GetCrossHairPosition(), true );

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

                m_frame->SaveCopyForRepeatItem( image );

                m_view->ClearPreview();
                m_view->AddToPreview( image->Clone() );
                m_view->RecacheAllItems();  // Bitmaps are cached in Opengl

                m_selectionTool->AddItemToSel( image );

                getViewControls()->SetCursorPosition( cursorPos, false );
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
        else if( image && ( evt->IsAction( &EE_ACTIONS::refreshPreview )
                         || evt->IsMotion() ) )
        {
            image->SetPosition( (wxPoint)cursorPos );
            m_view->ClearPreview();
            m_view->AddToPreview( image->Clone() );
            m_view->RecacheAllItems();  // Bitmaps are cached in Opengl
        }

        // Enable autopanning and cursor capture only when there is a module to be placed
        getViewControls()->SetAutoPan( !!image );
        getViewControls()->CaptureCursor( !!image );
    }

    m_frame->SetNoToolSelected();

    return 0;
}


int SCH_DRAWING_TOOLS::PlaceNoConnect( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_NOCONNECT_TOOL, wxCURSOR_PENCIL, _( "Add no connect" ) );
    return doSingleClickPlace( SCH_NO_CONNECT_T );
}


int SCH_DRAWING_TOOLS::PlaceJunction( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_JUNCTION_TOOL, wxCURSOR_PENCIL, _( "Add junction" ) );
    return doSingleClickPlace( SCH_JUNCTION_T );
}


int SCH_DRAWING_TOOLS::PlaceBusWireEntry( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_WIRETOBUS_ENTRY_TOOL, wxCURSOR_PENCIL, _( "Add wire to bus entry" ) );
    return doSingleClickPlace( SCH_BUS_WIRE_ENTRY_T );
}


int SCH_DRAWING_TOOLS::PlaceBusBusEntry( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_BUSTOBUS_ENTRY_TOOL, wxCURSOR_PENCIL, _( "Add bus to bus entry" ) );
    return doSingleClickPlace( SCH_BUS_BUS_ENTRY_T );
}


int SCH_DRAWING_TOOLS::doSingleClickPlace( KICAD_T aType )
{
    wxPoint cursorPos;

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
    getViewControls()->ShowCursor( true );
    getViewControls()->SetSnapping( true );

    Activate();

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        cursorPos = (wxPoint) getViewControls()->GetCursorPosition( !evt->Modifier( MD_ALT ) );

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

                m_frame->SaveCopyForRepeatItem( item );

                m_frame->SchematicCleanUp();
                m_frame->TestDanglingEnds();
                m_frame->OnModify();
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( m_selectionTool->GetSelection() );
        }
    }

    m_frame->SetNoToolSelected();

    return 0;
}


int SCH_DRAWING_TOOLS::PlaceLabel( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_LABEL_TOOL, wxCURSOR_PENCIL, _( "Add net label" ) );
    return doTwoClickPlace( SCH_LABEL_T );
}


int SCH_DRAWING_TOOLS::PlaceGlobalLabel( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_GLOBALLABEL_TOOL, wxCURSOR_PENCIL, _( "Add global label" ) );
    return doTwoClickPlace( SCH_GLOBAL_LABEL_T );
}


int SCH_DRAWING_TOOLS::PlaceHierarchicalLabel( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_HIERLABEL_TOOL, wxCURSOR_PENCIL, _( "Add hierarchical label" ) );
    return doTwoClickPlace( SCH_HIER_LABEL_T );
}


int SCH_DRAWING_TOOLS::PlaceSheetPin( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_SHEETPIN_TOOL, wxCURSOR_PENCIL, _( "Add sheet pins" ) );
    return doTwoClickPlace( SCH_SHEET_PIN_T );
}


int SCH_DRAWING_TOOLS::ImportSheetPin( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_IMPORT_SHEETPIN_TOOL, wxCURSOR_PENCIL, _( "Import sheet pins" ) );
    return doTwoClickPlace( SCH_SHEET_PIN_T );
}


int SCH_DRAWING_TOOLS::PlaceSchematicText( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_SCHEMATIC_TEXT_TOOL, wxCURSOR_PENCIL, _( "Add text" ) );
    return doTwoClickPlace( SCH_TEXT_T );
}


int SCH_DRAWING_TOOLS::doTwoClickPlace( KICAD_T aType )
{
    VECTOR2I  cursorPos;
    EDA_ITEM* item = nullptr;

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
    getViewControls()->ShowCursor( true );

    Activate();

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        cursorPos = getViewControls()->GetCursorPosition( !evt->Modifier( MD_ALT ) );

        if( TOOL_EVT_UTILS::IsCancelInteractive( evt.get() ) )
        {
            if( item )
            {
                m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
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
                m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

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
                {
                    SCH_HIERLABEL* label = nullptr;
                    SCH_SHEET*     sheet = (SCH_SHEET*) m_selectionTool->SelectPoint( cursorPos,
                                                                       EE_COLLECTOR::SheetsOnly );
                    if( !sheet )
                    {
                        m_statusPopup.reset( new STATUS_TEXT_POPUP( m_frame ) );
                        m_statusPopup->SetText( _( "Click over a sheet." ) );
                        m_statusPopup->Move( wxGetMousePosition() + wxPoint( 20, 20 ) );
                        m_statusPopup->PopupFor( 2000 );
                        break;
                    }

                    if( m_frame->GetToolId() == ID_IMPORT_SHEETPIN_TOOL )
                    {
                        label = m_frame->ImportHierLabel( sheet );

                        if( !label )
                        {
                            m_statusPopup.reset( new STATUS_TEXT_POPUP( m_frame ) );
                            m_statusPopup->SetText( _( "No new hierarchical labels found." ) );
                            m_statusPopup->Move( wxGetMousePosition() + wxPoint( 20, 20 ) );
                            m_statusPopup->PopupFor( 2000 );
                            break;
                        }
                    }

                    item = m_frame->CreateSheetPin( sheet, label );
                    break;
                }
                default:
                    wxFAIL_MSG( "doTwoClickPlace(): unknown type" );
                }

                // Restore cursor after dialog
                getViewControls()->WarpCursor( m_frame->GetCrossHairPosition(), true );

                if( item )
                {
                    item->SetFlags( IS_NEW | IS_MOVED );
                    m_view->ClearPreview();
                    m_view->AddToPreview( item->Clone() );
                    m_selectionTool->AddItemToSel( item );
                }

                getViewControls()->SetCursorPosition( cursorPos, false );
            }

            // ... and second click places:
            else
            {
                item->ClearFlags( IS_MOVED );
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
        else if( item && TOOL_EVT_UTILS::IsSelectionEvent( evt.get() ) )
        {
            // This happens if our text was replaced out from under us by ConvertTextType()
            EE_SELECTION& selection = m_selectionTool->GetSelection();

            if( selection.GetSize() == 1 )
            {
                item = (SCH_ITEM*) selection.Front();
                m_view->ClearPreview();
                m_view->AddToPreview( item->Clone() );
            }
            else
                item = nullptr;
        }
        else if( item && ( evt->IsAction( &EE_ACTIONS::refreshPreview )
                        || evt->IsMotion() ) )
        {
            static_cast<SCH_ITEM*>( item )->SetPosition( (wxPoint)cursorPos );
            m_view->ClearPreview();
            m_view->AddToPreview( item->Clone() );
        }

        // Enable autopanning and cursor capture only when there is a module to be placed
        getViewControls()->SetAutoPan( !!item );
        getViewControls()->CaptureCursor( !!item );
    }

    m_frame->SetNoToolSelected();

    return 0;
}


int SCH_DRAWING_TOOLS::DrawSheet( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_SHEET_TOOL, wxCURSOR_PENCIL, _( "Add sheet" ) );
    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
    getViewControls()->ShowCursor( true );

    SCH_SHEET* sheet = nullptr;

    Activate();

    // Main loop: keep receiving events
    while( auto evt = Wait() )
    {
        VECTOR2I cursorPos = getViewControls()->GetCursorPosition( !evt->Modifier( MD_ALT ) );

        if( TOOL_EVT_UTILS::IsCancelInteractive( evt.get() ) )
        {
            m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
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
            m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

            sheet = new SCH_SHEET( (wxPoint) cursorPos );
            sheet->SetFlags( IS_NEW | IS_RESIZED );
            sheet->SetTimeStamp( GetNewTimeStamp() );
            sheet->SetParent( m_frame->GetScreen() );
            sheet->SetScreen( NULL );
            sizeSheet( sheet, cursorPos );

            m_view->ClearPreview();
            m_view->AddToPreview( sheet->Clone() );
        }

        else if( sheet && ( evt->IsClick( BUT_LEFT )
                         || evt->IsAction( &EE_ACTIONS::finishSheet ) ) )
        {
            m_view->ClearPreview();

            if( m_frame->EditSheet( (SCH_SHEET*)sheet, g_CurrentSheet, nullptr ) )
            {
                m_frame->AddItemToScreenAndUndoList( sheet );
                m_selectionTool->AddItemToSel( sheet );
            }
            else 
            {
                delete sheet;
            }

            sheet = nullptr;
        }

        else if( sheet && ( evt->IsAction( &EE_ACTIONS::refreshPreview )
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
        getViewControls()->SetAutoPan( !!sheet );
        getViewControls()->CaptureCursor( !!sheet );
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
    Go( &SCH_DRAWING_TOOLS::PlaceSymbol,           EE_ACTIONS::placeSymbol.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::PlacePower,            EE_ACTIONS::placePower.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::PlaceNoConnect,        EE_ACTIONS::placeNoConnect.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::PlaceJunction,         EE_ACTIONS::placeJunction.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::PlaceBusWireEntry,     EE_ACTIONS::placeBusWireEntry.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::PlaceBusBusEntry,      EE_ACTIONS::placeBusBusEntry.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::PlaceLabel,            EE_ACTIONS::placeLabel.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::PlaceHierarchicalLabel,EE_ACTIONS::placeHierarchicalLabel.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::PlaceGlobalLabel,      EE_ACTIONS::placeGlobalLabel.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::DrawSheet,             EE_ACTIONS::drawSheet.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::PlaceSheetPin,         EE_ACTIONS::placeSheetPin.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::ImportSheetPin,        EE_ACTIONS::importSheetPin.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::PlaceSchematicText,    EE_ACTIONS::placeSchematicText.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::PlaceImage,            EE_ACTIONS::placeImage.MakeEvent() );

    Go( &SCH_DRAWING_TOOLS::AddJunction,           EE_ACTIONS::addJunction.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::AddLabel,              EE_ACTIONS::addLabel.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::AddLabel,              EE_ACTIONS::addGlobalLabel.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::AddLabel,              EE_ACTIONS::addHierLabel.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::AddSheetPin,           EE_ACTIONS::addSheetPin.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::AddSheetPin,           EE_ACTIONS::addImportedSheetPin.MakeEvent() );
}
