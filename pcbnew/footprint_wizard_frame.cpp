/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jaen-pierre.charras
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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
#include <macros.h>
#include <class_drawpanel.h>
#include <wxPcbStruct.h>
#include <3d_viewer.h>
#include <pcbcommon.h>

#include <class_board.h>
#include <class_module.h>

#include <pcbnew.h>
#include <pcbnew_id.h>
#include "footprint_wizard_frame.h"
#include <footprint_info.h>
#include <wx/grid.h>

#include <hotkeys.h>
#include <wildcards_and_files_ext.h>


BEGIN_EVENT_TABLE( FOOTPRINT_WIZARD_FRAME, EDA_DRAW_FRAME )
    /* Window events */
    EVT_CLOSE( FOOTPRINT_WIZARD_FRAME::OnCloseWindow )
    EVT_SIZE( FOOTPRINT_WIZARD_FRAME::OnSize )
    EVT_ACTIVATE( FOOTPRINT_WIZARD_FRAME::OnActivate )

    /* Sash drag events */
    EVT_SASH_DRAGGED( ID_FOOTPRINT_WIZARD_PAGES, FOOTPRINT_WIZARD_FRAME::OnSashDrag )
    EVT_SASH_DRAGGED( ID_FOOTPRINT_WIZARD_PARAMETERS, FOOTPRINT_WIZARD_FRAME::OnSashDrag )

    /* Toolbar events */
    EVT_TOOL( ID_FOOTPRINT_WIZARD_SELECT_WIZARD,
              FOOTPRINT_WIZARD_FRAME::SelectCurrentWizard)

    EVT_TOOL( ID_FOOTPRINT_WIZARD_NEXT,
              FOOTPRINT_WIZARD_FRAME::Process_Special_Functions )

    EVT_TOOL( ID_FOOTPRINT_WIZARD_PREVIOUS,
              FOOTPRINT_WIZARD_FRAME::Process_Special_Functions )
/*    EVT_TOOL( ID_FOOTPRINT_WIZARD_DONE,
              FOOTPRINT_WIZARD_FRAME::ExportSelectedFootprint )*/
    EVT_TOOL( ID_FOOTPRINT_WIZARD_SHOW_3D_VIEW, 
              FOOTPRINT_WIZARD_FRAME::Show3D_Frame )

    /* listbox events */
    EVT_LISTBOX( ID_FOOTPRINT_WIZARD_PAGE_LIST, FOOTPRINT_WIZARD_FRAME::ClickOnPageList )
    EVT_LISTBOX( ID_FOOTPRINT_WIZARD_PARAMETER_LIST, FOOTPRINT_WIZARD_FRAME::ClickOnParameterGrid )

    EVT_MENU( ID_SET_RELATIVE_OFFSET, FOOTPRINT_WIZARD_FRAME::OnSetRelativeOffset )
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


