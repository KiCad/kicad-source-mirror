/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2015 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2015 KiCad Developers, see change_log.txt for contributors.
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
#include <class_drawpanel.h>
#include <pcb_draw_panel_gal.h>
#include <wxPcbStruct.h>
#include <3d_viewer.h>
#include <dialog_helpers.h>
#include <msgpanel.h>
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

#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include "tools/pcbnew_control.h"
#include "tools/common_actions.h"

#include <boost/bind.hpp>


#define NEXT_PART       1
#define NEW_PART        0
#define PREVIOUS_PART   -1


BEGIN_EVENT_TABLE( FOOTPRINT_VIEWER_FRAME, EDA_DRAW_FRAME )
    // Window events
    EVT_CLOSE( FOOTPRINT_VIEWER_FRAME::OnCloseWindow )
    EVT_SIZE( FOOTPRINT_VIEWER_FRAME::OnSize )
    EVT_ACTIVATE( FOOTPRINT_VIEWER_FRAME::OnActivate )

    // Menu (and/or hotkey) events
    EVT_MENU( wxID_EXIT, FOOTPRINT_VIEWER_FRAME::CloseFootprintViewer )
    EVT_MENU( ID_SET_RELATIVE_OFFSET, FOOTPRINT_VIEWER_FRAME::OnSetRelativeOffset )

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

END_EVENT_TABLE()


/* Note:
 * FOOTPRINT_VIEWER_FRAME can be build in "modal mode", or as a usual frame.
 * In modal mode:
 *  a tool to export the selected footprint is shown in the toolbar
 *  the style is wxSTAY_ON_TOP on Windows and wxFRAME_FLOAT_ON_PARENT on unix
 * reason:
 * the parent is usually the kicad window manager (not easy to change)
 * On windows, when the frame with stype wxFRAME_FLOAT_ON_PARENT is displayed
 * its parent frame is brought to the foreground, on the top of the calling frame.
 * and stays displayed when closing the FOOTPRINT_VIEWER_FRAME frame.
 * this issue does not happen on unix
 *
 * So we use wxSTAY_ON_TOP on Windows, and wxFRAME_FLOAT_ON_PARENT on unix
 * to simulate a dialog called by ShowModal.
 */


#define FOOTPRINT_VIEWER_FRAME_NAME     wxT( "ModViewFrame" )


FOOTPRINT_VIEWER_FRAME::FOOTPRINT_VIEWER_FRAME( KIWAY* aKiway, wxWindow* aParent, FRAME_T aFrameType ) :
    PCB_BASE_FRAME( aKiway, aParent, aFrameType, _( "Footprint Library Browser" ),
            wxDefaultPosition, wxDefaultSize,
            aFrameType == FRAME_PCB_MODULE_VIEWER_MODAL ?
#ifdef __WINDOWS__
                KICAD_DEFAULT_DRAWFRAME_STYLE | wxSTAY_ON_TOP :
#else
                KICAD_DEFAULT_DRAWFRAME_STYLE | wxFRAME_FLOAT_ON_PARENT :
#endif
                KICAD_DEFAULT_DRAWFRAME_STYLE,
            GetFootprintViewerFrameName() )
{
    wxASSERT( aFrameType==FRAME_PCB_MODULE_VIEWER || aFrameType==FRAME_PCB_MODULE_VIEWER_MODAL );

    if( aFrameType == FRAME_PCB_MODULE_VIEWER_MODAL )
        SetModal( true );

    m_configPath = wxT( "FootprintViewer" );
    m_showAxis   = true;         // true to draw axis.

    // Give an icon
    wxIcon  icon;
    icon.CopyFromBitmap( KiBitmap( modview_icon_xpm ) );
    SetIcon( icon );

    m_hotkeysDescrList = g_Module_Viewer_Hokeys_Descr;

    m_libList = new wxListBox( this, ID_MODVIEW_LIB_LIST,
            wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_HSCROLL );

    m_footprintList = new wxListBox( this, ID_MODVIEW_FOOTPRINT_LIST,
            wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_HSCROLL );

    SetBoard( new BOARD() );
    // In viewer, the default net clearance is not known (it depends on the actual board).
    // So we do not show the default clearance, by setting it to 0
    // The footprint or pad specific clearance will be shown
    GetBoard()->GetDesignSettings().GetDefault()->SetClearance(0);

    // Ensure all layers and items are visible:
    GetBoard()->SetVisibleAlls();
    SetScreen( new PCB_SCREEN( GetPageSizeIU() ) );

    GetScreen()->m_Center = true;      // Center coordinate origins on screen.
    LoadSettings( config() );

    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId  );

    // Menu bar is not mandatory: uncomment/comment the next line
    // to add/remove the menubar
    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateVToolbar();

    ReCreateLibraryList();
    UpdateTitle();

    PCB_BASE_FRAME* parentFrame = static_cast<PCB_BASE_FRAME*>( Kiway().Player( FRAME_PCB, true ) );

    // Create GAL canvas
    PCB_DRAW_PANEL_GAL* drawPanel = new PCB_DRAW_PANEL_GAL( this, -1, wxPoint( 0, 0 ), m_FrameSize,
                                                            parentFrame->GetGalCanvas()->GetBackend() );
    SetGalCanvas( drawPanel );

    // Create the manager and dispatcher & route draw panel events to the dispatcher
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( GetBoard(), drawPanel->GetView(),
                                   drawPanel->GetViewControls(), this );
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager );
    drawPanel->SetEventDispatcher( m_toolDispatcher );

    m_toolManager->RegisterTool( new PCBNEW_CONTROL );
    m_toolManager->ResetTools( TOOL_BASE::RUN );

    // If a footprint was previously loaded, reload it
    if( getCurNickname().size() && getCurFootprintName().size() )
    {
        FPID id;

        id.SetLibNickname( getCurNickname() );
        id.SetFootprintName( getCurFootprintName() );
        GetBoard()->Add( loadFootprint( id ) );
    }

    drawPanel->DisplayBoard( m_Pcb );
    updateView();

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
    m_auimgr.AddPane( (wxWindow*) GetGalCanvas(),
                      wxAuiPaneInfo().Name( wxT( "DrawFrameGal" ) ).CentrePane().Hide() );

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

    // after changing something to the aui manager,
    // call Update()() to reflect the changes
    m_auimgr.Update();

    // Now Drawpanel is sized, we can use BestZoom to show the component (if any)
