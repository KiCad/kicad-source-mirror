/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2012 KiCad Developers, see change_log.txt for contributors.
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
 * @file modview_frame.cpp
 */

#include <fctsys.h>
#include <appl_wxstruct.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <wxPcbStruct.h>
#include <3d_viewer.h>
#include <pcbcommon.h>
#include <dialog_helpers.h>
#include <msgpanel.h>
#include <macros.h>
#include <fp_lib_table.h>
#include <fpid.h>
#include <confirm.h>

#include <class_board.h>
#include <class_module.h>

#include <pcbnew.h>
#include <pcbnew_id.h>
#include <modview_frame.h>
#include <footprint_info.h>

#include <hotkeys.h>
#include <wildcards_and_files_ext.h>
#include <pcbnew_config.h>


#define NEXT_PART      1
#define NEW_PART       0
#define PREVIOUS_PART -1


/**
 * Save previous component library viewer state.
 */
wxString FOOTPRINT_VIEWER_FRAME::m_libraryName;
wxString FOOTPRINT_VIEWER_FRAME::m_footprintName;

/// When the viewer is used to select a component in schematic, the selected component is here.
wxString FOOTPRINT_VIEWER_FRAME::m_selectedFootprintName;


BEGIN_EVENT_TABLE( FOOTPRINT_VIEWER_FRAME, EDA_DRAW_FRAME )
    /* Window events */
    EVT_CLOSE( FOOTPRINT_VIEWER_FRAME::OnCloseWindow )
    EVT_SIZE( FOOTPRINT_VIEWER_FRAME::OnSize )
    EVT_ACTIVATE( FOOTPRINT_VIEWER_FRAME::OnActivate )

    /* Toolbar events */
    EVT_TOOL( ID_MODVIEW_SELECT_LIB,
              FOOTPRINT_VIEWER_FRAME::SelectCurrentLibrary )
    EVT_TOOL( ID_MODVIEW_SELECT_PART,
              FOOTPRINT_VIEWER_FRAME::SelectCurrentFootprint )
    EVT_TOOL( ID_MODVIEW_NEXT,
              FOOTPRINT_VIEWER_FRAME::OnIterateFootprintList )
    EVT_TOOL( ID_MODVIEW_PREVIOUS,
              FOOTPRINT_VIEWER_FRAME::OnIterateFootprintList )
    EVT_TOOL( ID_MODVIEW_FOOTPRINT_EXPORT_TO_BOARD,
              FOOTPRINT_VIEWER_FRAME::ExportSelectedFootprint )
    EVT_TOOL( ID_MODVIEW_SHOW_3D_VIEW, FOOTPRINT_VIEWER_FRAME::Show3D_Frame )

    /* listbox events */
    EVT_LISTBOX( ID_MODVIEW_LIB_LIST, FOOTPRINT_VIEWER_FRAME::ClickOnLibList )
    EVT_LISTBOX( ID_MODVIEW_FOOTPRINT_LIST, FOOTPRINT_VIEWER_FRAME::ClickOnFootprintList )
    EVT_LISTBOX_DCLICK( ID_MODVIEW_FOOTPRINT_LIST, FOOTPRINT_VIEWER_FRAME::DClickOnFootprintList )

    EVT_MENU( ID_SET_RELATIVE_OFFSET, FOOTPRINT_VIEWER_FRAME::OnSetRelativeOffset )
END_EVENT_TABLE()


/*
 * This emulates the zoom menu entries found in the other KiCad applications.
 * The library viewer does not have any menus so add an accelerator table to
 * the main frame.
 */
static wxAcceleratorEntry accels[] =
{
    wxAcceleratorEntry( wxACCEL_NORMAL, WXK_F1, ID_ZOOM_IN ),
    wxAcceleratorEntry( wxACCEL_NORMAL, WXK_F2, ID_ZOOM_OUT ),
    wxAcceleratorEntry( wxACCEL_NORMAL, WXK_F3, ID_ZOOM_REDRAW ),
    wxAcceleratorEntry( wxACCEL_NORMAL, WXK_F4, ID_POPUP_ZOOM_CENTER ),
    wxAcceleratorEntry( wxACCEL_NORMAL, WXK_HOME, ID_ZOOM_PAGE ),
    wxAcceleratorEntry( wxACCEL_NORMAL, WXK_SPACE, ID_SET_RELATIVE_OFFSET )
};

