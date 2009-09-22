/****************************/
/*  EESchema - libframe.cpp */
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
#include "eeschema_id.h"
#include "class_library.h"
#include "libeditfrm.h"

#include <boost/foreach.hpp>


/* Library editor wxConfig entry names. */
const wxString lastLibExportPathEntry( wxT( "LastLibraryExportPath" ) );
const wxString lastLibImportPathEntry( wxT( "LastLibraryImportPath" ) );
const wxString showGridPathEntry( wxT( "ShowGrid" ) );

/* This method guarentees unique IDs for the library this run of Eeschema
 * which prevents ID conflicts and eliminates the need to recompile every
 * source file in the project when adding IDs to include/id.h. */
int ExportPartId = ::wxNewId();
int ImportPartId = ::wxNewId();
int CreateNewLibAndSavePartId = ::wxNewId();


LIB_COMPONENT* WinEDA_LibeditFrame::m_currentComponent = NULL;


/*****************************/
/* class WinEDA_LibeditFrame */
/*****************************/
BEGIN_EVENT_TABLE( WinEDA_LibeditFrame, WinEDA_DrawFrame )
    EVT_CLOSE( WinEDA_LibeditFrame::OnCloseWindow )
    EVT_SIZE( WinEDA_LibeditFrame::OnSize )

/* Main horizontal toolbar. */
    EVT_TOOL_RANGE( ID_ZOOM_IN, ID_ZOOM_PAGE, WinEDA_LibeditFrame::OnZoom )
    EVT_TOOL( ID_LIBEDIT_SAVE_CURRENT_LIB,
              WinEDA_LibeditFrame::SaveActiveLibrary )
    EVT_TOOL( ID_LIBEDIT_SELECT_CURRENT_LIB,
              WinEDA_LibeditFrame::Process_Special_Functions )
    EVT_TOOL( ID_LIBEDIT_DELETE_PART,
              WinEDA_LibeditFrame::DeleteOnePart )
    EVT_TOOL( ID_LIBEDIT_NEW_PART,
              WinEDA_LibeditFrame::CreateNewLibraryPart )
    EVT_TOOL( ID_LIBEDIT_SELECT_PART,
              WinEDA_LibeditFrame::Process_Special_Functions )
    EVT_TOOL( ID_LIBEDIT_SAVE_CURRENT_PART,
              WinEDA_LibeditFrame::Process_Special_Functions )
    EVT_TOOL( ID_LIBEDIT_UNDO,
              WinEDA_LibeditFrame::GetComponentFromUndoList )
    EVT_TOOL( ID_LIBEDIT_REDO,
              WinEDA_LibeditFrame::GetComponentFromRedoList )
    EVT_TOOL( ID_LIBEDIT_GET_FRAME_EDIT_PART,
              WinEDA_LibeditFrame::OnEditComponentProperties )
    EVT_TOOL( ID_LIBEDIT_GET_FRAME_EDIT_FIELDS,
              WinEDA_LibeditFrame::InstallFieldsEditorDialog )
    EVT_TOOL( ID_LIBEDIT_CHECK_PART,
              WinEDA_LibeditFrame::Process_Special_Functions )
    EVT_TOOL( ID_DE_MORGAN_NORMAL_BUTT,
              WinEDA_LibeditFrame::Process_Special_Functions )
    EVT_TOOL( ID_DE_MORGAN_CONVERT_BUTT,
              WinEDA_LibeditFrame::Process_Special_Functions )
    EVT_TOOL( ID_LIBEDIT_VIEW_DOC,
              WinEDA_LibeditFrame::Process_Special_Functions )
    EVT_TOOL( ID_LIBEDIT_EDIT_PIN_BY_PIN,
              WinEDA_LibeditFrame::Process_Special_Functions )
    EVT_TOOL( ExportPartId, WinEDA_LibeditFrame::OnExportPart )
    EVT_TOOL( CreateNewLibAndSavePartId, WinEDA_LibeditFrame::OnExportPart )
    EVT_TOOL( ImportPartId, WinEDA_LibeditFrame::OnImportPart )


    EVT_KICAD_CHOICEBOX( ID_LIBEDIT_SELECT_PART_NUMBER,
                         WinEDA_LibeditFrame::OnSelectPart )
    EVT_KICAD_CHOICEBOX( ID_LIBEDIT_SELECT_ALIAS,
                         WinEDA_LibeditFrame::OnSelectAlias )

