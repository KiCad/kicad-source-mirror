/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <pgm_base.h>
#include <kiface_i.h>
#include <bitmaps.h>
#include <macros.h>
#include <id.h>
#include <base_screen.h>
#include <msgpanel.h>
#include <eda_draw_frame.h>
#include <confirm.h>
#include <dialog_helpers.h>
#include <lockfile.h>
#include <trace_helpers.h>
#include <wx/snglinst.h>
#include <view/view.h>
#include <tool/tool_manager.h>
#include <tool/action_manager.h>
#include <tool/tool_dispatcher.h>
#include <tool/actions.h>
#include <ws_draw_item.h>
#include <page_info.h>
#include <title_block.h>
#include <tool/tool_menu.h>
#include <tool/selection_conditions.h>
#include <tool/zoom_menu.h>
#include <tool/grid_menu.h>
#include <tool/common_tools.h>


///@{
/// \ingroup config
static const wxChar FirstRunShownKeyword[] =        wxT( "FirstRunShown" );

static const wxChar FindReplaceFlagsEntry[] =       wxT( "LastFindReplaceFlags" );
static const wxChar FindStringEntry[] =             wxT( "LastFindString" );
static const wxChar ReplaceStringEntry[] =          wxT( "LastReplaceString" );
static const wxChar FindStringHistoryEntry[] =      wxT( "FindStringHistoryList%d" );
static const wxChar ReplaceStringHistoryEntry[] =   wxT( "ReplaceStringHistoryList%d" );
///@}

#define FR_HISTORY_LIST_CNT     10   ///< Maximum size of the find/replace history stacks.

/**
 * Integer to set the maximum number of undo items on the stack. If zero,
 * undo items are unlimited.
 *
 * Present as:
 *
 * - SchematicFrameDevelMaxUndoItems (file: eeschema)
 * - LibeditFrameDevelMaxUndoItems (file: eeschema)
 * - PcbFrameDevelMaxUndoItems (file: pcbnew)
 * - ModEditFrameDevelMaxUndoItems (file: pcbnew)
 *
 * \ingroup develconfig
 */
static const wxString MaxUndoItemsEntry(wxT( "DevelMaxUndoItems" ) );

EDA_DRAW_FRAME::EDA_DRAW_FRAME( KIWAY* aKiway, wxWindow* aParent, FRAME_T aFrameType,
                                const wxString& aTitle, const wxPoint& aPos, const wxSize& aSize,
                                long aStyle, const wxString & aFrameName ) :
    KIWAY_PLAYER( aKiway, aParent, aFrameType, aTitle, aPos, aSize, aStyle, aFrameName )
{
    m_socketServer        = nullptr;
    m_mainToolBar         = NULL;
    m_drawToolBar         = NULL;
    m_optionsToolBar      = NULL;
    m_auxiliaryToolBar    = NULL;
    m_gridSelectBox       = NULL;
    m_zoomSelectBox       = NULL;
    m_firstRunDialogSetting = 0;
    m_UndoRedoCountMax    = DEFAULT_MAX_UNDO_ITEMS;

    m_canvasType          = EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE;
    m_canvas              = NULL;
    m_toolDispatcher      = NULL;
    m_messagePanel        = NULL;
    m_currentScreen       = NULL;
    m_showBorderAndTitleBlock = false;  // true to display reference sheet.
    m_LastGridSizeId      = 0;
    m_drawGrid            = true;       // hide/Show grid. default = show
    m_gridColor           = COLOR4D( DARKGRAY );   // Default grid color
    m_showPageLimits      = false;
    m_drawBgColor         = COLOR4D( BLACK );   // the background color of the draw canvas:
                                                // BLACK for Pcbnew, BLACK or WHITE for eeschema
    m_MsgFrameHeight      = EDA_MSG_PANEL::GetRequiredHeight();
    m_zoomLevelCoeff      = 1.0;
    m_userUnits           = MILLIMETRES;
    m_PolarCoords         = false;
    m_findReplaceData     = new wxFindReplaceData( wxFR_DOWN );

    m_auimgr.SetFlags(wxAUI_MGR_DEFAULT);

    CreateStatusBar( 7 );

    // set the size of the status bar subwindows:

    wxWindow* stsbar = GetStatusBar();

    int dims[] = {

        // remainder of status bar on far left is set to a default or whatever is left over.
        -1,

        // When using GetTextSize() remember the width of character '1' is not the same
        // as the width of '0' unless the font is fixed width, and it usually won't be.

        // zoom:
        GetTextSize( wxT( "Z 762000" ), stsbar ).x + 10,

        // cursor coords
        GetTextSize( wxT( "X 0234.567890  Y 0234.567890" ), stsbar ).x + 10,

        // delta distances
        GetTextSize( wxT( "dx 0234.567890  dx 0234.567890  d 0234.567890" ), stsbar ).x + 10,

        // grid size
        GetTextSize( wxT( "grid X 0234.567890  Y 0234.567890" ), stsbar ).x + 10,

        // units display, Inches is bigger than mm
        GetTextSize( _( "Inches" ), stsbar ).x + 10,

            // Size for the "Current Tool" panel; longest string from SetTool()
        GetTextSize( wxT( "Add layer alignment target" ), stsbar ).x + 10,
    };

    SetStatusWidths( arrayDim( dims ), dims );

    // Create child subwindows.
    GetClientSize( &m_FrameSize.x, &m_FrameSize.y );
    m_FramePos.x   = m_FramePos.y = 0;
    m_FrameSize.y -= m_MsgFrameHeight;

    m_messagePanel  = new EDA_MSG_PANEL( this, -1, wxPoint( 0, m_FrameSize.y ),
                                         wxSize( m_FrameSize.x, m_MsgFrameHeight ) );

    m_messagePanel->SetBackgroundColour( COLOR4D( LIGHTGRAY ).ToColour() );
}