#define ACCEL_TABLE_CNT ( sizeof( accels ) / sizeof( wxAcceleratorEntry ) )

#define EXTRA_BORDER_SIZE 2

#define FOOTPRINT_VIEWER_FRAME_NAME wxT( "ModViewFrame" )

FOOTPRINT_VIEWER_FRAME::FOOTPRINT_VIEWER_FRAME( PCB_BASE_FRAME* aParent,
                                                FP_LIB_TABLE*   aTable,
                                                wxSemaphore*    aSemaphore,
                                                long            aStyle ) :
    PCB_BASE_FRAME( aParent, MODULE_VIEWER_FRAME_TYPE, _( "Footprint Library Browser" ),
                    wxDefaultPosition, wxDefaultSize, aStyle, GetFootprintViewerFrameName() )
{
    wxAcceleratorTable table( ACCEL_TABLE_CNT, accels );

    m_footprintLibTable = aTable;
    m_FrameName = GetFootprintViewerFrameName();
    m_configPath = wxT( "FootprintViewer" );
    m_showAxis = true;         // true to draw axis.

    // Give an icon
    wxIcon  icon;
    icon.CopyFromBitmap( KiBitmap( modview_icon_xpm ) );
    SetIcon( icon );

    m_HotkeysZoomAndGridList = g_Module_Viewer_Hokeys_Descr;
    m_FootprintList = NULL;
    m_LibList = NULL;
    m_LibListWindow = NULL;
    m_FootprintListWindow = NULL;
    m_Semaphore     = aSemaphore;
    m_selectedFootprintName.Empty();

    if( m_Semaphore )
        SetModalMode( true );

    SetBoard( new BOARD() );
    // Ensure all layers and items are visible:
    GetBoard()->SetVisibleAlls();
    SetScreen( new PCB_SCREEN( GetPageSizeIU() ) );
    GetScreen()->m_Center = true;      // Center coordinate origins on screen.
    LoadSettings();

    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );
    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId  );

    ReCreateHToolbar();
    ReCreateVToolbar();

    wxSize initialSashSize( 100, -1 );

     // Creates the libraries window display
    m_LibListWindow =
        new wxSashLayoutWindow( this, ID_MODVIEW_LIBWINDOW, wxDefaultPosition,
                                initialSashSize, wxCLIP_CHILDREN | wxSW_3D,
                                wxT( "LibWindow" ) );
    m_LibListWindow->SetOrientation( wxLAYOUT_VERTICAL );
    m_LibListWindow->SetAlignment( wxLAYOUT_LEFT );
    m_LibListWindow->SetSashVisible( wxSASH_RIGHT, true );
    m_LibListWindow->SetExtraBorderSize( EXTRA_BORDER_SIZE );
    m_LibList = new wxListBox( m_LibListWindow, ID_MODVIEW_LIB_LIST,
                               wxDefaultPosition, initialSashSize,
                               0, NULL, wxLB_HSCROLL );

    // Creates the footprint window display
    m_FootprintListWindow = new wxSashLayoutWindow( this, ID_MODVIEW_FOOTPRINT_WINDOW,
                                                    wxDefaultPosition, initialSashSize,
                                                    wxCLIP_CHILDREN | wxSW_3D,
                                                    wxT( "CmpWindow" ) );
    m_FootprintListWindow->SetOrientation( wxLAYOUT_VERTICAL );

    m_FootprintListWindow->SetSashVisible( wxSASH_RIGHT, true );
    m_FootprintListWindow->SetExtraBorderSize( EXTRA_BORDER_SIZE );
    m_FootprintList = new wxListBox( m_FootprintListWindow, ID_MODVIEW_FOOTPRINT_LIST,
                                     wxDefaultPosition, initialSashSize,
                                     0, NULL, wxLB_HSCROLL );

    ReCreateLibraryList();
    DisplayLibInfos();

    // If a footprint was previously loaded, reload it
    if( !m_libraryName.IsEmpty() && !m_footprintName.IsEmpty() )
    {
#if !defined( USE_FP_LIB_TABLE )
        MODULE* footprint = GetModuleLibrary( m_libraryName + wxT( "." ) +  LegacyFootprintLibPathExtension,
                                              m_footprintName, false );

        if( footprint )
            GetBoard()->Add( footprint, ADD_APPEND );
#else
        FPID id;
        id.SetLibNickname( m_libraryName );
        id.SetFootprintName( m_footprintName );
        GetBoard()->Add( loadFootprint( id ) );
#endif
    }

    if( m_canvas )
        m_canvas->SetAcceleratorTable( table );

    m_auimgr.SetManagedWindow( this );

    // Main toolbar is initially docked at the top of the main window and dockable on any side.
    // The close button is disable because the footprint viewer has no main menu to re-enable it.
    // The tool bar will only be dockable on the top or bottom of the main frame window.  This is
    // most likely due to the fact that the other windows are not dockable and are preventing the
    // tool bar from docking on the right and left.
    wxAuiPaneInfo toolbarPaneInfo;
    toolbarPaneInfo.Name( wxT( "m_mainToolBar" ) ).ToolbarPane().Top().CloseButton( false );

    EDA_PANEINFO info;
    info.InfoToolbarPane();

    EDA_PANEINFO mesg;
    mesg.MessageToolbarPane();

    // Manage main toolbar, top pane
    m_auimgr.AddPane( m_mainToolBar, toolbarPaneInfo );

    // Manage the list of libraries, left pane.
    if( m_LibListWindow )
        m_auimgr.AddPane( m_LibListWindow, wxAuiPaneInfo( info ).Name( wxT( "m_LibList" ) ).
                          Left().Row( 1 ) );

    // Manage the list of footprints, center pane.
    m_auimgr.AddPane( m_FootprintListWindow,
                      wxAuiPaneInfo( info ).Name( wxT( "m_FootprintList" ) ).Centre().Row( 1 ) );

    // Manage the draw panel, right pane.
    m_auimgr.AddPane( m_canvas,
                      wxAuiPaneInfo().Name( wxT( "DrawFrame" ) ).Right().Row( 1 ).CloseButton( false ) );

    // Manage the message panel, bottom pane.
    m_auimgr.AddPane( m_messagePanel,
                      wxAuiPaneInfo( mesg ).Name( wxT( "MsgPanel" ) ).Bottom() );

    /* Now the minimum windows are fixed, set library list
     * and component list of the previous values from last viewlib use
     */
    if( m_LibListWindow )
    {
        wxAuiPaneInfo& pane = m_auimgr.GetPane( m_LibListWindow );
        pane.MinSize( wxSize( 30, -1 ) );
    }

    if( m_FootprintListWindow )
    {
        wxAuiPaneInfo& pane = m_auimgr.GetPane( m_FootprintListWindow );
        pane.MinSize( wxSize( 30, -1 ) );
    }

    if( !m_perspective.IsEmpty() )
        m_auimgr.LoadPerspective( m_perspective );
    else
        m_auimgr.Update();

    // Now Drawpanel is sized, we can use BestZoom to show the component (if any)
