/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
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
#include <kiface_i.h>
#include <pgm_base.h>
#include <sch_draw_panel.h>
#include <sch_view.h>
#include <msgpanel.h>
#include <bitmaps.h>
#include <eda_dockart.h>
#include <sch_edit_frame.h>
#include <eeschema_id.h>
#include <general.h>
#include <viewlib_frame.h>
#include <symbol_lib_table.h>
#include <dialog_helpers.h>
#include <class_libentry.h>
#include <class_library.h>
#include <view/view_controls.h>
#include <sch_painter.h>
#include <confirm.h>
#include <tool/tool_manager.h>
#include <tool/action_toolbar.h>
#include <tool/tool_dispatcher.h>
#include <tools/ee_actions.h>
#include <tool/common_tools.h>
#include <tool/zoom_tool.h>
#include <tools/lib_control.h>
#include <tools/ee_inspection_tool.h>

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
    EVT_CHOICE( ID_LIBVIEW_SELECT_PART_NUMBER, LIB_VIEW_FRAME::onSelectSymbolUnit )

    // listbox events
    EVT_LISTBOX( ID_LIBVIEW_LIB_LIST, LIB_VIEW_FRAME::ClickOnLibList )
    EVT_LISTBOX( ID_LIBVIEW_CMP_LIST, LIB_VIEW_FRAME::ClickOnCmpList )
    EVT_LISTBOX_DCLICK( ID_LIBVIEW_CMP_LIST, LIB_VIEW_FRAME::DClickOnCmpList )

    // Menu (and/or hotkey) events
    EVT_MENU( wxID_CLOSE, LIB_VIEW_FRAME::CloseLibraryViewer )
    EVT_MENU( ID_GRID_SETTINGS, SCH_BASE_FRAME::OnGridSettings )

    EVT_UPDATE_UI( ID_LIBVIEW_SELECT_PART_NUMBER, LIB_VIEW_FRAME::onUpdateUnitChoice )

END_EVENT_TABLE()


#define LIB_VIEW_NAME "ViewlibFrame"
#define LIB_VIEW_NAME_MODAL "ViewlibFrameModal"

#define LIB_VIEW_STYLE ( KICAD_DEFAULT_DRAWFRAME_STYLE )
#define LIB_VIEW_STYLE_MODAL ( KICAD_DEFAULT_DRAWFRAME_STYLE | wxFRAME_FLOAT_ON_PARENT )


