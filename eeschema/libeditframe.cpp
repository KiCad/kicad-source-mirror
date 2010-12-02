/**
*  EESchema - libeditframe.cpp
* class LIB_EDIT_FRAME: the component editor frame
*
*/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "macros.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "eda_doc.h"
#include "bitmaps.h"
#include "gr_basic.h"
#include "class_sch_screen.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "protos.h"
#include "eeschema_id.h"
#include "libeditframe.h"
#include "class_library.h"
#include "lib_polyline.h"

#include "kicad_device_context.h"
#include "hotkeys.h"

#include "dialogs/dialog_lib_edit_text.h"
#include "dialogs/dialog_SVG_print.h"
#include "dialogs/dialog_edit_component_in_lib.h"
#include "dialogs/dialog_libedit_dimensions.h"

#include "dialog_helpers.h"

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
LIB_COMPONENT* LIB_EDIT_FRAME::m_component = NULL;
CMP_LIBRARY* LIB_EDIT_FRAME::  m_library   = NULL;

wxString LIB_EDIT_FRAME::      m_aliasName;
int LIB_EDIT_FRAME::           m_unit    = 1;
int LIB_EDIT_FRAME::           m_convert = 1;
LIB_DRAW_ITEM* LIB_EDIT_FRAME::m_lastDrawItem    = NULL;
LIB_DRAW_ITEM* LIB_EDIT_FRAME::m_drawItem        = NULL;
bool LIB_EDIT_FRAME::          m_showDeMorgan    = false;
wxSize LIB_EDIT_FRAME::        m_clientSize      = wxSize( -1, -1 );
int LIB_EDIT_FRAME::           m_textSize        = DEFAULT_SIZE_TEXT;
int LIB_EDIT_FRAME::           m_textOrientation = TEXT_ORIENT_HORIZ;
int LIB_EDIT_FRAME::           m_drawLineWidth   = 0;
FILL_T LIB_EDIT_FRAME::        m_drawFillStyle   = NO_FILL;