EDA_DRAW_FRAME::~EDA_DRAW_FRAME()
{
    delete m_socketServer;

    for( auto socket : m_sockets )
    {
        socket->Shutdown();
        socket->Destroy();
    }

    saveCanvasTypeSetting( m_canvasType );

    delete m_actions;
    delete m_toolManager;
    delete m_toolDispatcher;
    delete m_canvas;

    delete m_currentScreen;
    m_currentScreen = NULL;

    delete m_findReplaceData;

    m_auimgr.UnInit();

    ReleaseFile();
}


void EDA_DRAW_FRAME::ReleaseFile()
{
    m_file_checker = nullptr;
}


bool EDA_DRAW_FRAME::LockFile( const wxString& aFileName )
{
    m_file_checker = ::LockFile( aFileName );

    return bool( m_file_checker );
}


void EDA_DRAW_FRAME::unitsChangeRefresh()
{
    UpdateStatusBar();
    UpdateMsgPanel();
}


void EDA_DRAW_FRAME::CommonSettingsChanged( bool aEnvVarsChanged )
{
    EDA_BASE_FRAME::CommonSettingsChanged( aEnvVarsChanged );

    wxConfigBase*         settings = Pgm().CommonSettings();
    KIGFX::VIEW_CONTROLS* viewControls = GetCanvas()->GetViewControls();

    int autosaveInterval;
    settings->Read( AUTOSAVE_INTERVAL_KEY, &autosaveInterval );
    SetAutoSaveInterval( autosaveInterval );

    int historySize;
    settings->Read( FILE_HISTORY_SIZE_KEY, &historySize, DEFAULT_FILE_HISTORY_SIZE );
    Kiface().GetFileHistory().SetMaxFiles( (unsigned) std::max( 0, historySize ) );

    bool option;
    settings->Read( ENBL_MOUSEWHEEL_PAN_KEY, &option );
    viewControls->EnableMousewheelPan( option );

    settings->Read( ENBL_ZOOM_NO_CENTER_KEY, &option );
    viewControls->EnableCursorWarping( !option );

    settings->Read( ENBL_AUTO_PAN_KEY, &option );
    viewControls->EnableAutoPan( option );

    m_galDisplayOptions.ReadCommonConfig( *settings, this );
}


void EDA_DRAW_FRAME::EraseMsgBox()
{
    if( m_messagePanel )
        m_messagePanel->EraseMsgBox();
}


