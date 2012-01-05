/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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
 * @file viewlib_frame.cpp
 */

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "eeschema_id.h"
#include "class_drawpanel.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "protos.h"
#include "viewlib_frame.h"
#include "class_library.h"
#include "hotkeys.h"
#include "dialog_helpers.h"


/**
 * Save previous component library viewer state.
 */
wxString LIB_VIEW_FRAME::m_libraryName;
wxString LIB_VIEW_FRAME::m_entryName;
int LIB_VIEW_FRAME::m_unit = 1;
int LIB_VIEW_FRAME::m_convert = 1;


/// When the viewer is used to select a component in schematic, the selected component is here.
wxString LIB_VIEW_FRAME::m_exportToEeschemaCmpName;


BEGIN_EVENT_TABLE( LIB_VIEW_FRAME, EDA_DRAW_FRAME )
    /* Window events */
    EVT_CLOSE( LIB_VIEW_FRAME::OnCloseWindow )
    EVT_SIZE( LIB_VIEW_FRAME::OnSize )
    EVT_ACTIVATE( LIB_VIEW_FRAME::OnActivate )

    /* Sash drag events */
    EVT_SASH_DRAGGED( ID_LIBVIEW_LIBWINDOW, LIB_VIEW_FRAME::OnSashDrag )
    EVT_SASH_DRAGGED( ID_LIBVIEW_CMPWINDOW, LIB_VIEW_FRAME::OnSashDrag )

    /* Toolbar events */
    EVT_TOOL_RANGE( ID_LIBVIEW_NEXT, ID_LIBVIEW_DE_MORGAN_CONVERT_BUTT,
                    LIB_VIEW_FRAME::Process_Special_Functions )

    EVT_TOOL( ID_LIBVIEW_CMP_EXPORT_TO_SCHEMATIC, LIB_VIEW_FRAME::ExportToSchematicLibraryPart )
    EVT_COMBOBOX( ID_LIBVIEW_SELECT_PART_NUMBER, LIB_VIEW_FRAME::Process_Special_Functions )

    /* listbox events */
    EVT_LISTBOX( ID_LIBVIEW_LIB_LIST, LIB_VIEW_FRAME::ClickOnLibList )
    EVT_LISTBOX( ID_LIBVIEW_CMP_LIST, LIB_VIEW_FRAME::ClickOnCmpList )

    EVT_MENU( ID_SET_RELATIVE_OFFSET, LIB_VIEW_FRAME::OnSetRelativeOffset )
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