/************************/
/* class LIB_EDIT_FRAME */
/************************/
BEGIN_EVENT_TABLE( LIB_EDIT_FRAME, WinEDA_DrawFrame )
    EVT_CLOSE( LIB_EDIT_FRAME::OnCloseWindow )
    EVT_SIZE( LIB_EDIT_FRAME::OnSize )
    EVT_ACTIVATE( LIB_EDIT_FRAME::OnActivate )

    /* Main horizontal toolbar. */
    EVT_TOOL_RANGE( ID_ZOOM_IN, ID_ZOOM_PAGE, LIB_EDIT_FRAME::OnZoom )
    EVT_TOOL( ID_LIBEDIT_SAVE_CURRENT_LIB, LIB_EDIT_FRAME::SaveActiveLibrary )
    EVT_TOOL( ID_LIBEDIT_SELECT_CURRENT_LIB, LIB_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_LIBEDIT_DELETE_PART, LIB_EDIT_FRAME::DeleteOnePart )
    EVT_TOOL( ID_LIBEDIT_NEW_PART, LIB_EDIT_FRAME::CreateNewLibraryPart )
    EVT_TOOL( ID_LIBEDIT_NEW_PART_FROM_EXISTING, LIB_EDIT_FRAME::OnCreateNewPartFromExisting )

    EVT_TOOL( ID_LIBEDIT_SELECT_PART, LIB_EDIT_FRAME::LoadOneLibraryPart )
    EVT_TOOL( ID_LIBEDIT_SAVE_CURRENT_PART, LIB_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( wxID_UNDO, LIB_EDIT_FRAME::GetComponentFromUndoList )
    EVT_TOOL( wxID_REDO, LIB_EDIT_FRAME::GetComponentFromRedoList )
    EVT_TOOL( ID_LIBEDIT_GET_FRAME_EDIT_PART, LIB_EDIT_FRAME::OnEditComponentProperties )
    EVT_TOOL( ID_LIBEDIT_GET_FRAME_EDIT_FIELDS, LIB_EDIT_FRAME::InstallFieldsEditorDialog )
    EVT_TOOL( ID_LIBEDIT_CHECK_PART, LIB_EDIT_FRAME::OnCheckComponent )
    EVT_TOOL( ID_DE_MORGAN_NORMAL_BUTT, LIB_EDIT_FRAME::OnSelectBodyStyle )
    EVT_TOOL( ID_DE_MORGAN_CONVERT_BUTT, LIB_EDIT_FRAME::OnSelectBodyStyle )
    EVT_TOOL( ID_LIBEDIT_VIEW_DOC, LIB_EDIT_FRAME::OnViewEntryDoc )
    EVT_TOOL( ID_LIBEDIT_EDIT_PIN_BY_PIN, LIB_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ExportPartId, LIB_EDIT_FRAME::OnExportPart )
    EVT_TOOL( CreateNewLibAndSavePartId, LIB_EDIT_FRAME::OnExportPart )
    EVT_TOOL( ImportPartId, LIB_EDIT_FRAME::OnImportPart )

    EVT_KICAD_CHOICEBOX( ID_LIBEDIT_SELECT_PART_NUMBER, LIB_EDIT_FRAME::OnSelectPart )
    EVT_KICAD_CHOICEBOX( ID_LIBEDIT_SELECT_ALIAS, LIB_EDIT_FRAME::OnSelectAlias )

    /* Right vertical toolbar. */
    EVT_TOOL( ID_NO_SELECT_BUTT, LIB_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL_RANGE( ID_LIBEDIT_PIN_BUTT, ID_LIBEDIT_EXPORT_BODY_BUTT,
                    LIB_EDIT_FRAME::Process_Special_Functions )

    /* menubar commands */
    EVT_MENU( wxID_EXIT, LIB_EDIT_FRAME::CloseWindow )
    EVT_MENU( ID_LIBEDIT_SAVE_CURRENT_LIB_AS, LIB_EDIT_FRAME::SaveActiveLibrary )
    EVT_MENU( ID_LIBEDIT_GEN_PNG_FILE, LIB_EDIT_FRAME::OnPlotCurrentComponent )
    EVT_MENU( ID_LIBEDIT_GEN_SVG_FILE, LIB_EDIT_FRAME::OnPlotCurrentComponent )
    EVT_MENU( ID_GENERAL_HELP, WinEDA_DrawFrame::GetKicadHelp )

    EVT_MENU( ID_COLORS_SETUP, LIB_EDIT_FRAME::OnColorConfig )
    EVT_MENU( ID_CONFIG_REQ, LIB_EDIT_FRAME::InstallConfigFrame )
    EVT_MENU( ID_CONFIG_SAVE, LIB_EDIT_FRAME::Process_Config )
    EVT_MENU( ID_CONFIG_READ, LIB_EDIT_FRAME::Process_Config )
    EVT_MENU( ID_COLORS_SETUP, LIB_EDIT_FRAME::Process_Config )
    EVT_MENU( ID_LIBEDIT_DIMENSIONS, LIB_EDIT_FRAME::InstallDimensionsDialog )

    EVT_MENU_RANGE( ID_PREFERENCES_HOTKEY_START,
                    ID_PREFERENCES_HOTKEY_END,
                    LIB_EDIT_FRAME::Process_Config )

    EVT_MENU_RANGE( ID_LANGUAGE_CHOICE, ID_LANGUAGE_CHOICE_END, LIB_EDIT_FRAME::SetLanguage )

  /* Context menu events and commands. */
    EVT_MENU( ID_LIBEDIT_EDIT_PIN, LIB_EDIT_FRAME::OnEditPin )
    EVT_MENU( ID_LIBEDIT_ROTATE_PIN, LIB_EDIT_FRAME::OnRotatePin )

    EVT_MENU_RANGE( ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_ITEM,
                    ID_POPUP_LIBEDIT_ROTATE_GRAPHIC_TEXT,
                    LIB_EDIT_FRAME::Process_Special_Functions )

    EVT_MENU_RANGE( ID_POPUP_GENERAL_START_RANGE, ID_POPUP_GENERAL_END_RANGE,
                    LIB_EDIT_FRAME::Process_Special_Functions )

   /* Update user interface elements. */
    EVT_UPDATE_UI( ExportPartId, LIB_EDIT_FRAME::OnUpdateEditingPart )
    EVT_UPDATE_UI( CreateNewLibAndSavePartId, LIB_EDIT_FRAME::OnUpdateEditingPart )
    EVT_UPDATE_UI( ID_LIBEDIT_SAVE_CURRENT_PART, LIB_EDIT_FRAME::OnUpdateEditingPart )
    EVT_UPDATE_UI( ID_LIBEDIT_GET_FRAME_EDIT_FIELDS, LIB_EDIT_FRAME::OnUpdateEditingPart )
    EVT_UPDATE_UI( ID_LIBEDIT_CHECK_PART, LIB_EDIT_FRAME::OnUpdateEditingPart )
    EVT_UPDATE_UI( ID_LIBEDIT_GET_FRAME_EDIT_PART, LIB_EDIT_FRAME::OnUpdateEditingPart )
    EVT_UPDATE_UI( ID_LIBEDIT_NEW_PART_FROM_EXISTING, LIB_EDIT_FRAME::OnUpdateEditingPart )
    EVT_UPDATE_UI( wxID_UNDO, LIB_EDIT_FRAME::OnUpdateUndo )
    EVT_UPDATE_UI( wxID_REDO, LIB_EDIT_FRAME::OnUpdateRedo )
    EVT_UPDATE_UI( ID_LIBEDIT_SAVE_CURRENT_LIB, LIB_EDIT_FRAME::OnUpdateSaveCurrentLib )
    EVT_UPDATE_UI( ID_LIBEDIT_VIEW_DOC, LIB_EDIT_FRAME::OnUpdateViewDoc )
    EVT_UPDATE_UI( ID_LIBEDIT_EDIT_PIN_BY_PIN, LIB_EDIT_FRAME::OnUpdatePinByPin )
    EVT_UPDATE_UI( ID_LIBEDIT_SELECT_PART_NUMBER, LIB_EDIT_FRAME::OnUpdatePartNumber )
    EVT_UPDATE_UI( ID_LIBEDIT_SELECT_ALIAS, LIB_EDIT_FRAME::OnUpdateSelectAlias )
    EVT_UPDATE_UI( ID_DE_MORGAN_NORMAL_BUTT, LIB_EDIT_FRAME::OnUpdateDeMorganNormal )
    EVT_UPDATE_UI( ID_DE_MORGAN_CONVERT_BUTT, LIB_EDIT_FRAME::OnUpdateDeMorganConvert )
    EVT_UPDATE_UI_RANGE( ID_LIBEDIT_PIN_BUTT, ID_LIBEDIT_EXPORT_BODY_BUTT,
                         LIB_EDIT_FRAME::OnUpdateEditingPart )
END_EVENT_TABLE()


LIB_EDIT_FRAME::LIB_EDIT_FRAME( WinEDA_SchematicFrame* aParent,
                                const wxString& title,
                                const wxPoint&  pos,
                                const wxSize&   size,
                                long            style ) :
    WinEDA_DrawFrame( aParent, LIBEDITOR_FRAME, title, pos, size, style )
{
    wxASSERT( aParent );

    m_FrameName  = wxT( "LibeditFrame" );
    m_Draw_Axis  = true;            // true to draw axis
    m_ConfigPath = wxT( "LibraryEditor" );
    SetShowDeMorgan( false );
    m_drawSpecificConvert = true;
    m_drawSpecificUnit    = false;
    m_tempCopyComponent   = NULL;
    m_HotkeysZoomAndGridList = s_Libedit_Hokeys_Descr;

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
    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateVToolbar();
    DisplayLibInfos();
    DisplayCmpDoc();
    UpdateAliasSelectList();
    UpdatePartSelectList();

#ifdef USE_WX_GRAPHICS_CONTEXT
    GetScreen()->SetZoom( BestZoom() );
#else
    Zoom_Automatique( false );
#endif

    Show( true );

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
                      wxAuiPaneInfo( horiz ).Name( wxT( "m_HToolBar" ) ).Top().Row( 0 ) );

    m_auimgr.AddPane( m_VToolBar,
                      wxAuiPaneInfo( vert ).Name( wxT( "m_VToolBar" ) ).Right() );

    m_auimgr.AddPane( DrawPanel,
                      wxAuiPaneInfo().Name( wxT( "DrawFrame" ) ).CentrePane() );

    m_auimgr.AddPane( MsgPanel,
                      wxAuiPaneInfo( horiz ).Name( wxT( "MsgPanel" ) ).Bottom() );
    m_auimgr.Update();
}


LIB_EDIT_FRAME::~LIB_EDIT_FRAME()
{
    WinEDA_SchematicFrame* frame = (WinEDA_SchematicFrame*) wxGetApp().GetTopWindow();

    frame->m_LibeditFrame = NULL;
    m_drawItem = m_lastDrawItem = NULL;
    if ( m_tempCopyComponent )
        delete m_tempCopyComponent;
    m_tempCopyComponent = NULL;
}


/**
 * Load library editor frame specific configuration settings.
 *
 * Don't forget to call this base method from any derived classes or the
 * settings will not get loaded.
 */
void LIB_EDIT_FRAME::LoadSettings()
{
    wxConfig* cfg;

    WinEDA_DrawFrame::LoadSettings();

    wxConfigPathChanger cpc( wxGetApp().m_EDA_Config, m_ConfigPath );
    cfg = wxGetApp().m_EDA_Config;

    m_LastLibExportPath = cfg->Read( lastLibExportPathEntry, ::wxGetCwd() );
    m_LastLibImportPath = cfg->Read( lastLibImportPathEntry, ::wxGetCwd() );
}


void LIB_EDIT_FRAME::SetDrawItem( LIB_DRAW_ITEM* drawItem )
{
    m_drawItem = drawItem;
}


/**
 * Save library editor frame specific configuration settings.
 *
 * Don't forget to call this base method from any derived classes or the
 * settings will not get saved.
 */
void LIB_EDIT_FRAME::SaveSettings()
{
    wxConfig* cfg;

    WinEDA_DrawFrame::SaveSettings();

    wxConfigPathChanger cpc( wxGetApp().m_EDA_Config, m_ConfigPath );
    cfg = wxGetApp().m_EDA_Config;

    cfg->Write( lastLibExportPathEntry, m_LastLibExportPath );
    cfg->Write( lastLibImportPathEntry, m_LastLibImportPath );
}


void LIB_EDIT_FRAME::OnCloseWindow( wxCloseEvent& Event )
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

    BOOST_FOREACH( const CMP_LIBRARY &lib, CMP_LIBRARY::GetLibraryList() )
    {
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


int LIB_EDIT_FRAME::BestZoom()
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
    ii = wxRound( ( (double) dx / (double) size.x ) * (double) GetScreen()->m_ZoomScalar );
    jj = wxRound( ( (double) dy / (double) size.y ) * (double) GetScreen()->m_ZoomScalar );

    return MAX( ii + 1, jj + 1 );
}


void LIB_EDIT_FRAME::UpdateAliasSelectList()
{
    if( m_SelAliasBox == NULL )
        return;

    m_SelAliasBox->Clear();

    if( m_component == NULL )
        return;

    m_SelAliasBox->Append( m_component->GetAliasNames() );
    m_SelAliasBox->SetSelection( 0 );

    int index = m_SelAliasBox->FindString( m_aliasName );

    if( index != wxNOT_FOUND )
        m_SelAliasBox->SetSelection( index );
}


void LIB_EDIT_FRAME::UpdatePartSelectList()
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


void LIB_EDIT_FRAME::OnUpdateEditingPart( wxUpdateUIEvent& event )
{
    event.Enable( m_component != NULL );
}


void LIB_EDIT_FRAME::OnUpdateNotEditingPart( wxUpdateUIEvent& event )
{
    event.Enable( m_component == NULL );
}


void LIB_EDIT_FRAME::OnUpdateUndo( wxUpdateUIEvent& event )
{
    event.Enable( m_component != NULL && GetScreen() != NULL
                  && GetScreen()->GetUndoCommandCount() != 0 && !IsEditingDrawItem() );
}


void LIB_EDIT_FRAME::OnUpdateRedo( wxUpdateUIEvent& event )
{
    event.Enable( m_component != NULL && GetScreen() != NULL
                  && GetScreen()->GetRedoCommandCount() != 0 && !IsEditingDrawItem() );
}


void LIB_EDIT_FRAME::OnUpdateSaveCurrentLib( wxUpdateUIEvent& event )
{
    event.Enable( m_library != NULL && ( m_library->IsModified() || GetScreen()->IsModify() ) );
}


void LIB_EDIT_FRAME::OnUpdateViewDoc( wxUpdateUIEvent& event )
{
    bool enable = false;

    if( m_component != NULL && m_library != NULL )
    {
        LIB_ALIAS* alias = m_component->GetAlias( m_aliasName );

        wxCHECK_RET( alias != NULL, wxT( "Alias <" ) + m_aliasName + wxT( "> not found." ) );

        enable = !alias->GetDocFileName().IsEmpty();
    }

    event.Enable( enable );
}


void LIB_EDIT_FRAME::OnUpdatePinByPin( wxUpdateUIEvent& event )
{
    event.Enable( ( m_component != NULL )
                 && ( ( m_component->GetPartCount() > 1 ) || m_showDeMorgan ) );

    if( m_HToolBar )
        m_HToolBar->ToggleTool( event.GetId(), g_EditPinByPinIsOn );
}


void LIB_EDIT_FRAME::OnUpdatePartNumber( wxUpdateUIEvent& event )
{
    if( m_SelpartBox == NULL )
        return;

    /* Using the typical event.Enable() call doesn't seem to work with wxGTK
     * so use the pointer to alias combobox to directly enable or disable.
     */
    m_SelpartBox->Enable( m_component && m_component->GetPartCount() > 1 );
}


void LIB_EDIT_FRAME::OnUpdateDeMorganNormal( wxUpdateUIEvent& event )
{
    if( m_HToolBar == NULL )
        return;

    event.Enable( GetShowDeMorgan() || ( m_component && m_component->HasConversion() ) );
    m_HToolBar->ToggleTool( event.GetId(), m_convert <= 1 );
}


void LIB_EDIT_FRAME::OnUpdateDeMorganConvert( wxUpdateUIEvent& event )
{
    if( m_HToolBar == NULL )
        return;

    event.Enable( GetShowDeMorgan() || ( m_component && m_component->HasConversion() ) );
    m_HToolBar->ToggleTool( event.GetId(), m_convert > 1 );
}


void LIB_EDIT_FRAME::OnUpdateSelectAlias( wxUpdateUIEvent& event )
{
    if( m_SelAliasBox == NULL )
        return;

    /* Using the typical event.Enable() call doesn't seem to work with wxGTK
     * so use the pointer to alias combobox to directly enable or disable.
     */
    m_SelAliasBox->Enable( m_component != NULL && m_component->GetAliasCount() > 1 );
}


void LIB_EDIT_FRAME::OnSelectAlias( wxCommandEvent& event )
{
    if( m_SelAliasBox == NULL
        || m_SelAliasBox->GetStringSelection().CmpNoCase( m_aliasName ) == 0 )
        return;

    m_lastDrawItem = NULL;
    m_aliasName = m_SelAliasBox->GetStringSelection();

    DisplayCmpDoc();
    DrawPanel->Refresh();
}


void LIB_EDIT_FRAME::OnSelectPart( wxCommandEvent& event )
{
    int i = event.GetSelection();

    if( ( i == wxNOT_FOUND ) || ( ( i + 1 ) == m_unit ) )
        return;

    m_lastDrawItem = NULL;
    m_unit = i + 1;
    DrawPanel->Refresh();
    DisplayCmpDoc();
}


void LIB_EDIT_FRAME::OnViewEntryDoc( wxCommandEvent& event )
{
    if( m_component == NULL )
        return;

    wxString fileName;
    LIB_ALIAS* alias = m_component->GetAlias( m_aliasName );

    wxCHECK_RET( alias != NULL, wxT( "Alias not found." ) );

    fileName = alias->GetDocFileName();

    if( !fileName.IsEmpty() )
        GetAssociatedDocument( this, fileName, &wxGetApp().GetLibraryPathList() );
}


void LIB_EDIT_FRAME::OnSelectBodyStyle( wxCommandEvent& event )
{
    DrawPanel->UnManageCursor( 0, wxCURSOR_ARROW );

    if( event.GetId() == ID_DE_MORGAN_NORMAL_BUTT )
        m_convert = 1;
    else
        m_convert = 2;

    m_lastDrawItem = NULL;
    DrawPanel->Refresh();
}


void LIB_EDIT_FRAME::Process_Special_Functions( wxCommandEvent& event )
{
    int     id = event.GetId();
    wxPoint pos;

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
            DrawPanel->UnManageCursor();
        else
            DrawPanel->UnManageCursor( 0, wxCURSOR_ARROW );
        break;

    case ID_POPUP_LIBEDIT_DELETE_ITEM:
        DrawPanel->UnManageCursor();
        break;

    default:
        DrawPanel->UnManageCursor( 0, wxCURSOR_ARROW, wxEmptyString );
        break;
    }

    INSTALL_DC( dc, DrawPanel );

    switch( id )
    {
    case ID_POPUP_LIBEDIT_CANCEL_EDITING:
        break;

    case ID_LIBEDIT_SELECT_CURRENT_LIB:
        SelectActiveLibrary();
        break;

    case ID_LIBEDIT_SAVE_CURRENT_PART:
        SaveOnePartInMemory();
        break;

    case ID_LIBEDIT_EDIT_PIN_BY_PIN:
        g_EditPinByPinIsOn = m_HToolBar->GetToolState(ID_LIBEDIT_EDIT_PIN_BY_PIN);
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
    {
        // Delete the last created segment, while creating a polyline draw item
        if( m_drawItem == NULL )
            break;

        DrawPanel->MouseToCursorSchema();
        int oldFlags = m_drawItem->GetFlags();
        m_drawItem->SetFlags( 0 );
        m_drawItem->Draw( DrawPanel, &dc, wxPoint( 0, 0 ), -1, g_XorMode, NULL, DefaultTransform );
        ( (LIB_POLYLINE*) m_drawItem )->DeleteSegment( GetScreen()->GetCursorDrawPosition() );
        m_drawItem->Draw( DrawPanel, &dc, wxPoint( 0, 0 ), -1, g_XorMode, NULL, DefaultTransform );
        m_drawItem->SetFlags( oldFlags );
        break;
    }

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
        OnModify( );
        DrawPanel->CursorOn( &dc );
        break;

    case ID_POPUP_LIBEDIT_MOVE_ITEM_REQUEST:
        if( m_drawItem == NULL )
            break;
        DrawPanel->MouseToCursorSchema();
        if( m_drawItem->Type() == COMPONENT_PIN_DRAW_TYPE )
            StartMovePin( &dc );
        else
            StartMoveDrawSymbol( &dc );
        break;

    case ID_POPUP_LIBEDIT_MODIFY_ITEM:

        if( m_drawItem == NULL )
            break;

        DrawPanel->MouseToCursorSchema();
        if( m_drawItem->Type() == COMPONENT_RECT_DRAW_TYPE
            || m_drawItem->Type() == COMPONENT_CIRCLE_DRAW_TYPE
            || m_drawItem->Type() == COMPONENT_POLYLINE_DRAW_TYPE
            || m_drawItem->Type() == COMPONENT_ARC_DRAW_TYPE
            )
        {
            StartModifyDrawSymbol( &dc );
        }

        break;

    case ID_POPUP_LIBEDIT_ROTATE_GRAPHIC_TEXT:
        if( m_drawItem == NULL && m_drawItem->Type() != COMPONENT_GRAPHIC_TEXT_DRAW_TYPE )
            break;
        DrawPanel->MouseToCursorSchema();
        if( !m_drawItem->InEditMode() )
        {
            SaveCopyInUndoList( m_component );
            m_drawItem->SetUnit( m_unit );
        }

        m_drawItem->Rotate();
        DrawPanel->Refresh();
        break;

    case ID_POPUP_LIBEDIT_FIELD_ROTATE_ITEM:
    {
        if( m_drawItem == NULL || ( m_drawItem->Type() != COMPONENT_FIELD_DRAW_TYPE ) )
            break;
        DrawPanel->MouseToCursorSchema();

        if( !m_drawItem->InEditMode() )
        {
            SaveCopyInUndoList( m_component );
            m_drawItem->SetUnit( m_unit );
        }

        m_drawItem->Rotate();
        DrawPanel->Refresh();
        break;
    }

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
        DisplayError( this, wxT( "LIB_EDIT_FRAME::Process_Special_Functions error" ) );
        break;
    }

    DrawPanel->m_IgnoreMouseEvents = false;

    if( m_ID_current_state == 0 )
        m_lastDrawItem = NULL;
}


