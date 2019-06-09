/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2016 Wayne Stambaugh <stambaughw@verizon.net>
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
#include <kiway.h>
#include <pcb_draw_panel_gal.h>
#include <3d_viewer/eda_3d_viewer.h>
#include <dialog_helpers.h>
#include <msgpanel.h>
#include <fp_lib_table.h>
#include <lib_id.h>
#include <confirm.h>
#include <bitmaps.h>
#include <gal/graphics_abstraction_layer.h>
#include <eda_dockart.h>
#include <pcb_painter.h>
#include <class_board.h>
#include <class_module.h>
#include <pcbnew.h>
#include <pcbnew_id.h>
#include <footprint_viewer_frame.h>
#include <footprint_info.h>
#include <wildcards_and_files_ext.h>
#include <config_params.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tool/action_toolbar.h>
#include <tool/common_tools.h>
#include "tools/selection_tool.h"
#include "tools/pcbnew_control.h"
#include "tools/pcb_actions.h"
#include "board_commit.h"

#include <functional>
#include <memory>
using namespace std::placeholders;


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
    EVT_TOOL( ID_MODVIEW_SELECT_PART, FOOTPRINT_VIEWER_FRAME::SelectCurrentFootprint )
    EVT_TOOL( ID_MODVIEW_OPTIONS, FOOTPRINT_VIEWER_FRAME::InstallDisplayOptions )
    EVT_TOOL( ID_MODVIEW_NEXT, FOOTPRINT_VIEWER_FRAME::OnIterateFootprintList )
    EVT_TOOL( ID_MODVIEW_PREVIOUS, FOOTPRINT_VIEWER_FRAME::OnIterateFootprintList )
    EVT_TOOL( ID_ADD_FOOTPRINT_TO_BOARD, FOOTPRINT_VIEWER_FRAME::AddFootprintToPCB )
    EVT_CHOICE( ID_ON_ZOOM_SELECT, FOOTPRINT_VIEWER_FRAME::OnSelectZoom )
    EVT_CHOICE( ID_ON_GRID_SELECT, FOOTPRINT_VIEWER_FRAME::OnSelectGrid )

    EVT_UPDATE_UI( ID_ON_GRID_SELECT, FOOTPRINT_VIEWER_FRAME::OnUpdateSelectGrid )
    EVT_UPDATE_UI( ID_ON_ZOOM_SELECT, FOOTPRINT_VIEWER_FRAME::OnUpdateSelectZoom )
    EVT_UPDATE_UI( ID_ADD_FOOTPRINT_TO_BOARD, FOOTPRINT_VIEWER_FRAME::OnUpdateFootprintButton )

    // listbox events
    EVT_LISTBOX( ID_MODVIEW_LIB_LIST, FOOTPRINT_VIEWER_FRAME::ClickOnLibList )
    EVT_LISTBOX( ID_MODVIEW_FOOTPRINT_LIST, FOOTPRINT_VIEWER_FRAME::ClickOnFootprintList )
    EVT_LISTBOX_DCLICK( ID_MODVIEW_FOOTPRINT_LIST, FOOTPRINT_VIEWER_FRAME::DClickOnFootprintList )

END_EVENT_TABLE()


/* Note:
 * FOOTPRINT_VIEWER_FRAME can be created in "modal mode", or as a usual frame.
 * In modal mode:
 *  a tool to export the selected footprint is shown in the toolbar
 *  the style is wxFRAME_FLOAT_ON_PARENT
 * Note:
 * On windows, when the frame with type wxFRAME_FLOAT_ON_PARENT is displayed
 * its parent frame is sometimes brought to the foreground when closing the
 * LIB_VIEW_FRAME frame.
 * If it still happens, it could be better to use wxSTAY_ON_TOP
 * instead of wxFRAME_FLOAT_ON_PARENT
 */

#define PARENT_STYLE   ( KICAD_DEFAULT_DRAWFRAME_STYLE | wxFRAME_FLOAT_ON_PARENT )
#define MODAL_STYLE    ( KICAD_DEFAULT_DRAWFRAME_STYLE | wxSTAY_ON_TOP )
#define NONMODAL_STYLE ( KICAD_DEFAULT_DRAWFRAME_STYLE )


