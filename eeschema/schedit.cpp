/*****************/
/*  schedit.cpp  */
/*****************/

#include "fctsys.h"
#include "gr_basic.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "eda_doc.h"
#include "wxEeschemaStruct.h"
#include "class_sch_screen.h"
#include "kicad_device_context.h"

#include "general.h"
#include "eeschema_id.h"
#include "protos.h"
#include "class_library.h"
#include "sch_bus_entry.h"
#include "sch_marker.h"
#include "sch_component.h"
#include "sch_junction.h"
#include "sch_line.h"
#include "sch_sheet.h"


void SCH_EDIT_FRAME::Process_Special_Functions( wxCommandEvent& event )
{
    int         id = event.GetId();
    wxPoint     pos;
    SCH_SCREEN* screen = GetScreen();

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
    case ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_HLABEL:
    case ID_POPUP_SCH_SET_SHAPE_TEXT:
    case ID_POPUP_SCH_ROTATE_TEXT:
    case ID_POPUP_SCH_EDIT_SHEET:
    case ID_POPUP_SCH_CLEANUP_SHEET:
    case ID_POPUP_SCH_END_SHEET:
    case ID_POPUP_SCH_RESIZE_SHEET:
    case ID_POPUP_IMPORT_GLABEL:
    case ID_POPUP_SCH_EDIT_PINSHEET:
    case ID_POPUP_SCH_MOVE_PINSHEET:
    case ID_POPUP_SCH_MOVE_ITEM_REQUEST:
    case ID_POPUP_SCH_MOVE_CMP_REQUEST:
    case ID_POPUP_SCH_DRAG_CMP_REQUEST:
    case ID_POPUP_SCH_DRAG_WIRE_REQUEST:
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
    case ID_POPUP_SCH_GETINFO_MARKER:

        /* At this point: Do nothing. these commands do not need to stop the
         * current command (mainly a block command) or reset the current state
         * They will be executed later, in next switch structure.
         */
        break;

    case ID_POPUP_CANCEL_CURRENT_COMMAND:
        if( screen->m_BlockLocate.m_Command != BLOCK_IDLE )
            DrawPanel->SetCursor( wxCursor( DrawPanel->m_PanelCursor =
                                            DrawPanel->m_PanelDefaultCursor ) );

        // Stop the current command (if any) but keep the current tool
        DrawPanel->UnManageCursor();

        /* Should not be executed, except bug. */
        if( screen->m_BlockLocate.m_Command != BLOCK_IDLE )
        {
            screen->m_BlockLocate.m_Command = BLOCK_IDLE;
            screen->m_BlockLocate.m_State   = STATE_NO_BLOCK;
            screen->m_BlockLocate.ClearItemsList();
        }
        break;

    case ID_POPUP_SCH_DELETE_CMP:
    case ID_POPUP_SCH_DELETE:

        // Stop the current command (if any) but keep the current tool
        DrawPanel->UnManageCursor();
        break;

    default:

        // Stop the current command and deselect the current tool
        DrawPanel->m_PanelCursor = DrawPanel->m_PanelDefaultCursor = wxCURSOR_ARROW;
        DrawPanel->UnManageCursor( 0, DrawPanel->m_PanelCursor );
        break;
    }

    INSTALL_DC( dc, DrawPanel );
    switch( id )
    {
    case ID_HIERARCHY:
        InstallHierarchyFrame( &dc, pos );
        m_itemToRepeat = NULL;
        break;

    case wxID_CUT:
        if( screen->m_BlockLocate.m_Command != BLOCK_MOVE )
            break;
        HandleBlockEndByPopUp( BLOCK_DELETE, &dc );
        m_itemToRepeat = NULL;
        SetSheetNumberAndCount();
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
        SetToolID( id, wxCURSOR_PENCIL, _( "Add Lines" ) );
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
        SetToolID( id, wxCURSOR_PENCIL, _( "Add Hierarchical label" ) );
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

    case ID_IMPORT_HLABEL_BUTT:
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
        SetBusEntryShape( &dc, (SCH_BUS_ENTRY*) screen->GetCurItem(), '/' );
        break;

    case ID_POPUP_SCH_ENTRY_SELECT_ANTISLASH:
        DrawPanel->MouseToCursorSchema();
        SetBusEntryShape( &dc, (SCH_BUS_ENTRY*) screen->GetCurItem(), '\\' );
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
        EditSchematicText( (SCH_TEXT*) screen->GetCurItem() );
        break;

    case ID_POPUP_SCH_ROTATE_TEXT:
        DrawPanel->MouseToCursorSchema();
        ChangeTextOrient( (SCH_TEXT*) screen->GetCurItem(), &dc );
        break;

    case ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_LABEL:
        DrawPanel->MouseToCursorSchema();
        ConvertTextType( (SCH_TEXT*) screen->GetCurItem(), &dc, SCH_LABEL_T );
        break;

    case ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_GLABEL:
        DrawPanel->MouseToCursorSchema();
        ConvertTextType( (SCH_TEXT*) screen->GetCurItem(), &dc, SCH_GLOBAL_LABEL_T );
        break;

    case ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_HLABEL:
        DrawPanel->MouseToCursorSchema();
        ConvertTextType( (SCH_TEXT*) screen->GetCurItem(), &dc, SCH_HIERARCHICAL_LABEL_T );
        break;

    case ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_COMMENT:
        DrawPanel->MouseToCursorSchema();
        ConvertTextType( (SCH_TEXT*) screen->GetCurItem(), &dc, SCH_TEXT_T );
        break;

    case ID_POPUP_SCH_SET_SHAPE_TEXT:

        // Not used
        break;

    case ID_POPUP_SCH_ROTATE_FIELD:
        DrawPanel->MouseToCursorSchema();
        RotateCmpField( (SCH_FIELD*) screen->GetCurItem(), &dc );
        break;

    case ID_POPUP_SCH_EDIT_FIELD:
        EditCmpFieldText( (SCH_FIELD*) screen->GetCurItem(), &dc );
        break;

    case ID_POPUP_SCH_DELETE_NODE:
    case ID_POPUP_SCH_DELETE_CONNECTION:
        DrawPanel->MouseToCursorSchema();
        DeleteConnection( id == ID_POPUP_SCH_DELETE_CONNECTION ? TRUE : FALSE );
        screen->SetCurItem( NULL );
        m_itemToRepeat = NULL;
        screen->TestDanglingEnds( DrawPanel, &dc );
        DrawPanel->Refresh();
        break;

    case ID_POPUP_SCH_BREAK_WIRE:
    {
        DrawPanel->MouseToCursorSchema();
        SCH_ITEM* oldWiresList = screen->ExtractWires( true );
        screen->BreakSegment( screen->m_Curseur );

        if( oldWiresList )
            SaveCopyInUndoList( oldWiresList, UR_WIRE_IMAGE );

        screen->TestDanglingEnds( DrawPanel, &dc );
    }
    break;

    case ID_POPUP_SCH_DELETE_CMP:
        if( screen->GetCurItem() == NULL )
            break;

        // Ensure the struct is a component (could be a struct of a
        // component, like Field, text..)
        if( screen->GetCurItem()->Type() != SCH_COMPONENT_T )
            screen->SetCurItem( LocateSmallestComponent( screen ) );

    case ID_POPUP_SCH_DELETE:
    {
        SCH_ITEM* item = screen->GetCurItem();

        if( item == NULL )
            break;

        DeleteStruct( DrawPanel, &dc, item );
        screen->SetCurItem( NULL );
        m_itemToRepeat = NULL;
        screen->TestDanglingEnds( DrawPanel, &dc );
        SetSheetNumberAndCount();
        OnModify();
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
        ReSizeSheet( (SCH_SHEET*) screen->GetCurItem(), &dc );
        screen->TestDanglingEnds( DrawPanel, &dc );
        break;

    case ID_POPUP_SCH_EDIT_SHEET:
        if( EditSheet( (SCH_SHEET*) screen->GetCurItem(), &dc ) )
            OnModify();
        break;

    case ID_POPUP_IMPORT_GLABEL:
        if( screen->GetCurItem() && screen->GetCurItem()->Type() == SCH_SHEET_T )
            screen->SetCurItem( Import_PinSheet( (SCH_SHEET*) screen->GetCurItem(), &dc ) );
        break;

    case ID_POPUP_SCH_CLEANUP_SHEET:
        if( screen->GetCurItem() && screen->GetCurItem()->Type() == SCH_SHEET_T )
        {
            SCH_SHEET* sheet = (SCH_SHEET*) screen->GetCurItem();

            if( !sheet->HasUndefinedLabels() )
            {
                DisplayInfoMessage( this,
                                    _( "There are no undefined labels in this sheet to clean up." ) );
                return;
            }

            if( !IsOK( this, _( "Do you wish to cleanup this sheet?" ) ) )
                return;

            /* Save sheet in undo list before cleaning up unreferenced hierarchical labels. */
            SaveCopyInUndoList( sheet, UR_CHANGED );
            sheet->CleanupSheet();
            OnModify();
            DrawPanel->PostDirtyRect( sheet->GetBoundingBox() );
        }
        break;

    case ID_POPUP_SCH_EDIT_PINSHEET:
        Edit_PinSheet( (SCH_SHEET_PIN*) screen->GetCurItem(), &dc );
        break;

    case ID_POPUP_SCH_MOVE_PINSHEET:
        DrawPanel->MouseToCursorSchema();
        StartMove_PinSheet( (SCH_SHEET_PIN*) screen->GetCurItem(), &dc );
        break;

    case ID_POPUP_SCH_DRAG_CMP_REQUEST:
    case ID_POPUP_SCH_MOVE_CMP_REQUEST:

        // Ensure the struct is a component (could be a struct of a
        // component, like Field, text..) or a hierachical sheet
        // or a label
        if( (screen->GetCurItem()->Type() != SCH_COMPONENT_T)
           && (screen->GetCurItem()->Type() != SCH_LABEL_T)
           && (screen->GetCurItem()->Type() != SCH_GLOBAL_LABEL_T)
           && (screen->GetCurItem()->Type() != SCH_HIERARCHICAL_LABEL_T)
           && (screen->GetCurItem()->Type() != SCH_SHEET_T) )
            screen->SetCurItem( LocateSmallestComponent( screen ) );

        if( screen->GetCurItem() == NULL )
            break;

    // fall through
    case ID_POPUP_SCH_MOVE_ITEM_REQUEST:
        DrawPanel->MouseToCursorSchema();

        if( id == ID_POPUP_SCH_DRAG_CMP_REQUEST )
        {
            // The easiest way to handle a drag component or sheet command
            // is to simulate a block drag command
            if( screen->m_BlockLocate.m_State == STATE_NO_BLOCK )
            {
                if( !HandleBlockBegin( &dc, BLOCK_DRAG, screen->m_Curseur ) )
                    break;

                // Give a non null size to the search block:
                screen->m_BlockLocate.Inflate( 1 );
                HandleBlockEnd( &dc );
            }
        }
        else
            Process_Move_Item( (SCH_ITEM*) screen->GetCurItem(), &dc );
        break;

    case ID_POPUP_SCH_DRAG_WIRE_REQUEST:
        DrawPanel->MouseToCursorSchema();
        // The easiest way to handle a drag component is to simulate a
        // block drag command
        if( screen->m_BlockLocate.m_State == STATE_NO_BLOCK )
        {
            if( !HandleBlockBegin( &dc, BLOCK_DRAG, screen->m_Curseur ) )
                break;

            // Ensure the block selection contains the segment, or one end of
            // the segment.  The initial rect is only one point (w = h = 2)
            // The rect contains one or 0 ends.
            // If one end is selected, this is a drag Node
            // if no ends selected the current segment is dragged
            screen->m_BlockLocate.Inflate( 1 );
            HandleBlockEnd( &dc );
        }
        break;

    case ID_POPUP_SCH_EDIT_CMP:

        // Ensure the struct is a component (could be a struct of a
        // component, like Field, text..)
        if( screen->GetCurItem()->Type() != SCH_COMPONENT_T )
            screen->SetCurItem( LocateSmallestComponent( screen ) );

        if( screen->GetCurItem() == NULL )
            break;

        InstallCmpeditFrame( this, pos, (SCH_COMPONENT*) screen->GetCurItem() );
        break;

    case ID_POPUP_SCH_MIROR_X_CMP:
    case ID_POPUP_SCH_MIROR_Y_CMP:
    case ID_POPUP_SCH_ROTATE_CMP_CLOCKWISE:
    case ID_POPUP_SCH_ROTATE_CMP_COUNTERCLOCKWISE:
    case ID_POPUP_SCH_ORIENT_NORMAL_CMP:

        // Ensure the struct is a component (could be a struct of a
        // component, like Field, text..)
        if( screen->GetCurItem()->Type() != SCH_COMPONENT_T )
            screen->SetCurItem( LocateSmallestComponent( screen ) );

        if( screen->GetCurItem() == NULL )
            break;
        {
            int option;

            switch( id )
            {
            case ID_POPUP_SCH_MIROR_X_CMP:
                option = CMP_MIRROR_X; break;

            case ID_POPUP_SCH_MIROR_Y_CMP:
                option = CMP_MIRROR_Y; break;

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
                SaveCopyInUndoList( (SCH_ITEM*) screen->GetCurItem(), UR_CHANGED );

            CmpRotationMiroir( (SCH_COMPONENT*) screen->GetCurItem(), &dc, option );
            break;
        }

    case ID_POPUP_SCH_INIT_CMP:
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_SCH_EDIT_VALUE_CMP:

        // Ensure the struct is a component (could be a struct of a
        // component, like Field, text..)
        if( screen->GetCurItem()->Type() != SCH_COMPONENT_T )
            screen->SetCurItem( LocateSmallestComponent( screen ) );

        if( screen->GetCurItem() == NULL )
            break;

        EditComponentValue( (SCH_COMPONENT*) screen->GetCurItem(), &dc );
        break;

    case ID_POPUP_SCH_EDIT_REF_CMP:

        // Ensure the struct is a component (could be a struct of a
        // component, like Field, text..)
        if( screen->GetCurItem()->Type() != SCH_COMPONENT_T )
            screen->SetCurItem( LocateSmallestComponent( screen ) );

        if( screen->GetCurItem() == NULL )
            break;

        EditComponentReference( (SCH_COMPONENT*) screen->GetCurItem(), &dc );
        break;

    case ID_POPUP_SCH_EDIT_FOOTPRINT_CMP:

        // Ensure the struct is a component (could be a struct of a
        // component, like Field, text..)
        if( screen->GetCurItem()->Type() != SCH_COMPONENT_T )
            screen->SetCurItem( LocateSmallestComponent( screen ) );

        if( screen->GetCurItem() == NULL )
            break;

        EditComponentFootprint( (SCH_COMPONENT*) screen->GetCurItem(), &dc );
        break;


    case ID_POPUP_SCH_EDIT_CONVERT_CMP:

        // Ensure the struct is a component (could be a struct of a
        // component, like Field, text..)
        if( screen->GetCurItem()->Type() != SCH_COMPONENT_T )
            screen->SetCurItem( LocateSmallestComponent( screen ) );

        if( screen->GetCurItem() == NULL )
            break;

        DrawPanel->MouseToCursorSchema();
        ConvertPart( (SCH_COMPONENT*) screen->GetCurItem(), &dc );
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
        if( screen->GetCurItem()->Type() != SCH_COMPONENT_T )
            screen->SetCurItem( LocateSmallestComponent( screen ) );

        if( screen->GetCurItem() == NULL )
            break;

        DrawPanel->MouseToCursorSchema();
        SelPartUnit( (SCH_COMPONENT*) screen->GetCurItem(),
                     id + 1 - ID_POPUP_SCH_SELECT_UNIT1, &dc );
        break;

    case ID_POPUP_SCH_DISPLAYDOC_CMP:

        // Ensure the struct is a component (could be a piece of a
        // component, like Field, text..)
        if( screen->GetCurItem()->Type() != SCH_COMPONENT_T )
            screen->SetCurItem( LocateSmallestComponent( screen ) );

        if( screen->GetCurItem() == NULL )
            break;
        {
            LIB_ALIAS* LibEntry;
            LibEntry = CMP_LIBRARY::FindLibraryEntry(
                ( (SCH_COMPONENT*) screen->GetCurItem() )->GetLibName() );

            if( LibEntry && LibEntry->GetDocFileName() != wxEmptyString )
            {
                GetAssociatedDocument( this, LibEntry->GetDocFileName(),
                                       &wxGetApp().GetLibraryPathList() );
            }
        }
        break;

    case ID_POPUP_SCH_ENTER_SHEET:
    {
        EDA_ITEM* DrawStruct = screen->GetCurItem();

        if( DrawStruct && (DrawStruct->Type() == SCH_SHEET_T) )
        {
            InstallNextScreen( (SCH_SHEET*) DrawStruct );
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
        SetSheetNumberAndCount();
        break;

    case ID_POPUP_ROTATE_BLOCK:
        DrawPanel->MouseToCursorSchema();
        HandleBlockEndByPopUp( BLOCK_ROTATE, &dc );
        break;

    case ID_POPUP_MIRROR_X_BLOCK:
        DrawPanel->MouseToCursorSchema();
        HandleBlockEndByPopUp( BLOCK_MIRROR_X, &dc );
        break;

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
        screen->SetCurItem( CreateNewJunctionStruct( &dc, screen->m_Curseur, true ) );
        screen->TestDanglingEnds( DrawPanel, &dc );
        screen->SetCurItem( NULL );
        break;

    case ID_POPUP_SCH_ADD_LABEL:
    case ID_POPUP_SCH_ADD_GLABEL:
        screen->SetCurItem( CreateNewText( &dc, id == ID_POPUP_SCH_ADD_LABEL ?
                                           LAYER_LOCLABEL : LAYER_GLOBLABEL ) );
        if( screen->GetCurItem() )
        {
            ( (SCH_ITEM*) screen->GetCurItem() )->Place( this, &dc );
            screen->TestDanglingEnds( DrawPanel, &dc );
            screen->SetCurItem( NULL );
        }
        break;

    case ID_POPUP_SCH_GETINFO_MARKER:
        if( screen->GetCurItem() && screen->GetCurItem()->Type() == SCH_MARKER_T )
            ( (SCH_MARKER*) screen->GetCurItem() )->DisplayMarkerInfo( this );

        break;

    default:        // Log error:
        DisplayError( this, wxT( "SCH_EDIT_FRAME::Process_Special_Functions error" ) );
        break;
    }

    // End switch ( id )    (Command execution)

    if( m_ID_current_state == 0 )
        m_itemToRepeat = NULL;

    SetToolbars();
}


void SCH_EDIT_FRAME::Process_Move_Item( SCH_ITEM* DrawStruct, wxDC* DC )
{
    if( DrawStruct == NULL )
        return;

    DrawPanel->MouseToCursorSchema();

    switch( DrawStruct->Type() )
    {
    case SCH_JUNCTION_T:
        break;

    case SCH_BUS_ENTRY_T:
        StartMoveBusEntry( (SCH_BUS_ENTRY*) DrawStruct, DC );
        break;

    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIERARCHICAL_LABEL_T:
    case SCH_TEXT_T:
        StartMoveTexte( (SCH_TEXT*) DrawStruct, DC );
        break;

    case SCH_COMPONENT_T:
        StartMovePart( (SCH_COMPONENT*) DrawStruct, DC );
        break;

    case SCH_LINE_T:
        break;

    case SCH_SHEET_T:
        StartMoveSheet( (SCH_SHEET*) DrawStruct, DC );
        break;

    case SCH_NO_CONNECT_T:
        break;

    case SCH_FIELD_T:
        StartMoveCmpField( (SCH_FIELD*) DrawStruct, DC );
        break;

    case SCH_MARKER_T:
    case SCH_SHEET_LABEL_T:
    default:
        wxString msg;
        msg.Printf( wxT( "SCH_EDIT_FRAME::Move_Item Error: Bad DrawType %d" ),
                    DrawStruct->Type() );
        DisplayError( this, msg );
        break;
    }
}