LIB_VIEW_FRAME::LIB_VIEW_FRAME( KIWAY* aKiway, wxWindow* aParent, FRAME_T aFrameType,
                                const wxString& aLibraryName ) :
    SCH_BASE_FRAME( aKiway, aParent, aFrameType, _( "Symbol Library Browser" ),
                    wxDefaultPosition, wxDefaultSize,
                    aFrameType == FRAME_SCH_VIEWER_MODAL ? LIB_VIEW_STYLE_MODAL : LIB_VIEW_STYLE,
                    aFrameType == FRAME_SCH_VIEWER_MODAL ? LIB_VIEW_NAME_MODAL : LIB_VIEW_NAME ),
        m_libList( nullptr ), m_cmpList( nullptr ), m_previewItem( nullptr )
{
    wxASSERT( aFrameType == FRAME_SCH_VIEWER || aFrameType == FRAME_SCH_VIEWER_MODAL );

    if( aFrameType == FRAME_SCH_VIEWER_MODAL )
        SetModal( true );

    // Force the frame name used in config. the lib viewer frame has a name
    // depending on aFrameType (needed to identify the frame by wxWidgets),
    // but only one configuration is preferable.
    m_configName = LIB_VIEW_NAME;

    // Give an icon
    wxIcon  icon;
    icon.CopyFromBitmap( KiBitmap( library_browse_xpm ) );
    SetIcon( icon );

    m_libListWidth = 200;
    m_cmpListWidth = 300;
    m_listPowerCmpOnly = false;

    // Initialize grid id to the default value (50 mils):
    m_LastGridSizeId = ID_POPUP_GRID_LEVEL_50 - ID_POPUP_GRID_LEVEL_1000;

    SetScreen( new SCH_SCREEN( aKiway ) );
    GetScreen()->m_Center = true;      // Axis origin centered on screen.
    LoadSettings( config() );

    // Synchronize some draw options
    SetShowElectricalType( true );
    // Ensure axis are always drawn (initial default display was not drawn)
    KIGFX::GAL_DISPLAY_OPTIONS& gal_opts = GetGalDisplayOptions();
    gal_opts.m_axesEnabled = true;
    GetCanvas()->GetGAL()->SetAxesEnabled( true );
    GetRenderSettings()->m_ShowPinsElectricalType = GetShowElectricalType();
    GetCanvas()->GetGAL()->SetGridVisibility( IsGridVisible() );

    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    setupTools();
    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateVToolbar();

    m_libList = new wxListBox( this, ID_LIBVIEW_LIB_LIST, wxDefaultPosition, wxDefaultSize,
                               0, NULL, wxLB_HSCROLL | wxNO_BORDER );

    m_cmpList = new wxListBox( this, ID_LIBVIEW_CMP_LIST, wxDefaultPosition, wxDefaultSize,
                               0, NULL, wxLB_HSCROLL | wxNO_BORDER );

    if( aLibraryName.empty() )
    {
        ReCreateListLib();
    }
    else
    {
        m_libraryName = aLibraryName;
        m_entryName.Clear();
        m_unit = 1;
        m_convert = 1;
    }

    m_selection_changed = false;

    DisplayLibInfos();

    m_auimgr.SetManagedWindow( this );
    m_auimgr.SetArtProvider( new EDA_DOCKART() );

    // Manage main toolbar
    m_auimgr.AddPane( m_mainToolBar, EDA_PANE().HToolbar().Name( "MainToolbar" ).Top().Layer(6) );
    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( "MsgPanel" ).Bottom().Layer(6) );

    m_auimgr.AddPane( m_libList, EDA_PANE().Palette().Name( "Libraries" ).Left().Layer(3)
                      .CaptionVisible( false ).MinSize( 80, -1 ).BestSize( m_libListWidth, -1 ) );
    m_auimgr.AddPane( m_cmpList, EDA_PANE().Palette().Name( "Symbols" ).Left().Layer(1)
                      .CaptionVisible( false ).MinSize( 80, -1 ).BestSize( m_cmpListWidth, -1 ) );

    m_auimgr.AddPane( GetCanvas(), EDA_PANE().Canvas().Name( "DrawFrame" ).Center() );

    m_auimgr.GetPane( m_libList ).Show( aLibraryName.empty() );

    m_auimgr.Update();

    GetToolManager()->RunAction( ACTIONS::gridPreset, true, m_LastGridSizeId );

    if( !IsModal() )        // For modal mode, calling ShowModal() will show this frame
    {
        Raise();
        Show( true );
    }

    SyncView();
    GetCanvas()->GetViewControls()->SetSnapping( true );

    // Set the working/draw area size to display a symbol to a reasonable value:
    // A 450mm x 450mm with a origin at the area center looks like a large working area
    double max_size_x = Millimeter2iu( 450 );
    double max_size_y = Millimeter2iu( 450 );
    BOX2D bbox;
    bbox.SetOrigin( -max_size_x /2, -max_size_y/2 );
    bbox.SetSize( max_size_x, max_size_y );
    GetCanvas()->GetView()->SetBoundary( bbox );
    GetToolManager()->RunAction( ACTIONS::zoomFitScreen, true );
}


LIB_VIEW_FRAME::~LIB_VIEW_FRAME()
{
    if( m_previewItem )
        GetCanvas()->GetView()->Remove( m_previewItem );
}


void LIB_VIEW_FRAME::setupTools()
{
    // Create the manager and dispatcher & route draw panel events to the dispatcher
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( GetScreen(), GetCanvas()->GetView(),
                                   GetCanvas()->GetViewControls(), this );
    m_actions = new EE_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager, m_actions );

    // Register tools
    m_toolManager->RegisterTool( new COMMON_TOOLS );
    m_toolManager->RegisterTool( new ZOOM_TOOL );
    m_toolManager->RegisterTool( new EE_INSPECTION_TOOL );  // manage show datasheet
    m_toolManager->RegisterTool( new EE_SELECTION_TOOL );   // manage context menu
    m_toolManager->RegisterTool( new LIB_CONTROL );

    m_toolManager->InitTools();

    // Run the selection tool, it is supposed to be always active
    // It also manages the mouse right click to show the context menu
    m_toolManager->InvokeTool( "eeschema.InteractiveSelection" );

    GetCanvas()->SetEventDispatcher( m_toolDispatcher );
}