FOOTPRINT_VIEWER_FRAME::FOOTPRINT_VIEWER_FRAME( KIWAY* aKiway, wxWindow* aParent,
                                                FRAME_T aFrameType ) :
    PCB_BASE_FRAME( aKiway, aParent, aFrameType, _( "Footprint Library Browser" ),
            wxDefaultPosition, wxDefaultSize,
            aFrameType == FRAME_PCB_MODULE_VIEWER_MODAL ? ( aParent ? PARENT_STYLE : MODAL_STYLE )
                                                        : NONMODAL_STYLE,
            aFrameType == FRAME_PCB_MODULE_VIEWER_MODAL ? FOOTPRINT_VIEWER_FRAME_NAME_MODAL
                                                        : FOOTPRINT_VIEWER_FRAME_NAME )
{
    wxASSERT( aFrameType == FRAME_PCB_MODULE_VIEWER_MODAL ||
              aFrameType == FRAME_PCB_MODULE_VIEWER );

    if( aFrameType == FRAME_PCB_MODULE_VIEWER_MODAL )
        SetModal( true );

    // Force the frame name used in config. the footprint viewer frame has a name
    // depending on aFrameType (needed to identify the frame by wxWidgets),
    // but only one configuration is preferable.
    m_configName = FOOTPRINT_VIEWER_FRAME_NAME;

    m_showAxis   = true;         // true to draw axis.

    // Give an icon
    wxIcon  icon;
    icon.CopyFromBitmap( KiBitmap( modview_icon_xpm ) );
    SetIcon( icon );

    m_libList = new wxListBox( this, ID_MODVIEW_LIB_LIST, wxDefaultPosition, wxDefaultSize,
                               0, NULL, wxLB_HSCROLL | wxNO_BORDER );

    m_footprintList = new wxListBox( this, ID_MODVIEW_FOOTPRINT_LIST, wxDefaultPosition, wxDefaultSize,
                                     0, NULL, wxLB_HSCROLL | wxNO_BORDER );

    SetBoard( new BOARD() );
    // In viewer, the default net clearance is not known (it depends on the actual board).
    // So we do not show the default clearance, by setting it to 0
    // The footprint or pad specific clearance will be shown
    GetBoard()->GetDesignSettings().GetDefault()->SetClearance(0);

    // Don't show the default board solder mask clearance in the footprint viewer.  Only the
    // footprint or pad clearance setting should be shown if it is not 0.
    GetBoard()->GetDesignSettings().m_SolderMaskMargin = 0;

    // Ensure all layers and items are visible:
    GetBoard()->SetVisibleAlls();
    SetScreen( new PCB_SCREEN( GetPageSizeIU() ) );

    GetScreen()->m_Center = true;      // Center coordinate origins on screen.
    LoadSettings( config() );
    GetGalDisplayOptions().m_axesEnabled = true;

    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId  );

    // Create GAL canvas
    auto drawPanel = new PCB_DRAW_PANEL_GAL( this, -1, wxPoint( 0, 0 ), m_FrameSize,
                                             GetGalDisplayOptions(), LoadCanvasTypeSetting() );
    SetGalCanvas( drawPanel );

    // Create the manager and dispatcher & route draw panel events to the dispatcher
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( GetBoard(), drawPanel->GetView(),
                                   drawPanel->GetViewControls(), this );
    m_actions = new PCB_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager, m_actions );
    drawPanel->SetEventDispatcher( m_toolDispatcher );

    m_toolManager->RegisterTool( new PCBNEW_CONTROL );
    m_toolManager->RegisterTool( new SELECTION_TOOL );
    m_toolManager->RegisterTool( new COMMON_TOOLS );    // for std context menus (zoom & grid)
    m_toolManager->InitTools();
    m_toolManager->InvokeTool( "pcbnew.InteractiveSelection" );

    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateVToolbar();

    ReCreateLibraryList();
    UpdateTitle();

    // If a footprint was previously loaded, reload it
    if( getCurNickname().size() && getCurFootprintName().size() )
    {
        LIB_ID id;

        id.SetLibNickname( getCurNickname() );
        id.SetLibItemName( getCurFootprintName() );
        GetBoard()->Add( loadFootprint( id ) );
    }

    drawPanel->DisplayBoard( m_Pcb );

    m_auimgr.SetManagedWindow( this );
    m_auimgr.SetArtProvider( new EDA_DOCKART() );

    // Horizontal items; layers 4 - 6
    m_auimgr.AddPane( m_mainToolBar, EDA_PANE().VToolbar().Name( "MainToolbar" ).Top().Layer(6) );
    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( "MsgPanel" ).Bottom().Layer(6) );

    // Vertical items; layers 1 - 3
    m_auimgr.AddPane( m_libList, EDA_PANE().Palette().Name( "Libraries" ).Left().Layer(2)
                      .CaptionVisible( false ).MinSize( 100, -1 ).BestSize( 200, -1 ) );
    m_auimgr.AddPane( m_footprintList, EDA_PANE().Palette().Name( "Footprints" ).Left().Layer(1)
                      .CaptionVisible( false ).MinSize( 100, -1 ).BestSize( 300, -1 ) );

    m_auimgr.AddPane( GetGalCanvas(), EDA_PANE().Canvas().Name( "DrawFrame" ).Center() );

    // after changing something to the aui manager call Update() to reflect the changes
    m_auimgr.Update();

    GetGalCanvas()->GetGAL()->SetAxesEnabled( true );

    GetGalCanvas()->GetView()->SetScale( GetZoomLevelCoeff() / GetScreen()->GetZoom() );
    ActivateGalCanvas();

    // Restore last zoom.  (If auto-zooming we'll adjust when we load the footprint.)
    GetGalCanvas()->GetView()->SetScale( m_lastZoom );

    updateView();

    if( !IsModal() )        // For modal mode, calling ShowModal() will show this frame
    {
        Raise();            // On some window managers, this is needed
        Show( true );
    }
}


