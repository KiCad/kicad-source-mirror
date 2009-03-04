/****************************/
/*	EESchema - libframe.cpp	*/
/****************************/

/* Gestion de la frame d'edition des composants en librairie
 */

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "eda_doc.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "bitmaps.h"
#include "protos.h"
#include "id.h"

/*****************************/
/* class WinEDA_LibeditFrame */
/*****************************/
BEGIN_EVENT_TABLE( WinEDA_LibeditFrame, WinEDA_DrawFrame )
    EVT_CLOSE( WinEDA_LibeditFrame::OnCloseWindow )
    EVT_SIZE( WinEDA_LibeditFrame::OnSize )

    EVT_TOOL_RANGE( ID_ZOOM_IN, ID_ZOOM_PAGE, WinEDA_LibeditFrame::OnZoom )

// Tools et boutons de Libedit:

/* Main horizontal toolbar */
    EVT_TOOL_RANGE( ID_LIBEDIT_START_H_TOOL, ID_LIBEDIT_END_H_TOOL,
                    WinEDA_LibeditFrame::Process_Special_Functions )
    EVT_KICAD_CHOICEBOX( ID_LIBEDIT_SELECT_PART_NUMBER,
                         WinEDA_LibeditFrame::Process_Special_Functions )
    EVT_KICAD_CHOICEBOX( ID_LIBEDIT_SELECT_ALIAS,
                         WinEDA_LibeditFrame::Process_Special_Functions )

/* Right Vertical toolbar */
    EVT_TOOL( ID_NO_SELECT_BUTT, WinEDA_LibeditFrame::Process_Special_Functions )
    EVT_TOOL_RANGE( ID_LIBEDIT_START_V_TOOL, ID_LIBEDIT_END_V_TOOL,
                    WinEDA_LibeditFrame::Process_Special_Functions )

/* PopUp events and commands: */
    EVT_MENU_RANGE( ID_POPUP_START_RANGE, ID_POPUP_END_RANGE,
                    WinEDA_LibeditFrame::Process_Special_Functions )

// Annulation de commande en cours
    EVT_MENU_RANGE( ID_POPUP_GENERAL_START_RANGE, ID_POPUP_GENERAL_END_RANGE,
                    WinEDA_LibeditFrame::Process_Special_Functions )

// PopUp Menus pour Zooms trait�s dans drawpanel.cpp
END_EVENT_TABLE()


WinEDA_LibeditFrame::WinEDA_LibeditFrame( wxWindow*       father,
                                          const wxString& title,
                                          const wxPoint&  pos,
                                          const wxSize&   size,
                                          long style ) :
    WinEDA_DrawFrame( father, LIBEDITOR_FRAME, title, pos, size, style )
{
    m_FrameName = wxT( "LibeditFrame" );
    m_Draw_Axis = true;             // true pour avoir les axes dessines
    m_Draw_Grid = true;             // true pour avoir la axes dessinee

    // Give an icon
    SetIcon( wxIcon( libedit_xpm ) );
    SetBaseScreen( g_ScreenLib );
    GetScreen()->m_Center = true;       // set to true to have the coordinates origine -0,0) centered on screen
    GetSettings();
    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );
    if( DrawPanel )
        DrawPanel->m_Block_Enable = true;
    ReCreateHToolbar();
    ReCreateVToolbar();
    DisplayLibInfos();
    BestZoom();
    Show( true );
}


/**********************************************/
WinEDA_LibeditFrame::~WinEDA_LibeditFrame()
/**********************************************/
{
    WinEDA_SchematicFrame* frame =
        (WinEDA_SchematicFrame*) wxGetApp().GetTopWindow();
    frame->m_LibeditFrame = NULL;
}


/***********************************************************/
void WinEDA_LibeditFrame::OnCloseWindow( wxCloseEvent& Event )
/***********************************************************/
{
    LibraryStruct* Lib;

    if( GetScreen()->IsModify() )
    {
        if( !IsOK( this, _( "Component was modified!\nDiscard changes?" ) ) )
        {
            Event.Veto(); return;
        }
        else
            GetScreen()->ClrModify();
    }

    for( Lib = g_LibraryList; Lib != NULL; Lib = Lib->m_Pnext )
    {
        if( Lib->m_Modified )
        {
            wxString msg;
            msg.Printf( _( "Library \"%s\" was modified!\nDiscard changes?" ), Lib->m_Name.GetData() );
            if( !IsOK( this, msg ) )
            {
                Event.Veto();
                return;
            }
        }
    }

    SaveSettings();
    Destroy();
}


