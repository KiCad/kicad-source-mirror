/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file modedit.cpp
 */

#include <fctsys.h>
#include <kiface_i.h>
#include <kiway.h>
#include <class_drawpanel.h>
#include <pcb_draw_panel_gal.h>
#include <confirm.h>
#include <gestfich.h>
#include <pgm_base.h>
#include <trigo.h>
#include <3d_viewer.h>
#include <wxPcbStruct.h>
#include <kicad_device_context.h>
#include <macros.h>
#include <invoke_pcb_dialog.h>
#include <class_pcb_layer_widget.h>

#include <class_board.h>
#include <class_module.h>
#include <class_edge_mod.h>

#include <ratsnest_data.h>
#include <pcbnew.h>
#include <protos.h>
#include <pcbnew_id.h>
#include <module_editor_frame.h>
#include <modview_frame.h>
#include <collectors.h>
#include <tool/tool_manager.h>
#include <tools/common_actions.h>

#include <dialog_edit_module_for_Modedit.h>
#include <dialog_move_exact.h>
#include <dialog_create_array.h>
#include <wildcards_and_files_ext.h>
#include <menus_helpers.h>
#include <footprint_wizard_frame.h>
#include <pcbnew_config.h>

#include <boost/bind.hpp>


// Functions defined in block_module_editor, but used here
// These 3 functions are used in modedit to rotate, mirror or move the
// whole footprint so they are called with force_all = true
void MirrorMarkedItems( MODULE* module, wxPoint offset, bool force_all = false );
void RotateMarkedItems( MODULE* module, wxPoint offset, bool force_all = false );
void MoveMarkedItemsExactly( MODULE* module, const wxPoint& centre,
                             const wxPoint& translation, double rotation,
                             bool force_all = false );


BOARD_ITEM* FOOTPRINT_EDIT_FRAME::ModeditLocateAndDisplay( int aHotKeyCode )
{
    BOARD_ITEM* item = GetCurItem();

    if( GetBoard()->m_Modules == NULL )
        return NULL;

    GENERAL_COLLECTORS_GUIDE guide = GetCollectorsGuide();

    // Assign to scanList the proper item types desired based on tool type
    // or hotkey that is in play.

    const KICAD_T* scanList = NULL;

    if( aHotKeyCode )
    {
        // @todo: add switch here and add calls to PcbGeneralLocateAndDisplay(
        // int aHotKeyCode ) when searching is needed from a hotkey handler
    }
    else
    {
        scanList = GENERAL_COLLECTOR::ModulesAndTheirItems;
    }

    m_Collector->Collect( GetBoard(), scanList, RefPos( true ), guide );

    // Remove redundancies: when an item is found, we can remove the module from list
    if( m_Collector->GetCount() > 1 )
    {
        for( int ii = 0; ii < m_Collector->GetCount(); ii++ )
        {
            item = (*m_Collector)[ii];

            if( item->Type() != PCB_MODULE_T )
                continue;

            m_Collector->Remove( ii );
            ii--;
        }
    }

    if( m_Collector->GetCount() <= 1 )
    {
        item = (*m_Collector)[0];
        SetCurItem( item );
    }
    else    // we can't figure out which item user wants, do popup menu so user can choose
    {
        wxMenu      itemMenu;

        // Give a title to the selection menu. It also allow to close the popup menu without any action
        wxMenuItem* item_title = new wxMenuItem( &itemMenu, wxID_NONE, _( "Selection Clarification" ) );

#ifdef __WINDOWS__
        wxFont      bold_font( *wxNORMAL_FONT );
        bold_font.SetWeight( wxFONTWEIGHT_BOLD );
        bold_font.SetStyle( wxFONTSTYLE_ITALIC );
        item_title->SetFont( bold_font );
#endif

        itemMenu.Append( item_title );
        itemMenu.AppendSeparator();

        int limit = std::min( MAX_ITEMS_IN_PICKER, m_Collector->GetCount() );

        for( int ii = 0; ii<limit; ++ii )
        {
            item = (*m_Collector)[ii];

            wxString    text = item->GetSelectMenuText();
            BITMAP_DEF  xpm  = item->GetMenuImage();

            AddMenuItem( &itemMenu,
                         ID_POPUP_PCB_ITEM_SELECTION_START + ii,
                         text,
                         KiBitmap( xpm ) );
        }

        // this menu's handler is void
        // PCB_BASE_FRAME::ProcessItemSelection()
        // and it calls SetCurItem() which in turn calls DisplayInfo() on the
        // item.
        m_canvas->SetAbortRequest( true );   // changed in false if an item
        PopupMenu( &itemMenu );              // m_AbortRequest = false if an item is selected

        m_canvas->MoveCursorToCrossHair();
        m_canvas->SetIgnoreMouseEvents( false );

        // The function ProcessItemSelection() has set the current item, return it.
        item = GetCurItem();
    }

    if( item )
    {
        SetMsgPanel( item );
    }

    return item;
}