void LIB_VIEW_FRAME::SetUnitAndConvert( int aUnit, int aConvert )
{
    m_unit = aUnit > 0 ? aUnit : 1;
    m_convert = aConvert > 0 ? aConvert : LIB_ITEM::LIB_CONVERT::BASE;
    m_selection_changed = false;

    updatePreviewSymbol();
}


LIB_ALIAS* LIB_VIEW_FRAME::GetSelectedAlias() const
{
    LIB_ALIAS* alias = NULL;

    if( !m_libraryName.IsEmpty() && !m_entryName.IsEmpty() )
        alias = Prj().SchSymbolLibTable()->LoadSymbol( m_libraryName, m_entryName );

    return alias;
}


LIB_PART* LIB_VIEW_FRAME::GetSelectedSymbol() const
{
    LIB_PART* symbol = NULL;
    LIB_ALIAS* alias = GetSelectedAlias();

    if( alias )
        symbol = alias->GetPart();

    return symbol;
}


void LIB_VIEW_FRAME::updatePreviewSymbol()
{
    LIB_ALIAS* alias = GetSelectedAlias();
    KIGFX::SCH_VIEW* view = GetCanvas()->GetView();

    if( m_previewItem )
    {
        view->Remove( m_previewItem );
        m_previewItem = nullptr;
    }

    ClearMsgPanel();

    if( alias )
    {
        GetRenderSettings()->m_ShowUnit = m_unit;
        GetRenderSettings()->m_ShowConvert = m_convert;

        view->Add( alias );
        m_previewItem = alias;

        AppendMsgPanel( _( "Name" ), alias->GetName(), BLUE, 6 );
        AppendMsgPanel( _( "Description" ), alias->GetDescription(), CYAN, 6 );
        AppendMsgPanel( _( "Key words" ), alias->GetKeyWords(), DARKDARKGRAY );
    }

    GetCanvas()->ForceRefresh();
}


bool LIB_VIEW_FRAME::ShowModal( wxString* aSymbol, wxWindow* aParent )
{
    if( aSymbol && !aSymbol->IsEmpty() )
    {
        wxString msg;
        LIB_TABLE* libTable = Prj().SchSymbolLibTable();
        LIB_ID libid;

        libid.Parse( *aSymbol, LIB_ID::ID_SCH, true );

        if( libid.IsValid() )
        {
            wxString nickname = libid.GetLibNickname();

            if( !libTable->HasLibrary( libid.GetLibNickname(), false ) )
            {
                msg.sprintf( _( "The current configuration does not include a library with the\n"
                                "nickname \"%s\".  Use Manage Symbol Libraries\n"
                                "to edit the configuration." ), nickname );
                DisplayErrorMessage( aParent, _( "Symbol library not found." ), msg );
            }
            else if ( !libTable->HasLibrary( libid.GetLibNickname(), true ) )
            {
                msg.sprintf( _( "The library with the nickname \"%s\" is not enabled\n"
                                "in the current configuration.  Use Manage Symbol Libraries to\n"
                                "edit the configuration." ), nickname );
                DisplayErrorMessage( aParent, _( "Symbol library not enabled." ), msg );
            }
            else
            {
                SetSelectedLibrary( libid.GetLibNickname() );
                SetSelectedComponent( libid.GetLibItemName() );
            }
        }
    }

    return KIWAY_PLAYER::ShowModal( aSymbol, aParent );
}


