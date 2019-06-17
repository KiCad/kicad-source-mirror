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


SCH_DRAWING_TOOLS::SCH_DRAWING_TOOLS() :
    EE_TOOL_BASE<SCH_EDIT_FRAME>( "eeschema.InteractiveDrawing" )
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


// History lists for PlaceComponent()
static SCH_BASE_FRAME::HISTORY_LIST s_SymbolHistoryList;
static SCH_BASE_FRAME::HISTORY_LIST s_PowerHistoryList;


int SCH_DRAWING_TOOLS::PlaceComponent(  const TOOL_EVENT& aEvent  )
{
    SCH_COMPONENT* component = aEvent.Parameter<SCH_COMPONENT*>();
    bool           immediateMode = component || aEvent.HasPosition();
    SCHLIB_FILTER  filter;
    SCH_BASE_FRAME::HISTORY_LIST* historyList = nullptr;

    if( aEvent.IsAction( &EE_ACTIONS::placeSymbol ) )
        historyList = &s_SymbolHistoryList;
    else if (aEvent.IsAction( &EE_ACTIONS::placePower ) )
    {
        historyList = &s_PowerHistoryList;
        filter.FilterPowerParts( true );
    }
    else
        wxFAIL_MSG( "PlaceCompontent(): unexpected request" );

    getViewControls()->ShowCursor( true );

    // If a component was passed in get it ready for placement.
    if( component )
    {
        component->SetFlags( IS_NEW | IS_MOVED );

        m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
        m_selectionTool->AddItemToSel( component );
    }

    if( immediateMode )
        m_frame->PushTool( aEvent.GetCommandStr().get() );
    else
        m_frame->SetTool( aEvent.GetCommandStr().get() );

    Activate();

    // Prime the pump
    if( component )
    {
        getViewControls()->WarpCursor( getViewControls()->GetMousePosition( false ) );
        m_toolMgr->RunAction( EE_ACTIONS::refreshPreview );
    }
    else if( immediateMode )
        m_toolMgr->RunAction( EE_ACTIONS::cursorClick );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        VECTOR2I cursorPos = getViewControls()->GetCursorPosition( !evt->Modifier( MD_ALT ) );

        if( TOOL_EVT_UTILS::IsCancelInteractive( *evt ) )
        {
            if( component )
            {
                m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
                m_view->ClearPreview();
                delete component;
                component = nullptr;

                if( !evt->IsActivate() && !immediateMode )
                    continue;
            }

            if( !evt->IsActivate() && !immediateMode )
                m_frame->PopTool();

            break;
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( !component )
            {
                m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

                // Pick the module to be placed
                auto sel = m_frame->SelectCompFromLibTree( &filter, *historyList, true, 1, 1,
                                                           m_frame->GetShowFootprintPreviews());

                // Restore cursor after dialog
                getViewControls()->WarpCursor( getViewControls()->GetCursorPosition(), true );

                LIB_PART* part = sel.LibId.IsValid() ? m_frame->GetLibPart( sel.LibId ) : nullptr;

                if( !part )
                    continue;

                component = new SCH_COMPONENT( *part, g_CurrentSheet, sel, (wxPoint) cursorPos );
                component->SetFlags( IS_NEW | IS_MOVED );

                // Be sure the link to the corresponding LIB_PART is OK:
                component->Resolve( *m_frame->Prj().SchSymbolLibTable() );

                if( m_frame->GetAutoplaceFields() )
                    component->AutoplaceFields( /* aScreen */ NULL, /* aManual */ false );

                m_frame->SaveCopyForRepeatItem( component );

                m_view->ClearPreview();
                m_view->AddToPreview( component->Clone() );
                m_selectionTool->AddItemToSel( component );
            }
            else
            {
                m_frame->AddItemToScreenAndUndoList( component );
                component = nullptr;

                m_view->ClearPreview();
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !component )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu.ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else if( evt->Category() == TC_COMMAND && evt->Action() == TA_CHOICE_MENU_CHOICE )
        {
            if( evt->GetCommandId().get() >= ID_POPUP_SCH_SELECT_UNIT_CMP
                && evt->GetCommandId().get() <= ID_POPUP_SCH_SELECT_UNIT_CMP_MAX )
            {
                int unit = evt->GetCommandId().get() - ID_POPUP_SCH_SELECT_UNIT_CMP;

                if( component )
                {
                    m_frame->SelectUnit( component, unit );
                    m_toolMgr->RunAction( EE_ACTIONS::refreshPreview );
                }
            }
        }
        else if( component && ( evt->IsAction( &EE_ACTIONS::refreshPreview )
                             || evt->IsMotion() ) )
        {
            component->SetPosition( (wxPoint)cursorPos );
            m_view->ClearPreview();
            m_view->AddToPreview( component->Clone() );
        }

        // Enable autopanning and cursor capture only when there is a module to be placed
        getViewControls()->SetAutoPan( !!component );
        getViewControls()->CaptureCursor( !!component );
    }

    if( immediateMode )
        m_frame->PopTool();

    return 0;
}