/******************************************/
void WinEDA_LibeditFrame::SetToolbars()
/******************************************/

/* Enable or disable tools of the differents toolbars,
 *  according to the current conditions or options
 */
{
    if( m_HToolBar == NULL )
        return;

    if( CurrentLib == NULL )
    {
        if( m_HToolBar )
            m_HToolBar->EnableTool( ID_LIBEDIT_SAVE_CURRENT_LIB, false );
    }
    else
    {
        if( m_HToolBar )
            m_HToolBar->EnableTool( ID_LIBEDIT_SAVE_CURRENT_LIB, true );
    }

    if( CurrentLibEntry == NULL )
    {
        if( m_HToolBar )
        {
            m_HToolBar->EnableTool( ID_LIBEDIT_IMPORT_PART, true );
            m_HToolBar->EnableTool( ID_LIBEDIT_EXPORT_PART, false );
            m_HToolBar->EnableTool( ID_LIBEDIT_CREATE_NEW_LIB_AND_SAVE_CURRENT_PART, false );
            m_HToolBar->EnableTool( ID_LIBEDIT_SAVE_CURRENT_PART, false );
            m_HToolBar->EnableTool( ID_DE_MORGAN_CONVERT_BUTT, false );
            m_HToolBar->EnableTool( ID_DE_MORGAN_NORMAL_BUTT, false );
            m_HToolBar->EnableTool( ID_LIBEDIT_EDIT_PIN_BY_PIN, false );
            m_HToolBar->EnableTool( ID_LIBEDIT_VIEW_DOC, false );
            m_HToolBar->EnableTool( ID_LIBEDIT_CHECK_PART, false );
            m_HToolBar->EnableTool( ID_LIBEDIT_GET_FRAME_EDIT_FIELDS, false );
            m_SelpartBox->Enable( false );
            m_HToolBar->EnableTool( ID_LIBEDIT_UNDO, false );
            m_HToolBar->EnableTool( ID_LIBEDIT_REDO, false );
        }
        g_EditPinByPinIsOn = false;
        m_HToolBar->ToggleTool( ID_LIBEDIT_EDIT_PIN_BY_PIN, g_EditPinByPinIsOn );

        if( m_VToolBar )
        {
            m_VToolBar->EnableTool( ID_LIBEDIT_BODY_TEXT_BUTT, false );
            m_VToolBar->EnableTool( ID_LIBEDIT_BODY_LINE_BUTT, false );
            m_VToolBar->EnableTool( ID_LIBEDIT_BODY_RECT_BUTT, false );
            m_VToolBar->EnableTool( ID_LIBEDIT_BODY_CIRCLE_BUTT, false );
            m_VToolBar->EnableTool( ID_LIBEDIT_BODY_ARC_BUTT, false );
            m_VToolBar->EnableTool( ID_LIBEDIT_DELETE_ITEM_BUTT, false );
            m_VToolBar->EnableTool( ID_LIBEDIT_ANCHOR_ITEM_BUTT, false );
            m_VToolBar->EnableTool( ID_LIBEDIT_IMPORT_BODY_BUTT, false );
            m_VToolBar->EnableTool( ID_LIBEDIT_EXPORT_BODY_BUTT, false );
        }
    }
    else    // if we have a current entry to edit:
    {
        if( m_HToolBar )
        {
            m_HToolBar->EnableTool( ID_LIBEDIT_IMPORT_PART, true );
            m_HToolBar->EnableTool( ID_LIBEDIT_EXPORT_PART, true );
            m_HToolBar->EnableTool( ID_LIBEDIT_CREATE_NEW_LIB_AND_SAVE_CURRENT_PART, true );
            m_HToolBar->EnableTool( ID_LIBEDIT_SAVE_CURRENT_PART, true );
            m_HToolBar->EnableTool( ID_LIBEDIT_GET_FRAME_EDIT_FIELDS, true );
            if( (CurrentLibEntry->m_UnitCount > 1) || g_AsDeMorgan )
                m_HToolBar->EnableTool( ID_LIBEDIT_EDIT_PIN_BY_PIN, true );
            else
                m_HToolBar->EnableTool( ID_LIBEDIT_EDIT_PIN_BY_PIN, false );

            m_HToolBar->ToggleTool( ID_LIBEDIT_EDIT_PIN_BY_PIN, g_EditPinByPinIsOn );

            m_HToolBar->EnableTool( ID_DE_MORGAN_CONVERT_BUTT, g_AsDeMorgan );
            m_HToolBar->EnableTool( ID_DE_MORGAN_NORMAL_BUTT, g_AsDeMorgan );
            /* Enable the "get doc" tool */
            bool enable_dtool = false;
            if( !CurrentAliasName.IsEmpty() )
            {
                int AliasLocation = LocateAlias( CurrentLibEntry->m_AliasList, CurrentAliasName );
                if( AliasLocation >= 0 )
                    if( !CurrentLibEntry->m_AliasList[AliasLocation +
                                                      ALIAS_DOC_FILENAME].IsEmpty() )
                        enable_dtool = true;
            }
            else if( !CurrentLibEntry->m_DocFile.IsEmpty() )
                enable_dtool = true;
            if( enable_dtool )
                m_HToolBar->EnableTool( ID_LIBEDIT_VIEW_DOC, true );
            else
                m_HToolBar->EnableTool( ID_LIBEDIT_VIEW_DOC, false );
            m_HToolBar->EnableTool( ID_LIBEDIT_CHECK_PART, true );
            m_SelpartBox->Enable( (CurrentLibEntry->m_UnitCount > 1 ) ? true : false );

            if( GetScreen() )
            {
                m_HToolBar->EnableTool( ID_LIBEDIT_UNDO, GetScreen()->m_UndoList );
                m_HToolBar->EnableTool( ID_LIBEDIT_REDO, GetScreen()->m_RedoList );
            }
        }

        if( m_VToolBar )
        {
            m_VToolBar->EnableTool( ID_LIBEDIT_BODY_TEXT_BUTT, true );
            m_VToolBar->EnableTool( ID_LIBEDIT_BODY_LINE_BUTT, true );
            m_VToolBar->EnableTool( ID_LIBEDIT_BODY_RECT_BUTT, true );
            m_VToolBar->EnableTool( ID_LIBEDIT_BODY_CIRCLE_BUTT, true );
            m_VToolBar->EnableTool( ID_LIBEDIT_BODY_ARC_BUTT, true );
            m_VToolBar->EnableTool( ID_LIBEDIT_DELETE_ITEM_BUTT, true );
            m_VToolBar->EnableTool( ID_LIBEDIT_ANCHOR_ITEM_BUTT, true );
            m_VToolBar->EnableTool( ID_LIBEDIT_IMPORT_BODY_BUTT, true );
            m_VToolBar->EnableTool( ID_LIBEDIT_EXPORT_BODY_BUTT, true );
        }
    }

    DisplayUnitsMsg();
}