#ifdef USE_WX_GRAPHICS_CONTEXT
    GetScreen()->SetZoom( BestZoom() );
#else
    Zoom_Automatique( false );
#endif

    Show( true );
}


FOOTPRINT_VIEWER_FRAME::~FOOTPRINT_VIEWER_FRAME()
{
    if( m_Draw3DFrame )
        m_Draw3DFrame->Destroy();
}


const wxChar* FOOTPRINT_VIEWER_FRAME::GetFootprintViewerFrameName()
{
    return FOOTPRINT_VIEWER_FRAME_NAME;
}


FOOTPRINT_VIEWER_FRAME* FOOTPRINT_VIEWER_FRAME::GetActiveFootprintViewer()
{
    return (FOOTPRINT_VIEWER_FRAME*)
            wxWindow::FindWindowByName( GetFootprintViewerFrameName() );
}


void FOOTPRINT_VIEWER_FRAME::OnCloseWindow( wxCloseEvent& Event )
{
    SaveSettings();

    if( m_Semaphore )
    {
        m_Semaphore->Post();
        SetModalMode( false );
        // This window will be destroyed by the calling function,
        // to avoid side effects
    }
    else
        Destroy();
}


void FOOTPRINT_VIEWER_FRAME::OnSize( wxSizeEvent& SizeEv )
{
    if( m_auimgr.GetManagedWindow() )
        m_auimgr.Update();

    SizeEv.Skip();
}