void LIB_VIEW_FRAME::OnCloseWindow( wxCloseEvent& Event )
{
    GetCanvas()->StopDrawing();

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


void LIB_VIEW_FRAME::onUpdateUnitChoice( wxUpdateUIEvent& aEvent )
{
    LIB_PART* part = GetSelectedSymbol();

    int unit_count = 1;

    if( part )
        unit_count = std::max( part->GetUnitCount(), 1 );

    m_unitChoice->Enable( unit_count > 1 );

    if( unit_count > 1 )
    {
        // rebuild the unit list if it is not suitable (after a new selection for instance)
        if( unit_count != (int)m_unitChoice->GetCount() )
        {
            m_unitChoice->Clear();

            for( int ii = 0; ii < unit_count; ii++ )
                m_unitChoice->Append( wxString::Format( _( "Unit %c" ), 'A' + ii ) );

        }

        if( m_unitChoice->GetSelection() != std::max( 0, m_unit - 1 ) )
            m_unitChoice->SetSelection( std::max( 0, m_unit - 1 ) );
    }
    else if( m_unitChoice->GetCount() )
        m_unitChoice->Clear();
}


double LIB_VIEW_FRAME::BestZoom()
{
    LIB_PART*   part = nullptr;
    double      defaultLibraryZoom = 7.33;

    if( m_libraryName.IsEmpty() || m_entryName.IsEmpty() )
    {
        GetCanvas()->GetView()->SetCenter( VECTOR2D( 0, 0 ) );
        return defaultLibraryZoom;
    }

    LIB_ALIAS* alias = nullptr;

    try
    {
        alias = Prj().SchSymbolLibTable()->LoadSymbol( m_libraryName, m_entryName );
    }
    catch( ... )
    {
    }

    if( alias )
        part = alias->GetPart();

    if( !part )
    {
        GetCanvas()->GetView()->SetCenter( VECTOR2D( 0, 0 ) );
        return defaultLibraryZoom;
    }

    EDA_RECT boundingBox = part->GetUnitBoundingBox( m_unit, m_convert );

    double  sizeX  = (double) boundingBox.GetWidth();
    double  sizeY  = (double) boundingBox.GetHeight();
    wxPoint centre = boundingBox.Centre();

    // Reserve a 20% margin around component bounding box.
    double  margin_scale_factor = 1.2;

    return bestZoom( sizeX, sizeY, margin_scale_factor, centre );
}


bool LIB_VIEW_FRAME::ReCreateListLib()
{
    if( !m_libList )
        return false;

    m_libList->Clear();

    std::vector< wxString > libs = Prj().SchSymbolLibTable()->GetLogicalLibs();

    // Remove not allowed libs from main list, if the allowed lib list is not empty
    if( m_allowedLibs.GetCount() )
    {
        for( unsigned ii = 0; ii < libs.size(); )
        {
            if( m_allowedLibs.Index( libs[ii] ) == wxNOT_FOUND )
                libs.erase( libs.begin() + ii );
            else
                ii++;
        }
    }

    // Remove libs which have no power components, if this filter is activated
    if( m_listPowerCmpOnly )
    {
        for( unsigned ii = 0; ii < libs.size(); )
        {
            wxArrayString aliasNames;

            Prj().SchSymbolLibTable()->EnumerateSymbolLib( libs[ii], aliasNames, true );

            if( aliasNames.IsEmpty() )
                libs.erase( libs.begin() + ii );
            else
                ii++;
        }
    }

    if( libs.empty() )
        return true;

    wxArrayString libNames;

    for( const auto& name : libs )
        libNames.Add( name );

    m_libList->Append( libNames );

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
        m_convert = LIB_ITEM::LIB_CONVERT::BASE;
    }

    bool cmp_changed = ReCreateListCmp();
    DisplayLibInfos();
    GetCanvas()->Refresh();

    return cmp_changed;
}


bool LIB_VIEW_FRAME::ReCreateListCmp()
{
    if( m_cmpList == NULL )
        return false;

    wxArrayString aliasNames;

    try
    {
        Prj().SchSymbolLibTable()->EnumerateSymbolLib( m_libraryName, aliasNames,
                                                       m_listPowerCmpOnly );
    }
    catch( const IO_ERROR& ) {}   // ignore, it is handled below

    m_cmpList->Clear();

    if( aliasNames.IsEmpty() )
    {
        m_libraryName = wxEmptyString;
        m_entryName = wxEmptyString;
        m_convert = LIB_ITEM::LIB_CONVERT::BASE;
        m_unit    = 1;
        return true;
    }

    m_cmpList->Append( aliasNames );

    int index = m_cmpList->FindString( m_entryName );
    bool changed = false;

    if( index == wxNOT_FOUND )
    {
        // Select the first library entry when the previous entry name does not exist in
        // the current library.
        m_convert   = LIB_ITEM::LIB_CONVERT::BASE;
        m_unit      = 1;
        index       = 0;
        changed     = true;
        m_entryName = wxEmptyString;
    }

    m_cmpList->SetSelection( index, true );

    wxCommandEvent evt( wxEVT_COMMAND_LISTBOX_SELECTED, ID_LIBVIEW_CMP_LIST );
    ProcessEvent( evt );

    return changed;
}


void LIB_VIEW_FRAME::ClickOnLibList( wxCommandEvent& event )
{
    int ii = m_libList->GetSelection();

    if( ii < 0 )
        return;

    m_selection_changed = true;

    SetSelectedLibrary( m_libList->GetString( ii ) );
}