#ifdef USE_WX_GRAPHICS_CONTEXT
    GetScreen()->SetZoom( BestZoom() );
#else
    Zoom_Automatique( false );
#endif

    UseGalCanvas( parentFrame->IsGalCanvasActive() );

    if( !IsModal() )        // For modal mode, calling ShowModal() will show this frame
    {
        Raise();            // On some window managers, this is needed
        Show( true );
    }
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

    if( IsGalCanvasActive() )
        GetGalCanvas()->StopDrawing();

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
    int index =  m_libList->FindString( getCurNickname() );

    if( index != wxNOT_FOUND )
    {
        m_libList->SetSelection( index, true );
    }
    else
    {
        // If not found, clear current library selection because it can be
        // deleted after a configuration change.
        setCurNickname( wxEmptyString );
        setCurFootprintName( wxEmptyString );
    }

    ReCreateFootprintList();
    ReCreateHToolbar();

    m_canvas->Refresh();
}


void FOOTPRINT_VIEWER_FRAME::UseGalCanvas( bool aEnable )
{
    EDA_DRAW_FRAME::UseGalCanvas( aEnable );

    if( aEnable )
        GetGalCanvas()->StartDrawing();
}


void FOOTPRINT_VIEWER_FRAME::ReCreateFootprintList()
{
    m_footprintList->Clear();

    if( !getCurNickname() )
    {
        setCurFootprintName( wxEmptyString );
        return;
    }

    FOOTPRINT_LIST fp_info_list;

    wxString nickname = getCurNickname();

    fp_info_list.ReadFootprintFiles( Prj().PcbFootprintLibs(), !nickname ? NULL : &nickname );

    if( fp_info_list.GetErrorCount() )
    {
        fp_info_list.DisplayErrors( this );
        return;
    }

    BOOST_FOREACH( const FOOTPRINT_INFO& footprint, fp_info_list.GetList() )
    {
        m_footprintList->Append( footprint.GetFootprintName() );
    }

    int index = m_footprintList->FindString( getCurFootprintName() );

    if( index == wxNOT_FOUND )
        setCurFootprintName( wxEmptyString );
    else
        m_footprintList->SetSelection( index, true );
}