FOOTPRINT_VIEWER_FRAME::~FOOTPRINT_VIEWER_FRAME()
{
    GetGalCanvas()->StopDrawing();
    GetGalCanvas()->GetView()->Clear();
    // Be sure any event cannot be fired after frame deletion:
    GetGalCanvas()->SetEvtHandlerEnabled( false );
}


void FOOTPRINT_VIEWER_FRAME::OnCloseWindow( wxCloseEvent& Event )
{
    // A workaround to avoid flicker, in modal mode when modview frame is destroyed,
    // when the aui toolbar is not docked (i.e. shown in a miniframe)
    // (usefull on windows only)
    m_mainToolBar->SetFocus();

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

    GetGalCanvas()->Refresh();
}


void FOOTPRINT_VIEWER_FRAME::ReCreateFootprintList()
{
    m_footprintList->Clear();

    if( !getCurNickname() )
    {
        setCurFootprintName( wxEmptyString );
        return;
    }

    auto fp_info_list = FOOTPRINT_LIST::GetInstance( Kiway() );

    wxString nickname = getCurNickname();

    fp_info_list->ReadFootprintFiles( Prj().PcbFootprintLibs(), !nickname ? NULL : &nickname );

    if( fp_info_list->GetErrorCount() )
    {
        fp_info_list->DisplayErrors( this );

        // For footprint libraries that support one footprint per file, there may have been
        // valid footprints read so show the footprints that loaded properly.
        if( fp_info_list->GetList().size() == 0 )
            return;
    }

    for( auto& footprint : fp_info_list->GetList() )
    {
        m_footprintList->Append( footprint->GetFootprintName() );
    }

    int index = m_footprintList->FindString( getCurFootprintName() );

    if( index == wxNOT_FOUND )
        setCurFootprintName( wxEmptyString );
    else
    {
        m_footprintList->SetSelection( index, true );
        m_footprintList->EnsureVisible( index );
    }
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

        // Delete the current footprint (MUST reset tools first)
        GetToolManager()->ResetTools( TOOL_BASE::MODEL_RELOAD );

        GetBoard()->DeleteAllModules();

        LIB_ID id;
        id.SetLibNickname( getCurNickname() );
        id.SetLibItemName( getCurFootprintName() );

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
                        GetChars( ioe.What() ) );

            DisplayError( this, msg );
        }

        UpdateTitle();

        updateView();

        GetGalCanvas()->Refresh();
        Update3DView( true );
    }
}


void FOOTPRINT_VIEWER_FRAME::DClickOnFootprintList( wxCommandEvent& event )
{
    AddFootprintToPCB( event );
}