FOOTPRINT_WIZARD_FRAME::FOOTPRINT_WIZARD_FRAME( wxWindow* parent, wxSemaphore* semaphore ) :
    PCB_BASE_FRAME( parent, MODULE_VIEWER_FRAME, _( "Footprint Wizard" ),
                    wxDefaultPosition, wxDefaultSize )
{
    wxAcceleratorTable table( ACCEL_TABLE_CNT, accels );

    m_FrameName = wxT( "FootprintWizard" );
    m_configPath = wxT( "FootprintWizard" );
    m_showAxis = true;         // true to draw axis.

    // Give an icon
    wxIcon  icon;
    icon.CopyFromBitmap( KiBitmap( module_wizard_xpm) );
    SetIcon( icon );

    m_HotkeysZoomAndGridList = g_Module_Viewer_Hokeys_Descr;
    m_FootprintWizard = NULL;
    m_PageList= NULL;
    m_ParameterGrid = NULL;
    m_PageListWindow = NULL;
    m_ParameterGridWindow = NULL;
    m_Semaphore     = semaphore;
    m_wizardName.Empty();

    if( m_Semaphore )
        MakeModal(true);

    SetBoard( new BOARD() );
    // Ensure all layers and items are visible:
    GetBoard()->SetVisibleAlls();
    SetScreen( new PCB_SCREEN(GetPageSizeIU()) );
    GetScreen()->m_Center = true;      // Center coordinate origins on screen.
    LoadSettings();

    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );
    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId  );

    ReCreateHToolbar();
    ReCreateVToolbar();

    wxSize  size = GetClientSize();
    size.y -= m_MsgFrameHeight + 2;

    m_PageListSize.y = -1;

    wxPoint win_pos( 0, 0 );

     // Creates the libraries window display
    m_PageListWindow =
        new wxSashLayoutWindow( this, ID_FOOTPRINT_WIZARD_PAGES_WINDOW, win_pos,
                                wxDefaultSize, wxCLIP_CHILDREN | wxSW_3D,
                                wxT( "PagesWindow" ) );
    m_PageListWindow->SetOrientation( wxLAYOUT_VERTICAL );
    m_PageListWindow->SetAlignment( wxLAYOUT_LEFT );
    m_PageListWindow->SetSashVisible( wxSASH_RIGHT, true );
    m_PageListWindow->SetExtraBorderSize( EXTRA_BORDER_SIZE );
    m_PageList = new wxListBox( m_PageListWindow, ID_FOOTPRINT_WIZARD_PAGE_LIST,
                               wxPoint( 0, 0 ), wxDefaultSize,
                               0, NULL, wxLB_HSCROLL );

    // Creates the component window display
    m_ParameterGridSize.y = size.y;
    win_pos.x = m_PageListSize.x;
    m_ParameterGridWindow = new wxSashLayoutWindow( this, 
                                              ID_FOOTPRINT_WIZARD_PARAMETERS_WINDOW,
                                              win_pos, wxDefaultSize,
                                              wxCLIP_CHILDREN | wxSW_3D,
                                              wxT( "ParameterList" ) );
    
    m_ParameterGridWindow->SetOrientation( wxLAYOUT_VERTICAL );

    m_ParameterGridWindow->SetSashVisible( wxSASH_RIGHT, true );
    m_ParameterGridWindow->SetExtraBorderSize( EXTRA_BORDER_SIZE );
    m_ParameterGrid = new wxGrid(m_ParameterGridWindow,ID_FOOTPRINT_WIZARD_PARAMETER_LIST,
                                 wxPoint(0,0),wxDefaultSize);
    
    ReCreatePageList();

    DisplayWizardInfos();

    if( m_canvas )
        m_canvas->SetAcceleratorTable( table );

    m_auimgr.SetManagedWindow( this );


    EDA_PANEINFO horiz;
    horiz.HorizontalToolbarPane();

    EDA_PANEINFO vert;
    vert.VerticalToolbarPane();

    EDA_PANEINFO info;
    info.InfoToolbarPane();

    EDA_PANEINFO mesg;
    mesg.MessageToolbarPane();


    // Manage main toolbal
    m_auimgr.AddPane( m_mainToolBar,
                      wxAuiPaneInfo( horiz ).Name( wxT ("m_mainToolBar" ) ).Top().Row( 0 ) );

    wxSize minsize( 60, -1 );

    // Manage the left window (list of pages)
    if( m_PageListWindow )
        m_auimgr.AddPane( m_PageListWindow, wxAuiPaneInfo( info ).Name( wxT( "m_PageList" ) ).
                          Left().Row( 0 ));

    // Manage the list of parameters)
    m_auimgr.AddPane( m_ParameterGridWindow,
                      wxAuiPaneInfo( info ).Name( wxT( "m_ParameterGrid" ) ).
                      Left().Row( 1 ) );

    // Manage the draw panel
    m_auimgr.AddPane( m_canvas,
                      wxAuiPaneInfo().Name( wxT( "DrawFrame" ) ).Centre() );

    // Manage the message panel
    m_auimgr.AddPane( m_messagePanel,
                      wxAuiPaneInfo( mesg ).Name( wxT( "MsgPanel" ) ).Bottom().Layer(10) );

    /* Now the minimum windows are fixed, set library list
     * and component list of the previous values from last viewlib use
     */
    if( m_PageListWindow )
    {
        wxAuiPaneInfo& pane = m_auimgr.GetPane(m_PageListWindow);
        pane.MinSize( wxSize(m_PageListSize.x, -1));
    }
    wxAuiPaneInfo& pane = m_auimgr.GetPane(m_ParameterGridWindow);
    pane.MinSize(wxSize(m_ParameterGridSize.x, -1));

    m_auimgr.Update();

    // Now Drawpanel is sized, we can use BestZoom to show the component (if any)