LIB_VIEW_FRAME::LIB_VIEW_FRAME( wxWindow* father, CMP_LIBRARY* Library, wxSemaphore* semaphore ) :
    EDA_DRAW_FRAME( father, VIEWER_FRAME, _( "Library Browser" ),
                    wxDefaultPosition, wxDefaultSize )
{
    wxAcceleratorTable table( ACCEL_TABLE_CNT, accels );

    m_FrameName = wxT( "ViewlibFrame" );
    m_configPath = wxT( "LibraryViewer" );

    // Give an icon
    wxIcon  icon;
    icon.CopyFromBitmap( KiBitmap( library_browse_xpm ) );

    SetIcon( icon );

    m_HotkeysZoomAndGridList = s_Viewlib_Hokeys_Descr;
    m_CmpList = NULL;
    m_LibList = NULL;
    m_LibListWindow = NULL;
    m_CmpListWindow = NULL;
    m_Semaphore     = semaphore;
    m_exportToEeschemaCmpName.Empty();

    if( m_Semaphore )
        MakeModal(true);

    SetScreen( new SCH_SCREEN() );
    GetScreen()->m_Center = true;      // Center coordinate origins on screen.
    LoadSettings();

    // Initialize grid id to a default value if not found in config or bad:
    if( ( m_LastGridSizeId <= 0 ) ||
        ( m_LastGridSizeId < ( ID_POPUP_GRID_USER - ID_POPUP_GRID_LEVEL_1000 ) ) )
        m_LastGridSizeId = ID_POPUP_GRID_LEVEL_50 - ID_POPUP_GRID_LEVEL_1000;

    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );
    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId  );

    ReCreateHToolbar();
    ReCreateVToolbar();

    wxSize  size = GetClientSize();
    size.y -= m_MsgFrameHeight + 2;

    m_LibListSize.y = -1;

    wxPoint win_pos( 0, 0 );

    if( Library == NULL )
    {
        // Creates the libraries window display
        m_LibListWindow =
            new wxSashLayoutWindow( this, ID_LIBVIEW_LIBWINDOW, win_pos,
                                    wxDefaultSize, wxCLIP_CHILDREN | wxSW_3D,
                                    wxT( "LibWindow" ) );
        m_LibListWindow->SetOrientation( wxLAYOUT_VERTICAL );
        m_LibListWindow->SetAlignment( wxLAYOUT_LEFT );
        m_LibListWindow->SetSashVisible( wxSASH_RIGHT, true );
        m_LibListWindow->SetExtraBorderSize( EXTRA_BORDER_SIZE );
        m_LibList = new wxListBox( m_LibListWindow, ID_LIBVIEW_LIB_LIST,
                                   wxPoint( 0, 0 ), wxDefaultSize,
                                   0, NULL, wxLB_HSCROLL );
    }
    else
    {
        m_libraryName = Library->GetName();
        m_entryName.Clear();
        m_unit = 1;
        m_convert = 1;
        m_LibListSize.x = 0;
    }

    // Creates the component window display
    m_CmpListSize.y = size.y;
    win_pos.x = m_LibListSize.x;
    m_CmpListWindow = new wxSashLayoutWindow( this, ID_LIBVIEW_CMPWINDOW,
                                              win_pos, wxDefaultSize,
                                              wxCLIP_CHILDREN | wxSW_3D,
                                              wxT( "CmpWindow" ) );
    m_CmpListWindow->SetOrientation( wxLAYOUT_VERTICAL );

    m_CmpListWindow->SetSashVisible( wxSASH_RIGHT, true );
    m_CmpListWindow->SetExtraBorderSize( EXTRA_BORDER_SIZE );
    m_CmpList = new wxListBox( m_CmpListWindow, ID_LIBVIEW_CMP_LIST,
                               wxPoint( 0, 0 ), wxDefaultSize,
                               0, NULL, wxLB_HSCROLL );

    if( m_LibList )
        ReCreateListLib();

    DisplayLibInfos();

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

    // Manage the left window (list of libraries)
    if( m_LibListWindow )
        m_auimgr.AddPane( m_LibListWindow, wxAuiPaneInfo( info ).Name( wxT( "m_LibList" ) ).
                          Left().Row( 0 ));

    // Manage the list of components)
    m_auimgr.AddPane( m_CmpListWindow,
                      wxAuiPaneInfo( info ).Name( wxT( "m_CmpList" ) ).
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
    if( m_LibListWindow )
    {
        wxAuiPaneInfo& pane = m_auimgr.GetPane(m_LibListWindow);
        pane.MinSize( wxSize(m_LibListSize.x, -1));
    }
    wxAuiPaneInfo& pane = m_auimgr.GetPane(m_CmpListWindow);
    pane.MinSize(wxSize(m_CmpListSize.x, -1));

    m_auimgr.Update();

    // Now Drawpanel is sized, we can use BestZoom to show the component (if any)
#ifdef USE_WX_GRAPHICS_CONTEXT
    GetScreen()->SetZoom( BestZoom() );
#else
    Zoom_Automatique( false );
#endif

    Show( true );
}


LIB_VIEW_FRAME::~LIB_VIEW_FRAME()
{
    SCH_EDIT_FRAME* frame = (SCH_EDIT_FRAME*) wxGetApp().GetTopWindow();
    frame->SetLibraryViewerWindow( NULL );
}


void LIB_VIEW_FRAME::SetPageSettings( const PAGE_INFO& aPageSettings )
{
    GetScreen()->SetPageSettings( aPageSettings );
}


const PAGE_INFO& LIB_VIEW_FRAME::GetPageSettings () const
{
    return GetScreen()->GetPageSettings();
}


const wxSize LIB_VIEW_FRAME::GetPageSizeIU() const
{
    // GetSizeIU is compile time dependent:
    return GetScreen()->GetPageSettings().GetSizeIU();
}


const wxPoint& LIB_VIEW_FRAME::GetOriginAxisPosition() const
{
    wxASSERT( GetScreen() );
    return GetScreen()->GetOriginAxisPosition();
}