void FOOTPRINT_VIEWER_FRAME::AddFootprintToPCB( wxCommandEvent& event )
{
    if( IsModal() )
    {
        if( m_footprintList->GetSelection() >= 0 )
        {
            LIB_ID fpid( getCurNickname(), m_footprintList->GetStringSelection() );
            DismissModal( true, fpid.Format() );
        }
        else
        {
            DismissModal( false );
        }

        Close( true );
    }
    else if( GetBoard()->GetFirstModule() )
    {
        PCB_EDIT_FRAME* pcbframe = (PCB_EDIT_FRAME*) Kiway().Player( FRAME_PCB, false );

        if( pcbframe == NULL )      // happens when the board editor is not active (or closed)
        {
            DisplayErrorMessage( this, _( "No board currently open." ) );
            return;
        }

        pcbframe->GetToolManager()->RunAction( PCB_ACTIONS::selectionClear, true );
        BOARD_COMMIT commit( pcbframe );

        // Create the "new" module
        MODULE* newmodule = new MODULE( *GetBoard()->GetFirstModule() );
        newmodule->SetParent( pcbframe->GetBoard() );
        newmodule->SetLink( 0 );

        wxPoint cursor_pos = pcbframe->GetCrossHairPosition();

        commit.Add( newmodule );
        pcbframe->SetCrossHairPosition( wxPoint( 0, 0 ) );
        pcbframe->PlaceModule( newmodule );
        newmodule->SetPosition( wxPoint( 0, 0 ) );
        pcbframe->SetCrossHairPosition( cursor_pos );
        newmodule->SetTimeStamp( GetNewTimeStamp() );
        commit.Push( wxT( "Insert module" ) );

        pcbframe->Raise();
        pcbframe->GetToolManager()->RunAction( PCB_ACTIONS::placeModule, true, newmodule );

        newmodule->ClearFlags();
    }
}


void FOOTPRINT_VIEWER_FRAME::LoadSettings( wxConfigBase* aCfg )
{
    PCB_BASE_FRAME::LoadSettings( aCfg );

    // Fetch grid settings from Footprint Editor
    wxString footprintEditor = FOOTPRINT_EDIT_FRAME_NAME;
    bool     btmp;
    COLOR4D  wtmp;

    if( aCfg->Read( footprintEditor + ShowGridEntryKeyword, &btmp ) )
        SetGridVisibility( btmp );

    if( wtmp.SetFromWxString( aCfg->Read( footprintEditor + GridColorEntryKeyword, wxT( "NONE" ) ) ) )
        SetGridColor( wtmp );

    // Grid shape, etc.
    GetGalDisplayOptions().ReadAppConfig( *aCfg, footprintEditor );

    m_configSettings.Load( aCfg );  // mainly, load the color config

    aCfg->Read( ConfigBaseName() + AUTO_ZOOM_KEY, &m_autoZoom, true );
    aCfg->Read( ConfigBaseName() + ZOOM_KEY, &m_lastZoom, 10.0 );
}