void FOOTPRINT_VIEWER_FRAME::OnSetRelativeOffset( wxCommandEvent& event )
{
    GetScreen()->m_O_Curseur = GetCrossHairPosition();
    UpdateStatusBar();
}


void FOOTPRINT_VIEWER_FRAME::ReCreateLibraryList()
{
    if( m_LibList == NULL )
        return;

    m_LibList->Clear();

#if !defined( USE_FP_LIB_TABLE )
    for( unsigned ii = 0; ii < g_LibraryNames.GetCount(); ii++ )
    {
        m_LibList->Append( g_LibraryNames[ii] );
    }
#else
    std::vector< wxString > libName = m_footprintLibTable->GetLogicalLibs();

    for( unsigned ii = 0; ii < libName.size(); ii++ )
        m_LibList->Append( libName[ii] );
#endif

    // Search for a previous selection:
    int index =  m_LibList->FindString( m_libraryName );

    if( index != wxNOT_FOUND )
    {
        m_LibList->SetSelection( index, true );
    }
    else
    {
        // If not found, clear current library selection because it can be deleted after
        // a configuration change.
        m_libraryName = wxEmptyString;
        m_footprintName = wxEmptyString;
    }

    ReCreateFootprintList();
    ReCreateHToolbar();
    DisplayLibInfos();
    m_canvas->Refresh();
}


void FOOTPRINT_VIEWER_FRAME::ReCreateFootprintList()
{
    if( m_FootprintList == NULL )
        return;

    m_FootprintList->Clear();

    if( m_libraryName.IsEmpty() )
    {
        m_footprintName = wxEmptyString;
        return;
    }

    bool           libLoaded = false;
    FOOTPRINT_LIST fp_info_list;
    wxArrayString  libsList;

#if !defined( USE_FP_LIB_TABLE )

    libsList.Add( m_libraryName );
    libLoaded = fp_info_list.ReadFootprintFiles( libsList );

#else

    libLoaded = fp_info_list.ReadFootprintFiles( m_footprintLibTable, &m_libraryName );

#endif

    if( !libLoaded )
    {
        m_footprintName = wxEmptyString;
        m_libraryName = wxEmptyString;

        wxString msg;
        msg.Format( _( "Error occurred attempting to load footprint library <%s>:\n\n" ),
                    GetChars( m_libraryName ) );

        if( !fp_info_list.m_filesNotFound.IsEmpty() )
            msg += _( "Files not found:\n\n" ) + fp_info_list.m_filesNotFound;

        if( !fp_info_list.m_filesInvalid.IsEmpty() )
            msg +=  _( "\n\nFile load errors:\n\n" ) + fp_info_list.m_filesInvalid;

        DisplayError( this, msg );
        return;
    }

    wxArrayString  fpList;

    BOOST_FOREACH( FOOTPRINT_INFO& footprint, fp_info_list.m_List )
    {
        fpList.Add( footprint.m_Module );
    }

    m_FootprintList->Append( fpList );

    int index = m_FootprintList->FindString( m_footprintName );

    if( index == wxNOT_FOUND )
        m_footprintName = wxEmptyString;
    else
        m_FootprintList->SetSelection( index, true );
}


void FOOTPRINT_VIEWER_FRAME::ClickOnLibList( wxCommandEvent& event )
{
    int ii = m_LibList->GetSelection();

    if( ii < 0 )
        return;

    wxString name = m_LibList->GetString( ii );

    if( m_libraryName == name )
        return;

    m_libraryName = name;
    ReCreateFootprintList();
    m_canvas->Refresh();
    DisplayLibInfos();
    ReCreateHToolbar();
}