/* Right vertical toolbar. */
    EVT_TOOL( ID_NO_SELECT_BUTT, WinEDA_LibeditFrame::Process_Special_Functions )
    EVT_TOOL_RANGE( ID_LIBEDIT_PIN_BUTT, ID_LIBEDIT_EXPORT_BODY_BUTT,
                    WinEDA_LibeditFrame::Process_Special_Functions )

/* Context menu events and commands. */
    EVT_MENU_RANGE( ID_POPUP_LIBEDIT_PIN_EDIT,
                    ID_POPUP_LIBEDIT_ROTATE_GRAPHIC_TEXT,
                    WinEDA_LibeditFrame::Process_Special_Functions )

    EVT_MENU_RANGE( ID_POPUP_GENERAL_START_RANGE, ID_POPUP_GENERAL_END_RANGE,
                    WinEDA_LibeditFrame::Process_Special_Functions )

/* Update user interface elements. */
    EVT_UPDATE_UI( ExportPartId, WinEDA_LibeditFrame::OnUpdateEditingPart )
    EVT_UPDATE_UI( CreateNewLibAndSavePartId,
                   WinEDA_LibeditFrame::OnUpdateEditingPart )
    EVT_UPDATE_UI( ID_LIBEDIT_SAVE_CURRENT_PART,
                   WinEDA_LibeditFrame::OnUpdateEditingPart )
    EVT_UPDATE_UI( ID_LIBEDIT_GET_FRAME_EDIT_FIELDS,
                   WinEDA_LibeditFrame::OnUpdateEditingPart )
    EVT_UPDATE_UI( ID_LIBEDIT_CHECK_PART,
                   WinEDA_LibeditFrame::OnUpdateEditingPart )
    EVT_UPDATE_UI( ID_LIBEDIT_UNDO, WinEDA_LibeditFrame::OnUpdateUndo )
    EVT_UPDATE_UI( ID_LIBEDIT_REDO, WinEDA_LibeditFrame::OnUpdateRedo )
    EVT_UPDATE_UI( ID_LIBEDIT_SAVE_CURRENT_LIB,
                   WinEDA_LibeditFrame::OnUpdateSaveCurrentLib )
    EVT_UPDATE_UI( ID_LIBEDIT_VIEW_DOC, WinEDA_LibeditFrame::OnUpdateViewDoc )
    EVT_UPDATE_UI( ID_LIBEDIT_EDIT_PIN_BY_PIN,
                   WinEDA_LibeditFrame::OnUpdatePinByPin )
    EVT_UPDATE_UI( ID_LIBEDIT_SELECT_PART_NUMBER,
                   WinEDA_LibeditFrame::OnUpdatePartNumber )
    EVT_UPDATE_UI( ID_LIBEDIT_SELECT_ALIAS,
                   WinEDA_LibeditFrame::OnUpdateSelectAlias )
    EVT_UPDATE_UI( ID_DE_MORGAN_NORMAL_BUTT,
                   WinEDA_LibeditFrame::OnUpdateDeMorganNormal )
    EVT_UPDATE_UI( ID_DE_MORGAN_CONVERT_BUTT,
                   WinEDA_LibeditFrame::OnUpdateDeMorganConvert )
    EVT_UPDATE_UI_RANGE( ID_LIBEDIT_PIN_BUTT, ID_LIBEDIT_EXPORT_BODY_BUTT,
                         WinEDA_LibeditFrame::OnUpdateEditingPart )

END_EVENT_TABLE()