void EDA_DRAW_FRAME::OnUpdateSelectGrid( wxUpdateUIEvent& aEvent )
{
    // No need to update the grid select box if it doesn't exist or the grid setting change
    // was made using the select box.
    if( m_gridSelectBox == NULL )
        return;

    int select = wxNOT_FOUND;

    for( size_t i = 0; i < GetScreen()->GetGridCount(); i++ )
    {
        if( GetScreen()->GetGridCmdId() == GetScreen()->GetGrid( i ).m_CmdId )
        {
            select = (int) i;
            break;
        }
    }

    if( select != m_gridSelectBox->GetSelection() )
        m_gridSelectBox->SetSelection( select );
}


void EDA_DRAW_FRAME::PrintPage( wxDC* aDC )
{
    wxMessageBox( wxT("EDA_DRAW_FRAME::PrintPage() error") );
}


/*
 * Respond to selections in the toolbar grid popup
 */
void EDA_DRAW_FRAME::OnSelectGrid( wxCommandEvent& event )
{
    wxCHECK_RET( m_gridSelectBox, "m_gridSelectBox uninitialized" );

    int id = m_gridSelectBox->GetCurrentSelection() + ID_POPUP_GRID_FIRST;

    if( id == ID_POPUP_GRID_SEPARATOR )
    {
        // wxWidgets will check the separator, which we don't want.
        // Re-check the current grid.
        wxUpdateUIEvent dummy;
        OnUpdateSelectGrid( dummy );
    }
    else if( id == ID_POPUP_GRID_SETTINGS )
    {
        // wxWidgets will check the Grid Settings... entry, which we don't want.
        // R-check the current grid.
        wxUpdateUIEvent dummy;
        OnUpdateSelectGrid( dummy );
        // Now run the Grid Settings... dialog
        wxCommandEvent dummy2;
        OnGridSettings( dummy2 );
    }
    else if( id >= ID_POPUP_GRID_FIRST && id < ID_POPUP_GRID_SEPARATOR  )
    {
        m_toolManager->RunAction( ACTIONS::gridPreset, true, id - ID_POPUP_GRID_FIRST );
    }

    UpdateStatusBar();
    m_canvas->Refresh();
}


void EDA_DRAW_FRAME::InitExitKey()
{
    wxAcceleratorEntry entries[1];
    entries[0].Set( wxACCEL_CTRL, int( 'Q' ), wxID_EXIT );
    wxAcceleratorTable accel( 1, entries );
    SetAcceleratorTable( accel );
}


/*
 * Respond to selections in the toolbar zoom popup
 */
void EDA_DRAW_FRAME::OnSelectZoom( wxCommandEvent& event )
{
    wxCHECK_RET( m_zoomSelectBox, "m_zoomSelectBox uninitialized" );

    int id = m_zoomSelectBox->GetCurrentSelection();

    if( id < 0 || !( id < (int)m_zoomSelectBox->GetCount() ) )
        return;

    m_toolManager->RunAction( "common.Control.zoomPreset", true, id );
    UpdateStatusBar();
    m_canvas->Refresh();
}


double EDA_DRAW_FRAME::GetZoom()
{
    return GetScreen()->GetZoom();
}


void EDA_DRAW_FRAME::AddStandardSubMenus( TOOL_MENU& aToolMenu )
{
    COMMON_TOOLS*     commonTools = m_toolManager->GetTool<COMMON_TOOLS>();
    CONDITIONAL_MENU& aMenu = aToolMenu.GetMenu();

    aMenu.AddSeparator( 1000 );
    aMenu.AddItem( ACTIONS::zoomCenter,    SELECTION_CONDITIONS::ShowAlways, 1000 );
    aMenu.AddItem( ACTIONS::zoomIn,        SELECTION_CONDITIONS::ShowAlways, 1000 );
    aMenu.AddItem( ACTIONS::zoomOut,       SELECTION_CONDITIONS::ShowAlways, 1000 );
    aMenu.AddItem( ACTIONS::zoomFitScreen, SELECTION_CONDITIONS::ShowAlways, 1000 );

    aMenu.AddSeparator( 1000 );

    auto zoomMenu = std::make_shared<ZOOM_MENU>( this );
    zoomMenu->SetTool( commonTools );
    aToolMenu.AddSubMenu( zoomMenu );

    auto gridMenu = std::make_shared<GRID_MENU>( this );
    gridMenu->SetTool( commonTools );
    aToolMenu.AddSubMenu( gridMenu );

    aMenu.AddMenu( zoomMenu.get(),   SELECTION_CONDITIONS::ShowAlways, 1000 );
    aMenu.AddMenu( gridMenu.get(), SELECTION_CONDITIONS::ShowAlways, 1000 );
}