void FOOTPRINT_VIEWER_FRAME::SaveSettings( wxConfigBase* aCfg )
{
    PCB_BASE_FRAME::SaveSettings( aCfg );

    aCfg->Write( ConfigBaseName() + AUTO_ZOOM_KEY, m_autoZoom );
    aCfg->Write( ConfigBaseName() + ZOOM_KEY, GetGalCanvas()->GetView()->GetScale() );
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


void FOOTPRINT_VIEWER_FRAME::OnUpdateFootprintButton( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( GetBoard()->GetFirstModule() != nullptr );
}


bool FOOTPRINT_VIEWER_FRAME::ShowModal( wxString* aFootprint, wxWindow* aParent )
{
    if( aFootprint && !aFootprint->IsEmpty() )
    {
        wxString msg;
        LIB_TABLE* fpTable = Prj().PcbFootprintLibs();
        LIB_ID fpid;

        fpid.Parse( *aFootprint, LIB_ID::ID_PCB, true );

        if( fpid.IsValid() )
        {
            wxString nickname = fpid.GetLibNickname();

            if( !fpTable->HasLibrary( fpid.GetLibNickname(), false ) )
            {
                msg.sprintf( _( "The current configuration does not include a library with the\n"
                                "nickname \"%s\".  Use Manage Footprint Libraries\n"
                                "to edit the configuration." ), nickname );
                DisplayErrorMessage( aParent, _( "Footprint library not found." ), msg );
            }
            else if ( !fpTable->HasLibrary( fpid.GetLibNickname(), true ) )
            {
                msg.sprintf( _( "The library with the nickname \"%s\" is not enabled\n"
                                "in the current configuration.  Use Manage Footprint Libraries to\n"
                                "edit the configuration." ), nickname );
                DisplayErrorMessage( aParent, _( "Footprint library not enabled." ), msg );
            }
            else
            {
                setCurNickname( nickname );
                setCurFootprintName( fpid.GetLibItemName() );
                ReCreateFootprintList();
            }

            SelectAndViewFootprint( NEW_PART );
        }
    }

    return KIWAY_PLAYER::ShowModal( aFootprint, aParent );
}


void FOOTPRINT_VIEWER_FRAME::Update3DView( bool aForceReload, const wxString* aTitle )
{
    wxString title = wxString::Format( _( "3D Viewer" ) + wxT( " \u2014 %s" ),
                                       getCurFootprintName() );
    PCB_BASE_FRAME::Update3DView( aForceReload, &title );
}


COLOR4D FOOTPRINT_VIEWER_FRAME::GetGridColor()
{
    return Settings().Colors().GetItemColor( LAYER_GRID );
}


void FOOTPRINT_VIEWER_FRAME::OnIterateFootprintList( wxCommandEvent& event )
{
    switch( event.GetId() )
    {
    case ID_MODVIEW_NEXT:
        SelectAndViewFootprint( NEXT_PART );
        break;

    case ID_MODVIEW_PREVIOUS:
        SelectAndViewFootprint( PREVIOUS_PART );
        break;

    default:
        wxString id = wxString::Format( "%i", event.GetId() );
        wxFAIL_MSG( "FOOTPRINT_VIEWER_FRAME::OnIterateFootprintList error: id = " + id );
    }
}


void FOOTPRINT_VIEWER_FRAME::UpdateTitle()
{
    wxString title;
    wxString path;

    title.Printf( _( "Footprint Library Browser" ) + L" \u2014 %s",
                  getCurNickname().IsEmpty() ? _( "no library selected" ) : getCurNickname() );

    // Now, add the full path, for info
    if( !getCurNickname().IsEmpty() )
    {
        FP_LIB_TABLE* libtable = Prj().PcbFootprintLibs();
        const LIB_TABLE_ROW* row = libtable->FindRow( getCurNickname() );

        if( row )
            title << L" \u2014 " << row->GetFullURI( true );
    }

    SetTitle( title );
}


void FOOTPRINT_VIEWER_FRAME::SelectCurrentFootprint( wxCommandEvent& event )
{
    LIB_ID current( getCurNickname(), getCurFootprintName() );

    MODULE* module = SelectFootprintFromLibTree( current, false );

    if( module )
    {
        const LIB_ID& fpid = module->GetFPID();

        setCurNickname( fpid.GetLibNickname() );
        setCurFootprintName( fpid.GetLibItemName() );

        int index = m_libList->FindString( fpid.GetLibNickname() );

        if( index != wxNOT_FOUND )
        {
            m_libList->SetSelection( index, true );
            m_libList->EnsureVisible( index );
        }

        ReCreateFootprintList();

        SelectAndViewFootprint( NEW_PART );
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
        m_footprintList->EnsureVisible( selection );

        setCurFootprintName( m_footprintList->GetString( (unsigned) selection ) );

        // Delete the current footprint
        GetBoard()->DeleteAllModules();

        MODULE* footprint = Prj().PcbFootprintLibs()->FootprintLoad( getCurNickname(),
                                                                     getCurFootprintName() );

        if( footprint )
            GetBoard()->Add( footprint, ADD_APPEND );

        Update3DView( true );

        updateView();
    }

    UpdateTitle();

    GetGalCanvas()->Refresh();
}


void FOOTPRINT_VIEWER_FRAME::ApplyDisplaySettingsToGAL()
{
    auto painter = static_cast<KIGFX::PCB_PAINTER*>( GetGalCanvas()->GetView()->GetPainter() );
    KIGFX::PCB_RENDER_SETTINGS* settings = painter->GetSettings();
    settings->LoadDisplayOptions( &m_DisplayOptions, false );

    GetGalCanvas()->GetView()->UpdateAllItems( KIGFX::ALL );
    GetGalCanvas()->Refresh();
}


void FOOTPRINT_VIEWER_FRAME::updateView()
{
    auto dp = static_cast<PCB_DRAW_PANEL_GAL*>( GetGalCanvas() );
    dp->UseColorScheme( &Settings().Colors() );
    dp->DisplayBoard( GetBoard() );

    m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );

    if( m_autoZoom )
        m_toolManager->RunAction( ACTIONS::zoomFitScreen, true );
    else
        m_toolManager->RunAction( ACTIONS::centerContents, true );

    UpdateMsgPanel();
}


void FOOTPRINT_VIEWER_FRAME::CloseFootprintViewer( wxCommandEvent& event )
{
    Close();
}
