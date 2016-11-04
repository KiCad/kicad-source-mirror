/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2016 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <kiface_i.h>
#include <pgm_base.h>
#include <eeschema_id.h>
#include <class_drawpanel.h>
#include <schframe.h>
#include <msgpanel.h>

#include <general.h>
#include <viewlib_frame.h>
#include <class_library.h>
#include <hotkeys.h>
#include <dialog_helpers.h>


// Save previous component library viewer state.
wxString LIB_VIEW_FRAME::m_libraryName;
wxString LIB_VIEW_FRAME::m_entryName;

int LIB_VIEW_FRAME::m_unit = 1;
int LIB_VIEW_FRAME::m_convert = 1;


BEGIN_EVENT_TABLE( LIB_VIEW_FRAME, EDA_DRAW_FRAME )
    // Window events
    EVT_CLOSE( LIB_VIEW_FRAME::OnCloseWindow )
    EVT_SIZE( LIB_VIEW_FRAME::OnSize )
    EVT_ACTIVATE( LIB_VIEW_FRAME::OnActivate )

    // Toolbar events
    EVT_TOOL( ID_LIBVIEW_SELECT_PART, LIB_VIEW_FRAME::OnSelectSymbol )
    EVT_TOOL( ID_LIBVIEW_NEXT, LIB_VIEW_FRAME::onSelectNextSymbol )
    EVT_TOOL( ID_LIBVIEW_PREVIOUS, LIB_VIEW_FRAME::onSelectPreviousSymbol )
    EVT_TOOL( ID_LIBVIEW_VIEWDOC, LIB_VIEW_FRAME::onViewSymbolDocument )
    EVT_TOOL_RANGE( ID_LIBVIEW_DE_MORGAN_NORMAL_BUTT, ID_LIBVIEW_DE_MORGAN_CONVERT_BUTT,
                    LIB_VIEW_FRAME::onSelectSymbolBodyStyle )
    EVT_TOOL( ID_LIBVIEW_CMP_EXPORT_TO_SCHEMATIC, LIB_VIEW_FRAME::ExportToSchematicLibraryPart )
    EVT_COMBOBOX( ID_LIBVIEW_SELECT_PART_NUMBER, LIB_VIEW_FRAME::onSelectSymbolUnit )

    // listbox events
    EVT_LISTBOX( ID_LIBVIEW_LIB_LIST, LIB_VIEW_FRAME::ClickOnLibList )
    EVT_LISTBOX( ID_LIBVIEW_CMP_LIST, LIB_VIEW_FRAME::ClickOnCmpList )
    EVT_LISTBOX_DCLICK( ID_LIBVIEW_CMP_LIST, LIB_VIEW_FRAME::DClickOnCmpList )

    // Menu (and/or hotkey) events
    EVT_MENU( wxID_HELP, EDA_DRAW_FRAME::GetKicadHelp )
    EVT_MENU( wxID_EXIT, LIB_VIEW_FRAME::CloseLibraryViewer )
    EVT_MENU( ID_HELP_GET_INVOLVED, EDA_DRAW_FRAME::GetKicadContribute )
    EVT_MENU( ID_SET_RELATIVE_OFFSET, LIB_VIEW_FRAME::OnSetRelativeOffset )
    EVT_MENU( ID_LIBVIEW_SHOW_ELECTRICAL_TYPE, LIB_VIEW_FRAME::OnShowElectricalType )

    EVT_UPDATE_UI( ID_LIBVIEW_VIEWDOC, LIB_VIEW_FRAME::onUpdateViewDoc )
    EVT_UPDATE_UI( ID_LIBVIEW_DE_MORGAN_NORMAL_BUTT, LIB_VIEW_FRAME::onUpdateNormalBodyStyleButton )
    EVT_UPDATE_UI( ID_LIBVIEW_DE_MORGAN_CONVERT_BUTT, LIB_VIEW_FRAME::onUpdateAlternateBodyStyleButton )
    EVT_UPDATE_UI( ID_LIBVIEW_SHOW_ELECTRICAL_TYPE, LIB_VIEW_FRAME::OnUpdateElectricalType )

END_EVENT_TABLE()