void EDA_DRAW_FRAME::DisplayToolMsg( const wxString& msg )
{
    SetStatusText( msg, 6 );
}


/*
 * Display the grid status.
 */
void EDA_DRAW_FRAME::DisplayGridMsg()
{
    wxString line;
    wxString gridformatter;

    switch( m_userUnits )
    {
    case INCHES:
        gridformatter = "grid %.3f";
        break;

    case MILLIMETRES:
        gridformatter = "grid %.4f";
        break;

    default:
        gridformatter = "grid %f";
        break;
    }

    wxRealPoint curr_grid_size = GetScreen()->GetGridSize();
    double grid = To_User_Unit( m_userUnits, curr_grid_size.x );
    line.Printf( gridformatter, grid );

    SetStatusText( line, 4 );
}


void EDA_DRAW_FRAME::DisplayUnitsMsg()
{
    wxString msg;

    switch( m_userUnits )
    {
    case INCHES:      msg = _( "Inches" ); break;
    case MILLIMETRES: msg = _( "mm" );     break;
    default:          msg = _( "Units" );  break;
    }

    SetStatusText( msg, 5 );
}


void EDA_DRAW_FRAME::OnSize( wxSizeEvent& SizeEv )
{
    m_FrameSize = GetClientSize( );

    SizeEv.Skip();
}


void EDA_DRAW_FRAME::UpdateStatusBar()
{
    SetStatusText( GetZoomLevelIndicator(), 1 );

    // Absolute and relative cursor positions are handled by overloading this function and
    // handling the internal to user units conversion at the appropriate level.

    // refresh units display
    DisplayUnitsMsg();
}


const wxString EDA_DRAW_FRAME::GetZoomLevelIndicator() const
{
    // returns a human readable value which can be displayed as zoom
    // level indicator in dialogs.
    return wxString::Format( wxT( "Z %.2f" ), m_canvas->GetGAL()->GetZoomFactor() );
}


void EDA_DRAW_FRAME::LoadSettings( wxConfigBase* aCfg )
{
    EDA_BASE_FRAME::LoadSettings( aCfg );

    wxString baseCfgName = ConfigBaseName();
    wxConfigBase* cmnCfg = Pgm().CommonSettings();

    // Read units used in dialogs and toolbars
    EDA_UNITS_T unitsTmp;

    if( aCfg->Read( baseCfgName + UserUnitsEntryKeyword, (int*) &unitsTmp ) )
        SetUserUnits( unitsTmp );
    else
        SetUserUnits( MILLIMETRES );

    // Read show/hide grid entry
    bool btmp;
    if( aCfg->Read( baseCfgName + ShowGridEntryKeyword, &btmp ) )
        SetGridVisibility( btmp );

    aCfg->Read( baseCfgName + LastGridSizeIdKeyword, &m_LastGridSizeId, m_LastGridSizeId );

    // m_LastGridSizeId is an offset, expected to be >= 0
    if( m_LastGridSizeId < 0 )
        m_LastGridSizeId = 0;

    m_UndoRedoCountMax = aCfg->Read( baseCfgName + MaxUndoItemsEntry,
                                     long( DEFAULT_MAX_UNDO_ITEMS ) );

    aCfg->Read( baseCfgName + FirstRunShownKeyword, &m_firstRunDialogSetting, 0L );

    m_galDisplayOptions.ReadConfig( *cmnCfg, *aCfg, baseCfgName, this );

    long tmp;
    aCfg->Read( FindReplaceFlagsEntry, &tmp, (long) wxFR_DOWN );
    m_findReplaceData->SetFlags( (wxUint32) tmp & ~FR_REPLACE_ITEM_FOUND );
    m_findReplaceData->SetFindString( aCfg->Read( FindStringEntry, wxEmptyString ) );
    m_findReplaceData->SetReplaceString( aCfg->Read( ReplaceStringEntry, wxEmptyString ) );

    // Load the find and replace string history list.
    for( int i = 0; i < FR_HISTORY_LIST_CNT; ++i )
    {
        wxString tmpHistory;
        wxString entry;
        entry.Printf( FindStringHistoryEntry, i );
        tmpHistory = aCfg->Read( entry, wxEmptyString );

        if( !tmpHistory.IsEmpty() )
            m_findStringHistoryList.Add( tmpHistory );

        entry.Printf( ReplaceStringHistoryEntry, i );
        tmpHistory = aCfg->Read( entry, wxEmptyString );

        if( !tmpHistory.IsEmpty() )
            m_replaceStringHistoryList.Add( tmpHistory );
    }
}


