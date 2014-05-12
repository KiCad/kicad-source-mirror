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
#include <pgm_base.h>
#include <kiway.h>
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
    // Window events
    EVT_CLOSE( FOOTPRINT_VIEWER_FRAME::OnCloseWindow )
    EVT_SIZE( FOOTPRINT_VIEWER_FRAME::OnSize )
    EVT_ACTIVATE( FOOTPRINT_VIEWER_FRAME::OnActivate )

    // Toolbar events
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

    // listbox events
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


#define EXTRA_BORDER_SIZE               2

#define FOOTPRINT_VIEWER_FRAME_NAME     wxT( "ModViewFrame" )


FOOTPRINT_VIEWER_FRAME::FOOTPRINT_VIEWER_FRAME( KIWAY* aKiway, wxWindow* aParent, FRAME_T aFrameType ) :
    PCB_BASE_FRAME( aKiway, aParent, aFrameType, _( "Footprint Library Browser" ),
            wxDefaultPosition, wxDefaultSize,
            aFrameType == FRAME_PCB_MODULE_VIEWER_MODAL ?
                KICAD_DEFAULT_DRAWFRAME_STYLE | wxFRAME_FLOAT_ON_PARENT :
                KICAD_DEFAULT_DRAWFRAME_STYLE,
            GetFootprintViewerFrameName() )
{
    wxASSERT( aFrameType==FRAME_PCB_MODULE_VIEWER || aFrameType==FRAME_PCB_MODULE_VIEWER_MODAL );

    if( aFrameType == FRAME_PCB_MODULE_VIEWER_MODAL )
        SetModal( true );

    wxAcceleratorTable table( DIM( accels ), accels );

    m_FrameName  = GetFootprintViewerFrameName();
    m_configPath = wxT( "FootprintViewer" );
    m_showAxis   = true;         // true to draw axis.

    // Give an icon
    wxIcon  icon;
    icon.CopyFromBitmap( KiBitmap( modview_icon_xpm ) );
    SetIcon( icon );

    m_HotkeysZoomAndGridList = g_Module_Viewer_Hokeys_Descr;

    m_libList = new wxListBox( this, ID_MODVIEW_LIB_LIST,
            wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_HSCROLL );

    m_footprintList = new wxListBox( this, ID_MODVIEW_FOOTPRINT_LIST,
            wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_HSCROLL );

    m_selectedFootprintName.Empty();

    SetBoard( new BOARD() );

    // Ensure all layers and items are visible:
    GetBoard()->SetVisibleAlls();
    SetScreen( new PCB_SCREEN( GetPageSizeIU() ) );

    GetScreen()->m_Center = true;      // Center coordinate origins on screen.
    LoadSettings( config() );

    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId  );

    ReCreateHToolbar();
    ReCreateVToolbar();

    ReCreateLibraryList();
    UpdateTitle();

    // If a footprint was previously loaded, reload it
    if( !m_libraryName.IsEmpty() && !m_footprintName.IsEmpty() )
    {
        FPID id;

        id.SetLibNickname( m_libraryName );
        id.SetFootprintName( m_footprintName );
        GetBoard()->Add( loadFootprint( id ) );
    }

    if( m_canvas )
        m_canvas->SetAcceleratorTable( table );

    m_auimgr.SetManagedWindow( this );

    wxSize minsize(100,-1);     // Min size of list boxes

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
    m_auimgr.AddPane( m_libList,
                      wxAuiPaneInfo( info ).Name( wxT( "m_libList" ) )
                      .Left().Row( 1 ).MinSize( minsize ) );

    // Manage the list of footprints, center pane.
    m_auimgr.AddPane( m_footprintList,
                      wxAuiPaneInfo( info ).Name( wxT( "m_footprintList" ) )
                      .Left().Row( 2 ).MinSize( minsize ) );

    // Manage the draw panel, right pane.
    m_auimgr.AddPane( m_canvas,
                      wxAuiPaneInfo().Name( wxT( "DrawFrame" ) ).CentrePane() );

    // Manage the message panel, bottom pane.
    m_auimgr.AddPane( m_messagePanel,
                      wxAuiPaneInfo( mesg ).Name( wxT( "MsgPanel" ) ).Bottom() );

    if( !m_perspective.IsEmpty() )
    {
        // Restore last saved sizes, pos and other params
        // However m_mainToolBar size cannot be set to its last saved size
        // because the actual size change depending on the way modview was called:
        // the tool to export the current footprint exist or not.
        // and the saved size is not always OK
        // the trick is to get the default toolbar size, and set the size after
        // calling LoadPerspective
        wxSize tbsize = m_mainToolBar->GetSize();
        m_auimgr.LoadPerspective( m_perspective, false );
        m_auimgr.GetPane( m_mainToolBar ).BestSize( tbsize );
    }

#if 0   // no.
    // Set min size (overwrite params read in LoadPerspective(), if any)
    m_auimgr.GetPane( m_libList ).MinSize( minsize );
    m_auimgr.GetPane( m_footprintList ).MinSize( minsize );
