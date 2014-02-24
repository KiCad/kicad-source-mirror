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

#include <fctsys.h>
#include <appl_wxstruct.h>
#include <eeschema_id.h>
#include <class_drawpanel.h>
#include <wxEeschemaStruct.h>
#include <msgpanel.h>

#include <general.h>
#include <viewlib_frame.h>
#include <class_library.h>
#include <hotkeys.h>
#include <dialog_helpers.h>


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

    /* Toolbar events */
    EVT_TOOL_RANGE( ID_LIBVIEW_NEXT, ID_LIBVIEW_DE_MORGAN_CONVERT_BUTT,
                    LIB_VIEW_FRAME::Process_Special_Functions )

    EVT_TOOL( ID_LIBVIEW_CMP_EXPORT_TO_SCHEMATIC, LIB_VIEW_FRAME::ExportToSchematicLibraryPart )
    EVT_COMBOBOX( ID_LIBVIEW_SELECT_PART_NUMBER, LIB_VIEW_FRAME::Process_Special_Functions )

    /* listbox events */
    EVT_LISTBOX( ID_LIBVIEW_LIB_LIST, LIB_VIEW_FRAME::ClickOnLibList )
    EVT_LISTBOX( ID_LIBVIEW_CMP_LIST, LIB_VIEW_FRAME::ClickOnCmpList )
    EVT_LISTBOX_DCLICK( ID_LIBVIEW_CMP_LIST, LIB_VIEW_FRAME::DClickOnCmpList )

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
#define LIB_VIEW_FRAME_NAME wxT( "ViewlibFrame" )

LIB_VIEW_FRAME::LIB_VIEW_FRAME( SCH_BASE_FRAME* aParent, CMP_LIBRARY* aLibrary,
                                wxSemaphore* aSemaphore, long aStyle ) :
    SCH_BASE_FRAME( aParent, VIEWER_FRAME_TYPE, _( "Library Browser" ),
                    wxDefaultPosition, wxDefaultSize, aStyle, GetLibViewerFrameName() )
{
    wxAcceleratorTable table( ACCEL_TABLE_CNT, accels );

    m_FrameName = GetLibViewerFrameName();
    m_configPath = wxT( "LibraryViewer" );

    // Give an icon
    wxIcon  icon;
    icon.CopyFromBitmap( KiBitmap( library_browse_xpm ) );

    SetIcon( icon );

    m_HotkeysZoomAndGridList = s_Viewlib_Hokeys_Descr;
    m_cmpList   = NULL;
    m_libList   = NULL;
    m_semaphore = aSemaphore;

    if( m_semaphore )
        SetModalMode( true );

    m_exportToEeschemaCmpName.Empty();

    SetScreen( new SCH_SCREEN() );
    GetScreen()->m_Center = true;      // Axis origin centered on screen.
    LoadSettings();

    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    // Initialize grid id to the default value (50 mils):
    m_LastGridSizeId = ID_POPUP_GRID_LEVEL_50 - ID_POPUP_GRID_LEVEL_1000;
    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId  );

    ReCreateHToolbar();
    ReCreateVToolbar();

    wxSize  size = GetClientSize();
    size.y -= m_MsgFrameHeight + 2;

    wxPoint win_pos( 0, 0 );

    if( aLibrary == NULL )
    {
        // Creates the libraries window display
         m_libList = new wxListBox( this, ID_LIBVIEW_LIB_LIST,
                                   wxPoint( 0, 0 ), wxSize(m_libListWidth, -1),
                                   0, NULL, wxLB_HSCROLL );
    }
    else
    {
        m_libraryName = aLibrary->GetName();
        m_entryName.Clear();
        m_unit = 1;
        m_convert = 1;
        m_libListWidth = 0;
    }

    // Creates the component window display
    win_pos.x = m_libListWidth;
    m_cmpList = new wxListBox( this, ID_LIBVIEW_CMP_LIST,
                               wxPoint( 0, 0 ), wxSize(m_cmpListWidth, -1),
                               0, NULL, wxLB_HSCROLL );

    if( m_libList )
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

    // Manage the left window (list of libraries)
    if( m_libList )
        m_auimgr.AddPane( m_libList, wxAuiPaneInfo( info ).Name( wxT( "m_libList" ) ).
                          Left().Row( 0 ) );

    // Manage the list of components)
    m_auimgr.AddPane( m_cmpList,
                      wxAuiPaneInfo( info ).Name( wxT( "m_cmpList" ) ).
                      Left().Row( 1 ) );

    // Manage the draw panel
    m_auimgr.AddPane( m_canvas,
                      wxAuiPaneInfo().Name( wxT( "DrawFrame" ) ).CentrePane() );

    // Manage the message panel
    m_auimgr.AddPane( m_messagePanel,
                      wxAuiPaneInfo( mesg ).Name( wxT( "MsgPanel" ) ).Bottom().Layer(10) );

    /* Now the minimum windows are fixed, set library list
     * and component list of the previous values from last viewlib use
     */
    if( m_libList )
    {
        m_auimgr.GetPane( m_libList ).MinSize( wxSize( 80, -1) );
        m_auimgr.GetPane( m_libList ).BestSize( wxSize(m_libListWidth, -1) );
    }

    m_auimgr.GetPane( m_cmpList ).MinSize( wxSize( 80, -1) );
    m_auimgr.GetPane( m_cmpList ).BestSize(wxSize(m_cmpListWidth, -1) );

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
}