void LIB_VIEW_FRAME::SetOriginAxisPosition( const wxPoint& aPosition )
{
    wxASSERT( GetScreen() );
    GetScreen()->SetOriginAxisPosition( aPosition );
}


void LIB_VIEW_FRAME::OnCloseWindow( wxCloseEvent& Event )
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


void LIB_VIEW_FRAME::OnSashDrag( wxSashEvent& event )
{
    if( event.GetDragStatus() == wxSASH_STATUS_OUT_OF_RANGE )
        return;

    m_LibListSize.y = GetClientSize().y - m_MsgFrameHeight;
    m_CmpListSize.y = m_LibListSize.y;

    switch( event.GetId() )
    {
    case ID_LIBVIEW_LIBWINDOW:
        if( m_LibListWindow )
        {
            wxAuiPaneInfo& pane = m_auimgr.GetPane( m_LibListWindow );
            m_LibListSize.x = event.GetDragRect().width;
            pane.MinSize( m_LibListSize );
            m_auimgr.Update();
        }
        break;

    case ID_LIBVIEW_CMPWINDOW:
    {
        wxAuiPaneInfo& pane = m_auimgr.GetPane( m_CmpListWindow );
        m_CmpListSize.x = event.GetDragRect().width;
        pane.MinSize( m_CmpListSize );
        m_auimgr.Update();
    }
        break;
    }
}


void LIB_VIEW_FRAME::OnSize( wxSizeEvent& SizeEv )
{
    if( m_auimgr.GetManagedWindow() )
        m_auimgr.Update();

    SizeEv.Skip();
}


void LIB_VIEW_FRAME::OnSetRelativeOffset( wxCommandEvent& event )
{
    GetScreen()->m_O_Curseur = GetScreen()->GetCrossHairPosition();
    UpdateStatusBar();
}


double LIB_VIEW_FRAME::BestZoom()
{
/* Please, note: wxMSW before version 2.9 seems have
 * problems with zoom values < 1 ( i.e. userscale > 1) and needs to be patched:
 * edit file <wxWidgets>/src/msw/dc.cpp
 * search for line static const int VIEWPORT_EXTENT = 1000;
 * and replace by static const int VIEWPORT_EXTENT = 10000;
 */
    LIB_COMPONENT* component = NULL;
    CMP_LIBRARY* lib;
    double bestzoom = 16.0;      // default value for bestzoom

    lib = CMP_LIBRARY::FindLibrary( m_libraryName );

    if( lib  )
        component = lib->FindComponent( m_entryName );

    if( component == NULL )
    {
        GetScreen()->SetScrollCenterPosition( wxPoint( 0, 0 ) );
        return bestzoom;
    }

    wxSize size = m_canvas->GetClientSize();

    EDA_RECT BoundaryBox = component->GetBoundingBox( m_unit, m_convert );

    // Reserve a 10% margin around component bounding box.
    double margin_scale_factor = 0.8;
    double zx =(double) BoundaryBox.GetWidth() /
               ( margin_scale_factor * (double)size.x );
    double zy = (double) BoundaryBox.GetHeight() /
                ( margin_scale_factor * (double)size.y);

    // Calculates the best zoom
    bestzoom = MAX( zx, zy );

    // keep it >= minimal existing zoom (can happen for very small components
    // like small power symbols
    if( bestzoom  < GetScreen()->m_ZoomList[0] )
        bestzoom  = GetScreen()->m_ZoomList[0];

    GetScreen()->SetScrollCenterPosition( BoundaryBox.Centre() );

    return bestzoom;
}


void LIB_VIEW_FRAME::ReCreateListLib()
{
    if( m_LibList == NULL )
        return;

    m_LibList->Clear();
    m_LibList->Append( CMP_LIBRARY::GetLibraryNames() );

    // Search for a previous selection:
    int index =  m_LibList->FindString( m_libraryName );

    if( index != wxNOT_FOUND )
    {
        m_LibList->SetSelection( index, true );
    }
    else
    {
        /* If not found, clear current library selection because it can be
         * deleted after a config change. */
        m_libraryName = wxEmptyString;
        m_entryName = wxEmptyString;
        m_unit = 1;
        m_convert = 1;
    }

    ReCreateListCmp();
    ReCreateHToolbar();
    DisplayLibInfos();
    m_canvas->Refresh();
}