void FOOTPRINT_VIEWER_FRAME::ClickOnFootprintList( wxCommandEvent& event )
{
    if( m_FootprintList->GetCount() == 0 )
        return;

    int ii = m_FootprintList->GetSelection();

    if( ii < 0 )
        return;

    wxString name = m_FootprintList->GetString( ii );

    if( m_footprintName.CmpNoCase( name ) != 0 )
    {
        m_footprintName = name;
        SetCurItem( NULL );
        // Delete the current footprint
        GetBoard()->m_Modules.DeleteAll();
#if !defined( USE_FP_LIB_TABLE )
        MODULE* footprint = GetModuleLibrary( m_libraryName + wxT( "." ) + LegacyFootprintLibPathExtension,
                                              m_footprintName, true );

        if( footprint )
            GetBoard()->Add( footprint, ADD_APPEND );
#else
        FPID id;
        id.SetLibNickname( m_libraryName );
        id.SetFootprintName( m_footprintName );

        try
        {
            GetBoard()->Add( loadFootprint( id ) );
        }
        catch( IO_ERROR ioe )
        {
            wxString msg;
            msg.Printf( _( "Could not load footprint \"%s\" from library \"%s\".\n\n"
                           "Error %s." ), GetChars( m_footprintName ), GetChars( m_libraryName ),
                        GetChars( ioe.errorText ) );
            DisplayError( this, msg );
        }
#endif

        DisplayLibInfos();
        Zoom_Automatique( false );
        m_canvas->Refresh();
        Update3D_Frame();
    }
}


void FOOTPRINT_VIEWER_FRAME::DClickOnFootprintList( wxCommandEvent& event )
{
    if( m_Semaphore )
    {
        ExportSelectedFootprint( event );
        // Prevent the double click from being as a single mouse button release
        // event in the parent window which would cause the part to be parked
        // rather than staying in mode mode.
        // Remember the mouse button will be released in the parent window
        // thus creating a mouse button release event which should be ignored
        ((PCB_BASE_FRAME*)GetParent())->SkipNextLeftButtonReleaseEvent();
    }
}


void FOOTPRINT_VIEWER_FRAME::ExportSelectedFootprint( wxCommandEvent& event )
{
    int ii = m_FootprintList->GetSelection();

    if( ii >= 0 )
        m_selectedFootprintName = m_FootprintList->GetString( ii );
    else
        m_selectedFootprintName.Empty();

    Close( true );
}


void FOOTPRINT_VIEWER_FRAME::LoadSettings( )
{
    wxConfig* cfg ;

    EDA_DRAW_FRAME::LoadSettings();

    wxConfigPathChanger cpc( wxGetApp().GetSettings(), m_configPath );
    cfg = wxGetApp().GetSettings();
}


void FOOTPRINT_VIEWER_FRAME::SaveSettings()
{
    wxConfig* cfg;

    EDA_DRAW_FRAME::SaveSettings();

    wxConfigPathChanger cpc( wxGetApp().GetSettings(), m_configPath );
    cfg = wxGetApp().GetSettings();
}


void FOOTPRINT_VIEWER_FRAME::OnActivate( wxActivateEvent& event )
{
    EDA_DRAW_FRAME::OnActivate( event );

    // Ensure we do not have old selection:
    if( ! m_FrameIsActive )
        return;

    m_selectedFootprintName.Empty();

    // Ensure we have the right library list:
#if !defined( USE_FP_LIB_TABLE )
    if( g_LibraryNames.GetCount() == m_LibList->GetCount() )
    {
        unsigned ii;

        for( ii = 0; ii < g_LibraryNames.GetCount(); ii++ )
        {
            if( m_LibList->GetString( ii ) != g_LibraryNames[ii] )
                break;
        }

        if( ii == g_LibraryNames.GetCount() )
            return;
    }
#else
    std::vector< wxString > libNicknames = m_footprintLibTable->GetLogicalLibs();

    if( libNicknames.size() == m_LibList->GetCount() )
    {
        unsigned ii;

        for( ii = 0;  ii < libNicknames.size();  ii++ )
        {
            if( libNicknames[ii] != m_LibList->GetString( ii ) )
                break;
        }

        if( ii == libNicknames.size() )
            return;
    }
#endif

    // If we are here, the library list has changed, rebuild it
    ReCreateLibraryList();
    DisplayLibInfos();
}