void FOOTPRINT_VIEWER_FRAME::ClickOnLibList( wxCommandEvent& event )
{
    int ii = m_libList->GetSelection();

    if( ii < 0 )
        return;

    wxString name = m_libList->GetString( ii );

    if( getCurNickname() == name )
        return;

    setCurNickname( name );

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

    if( getCurFootprintName().CmpNoCase( name ) != 0 )
    {
        setCurFootprintName( name );

        SetCurItem( NULL );

        // Delete the current footprint
        GetBoard()->m_Modules.DeleteAll();

        FPID id;
        id.SetLibNickname( getCurNickname() );
        id.SetFootprintName( getCurFootprintName() );

        try
        {
            GetBoard()->Add( loadFootprint( id ) );
        }
        catch( const IO_ERROR& ioe )
        {
            wxString msg = wxString::Format(
                        _( "Could not load footprint \"%s\" from library \"%s\".\n\nError %s." ),
                        GetChars( getCurFootprintName() ),
                        GetChars( getCurNickname() ),
                        GetChars( ioe.errorText ) );

            DisplayError( this, msg );
        }

        UpdateTitle();

        if( IsGalCanvasActive() )
            updateView();

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

        FPID fpid;

        fpid.SetLibNickname( getCurNickname() );
        fpid.SetFootprintName( fp_name );

        DismissModal( true, fpid.Format() );
    }
    else
    {
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


const wxString FOOTPRINT_VIEWER_FRAME::getCurNickname()
{
    return Prj().GetRString( PROJECT::PCB_FOOTPRINT_VIEWER_NICKNAME );
}


void FOOTPRINT_VIEWER_FRAME::setCurNickname( const wxString& aNickname )
{
    Prj().SetRString( PROJECT::PCB_FOOTPRINT_VIEWER_NICKNAME, aNickname );
}


const wxString FOOTPRINT_VIEWER_FRAME::getCurFootprintName()
{
    return Prj().GetRString( PROJECT::PCB_FOOTPRINT_VIEWER_FPNAME );
}


void FOOTPRINT_VIEWER_FRAME::setCurFootprintName( const wxString& aName )
{
    Prj().SetRString( PROJECT::PCB_FOOTPRINT_VIEWER_FPNAME, aName );
}


void FOOTPRINT_VIEWER_FRAME::OnActivate( wxActivateEvent& event )
{
    EDA_DRAW_FRAME::OnActivate( event );

    // Ensure we do not have old selection:
    if( !event.GetActive() )
        return;

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


bool FOOTPRINT_VIEWER_FRAME::GeneralControl( wxDC* aDC, const wxPoint& aPosition, int aHotKey )
{
    bool eventHandled = true;

    // Filter out the 'fake' mouse motion after a keyboard movement
    if( !aHotKey && m_movingCursorWithKeyboard )
    {
        m_movingCursorWithKeyboard = false;
        return false;
    }

    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );
    cmd.SetEventObject( this );

    wxPoint oldpos = GetCrossHairPosition();
    wxPoint pos = aPosition;
    GeneralControlKeyMovement( aHotKey, &pos, true );

    if( aHotKey )
    {
        eventHandled = OnHotKey( aDC, aHotKey, aPosition );
    }

    SetCrossHairPosition( pos );
    RefreshCrossHair( oldpos, aPosition, aDC );

    UpdateStatusBar();    // Display new cursor coordinates

    return eventHandled;
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

    wxString frm3Dtitle = wxString::Format(
                _( "ModView: 3D Viewer [%s]" ),
                GetChars( getCurFootprintName() ) );

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
        wxString id = wxString::Format(wxT("%i"),event.GetId());
        wxFAIL_MSG( wxT( "FOOTPRINT_VIEWER_FRAME::OnIterateFootprintList error: id = " ) + id );
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

    if( getCurNickname().size() )
        msg << getCurNickname();
    else
        msg += _( "no library selected" );

    msg << wxT( "]" );

    SetTitle( msg );
}


void FOOTPRINT_VIEWER_FRAME::SelectCurrentLibrary( wxCommandEvent& event )
{
    wxString selection = SelectLibrary( getCurNickname() );

    if( !!selection && selection != getCurNickname() )
    {
        setCurNickname( selection );

        UpdateTitle();
        ReCreateFootprintList();

        int id = m_libList->FindString( getCurNickname() );

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

    wxString        nickname = getCurNickname();
    MODULE*         oldmodule = GetBoard()->m_Modules;
    MODULE*         module = LoadModuleFromLibrary( nickname, Prj().PcbFootprintLibs(), false );

    if( module )
    {
        module->SetPosition( wxPoint( 0, 0 ) );

        // Only one footprint allowed: remove the previous footprint (if exists)
        if( oldmodule )
        {
            GetBoard()->Remove( oldmodule );
            delete oldmodule;
        }

        setCurFootprintName( module->GetFPID().GetFootprintName() );

        wxString nickname = module->GetFPID().GetLibNickname();

        if( !getCurNickname() && nickname.size() )
        {
            // Set the listbox
            int index =  m_libList->FindString( nickname );
            if( index != wxNOT_FOUND )
                m_libList->SetSelection( index, true );

            setCurNickname( nickname );
        }

        module->ClearFlags();
        SetCurItem( NULL );

        Zoom_Automatique( false );
        m_canvas->Refresh();
        Update3D_Frame();
        m_footprintList->SetStringSelection( getCurFootprintName() );
   }
}


void FOOTPRINT_VIEWER_FRAME::SelectAndViewFootprint( int aMode )
{
    if( !getCurNickname() )
        return;

    int selection = m_footprintList->FindString( getCurFootprintName() );

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
        setCurFootprintName( m_footprintList->GetString( selection ) );
        SetCurItem( NULL );

        // Delete the current footprint
        GetBoard()->m_Modules.DeleteAll();

        MODULE* footprint = Prj().PcbFootprintLibs()->FootprintLoad(
                                getCurNickname(), getCurFootprintName() );

        if( footprint )
            GetBoard()->Add( footprint, ADD_APPEND );

        Update3D_Frame();

        if( IsGalCanvasActive() )
            updateView();
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


void FOOTPRINT_VIEWER_FRAME::updateView()
{
    if( IsGalCanvasActive() )
    {
        static_cast<PCB_DRAW_PANEL_GAL*>( GetGalCanvas() )->DisplayBoard( GetBoard() );
        m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );
        m_toolManager->RunAction( COMMON_ACTIONS::zoomFitScreen, true );
    }
}


void FOOTPRINT_VIEWER_FRAME::CloseFootprintViewer( wxCommandEvent& event )
{
    Close();
}