WinEDA_LibeditFrame::WinEDA_LibeditFrame( wxWindow*       father,
                                          const wxString& title,
                                          const wxPoint&  pos,
                                          const wxSize&   size,
                                          long            style ) :
    WinEDA_DrawFrame( father, LIBEDITOR_FRAME, title, pos, size, style )
{
    m_FrameName = wxT( "LibeditFrame" );
    m_Draw_Axis = true;             // true pour avoir les axes dessines
    m_Draw_Grid = true;             // true pour avoir la axes dessinee
    m_ConfigPath = wxT( "LibraryEditor" );

    // Give an icon
    SetIcon( wxIcon( libedit_xpm ) );
    SetBaseScreen( g_ScreenLib );
    GetScreen()->m_Center = true;
    LoadSettings();
    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );
    if( DrawPanel )
        DrawPanel->m_Block_Enable = true;
    ReCreateHToolbar();
    ReCreateVToolbar();
    DisplayLibInfos();
    DisplayCmpDoc();
    UpdateAliasSelectList();
    UpdatePartSelectList();
    BestZoom();
    Show( true );
}


WinEDA_LibeditFrame::~WinEDA_LibeditFrame()
{
    WinEDA_SchematicFrame* frame =
        (WinEDA_SchematicFrame*) wxGetApp().GetTopWindow();
    frame->m_LibeditFrame = NULL;
}


/**
 * Load library editor frame specific configuration settings.
 *
 * Don't forget to call this base method from any derived classes or the
 * settings will not get loaded.
 */
void WinEDA_LibeditFrame::LoadSettings( )
{
    wxConfig* cfg;

    WinEDA_DrawFrame::LoadSettings();

    wxConfigPathChanger cpc( wxGetApp().m_EDA_Config, m_ConfigPath );
    cfg = wxGetApp().m_EDA_Config;

    m_LastLibExportPath = cfg->Read( lastLibExportPathEntry, ::wxGetCwd() );
    m_LastLibImportPath = cfg->Read( lastLibImportPathEntry, ::wxGetCwd() );
}


/**
 * Save library editor frame specific configuration settings.
 *
 * Don't forget to call this base method from any derived classes or the
 * settings will not get saved.
 */
void WinEDA_LibeditFrame::SaveSettings()
{
    wxConfig* cfg;

    WinEDA_DrawFrame::SaveSettings();

    wxConfigPathChanger cpc( wxGetApp().m_EDA_Config, m_ConfigPath );
    cfg = wxGetApp().m_EDA_Config;

    cfg->Write( lastLibExportPathEntry, m_LastLibExportPath );
    cfg->Write( lastLibImportPathEntry, m_LastLibImportPath );
}