#ifdef USE_WX_GRAPHICS_CONTEXT
    GetScreen()->SetZoom( BestZoom() );
#else
    Zoom_Automatique( false );
#endif

    
    Show( true );
    
    this->SelectFootprintWizard();
}


FOOTPRINT_WIZARD_FRAME::~FOOTPRINT_WIZARD_FRAME()
{
    if( m_Draw3DFrame )
        m_Draw3DFrame->Destroy();
    PCB_BASE_FRAME* frame = (PCB_BASE_FRAME*) GetParent();
    frame->m_ModuleViewerFrame = NULL;
}


void FOOTPRINT_WIZARD_FRAME::OnCloseWindow( wxCloseEvent& Event )
{
    SaveSettings();

    if( m_Semaphore )
    {
        m_Semaphore->Post();
        MakeModal(false);
        // This window will be destroyed by the calling function,
        // to avoid side effects
    }
    else
    {
        Destroy();
    }
}


void FOOTPRINT_WIZARD_FRAME::OnSashDrag( wxSashEvent& event )
{
    if( event.GetDragStatus() == wxSASH_STATUS_OUT_OF_RANGE )
        return;

    m_PageListSize.y = GetClientSize().y - m_MsgFrameHeight;
    m_ParameterGridSize.y = m_PageListSize.y;

    switch( event.GetId() )
    {
    case ID_FOOTPRINT_WIZARD_WINDOW:
        if( m_PageListWindow )
        {
            wxAuiPaneInfo& pane = m_auimgr.GetPane( m_PageListWindow );
            m_PageListSize.x = event.GetDragRect().width;
            pane.MinSize( m_PageListSize );
            m_auimgr.Update();
        }
        break;

    case ID_FOOTPRINT_WIZARD_PARAMETERS_WINDOW:
    {
        wxAuiPaneInfo& pane = m_auimgr.GetPane( m_ParameterGridWindow );
        m_ParameterGridSize.x = event.GetDragRect().width;
        pane.MinSize( m_ParameterGridSize );
        m_auimgr.Update();
    }
        break;
    }
}


void FOOTPRINT_WIZARD_FRAME::OnSize( wxSizeEvent& SizeEv )
{
    if( m_auimgr.GetManagedWindow() )
        m_auimgr.Update();

    SizeEv.Skip();
}


void FOOTPRINT_WIZARD_FRAME::OnSetRelativeOffset( wxCommandEvent& event )
{
    GetScreen()->m_O_Curseur = GetScreen()->GetCrossHairPosition();
    UpdateStatusBar();
}


void FOOTPRINT_WIZARD_FRAME::ReCreatePageList()
{
    if( m_PageList == NULL )
        return;
    
    if (m_FootprintWizard == NULL)
        return;

    m_PageList->Clear();
    int max_page = m_FootprintWizard->GetNumParameterPages();
    for (int i=0;i<max_page;i++)
    {
        wxString name = m_FootprintWizard->GetParameterPageName(i);
        m_PageList->Append(name);
    }
        
    m_PageList->SetSelection( 0, true );
    
    ReCreateParameterList();
    ReCreateHToolbar();
    DisplayWizardInfos();
    m_canvas->Refresh();
}


void FOOTPRINT_WIZARD_FRAME::ReCreateParameterList()
{
    if( m_ParameterGrid == NULL )
        return;
    
    if (m_FootprintWizard == NULL )
        return;
    
    int page = m_PageList->GetSelection();
    
    if (page<0)
        return;

    m_ParameterGrid->ClearGrid();
    
    
    // Columns
    m_ParameterGrid->AutoSizeColumns();
    m_ParameterGrid->SetColLabelSize( 20 );
    m_ParameterGrid->SetColLabelValue( 0, _("Parameter") );
    m_ParameterGrid->SetColLabelValue( 1, _("Value") );
    m_ParameterGrid->SetColLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTRE );
    
    
    // Rows
    m_ParameterGrid->AutoSizeRows();
    m_ParameterGrid->EnableDragRowSize( true );
    m_ParameterGrid->SetRowLabelSize( 1 );
    m_ParameterGrid->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
    
    

    wxArrayString fpList = m_FootprintWizard->GetParameterNames(page);
    wxArrayString fvList = m_FootprintWizard->GetParameterValues(page);
    
    m_ParameterGrid->CreateGrid(fpList.size(),2);
    
    for (unsigned int i=0;i<fpList.size();i++)
    {
        m_ParameterGrid->SetCellValue( i, 0, fpList[i] );
        m_ParameterGrid->SetReadOnly( i, 0 );
        m_ParameterGrid->SetCellValue( i, 1 , fvList[i] );
    }
    m_ParameterGrid->AutoSizeColumns();
    

}