void LIB_EDIT_FRAME::OnActivate( wxActivateEvent& event )
{
    WinEDA_DrawFrame::OnActivate( event );

    // Verify the existence of the current active library
    // (can be removed or changed by the schematic editor)
    EnsureActiveLibExists();
}


void LIB_EDIT_FRAME::EnsureActiveLibExists()
{
    if( m_library == NULL )
        return;

    bool exists = CMP_LIBRARY::LibraryExists( m_library );
    if( exists )
        return;
    else
        m_library = NULL;
}


void LIB_EDIT_FRAME::SetLanguage( wxCommandEvent& event )
{
    WinEDA_BasicFrame::SetLanguage( event );
    WinEDA_SchematicFrame *parent = (WinEDA_SchematicFrame *)GetParent();
    parent->WinEDA_BasicFrame::SetLanguage( event );
}


/**
 * Function TempCopyComponent
 * create a temporary copy of the current edited component
 * Used to prepare an Undo ant/or abort command before editing the component
 */
void LIB_EDIT_FRAME::TempCopyComponent()
{
    if( m_tempCopyComponent )
        delete m_tempCopyComponent;
    m_tempCopyComponent = NULL;
    if( m_component )
        m_tempCopyComponent = new LIB_COMPONENT( *m_component );
}

/**
 * Function RestoreComponent
 * Restore the current edited component from its temporary copy.
 * Used to abort a command
 */