void EDA_DRAW_FRAME::SaveSettings( wxConfigBase* aCfg )
{
    EDA_BASE_FRAME::SaveSettings( aCfg );

    wxString baseCfgName = ConfigBaseName();

    aCfg->Write( baseCfgName + UserUnitsEntryKeyword, (int) m_userUnits );
    aCfg->Write( baseCfgName + ShowGridEntryKeyword, IsGridVisible() );
    aCfg->Write( baseCfgName + LastGridSizeIdKeyword, ( long ) m_LastGridSizeId );
    aCfg->Write( baseCfgName + FirstRunShownKeyword, m_firstRunDialogSetting );

    if( GetScreen() )
        aCfg->Write( baseCfgName + MaxUndoItemsEntry, long( GetScreen()->GetMaxUndoItems() ) );

    m_galDisplayOptions.WriteConfig( *aCfg, baseCfgName );

    // Save find dialog session setting.
    aCfg->Write( FindReplaceFlagsEntry, (long) m_findReplaceData->GetFlags() );
    aCfg->Write( FindStringEntry, m_findReplaceData->GetFindString() );
    aCfg->Write( ReplaceStringEntry, m_findReplaceData->GetReplaceString() );

    // Save the find and replace string history list.
    unsigned i;
    wxString tmpHistory;
    wxString entry;     // invoke constructor outside of any loops

    for( i = 0; i < m_findStringHistoryList.GetCount() && i < FR_HISTORY_LIST_CNT; i++ )
    {
        entry.Printf( FindStringHistoryEntry, i );
        aCfg->Write( entry, m_findStringHistoryList[ i ] );
    }

    for( i = 0; i < m_replaceStringHistoryList.GetCount() && i < FR_HISTORY_LIST_CNT; i++ )
    {
        entry.Printf( ReplaceStringHistoryEntry, i );
        aCfg->Write( entry, m_replaceStringHistoryList[ i ] );
    }
}


void EDA_DRAW_FRAME::AppendMsgPanel( const wxString& textUpper, const wxString& textLower,
                                     COLOR4D color, int pad )
{
    if( m_messagePanel )
        m_messagePanel->AppendMessage( textUpper, textLower, color, pad );
}


void EDA_DRAW_FRAME::ClearMsgPanel()
{
    if( m_messagePanel )
        m_messagePanel->EraseMsgBox();
}


void EDA_DRAW_FRAME::SetMsgPanel( const MSG_PANEL_ITEMS& aList )
{
    if( m_messagePanel )
    {
        m_messagePanel->EraseMsgBox();

        for( const MSG_PANEL_ITEM& item : aList )
            m_messagePanel->AppendMessage( item );
    }
}


void EDA_DRAW_FRAME::SetMsgPanel( EDA_ITEM* aItem )
{
    wxCHECK_RET( aItem, wxT( "Invalid EDA_ITEM pointer.  Bad programmer." ) );

    MSG_PANEL_ITEMS items;
    aItem->GetMsgPanelInfo( m_userUnits, items );
    SetMsgPanel( items );
}


void EDA_DRAW_FRAME::UpdateMsgPanel()
{
    GetToolManager()->PostEvent( EVENTS::SelectedItemsModified );
}


void EDA_DRAW_FRAME::ActivateGalCanvas()
{
    GetCanvas()->SetEvtHandlerEnabled( true );
    GetCanvas()->StartDrawing();
}