const wxChar* LIB_VIEW_FRAME::GetLibViewerFrameName()
{
    return LIB_VIEW_FRAME_NAME;
}


LIB_VIEW_FRAME* LIB_VIEW_FRAME::GetActiveLibraryViewer()
{
    return (LIB_VIEW_FRAME*) wxWindow::FindWindowByName(GetLibViewerFrameName());
}


void LIB_VIEW_FRAME::OnCloseWindow( wxCloseEvent& Event )
{
    if( m_semaphore )
    {
        m_semaphore->Post();
        // This window will be destroyed by the calling function,
        // if needed
        SetModalMode( false );
    }
    else
    {
        Destroy();
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
    GetScreen()->m_O_Curseur = GetCrossHairPosition();
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
    LIB_COMPONENT*  component = NULL;
    double          bestzoom = 16.0;      // default value for bestzoom
    CMP_LIBRARY*    lib = CMP_LIBRARY::FindLibrary( m_libraryName );

    if( lib  )
        component = lib->FindComponent( m_entryName );

    if( component == NULL )
    {
        SetScrollCenterPosition( wxPoint( 0, 0 ) );
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
    bestzoom = std::max( zx, zy );

    // keep it >= minimal existing zoom (can happen for very small components
    // like small power symbols
    if( bestzoom  < GetScreen()->m_ZoomList[0] )
        bestzoom  = GetScreen()->m_ZoomList[0];

    SetScrollCenterPosition( BoundaryBox.Centre() );

    return bestzoom;
}


void LIB_VIEW_FRAME::ReCreateListLib()
{
    if( m_libList == NULL )
        return;

    m_libList->Clear();
    m_libList->Append( CMP_LIBRARY::GetLibraryNames() );

    // Search for a previous selection:
    int index = m_libList->FindString( m_libraryName );

    if( index != wxNOT_FOUND )
    {
        m_libList->SetSelection( index, true );
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
    if( m_cmpList == NULL )
        return;

    m_cmpList->Clear();

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
    m_cmpList->Append( nameList );

    int index = m_cmpList->FindString( m_entryName );

    if( index == wxNOT_FOUND )
    {
        m_entryName = wxEmptyString;
        m_convert = 1;
        m_unit    = 1;
    }
    else
    {
        m_cmpList->SetSelection( index, true );
    }
}


void LIB_VIEW_FRAME::ClickOnLibList( wxCommandEvent& event )
{
    int ii = m_libList->GetSelection();

    if( ii < 0 )
        return;

    SetSelectedLibrary( m_libList->GetString( ii ) );
}


void LIB_VIEW_FRAME::SetSelectedLibrary( const wxString& aLibraryName )
{
    if( m_libraryName == aLibraryName )
        return;

    m_libraryName = aLibraryName;
    ReCreateListCmp();
    m_canvas->Refresh();
    DisplayLibInfos();
    ReCreateHToolbar();
    // Ensure the corresponding line in m_libList is selected
    // (which is not necessary the case if SetSelectedLibrary is called
    // by an other caller than ClickOnLibList.
    m_libList->SetStringSelection( m_libraryName, true );
}


void LIB_VIEW_FRAME::ClickOnCmpList( wxCommandEvent& event )
{
    int ii = m_cmpList->GetSelection();

    if( ii < 0 )
        return;

    SetSelectedComponent( m_cmpList->GetString( ii ) );
}


void LIB_VIEW_FRAME::SetSelectedComponent( const wxString& aComponentName )
{
    if( m_entryName.CmpNoCase( aComponentName ) != 0 )
    {
        m_entryName = aComponentName;
        // Ensure the corresponding line in m_cmpList is selected
        // (which is not necessary the case if SetSelectedComponent is called
        // by an other caller than ClickOnCmpList.
        m_cmpList->SetStringSelection( aComponentName, true );
        DisplayLibInfos();
        m_unit    = 1;
        m_convert = 1;
        Zoom_Automatique( false );
        ReCreateHToolbar();
        m_canvas->Refresh();
    }
}


void LIB_VIEW_FRAME::DClickOnCmpList( wxCommandEvent& event )
{
    if( m_semaphore )
    {
        ExportToSchematicLibraryPart( event );

        // Prevent the double click from being as a single click in the parent
        // window which would cause the part to be parked rather than staying
        // in drag mode.
        ((SCH_BASE_FRAME*) GetParent())->SkipNextLeftButtonReleaseEvent();
    }
}


void LIB_VIEW_FRAME::ExportToSchematicLibraryPart( wxCommandEvent& event )
{
    int ii = m_cmpList->GetSelection();

    if( ii >= 0 )
        m_exportToEeschemaCmpName = m_cmpList->GetString( ii );
    else
        m_exportToEeschemaCmpName.Empty();

    Close( true );
}


#define LIBLIST_WIDTH_KEY wxT( "ViewLiblistWidth" )
#define CMPLIST_WIDTH_KEY wxT( "ViewCmplistWidth" )


void LIB_VIEW_FRAME::LoadSettings( )
{
    wxConfig* cfg ;

    EDA_DRAW_FRAME::LoadSettings();

    wxConfigPathChanger cpc( wxGetApp().GetSettings(), m_configPath );
    cfg = wxGetApp().GetSettings();

    cfg->Read( LIBLIST_WIDTH_KEY, &m_libListWidth, 100 );
    cfg->Read( CMPLIST_WIDTH_KEY, &m_cmpListWidth, 100 );

    // Set parameters to a reasonable value.
    if( m_libListWidth > m_FrameSize.x/2 )
        m_libListWidth = m_FrameSize.x/2;

    if( m_cmpListWidth > m_FrameSize.x/2 )
        m_cmpListWidth = m_FrameSize.x/2;
}


void LIB_VIEW_FRAME::SaveSettings()
{
    wxConfig* cfg;

    EDA_DRAW_FRAME::SaveSettings();

    wxConfigPathChanger cpc( wxGetApp().GetSettings(), m_configPath );
    cfg = wxGetApp().GetSettings();

    if( m_libListWidth && m_libList)
    {
        m_libListWidth = m_libList->GetSize().x;
        cfg->Write( LIBLIST_WIDTH_KEY, m_libListWidth );
    }

    m_cmpListWidth = m_cmpList->GetSize().x;
    cfg->Write( CMPLIST_WIDTH_KEY, m_cmpListWidth );
}


void LIB_VIEW_FRAME::OnActivate( wxActivateEvent& event )
{
    EDA_DRAW_FRAME::OnActivate( event );

    // Ensure we do not have old selection:
    if( m_FrameIsActive )
        m_exportToEeschemaCmpName.Empty();

    if( m_libList )
        ReCreateListLib();

    DisplayLibInfos();
}
