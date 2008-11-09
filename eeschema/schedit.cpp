/******************************************************/
/* schedit.cpp: fonctions generales de la schematique */
/******************************************************/

#include "fctsys.h"

#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "id.h"

#include "protos.h"


/*****************************************************************************
*
* Traite les selections d'outils et les commandes appelees du menu POPUP
*
*****************************************************************************/
void WinEDA_SchematicFrame::Process_Special_Functions( wxCommandEvent& event )
{
    int         id = event.GetId();
    wxPoint     pos;
    wxClientDC  dc( DrawPanel );
    SCH_SCREEN* screen = GetScreen();

    DrawPanel->PrepareGraphicContext( &dc );

    pos = wxGetMousePosition();

    pos.y += 20;

    // If needed, stop the current command and deselect current tool
    switch( id )
    {
    case ID_POPUP_SCH_ENTRY_SELECT_SLASH:
    case ID_POPUP_SCH_ENTRY_SELECT_ANTISLASH:
    case ID_POPUP_END_LINE:
    case ID_POPUP_SCH_EDIT_TEXT:
    case ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_LABEL:
    case ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_GLABEL:
    case ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_COMMENT:
    case ID_POPUP_SCH_SET_SHAPE_TEXT:
    case ID_POPUP_SCH_ROTATE_TEXT:
    case ID_POPUP_SCH_EDIT_SHEET:
    case ID_POPUP_SCH_CLEANUP_SHEET:
    case ID_POPUP_SCH_END_SHEET:
    case ID_POPUP_SCH_RESIZE_SHEET:
    case ID_POPUP_SCH_EDIT_PINSHEET:
    case ID_POPUP_SCH_MOVE_PINSHEET:
    case ID_POPUP_SCH_MOVE_ITEM_REQUEST:
    case ID_POPUP_SCH_MOVE_CMP_REQUEST:
    case ID_POPUP_SCH_DRAG_CMP_REQUEST:
    case ID_POPUP_SCH_EDIT_CMP:
    case ID_POPUP_SCH_MIROR_X_CMP:
    case ID_POPUP_SCH_MIROR_Y_CMP:
    case ID_POPUP_SCH_ROTATE_CMP_CLOCKWISE:
    case ID_POPUP_SCH_ROTATE_CMP_COUNTERCLOCKWISE:
    case ID_POPUP_SCH_ORIENT_NORMAL_CMP:
    case ID_POPUP_SCH_INIT_CMP:
    case ID_POPUP_SCH_DISPLAYDOC_CMP:
    case ID_POPUP_SCH_EDIT_VALUE_CMP:
    case ID_POPUP_SCH_EDIT_REF_CMP:
    case ID_POPUP_SCH_EDIT_FOOTPRINT_CMP:
    case ID_POPUP_SCH_EDIT_CONVERT_CMP:
    case ID_POPUP_SCH_SELECT_UNIT_CMP:
    case ID_POPUP_SCH_SELECT_UNIT1:
    case ID_POPUP_SCH_SELECT_UNIT2:
    case ID_POPUP_SCH_SELECT_UNIT3:
    case ID_POPUP_SCH_SELECT_UNIT4:
    case ID_POPUP_SCH_SELECT_UNIT5:
    case ID_POPUP_SCH_SELECT_UNIT6:
    case ID_POPUP_SCH_SELECT_UNIT7:
    case ID_POPUP_SCH_SELECT_UNIT8:
    case ID_POPUP_SCH_SELECT_UNIT9:
    case ID_POPUP_SCH_SELECT_UNIT10:
    case ID_POPUP_SCH_SELECT_UNIT11:
    case ID_POPUP_SCH_SELECT_UNIT12:
    case ID_POPUP_SCH_SELECT_UNIT13:
    case ID_POPUP_SCH_SELECT_UNIT14:
    case ID_POPUP_SCH_SELECT_UNIT15:
    case ID_POPUP_SCH_SELECT_UNIT16:
    case ID_POPUP_SCH_SELECT_UNIT17:
    case ID_POPUP_SCH_SELECT_UNIT18:
    case ID_POPUP_SCH_SELECT_UNIT19:
    case ID_POPUP_SCH_SELECT_UNIT20:
    case ID_POPUP_SCH_SELECT_UNIT21:
    case ID_POPUP_SCH_SELECT_UNIT22:
    case ID_POPUP_SCH_SELECT_UNIT23:
    case ID_POPUP_SCH_SELECT_UNIT24:
    case ID_POPUP_SCH_SELECT_UNIT25:
    case ID_POPUP_SCH_SELECT_UNIT26:
    case ID_POPUP_SCH_ROTATE_FIELD:
    case ID_POPUP_SCH_EDIT_FIELD:
    case ID_POPUP_DELETE_BLOCK:
    case ID_POPUP_PLACE_BLOCK:
    case ID_POPUP_ZOOM_BLOCK:
    case ID_POPUP_DRAG_BLOCK:
    case ID_POPUP_COPY_BLOCK:
    case ID_POPUP_ROTATE_BLOCK:
    case ID_POPUP_MIRROR_X_BLOCK:
    case ID_POPUP_MIRROR_Y_BLOCK:
    case ID_POPUP_SCH_DELETE_NODE:
    case ID_POPUP_SCH_DELETE_CONNECTION:
    case wxID_CUT:
    case wxID_COPY:
    case ID_POPUP_SCH_ENTER_SHEET:
    case ID_POPUP_SCH_LEAVE_SHEET:
    case ID_POPUP_SCH_ADD_JUNCTION:
    case ID_POPUP_SCH_ADD_LABEL:

        /* At this point: Do nothing. these commands do not need to stop the current command
          * (mainly a block command) or reset the current state
          * They will be executed later, in next switch structure.
         */
        break;

    case ID_POPUP_CANCEL_CURRENT_COMMAND:
        if( screen->BlockLocate.m_Command != BLOCK_IDLE )
            DrawPanel->SetCursor( wxCursor( DrawPanel->m_PanelCursor =
                                                DrawPanel->
                                                m_PanelDefaultCursor ) );

        if( DrawPanel->ManageCurseur && DrawPanel->ForceCloseManageCurseur )
        {
            DrawPanel->ForceCloseManageCurseur( DrawPanel, &dc );
        }
        /* ne devrait pas etre execute, sauf bug: */
        if( screen->BlockLocate.m_Command != BLOCK_IDLE )
        {
            screen->BlockLocate.m_Command = BLOCK_IDLE;
            screen->BlockLocate.m_State   = STATE_NO_BLOCK;
            screen->BlockLocate.m_BlockDrawStruct = NULL;
        }
        break;

    case ID_POPUP_SCH_DELETE_CMP:
    case ID_POPUP_SCH_DELETE:       // Stop the, current command, keep the current tool
        if( DrawPanel->ManageCurseur && DrawPanel->ForceCloseManageCurseur )
        {
            DrawPanel->ForceCloseManageCurseur( DrawPanel, &dc );
        }
        break;

    default:        // Stop the current command, and deselect the current tool
        if( DrawPanel->ManageCurseur && DrawPanel->ForceCloseManageCurseur )
        {
            DrawPanel->ForceCloseManageCurseur( DrawPanel, &dc );
        }
        DrawPanel->m_PanelCursor = DrawPanel->m_PanelDefaultCursor =
                                       wxCURSOR_ARROW;
        SetToolID( 0, DrawPanel->m_PanelCursor, wxEmptyString );
        break;
    }

    // End switch commande en cours

    switch( id )    // Command execution:
    {
    case ID_HIERARCHY:
        InstallHierarchyFrame( &dc, pos );
        g_ItemToRepeat = NULL;
        break;

    case wxID_CUT:
        if( screen->BlockLocate.m_Command != BLOCK_MOVE )
            break;
        HandleBlockEndByPopUp( BLOCK_DELETE, &dc );
        g_ItemToRepeat = NULL;
        break;

    case wxID_PASTE:
        HandleBlockBegin( &dc, BLOCK_PASTE, screen->m_Curseur );
        break;

    case ID_HIERARCHY_PUSH_POP_BUTT:
        SetToolID( id, wxCURSOR_HAND, _( "Push/Pop Hierarchy" ) );
        break;

    case ID_NOCONN_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add NoConnect Flag" ) );
        break;

    case ID_WIRE_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add Wire" ) );
        break;

    case ID_BUS_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add Bus" ) );
        break;

    case ID_LINE_COMMENT_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add Drawing" ) );
        break;

    case ID_JUNCTION_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add Junction" ) );
        break;

    case ID_LABEL_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add Label" ) );
        break;

    case ID_GLABEL_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add Global label" ) );
        break;

    case ID_HIERLABEL_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add Hierarchal label" ) );
        break;

    case ID_TEXT_COMMENT_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add Text" ) );
        break;

    case ID_WIRETOBUS_ENTRY_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add Wire to Bus entry" ) );
        break;

    case ID_BUSTOBUS_ENTRY_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add Bus to Bus entry" ) );
        break;

    case ID_SHEET_SYMBOL_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add Sheet" ) );
        break;

    case ID_SHEET_LABEL_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add PinSheet" ) );
        break;

    case ID_IMPORT_GLABEL_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Import PinSheet" ) );
        break;

    case ID_COMPONENT_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add Component" ) );
        break;

    case ID_PLACE_POWER_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add Power" ) );
        break;

    case ID_POPUP_SCH_ENTRY_SELECT_SLASH:
        DrawPanel->MouseToCursorSchema();
        SetBusEntryShape( &dc,
                          (DrawBusEntryStruct*) screen->GetCurItem(), '/' );
        break;

    case ID_POPUP_SCH_ENTRY_SELECT_ANTISLASH:
        DrawPanel->MouseToCursorSchema();
        SetBusEntryShape( &dc,
                          (DrawBusEntryStruct*) screen->GetCurItem(), '\\' );
        break;

    case ID_NO_SELECT_BUTT:
        SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
        break;

    case ID_POPUP_CANCEL_CURRENT_COMMAND:
        if( m_ID_current_state == 0 )
            SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
        break;

    case ID_POPUP_END_LINE:
        DrawPanel->MouseToCursorSchema();
        EndSegment( &dc );
        break;

    case ID_POPUP_SCH_EDIT_TEXT:
        EditSchematicText( (SCH_TEXT*) screen->GetCurItem(), &dc );
        break;

    case ID_POPUP_SCH_ROTATE_TEXT:
        DrawPanel->MouseToCursorSchema();
        ChangeTextOrient( (SCH_TEXT*) screen->GetCurItem(), &dc );
        break;

    case ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_LABEL:
        DrawPanel->MouseToCursorSchema();
        ConvertTextType( (SCH_TEXT*) screen->GetCurItem(),
                        &dc, TYPE_SCH_LABEL );
        break;

    case ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_GLABEL:
        DrawPanel->MouseToCursorSchema();
        ConvertTextType( (SCH_TEXT*) screen->GetCurItem(),
                        &dc, TYPE_SCH_GLOBALLABEL );
        break;

    case ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_HLABEL:
        DrawPanel->MouseToCursorSchema();
        ConvertTextType( (SCH_TEXT*) screen->GetCurItem(),
                        &dc, TYPE_SCH_HIERLABEL );
        break;

    case ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_COMMENT:
        DrawPanel->MouseToCursorSchema();
        ConvertTextType( (SCH_TEXT*) screen->GetCurItem(),
                        &dc, TYPE_SCH_TEXT );
        break;

    case ID_POPUP_SCH_SET_SHAPE_TEXT:

        // Not used
        break;

    case ID_POPUP_SCH_ROTATE_FIELD:
        DrawPanel->MouseToCursorSchema();
        RotateCmpField( (SCH_CMP_FIELD*) screen->GetCurItem(), &dc );
        break;

    case ID_POPUP_SCH_EDIT_FIELD:
        EditCmpFieldText( (SCH_CMP_FIELD*) screen->GetCurItem(), &dc );
        break;

    case ID_POPUP_SCH_DELETE_NODE:
    case ID_POPUP_SCH_DELETE_CONNECTION:
        DrawPanel->MouseToCursorSchema();
        DeleteConnection( &dc,
                          id == ID_POPUP_SCH_DELETE_CONNECTION ? TRUE : FALSE );
        screen->SetCurItem( NULL );
        g_ItemToRepeat = NULL;
        TestDanglingEnds( screen->EEDrawList, &dc );
        break;

    case ID_POPUP_SCH_BREAK_WIRE:
    {
        DrawPickedStruct* ListForUndo;
        DrawPanel->MouseToCursorSchema();
        ListForUndo = BreakSegment( screen, screen->m_Curseur, TRUE );
        if( ListForUndo )
            SaveCopyInUndoList( ListForUndo, IS_NEW | IS_CHANGED );
        TestDanglingEnds( screen->EEDrawList, &dc );
    }
        break;

    case ID_POPUP_SCH_DELETE_CMP:
        if( screen->GetCurItem() == NULL )
            break;

        // Ensure the struct is a component (could be a struct of a
        // component, like Field, text..)
        if( screen->GetCurItem()->Type() != TYPE_SCH_COMPONENT )
            screen->SetCurItem( LocateSmallestComponent( screen ) );

    case ID_POPUP_SCH_DELETE:
        {
            SCH_ITEM* item = screen->GetCurItem();
            if( item == NULL )
                break;

            DeleteStruct( DrawPanel, &dc, item );
            screen->SetCurItem( NULL );
            g_ItemToRepeat = NULL;
            TestDanglingEnds( screen->EEDrawList, &dc );
            screen->SetModify();
        }
        break;

    case ID_SCHEMATIC_DELETE_ITEM_BUTT:
        SetToolID( id, wxCURSOR_BULLSEYE, _( "Delete item" ) );
        break;

    case ID_POPUP_SCH_END_SHEET:
        DrawPanel->MouseToCursorSchema();
        screen->GetCurItem()->Place( this, &dc );
        break;

    case ID_POPUP_SCH_RESIZE_SHEET:
        DrawPanel->MouseToCursorSchema();
        ReSizeSheet( (DrawSheetStruct*) screen->GetCurItem(), &dc );
        TestDanglingEnds( screen->EEDrawList, &dc );
        break;

    case ID_POPUP_SCH_EDIT_SHEET:
        EditSheet( (DrawSheetStruct*) screen->GetCurItem(), &dc );
        break;

    case ID_POPUP_SCH_CLEANUP_SHEET:
        ( (DrawSheetStruct*)
         screen->GetCurItem() )->CleanupSheet( this, true );
        break;

    case ID_POPUP_SCH_EDIT_PINSHEET:
        Edit_PinSheet( (Hierarchical_PIN_Sheet_Struct*)
                      screen->GetCurItem(), &dc );
        break;

    case ID_POPUP_SCH_MOVE_PINSHEET:
        DrawPanel->MouseToCursorSchema();
        StartMove_PinSheet( (Hierarchical_PIN_Sheet_Struct*)
                           screen->GetCurItem(), &dc );
        break;

    case ID_POPUP_SCH_DRAG_CMP_REQUEST:
    case ID_POPUP_SCH_MOVE_CMP_REQUEST:

        // Ensure the struct is a component (could be a struct of a
        // component, like Field, text..)
        if( screen->GetCurItem()->Type() != TYPE_SCH_COMPONENT )
            screen->SetCurItem( LocateSmallestComponent( screen ) );
        if( screen->GetCurItem() == NULL )
            break;

    case ID_POPUP_SCH_MOVE_ITEM_REQUEST:
        DrawPanel->MouseToCursorSchema();
        if( id == ID_POPUP_SCH_DRAG_CMP_REQUEST )
        {
            // The easiest way to handle a drag component is simulate a
            // block drag command
            if( screen->BlockLocate.m_State == STATE_NO_BLOCK )
            {
                if( !HandleBlockBegin( &dc, BLOCK_DRAG,
                                       screen->m_Curseur ) )
                    break;
                HandleBlockEnd( &dc );
            }
        }
        else
            Process_Move_Item( (SCH_ITEM*) screen->GetCurItem(), &dc );
        break;

    case ID_POPUP_SCH_EDIT_CMP:

        // Ensure the struct is a component (could be a struct of a
        // component, like Field, text..)
        if( screen->GetCurItem()->Type() != TYPE_SCH_COMPONENT )
            screen->SetCurItem( LocateSmallestComponent( screen ) );
        if( screen->GetCurItem() == NULL )
            break;
        InstallCmpeditFrame( this, pos,
                            (SCH_COMPONENT*) screen->GetCurItem() );
        break;

    case ID_POPUP_SCH_MIROR_X_CMP:
    case ID_POPUP_SCH_MIROR_Y_CMP:
    case ID_POPUP_SCH_ROTATE_CMP_CLOCKWISE:
    case ID_POPUP_SCH_ROTATE_CMP_COUNTERCLOCKWISE:
    case ID_POPUP_SCH_ORIENT_NORMAL_CMP:

        // Ensure the struct is a component (could be a struct of a
        // component, like Field, text..)
        if( screen->GetCurItem()->Type() != TYPE_SCH_COMPONENT )
            screen->SetCurItem( LocateSmallestComponent( screen ) );
        if( screen->GetCurItem() == NULL )
            break;
        {
            int option;

            switch( id )
            {
            case ID_POPUP_SCH_MIROR_X_CMP:
                option = CMP_MIROIR_X; break;

            case ID_POPUP_SCH_MIROR_Y_CMP:
                option = CMP_MIROIR_Y; break;

            case ID_POPUP_SCH_ROTATE_CMP_COUNTERCLOCKWISE:
                option = CMP_ROTATE_COUNTERCLOCKWISE; break;

            case ID_POPUP_SCH_ROTATE_CMP_CLOCKWISE:
                option = CMP_ROTATE_CLOCKWISE; break;

            default:
            case ID_POPUP_SCH_ORIENT_NORMAL_CMP:
                option = CMP_NORMAL; break;
            }

            DrawPanel->MouseToCursorSchema();
            if( screen->GetCurItem()->m_Flags == 0 )
                SaveCopyInUndoList( (SCH_ITEM*) screen->GetCurItem(), IS_CHANGED );

            CmpRotationMiroir(
                (SCH_COMPONENT*) screen->GetCurItem(),
                &dc, option );
            break;
        }

    case ID_POPUP_SCH_INIT_CMP:
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_SCH_EDIT_VALUE_CMP:

        // Ensure the struct is a component (could be a struct of a
        // component, like Field, text..)
        if( screen->GetCurItem()->Type() != TYPE_SCH_COMPONENT )
            screen->SetCurItem( LocateSmallestComponent( screen ) );
        if( screen->GetCurItem() == NULL )
            break;

        EditComponentValue(
            (SCH_COMPONENT*) screen->GetCurItem(), &dc );
        break;

    case ID_POPUP_SCH_EDIT_REF_CMP:

        // Ensure the struct is a component (could be a struct of a
        // component, like Field, text..)
        if( screen->GetCurItem()->Type() != TYPE_SCH_COMPONENT )
            screen->SetCurItem( LocateSmallestComponent( screen ) );
        if( screen->GetCurItem() == NULL )
            break;

        EditComponentReference(
            (SCH_COMPONENT*) screen->GetCurItem(), &dc );
        break;

    case ID_POPUP_SCH_EDIT_FOOTPRINT_CMP:

        // Ensure the struct is a component (could be a struct of a
        // component, like Field, text..)
        if( screen->GetCurItem()->Type() != TYPE_SCH_COMPONENT )
            screen->SetCurItem( LocateSmallestComponent( screen ) );
        if( screen->GetCurItem() == NULL )
            break;
        EditComponentFootprint(
            (SCH_COMPONENT*) screen->GetCurItem(), &dc );
        break;


    case ID_POPUP_SCH_EDIT_CONVERT_CMP:

        // Ensure the struct is a component (could be a struct of a
        // component, like Field, text..)
        if( screen->GetCurItem()->Type() != TYPE_SCH_COMPONENT )
            screen->SetCurItem( LocateSmallestComponent( screen ) );
        if( screen->GetCurItem() == NULL )
            break;
        DrawPanel->MouseToCursorSchema();
        ConvertPart(
            (SCH_COMPONENT*) screen->GetCurItem(),
            &dc );
        break;

    case ID_POPUP_SCH_COPY_COMPONENT_CMP:
        DrawPanel->MouseToCursorSchema();
        {
            SCH_COMPONENT* olditem, * newitem;
            if( screen->GetCurItem()->Type() != TYPE_SCH_COMPONENT )
                screen->SetCurItem( LocateSmallestComponent( screen ) );
            olditem = (SCH_COMPONENT*) screen->GetCurItem();
            if( olditem == NULL )
                break;
            newitem = olditem->GenCopy();
            newitem->m_TimeStamp = GetTimeStamp();
            newitem->ClearAnnotation(NULL);
            newitem->m_Flags = IS_NEW;
            StartMovePart( newitem, &dc );

            /* Redraw the original part, because StartMovePart() has erase
             * it from screen */
            RedrawOneStruct( DrawPanel, &dc, olditem, GR_DEFAULT_DRAWMODE );
        }
        break;

    case ID_POPUP_SCH_SELECT_UNIT1:
    case ID_POPUP_SCH_SELECT_UNIT2:
    case ID_POPUP_SCH_SELECT_UNIT3:
    case ID_POPUP_SCH_SELECT_UNIT4:
    case ID_POPUP_SCH_SELECT_UNIT5:
    case ID_POPUP_SCH_SELECT_UNIT6:
    case ID_POPUP_SCH_SELECT_UNIT7:
    case ID_POPUP_SCH_SELECT_UNIT8:
    case ID_POPUP_SCH_SELECT_UNIT9:
    case ID_POPUP_SCH_SELECT_UNIT10:
    case ID_POPUP_SCH_SELECT_UNIT11:
    case ID_POPUP_SCH_SELECT_UNIT12:
    case ID_POPUP_SCH_SELECT_UNIT13:
    case ID_POPUP_SCH_SELECT_UNIT14:
    case ID_POPUP_SCH_SELECT_UNIT15:
    case ID_POPUP_SCH_SELECT_UNIT16:
    case ID_POPUP_SCH_SELECT_UNIT17:
    case ID_POPUP_SCH_SELECT_UNIT18:
    case ID_POPUP_SCH_SELECT_UNIT19:
    case ID_POPUP_SCH_SELECT_UNIT20:
    case ID_POPUP_SCH_SELECT_UNIT21:
    case ID_POPUP_SCH_SELECT_UNIT22:
    case ID_POPUP_SCH_SELECT_UNIT23:
    case ID_POPUP_SCH_SELECT_UNIT24:
    case ID_POPUP_SCH_SELECT_UNIT25:
    case ID_POPUP_SCH_SELECT_UNIT26:

        // Ensure the struct is a component (could be a struct of a
        // component, like Field, text..)
        if( screen->GetCurItem()->Type() != TYPE_SCH_COMPONENT )
            screen->SetCurItem( LocateSmallestComponent( screen ) );
        if( screen->GetCurItem() == NULL )
            break;
        DrawPanel->MouseToCursorSchema();
        SelPartUnit(
            (SCH_COMPONENT*) screen->GetCurItem(),
            id + 1 - ID_POPUP_SCH_SELECT_UNIT1,
            &dc );
        break;

    case ID_POPUP_SCH_DISPLAYDOC_CMP:

        // Ensure the struct is a component (could be a piece of a
        // component, like Field, text..)
        if( screen->GetCurItem()->Type() != TYPE_SCH_COMPONENT )
            screen->SetCurItem( LocateSmallestComponent( screen ) );
        if( screen->GetCurItem() == NULL )
            break;
        {
            EDA_LibComponentStruct* LibEntry;
            LibEntry = FindLibPart(
                ( (SCH_COMPONENT*) screen->GetCurItem() )->m_ChipName.GetData(),
                wxEmptyString,
                FIND_ALIAS );
            if( LibEntry && LibEntry->m_DocFile != wxEmptyString )
                GetAssociatedDocument( this,
                                       g_RealLibDirBuffer,
                                       LibEntry->m_DocFile );
        }
        break;

    case ID_POPUP_SCH_ENTER_SHEET:
    {
        EDA_BaseStruct* DrawStruct = screen->GetCurItem();
        if( DrawStruct && (DrawStruct->Type() == DRAW_SHEET_STRUCT_TYPE) )
        {
            InstallNextScreen( (DrawSheetStruct*) DrawStruct );
        }
    }
        break;

    case ID_POPUP_SCH_LEAVE_SHEET:
        InstallPreviousSheet();
        break;

    case ID_POPUP_CLOSE_CURRENT_TOOL:
        SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
        break;

    case wxID_COPY:         // really this is a Save block for paste
        HandleBlockEndByPopUp( BLOCK_SAVE, &dc );
        break;

    case ID_POPUP_PLACE_BLOCK:
        DrawPanel->m_AutoPAN_Request = FALSE;
        DrawPanel->MouseToCursorSchema();
        HandleBlockPlace( &dc );
        break;

    case ID_POPUP_ZOOM_BLOCK:
        HandleBlockEndByPopUp( BLOCK_ZOOM, &dc );
        break;

    case ID_POPUP_DELETE_BLOCK:
        DrawPanel->MouseToCursorSchema();
        HandleBlockEndByPopUp( BLOCK_DELETE, &dc );
        break;

    case ID_POPUP_ROTATE_BLOCK:
        DrawPanel->MouseToCursorSchema();
        HandleBlockEndByPopUp( BLOCK_ROTATE, &dc );
        break;

    case ID_POPUP_MIRROR_X_BLOCK:
    case ID_POPUP_MIRROR_Y_BLOCK:
        DrawPanel->MouseToCursorSchema();
        HandleBlockEndByPopUp( BLOCK_MIRROR_Y, &dc );
        break;

    case ID_POPUP_COPY_BLOCK:
        DrawPanel->MouseToCursorSchema();
        HandleBlockEndByPopUp( BLOCK_COPY, &dc );
        break;

    case ID_POPUP_DRAG_BLOCK:
        DrawPanel->MouseToCursorSchema();
        HandleBlockEndByPopUp( BLOCK_DRAG, &dc );
        break;

    case ID_POPUP_SCH_ADD_JUNCTION:
        DrawPanel->MouseToCursorSchema();
        screen->SetCurItem(
            CreateNewJunctionStruct( &dc, screen->m_Curseur, TRUE ) );
        TestDanglingEnds( screen->EEDrawList, &dc );
        screen->SetCurItem( NULL );
        break;

    case ID_POPUP_SCH_ADD_LABEL:
    case ID_POPUP_SCH_ADD_GLABEL:
        screen->SetCurItem(
            CreateNewText( &dc,
                           id == ID_POPUP_SCH_ADD_LABEL ?
                           LAYER_LOCLABEL : LAYER_GLOBLABEL ) );
        if( screen->GetCurItem() )
        {
            ((SCH_ITEM*)screen->GetCurItem())->Place( this, &dc );
            TestDanglingEnds( screen->EEDrawList, &dc );
            screen->SetCurItem( NULL );
        }
        break;

    case ID_SCHEMATIC_UNDO:
        if( GetSchematicFromUndoList() )
        {
            TestDanglingEnds( screen->EEDrawList, NULL );
            DrawPanel->Refresh( TRUE );
        }
        break;

    case ID_SCHEMATIC_REDO:
        if( GetSchematicFromRedoList() )
        {
            TestDanglingEnds( screen->EEDrawList, NULL );
            DrawPanel->Refresh( TRUE );
        }
        break;

    default:        // Log error:
        DisplayError( this,
                     wxT( "WinEDA_SchematicFrame::Process_Special_Functions error" ) );
        break;
    }

    // End switch ( id )    (Command execution)

    if( m_ID_current_state == 0 )
        g_ItemToRepeat = NULL;
    SetToolbars();
}