/* Note:
 * LIB_VIEW_FRAME can be created in "modal mode", or as a usual frame.
 * In modal mode:
 *  a tool to export the selected symbol is shown in the toolbar
 *  the style is wxFRAME_FLOAT_ON_PARENT
 * Note:
 * On windows, when the frame with type wxFRAME_FLOAT_ON_PARENT is displayed
 * its parent frame is sometimes brought to the foreground when closing the
 * LIB_VIEW_FRAME frame.
 * If it still happens, it could be better to use wxSTAY_ON_TOP
 * instead of wxFRAME_FLOAT_ON_PARENT
 */
#ifdef __WINDOWS__
#define MODAL_MODE_EXTRASTYLE wxFRAME_FLOAT_ON_PARENT   // could be wxSTAY_ON_TOP if issues
#else
#define MODAL_MODE_EXTRASTYLE wxFRAME_FLOAT_ON_PARENT
#endif

#define LIB_VIEW_FRAME_NAME "ViewlibFrame"
#define LIB_VIEW_FRAME_NAME_MODAL "ViewlibFrameModal"

LIB_VIEW_FRAME::LIB_VIEW_FRAME( KIWAY* aKiway, wxWindow* aParent, FRAME_T aFrameType,
        PART_LIB* aLibrary ) :
    SCH_BASE_FRAME( aKiway, aParent, aFrameType, _( "Library Browser" ),
            wxDefaultPosition, wxDefaultSize,
            aFrameType == FRAME_SCH_VIEWER_MODAL ?
                aParent ? KICAD_DEFAULT_DRAWFRAME_STYLE | MODAL_MODE_EXTRASTYLE
                          : KICAD_DEFAULT_DRAWFRAME_STYLE | wxSTAY_ON_TOP
                : KICAD_DEFAULT_DRAWFRAME_STYLE,
            aFrameType == FRAME_SCH_VIEWER_MODAL ?
                          LIB_VIEW_FRAME_NAME_MODAL : LIB_VIEW_FRAME_NAME )
{
    wxASSERT( aFrameType == FRAME_SCH_VIEWER ||
              aFrameType == FRAME_SCH_VIEWER_MODAL );

    if( aFrameType == FRAME_SCH_VIEWER_MODAL )
        SetModal( true );

    // Force the frame name used in config. the lib viewer frame has a name
    // depending on aFrameType (needed to identify the frame by wxWidgets),
    // but only one configuration is preferable.
    m_configFrameName = LIB_VIEW_FRAME_NAME;

    // Give an icon
    wxIcon  icon;
    icon.CopyFromBitmap( KiBitmap( library_browse_xpm ) );
    SetIcon( icon );

    m_hotkeysDescrList = g_Viewlib_Hokeys_Descr;
    m_cmpList   = NULL;
    m_libList   = NULL;
    m_listPowerCmpOnly = false;
    SetShowElectricalType( true );

    SetScreen( new SCH_SCREEN( aKiway ) );
    GetScreen()->m_Center = true;      // Axis origin centered on screen.
    LoadSettings( config() );

    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    // Initialize grid id to the default value (50 mils):
    m_LastGridSizeId = ID_POPUP_GRID_LEVEL_50 - ID_POPUP_GRID_LEVEL_1000;
    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId  );

    // Menu bar is not mandatory: uncomment/comment the next line
    // to add/remove the menubar
    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateVToolbar();

    if( !aLibrary )
    {
        // Creates the libraries window display
        m_libList = new wxListBox( this, ID_LIBVIEW_LIB_LIST,
                                   wxPoint( 0, 0 ), wxSize( m_libListWidth, -1 ),
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
    m_cmpList = new wxListBox( this, ID_LIBVIEW_CMP_LIST, wxPoint( 0, 0 ),
                               wxSize( m_cmpListWidth, -1 ), 0, NULL, wxLB_HSCROLL );

    if( m_libList )
        ReCreateListLib();

    DisplayLibInfos();

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
                      wxAuiPaneInfo( horiz ).Name( "m_mainToolBar" ).Top().Row( 0 ) );

    // Manage the left window (list of libraries)
    if( m_libList )
        m_auimgr.AddPane( m_libList, wxAuiPaneInfo( info ).Name( "m_libList" ).Left().Row( 0 ) );

    // Manage the list of components)
    m_auimgr.AddPane( m_cmpList, wxAuiPaneInfo( info ).Name( "m_cmpList" ).Left().Row( 1 ) );

    // Manage the draw panel
    m_auimgr.AddPane( m_canvas, wxAuiPaneInfo().Name( "DrawFrame" ).CentrePane() );

    // Manage the message panel
    m_auimgr.AddPane( m_messagePanel,
                      wxAuiPaneInfo( mesg ).Name( "MsgPanel" ).Bottom().Layer( 10 ) );

    /* Now the minimum windows are fixed, set library list
     * and component list of the previous values from last viewlib use
     */
    if( m_libList )
    {
        m_auimgr.GetPane( m_libList ).MinSize( wxSize( 80, -1 ) );
        m_auimgr.GetPane( m_libList ).BestSize( wxSize( m_libListWidth, -1 ) );
    }

    m_auimgr.GetPane( m_cmpList ).MinSize( wxSize( 80, -1) );
    m_auimgr.GetPane( m_cmpList ).BestSize( wxSize( m_cmpListWidth, -1 ) );

    m_auimgr.Update();

    // Now Drawpanel is sized, we can use BestZoom to show the component (if any)
#ifdef USE_WX_GRAPHICS_CONTEXT
    GetScreen()->SetZoom( BestZoom() );
#else
    Zoom_Automatique( false );
#endif

    if( !IsModal() )        // For modal mode, calling ShowModal() will show this frame
    {
        Raise();
        Show( true );
    }
}