void FOOTPRINT_EDIT_FRAME::LoadModuleFromBoard( wxCommandEvent& event )
{
    if( GetScreen()->IsModify() )
    {
        if( !IsOK( this,
                   _( "Current footprint changes will be lost and this operation cannot be undone. Continue?" ) ) )
            return;
    }

    if( ! Load_Module_From_BOARD( NULL ) )
        return;

    GetScreen()->ClearUndoRedoList();
    GetScreen()->ClrModify();

    if( m_Draw3DFrame )
        m_Draw3DFrame->NewDisplay();
}


void FOOTPRINT_EDIT_FRAME::Process_Special_Functions( wxCommandEvent& event )
{
    int        id = event.GetId();
    wxPoint    pos;

    INSTALL_UNBUFFERED_DC( dc, m_canvas );

    wxGetMousePosition( &pos.x, &pos.y );

    pos.y += 20;

    switch( id )
    {
    case wxID_CUT:
    case wxID_COPY:
    case ID_TOOLBARH_PCB_SELECT_LAYER:
    case ID_MODEDIT_PAD_SETTINGS:
    case ID_PCB_USER_GRID_SETUP:
    case ID_POPUP_PCB_ROTATE_TEXTEPCB:
    case ID_POPUP_PCB_EDIT_TEXTEPCB:
    case ID_POPUP_PCB_ROTATE_TEXTMODULE:
    case ID_POPUP_PCB_ROTATE_MODULE_CLOCKWISE:
    case ID_POPUP_PCB_ROTATE_MODULE_COUNTERCLOCKWISE:
    case ID_POPUP_PCB_EDIT_TEXTMODULE:
    case ID_POPUP_PCB_IMPORT_PAD_SETTINGS:
    case ID_POPUP_PCB_EXPORT_PAD_SETTINGS:
    case ID_POPUP_PCB_GLOBAL_IMPORT_PAD_SETTINGS:
    case ID_POPUP_PCB_STOP_CURRENT_DRAWING:
    case ID_POPUP_MODEDIT_EDIT_BODY_ITEM:
    case ID_POPUP_MODEDIT_EDIT_WIDTH_ALL_EDGE:
    case ID_POPUP_MODEDIT_EDIT_LAYER_ALL_EDGE:
    case ID_POPUP_MODEDIT_ENTER_EDGE_WIDTH:
    case ID_POPUP_PCB_DELETE_EDGE:
    case ID_POPUP_PCB_DELETE_TEXTMODULE:
    case ID_POPUP_PCB_DELETE_PAD:
    case ID_POPUP_DELETE_BLOCK:
    case ID_POPUP_PLACE_BLOCK:
    case ID_POPUP_ZOOM_BLOCK:
    case ID_POPUP_MIRROR_X_BLOCK:
    case ID_POPUP_ROTATE_BLOCK:
    case ID_POPUP_COPY_BLOCK:
        break;

    case ID_POPUP_CANCEL_CURRENT_COMMAND:
    default:
        if( m_canvas->IsMouseCaptured() )
        {
            //  for all other commands: stop the move in progress
            m_canvas->CallEndMouseCapture( &dc );
        }

        if( id != ID_POPUP_CANCEL_CURRENT_COMMAND )
            SetToolID( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor(), wxEmptyString );

        break;
    }

    switch( id )
    {
    case ID_EXIT:
        Close( true );
        break;

    case ID_MODEDIT_SELECT_CURRENT_LIB:
        {
            wxString library = SelectLibrary( GetCurrentLib() );

            if( library.size() )
            {
                Prj().SetRString( PROJECT::PCB_LIB_NICKNAME, library );
                updateTitle();
            }
        }
        break;

    case ID_OPEN_MODULE_VIEWER:
        {
            FOOTPRINT_VIEWER_FRAME* viewer = (FOOTPRINT_VIEWER_FRAME*) Kiway().Player( FRAME_PCB_MODULE_VIEWER, false );

            if( !viewer )
            {
                viewer = (FOOTPRINT_VIEWER_FRAME*) Kiway().Player( FRAME_PCB_MODULE_VIEWER, true );
                viewer->Show( true );
                viewer->Zoom_Automatique( false );
            }
            else
            {
                // On Windows, Raise() does not bring the window on screen, when iconized
                if( viewer->IsIconized() )
                    viewer->Iconize( false );

                viewer->Raise();

                // Raising the window does not set the focus on Linux.  This should work on
                // any platform.
                if( wxWindow::FindFocus() != viewer )
                    viewer->SetFocus();
            }
        }
        break;

    case ID_MODEDIT_DELETE_PART:
        DeleteModuleFromCurrentLibrary();
        break;

    case ID_MODEDIT_NEW_MODULE:
        {
            if( !Clear_Pcb( true ) )
                break;

            SetCrossHairPosition( wxPoint( 0, 0 ) );

            MODULE* module = CreateNewModule( wxEmptyString );

            if( module )        // i.e. if create module command not aborted
            {
                // Initialize data relative to nets and netclasses (for a new
                // module the defaults are used)
                // This is mandatory to handle and draw pads
                GetBoard()->BuildListOfNets();
                module->SetPosition( wxPoint( 0, 0 ) );

                if( GetBoard()->m_Modules )
                    GetBoard()->m_Modules->ClearFlags();

                Zoom_Automatique( false );
            }

            updateView();
            m_canvas->Refresh();

            GetScreen()->ClrModify();
        }
        break;

    case ID_MODEDIT_NEW_MODULE_FROM_WIZARD:
        {
            if( GetScreen()->IsModify() && !GetBoard()->IsEmpty() )
            {
                if( !IsOK( this,
                           _( "Current Footprint will be lost and this operation cannot be undone. Continue ?" ) ) )
                    break;
            }

            FOOTPRINT_WIZARD_FRAME* wizard = (FOOTPRINT_WIZARD_FRAME*) Kiway().Player(
                        FRAME_PCB_FOOTPRINT_WIZARD_MODAL, true );

            if( wizard->ShowModal( NULL, this ) )
            {
                // Creates the new footprint from python script wizard
                MODULE* module = wizard->GetBuiltFootprint();

                if( module == NULL )        // i.e. if create module command aborted
                    break;

                Clear_Pcb( false );

                SetCrossHairPosition( wxPoint( 0, 0 ) );

                //  Add the new object to board
                GetBoard()->Add( module, ADD_APPEND );

                // Initialize data relative to nets and netclasses (for a new
                // module the defaults are used)
                // This is mandatory to handle and draw pads
                GetBoard()->BuildListOfNets();
                module->SetPosition( wxPoint( 0, 0 ) );
                module->ClearFlags();

                Zoom_Automatique( false );
                updateView();
                m_canvas->Refresh();

                if( m_Draw3DFrame )
                    m_Draw3DFrame->NewDisplay();

                GetScreen()->ClrModify();
            }

            wizard->Destroy();
        }
        break;

    case ID_MODEDIT_SAVE_LIBMODULE:
        if( GetBoard()->m_Modules && GetCurrentLib().size() )
        {
            SaveFootprintInLibrary( GetCurrentLib(), GetBoard()->m_Modules, true, true );
            GetScreen()->ClrModify();
        }
        break;

    case ID_MODEDIT_INSERT_MODULE_IN_BOARD:
    case ID_MODEDIT_UPDATE_MODULE_IN_BOARD:
        {
            // update module in the current board,
            // not just add it to the board with total disregard for the netlist...
            PCB_EDIT_FRAME* pcbframe = (PCB_EDIT_FRAME*) Kiway().Player( FRAME_PCB, false );

            if( pcbframe == NULL )      // happens when the board editor is not active (or closed)
            {
                wxMessageBox( _("No board currently edited" ) );
                break;
            }

            BOARD*          mainpcb  = pcbframe->GetBoard();
            MODULE*         source_module  = NULL;
            MODULE*         module_in_edit = GetBoard()->m_Modules;

            // Search the old module (source) if exists
            // Because this source could be deleted when editing the main board...
            if( module_in_edit->GetLink() )        // this is not a new module ...
            {
                source_module = mainpcb->m_Modules;

                for( ; source_module != NULL; source_module = (MODULE*) source_module->Next() )
                {
                    if( module_in_edit->GetLink() == source_module->GetTimeStamp() )
                        break;
                }
            }

            if( ( source_module == NULL )
              && ( id == ID_MODEDIT_UPDATE_MODULE_IN_BOARD ) ) // source not found
            {
                wxString msg;
                msg.Printf( _( "Unable to find the footprint source on the main board" ) );
                msg << _( "\nCannot update the footprint" );
                DisplayError( this, msg );
                break;
            }

            if( ( source_module != NULL )
              && ( id == ID_MODEDIT_INSERT_MODULE_IN_BOARD ) ) // source not found
            {
                wxString msg;
                msg.Printf( _( "A footprint source was found on the main board" ) );
                msg << _( "\nCannot insert this footprint" );
                DisplayError( this, msg );
                break;
            }

            m_toolManager->RunAction( COMMON_ACTIONS::selectionClear, true );

            // Create the "new" module
            MODULE* newmodule = new MODULE( *module_in_edit );
            newmodule->SetParent( mainpcb );
            newmodule->SetLink( 0 );

            // Put the footprint in the main pcb linked list.
            mainpcb->Add( newmodule );

            if( source_module )         // this is an update command
            {
                // In the main board,
                // the new module replace the old module (pos, orient, ref, value
                // and connexions are kept)
                // and the source_module (old module) is deleted
                PICKED_ITEMS_LIST pickList;

                pcbframe->Exchange_Module( source_module, newmodule, &pickList );
                newmodule->SetTimeStamp( module_in_edit->GetLink() );

                if( pickList.GetCount() )
                    pcbframe->SaveCopyInUndoList( pickList, UR_UNSPECIFIED );
            }
            else        // This is an insert command
            {
                wxPoint cursor_pos = pcbframe->GetCrossHairPosition();

                pcbframe->SetCrossHairPosition( wxPoint( 0, 0 ) );
                pcbframe->PlaceModule( newmodule, NULL );
                newmodule->SetPosition( wxPoint( 0, 0 ) );
                pcbframe->SetCrossHairPosition( cursor_pos );
                newmodule->SetTimeStamp( GetNewTimeStamp() );
                pcbframe->SaveCopyInUndoList( newmodule, UR_NEW );

                if( IsGalCanvasActive() )
                {
                    KIGFX::VIEW* view = pcbframe->GetGalCanvas()->GetView();

                    newmodule->RunOnChildren( boost::bind( &KIGFX::VIEW::Add, view, _1 ) );
                    view->Add( newmodule );
                }
            }

            newmodule->ClearFlags();
            GetScreen()->ClrModify();
            pcbframe->SetCurItem( NULL );
            mainpcb->m_Status_Pcb = 0;

            if( IsGalCanvasActive() )
            {
                RN_DATA* ratsnest = pcbframe->GetBoard()->GetRatsnest();
                ratsnest->Update( newmodule );
                ratsnest->Recalculate();
                GetGalCanvas()->ForceRefresh();
            }
        }
        break;

    case ID_MODEDIT_IMPORT_PART:
        if( ! Clear_Pcb( true ) )
            break;                  // //this command is aborted

        SetCrossHairPosition( wxPoint( 0, 0 ) );
        Import_Module();

        if( GetBoard()->m_Modules )
            GetBoard()->m_Modules->ClearFlags();

        GetScreen()->ClrModify();
        Zoom_Automatique( false );
        m_canvas->Refresh();

        if( m_Draw3DFrame )
            m_Draw3DFrame->NewDisplay();

        break;

    case ID_MODEDIT_EXPORT_PART:
        if( GetBoard()->m_Modules )
            Export_Module( GetBoard()->m_Modules );
        break;

    case ID_MODEDIT_CREATE_NEW_LIB_AND_SAVE_CURRENT_PART:
        if( GetBoard()->m_Modules )
        {
            // CreateModuleLibrary() only creates a new library, does not save footprint
            wxString libPath = CreateNewLibrary();
            if( libPath.size() )
                SaveCurrentModule( &libPath );
        }
        break;

    case ID_MODEDIT_SHEET_SET:
        break;

    case ID_MODEDIT_LOAD_MODULE:
        wxLogDebug( wxT( "Loading module from library " ) + getLibPath() );

        if( ! Clear_Pcb( true ) )
            break;

        SetCrossHairPosition( wxPoint( 0, 0 ) );

        LoadModuleFromLibrary( GetCurrentLib(), Prj().PcbFootprintLibs(), true );

        if( GetBoard()->m_Modules )
             GetBoard()->m_Modules->ClearFlags();

        // if either m_Reference or m_Value are gone, reinstall them -
        // otherwise you cannot see what you are doing on board
        if( GetBoard() && GetBoard()->m_Modules )
        {
            TEXTE_MODULE* ref = &GetBoard()->m_Modules->Reference();
            TEXTE_MODULE* val = &GetBoard()->m_Modules->Value();

            if( val && ref )
            {
                ref->SetType( TEXTE_MODULE::TEXT_is_REFERENCE );    // just in case ...

                if( ref->GetLength() == 0 )
                    ref->SetText( wxT( "Ref**" ) );

                val->SetType( TEXTE_MODULE::TEXT_is_VALUE );        // just in case ...

                if( val->GetLength() == 0 )
                    val->SetText( wxT( "Val**" ) );
            }
        }

        Zoom_Automatique( false );

        if( m_Draw3DFrame )
            m_Draw3DFrame->NewDisplay();

        GetScreen()->ClrModify();

        updateView();
        m_canvas->Refresh();

        break;

    case ID_MODEDIT_PAD_SETTINGS:
        InstallPadOptionsFrame( NULL );
        break;

    case ID_MODEDIT_CHECK:
        // Currently: not implemented
        break;

    case ID_MODEDIT_EDIT_MODULE_PROPERTIES:
        if( GetBoard()->m_Modules )
        {
            SetCurItem( GetBoard()->m_Modules );

            DIALOG_MODULE_MODULE_EDITOR dialog( this, (MODULE*) GetScreen()->GetCurItem() );

            int ret = dialog.ShowModal();
            GetScreen()->GetCurItem()->ClearFlags();
            GetBoard()->m_Modules.GetFirst()->ViewUpdate();

            if( ret > 0 )
                m_canvas->Refresh();
        }
        break;

    case ID_POPUP_CLOSE_CURRENT_TOOL:
        break;

    case ID_POPUP_CANCEL_CURRENT_COMMAND:
        break;

    case ID_POPUP_PCB_ROTATE_MODULE_COUNTERCLOCKWISE:
        m_canvas->MoveCursorToCrossHair();
        Rotate_Module( NULL, (MODULE*) GetScreen()->GetCurItem(), 900, true );
        m_canvas->Refresh();
        break;

    case ID_POPUP_PCB_ROTATE_MODULE_CLOCKWISE:
        m_canvas->MoveCursorToCrossHair();
        Rotate_Module( NULL, (MODULE*) GetScreen()->GetCurItem(), -900, true );
        m_canvas->Refresh();
        break;

    case ID_POPUP_PCB_EDIT_MODULE_PRMS:
        {
            DIALOG_MODULE_MODULE_EDITOR dialog( this, (MODULE*) GetScreen()->GetCurItem() );
            dialog.ShowModal();
            GetScreen()->GetCurItem()->ClearFlags();
            m_canvas->MoveCursorToCrossHair();
            m_canvas->Refresh();
        }
        break;

    case ID_POPUP_PCB_MOVE_PAD_REQUEST:
        m_canvas->MoveCursorToCrossHair();
        StartMovePad( (D_PAD*) GetScreen()->GetCurItem(), &dc, false );
        break;

    case ID_POPUP_PCB_EDIT_PAD:
        InstallPadOptionsFrame( (D_PAD*) GetScreen()->GetCurItem() );
        m_canvas->MoveCursorToCrossHair();
    break;

    case ID_POPUP_PCB_DELETE_PAD:
        SaveCopyInUndoList( GetBoard()->m_Modules, UR_MODEDIT );
        DeletePad( (D_PAD*) GetScreen()->GetCurItem(), false );
        SetCurItem( NULL );
        m_canvas->MoveCursorToCrossHair();
        break;

    case ID_POPUP_PCB_DUPLICATE_ITEM:
        duplicateItems( false );
        break;

    case ID_POPUP_PCB_DUPLICATE_ITEM_AND_INCREMENT:
        duplicateItems( true );
        break;

    case ID_POPUP_PCB_MOVE_EXACT:
        moveExact();
        break;

    case ID_POPUP_PCB_CREATE_ARRAY:
        createArray();
        break;

    case ID_POPUP_PCB_IMPORT_PAD_SETTINGS:
        SaveCopyInUndoList( GetBoard()->m_Modules, UR_MODEDIT );
        m_canvas->MoveCursorToCrossHair();
        Import_Pad_Settings( (D_PAD*) GetScreen()->GetCurItem(), true );
        break;

    case ID_POPUP_PCB_GLOBAL_IMPORT_PAD_SETTINGS:
        SaveCopyInUndoList( GetBoard()->m_Modules, UR_MODEDIT );
        // Calls the global change dialog:
        DlgGlobalChange_PadSettings( (D_PAD*) GetScreen()->GetCurItem() );
        m_canvas->MoveCursorToCrossHair();
        break;

    case ID_POPUP_PCB_EXPORT_PAD_SETTINGS:
        m_canvas->MoveCursorToCrossHair();
        Export_Pad_Settings( (D_PAD*) GetScreen()->GetCurItem() );
        break;

    case ID_POPUP_PCB_EDIT_TEXTMODULE:
        InstallTextModOptionsFrame( static_cast<TEXTE_MODULE*>( GetScreen()->GetCurItem() ), &dc );
        m_canvas->MoveCursorToCrossHair();
        break;

    case ID_POPUP_PCB_MOVE_TEXTMODULE_REQUEST:
        m_canvas->MoveCursorToCrossHair();
        StartMoveTexteModule( static_cast<TEXTE_MODULE*>( GetScreen()->GetCurItem() ), &dc );
        break;

    case ID_POPUP_PCB_ROTATE_TEXTMODULE:
        RotateTextModule( static_cast<TEXTE_MODULE*>( GetScreen()->GetCurItem() ), &dc );
        m_canvas->MoveCursorToCrossHair();
        break;

    case ID_POPUP_PCB_DELETE_TEXTMODULE:
        SaveCopyInUndoList( GetBoard()->m_Modules, UR_MODEDIT );
        DeleteTextModule( static_cast<TEXTE_MODULE*>( GetScreen()->GetCurItem() ) );
        SetCurItem( NULL );
        m_canvas->MoveCursorToCrossHair();
        break;

    case ID_POPUP_PCB_MOVE_EDGE:
        Start_Move_EdgeMod( static_cast<EDGE_MODULE*>( GetScreen()->GetCurItem() ), &dc );
        m_canvas->MoveCursorToCrossHair();
        break;

    case ID_POPUP_PCB_STOP_CURRENT_DRAWING:
        m_canvas->MoveCursorToCrossHair();

        if( GetScreen()->GetCurItem()->IsNew() )
        {
            End_Edge_Module( (EDGE_MODULE*) GetScreen()->GetCurItem() );
            SetCurItem( NULL );
        }
        break;

    case ID_POPUP_MODEDIT_ENTER_EDGE_WIDTH:
        {
            EDGE_MODULE* edge = NULL;

            if( GetScreen()->GetCurItem()
              && ( GetScreen()->GetCurItem()->Type() == PCB_MODULE_EDGE_T ) )
            {
                edge = (EDGE_MODULE*) GetScreen()->GetCurItem();
            }

            Enter_Edge_Width( edge );
            m_canvas->MoveCursorToCrossHair();

            if( edge )
                m_canvas->Refresh();
        }
        break;

    case  ID_POPUP_MODEDIT_EDIT_BODY_ITEM :
        m_canvas->MoveCursorToCrossHair();
        InstallFootprintBodyItemPropertiesDlg( (EDGE_MODULE*) GetScreen()->GetCurItem() );
        m_canvas->Refresh();
        break;

    case ID_POPUP_MODEDIT_EDIT_WIDTH_ALL_EDGE:
        m_canvas->MoveCursorToCrossHair();
        Edit_Edge_Width( NULL );
        m_canvas->Refresh();
        break;

    case ID_POPUP_MODEDIT_EDIT_LAYER_ALL_EDGE:
        m_canvas->MoveCursorToCrossHair();
        Edit_Edge_Layer( NULL );
        m_canvas->Refresh();
        break;

    case ID_POPUP_PCB_DELETE_EDGE:
        SaveCopyInUndoList( GetBoard()->m_Modules, UR_MODEDIT );
        m_canvas->MoveCursorToCrossHair();
        RemoveStruct( GetScreen()->GetCurItem() );
        SetCurItem( NULL );
        break;

    case ID_MODEDIT_MODULE_ROTATE:
    case ID_MODEDIT_MODULE_MIRROR:
    case ID_MODEDIT_MODULE_MOVE_EXACT:
        SaveCopyInUndoList( GetBoard()->m_Modules, UR_MODEDIT );
        Transform( (MODULE*) GetScreen()->GetCurItem(), id );
        m_canvas->Refresh();
        break;

    case ID_PCB_DRAWINGS_WIDTHS_SETUP:
        InstallOptionsFrame( pos );
        break;

    case ID_PCB_PAD_SETUP:
        {
            BOARD_ITEM* item = GetCurItem();

            if( item )
            {
                if( item->Type() != PCB_PAD_T )
                    item = NULL;
            }

            InstallPadOptionsFrame( (D_PAD*) item );
        }
        break;

    case ID_PCB_USER_GRID_SETUP:
        InvokeDialogGrid();
        break;

    case ID_POPUP_PLACE_BLOCK:
        GetScreen()->m_BlockLocate.SetCommand( BLOCK_MOVE );
        m_canvas->SetAutoPanRequest( false );
        HandleBlockPlace( &dc );
        break;

    case ID_POPUP_COPY_BLOCK:
        GetScreen()->m_BlockLocate.SetCommand( BLOCK_COPY );
        GetScreen()->m_BlockLocate.SetMessageBlock( this );
        m_canvas->SetAutoPanRequest( false );
        HandleBlockPlace( &dc );
        break;

    case ID_POPUP_ZOOM_BLOCK:
        GetScreen()->m_BlockLocate.SetCommand( BLOCK_ZOOM );
        GetScreen()->m_BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( &dc );
        break;

    case ID_POPUP_DELETE_BLOCK:
        GetScreen()->m_BlockLocate.SetCommand( BLOCK_DELETE );
        GetScreen()->m_BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( &dc );
        break;

    case ID_POPUP_ROTATE_BLOCK:
        GetScreen()->m_BlockLocate.SetCommand( BLOCK_ROTATE );
        GetScreen()->m_BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( &dc );
        break;

    case ID_POPUP_MIRROR_X_BLOCK:
        GetScreen()->m_BlockLocate.SetCommand( BLOCK_MIRROR_X );
        GetScreen()->m_BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( &dc );
        break;

    case ID_POPUP_MOVE_BLOCK_EXACT:
        GetScreen()->m_BlockLocate.SetCommand( BLOCK_MOVE_EXACT );
        GetScreen()->m_BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( &dc );
        break;

    case ID_GEN_IMPORT_DXF_FILE:
        InvokeDXFDialogModuleImport( this, GetBoard()->m_Modules );
        m_canvas->Refresh();
        break;

    default:
        DisplayError( this,
                      wxT( "FOOTPRINT_EDIT_FRAME::Process_Special_Functions error" ) );
        break;
    }
}