int SCH_DRAWING_TOOLS::PlaceImage( const TOOL_EVENT& aEvent )
{
    SCH_BITMAP* image = aEvent.Parameter<SCH_BITMAP*>();
    bool        immediateMode = image || aEvent.HasPosition();

    if( immediateMode )
        m_frame->PushTool( aEvent.GetCommandStr().get() );
    else
        m_frame->SetTool( aEvent.GetCommandStr().get() );

    VECTOR2I cursorPos = getViewControls()->GetCursorPosition();

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
    getViewControls()->ShowCursor( true );

    // Add all the drawable parts to preview
    if( image )
    {
        image->SetPosition( (wxPoint)cursorPos );
        m_view->ClearPreview();
        m_view->AddToPreview( image->Clone() );
    }

    Activate();

    // Prime the pump
    if( image )
        m_toolMgr->RunAction( EE_ACTIONS::refreshPreview );
    else if( immediateMode )
        m_toolMgr->RunAction( ACTIONS::cursorClick );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        cursorPos = getViewControls()->GetCursorPosition( !evt->Modifier( MD_ALT ) );

        if( TOOL_EVT_UTILS::IsCancelInteractive( *evt ) )
        {
            if( image )
            {
                m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
                m_view->ClearPreview();
                delete image;
                image = nullptr;

                if( !evt->IsActivate() && !immediateMode )
                    continue;
            }

            if( !evt->IsActivate() && !immediateMode )
                m_frame->PopTool();

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
                getViewControls()->WarpCursor( getViewControls()->GetCursorPosition(), true );

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

                if( immediateMode )
                    break;
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

    if( immediateMode )
        m_frame->PopTool();

    return 0;
}


int SCH_DRAWING_TOOLS::SingleClickPlace( const TOOL_EVENT& aEvent )
{
    bool     immediateMode = aEvent.HasPosition();
    wxPoint  cursorPos;
    KICAD_T  type = aEvent.Parameter<KICAD_T>();

    if( type == SCH_JUNCTION_T )
    {
        if( immediateMode )
        {
            EE_SELECTION& selection = m_selectionTool->GetSelection();
            SCH_LINE*     wire = dynamic_cast<SCH_LINE*>( selection.Front() );

            if( wire )
            {
                SEG seg( wire->GetStartPoint(), wire->GetEndPoint() );
                VECTOR2I nearest = seg.NearestPoint( getViewControls()->GetCursorPosition() );
                getViewControls()->SetCrossHairCursorPosition( nearest, false );
                getViewControls()->WarpCursor( getViewControls()->GetCursorPosition(), true );
            }
        }
    }
    else if( aEvent.IsAction( &EE_ACTIONS::placeSheetPin ) )
        type = SCH_SHEET_PIN_T;

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
    getViewControls()->ShowCursor( true );
    getViewControls()->SetSnapping( true );

    if( immediateMode )
        m_frame->PushTool( aEvent.GetCommandStr().get() );
    else
        m_frame->SetTool( aEvent.GetCommandStr().get() );

    Activate();

    // Prime the pump
    if( immediateMode )
        m_toolMgr->RunAction( ACTIONS::cursorClick );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        cursorPos = (wxPoint) getViewControls()->GetCursorPosition( !evt->Modifier( MD_ALT ) );

        if( TOOL_EVT_UTILS::IsCancelInteractive( *evt ) )
        {
            if( !evt->IsActivate() && !immediateMode )
                m_frame->PopTool();

            break;
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            SCH_ITEM* item = nullptr;

            if( !m_frame->GetScreen()->GetItem( cursorPos, 0, type ) )
            {
                switch( type )
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
                    break;
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

            if( immediateMode )
                break;
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( m_selectionTool->GetSelection() );
        }
    }

    if( immediateMode )
        m_frame->PopTool();

    return 0;
}


int SCH_DRAWING_TOOLS::TwoClickPlace( const TOOL_EVENT& aEvent )
{
    EDA_ITEM* item = nullptr;
    bool      immediateMode = aEvent.HasPosition();
    KICAD_T   type = aEvent.Parameter<KICAD_T>();

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
    getViewControls()->ShowCursor( true );

    if( immediateMode )
        m_frame->PushTool( aEvent.GetCommandStr().get() );
    else
        m_frame->SetTool( aEvent.GetCommandStr().get() );

    Activate();

    // Prime the pump
    if( immediateMode )
        m_toolMgr->RunAction( ACTIONS::cursorClick );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        VECTOR2I cursorPos = getViewControls()->GetCursorPosition( !evt->Modifier( MD_ALT ) );

        if( TOOL_EVT_UTILS::IsCancelInteractive( *evt ) )
        {
            if( item )
            {
                m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
                m_view->ClearPreview();
                delete item;
                item = nullptr;

                if( !evt->IsActivate() && !immediateMode )
                    continue;
            }

            if( !evt->IsActivate() && !immediateMode )
                m_frame->PopTool();

            break;
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            // First click creates...
            if( !item )
            {
                m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

                switch( type )
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

                    if( aEvent.IsAction( &EE_ACTIONS::importSheetPin ) )
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
                    break;
                }

                // Restore cursor after dialog
                getViewControls()->WarpCursor( getViewControls()->GetCursorPosition(), true );

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

                if( immediateMode )
                    break;
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !item )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu.ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else if( item && TOOL_EVT_UTILS::IsSelectionEvent( *evt ) )
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
        getViewControls()->SetAutoPan( item != nullptr );
        getViewControls()->CaptureCursor( item != nullptr );
    }

    if( immediateMode )
        m_frame->PopTool();

    return 0;
}