#endif

    // after changing something to the aui manager,
    // call Update()() to reflect the changes
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


void FOOTPRINT_VIEWER_FRAME::OnCloseWindow( wxCloseEvent& Event )
{
    DBG(printf( "%s:\n", __func__ );)
    if( IsModal() )
    {
        // Only dismiss a modal frame once, so that the return values set by
        // the prior DismissModal() are not bashed for ShowModal().
        if( !IsDismissed() )
            DismissModal( false );

        // window to be destroyed by the caller of KIWAY_PLAYER::ShowModal()
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
    m_libList->Clear();

    std::vector< wxString > nicknames = Prj().PcbFootprintLibs()->GetLogicalLibs();

    for( unsigned ii = 0; ii < nicknames.size(); ii++ )
        m_libList->Append( nicknames[ii] );

    // Search for a previous selection:
    int index =  m_libList->FindString( m_libraryName );

    if( index != wxNOT_FOUND )
    {
        m_libList->SetSelection( index, true );
    }
    else
    {
        // If not found, clear current library selection because it can be
        // deleted after a configuration change.
        m_libraryName = wxEmptyString;
        m_footprintName = wxEmptyString;
    }

    ReCreateFootprintList();
    ReCreateHToolbar();

    m_canvas->Refresh();
}


void FOOTPRINT_VIEWER_FRAME::ReCreateFootprintList()
{
    m_footprintList->Clear();

    if( m_libraryName.IsEmpty() )
    {
        m_footprintName = wxEmptyString;
        return;
    }

    FOOTPRINT_LIST fp_info_list;

    fp_info_list.ReadFootprintFiles( Prj().PcbFootprintLibs(), &m_libraryName );

    if( fp_info_list.GetErrorCount() )
    {
        fp_info_list.DisplayErrors( this );
        return;
    }

    BOOST_FOREACH( const FOOTPRINT_INFO& footprint, fp_info_list.GetList() )
    {
        m_footprintList->Append( footprint.GetFootprintName() );
    }

    int index = m_footprintList->FindString( m_footprintName );

    if( index == wxNOT_FOUND )
        m_footprintName = wxEmptyString;
    else
        m_footprintList->SetSelection( index, true );
}


void FOOTPRINT_VIEWER_FRAME::ClickOnLibList( wxCommandEvent& event )
{
    int ii = m_libList->GetSelection();

    if( ii < 0 )
        return;

    wxString name = m_libList->GetString( ii );

    if( m_libraryName == name )
        return;

    m_libraryName = name;

    ReCreateFootprintList();
    UpdateTitle();
    ReCreateHToolbar();
}


void FOOTPRINT_VIEWER_FRAME::ClickOnFootprintList( wxCommandEvent& event )
{
    if( m_footprintList->GetCount() == 0 )
        return;

    int ii = m_footprintList->GetSelection();

    if( ii < 0 )
        return;

    wxString name = m_footprintList->GetString( ii );

    if( m_footprintName.CmpNoCase( name ) != 0 )
    {
        m_footprintName = name;
        SetCurItem( NULL );

        // Delete the current footprint
        GetBoard()->m_Modules.DeleteAll();

        FPID id;
        id.SetLibNickname( m_libraryName );
        id.SetFootprintName( m_footprintName );

        try
        {
            GetBoard()->Add( loadFootprint( id ) );
        }
        catch( const IO_ERROR& ioe )
        {
            wxString msg;
            msg.Printf( _( "Could not load footprint \"%s\" from library \"%s\".\n\n"
                           "Error %s." ), GetChars( m_footprintName ), GetChars( m_libraryName ),
                        GetChars( ioe.errorText ) );
            DisplayError( this, msg );
        }

        UpdateTitle();
        Zoom_Automatique( false );
        m_canvas->Refresh();
        Update3D_Frame();
    }
}


void FOOTPRINT_VIEWER_FRAME::DClickOnFootprintList( wxCommandEvent& event )
{
    if( IsModal() )
    {
        // @todo(DICK)
        ExportSelectedFootprint( event );

        // Prevent the double click from being as a single mouse button release
        // event in the parent window which would cause the part to be parked
        // rather than staying in move mode.
        // Remember the mouse button will be released in the parent window
        // thus creating a mouse button release event which should be ignored
        PCB_EDIT_FRAME* pcbframe = dynamic_cast<PCB_EDIT_FRAME*>( GetParent() );

        // The parent may not be the board editor:
        if( pcbframe )
        {
            pcbframe->SkipNextLeftButtonReleaseEvent();
        }
    }
}


void FOOTPRINT_VIEWER_FRAME::ExportSelectedFootprint( wxCommandEvent& event )
{
    int ii = m_footprintList->GetSelection();

    if( ii >= 0 )
    {
        wxString fp_name = m_footprintList->GetString( ii );

        // @todo(DICK) assign to static now, later PROJECT retained string.
        m_selectedFootprintName = fp_name;

        FPID fpid;

        fpid.SetLibNickname( GetSelectedLibrary() );
        fpid.SetFootprintName( fp_name );

        DismissModal( true, fpid.Format() );
    }
    else
    {
        m_selectedFootprintName.Empty();
        DismissModal( false );
    }

    Close( true );
}


void FOOTPRINT_VIEWER_FRAME::LoadSettings( wxConfigBase* aCfg )
{
    EDA_DRAW_FRAME::LoadSettings( aCfg );
}


void FOOTPRINT_VIEWER_FRAME::SaveSettings( wxConfigBase* aCfg )
{
    EDA_DRAW_FRAME::SaveSettings( aCfg );
}


void FOOTPRINT_VIEWER_FRAME::OnActivate( wxActivateEvent& event )
{
    EDA_DRAW_FRAME::OnActivate( event );

    // Ensure we do not have old selection:
    if( ! m_FrameIsActive )
        return;

    m_selectedFootprintName.Empty();

    // Ensure we have the right library list:
    std::vector< wxString > libNicknames = Prj().PcbFootprintLibs()->GetLogicalLibs();

    if( libNicknames.size() == m_libList->GetCount() )
    {
        unsigned ii;

        for( ii = 0;  ii < libNicknames.size();  ii++ )
        {
            if( libNicknames[ii] != m_libList->GetString( ii ) )
                break;
        }

        if( ii == libNicknames.size() )
            return;
    }

    // If we are here, the library list has changed, rebuild it
    ReCreateLibraryList();
    UpdateTitle();
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

    case WXK_NUMPAD8:       // cursor moved up
    case WXK_UP:
        pos.y -= KiROUND( gridSize.y );
        m_canvas->MoveCursor( pos );
        break;

    case WXK_NUMPAD2:       // cursor moved down
    case WXK_DOWN:
        pos.y += KiROUND( gridSize.y );
        m_canvas->MoveCursor( pos );
        break;

    case WXK_NUMPAD4:       //  cursor moved left
    case WXK_LEFT:
        pos.x -= KiROUND( gridSize.x );
        m_canvas->MoveCursor( pos );
        break;

    case WXK_NUMPAD6:      //  cursor moved right
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

    UpdateStatusBar();    // Display new cursor coordinates
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

    m_Draw3DFrame = new EDA_3D_FRAME( &Kiway(), this, wxEmptyString );
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


void FOOTPRINT_VIEWER_FRAME::UpdateTitle()
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
    wxString selection = SelectLibrary( m_libraryName );

    if( !!selection && selection != m_libraryName )
    {
        m_libraryName = selection;

        UpdateTitle();
        ReCreateFootprintList();

        int id = m_libList->FindString( m_libraryName );

        if( id >= 0 )
            m_libList->SetSelection( id );
    }
}


void FOOTPRINT_VIEWER_FRAME::SelectCurrentFootprint( wxCommandEvent& event )
{
#if 0 // cannot remember why this is here
    // The PCB_EDIT_FRAME may not be the FOOTPRINT_VIEW_FRAME's parent,
    // so use Kiway().Player() to fetch.
    PCB_EDIT_FRAME* parent = (PCB_EDIT_FRAME*) Kiway().Player( FRAME_PCB, true );
    (void*) parent;
#endif

    wxString        libname = m_libraryName + wxT( "." ) + LegacyFootprintLibPathExtension;
    MODULE*         oldmodule = GetBoard()->m_Modules;
    MODULE*         module = LoadModuleFromLibrary( libname, Prj().PcbFootprintLibs(), false );

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
        m_footprintList->SetStringSelection( m_footprintName );
   }
}