void WinEDA_LibeditFrame::OnCloseWindow( wxCloseEvent& Event )
{
    if( GetScreen()->IsModify() )
    {
        if( !IsOK( this, _( "Component was modified!\nDiscard changes?" ) ) )
        {
            Event.Veto();
            return;
        }
        else
            GetScreen()->ClrModify();
    }

    BOOST_FOREACH( const CMP_LIBRARY& lib, CMP_LIBRARY::GetLibraryList() )
    {
        if( lib.IsModified() )
        {
            wxString msg;
            msg.Printf( _( "Library \"%s\" was modified!\nDiscard changes?" ),
                        (const wxChar*) lib.GetName() );
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


int WinEDA_LibeditFrame::BestZoom()
{
    int      dx, dy, ii, jj;
    int      bestzoom;
    wxSize   size;
    EDA_Rect BoundaryBox;

    if( m_currentComponent )
    {
        BoundaryBox = m_currentComponent->GetBoundaryBox( CurrentUnit,
                                                          CurrentConvert );
        dx = BoundaryBox.GetWidth();
        dy = BoundaryBox.GetHeight();
    }
    else
    {
        dx = GetScreen()->m_CurrentSheetDesc->m_Size.x;
        dy = GetScreen()->m_CurrentSheetDesc->m_Size.y;
    }

    size    = DrawPanel->GetClientSize();
    size -= wxSize( 100, 100 );   // reserve 100 mils margin
    ii = abs( dx / size.x );
    jj = abs( dy / size.y );

    bestzoom = MAX( ii, jj ) + 1;

    if( m_currentComponent )
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


void WinEDA_LibeditFrame::UpdateAliasSelectList()
{
    if( m_SelAliasBox == NULL )
        return;

    m_SelAliasBox->Clear();

    if( m_currentComponent == NULL )
        return;

    m_SelAliasBox->Append( m_currentComponent->GetName() );
    m_SelAliasBox->SetSelection( 0 );

    if( !m_currentComponent->m_AliasList.IsEmpty() )
    {
        m_SelAliasBox->Append( m_currentComponent->m_AliasList );

        int index = m_SelAliasBox->FindString( CurrentAliasName );

        if( index != wxNOT_FOUND )
            m_SelAliasBox->SetSelection( index );
    }
}


void WinEDA_LibeditFrame::UpdatePartSelectList()
{
    if( m_SelpartBox == NULL )
        return;


    if( m_SelpartBox->GetCount() != 0 )
        m_SelpartBox->Clear();

    if( m_currentComponent == NULL || m_currentComponent->m_UnitCount <= 1 )
    {
        m_SelpartBox->Append( wxEmptyString );
    }
    else
    {
        for( int i = 0; i < m_currentComponent->m_UnitCount; i++ )
        {
            wxString msg;
            msg.Printf( _( "Part %c" ), 'A' + i );
            m_SelpartBox->Append( msg );
        }
    }

    m_SelpartBox->SetSelection( ( CurrentUnit > 0 ) ? CurrentUnit - 1 : 0 );
}


void WinEDA_LibeditFrame::OnUpdateEditingPart( wxUpdateUIEvent& event )
{
    event.Enable( m_currentComponent != NULL );
}


void WinEDA_LibeditFrame::OnUpdateNotEditingPart( wxUpdateUIEvent& event )
{
    event.Enable( m_currentComponent == NULL );
}


void WinEDA_LibeditFrame::OnUpdateUndo( wxUpdateUIEvent& event )
{
    event.Enable( m_currentComponent != NULL && GetScreen() != NULL
                  && GetScreen()->GetUndoCommandCount() != 0 );
}


void WinEDA_LibeditFrame::OnUpdateRedo( wxUpdateUIEvent& event )
{
    event.Enable( m_currentComponent != NULL && GetScreen() != NULL
                  && GetScreen()->GetRedoCommandCount() != 0 );
}


void WinEDA_LibeditFrame::OnUpdateSaveCurrentLib( wxUpdateUIEvent& event )
{
    event.Enable( CurrentLib != NULL
                  && ( CurrentLib->IsModified() || GetScreen()->IsModify() ) );
}


void WinEDA_LibeditFrame::OnUpdateViewDoc( wxUpdateUIEvent& event )
{
    bool enable = false;

    if( m_currentComponent != NULL && CurrentLib != NULL )
    {
        if( !CurrentAliasName.IsEmpty() )
        {
            CMP_LIB_ENTRY* entry = CurrentLib->FindEntry( CurrentAliasName );

            if( entry != NULL )
                enable = !entry->m_DocFile.IsEmpty();
        }
        else if( !m_currentComponent->m_DocFile.IsEmpty() )
        {
            enable = true;
        }
    }

    event.Enable( enable );
}


void WinEDA_LibeditFrame::OnUpdatePinByPin( wxUpdateUIEvent& event )
{
    event.Enable( ( m_currentComponent != NULL )
                  && ( ( m_currentComponent->m_UnitCount > 1 )
                       || g_AsDeMorgan ) );

    if( m_HToolBar )
        m_HToolBar->ToggleTool( event.GetId(), g_EditPinByPinIsOn );
}


void WinEDA_LibeditFrame::OnUpdatePartNumber( wxUpdateUIEvent& event )
{
    if( m_SelpartBox == NULL )
        return;

    /* Using the typical event.Enable() call dosen't seem to work with wxGTK
     * so use the pointer to alias combobox to directly enable or disable.
     */
    m_SelpartBox->Enable( m_currentComponent != NULL
                          && m_currentComponent->m_UnitCount > 1 );
}


void WinEDA_LibeditFrame::OnUpdateDeMorganNormal( wxUpdateUIEvent& event )
{
    if( m_HToolBar == NULL )
        return;

    event.Enable( m_currentComponent != NULL
                  && m_currentComponent->HasConversion() );
    m_HToolBar->ToggleTool( event.GetId(), CurrentConvert <= 1 );
}


void WinEDA_LibeditFrame::OnUpdateDeMorganConvert( wxUpdateUIEvent& event )
{
    if( m_HToolBar == NULL )
        return;

    event.Enable( m_currentComponent != NULL
                  && m_currentComponent->HasConversion() );
    m_HToolBar->ToggleTool( event.GetId(), CurrentConvert > 1 );
}


void WinEDA_LibeditFrame::OnUpdateSelectAlias( wxUpdateUIEvent& event )
{
    if( m_SelAliasBox == NULL )
        return;

    /* Using the typical event.Enable() call dosen't seem to work with wxGTK
     * so use the pointer to alias combobox to directly enable or disable.
     */
    m_SelAliasBox->Enable( m_currentComponent != NULL
                           && !m_currentComponent->m_AliasList.IsEmpty() );
}


void WinEDA_LibeditFrame::OnSelectAlias( wxCommandEvent& event )
{
    if( m_SelAliasBox == NULL
        || m_SelAliasBox->GetStringSelection().CmpNoCase( CurrentAliasName ) == 0 )
        return;

    LibItemToRepeat = NULL;

    if( m_SelAliasBox->GetStringSelection().CmpNoCase(m_currentComponent->GetName() ) == 0 )
        CurrentAliasName.Empty();
    else
        CurrentAliasName = m_SelAliasBox->GetStringSelection();

    DisplayCmpDoc();
    DrawPanel->Refresh();
}


void WinEDA_LibeditFrame::OnSelectPart( wxCommandEvent& event )
{
    int i = event.GetSelection();

    if( ( i == wxNOT_FOUND ) || ( ( i + 1 ) == CurrentUnit ) )
        return;

    LibItemToRepeat = NULL;
    CurrentUnit = i + 1;
    DrawPanel->Refresh();
    DisplayCmpDoc();
}


void WinEDA_LibeditFrame::Process_Special_Functions( wxCommandEvent& event )
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
    case ID_POPUP_MIRROR_Y_BLOCK:
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
    case ID_LIBEDIT_SELECT_CURRENT_LIB:
        SelectActiveLibrary();
        break;

    case ID_LIBEDIT_SELECT_PART:
        LibItemToRepeat = NULL;
        if( LoadOneLibraryPart() )
        {
            g_EditPinByPinIsOn = false;
            GetScreen()->ClearUndoRedoList();
        }
        DrawPanel->Refresh();
        break;

    case ID_LIBEDIT_SAVE_CURRENT_PART:
        SaveOnePartInMemory();
        break;

    case ID_LIBEDIT_CHECK_PART:
        if( m_currentComponent && TestPins( m_currentComponent ) == false )
            DisplayInfoMessage( this, _( " Pins Test OK!" ) );
        break;

    case ID_DE_MORGAN_NORMAL_BUTT:
        LibItemToRepeat = NULL;
        CurrentConvert  = 1;
        DrawPanel->Refresh();
        break;

    case ID_DE_MORGAN_CONVERT_BUTT:
        LibItemToRepeat = NULL;
        CurrentConvert  = 2;
        DrawPanel->Refresh();
        break;

    case ID_LIBEDIT_VIEW_DOC:
        if( m_currentComponent )
        {
            wxString docfilename;
            if( !CurrentAliasName.IsEmpty() )
            {
                CMP_LIB_ENTRY* entry = CurrentLib->FindEntry( CurrentAliasName );
                if( entry != NULL )
                    docfilename = entry->m_DocFile;
            }
            else
                docfilename = m_currentComponent->m_DocFile;

            if( !docfilename.IsEmpty() )
                GetAssociatedDocument( this, docfilename,
                                       &wxGetApp().GetLibraryPathList() );
        }
        break;

    case ID_LIBEDIT_EDIT_PIN_BY_PIN:
        g_EditPinByPinIsOn = g_EditPinByPinIsOn ? false : true;
        break;

    case ID_POPUP_LIBEDIT_PIN_EDIT:
        InstallPineditFrame( this, &dc, pos );
        break;

    case ID_LIBEDIT_PIN_BUTT:
        if( m_currentComponent )
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
        if( m_currentComponent == NULL )
        {
            wxBell();
            break;
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
        SaveCopyInUndoList( m_currentComponent );
        if( CurrentDrawItem->Type() == COMPONENT_PIN_DRAW_TYPE )
        {
            DeletePin( &dc, m_currentComponent, (LibDrawPin*) CurrentDrawItem );
        }
        else
        {
            if( DrawPanel->ManageCurseur && DrawPanel->ForceCloseManageCurseur )
                DrawPanel->ForceCloseManageCurseur( DrawPanel, &dc );
            else
                m_currentComponent->RemoveDrawItem( CurrentDrawItem, DrawPanel,
                                                    &dc );
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
            SaveCopyInUndoList( m_currentComponent );
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
            SaveCopyInUndoList( m_currentComponent );
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
        if( (CurrentDrawItem == NULL )
           || (CurrentDrawItem->Type() != COMPONENT_PIN_DRAW_TYPE) )
            break;
        SaveCopyInUndoList( m_currentComponent );
        GlobalSetPins( &dc, (LibDrawPin*) CurrentDrawItem, id );
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_ZOOM_BLOCK:
        DrawPanel->m_AutoPAN_Request = false;
        GetScreen()->m_BlockLocate.m_Command = BLOCK_ZOOM;
        HandleBlockEnd( &dc );
        break;

    case ID_POPUP_DELETE_BLOCK:
        DrawPanel->m_AutoPAN_Request = false;
        GetScreen()->m_BlockLocate.m_Command = BLOCK_DELETE;
        DrawPanel->MouseToCursorSchema();
        HandleBlockEnd( &dc );
        break;

    case ID_POPUP_COPY_BLOCK:
        DrawPanel->m_AutoPAN_Request = false;
        GetScreen()->m_BlockLocate.m_Command = BLOCK_COPY;
        DrawPanel->MouseToCursorSchema();
        HandleBlockPlace( &dc );
        break;

    case ID_POPUP_SELECT_ITEMS_BLOCK:
        DrawPanel->m_AutoPAN_Request = false;
        GetScreen()->m_BlockLocate.m_Command = BLOCK_SELECT_ITEMS_ONLY;
        DrawPanel->MouseToCursorSchema();
        HandleBlockEnd( &dc );
        break;

    case ID_POPUP_MIRROR_Y_BLOCK:
        DrawPanel->m_AutoPAN_Request = false;
        GetScreen()->m_BlockLocate.m_Command = BLOCK_MIRROR_Y;
        DrawPanel->MouseToCursorSchema();
        HandleBlockPlace( &dc );
        break;

    case ID_POPUP_PLACE_BLOCK:
        DrawPanel->m_AutoPAN_Request = false;
        DrawPanel->MouseToCursorSchema();
        HandleBlockPlace( &dc );
        break;

    default:
        DisplayError( this,
                      wxT( "WinEDA_LibeditFrame::Process_Special_Functions error" ) );
        break;
    }

    DrawPanel->m_IgnoreMouseEvents = false;

    if( m_ID_current_state == 0 )
        LibItemToRepeat = NULL;
}