void FOOTPRINT_WIZARD_FRAME::ClickOnPageList( wxCommandEvent& event )
{
    int ii = m_PageList->GetSelection();

    if( ii < 0 )
        return;    
    
    ReCreateParameterList();
    m_canvas->Refresh();
    DisplayWizardInfos();
}


void FOOTPRINT_WIZARD_FRAME::ClickOnParameterGrid( wxCommandEvent& event )
{
    int n=m_ParameterGrid->GetNumberRows();
    
    for (int i=0;i<n;i++)
    {
        // Get values, send them to the object..
    }
    
    
    ReloadFootprint();
    DisplayWizardInfos();
    
}


#define PARTLIST_WIDTH_KEY wxT( "Partlist_width" )
#define PARAMLIST_WIDTH_KEY wxT( "Paramlist_width" )


void FOOTPRINT_WIZARD_FRAME::LoadSettings( )
{
    wxConfig* cfg ;

    EDA_DRAW_FRAME::LoadSettings();

    wxConfigPathChanger cpc( wxGetApp().GetSettings(), m_configPath );
    cfg = wxGetApp().GetSettings();

    m_PageListSize.x = 150; // default width of libs list
    m_ParameterGridSize.x = 250; // default width of component list

    cfg->Read( PARTLIST_WIDTH_KEY , &m_PageListSize.x );
    cfg->Read( PARAMLIST_WIDTH_KEY, &m_ParameterGridSize.x );

    // Set parameters to a reasonable value.
    if ( m_PageListSize.x > m_FrameSize.x/2 )
        m_PageListSize.x = m_FrameSize.x/2;

    if ( m_ParameterGridSize.x > m_FrameSize.x/2 )
        m_ParameterGridSize.x = m_FrameSize.x/2;
}


void FOOTPRINT_WIZARD_FRAME::SaveSettings()
{
    wxConfig* cfg;

    EDA_DRAW_FRAME::SaveSettings();

    wxConfigPathChanger cpc( wxGetApp().GetSettings(), m_configPath );
    cfg = wxGetApp().GetSettings();

    if ( m_PageListSize.x )
        cfg->Write( PARTLIST_WIDTH_KEY, m_PageListSize.x );

    cfg->Write( PARAMLIST_WIDTH_KEY, m_ParameterGridSize.x );
}


void FOOTPRINT_WIZARD_FRAME::OnActivate( wxActivateEvent& event )
{
    EDA_DRAW_FRAME::OnActivate( event );

    // Ensure we do not have old selection:
    if( ! m_FrameIsActive )
        return;

    bool footprintWizardsChanged=false;
    if (footprintWizardsChanged)
    {
       // If we are here, the library list has changed, rebuild it
        ReCreatePageList();
        DisplayWizardInfos();
        
    }
}


void FOOTPRINT_WIZARD_FRAME::GeneralControl( wxDC* aDC, const wxPoint& aPosition, int aHotKey )
{
    wxRealPoint gridSize;
    wxPoint     oldpos;
    PCB_SCREEN* screen = GetScreen();
    wxPoint     pos = aPosition;

    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );
    cmd.SetEventObject( this );

    pos = screen->GetNearestGridPosition( pos );
    oldpos = screen->GetCrossHairPosition();
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
        screen->m_O_Curseur = screen->GetCrossHairPosition();
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

    screen->SetCrossHairPosition( pos );

    if( oldpos != screen->GetCrossHairPosition() )
    {
        pos = screen->GetCrossHairPosition();
        screen->SetCrossHairPosition( oldpos );
        m_canvas->CrossHairOff( aDC );
        screen->SetCrossHairPosition( pos );
        m_canvas->CrossHairOn( aDC );

        if( m_canvas->IsMouseCaptured() )
        {
            m_canvas->CallMouseCapture( aDC, aPosition, 0 );
        }
    }

    UpdateStatusBar();    /* Display new cursor coordinates */
}