void EDA_DRAW_FRAME::SwitchCanvas( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType )
{
    GetCanvas()->SwitchBackend( aCanvasType );
    m_canvasType = GetCanvas()->GetBackend();

    ActivateGalCanvas();
}


EDA_DRAW_PANEL_GAL::GAL_TYPE EDA_DRAW_FRAME::LoadCanvasTypeSetting()
{
    EDA_DRAW_PANEL_GAL::GAL_TYPE canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE;
    wxConfigBase* cfg = Kiface().KifaceSettings();

    if( cfg )
    {
        canvasType = (EDA_DRAW_PANEL_GAL::GAL_TYPE)
                          cfg->ReadLong( GetCanvasTypeKey(), EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE );
    }

    if( canvasType < EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE
            || canvasType >= EDA_DRAW_PANEL_GAL::GAL_TYPE_LAST )
    {
        wxASSERT( false );
        canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE;
    }

    // Legacy canvas no longer supported.  Switch to Cairo, and on the first instantiation
    // the user will be prompted to switch to OpenGL
    if( canvasType == EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE )
    {
#ifdef __WXMAC__
        // Cairo renderer doesn't handle Retina displays
        canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL;
#else
        canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO;
#endif
    }

    return canvasType;
}


bool EDA_DRAW_FRAME::saveCanvasTypeSetting( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType )
{
    // Not all classes derived from EDA_DRAW_FRAME can save the canvas type, because some
    // have a fixed type, or do not have a option to set the canvas type (they inherit from
    // a parent frame)
    FRAME_T allowed_frames[] =
    {
        FRAME_SCH, FRAME_PCB, FRAME_PCB_MODULE_EDITOR
    };

    bool allow_save = false;

    for( int ii = 0; ii < 3; ii++ )
    {
        if( m_Ident == allowed_frames[ii] )
        {
            allow_save = true;
            break;
        }
    }

    if( !allow_save )
        return false;

    if( aCanvasType < EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE
            || aCanvasType >= EDA_DRAW_PANEL_GAL::GAL_TYPE_LAST )
    {
        wxASSERT( false );
        return false;
    }

    wxConfigBase* cfg = Kiface().KifaceSettings();

    if( cfg )
        return cfg->Write( GetCanvasTypeKey(), (long) aCanvasType );

    return false;
}

//-----< BASE_SCREEN API moved here >--------------------------------------------

wxPoint EDA_DRAW_FRAME::GetNearestGridPosition( const wxPoint& aPosition ) const
{
    return GetScreen()->getNearestGridPosition( aPosition, GetGridOrigin() );
}

//-----</BASE_SCREEN API moved here >--------------------------------------------

const BOX2I EDA_DRAW_FRAME::GetDocumentExtents() const
{
    return BOX2I();
}


void EDA_DRAW_FRAME::HardRedraw()
{
    // To be implemented by subclasses.
}


// Factor out the calculation portion of the various BestZoom() implementations.
//
// Note that like it's forerunners this routine has an intentional side-effect: it
// sets the scroll centre position.  While I'm not happy about that, it's probably
// not worth fixing as its days are numbered (GAL canvases use a different method).
double EDA_DRAW_FRAME::bestZoom( double sizeX, double sizeY, double scaleFactor, wxPoint centre )
{
	return 1.0;
}


void EDA_DRAW_FRAME::Zoom_Automatique( bool aWarpPointer )
{
    m_toolManager->RunAction( ACTIONS::zoomFitScreen, true );
}


// Find the first child dialog.
wxWindow* findDialog( wxWindowList& aList )
{
    for( wxWindow* window : aList )
    {
        if( dynamic_cast<DIALOG_SHIM*>( window ) )
            return window;
    }
    return NULL;
}


void EDA_DRAW_FRAME::FocusOnLocation( const wxPoint& aPos, bool aCenterView )
{
    if( aCenterView )
    {
        wxWindow* dialog = findDialog( GetChildren() );

        // If a dialog partly obscures the window, then center on the uncovered area.
        if( dialog )
        {
            wxRect dialogRect( GetCanvas()->ScreenToClient( dialog->GetScreenPosition() ),
                               dialog->GetSize() );
            GetCanvas()->GetView()->SetCenter( aPos, dialogRect );
        }
        else
            GetCanvas()->GetView()->SetCenter( aPos );
    }

    GetCanvas()->GetViewControls()->SetCrossHairCursorPosition( aPos );
}


