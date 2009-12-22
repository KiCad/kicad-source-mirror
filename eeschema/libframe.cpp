/****************************/
/*  EESchema - libframe.cpp */
/****************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "eda_doc.h"
#include "bitmaps.h"

#include "program.h"
#include "general.h"
#include "protos.h"
#include "eeschema_id.h"
#include "libeditfrm.h"
#include "class_library.h"

#include "kicad_device_context.h"

#include <boost/foreach.hpp>


/* Library editor wxConfig entry names. */
const wxString lastLibExportPathEntry( wxT( "LastLibraryExportPath" ) );
const wxString lastLibImportPathEntry( wxT( "LastLibraryImportPath" ) );
const wxString showGridPathEntry( wxT( "ShowGrid" ) );

/* This method guarantees unique IDs for the library this run of Eeschema
 * which prevents ID conflicts and eliminates the need to recompile every
 * source file in the project when adding IDs to include/id.h. */
int ExportPartId = ::wxNewId();
int ImportPartId = ::wxNewId();
int CreateNewLibAndSavePartId = ::wxNewId();


/*
 * Static component library editor members.  These are static so their
 * state is saved between editing sessions.  This way the last component
 * that was being edited will be displayed.  These members are protected
 * making it necessary to use the class access methods.
 */
LIB_COMPONENT* WinEDA_LibeditFrame::m_component = NULL;
CMP_LIBRARY* WinEDA_LibeditFrame::m_library = NULL;

wxString WinEDA_LibeditFrame::m_aliasName;
int WinEDA_LibeditFrame::m_unit = 1;
int WinEDA_LibeditFrame::m_convert = 1;
LIB_DRAW_ITEM* WinEDA_LibeditFrame::m_lastDrawItem = NULL;
LIB_DRAW_ITEM* WinEDA_LibeditFrame::m_drawItem = NULL;
bool WinEDA_LibeditFrame::m_showDeMorgan = false;
wxSize WinEDA_LibeditFrame::m_clientSize = wxSize( -1, -1 );
int WinEDA_LibeditFrame::m_textSize = DEFAULT_SIZE_TEXT;
int WinEDA_LibeditFrame::m_textOrientation = TEXT_ORIENT_HORIZ;
int WinEDA_LibeditFrame::m_drawLineWidth = 0;
FILL_T WinEDA_LibeditFrame::m_drawFillStyle = NO_FILL;


/*****************************/
/* class WinEDA_LibeditFrame */
/*****************************/
BEGIN_EVENT_TABLE( WinEDA_LibeditFrame, WinEDA_DrawFrame )
    EVT_CLOSE( WinEDA_LibeditFrame::OnCloseWindow )
    EVT_SIZE( WinEDA_LibeditFrame::OnSize )
    EVT_ACTIVATE( WinEDA_LibeditFrame::OnActivate )

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
              WinEDA_LibeditFrame::LoadOneLibraryPart )
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
              WinEDA_LibeditFrame::OnCheckComponent )
    EVT_TOOL( ID_DE_MORGAN_NORMAL_BUTT,
              WinEDA_LibeditFrame::OnSelectBodyStyle )
    EVT_TOOL( ID_DE_MORGAN_CONVERT_BUTT,
              WinEDA_LibeditFrame::OnSelectBodyStyle )
    EVT_TOOL( ID_LIBEDIT_VIEW_DOC,
              WinEDA_LibeditFrame::OnViewEntryDoc )
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
    EVT_MENU( ID_LIBEDIT_EDIT_PIN, WinEDA_LibeditFrame::OnEditPin )

    EVT_MENU_RANGE( ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_ITEM,
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

WinEDA_LibeditFrame::WinEDA_LibeditFrame( wxWindow* father,
                                          const wxString& title,
                                          const wxPoint& pos,
                                          const wxSize& size,
                                          long style ) :
    WinEDA_DrawFrame( father, LIBEDITOR_FRAME, title, pos, size, style )
{
    m_FrameName  = wxT( "LibeditFrame" );
    m_Draw_Axis  = true;            // true to draw axis
    m_Draw_Grid  = true;            // true to draw grid
    m_ConfigPath = wxT( "LibraryEditor" );
    SetShowDeMorgan( false );
    m_drawSpecificConvert = true;
    m_drawSpecificUnit    = false;

    // Give an icon
    SetIcon( wxIcon( libedit_xpm ) );
    SetBaseScreen( new SCH_SCREEN() );
    GetScreen()->m_Center = true;
    LoadSettings();

    // Initilialize grid id to a default value if not found in config or bad:
    if( (m_LastGridSizeId <= 0)
       || ( m_LastGridSizeId < (ID_POPUP_GRID_USER - ID_POPUP_GRID_LEVEL_1000) ) )
        m_LastGridSizeId = ID_POPUP_GRID_LEVEL_50 - ID_POPUP_GRID_LEVEL_1000;

    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );
    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId  );

    if( DrawPanel )
        DrawPanel->m_Block_Enable = true;

    EnsureActiveLibExists();
    ReCreateHToolbar();
    ReCreateVToolbar();
    DisplayLibInfos();
    DisplayCmpDoc();
    UpdateAliasSelectList();
    UpdatePartSelectList();
    Zoom_Automatique( false );
    Show( true );