void FOOTPRINT_VIEWER_FRAME::GeneralControl( wxDC* aDC, const wxPoint& aPosition, int aHotKey )
{
    wxRealPoint gridSize;
    wxPoint     oldpos;
    PCB_SCREEN* screen = GetScreen();
    wxPoint     pos = aPosition;

    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );
    cmd.SetEventObject( this );

    pos = GetNearestGridPosition( pos );
    oldpos = GetCrossHairPosition();
    gridSize = screen->GetGridSize();

    switch( aHotKey )
    {
    case WXK_F1:
        cmd.SetId( ID_POPUP_ZOOM_IN );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case WXK_F2:
        cmd.SetId( ID_POPUP_ZOOM_OUT );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case WXK_F3:
        cmd.SetId( ID_ZOOM_REDRAW );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case WXK_F4:
        cmd.SetId( ID_POPUP_ZOOM_CENTER );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case WXK_HOME:
        cmd.SetId( ID_ZOOM_PAGE );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case ' ':
        screen->m_O_Curseur = GetCrossHairPosition();
        break;

    case WXK_NUMPAD8:       /* cursor moved up */
    case WXK_UP:
        pos.y -= KiROUND( gridSize.y );
        m_canvas->MoveCursor( pos );
        break;

    case WXK_NUMPAD2:       /* cursor moved down */
    case WXK_DOWN:
        pos.y += KiROUND( gridSize.y );
        m_canvas->MoveCursor( pos );
        break;

    case WXK_NUMPAD4:       /*  cursor moved left */
    case WXK_LEFT:
        pos.x -= KiROUND( gridSize.x );
        m_canvas->MoveCursor( pos );
        break;

    case WXK_NUMPAD6:      /*  cursor moved right */
    case WXK_RIGHT:
        pos.x += KiROUND( gridSize.x );
        m_canvas->MoveCursor( pos );
        break;
    }

    SetCrossHairPosition( pos );

    if( oldpos != GetCrossHairPosition() )
    {
        pos = GetCrossHairPosition();
        SetCrossHairPosition( oldpos );
        m_canvas->CrossHairOff( aDC );
        SetCrossHairPosition( pos );
        m_canvas->CrossHairOn( aDC );

        if( m_canvas->IsMouseCaptured() )
        {
            m_canvas->CallMouseCapture( aDC, aPosition, 0 );
        }
    }

    UpdateStatusBar();    /* Display new cursor coordinates */
}


void FOOTPRINT_VIEWER_FRAME::Show3D_Frame( wxCommandEvent& event )
{
    if( m_Draw3DFrame )
    {
        // Raising the window does not show the window on Windows if iconized.
        // This should work on any platform.
        if( m_Draw3DFrame->IsIconized() )
             m_Draw3DFrame->Iconize( false );

        m_Draw3DFrame->Raise();

        // Raising the window does not set the focus on Linux.  This should work on any platform.
        if( wxWindow::FindFocus() != m_Draw3DFrame )
            m_Draw3DFrame->SetFocus();

        return;
    }

    m_Draw3DFrame = new EDA_3D_FRAME( this, wxEmptyString );
    Update3D_Frame( false );
    m_Draw3DFrame->Show( true );
}


void FOOTPRINT_VIEWER_FRAME::Update3D_Frame( bool aForceReloadFootprint )
{
    if( m_Draw3DFrame == NULL )
        return;

    wxString frm3Dtitle;
    frm3Dtitle.Printf( _( "ModView: 3D Viewer [%s]" ), GetChars( m_footprintName ) );
    m_Draw3DFrame->SetTitle( frm3Dtitle );

    if( aForceReloadFootprint )
    {
        m_Draw3DFrame->ReloadRequest();

        // Force 3D screen refresh immediately
        if( GetBoard()->m_Modules )
            m_Draw3DFrame->NewDisplay();
    }
}


EDA_COLOR_T FOOTPRINT_VIEWER_FRAME::GetGridColor() const
{
    return g_ColorsSettings.GetItemColor( GRID_VISIBLE );
}


void FOOTPRINT_VIEWER_FRAME::OnIterateFootprintList( wxCommandEvent& event )
{
    wxString   msg;

    switch( event.GetId() )
    {
    case ID_MODVIEW_NEXT:
        SelectAndViewFootprint( NEXT_PART );
        break;

    case ID_MODVIEW_PREVIOUS:
        SelectAndViewFootprint( PREVIOUS_PART );
        break;

    default:
        wxFAIL_MSG( wxT( "FOOTPRINT_VIEWER_FRAME::OnIterateFootprintList error: id = " ) +
                    event.GetId() );
    }
}


void FOOTPRINT_VIEWER_FRAME::OnLeftClick( wxDC* DC, const wxPoint& MousePos )
{
}


bool FOOTPRINT_VIEWER_FRAME::OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu )
{
    return true;
}


void FOOTPRINT_VIEWER_FRAME::DisplayLibInfos()
{
    wxString     msg;

    msg = _( "Library Browser" );
    msg << wxT( " [" );

    if( ! m_libraryName.IsEmpty() )
        msg << m_libraryName;
    else
        msg += _( "no library selected" );

    msg << wxT( "]" );

    SetTitle( msg );
}