int SCH_DRAWING_TOOLS::DrawSheet( const TOOL_EVENT& aEvent )
{
    SCH_SHEET* sheet = nullptr;
    bool       immediateMode = aEvent.HasPosition();

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
    getViewControls()->ShowCursor( true );

    if( immediateMode )
        m_frame->PushTool( aEvent.GetCommandStr().get() );
    else
        m_frame->SetTool( aEvent.GetCommandStr().get() );

    Activate();

    // Prime the pump
    if( immediateMode )
        m_toolMgr->RunAction( ACTIONS::cursorClick );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        VECTOR2I cursorPos = getViewControls()->GetCursorPosition( !evt->Modifier( MD_ALT ) );

        if( TOOL_EVT_UTILS::IsCancelInteractive( *evt ) )
        {
            m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
            m_view->ClearPreview();

            if( sheet )
            {
                delete sheet;
                sheet = nullptr;

                if( !evt->IsActivate() && !immediateMode )
                    continue;
            }

            if( !evt->IsActivate() && !immediateMode )
                m_frame->PopTool();

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

            if( immediateMode )
                break;
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

    if( immediateMode )
        m_frame->PopTool();

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
    Go( &SCH_DRAWING_TOOLS::PlaceComponent,      EE_ACTIONS::placeSymbol.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::PlaceComponent,      EE_ACTIONS::placePower.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::SingleClickPlace,    EE_ACTIONS::placeNoConnect.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::SingleClickPlace,    EE_ACTIONS::placeJunction.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::SingleClickPlace,    EE_ACTIONS::placeBusWireEntry.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::SingleClickPlace,    EE_ACTIONS::placeBusBusEntry.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::TwoClickPlace,       EE_ACTIONS::placeLabel.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::TwoClickPlace,       EE_ACTIONS::placeHierLabel.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::TwoClickPlace,       EE_ACTIONS::placeGlobalLabel.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::DrawSheet,           EE_ACTIONS::drawSheet.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::TwoClickPlace,       EE_ACTIONS::placeSheetPin.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::TwoClickPlace,       EE_ACTIONS::importSheetPin.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::TwoClickPlace,       EE_ACTIONS::placeSchematicText.MakeEvent() );
    Go( &SCH_DRAWING_TOOLS::PlaceImage,          EE_ACTIONS::placeImage.MakeEvent() );
}