LIB_VIEW_FRAME::~LIB_VIEW_FRAME()
{
}


LIB_ALIAS* LIB_VIEW_FRAME::getSelectedAlias()
{
    LIB_ALIAS* alias = NULL;

    if( !m_libraryName.IsEmpty() && !m_entryName.IsEmpty() )
    {
        PART_LIB* lib = Prj().SchLibs()->FindLibrary( m_libraryName );

        if( lib )
            alias = lib->FindAlias( m_entryName );
    }

    return alias;
}


LIB_PART* LIB_VIEW_FRAME::getSelectedSymbol()
{
    LIB_PART* symbol = NULL;
    LIB_ALIAS* alias = getSelectedAlias();

    if( alias )
        symbol = alias->GetPart();

    return symbol;
}


void LIB_VIEW_FRAME::onUpdateAlternateBodyStyleButton( wxUpdateUIEvent& aEvent )
{
    LIB_PART* symbol = getSelectedSymbol();

    aEvent.Enable( symbol && symbol->HasConversion() );

    if( symbol )
        aEvent.Check( m_convert > 1 );
    else
        aEvent.Check( false );
}


void LIB_VIEW_FRAME::onUpdateNormalBodyStyleButton( wxUpdateUIEvent& aEvent )
{
    LIB_PART* symbol = getSelectedSymbol();

    aEvent.Enable( symbol && symbol->HasConversion() );

    if( symbol )
        aEvent.Check( m_convert <= 1 );
    else
        aEvent.Check( true );
}


void LIB_VIEW_FRAME::onUpdateViewDoc( wxUpdateUIEvent& aEvent )
{
    LIB_ALIAS* alias = getSelectedAlias();

    aEvent.Enable( alias && !alias->GetDocFileName().IsEmpty() );
}