#if defined(KICAD_AUIMANAGER)
    m_auimgr.SetManagedWindow( this );

    wxAuiPaneInfo horiz;
    horiz.Gripper( false );
    horiz.DockFixed( true );
    horiz.Movable( false );
    horiz.Floatable( false );
    horiz.CloseButton( false );
    horiz.CaptionVisible( false );

    wxAuiPaneInfo vert( horiz );

    vert.TopDockable( false ).BottomDockable( false );
    horiz.LeftDockable( false ).RightDockable( false );

    m_auimgr.AddPane( m_HToolBar,
                      wxAuiPaneInfo( horiz ).Name( wxT( "m_HToolBar" ) ).Top().
                      Row( 0 ) );

    m_auimgr.AddPane( m_VToolBar,
                      wxAuiPaneInfo( vert ).Name( wxT( "m_VToolBar" ) ).Right() );

    m_auimgr.AddPane( DrawPanel,
                      wxAuiPaneInfo().Name( wxT( "DrawFrame" ) ).CentrePane() );

    m_auimgr.AddPane( MsgPanel,
                      wxAuiPaneInfo( horiz ).Name( wxT( "MsgPanel" ) ).Bottom() );
    m_auimgr.Update();
#endif
}


WinEDA_LibeditFrame::~WinEDA_LibeditFrame()
{
    WinEDA_SchematicFrame* frame =
        (WinEDA_SchematicFrame*) wxGetApp().GetTopWindow();

    frame->m_LibeditFrame = NULL;
    m_drawItem = m_lastDrawItem = NULL;
}


/**
 * Load library editor frame specific configuration settings.
 *
 * Don't forget to call this base method from any derived classes or the
 * settings will not get loaded.
 */
void WinEDA_LibeditFrame::LoadSettings()
{
    wxConfig* cfg;

    WinEDA_DrawFrame::LoadSettings();

    wxConfigPathChanger cpc( wxGetApp().m_EDA_Config, m_ConfigPath );
    cfg = wxGetApp().m_EDA_Config;

    m_LastLibExportPath = cfg->Read( lastLibExportPathEntry, ::wxGetCwd() );
    m_LastLibImportPath = cfg->Read( lastLibImportPathEntry, ::wxGetCwd() );
}