void FOOTPRINT_VIEWER_FRAME::SelectCurrentLibrary( wxCommandEvent& event )
{
    wxString msg;

    if( g_LibraryNames.GetCount() == 0 )
        return;

    wxArrayString headers;
    headers.Add( wxT( "Library" ) );
    std::vector<wxArrayString> itemsToDisplay;

    // Conversion from wxArrayString to vector of ArrayString
    for( unsigned i = 0; i < g_LibraryNames.GetCount(); i++ )
    {
        wxArrayString item;
        item.Add( g_LibraryNames[i] );
        itemsToDisplay.push_back( item );
    }

    EDA_LIST_DIALOG dlg( this, _( "Select Current Library:" ),
                         headers, itemsToDisplay, m_libraryName );

    if( dlg.ShowModal() != wxID_OK )
        return;

    if( m_libraryName == dlg.GetTextSelection() )
        return;

    m_libraryName = dlg.GetTextSelection();
    m_footprintName.Empty();
    DisplayLibInfos();
    ReCreateFootprintList();

    int id = m_LibList->FindString( m_libraryName );

    if( id >= 0 )
        m_LibList->SetSelection( id );
}


void FOOTPRINT_VIEWER_FRAME::SelectCurrentFootprint( wxCommandEvent& event )
{
    PCB_EDIT_FRAME* parent = (PCB_EDIT_FRAME*) GetParent();
    wxString        libname = m_libraryName + wxT( "." ) + LegacyFootprintLibPathExtension;
    MODULE*         oldmodule = GetBoard()->m_Modules;
    MODULE*         module = LoadModuleFromLibrary( libname, parent->GetFootprintLibraryTable(),
                                                    false );

    if( module )
    {
        module->SetPosition( wxPoint( 0, 0 ) );

        // Only one footprint allowed: remove the previous footprint (if exists)
        if( oldmodule )
        {
            GetBoard()->Remove( oldmodule );
            delete oldmodule;
        }

        m_footprintName = FROM_UTF8( module->GetFPID().GetFootprintName().c_str() );
        module->ClearFlags();
        SetCurItem( NULL );

        Zoom_Automatique( false );
        m_canvas->Refresh();
        Update3D_Frame();
        m_FootprintList->SetStringSelection( m_footprintName );
   }
}


const wxString FOOTPRINT_VIEWER_FRAME::GetSelectedLibraryFullName( void )
{
    wxString fullname = m_libraryName + wxT( "." ) + LegacyFootprintLibPathExtension;
    return fullname;
}


void FOOTPRINT_VIEWER_FRAME::SelectAndViewFootprint( int aMode )
{
    if( m_libraryName.IsEmpty() )
        return;

    int selection = m_FootprintList->FindString( m_footprintName );

    if( aMode == NEXT_PART )
    {
        if( selection != wxNOT_FOUND && selection < (int)m_FootprintList->GetCount()-1 )
            selection++;
    }

    if( aMode == PREVIOUS_PART )
    {
        if( selection != wxNOT_FOUND && selection > 0 )
            selection--;
    }

    if( selection != wxNOT_FOUND )
    {
        m_FootprintList->SetSelection( selection );
        m_footprintName = m_FootprintList->GetString( selection );
        SetCurItem( NULL );

        // Delete the current footprint
        GetBoard()->m_Modules.DeleteAll();
        MODULE* footprint = GetModuleLibrary( GetSelectedLibraryFullName(), m_footprintName, true );

        if( footprint )
            GetBoard()->Add( footprint, ADD_APPEND );

        Update3D_Frame();
    }

    DisplayLibInfos();
    Zoom_Automatique( false );
    m_canvas->Refresh();
}


void FOOTPRINT_VIEWER_FRAME::RedrawActiveWindow( wxDC* DC, bool EraseBg )
{
    if( !GetBoard() )
        return;

    m_canvas->DrawBackGround( DC );
    GetBoard()->Draw( m_canvas, DC, GR_COPY );

    MODULE* module = GetBoard()->m_Modules;

    m_canvas->DrawCrossHair( DC );

    ClearMsgPanel();

    if( module )
        SetMsgPanel( module );
}