void LIB_VIEW_FRAME::OnCloseWindow( wxCloseEvent& Event )
{
    if( !IsModal() )
    {
        Destroy();
    }
    else if( !IsDismissed() )
    {
        // only dismiss modal frame if not already dismissed.
        DismissModal( false );

        // Modal frame will be destroyed by the calling function.
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


void LIB_VIEW_FRAME::OnShowElectricalType( wxCommandEvent& event )
{
    SetShowElectricalType( not GetShowElectricalType() );
    m_canvas->Refresh();
}


void LIB_VIEW_FRAME::OnUpdateElectricalType( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( GetShowElectricalType() );
}

double LIB_VIEW_FRAME::BestZoom()
{
    /* Please, note: wxMSW before version 2.9 seems have
     * problems with zoom values < 1 ( i.e. userscale > 1) and needs to be patched:
     * edit file <wxWidgets>/src/msw/dc.cpp
     * search for line static const int VIEWPORT_EXTENT = 1000;
     * and replace by static const int VIEWPORT_EXTENT = 10000;
     */

    LIB_PART*   part = NULL;
    double      bestzoom = 16.0;      // default value for bestzoom
    PART_LIB*   lib = Prj().SchLibs()->FindLibrary( m_libraryName );

    if( lib  )
        part = lib->FindPart( m_entryName );

    if( !part )
    {
        SetScrollCenterPosition( wxPoint( 0, 0 ) );
        return bestzoom;
    }

    wxSize size = m_canvas->GetClientSize();

    EDA_RECT boundingBox = part->GetUnitBoundingBox( m_unit, m_convert );

    // Reserve a 10% margin around component bounding box.
    double margin_scale_factor = 0.8;
    double zx =(double) boundingBox.GetWidth() /
               ( margin_scale_factor * (double)size.x );
    double zy = (double) boundingBox.GetHeight() /
                ( margin_scale_factor * (double)size.y);

    // Calculates the best zoom
    bestzoom = std::max( zx, zy );

    // keep it >= minimal existing zoom (can happen for very small components
    // like small power symbols
    if( bestzoom  < GetScreen()->m_ZoomList[0] )
        bestzoom  = GetScreen()->m_ZoomList[0];

    SetScrollCenterPosition( boundingBox.Centre() );

    return bestzoom;
}


void LIB_VIEW_FRAME::ReCreateListLib()
{
    if( !m_libList )
        return;

    m_libList->Clear();

    wxArrayString libs = Prj().SchLibs()->GetLibraryNames();

    // Remove not allowed libs from main list, if the allowed lib list is not empty
    if( m_allowedLibs.GetCount() )
    {
        for( unsigned ii = 0; ii < libs.GetCount(); )
        {
            if( m_allowedLibs.Index( libs[ii] ) == wxNOT_FOUND )
                libs.RemoveAt( ii );
            else
                ii++;
        }
    }

    // Remove libs which have no power components, if this filter is activated
    if( m_listPowerCmpOnly )
    {
        for( unsigned ii = 0; ii < libs.GetCount(); )
        {
            PART_LIB* lib = Prj().SchLibs()->FindLibrary( libs[ii] );

            if( lib && !lib->HasPowerParts() )
                libs.RemoveAt( ii );
            else
                ii++;
        }
    }

    if( libs.IsEmpty() )
        return;

    m_libList->Append( libs );

    // Search for a previous selection:
    int index = m_libList->FindString( m_libraryName );

    if( index != wxNOT_FOUND )
    {
        m_libList->SetSelection( index, true );
    }
    else
    {
        // If not found, clear current library selection because it can be
        // deleted after a config change.
        m_libraryName = libs[0];
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

    PART_LIB* lib = Prj().SchLibs()->FindLibrary( m_libraryName );

    if( !lib || lib->IsEmpty() )
    {
        m_libraryName = wxEmptyString;
        m_entryName = wxEmptyString;
        m_convert = 1;
        m_unit    = 1;
        return;
    }

    wxArrayString  nameList;

    if( m_listPowerCmpOnly )
        lib->GetEntryTypePowerNames( nameList );
    else
        lib->GetAliasNames( nameList );

    m_cmpList->Append( nameList );

    int index = m_cmpList->FindString( m_entryName );

    if( index == wxNOT_FOUND )
    {
        // Select the first library entry when the previous entry name does not exist in
        // the current library.
        m_convert   = 1;
        m_unit      = 1;
        index       = 0;
    }

    m_entryName = wxEmptyString;
    m_cmpList->SetSelection( index, true );

    wxCommandEvent evt( wxEVT_COMMAND_LISTBOX_SELECTED, ID_LIBVIEW_CMP_LIST );
    ProcessEvent( evt );
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
    // Aren't component names case sensitive now?
    if( m_entryName.CmpNoCase( aComponentName ) != 0 )
    {
        m_entryName = aComponentName;

        // Ensure the corresponding line in m_cmpList is selected
        // (which is not necessarily the case if SetSelectedComponent is called
        // by another caller than ClickOnCmpList.
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
    if( IsModal() )
    {
        ExportToSchematicLibraryPart( event );

        // The schematic editor might not be the parent of the library viewer.
        // It could be a python window.
        SCH_EDIT_FRAME* schframe = dynamic_cast<SCH_EDIT_FRAME*>( GetParent() );

        if( schframe )
        {
            // Prevent the double click from being as a single click in the parent
            // window which would cause the part to be parked rather than staying
            // in drag mode.
            schframe->SkipNextLeftButtonReleaseEvent();
        }
    }
}


void LIB_VIEW_FRAME::ExportToSchematicLibraryPart( wxCommandEvent& event )
{
    int ii = m_cmpList->GetSelection();

    if( ii >= 0 )
    {
        wxString part_name = m_cmpList->GetString( ii );

        // a selection was made, pass true
        DismissModal( true, part_name );
    }
    else
    {
        // no selection was made, pass false
        DismissModal( false );
    }

    Close( true );
}


#define LIBLIST_WIDTH_KEY "ViewLiblistWidth"
#define CMPLIST_WIDTH_KEY "ViewCmplistWidth"
#define CMPVIEW_SHOW_PINELECTRICALTYPE_KEY "ViewCmpShowPinElectricalType"

// Currently, the library viewer has no dialog to change the background color
// of the draw canvas. Therefore the background color is here just
// in case of this option is added to some library viewer config dialog
#define LIBVIEW_BGCOLOR   "LibviewBgColor"


void LIB_VIEW_FRAME::LoadSettings( wxConfigBase* aCfg )
{
    EDA_DRAW_FRAME::LoadSettings( aCfg );

    SetGridColor( GetLayerColor( LAYER_GRID ) );
    SetDrawBgColor( GetLayerColor( LAYER_BACKGROUND ) );

    aCfg->Read( LIBLIST_WIDTH_KEY, &m_libListWidth, 150 );
    aCfg->Read( CMPLIST_WIDTH_KEY, &m_cmpListWidth, 150 );
    aCfg->Read( CMPLIST_WIDTH_KEY, &m_cmpListWidth, 150 );
    m_showPinElectricalTypeName = aCfg->Read( CMPVIEW_SHOW_PINELECTRICALTYPE_KEY, true );

    // Set parameters to a reasonable value.
    if( m_libListWidth > m_FrameSize.x/2 )
        m_libListWidth = m_FrameSize.x/2;

    if( m_cmpListWidth > m_FrameSize.x/2 )
        m_cmpListWidth = m_FrameSize.x/2;
}


void LIB_VIEW_FRAME::SaveSettings( wxConfigBase* aCfg )
{
    EDA_DRAW_FRAME::SaveSettings( aCfg );

    if( m_libListWidth && m_libList )
    {
        m_libListWidth = m_libList->GetSize().x;
        aCfg->Write( LIBLIST_WIDTH_KEY, m_libListWidth );
    }

    m_cmpListWidth = m_cmpList->GetSize().x;
    aCfg->Write( CMPLIST_WIDTH_KEY, m_cmpListWidth );

    aCfg->Write( CMPVIEW_SHOW_PINELECTRICALTYPE_KEY, m_showPinElectricalTypeName );
}


void LIB_VIEW_FRAME::OnActivate( wxActivateEvent& event )
{
    EDA_DRAW_FRAME::OnActivate( event );

    if( m_libList )
        ReCreateListLib();

    DisplayLibInfos();
}


void LIB_VIEW_FRAME::CloseLibraryViewer( wxCommandEvent& event )
{
    Close();
}


void LIB_VIEW_FRAME::SetFilter( const SCHLIB_FILTER* aFilter )
{
    m_listPowerCmpOnly = false;
    m_allowedLibs.Clear();

    if( aFilter )
    {
        m_allowedLibs = aFilter->GetAllowedLibList();
        m_listPowerCmpOnly = aFilter->GetFilterPowerParts();
    }

    ReCreateListLib();
}