/*************************************************************************************/
void WinEDA_SchematicFrame::Process_Move_Item( SCH_ITEM* DrawStruct, wxDC*  DC )
/*************************************************************************************/
{
    if( DrawStruct == NULL )
        return;

    DrawPanel->MouseToCursorSchema();

    switch( DrawStruct->Type() )
    {
    case DRAW_JUNCTION_STRUCT_TYPE:
        break;

    case DRAW_BUSENTRY_STRUCT_TYPE:
        StartMoveBusEntry( (DrawBusEntryStruct*) DrawStruct, DC );
        break;

    case TYPE_SCH_LABEL:
    case TYPE_SCH_GLOBALLABEL:
    case TYPE_SCH_HIERLABEL:
    case TYPE_SCH_TEXT:
        StartMoveTexte( (SCH_TEXT*) DrawStruct, DC );
        break;

    case TYPE_SCH_COMPONENT:
        StartMovePart( (SCH_COMPONENT*) DrawStruct, DC );
        break;

    case DRAW_SEGMENT_STRUCT_TYPE:
        break;

    case DRAW_SHEET_STRUCT_TYPE:
        StartMoveSheet( (DrawSheetStruct*) DrawStruct, DC );
        break;

    case DRAW_NOCONNECT_STRUCT_TYPE:
        break;

    case DRAW_PART_TEXT_STRUCT_TYPE:
        StartMoveCmpField( (SCH_CMP_FIELD*) DrawStruct, DC );
        break;

    case DRAW_MARKER_STRUCT_TYPE:
    case DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE:
    default:
        wxString msg;
        msg.Printf(
            wxT( "WinEDA_SchematicFrame::Move_Item Error: Bad DrawType %d" ),
            DrawStruct->Type() );
        DisplayError( this, msg );
        break;
    }
}