void LIB_EDIT_FRAME::RestoreComponent()
{
    if( m_tempCopyComponent == NULL )
        return;
    if( m_component )
        delete m_component;
    m_component = m_tempCopyComponent;
    m_tempCopyComponent = NULL;
}

/**
 * Function ClearTempCopyComponent
 * delete temporary copy of the current component and clear pointer
 */
void LIB_EDIT_FRAME::ClearTempCopyComponent()
{
    delete m_tempCopyComponent;
    m_tempCopyComponent = NULL;
}


/* Creates the SVG print file for the current edited component.
 */
void LIB_EDIT_FRAME::SVG_Print_Component( const wxString& FullFileName )
{
    DIALOG_SVG_PRINT::DrawSVGPage( this, FullFileName, GetScreen() );
}


void LIB_EDIT_FRAME::EditSymbolText( wxDC* DC, LIB_DRAW_ITEM* DrawItem )
{
    if ( ( DrawItem == NULL ) || ( DrawItem->Type() != COMPONENT_GRAPHIC_TEXT_DRAW_TYPE ) )
        return;

    /* Deleting old text. */
    if( DC && !DrawItem->InEditMode() )
        DrawItem->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, g_XorMode, NULL, DefaultTransform );


    DIALOG_LIB_EDIT_TEXT* frame = new DIALOG_LIB_EDIT_TEXT( this, (LIB_TEXT*) DrawItem );
    frame->ShowModal();
    frame->Destroy();
    OnModify();

    /* Display new text. */
    if( DC && !DrawItem->InEditMode() )
        DrawItem->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, GR_DEFAULT_DRAWMODE, NULL,
                        DefaultTransform );
}