void FOOTPRINT_EDIT_FRAME::moveExact()
{
    wxPoint translation;
    double rotation = 0;

    DIALOG_MOVE_EXACT dialog( this, translation, rotation );
    int ret = dialog.ShowModal();

    if( ret == wxID_OK )
    {
        SaveCopyInUndoList( GetBoard()->m_Modules, UR_MODEDIT );

        BOARD_ITEM* item = GetScreen()->GetCurItem();

        item->Move( translation );
        item->Rotate( item->GetPosition(), rotation );
        m_canvas->Refresh();
    }

    m_canvas->MoveCursorToCrossHair();
}


void FOOTPRINT_EDIT_FRAME::duplicateItems( bool aIncrement )
{
    BOARD_ITEM* item = GetScreen()->GetCurItem();

    PCB_BASE_EDIT_FRAME::duplicateItem( item, aIncrement );
}


void FOOTPRINT_EDIT_FRAME::Transform( MODULE* module, int transform )
{
    switch( transform )
    {
    case ID_MODEDIT_MODULE_ROTATE:
        RotateMarkedItems( module, wxPoint(0,0), true );
        break;

    case ID_MODEDIT_MODULE_MIRROR:
        MirrorMarkedItems( module, wxPoint(0,0), true );
        break;

    case ID_MODEDIT_MODULE_MOVE_EXACT:
    {
        wxPoint translation;
        double rotation = 0;

        DIALOG_MOVE_EXACT dialog( this, translation, rotation  );
        int ret = dialog.ShowModal();

        if( ret == wxID_OK )
        {
            MoveMarkedItemsExactly( module, wxPoint(0, 0),
                                    translation, rotation, true );
        }

        break;
    }

    default:
        DisplayInfoMessage( this, wxT( "Not available" ) );
        break;
    }

    module->CalculateBoundingBox();
    OnModify();
}