void WinEDA_LibeditFrame::SetDrawItem( LIB_DRAW_ITEM* drawItem )
{
    m_drawItem = drawItem;
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

    BOOST_FOREACH( const CMP_LIBRARY &lib, CMP_LIBRARY::GetLibraryList() ) {
        if( lib.IsModified() )
        {
            wxString msg;
            msg.Printf( _( "Library \"%s\" was modified!\nDiscard changes?" ),
                        GetChars( lib.GetName() ) );
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
    wxSize   size;
    EDA_Rect BoundaryBox;

    if( m_component )
    {
        BoundaryBox = m_component->GetBoundaryBox( m_unit, m_convert );
        dx = BoundaryBox.GetWidth();
        dy = BoundaryBox.GetHeight();
        GetScreen()->m_Curseur = BoundaryBox.Centre();
    }
    else
    {
        dx = GetScreen()->m_CurrentSheetDesc->m_Size.x;
        dy = GetScreen()->m_CurrentSheetDesc->m_Size.y;
        GetScreen()->m_Curseur = wxPoint( 0, 0 );
    }

    /*
     * This fixes a bug where the client size of the drawing area is not
     * correctly reported until after the window is shown.  This is most
     * likely due to the unmanaged windows ( vertical tool bars and message
     * panel ) that are drawn in the main window which wxWidgets knows
     * nothing about.  When the library editor is reopened with a component
     * already loading, the zoom will be calculated correctly.
     */
    if( !IsShownOnScreen() )
    {
        if( m_clientSize != wxSize( -1, -1 ) )
            size = m_clientSize;
        else
            size = DrawPanel->GetClientSize();
    }
    else
    {
        if( m_clientSize == wxSize( -1, -1 ) )
            m_clientSize = DrawPanel->GetClientSize();
        size = m_clientSize;
    }

    size -= wxSize( 25, 25 );   // reserve 100 mils margin
    ii    = wxRound( ( (double) dx / (double) size.x ) *
                     (double) GetScreen()->m_ZoomScalar );
    jj = wxRound( ( (double) dy / (double) size.y ) *
                  (double) GetScreen()->m_ZoomScalar );

    return MAX( ii + 1, jj + 1 );
}


void WinEDA_LibeditFrame::UpdateAliasSelectList()
{
    if( m_SelAliasBox == NULL )
        return;

    m_SelAliasBox->Clear();

    if( m_component == NULL )
        return;

    m_SelAliasBox->Append( m_component->GetName() );
    m_SelAliasBox->SetSelection( 0 );

    if( !m_component->m_AliasList.IsEmpty() )
    {
        m_SelAliasBox->Append( m_component->m_AliasList );

        int index = m_SelAliasBox->FindString( m_aliasName );

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

    if( m_component == NULL || m_component->GetPartCount() <= 1 )
    {
        m_SelpartBox->Append( wxEmptyString );
    }
    else
    {
        for( int i = 0; i < m_component->GetPartCount(); i++ )
        {
            wxString msg;
            msg.Printf( _( "Part %c" ), 'A' + i );
            m_SelpartBox->Append( msg );
        }
    }

    m_SelpartBox->SetSelection( ( m_unit > 0 ) ? m_unit - 1 : 0 );
}


void WinEDA_LibeditFrame::OnUpdateEditingPart( wxUpdateUIEvent& event )
{
    event.Enable( m_component != NULL );
}


void WinEDA_LibeditFrame::OnUpdateNotEditingPart( wxUpdateUIEvent& event )
{
    event.Enable( m_component == NULL );
}


void WinEDA_LibeditFrame::OnUpdateUndo( wxUpdateUIEvent& event )
{
    event.Enable( m_component != NULL && GetScreen() != NULL
                  && GetScreen()->GetUndoCommandCount() != 0 );
}


void WinEDA_LibeditFrame::OnUpdateRedo( wxUpdateUIEvent& event )
{
    event.Enable( m_component != NULL && GetScreen() != NULL
                  && GetScreen()->GetRedoCommandCount() != 0 );
}


void WinEDA_LibeditFrame::OnUpdateSaveCurrentLib( wxUpdateUIEvent& event )
{
    event.Enable( m_library != NULL
                  && ( m_library->IsModified()|| GetScreen()->IsModify() ) );
}


void WinEDA_LibeditFrame::OnUpdateViewDoc( wxUpdateUIEvent& event )
{
    bool enable = false;

    if( m_component != NULL && m_library != NULL )
    {
        if( !m_aliasName.IsEmpty() )
        {
            CMP_LIB_ENTRY* entry = m_library->FindEntry( m_aliasName );

            if( entry != NULL )
                enable = !entry->GetDocFileName().IsEmpty();
        }
        else if( !m_component->GetDocFileName().IsEmpty() )
        {
            enable = true;
        }
    }

    event.Enable( enable );
}


void WinEDA_LibeditFrame::OnUpdatePinByPin( wxUpdateUIEvent& event )
{
    event.Enable( ( m_component != NULL )
                  && ( ( m_component->GetPartCount() > 1 ) || m_showDeMorgan ) );

    if( m_HToolBar )
        m_HToolBar->ToggleTool( event.GetId(), g_EditPinByPinIsOn );
}


void WinEDA_LibeditFrame::OnUpdatePartNumber( wxUpdateUIEvent& event )
{
    if( m_SelpartBox == NULL )
        return;

    /* Using the typical event.Enable() call doesn't seem to work with wxGTK
     * so use the pointer to alias combobox to directly enable or disable.
     */
    m_SelpartBox->Enable( m_component && m_component->GetPartCount() > 1 );
}


void WinEDA_LibeditFrame::OnUpdateDeMorganNormal( wxUpdateUIEvent& event )
{
    if( m_HToolBar == NULL )
        return;

    event.Enable( GetShowDeMorgan()
                  || ( m_component && m_component->HasConversion() ) );
    m_HToolBar->ToggleTool( event.GetId(), m_convert <= 1 );
}


void WinEDA_LibeditFrame::OnUpdateDeMorganConvert( wxUpdateUIEvent& event )
{
    if( m_HToolBar == NULL )
        return;

    event.Enable( GetShowDeMorgan()
                  || ( m_component && m_component->HasConversion() ) );
    m_HToolBar->ToggleTool( event.GetId(), m_convert > 1 );
}


void WinEDA_LibeditFrame::OnUpdateSelectAlias( wxUpdateUIEvent& event )
{
    if( m_SelAliasBox == NULL )
        return;

    /* Using the typical event.Enable() call doesn't seem to work with wxGTK
     * so use the pointer to alias combobox to directly enable or disable.
     */
    m_SelAliasBox->Enable( m_component != NULL
                          && !m_component->m_AliasList.IsEmpty() );
}


void WinEDA_LibeditFrame::OnSelectAlias( wxCommandEvent& event )
{
    if( m_SelAliasBox == NULL
        || m_SelAliasBox->GetStringSelection().CmpNoCase( m_aliasName ) == 0 )
        return;

    m_lastDrawItem = NULL;

    if( m_SelAliasBox->GetStringSelection().CmpNoCase( m_component->GetName() )
        == 0 )
        m_aliasName.Empty();
    else
        m_aliasName = m_SelAliasBox->GetStringSelection();

    DisplayCmpDoc();
    DrawPanel->Refresh();
}


void WinEDA_LibeditFrame::OnSelectPart( wxCommandEvent& event )
{
    int i = event.GetSelection();

    if( ( i == wxNOT_FOUND ) || ( ( i + 1 ) == m_unit ) )
        return;

    m_lastDrawItem = NULL;
    m_unit = i + 1;
    DrawPanel->Refresh();
    DisplayCmpDoc();
}


void WinEDA_LibeditFrame::OnViewEntryDoc( wxCommandEvent& event )
{
    if( m_component == NULL )
        return;

    wxString fileName;

    if( !m_aliasName.IsEmpty() )
    {
        CMP_LIB_ENTRY* entry =
            m_library->FindEntry( m_aliasName );

        if( entry != NULL )
            fileName = entry->GetDocFileName();
    }
    else
    {
        fileName = m_component->GetDocFileName();
    }

    if( !fileName.IsEmpty() )
        GetAssociatedDocument( this, fileName,
                               &wxGetApp().GetLibraryPathList() );
}


void WinEDA_LibeditFrame::OnSelectBodyStyle( wxCommandEvent& event )
{
    DrawPanel->UnManageCursor( 0, wxCURSOR_ARROW );

    if( event.GetId() == ID_DE_MORGAN_NORMAL_BUTT )
        m_convert = 1;
    else
        m_convert = 2;

    m_lastDrawItem = NULL;
    DrawPanel->Refresh();
}


void WinEDA_LibeditFrame::Process_Special_Functions( wxCommandEvent& event )
{
    int        id = event.GetId();
    wxPoint    pos;

    DrawPanel->m_IgnoreMouseEvents = true;

    wxGetMousePosition( &pos.x, &pos.y );
    pos.y += 20;

    switch( id )   // Stop placement commands before handling new command.
    {
    case ID_POPUP_LIBEDIT_END_CREATE_ITEM:
    case ID_LIBEDIT_EDIT_PIN:
    case ID_POPUP_LIBEDIT_BODY_EDIT_ITEM:
    case ID_POPUP_LIBEDIT_FIELD_ROTATE_ITEM:
    case ID_POPUP_LIBEDIT_FIELD_EDIT_ITEM:
    case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINSIZE_ITEM:
    case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNAMESIZE_ITEM:
    case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNUMSIZE_ITEM:
    case ID_POPUP_ZOOM_BLOCK:
    case ID_POPUP_DELETE_BLOCK:
    case ID_POPUP_COPY_BLOCK:
    case ID_POPUP_SELECT_ITEMS_BLOCK:
    case ID_POPUP_MIRROR_Y_BLOCK:
    case ID_POPUP_PLACE_BLOCK:
    case ID_POPUP_LIBEDIT_DELETE_CURRENT_POLY_SEGMENT:
    case ID_POPUP_LIBEDIT_ROTATE_GRAPHIC_TEXT:
        break;

    case ID_POPUP_LIBEDIT_CANCEL_EDITING:
        if( DrawPanel->ManageCurseur && DrawPanel->ForceCloseManageCurseur )
            DrawPanel->UnManageCursor( );
        else
            DrawPanel->UnManageCursor( 0, wxCURSOR_ARROW );
        break;

    case ID_POPUP_LIBEDIT_DELETE_ITEM:
        DrawPanel->UnManageCursor( );
        break;

    default:
        DrawPanel->UnManageCursor( 0, wxCURSOR_ARROW, wxEmptyString );
        break;
    }

    INSTALL_DC( dc, DrawPanel );
    switch( id )
    {
    case ID_LIBEDIT_SELECT_CURRENT_LIB:
        SelectActiveLibrary();
        break;

    case ID_LIBEDIT_SAVE_CURRENT_PART:
        SaveOnePartInMemory();
        break;

    case ID_LIBEDIT_EDIT_PIN_BY_PIN:
        g_EditPinByPinIsOn = g_EditPinByPinIsOn ? false : true;
        break;

    case ID_LIBEDIT_PIN_BUTT:
        if( m_component )
        {
            SetToolID( id, wxCURSOR_PENCIL, _( "Add pin" ) );
        }
        else
        {
            SetToolID( id, wxCURSOR_ARROW, _( "Set pin options" ) );
            wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );
            cmd.SetId( ID_LIBEDIT_EDIT_PIN );
            GetEventHandler()->ProcessEvent( cmd );
            SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
        }
        break;

    case ID_NO_SELECT_BUTT:
        SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
        break;

    case ID_LIBEDIT_BODY_TEXT_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add text" ) );
        break;

    case ID_LIBEDIT_BODY_RECT_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add rectangle" ) );
        break;

    case ID_LIBEDIT_BODY_CIRCLE_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add circle" ) );
        break;

    case ID_LIBEDIT_BODY_ARC_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add arc" ) );
        break;

    case ID_LIBEDIT_BODY_LINE_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add line" ) );
        break;

    case ID_LIBEDIT_ANCHOR_ITEM_BUTT:
        SetToolID( id, wxCURSOR_HAND, _( "Set anchor position" ) );
        break;

    case ID_LIBEDIT_IMPORT_BODY_BUTT:
        SetToolID( id, wxCURSOR_ARROW, _( "Import" ) );
        LoadOneSymbol();
        SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
        break;

    case ID_LIBEDIT_EXPORT_BODY_BUTT:
        SetToolID( id, wxCURSOR_ARROW, _( "Export" ) );
        SaveOneSymbol();
        SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
        break;

    case ID_POPUP_LIBEDIT_END_CREATE_ITEM:
        DrawPanel->MouseToCursorSchema();
        if( m_drawItem )
        {
            EndDrawGraphicItem( &dc );
        }
        break;

    case ID_POPUP_LIBEDIT_BODY_EDIT_ITEM:
        if( m_drawItem )
        {
            DrawPanel->CursorOff( &dc );

            switch( m_drawItem->Type() )
            {
            case COMPONENT_ARC_DRAW_TYPE:
            case COMPONENT_CIRCLE_DRAW_TYPE:
            case COMPONENT_RECT_DRAW_TYPE:
            case COMPONENT_POLYLINE_DRAW_TYPE:
            case COMPONENT_LINE_DRAW_TYPE:
                EditGraphicSymbol( &dc, m_drawItem );
                break;

            case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
                EditSymbolText( &dc, m_drawItem );
                break;

            default:
                ;
            }

            DrawPanel->CursorOn( &dc );
        }
        break;


    case ID_LIBEDIT_DELETE_ITEM_BUTT:
        if( m_component == NULL )
        {
            wxBell();
            break;
        }
        SetToolID( id, wxCURSOR_BULLSEYE, _( "Delete item" ) );
        break;


    case ID_POPUP_LIBEDIT_DELETE_CURRENT_POLY_SEGMENT:

        // Delete the last created segment, while creating a polyline draw item
        if( m_drawItem == NULL )
            break;
        DrawPanel->MouseToCursorSchema();
        DeleteDrawPoly( &dc );
        break;

    case ID_POPUP_LIBEDIT_DELETE_ITEM:
        if( m_drawItem == NULL )
            break;
        DrawPanel->MouseToCursorSchema();
        DrawPanel->CursorOff( &dc );
        SaveCopyInUndoList( m_component );
        if( m_drawItem->Type() == COMPONENT_PIN_DRAW_TYPE )
        {
            DeletePin( &dc, m_component, (LIB_PIN*) m_drawItem );
        }
        else
        {
            if( DrawPanel->ManageCurseur && DrawPanel->ForceCloseManageCurseur )
                DrawPanel->ForceCloseManageCurseur( DrawPanel, &dc );
            else
                m_component->RemoveDrawItem( m_drawItem, DrawPanel, &dc );
        }

        m_drawItem = NULL;
        GetScreen()->SetModify();
        DrawPanel->CursorOn( &dc );
        break;

    case ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST:
        if( m_drawItem == NULL )
            break;
        DrawPanel->MouseToCursorSchema();
        if( m_drawItem->Type() == COMPONENT_PIN_DRAW_TYPE )
            StartMovePin( &dc );
        else if( m_drawItem->Type() == COMPONENT_FIELD_DRAW_TYPE )
            StartMoveField( &dc, (LIB_FIELD*) m_drawItem );
        else
            StartMoveDrawSymbol( &dc );
        break;

    case ID_POPUP_LIBEDIT_ROTATE_GRAPHIC_TEXT:
        if( m_drawItem == NULL )
            break;
        DrawPanel->CursorOff( &dc );
        DrawPanel->MouseToCursorSchema();
        if( (m_drawItem->m_Flags & IS_NEW) == 0 )
            SaveCopyInUndoList( m_component );
        RotateSymbolText( &dc );
        DrawPanel->CursorOn( &dc );
        break;

    case ID_POPUP_LIBEDIT_FIELD_ROTATE_ITEM:
        if( m_drawItem == NULL )
            break;
        DrawPanel->CursorOff( &dc );
        DrawPanel->MouseToCursorSchema();
        if( m_drawItem->Type() == COMPONENT_FIELD_DRAW_TYPE )
        {
            SaveCopyInUndoList( m_component );
            RotateField( &dc, (LIB_FIELD*) m_drawItem );
        }
        DrawPanel->CursorOn( &dc );
        break;

    case ID_POPUP_LIBEDIT_FIELD_EDIT_ITEM:
        if( m_drawItem == NULL )
            break;
        DrawPanel->CursorOff( &dc );
        if( m_drawItem->Type() == COMPONENT_FIELD_DRAW_TYPE )
        {
            EditField( &dc, (LIB_FIELD*) m_drawItem );
        }
        DrawPanel->MouseToCursorSchema();
        DrawPanel->CursorOn( &dc );
        break;

    case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINSIZE_ITEM:
    case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNAMESIZE_ITEM:
    case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNUMSIZE_ITEM:
        if( (m_drawItem == NULL )
           || (m_drawItem->Type() != COMPONENT_PIN_DRAW_TYPE) )
            break;
        SaveCopyInUndoList( m_component );
        GlobalSetPins( &dc, (LIB_PIN*) m_drawItem, id );
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
        m_lastDrawItem = NULL;
}

/** Called on activate the frame.
 * Test if the current library exists
 * the library list can be changed by the schematic editor after reloading a new schematic
 * and the current m_library can point a non existent lib.
 */
void WinEDA_LibeditFrame::OnActivate( wxActivateEvent& event )
{
    WinEDA_DrawFrame::OnActivate( event );

    // Verify the existence of the current active library
    // (can be removed or changed by the schematic editor)
    EnsureActiveLibExists();
}

void WinEDA_LibeditFrame::EnsureActiveLibExists()
{
    if( m_library == NULL )
        return;

    bool exists = CMP_LIBRARY::LibraryExists( m_library );
    if ( exists )
        return;
    else
        m_library = NULL;
}