/**************************************/
int WinEDA_LibeditFrame::BestZoom()
/**************************************/
{
    int      dx, dy, ii, jj;
    int      bestzoom;
    wxSize   size;
    EDA_Rect BoundaryBox;

    if( CurrentLibEntry )
    {
        BoundaryBox = CurrentLibEntry->GetBoundaryBox( CurrentUnit, CurrentConvert );
        dx = BoundaryBox.GetWidth();
        dy = BoundaryBox.GetHeight();
    }
    else
    {
        dx = GetScreen()->m_CurrentSheetDesc->m_Size.x;
        dy = GetScreen()->m_CurrentSheetDesc->m_Size.y;
    }

    size    = DrawPanel->GetClientSize();
    size -= wxSize(100,100);   // reserve 100 mils margin
    ii = abs( dx / size.x );
    jj = abs( dy / size.y );

    bestzoom = MAX( ii, jj ) + 1;

    if( CurrentLibEntry )
    {
        GetScreen()->m_Curseur = BoundaryBox.Centre();
    }
    else
    {
        GetScreen()->m_Curseur.x = 0;
        GetScreen()->m_Curseur.y = 0;
    }

    return bestzoom * GetScreen()->m_ZoomScalar;
}


/*************************************************************************/
void WinEDA_LibeditFrame::Process_Special_Functions( wxCommandEvent& event )
/*************************************************************************/
{
    int        id = event.GetId();
    wxPoint    pos;

    wxClientDC dc( DrawPanel );

    DrawPanel->m_IgnoreMouseEvents = true;

    DrawPanel->PrepareGraphicContext( &dc );

    wxGetMousePosition( &pos.x, &pos.y );
    pos.y += 20;

    switch( id )   // Arret de la commande de d�placement en cours
    {
    case ID_POPUP_LIBEDIT_END_CREATE_ITEM:
    case ID_POPUP_LIBEDIT_PIN_EDIT:
    case ID_POPUP_LIBEDIT_BODY_EDIT_ITEM:
    case ID_POPUP_LIBEDIT_FIELD_ROTATE_ITEM:
    case ID_POPUP_LIBEDIT_FIELD_EDIT_ITEM:
    case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINSIZE_ITEM:
    case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNAMESIZE_ITEM:
    case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNUMSIZE_ITEM:
    case ID_POPUP_LIBEDIT_CANCEL_EDITING:
    case ID_POPUP_ZOOM_BLOCK:
    case ID_POPUP_DELETE_BLOCK:
    case ID_POPUP_COPY_BLOCK:
    case ID_POPUP_SELECT_ITEMS_BLOCK:
    case ID_POPUP_INVERT_BLOCK:
    case ID_POPUP_PLACE_BLOCK:
    case ID_POPUP_LIBEDIT_DELETE_CURRENT_POLY_SEGMENT:
    case ID_POPUP_LIBEDIT_ROTATE_GRAPHIC_TEXT:
        break;

    case ID_POPUP_LIBEDIT_DELETE_ITEM:
        if( DrawPanel->ManageCurseur && DrawPanel->ForceCloseManageCurseur )
            DrawPanel->ForceCloseManageCurseur( DrawPanel, &dc );
        break;

    default:
        if( DrawPanel->ManageCurseur && DrawPanel->ForceCloseManageCurseur )
            DrawPanel->ForceCloseManageCurseur( DrawPanel, &dc );
        SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
        break;
    }

    switch( id )
    {
    case ID_LIBEDIT_SAVE_CURRENT_LIB:
        if( GetScreen()->IsModify() )
        {
            if( IsOK( this, _( "Include last component changes?" ) ) )
                SaveOnePartInMemory();
        }
        SaveActiveLibrary();
        break;

    case ID_LIBEDIT_NEW_PART:
    {
        g_EditPinByPinIsOn = false;
        LibItemToRepeat    = NULL;
        CreateNewLibraryPart();
        GetScreen()->ClearUndoRedoList();
        DrawPanel->Refresh();
        SetToolbars();
        break;
    }

    case ID_LIBEDIT_SELECT_CURRENT_LIB:
        SelectActiveLibrary();
        break;

    case ID_LIBEDIT_SELECT_PART:
        LibItemToRepeat = NULL;
        if( LoadOneLibraryPart() )
        {
            g_EditPinByPinIsOn = false;
            GetScreen()->ClearUndoRedoList();
            SetToolbars();
        }
        DrawPanel->Refresh();
        break;

    case ID_LIBEDIT_SAVE_CURRENT_PART:
        SaveOnePartInMemory();
        break;

    case ID_LIBEDIT_GET_FRAME_EDIT_PART:
        InstallLibeditFrame( );
        break;

    case ID_LIBEDIT_GET_FRAME_EDIT_FIELDS:
        InstallFieldsEditorDialog( );
        break;

    case ID_LIBEDIT_DELETE_PART:
        LibItemToRepeat = NULL;
        DeleteOnePart();
        break;

    case ID_LIBEDIT_IMPORT_PART:
        LibItemToRepeat = NULL;
        ImportOnePart();
        GetScreen()->ClearUndoRedoList();
        DrawPanel->Refresh();
        break;

    case ID_LIBEDIT_EXPORT_PART:
        ExportOnePart( false );
        break;

    case ID_LIBEDIT_CREATE_NEW_LIB_AND_SAVE_CURRENT_PART:
        ExportOnePart( true );
        break;

    case ID_LIBEDIT_CHECK_PART:
        if( CurrentLibEntry )
            if( TestPins( CurrentLibEntry ) == false )
                DisplayInfo( this, _( " Pins Test OK!" ) );
        break;

    case ID_DE_MORGAN_NORMAL_BUTT:
        m_HToolBar->ToggleTool( ID_DE_MORGAN_NORMAL_BUTT, true );
        m_HToolBar->ToggleTool( ID_DE_MORGAN_CONVERT_BUTT, false );
        LibItemToRepeat = NULL;
        CurrentConvert  = 1;
        DrawPanel->Refresh();
        break;

    case ID_DE_MORGAN_CONVERT_BUTT:
        m_HToolBar->ToggleTool( ID_DE_MORGAN_NORMAL_BUTT, false );
        m_HToolBar->ToggleTool( ID_DE_MORGAN_CONVERT_BUTT, true );
        LibItemToRepeat = NULL;
        CurrentConvert  = 2;
        DrawPanel->Refresh();
        break;

    case ID_LIBEDIT_VIEW_DOC:
        if( CurrentLibEntry )
        {
            wxString docfilename;
            if( !CurrentAliasName.IsEmpty() )
            {
                int AliasLocation = LocateAlias( CurrentLibEntry->m_AliasList, CurrentAliasName );
                if( AliasLocation >= 0 )
                    docfilename = CurrentLibEntry->m_AliasList[AliasLocation + ALIAS_DOC_FILENAME];
            }
            else
                docfilename = CurrentLibEntry->m_DocFile;

            if( !docfilename.IsEmpty() )
                GetAssociatedDocument( this, g_RealLibDirBuffer, docfilename );
        }
        break;

    case ID_LIBEDIT_EDIT_PIN_BY_PIN:
        g_EditPinByPinIsOn = g_EditPinByPinIsOn ? false : true;
        m_HToolBar->ToggleTool( ID_LIBEDIT_EDIT_PIN_BY_PIN, g_EditPinByPinIsOn );
        break;

    case ID_LIBEDIT_SELECT_PART_NUMBER:
    {
        int ii = m_SelpartBox->GetChoice();
        if( ii < 0 )
            return;
        LibItemToRepeat = NULL;
        CurrentUnit = ii + 1;
        DrawPanel->Refresh();
    }
        break;

    case ID_LIBEDIT_SELECT_ALIAS:
    {
        int ii = m_SelAliasBox->GetChoice();
        if( ii < 0 )
            return;
        LibItemToRepeat = NULL;
        if( ii > 0 )
            CurrentAliasName = m_SelAliasBox->GetValue();
        else
            CurrentAliasName.Empty();
        DrawPanel->Refresh();
    }
        break;

    case ID_POPUP_LIBEDIT_PIN_EDIT:
        InstallPineditFrame( this, &dc, pos );
        break;

    case ID_LIBEDIT_PIN_BUTT:
        if( CurrentLibEntry )
        {
            SetToolID( id, wxCURSOR_PENCIL, _( "Add Pin" ) );
        }
        else
        {
            SetToolID( id, wxCURSOR_ARROW, _( "Set Pin Options" ) );
            InstallPineditFrame( this, &dc, pos );
            SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
        }
        break;

    case ID_POPUP_LIBEDIT_CANCEL_EDITING:
        if( DrawPanel->ManageCurseur && DrawPanel->ForceCloseManageCurseur )
            DrawPanel->ForceCloseManageCurseur( DrawPanel, &dc );
        else
            SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
        break;

    case ID_NO_SELECT_BUTT:
        SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
        break;

    case ID_LIBEDIT_BODY_TEXT_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add Text" ) );
        break;

    case ID_LIBEDIT_BODY_RECT_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add Rectangle" ) );
        break;

    case ID_LIBEDIT_BODY_CIRCLE_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add Circle" ) );
        break;

    case ID_LIBEDIT_BODY_ARC_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add Arc" ) );
        break;

    case ID_LIBEDIT_BODY_LINE_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add Line" ) );
        break;

    case ID_LIBEDIT_ANCHOR_ITEM_BUTT:
        SetToolID( id, wxCURSOR_HAND, _( "Anchor" ) );
        break;

    case ID_LIBEDIT_IMPORT_BODY_BUTT:
        SetToolID( id, wxCURSOR_ARROW, _( "Import" ) );
        LoadOneSymbol( );
        SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
        break;

    case ID_LIBEDIT_EXPORT_BODY_BUTT:
        SetToolID( id, wxCURSOR_ARROW, _( "Export" ) );
        SaveOneSymbol();
        SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
        break;

    case ID_POPUP_LIBEDIT_END_CREATE_ITEM:
        DrawPanel->MouseToCursorSchema();
        if( CurrentDrawItem )
        {
            EndDrawGraphicItem( &dc );
        }
        break;

    case ID_POPUP_LIBEDIT_BODY_EDIT_ITEM:
        if( CurrentDrawItem )
        {
            DrawPanel->CursorOff( &dc );

            switch( CurrentDrawItem->Type() )
            {
            case COMPONENT_ARC_DRAW_TYPE:
            case COMPONENT_CIRCLE_DRAW_TYPE:
            case COMPONENT_RECT_DRAW_TYPE:
            case COMPONENT_POLYLINE_DRAW_TYPE:
            case COMPONENT_LINE_DRAW_TYPE:
                EditGraphicSymbol( &dc, CurrentDrawItem );
                break;

            case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
                EditSymbolText( &dc, CurrentDrawItem );
                break;

            default:
                ;
            }

            DrawPanel->CursorOn( &dc );
        }
        break;


    case ID_LIBEDIT_DELETE_ITEM_BUTT:
        if( CurrentLibEntry == NULL )
        {
            wxBell(); break;
        }
        SetToolID( id, wxCURSOR_BULLSEYE, _( "Delete item" ) );
        break;


    case ID_POPUP_LIBEDIT_DELETE_CURRENT_POLY_SEGMENT:

        // Delete the last created segment, while creating a polyline draw item
        if( CurrentDrawItem == NULL )
            break;
        DrawPanel->MouseToCursorSchema();
        DeleteDrawPoly( &dc );
        break;

    case ID_POPUP_LIBEDIT_DELETE_ITEM:
        if( CurrentDrawItem == NULL )
            break;
        DrawPanel->MouseToCursorSchema();
        DrawPanel->CursorOff( &dc );
        SaveCopyInUndoList( CurrentLibEntry );
        if( CurrentDrawItem->Type() == COMPONENT_PIN_DRAW_TYPE )
        {
            DeletePin( &dc, CurrentLibEntry, (LibDrawPin*) CurrentDrawItem );
        }
        else
        {
            if( DrawPanel->ManageCurseur && DrawPanel->ForceCloseManageCurseur )
                DrawPanel->ForceCloseManageCurseur( DrawPanel, &dc );
            else
                DeleteOneLibraryDrawStruct( DrawPanel, &dc, CurrentLibEntry, CurrentDrawItem, true );
        }

        CurrentDrawItem = NULL;
        GetScreen()->SetModify();
        DrawPanel->CursorOn( &dc );
        break;

    case ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST:
        if( CurrentDrawItem == NULL )
            break;
        DrawPanel->MouseToCursorSchema();
        if( CurrentDrawItem->Type() == COMPONENT_PIN_DRAW_TYPE )
            StartMovePin( &dc );
        else if( CurrentDrawItem->Type() == COMPONENT_FIELD_DRAW_TYPE )
            StartMoveField( &dc, (LibDrawField*) CurrentDrawItem );
        else
            StartMoveDrawSymbol( &dc );
        break;

    case ID_POPUP_LIBEDIT_ROTATE_GRAPHIC_TEXT:
        if( CurrentDrawItem == NULL )
            break;
        DrawPanel->CursorOff( &dc );
        DrawPanel->MouseToCursorSchema();
        if( (CurrentDrawItem->m_Flags & IS_NEW) == 0 )
            SaveCopyInUndoList( CurrentLibEntry );
        RotateSymbolText( &dc );
        DrawPanel->CursorOn( &dc );
        break;

    case ID_POPUP_LIBEDIT_FIELD_ROTATE_ITEM:
        if( CurrentDrawItem == NULL )
            break;
        DrawPanel->CursorOff( &dc );
        DrawPanel->MouseToCursorSchema();
        if( CurrentDrawItem->Type() == COMPONENT_FIELD_DRAW_TYPE )
        {
            SaveCopyInUndoList( CurrentLibEntry );
            RotateField( &dc, (LibDrawField*) CurrentDrawItem );
        }
        DrawPanel->CursorOn( &dc );
        break;

    case ID_POPUP_LIBEDIT_FIELD_EDIT_ITEM:
        if( CurrentDrawItem == NULL )
            break;
        DrawPanel->CursorOff( &dc );
        if( CurrentDrawItem->Type() == COMPONENT_FIELD_DRAW_TYPE )
        {
            EditField( &dc, (LibDrawField*) CurrentDrawItem );
        }
        DrawPanel->MouseToCursorSchema();
        DrawPanel->CursorOn( &dc );
        break;

    case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINSIZE_ITEM:
    case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNAMESIZE_ITEM:
    case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNUMSIZE_ITEM:
        if( (CurrentDrawItem == NULL)
           || (CurrentDrawItem->Type() != COMPONENT_PIN_DRAW_TYPE) )
            break;
        SaveCopyInUndoList( CurrentLibEntry );
        GlobalSetPins( &dc, (LibDrawPin*) CurrentDrawItem, id );
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_ZOOM_BLOCK:
        DrawPanel->m_AutoPAN_Request = false;
        GetScreen()->BlockLocate.m_Command = BLOCK_ZOOM;
        HandleBlockEnd( &dc );
        break;

    case ID_POPUP_DELETE_BLOCK:
        DrawPanel->m_AutoPAN_Request = false;
        GetScreen()->BlockLocate.m_Command = BLOCK_DELETE;
        DrawPanel->MouseToCursorSchema();
        HandleBlockEnd( &dc );
        break;

    case ID_POPUP_COPY_BLOCK:
        DrawPanel->m_AutoPAN_Request = false;
        GetScreen()->BlockLocate.m_Command = BLOCK_COPY;
        DrawPanel->MouseToCursorSchema();
        HandleBlockPlace( &dc );
        break;

    case ID_POPUP_SELECT_ITEMS_BLOCK:
        DrawPanel->m_AutoPAN_Request = false;
        GetScreen()->BlockLocate.m_Command = BLOCK_SELECT_ITEMS_ONLY;
        DrawPanel->MouseToCursorSchema();
        HandleBlockEnd( &dc );
        break;

    case ID_POPUP_INVERT_BLOCK:
        DrawPanel->m_AutoPAN_Request = false;
        GetScreen()->BlockLocate.m_Command = BLOCK_INVERT;
        DrawPanel->MouseToCursorSchema();
        HandleBlockPlace( &dc );
        break;

    case ID_POPUP_PLACE_BLOCK:
        DrawPanel->m_AutoPAN_Request = false;
        DrawPanel->MouseToCursorSchema();
        HandleBlockPlace( &dc );
        break;

    case ID_LIBEDIT_UNDO:
        if( GetComponentFromUndoList() )
            DrawPanel->Refresh( true );
        break;

    case ID_LIBEDIT_REDO:
        if( GetComponentFromRedoList() )
            DrawPanel->Refresh( true );
        break;

    default:
        DisplayError( this, wxT( "WinEDA_LibeditFrame::Process_Special_Functions error" ) );
        break;
    }

    DrawPanel->m_IgnoreMouseEvents = false;

    if( m_ID_current_state == 0 )
        LibItemToRepeat = NULL;
}