void LIB_VIEW_FRAME::SetSelectedLibrary( const wxString& aLibraryName )
{
    if( m_libraryName == aLibraryName )
        return;

    m_libraryName = aLibraryName;
    ReCreateListCmp();
    GetCanvas()->Refresh();
    DisplayLibInfos();

    // Ensure the corresponding line in m_libList is selected
    // (which is not necessary the case if SetSelectedLibrary is called
    // by another caller than ClickOnLibList.
    m_libList->SetStringSelection( m_libraryName, true );
}


void LIB_VIEW_FRAME::ClickOnCmpList( wxCommandEvent& event )
{
    int ii = m_cmpList->GetSelection();

    if( ii < 0 )
        return;

    m_selection_changed = true;

    SetSelectedComponent( m_cmpList->GetString( ii ) );
}


void LIB_VIEW_FRAME::SetSelectedComponent( const wxString& aComponentName )
{
    if( m_entryName != aComponentName )
    {
        m_entryName = aComponentName;

        // Ensure the corresponding line in m_cmpList is selected
        // (which is not necessarily the case if SetSelectedComponent is called
        // by another caller than ClickOnCmpList.
        m_cmpList->SetStringSelection( aComponentName, true );
        DisplayLibInfos();

        if( m_selection_changed )
        {
            m_unit = 1;
            m_convert = LIB_ITEM::LIB_CONVERT::BASE;
            m_selection_changed = false;
        }

        updatePreviewSymbol();
        m_toolManager->RunAction( ACTIONS::zoomFitScreen, true );
    }
}


void LIB_VIEW_FRAME::DClickOnCmpList( wxCommandEvent& event )
{
    m_toolManager->RunAction( EE_ACTIONS::addSymbolToSchematic, true );
}


#define LIBLIST_WIDTH_KEY "ViewLiblistWidth"
#define CMPLIST_WIDTH_KEY "ViewCmplistWidth"
#define CMPVIEW_SHOW_PINELECTRICALTYPE_KEY "ViewCmpShowPinElectricalType"


void LIB_VIEW_FRAME::LoadSettings( wxConfigBase* aCfg )
{
    EDA_DRAW_FRAME::LoadSettings( aCfg );

    // Fetch display settings from Symbol Editor as the Symbol Viewer
    // doesn't have its own config
    wxString symbolEditor = LIB_EDIT_FRAME_NAME;
    bool     btmp;
    COLOR4D  wtmp;

    if( aCfg->Read( symbolEditor + ShowGridEntryKeyword, &btmp ) )
        SetGridVisibility( btmp );

    if( wtmp.SetFromWxString( aCfg->Read( symbolEditor + GridColorEntryKeyword, wxT( "NONE" ) ) ) )
        SetGridColor( wtmp );

    // Grid shape, etc.
    GetGalDisplayOptions().ReadAppConfig( *aCfg, symbolEditor );

    aCfg->Read( LIBLIST_WIDTH_KEY, &m_libListWidth, 150 );
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


void LIB_VIEW_FRAME::CommonSettingsChanged( bool aEnvVarsChanged )
{
    SCH_BASE_FRAME::CommonSettingsChanged( aEnvVarsChanged );

    if( aEnvVarsChanged )
        ReCreateListLib();
}


void LIB_VIEW_FRAME::OnActivate( wxActivateEvent& event )
{
    bool changed = m_libList ? ReCreateListLib() : false;

    if (changed)
        m_selection_changed = true;

    updatePreviewSymbol();

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


const BOX2I LIB_VIEW_FRAME::GetDocumentExtents() const
{
    LIB_ALIAS*  alias = GetSelectedAlias();
    LIB_PART*   part = alias ? alias->GetPart() : nullptr;

    if( !part )
    {
        return BOX2I( VECTOR2I(-200, -200), VECTOR2I( 400, 400 ) );
    }
    else
    {
        EDA_RECT bbox = part->GetUnitBoundingBox( m_unit, m_convert );
        return BOX2I( bbox.GetOrigin(), VECTOR2I( bbox.GetWidth(), bbox.GetHeight() ) );

    }
}


void LIB_VIEW_FRAME::FinishModal()
{
    if( m_cmpList->GetSelection() >= 0 )
        DismissModal( true, m_libraryName + ':' + m_cmpList->GetStringSelection() );
    else
        DismissModal( false );

    Close( true );
}