void LIB_EDIT_FRAME::OnEditComponentProperties( wxCommandEvent& event )
{
    bool partLocked = GetComponent()->UnitsLocked();

    DIALOG_EDIT_COMPONENT_IN_LIBRARY dlg( this );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    if( partLocked != GetComponent()->UnitsLocked() )
    {
        // g_EditPinByPinIsOn is set to the better value, if m_UnitSelectionLocked has changed
        g_EditPinByPinIsOn = GetComponent()->UnitsLocked() ? true : false;
    }

    UpdateAliasSelectList();
    UpdatePartSelectList();
    DisplayLibInfos();
    DisplayCmpDoc();
    OnModify();
    DrawPanel->Refresh();
}


void LIB_EDIT_FRAME::InstallDimensionsDialog( wxCommandEvent& event )
{
    DIALOG_LIBEDIT_DIMENSIONS dlg( this );
    dlg.ShowModal();
}


void LIB_EDIT_FRAME::OnCreateNewPartFromExisting( wxCommandEvent& event )
{
    wxCHECK_RET( m_component != NULL,
                 wxT( "Cannot create new part from non-existant current part." ) );

    INSTALL_DC( dc, DrawPanel );
    DrawPanel->CursorOff( &dc );
    EditField( &dc, &m_component->GetValueField() );
    DrawPanel->MouseToCursorSchema();
    DrawPanel->CursorOn( &dc );
}