const wxString FOOTPRINT_VIEWER_FRAME::GetSelectedLibraryFullName( void )
{
    wxString fullname = m_libraryName + wxT( "." ) + LegacyFootprintLibPathExtension;
    return fullname;
}


void FOOTPRINT_VIEWER_FRAME::SelectAndViewFootprint( int aMode )
{
    if( !m_libraryName )
        return;

    int selection = m_footprintList->FindString( m_footprintName );

    if( aMode == NEXT_PART )
    {
        if( selection != wxNOT_FOUND && selection < (int)m_footprintList->GetCount()-1 )
            selection++;
    }

    if( aMode == PREVIOUS_PART )
    {
        if( selection != wxNOT_FOUND && selection > 0 )
            selection--;
    }

    if( selection != wxNOT_FOUND )
    {
        m_footprintList->SetSelection( selection );
        m_footprintName = m_footprintList->GetString( selection );
        SetCurItem( NULL );

        // Delete the current footprint
        GetBoard()->m_Modules.DeleteAll();

        MODULE* footprint = Prj().PcbFootprintLibs()->FootprintLoad( m_libraryName, m_footprintName );

        if( footprint )
            GetBoard()->Add( footprint, ADD_APPEND );

        Update3D_Frame();
    }

    UpdateTitle();
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