void FOOTPRINT_EDIT_FRAME::OnVerticalToolbar( wxCommandEvent& aEvent )
{
    int id = aEvent.GetId();

    SetToolID( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor(), wxEmptyString );

    switch( id )
    {
    case ID_NO_TOOL_SELECTED:
        break;

    case ID_MODEDIT_LINE_TOOL:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add line" ) );
        break;

    case ID_MODEDIT_ARC_TOOL:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add arc" ) );
        break;

    case ID_MODEDIT_CIRCLE_TOOL:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add circle" ) );
        break;

    case ID_MODEDIT_TEXT_TOOL:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add text" ) );
        break;

    case ID_MODEDIT_ANCHOR_TOOL:
        SetToolID( id, wxCURSOR_PENCIL, _( "Place anchor" ) );
        break;

    case ID_MODEDIT_PLACE_GRID_COORD:
        SetToolID( id, wxCURSOR_PENCIL, _( "Set grid origin" ) );
        break;

    case ID_MODEDIT_PAD_TOOL:
        if( GetBoard()->m_Modules )
        {
            SetToolID( id, wxCURSOR_PENCIL, _( "Add pad" ) );
        }
        else
        {
            SetToolID( id, wxCURSOR_ARROW, _( "Pad settings" ) );
            InstallPadOptionsFrame( NULL );
            SetToolID( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor(), wxEmptyString );
        }
        break;

    case ID_MODEDIT_DELETE_TOOL:
        SetToolID( id, wxCURSOR_BULLSEYE, _( "Delete item" ) );
        break;

    default:
        wxFAIL_MSG( wxT( "Unknown command id." ) );
        SetToolID( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor(), wxEmptyString );
    }
}


EDA_COLOR_T FOOTPRINT_EDIT_FRAME::GetGridColor() const
{
    return g_ColorsSettings.GetItemColor( GRID_VISIBLE );
}


void FOOTPRINT_EDIT_FRAME::SetActiveLayer( LAYER_ID aLayer )
{
    PCB_BASE_FRAME::SetActiveLayer( aLayer );

    GetGalCanvas()->SetHighContrastLayer( aLayer );

    m_Layers->SelectLayer( GetActiveLayer() );
    m_Layers->OnLayerSelected();

    if( IsGalCanvasActive() )
        GetGalCanvas()->Refresh();
}

