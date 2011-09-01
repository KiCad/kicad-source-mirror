/****************/
/* modedit.cpp  */
/****************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "gestfich.h"
#include "appl_wxstruct.h"
#include "pcbnew_id.h"
#include "trigo.h"

#include "3d_viewer.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "module_editor_frame.h"
#include "protos.h"

#include "kicad_device_context.h"

#include "dialog_edit_module_for_Modedit.h"

#include "collectors.h"


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

    m_Collector->Collect( GetBoard(), scanList, GetScreen()->RefPos( true ), guide );

    /* Remove redundancies: when an item is found, we can remove the module from list */
    if( m_Collector->GetCount() > 1 )
    {
        for( int ii = 0; ii < m_Collector->GetCount(); ii++ )
        {
            item = (*m_Collector)[ii];

            if( item->Type() != TYPE_MODULE )
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

        /* Give a title to the selection menu. This is also a cancel menu item **/
        wxMenuItem* item_title = new wxMenuItem( &itemMenu, -1, _( "Selection Clarification" ) );

#ifdef __WINDOWS__
        wxFont      bold_font( *wxNORMAL_FONT );
        bold_font.SetWeight( wxFONTWEIGHT_BOLD );
        bold_font.SetStyle( wxFONTSTYLE_ITALIC );
        item_title->SetFont( bold_font );
#endif

        itemMenu.Append( item_title );
        itemMenu.AppendSeparator();

        int limit = MIN( MAX_ITEMS_IN_PICKER, m_Collector->GetCount() );

        for( int ii = 0; ii<limit; ++ii )
        {
            wxString    text;
            BITMAP_DEF  xpm;

            item = (*m_Collector)[ii];

            text = item->GetSelectMenuText();
            xpm  = item->GetMenuImage();

            AddMenuItem( &itemMenu,
                         ID_POPUP_PCB_ITEM_SELECTION_START + ii,
                         text,
                         xpm );
        }

        // this menu's handler is void
        // PCB_BASE_FRAME::ProcessItemSelection()
        // and it calls SetCurItem() which in turn calls DisplayInfo() on the
        // item.
        DrawPanel->m_AbortRequest = true;   // changed in false if an item
        PopupMenu( &itemMenu );             // m_AbortRequest = false if an
                                            // item is selected

        DrawPanel->MoveCursorToCrossHair();
        DrawPanel->m_IgnoreMouseEvents = false;

        // The function ProcessItemSelection() has set the current item, return it.
        item = GetCurItem();
    }

    if( item )
    {
        item->DisplayInfo( this );
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
    bool       redraw = false;

    INSTALL_UNBUFFERED_DC( dc, DrawPanel );

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
    case ID_POPUP_PCB_EDIT_EDGE:
    case ID_POPUP_PCB_EDIT_WIDTH_CURRENT_EDGE:
    case ID_POPUP_PCB_EDIT_WIDTH_ALL_EDGE:
    case ID_POPUP_PCB_EDIT_LAYER_CURRENT_EDGE:
    case ID_POPUP_PCB_EDIT_LAYER_ALL_EDGE:
    case ID_POPUP_PCB_ENTER_EDGE_WIDTH:
    case ID_POPUP_DELETE_BLOCK:
    case ID_POPUP_PLACE_BLOCK:
    case ID_POPUP_ZOOM_BLOCK:
    case ID_POPUP_MIRROR_X_BLOCK:
    case ID_POPUP_ROTATE_BLOCK:
    case ID_POPUP_COPY_BLOCK:
        break;

    case ID_POPUP_CANCEL_CURRENT_COMMAND:
    default:
        if( DrawPanel->IsMouseCaptured() )
        {
            //  for all other commands: stop the move in progress
            DrawPanel->m_endMouseCaptureCallback( DrawPanel, &dc );
        }

        if( id != ID_POPUP_CANCEL_CURRENT_COMMAND )
            SetToolID( ID_NO_TOOL_SELECTED, DrawPanel->GetDefaultCursor(), wxEmptyString );

        break;
    }

    switch( id )
    {
    case ID_EXIT:
        Close( true );
        break;

    case ID_MODEDIT_SELECT_CURRENT_LIB:
        Select_Active_Library();
        break;

    case ID_MODEDIT_DELETE_PART:
    {
        wxFileName fn = wxFileName( wxEmptyString, m_CurrentLib, ModuleFileExtension );
        wxString   full_libraryfilename = wxGetApp().FindLibraryPath( fn );

        if( wxFileName::FileExists( full_libraryfilename ) )
            Delete_Module_In_Library( full_libraryfilename );

        break;
    }

    case ID_MODEDIT_NEW_MODULE:
    {
        Clear_Pcb( true );
        GetScreen()->ClearUndoRedoList();
        SetCurItem( NULL );
        GetScreen()->SetCrossHairPosition( wxPoint( 0, 0 ) );

        MODULE* module = Create_1_Module( wxEmptyString );

        if( module )        // i.e. if create module command not aborted
        {
            // Initialize data relative to nets and netclasses (for a new
            // module the defaults are used)
            // This is mandatory to handle and draw pads
            GetBoard()->m_NetInfo->BuildListOfNets();
            redraw = true;
            module->SetPosition( wxPoint( 0, 0 ) );

            if( GetBoard()->m_Modules )
                GetBoard()->m_Modules->m_Flags = 0;

            Zoom_Automatique( false );
        }
        break;
    }

    case ID_MODEDIT_SAVE_LIBMODULE:
        if( GetBoard()->m_Modules == NULL )
            break;
    {
        wxFileName fn;
        fn = wxFileName( wxEmptyString, m_CurrentLib, ModuleFileExtension );
        wxString   full_filename = wxGetApp().FindLibraryPath( fn );
        Save_Module_In_Library( full_filename, GetBoard()->m_Modules, true, true );
        GetScreen()->ClrModify();
        break;
    }

    case ID_MODEDIT_INSERT_MODULE_IN_BOARD:
    case ID_MODEDIT_UPDATE_MODULE_IN_BOARD:
    {
        // update module in the current board,
        // not just add it to the board with total disregard for the netlist...
        PCB_EDIT_FRAME* pcbframe = (PCB_EDIT_FRAME*) GetParent();
        BOARD*          mainpcb  = pcbframe->GetBoard();
        MODULE*         source_module  = NULL;
        MODULE*         module_in_edit = GetBoard()->m_Modules;

        // Search the old module (source) if exists
        // Because this source could be deleted when editing the main board...
        if( module_in_edit->m_Link )        // this is not a new module ...
        {
            source_module = mainpcb->m_Modules;

            for( ; source_module != NULL; source_module = (MODULE*) source_module->Next() )
            {
                if( module_in_edit->m_Link == source_module->m_TimeStamp )
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

        // Create the "new" module
        MODULE* newmodule = new MODULE( mainpcb );
        newmodule->Copy( module_in_edit );
        newmodule->m_Link = 0;

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
            newmodule->m_TimeStamp = module_in_edit->m_Link;

            if( pickList.GetCount() )
                pcbframe->SaveCopyInUndoList( pickList, UR_UNSPECIFIED );
        }
        else        // This is an insert command
        {
            wxPoint cursor_pos = pcbframe->GetScreen()->GetCrossHairPosition();
            pcbframe->GetScreen()->SetCrossHairPosition( wxPoint( 0, 0 ) );
            pcbframe->Place_Module( newmodule, NULL );
            pcbframe->GetScreen()->SetCrossHairPosition( cursor_pos );
            newmodule->m_TimeStamp = GetTimeStamp();
            pcbframe->SaveCopyInUndoList( newmodule, UR_NEW );
        }

        newmodule->m_Flags = 0;
        GetScreen()->ClrModify();
        pcbframe->SetCurItem( NULL );
        mainpcb->m_Status_Pcb = 0;
    }
    break;

    case ID_MODEDIT_IMPORT_PART:
        if( ! Clear_Pcb( true ) )
            break;                  // //this command is aborted

        GetScreen()->ClearUndoRedoList();
        SetCurItem( NULL );
        GetScreen()->SetCrossHairPosition( wxPoint( 0, 0 ) );
        Import_Module();
        redraw = true;

        if( GetBoard()->m_Modules )
            GetBoard()->m_Modules->m_Flags = 0;

        GetScreen()->ClrModify();
        Zoom_Automatique( false );

        if( m_Draw3DFrame )
            m_Draw3DFrame->NewDisplay();

        break;

    case ID_MODEDIT_EXPORT_PART:
        if( GetBoard()->m_Modules )
            Export_Module( GetBoard()->m_Modules, false );
        break;

    case ID_MODEDIT_CREATE_NEW_LIB_AND_SAVE_CURRENT_PART:
        if( GetBoard()->m_Modules )
            Export_Module( GetBoard()->m_Modules, true );
        break;

    case ID_MODEDIT_SHEET_SET:
        break;

    case ID_MODEDIT_LOAD_MODULE:
    {
        wxString full_libraryfilename;

        if( !m_CurrentLib.IsEmpty() )
        {
            wxFileName fn = wxFileName( wxEmptyString, m_CurrentLib, ModuleFileExtension );
            full_libraryfilename = wxGetApp().FindLibraryPath( fn );
        }

        wxLogDebug( wxT( "Loading module from library " ) + full_libraryfilename );

        GetScreen()->ClearUndoRedoList();
        SetCurItem( NULL );
        Clear_Pcb( true );
        GetScreen()->SetCrossHairPosition( wxPoint( 0, 0 ) );
        Load_Module_From_Library( full_libraryfilename, NULL );
        redraw = true;
    }

        if( GetBoard()->m_Modules )
            GetBoard()->m_Modules->m_Flags = 0;

        // if either m_Reference or m_Value are gone, reinstall them -
        // otherwise you cannot see what you are doing on board
        if( GetBoard() && GetBoard()->m_Modules )
        {
            TEXTE_MODULE* ref = GetBoard()->m_Modules->m_Reference;
            TEXTE_MODULE* val = GetBoard()->m_Modules->m_Value;

            if( val && ref )
            {
                ref->m_Type = TEXT_is_REFERENCE;    // just in case ...

                if( ref->m_Text.Length() == 0 )
                    ref->m_Text = L"Ref**";

                val->m_Type = TEXT_is_VALUE;        // just in case ...

                if( val->m_Text.Length() == 0 )
                    val->m_Text = L"Val**";
            }
        }

        GetScreen()->ClrModify();
        Zoom_Automatique( false );

        if( m_Draw3DFrame )
            m_Draw3DFrame->NewDisplay();

        break;

    case ID_MODEDIT_PAD_SETTINGS:
        InstallPadOptionsFrame( NULL );
        break;

    case ID_MODEDIT_CHECK:
        break;

    case ID_MODEDIT_EDIT_MODULE_PROPERTIES:
        if( GetBoard()->m_Modules )
        {
            SetCurItem( GetBoard()->m_Modules );
            DIALOG_MODULE_MODULE_EDITOR dialog( this, (MODULE*) GetScreen()-> GetCurItem() );
            int ret = dialog.ShowModal();
            GetScreen()->GetCurItem()->m_Flags = 0;

            if( ret > 0 )
                DrawPanel->Refresh();
        }
        break;

    case ID_POPUP_CLOSE_CURRENT_TOOL:
        break;

    case ID_POPUP_CANCEL_CURRENT_COMMAND:
        break;

    case ID_POPUP_PCB_ROTATE_MODULE_COUNTERCLOCKWISE:
        DrawPanel->MoveCursorToCrossHair();
        Rotate_Module( NULL, (MODULE*) GetScreen()->GetCurItem(), 900, true );
        redraw = true;
        break;

    case ID_POPUP_PCB_ROTATE_MODULE_CLOCKWISE:
        DrawPanel->MoveCursorToCrossHair();
        Rotate_Module( NULL, (MODULE*) GetScreen()->GetCurItem(), -900, true );
        redraw = true;
        break;

    case ID_POPUP_PCB_EDIT_MODULE:
    {
        DIALOG_MODULE_MODULE_EDITOR dialog( this, (MODULE*) GetScreen()->GetCurItem() );
        int ret = dialog.ShowModal();
        GetScreen()->GetCurItem()->m_Flags = 0;
        GetScreen()->GetCurItem()->m_Flags = 0;
        DrawPanel->MoveCursorToCrossHair();

        if( ret > 0 )
            DrawPanel->Refresh();
    }
    break;

    case ID_POPUP_PCB_MOVE_PAD_REQUEST:
    {
        DrawPanel->MoveCursorToCrossHair();
        StartMovePad( (D_PAD*) GetScreen()->GetCurItem(), &dc );
    }
    break;

    case ID_POPUP_PCB_EDIT_PAD:
    {
        InstallPadOptionsFrame( (D_PAD*) GetScreen()->GetCurItem() );
        DrawPanel->MoveCursorToCrossHair();
    }
    break;

    case ID_POPUP_PCB_DELETE_PAD:
        SaveCopyInUndoList( GetBoard()->m_Modules, UR_MODEDIT );
        DeletePad( (D_PAD*) GetScreen()->GetCurItem(), false );
        SetCurItem( NULL );
        DrawPanel->MoveCursorToCrossHair();
        break;

    case ID_POPUP_PCB_IMPORT_PAD_SETTINGS:
        SaveCopyInUndoList( GetBoard()->m_Modules, UR_MODEDIT );
        DrawPanel->MoveCursorToCrossHair();
        Import_Pad_Settings( (D_PAD*) GetScreen()->GetCurItem(), true );
        break;

    case ID_POPUP_PCB_GLOBAL_IMPORT_PAD_SETTINGS:
        SaveCopyInUndoList( GetBoard()->m_Modules, UR_MODEDIT );
        Global_Import_Pad_Settings( (D_PAD*) GetScreen()->GetCurItem(), true );
        DrawPanel->MoveCursorToCrossHair();
        break;

    case ID_POPUP_PCB_EXPORT_PAD_SETTINGS:
        DrawPanel->MoveCursorToCrossHair();
        Export_Pad_Settings( (D_PAD*) GetScreen()->GetCurItem() );
        break;

    case ID_POPUP_PCB_EDIT_TEXTMODULE:
    {
        InstallTextModOptionsFrame( (TEXTE_MODULE*) GetScreen()->GetCurItem(), &dc );
        DrawPanel->MoveCursorToCrossHair();
    }
    break;

    case ID_POPUP_PCB_MOVE_TEXTMODULE_REQUEST:
    {
        DrawPanel->MoveCursorToCrossHair();
        StartMoveTexteModule( (TEXTE_MODULE*) GetScreen()->GetCurItem(), &dc );
    }
    break;

    case ID_POPUP_PCB_ROTATE_TEXTMODULE:
    {
        RotateTextModule( (TEXTE_MODULE*) GetScreen()->GetCurItem(), &dc );
        DrawPanel->MoveCursorToCrossHair();
    }
    break;

    case ID_POPUP_PCB_DELETE_TEXTMODULE:
        SaveCopyInUndoList( GetBoard()->m_Modules, UR_MODEDIT );
        DeleteTextModule( (TEXTE_MODULE*) GetScreen()->GetCurItem() );
        SetCurItem( NULL );
        DrawPanel->MoveCursorToCrossHair();
        break;

    case ID_POPUP_PCB_MOVE_EDGE:
    {
        Start_Move_EdgeMod( (EDGE_MODULE*) GetScreen()->GetCurItem(), &dc );
        DrawPanel->MoveCursorToCrossHair();
    }
    break;

    case ID_POPUP_PCB_STOP_CURRENT_DRAWING:
        DrawPanel->MoveCursorToCrossHair();

        if( (GetScreen()->GetCurItem()->m_Flags & IS_NEW) )
        {
            End_Edge_Module( (EDGE_MODULE*) GetScreen()->GetCurItem() );
            SetCurItem( NULL );
        }

        break;

    case ID_POPUP_PCB_ENTER_EDGE_WIDTH:
    {
        EDGE_MODULE* edge = NULL;
        if( GetScreen()->GetCurItem()
           && ( GetScreen()->GetCurItem()->Type() == TYPE_EDGE_MODULE ) )
        {
            edge = (EDGE_MODULE*) GetScreen()->GetCurItem();
        }

        Enter_Edge_Width( edge );
        DrawPanel->MoveCursorToCrossHair();

        if( edge )
            DrawPanel->Refresh();
    }
    break;

    case ID_POPUP_PCB_EDIT_WIDTH_CURRENT_EDGE:
        DrawPanel->MoveCursorToCrossHair();
        Edit_Edge_Width( (EDGE_MODULE*) GetScreen()->GetCurItem() );
        DrawPanel->Refresh();
        break;

    case ID_POPUP_PCB_EDIT_WIDTH_ALL_EDGE:
        DrawPanel->MoveCursorToCrossHair();
        Edit_Edge_Width( NULL );
        DrawPanel->Refresh();
        break;

    case ID_POPUP_PCB_EDIT_LAYER_CURRENT_EDGE:
        DrawPanel->MoveCursorToCrossHair();
        Edit_Edge_Layer( (EDGE_MODULE*) GetScreen()->GetCurItem() );
        DrawPanel->Refresh();
        break;

    case ID_POPUP_PCB_EDIT_LAYER_ALL_EDGE:
        DrawPanel->MoveCursorToCrossHair();
        Edit_Edge_Layer( NULL );
        DrawPanel->Refresh();
        break;

    case ID_POPUP_PCB_DELETE_EDGE:
        SaveCopyInUndoList( GetBoard()->m_Modules, UR_MODEDIT );
        DrawPanel->MoveCursorToCrossHair();
        RemoveStruct( GetScreen()->GetCurItem() );
        SetCurItem( NULL );
        break;

    case ID_MODEDIT_MODULE_ROTATE:
    case ID_MODEDIT_MODULE_MIRROR:
        SaveCopyInUndoList( GetBoard()->m_Modules, UR_MODEDIT );
        Transform( (MODULE*) GetScreen()->GetCurItem(), id );
        redraw = true;
        break;

    case ID_PCB_DRAWINGS_WIDTHS_SETUP:
        InstallOptionsFrame( pos );
        break;

    case ID_PCB_PAD_SETUP:
    {
        BOARD_ITEM* item = GetCurItem();

        if( item )
        {
            if( item->Type() != TYPE_PAD )
                item = NULL;
        }

        InstallPadOptionsFrame( (D_PAD*) item );
    }
    break;

    case ID_PCB_USER_GRID_SETUP:
        InstallGridFrame( pos );
        break;

    case ID_POPUP_PLACE_BLOCK:
        GetScreen()->m_BlockLocate.m_Command = BLOCK_MOVE;
        DrawPanel->m_AutoPAN_Request = false;
        HandleBlockPlace( &dc );
        break;

    case ID_POPUP_COPY_BLOCK:
        GetScreen()->m_BlockLocate.m_Command = BLOCK_COPY;
        GetScreen()->m_BlockLocate.SetMessageBlock( this );
        DrawPanel->m_AutoPAN_Request = false;
        HandleBlockPlace( &dc );
        break;

    case ID_POPUP_ZOOM_BLOCK:
        GetScreen()->m_BlockLocate.m_Command = BLOCK_ZOOM;
        GetScreen()->m_BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( &dc );
        break;

    case ID_POPUP_DELETE_BLOCK:
        GetScreen()->m_BlockLocate.m_Command = BLOCK_DELETE;
        GetScreen()->m_BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( &dc );
        break;

    case ID_POPUP_ROTATE_BLOCK:
        GetScreen()->m_BlockLocate.m_Command = BLOCK_ROTATE;
        GetScreen()->m_BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( &dc );
        break;

    case ID_POPUP_MIRROR_X_BLOCK:
        GetScreen()->m_BlockLocate.m_Command = BLOCK_MIRROR_X;
        GetScreen()->m_BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( &dc );
        break;

    default:
        DisplayError( this,
                      wxT( "FOOTPRINT_EDIT_FRAME::Process_Special_Functions error" ) );
        break;
    }

    if( redraw )
        DrawPanel->Refresh();
}


void FOOTPRINT_EDIT_FRAME::Transform( MODULE* module, int transform )
{
    D_PAD*        pad = module->m_Pads;
    EDA_ITEM*     PtStruct = module->m_Drawings;
    TEXTE_MODULE* textmod;
    EDGE_MODULE*  edgemod;
    int           angle = 900; // Necessary +- 900 (+- 90 degrees) )

    switch( transform )
    {
    case ID_MODEDIT_MODULE_ROTATE:
        module->SetOrientation( angle );

        for( ; pad != NULL; pad = (D_PAD*) pad->Next() )
        {
            pad->m_Pos0    = pad->m_Pos;
            pad->m_Orient -= angle;
            RotatePoint( &pad->m_Offset.x, &pad->m_Offset.y, angle );
            EXCHG( pad->m_Size.x, pad->m_Size.y );
            RotatePoint( &pad->m_DeltaSize.x, &pad->m_DeltaSize.y, -angle );
        }

        module->m_Reference->m_Pos0    = module->m_Reference->m_Pos;
        module->m_Reference->m_Orient += angle;

        if( module->m_Reference->m_Orient >= 1800 )
            module->m_Reference->m_Orient -= 1800;

        module->m_Value->m_Pos0    = module->m_Value->m_Pos;
        module->m_Value->m_Orient += angle;

        if( module->m_Value->m_Orient >= 1800 )
            module->m_Value->m_Orient -= 1800;

        for( ; PtStruct != NULL; PtStruct = PtStruct->Next() )
        {
            if( PtStruct->Type() == TYPE_EDGE_MODULE )
            {
                edgemod = (EDGE_MODULE*) PtStruct;
                edgemod->m_Start0 = edgemod->m_Start;
                edgemod->m_End0   = edgemod->m_End;
            }

            if( PtStruct->Type() == TYPE_TEXTE_MODULE )
            {
                textmod = (TEXTE_MODULE*) PtStruct;
                textmod->m_Pos0 = textmod->m_Pos;
            }
        }

        module->m_Orient = 0;
        break;

    case ID_MODEDIT_MODULE_MIRROR:
        for( ; pad != NULL; pad = (D_PAD*) pad->Next() )
        {
            NEGATE( pad->m_Pos.y );
            NEGATE( pad->m_Pos0.y );
            NEGATE( pad->m_Offset.y );
            NEGATE( pad->m_DeltaSize.y );

            if( pad->m_Orient )
                pad->m_Orient = 3600 - pad->m_Orient;
        }

        /* Reverse mirror of reference. */
        textmod = module->m_Reference;
        NEGATE( textmod->m_Pos.y );
        NEGATE( textmod->m_Pos0.y );

        if( textmod->m_Orient )
            textmod->m_Orient = 3600 - textmod->m_Orient;

        /* Reverse mirror of value. */
        textmod = module->m_Value;
        NEGATE( textmod->m_Pos.y );
        NEGATE( textmod->m_Pos0.y );

        if( textmod->m_Orient )
            textmod->m_Orient = 3600 - textmod->m_Orient;

        /* Reverse mirror of footprints. */
        PtStruct = module->m_Drawings;

        for( ; PtStruct != NULL; PtStruct = PtStruct->Next() )
        {
            switch( PtStruct->Type() )
            {
            case TYPE_EDGE_MODULE:
                edgemod = (EDGE_MODULE*) PtStruct;
                NEGATE( edgemod->m_Start.y );
                NEGATE( edgemod->m_End.y );
                /* Invert local coordinates */
                NEGATE( edgemod->m_Start0.y );
                NEGATE( edgemod->m_End0.y );
                NEGATE( edgemod->m_Angle );
                break;

            case TYPE_TEXTE_MODULE:
                /* Reverse mirror position and mirror. */
                textmod = (TEXTE_MODULE*) PtStruct;
                NEGATE( textmod->m_Pos.y );
                NEGATE( textmod->m_Pos0.y );
                if( textmod->m_Orient )
                    textmod->m_Orient = 3600 - textmod->m_Orient;
                break;

            default:
                DisplayError( this, wxT( "Draw type undefined" ) );
                break;
            }
        }

        break;

    default:
        DisplayInfoMessage( this, wxT( "Not available" ) );
        break;
    }

    module->Set_Rectangle_Encadrement();
    OnModify();
}


void FOOTPRINT_EDIT_FRAME::OnVerticalToolbar( wxCommandEvent& aEvent )
{
    int id = aEvent.GetId();

    SetToolID( ID_NO_TOOL_SELECTED, DrawPanel->GetDefaultCursor(), wxEmptyString );

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
            SetToolID( ID_NO_TOOL_SELECTED, DrawPanel->GetDefaultCursor(), wxEmptyString );
        }
        break;

    case ID_MODEDIT_DELETE_TOOL:
        SetToolID( id, wxCURSOR_BULLSEYE, _( "Delete item" ) );
        break;

    default:
        wxFAIL_MSG( wxT( "Unknown command id." ) );
        SetToolID( ID_NO_TOOL_SELECTED, DrawPanel->GetDefaultCursor(), wxEmptyString );
    }
}