void FOOTPRINT_WIZARD_FRAME::Show3D_Frame( wxCommandEvent& event )
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

/**
 * Function Update3D_Frame
 * must be called after a footprint selection
 * Updates the 3D view and 3D frame title.
 */
void FOOTPRINT_WIZARD_FRAME::Update3D_Frame( bool aForceReloadFootprint )
{
    if( m_Draw3DFrame == NULL )
        return;

    wxString frm3Dtitle;
    frm3Dtitle.Printf( _( "ModView: 3D Viewer [%s]" ), GetChars( m_wizardName ) );
    m_Draw3DFrame->SetTitle( frm3Dtitle );

    if( aForceReloadFootprint )
    {
        m_Draw3DFrame->ReloadRequest();
        // Force 3D screen refresh immediately
        if( GetBoard()->m_Modules )
            m_Draw3DFrame->NewDisplay();
    }
}


void FOOTPRINT_WIZARD_FRAME::ReCreateHToolbar()
{
    wxString msg;

    if( m_mainToolBar  == NULL )
    {
        m_mainToolBar = new wxAuiToolBar( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                          wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORZ_LAYOUT );

        // Set up toolbar
        m_mainToolBar->AddTool( ID_FOOTPRINT_WIZARD_SELECT_WIZARD, wxEmptyString,
                                KiBitmap( library_xpm ),
                                _( "Select wizard to use" ) );

        m_mainToolBar->AddSeparator();
        m_mainToolBar->AddTool( ID_FOOTPRINT_WIZARD_PREVIOUS, wxEmptyString,
                                KiBitmap( lib_previous_xpm ),
                                _( "Display previous page" ) );

        m_mainToolBar->AddTool( ID_FOOTPRINT_WIZARD_NEXT, wxEmptyString,
                                KiBitmap( lib_next_xpm ),
                                _( "Display next page" ) );

        m_mainToolBar->AddSeparator();
        m_mainToolBar->AddTool( ID_FOOTPRINT_WIZARD_SHOW_3D_VIEW, wxEmptyString,
                                KiBitmap( three_d_xpm ),
                                _( "Show footprint in 3D viewer" ) );

        m_mainToolBar->AddSeparator();
        msg = AddHotkeyName( _( "Zoom in" ), g_Module_Editor_Hokeys_Descr,
                             HK_ZOOM_IN, IS_COMMENT );
        m_mainToolBar->AddTool( ID_ZOOM_IN, wxEmptyString,
                                KiBitmap( zoom_in_xpm ), msg );

        msg = AddHotkeyName( _( "Zoom out" ), g_Module_Editor_Hokeys_Descr,
                             HK_ZOOM_OUT, IS_COMMENT );
        m_mainToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString,
                                KiBitmap( zoom_out_xpm ), msg );

        msg = AddHotkeyName( _( "Redraw view" ), g_Module_Editor_Hokeys_Descr,
                             HK_ZOOM_REDRAW, IS_COMMENT );
        m_mainToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString,
                             KiBitmap( zoom_redraw_xpm ), msg );

        msg = AddHotkeyName( _( "Zoom auto" ), g_Module_Editor_Hokeys_Descr,
                             HK_ZOOM_AUTO, IS_COMMENT );
        m_mainToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString,
                                KiBitmap( zoom_fit_in_page_xpm ), msg );
#if 0 
        if( m_Semaphore )
        {
            // The library browser is called from a "load component" command
            m_mainToolBar->AddSeparator();
            m_mainToolBar->AddTool( ID_FOOTPRINT_WIZARD_FOOTPRINT_EXPORT_TO_BOARD,
                                    wxEmptyString, KiBitmap( export_footprint_names_xpm ),
                                    _( "Insert footprint in board" ) );
        }
#endif
        // after adding the buttons to the toolbar, must call Realize() to
        // reflect the changes
        m_mainToolBar->Realize();
    }

    m_mainToolBar->Refresh();
}


void FOOTPRINT_WIZARD_FRAME::ReCreateVToolbar()
{
}