static const wxString productName = wxT( "KiCad E.D.A.  " );

void PrintPageLayout( wxDC* aDC, const PAGE_INFO& aPageInfo, const wxString& aFullSheetName,
                      const wxString& aFileName, const TITLE_BLOCK& aTitleBlock, int aSheetCount,
                      int aSheetNumber, int aPenWidth, double aScalar, COLOR4D aColor,
                      const wxString& aSheetLayer )
{
    WS_DRAW_ITEM_LIST drawList;

    drawList.SetDefaultPenSize( aPenWidth );
    drawList.SetMilsToIUfactor( aScalar );
    drawList.SetSheetNumber( aSheetNumber );
    drawList.SetSheetCount( aSheetCount );
    drawList.SetFileName( aFileName );
    drawList.SetSheetName( aFullSheetName );
    drawList.SetSheetLayer( aSheetLayer );

    drawList.BuildWorkSheetGraphicList( aPageInfo, aTitleBlock );

    // Draw item list
    drawList.Print( aDC, aColor );
}


void EDA_DRAW_FRAME::PrintWorkSheet( wxDC* aDC, BASE_SCREEN* aScreen, int aLineWidth,
                                     double aScalar, const wxString &aFilename,
                                     const wxString &aSheetLayer, COLOR4D aColor )
{
    if( !m_showBorderAndTitleBlock )
        return;

    COLOR4D color = ( aColor != COLOR4D::UNSPECIFIED ) ? aColor : COLOR4D( RED );

    wxPoint origin = aDC->GetDeviceOrigin();

    if( origin.y > 0 )
    {
        aDC->SetDeviceOrigin( 0, 0 );
        aDC->SetAxisOrientation( true, false );
    }

    PrintPageLayout( aDC, GetPageSettings(), GetScreenDesc(), aFilename, GetTitleBlock(),
                     aScreen->m_NumberOfScreens, aScreen->m_ScreenNumber, aLineWidth, aScalar,
                     color, aSheetLayer );

    if( origin.y > 0 )
    {
        aDC->SetDeviceOrigin( origin.x, origin.y );
        aDC->SetAxisOrientation( true, true );
    }
}


wxString EDA_DRAW_FRAME::GetScreenDesc() const
{
    // Virtual function. Base class implementation returns an empty string.
    return wxEmptyString;
}

bool EDA_DRAW_FRAME::LibraryFileBrowser( bool doOpen, wxFileName& aFilename,
                                         const wxString& wildcard, const wxString& ext,
                                         bool isDirectory )
{
    wxString prompt = doOpen ? _( "Select Library" ) : _( "New Library" );
    aFilename.SetExt( ext );

    if( isDirectory && doOpen )
    {
        wxDirDialog dlg( this, prompt, Prj().GetProjectPath(),
                         wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        aFilename = dlg.GetPath();
        aFilename.SetExt( ext );
    }
    else
    {
        wxFileDialog dlg( this, prompt, Prj().GetProjectPath(), aFilename.GetFullName() ,
                          wildcard, doOpen ? wxFD_OPEN | wxFD_FILE_MUST_EXIST
                                           : wxFD_SAVE | wxFD_CHANGE_DIR | wxFD_OVERWRITE_PROMPT );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        aFilename = dlg.GetPath();
        aFilename.SetExt( ext );
    }

    return true;
}


void EDA_DRAW_FRAME::RecreateToolbars()
{
    // Rebuild all toolbars, and update the checked state of check tools
    if( m_mainToolBar )
        ReCreateHToolbar();

    if( m_drawToolBar )         // Drawing tools (typically on right edge of window)
        ReCreateVToolbar();

    if( m_optionsToolBar )      // Options (typically on left edge of window)
        ReCreateOptToolbar();

    if( m_auxiliaryToolBar )    // Additional tools under main toolbar
       ReCreateOptToolbar();

    // Update the checked state of tools
    SyncToolbars();
}