void LIB_VIEW_FRAME::ReCreateListCmp()
{
    if( m_CmpList == NULL )
        return;

    m_CmpList->Clear();

    CMP_LIBRARY* Library = CMP_LIBRARY::FindLibrary( m_libraryName );

    if( Library == NULL )
    {
        m_libraryName = wxEmptyString;
        m_entryName = wxEmptyString;
        m_convert = 1;
        m_unit    = 1;
        return;
    }

    wxArrayString  nameList;
    Library->GetEntryNames( nameList );
    m_CmpList->Append( nameList );

    int index = m_CmpList->FindString( m_entryName );

    if( index == wxNOT_FOUND )
    {
        m_entryName = wxEmptyString;
        m_convert = 1;
        m_unit    = 1;
    }
    else
    {
        m_CmpList->SetSelection( index, true );
    }
}


void LIB_VIEW_FRAME::ClickOnLibList( wxCommandEvent& event )
{
    int ii = m_LibList->GetSelection();

    if( ii < 0 )
        return;

    wxString name = m_LibList->GetString( ii );

    if( m_libraryName == name )
        return;

    m_libraryName = name;
    ReCreateListCmp();
    m_canvas->Refresh();
    DisplayLibInfos();
    ReCreateHToolbar();
}


void LIB_VIEW_FRAME::ClickOnCmpList( wxCommandEvent& event )
{
    int ii = m_CmpList->GetSelection();

    if( ii < 0 )
        return;

    wxString name = m_CmpList->GetString( ii );

    if( m_entryName.CmpNoCase( name ) != 0 )
    {
        m_entryName = name;
        DisplayLibInfos();
        m_unit    = 1;
        m_convert = 1;
        Zoom_Automatique( false );
        ReCreateHToolbar();
        m_canvas->Refresh();
    }
}


void LIB_VIEW_FRAME::ExportToSchematicLibraryPart( wxCommandEvent& event )
{
    int ii = m_CmpList->GetSelection();

    if( ii >= 0 )
        m_exportToEeschemaCmpName = m_CmpList->GetString( ii );
    else
        m_exportToEeschemaCmpName.Empty();

    Close( true );
}


#define LIBLIST_WIDTH_KEY wxT( "Liblist_width" )
#define CMPLIST_WIDTH_KEY wxT( "Cmplist_width" )


void LIB_VIEW_FRAME::LoadSettings( )
{
    wxConfig* cfg ;

    EDA_DRAW_FRAME::LoadSettings();

    wxConfigPathChanger cpc( wxGetApp().GetSettings(), m_configPath );
    cfg = wxGetApp().GetSettings();

    m_LibListSize.x = 150; // default width of libs list
    m_CmpListSize.x = 150; // default width of component list

    cfg->Read( LIBLIST_WIDTH_KEY, &m_LibListSize.x );
    cfg->Read( CMPLIST_WIDTH_KEY, &m_CmpListSize.x );

    // Set parameters to a reasonable value.
    if ( m_LibListSize.x > m_FrameSize.x/2 )
        m_LibListSize.x = m_FrameSize.x/2;

    if ( m_CmpListSize.x > m_FrameSize.x/2 )
        m_CmpListSize.x = m_FrameSize.x/2;
}


void LIB_VIEW_FRAME::SaveSettings()
{
    wxConfig* cfg;

    EDA_DRAW_FRAME::SaveSettings();

    wxConfigPathChanger cpc( wxGetApp().GetSettings(), m_configPath );
    cfg = wxGetApp().GetSettings();

    if ( m_LibListSize.x )
        cfg->Write( LIBLIST_WIDTH_KEY, m_LibListSize.x );

    cfg->Write( CMPLIST_WIDTH_KEY, m_CmpListSize.x );
}


void LIB_VIEW_FRAME::OnActivate( wxActivateEvent& event )
{
    EDA_DRAW_FRAME::OnActivate( event );

    // Ensure we do not have old selection:
    if( m_FrameIsActive )
        m_exportToEeschemaCmpName.Empty();

    if( m_LibList )
        ReCreateListLib();

    DisplayLibInfos();
}
