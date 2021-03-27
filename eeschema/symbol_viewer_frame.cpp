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

#include <bitmaps.h>
#include <class_library.h>
#include <confirm.h>
#include <dialog_helpers.h>
#include <dialog_choose_symbol.h>
#include <eeschema_id.h>
#include <eeschema_settings.h>
#include <kiface_i.h>
#include <kiway.h>
#include <symbol_viewer_frame.h>
#include <widgets/msgpanel.h>
#include <sch_view.h>
#include <sch_painter.h>
#include <symbol_lib_table.h>
#include <symbol_tree_model_adapter.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <tool/action_toolbar.h>
#include <tool/common_control.h>
#include <tool/common_tools.h>
#include <tool/editor_conditions.h>
#include <tool/selection.h>
#include <tool/tool_dispatcher.h>
#include <tool/tool_manager.h>
#include <tool/zoom_tool.h>
#include <tools/ee_actions.h>
#include <tools/symbol_editor_control.h>
#include <tools/ee_inspection_tool.h>
#include <view/view_controls.h>

#include <default_values.h>

// Save previous component library viewer state.
wxString SYMBOL_VIEWER_FRAME::m_libraryName;
wxString SYMBOL_VIEWER_FRAME::m_entryName;

int SYMBOL_VIEWER_FRAME::m_unit = 1;
int SYMBOL_VIEWER_FRAME::m_convert = 1;


BEGIN_EVENT_TABLE( SYMBOL_VIEWER_FRAME, EDA_DRAW_FRAME )
    // Window events
    EVT_SIZE( SYMBOL_VIEWER_FRAME::OnSize )
    EVT_ACTIVATE( SYMBOL_VIEWER_FRAME::OnActivate )

    // Toolbar events
    EVT_TOOL( ID_LIBVIEW_SELECT_PART, SYMBOL_VIEWER_FRAME::OnSelectSymbol )
    EVT_TOOL( ID_LIBVIEW_NEXT, SYMBOL_VIEWER_FRAME::onSelectNextSymbol )
    EVT_TOOL( ID_LIBVIEW_PREVIOUS, SYMBOL_VIEWER_FRAME::onSelectPreviousSymbol )
    EVT_CHOICE( ID_LIBVIEW_SELECT_PART_NUMBER, SYMBOL_VIEWER_FRAME::onSelectSymbolUnit )

    // listbox events
    EVT_LISTBOX( ID_LIBVIEW_LIB_LIST, SYMBOL_VIEWER_FRAME::ClickOnLibList )
    EVT_LISTBOX( ID_LIBVIEW_CMP_LIST, SYMBOL_VIEWER_FRAME::ClickOnCmpList )
    EVT_LISTBOX_DCLICK( ID_LIBVIEW_CMP_LIST, SYMBOL_VIEWER_FRAME::DClickOnCmpList )

    // Menu (and/or hotkey) events
    EVT_MENU( wxID_CLOSE, SYMBOL_VIEWER_FRAME::CloseLibraryViewer )
    EVT_MENU( ID_GRID_SETTINGS, SCH_BASE_FRAME::OnGridSettings )

    EVT_UPDATE_UI( ID_LIBVIEW_SELECT_PART_NUMBER, SYMBOL_VIEWER_FRAME::onUpdateUnitChoice )

END_EVENT_TABLE()


#define LIB_VIEW_NAME "ViewlibFrame"
#define LIB_VIEW_NAME_MODAL "ViewlibFrameModal"

#define LIB_VIEW_STYLE ( KICAD_DEFAULT_DRAWFRAME_STYLE )
#define LIB_VIEW_STYLE_MODAL ( KICAD_DEFAULT_DRAWFRAME_STYLE | wxFRAME_FLOAT_ON_PARENT )


SYMBOL_VIEWER_FRAME::SYMBOL_VIEWER_FRAME( KIWAY* aKiway, wxWindow* aParent, FRAME_T aFrameType,
                                          const wxString& aLibraryName ) :
    SCH_BASE_FRAME( aKiway, aParent, aFrameType, _( "Symbol Library Browser" ),
                    wxDefaultPosition, wxDefaultSize,
                    aFrameType == FRAME_SCH_VIEWER_MODAL ? LIB_VIEW_STYLE_MODAL : LIB_VIEW_STYLE,
                    aFrameType == FRAME_SCH_VIEWER_MODAL ? LIB_VIEW_NAME_MODAL : LIB_VIEW_NAME ),
        m_unitChoice( nullptr ),
        m_libList( nullptr ),
        m_symbolList( nullptr ),
        m_previewItem( nullptr )
{
    wxASSERT( aFrameType == FRAME_SCH_VIEWER || aFrameType == FRAME_SCH_VIEWER_MODAL );

    if( aFrameType == FRAME_SCH_VIEWER_MODAL )
        SetModal( true );

    m_aboutTitle = _( "KiCad Symbol Library Viewer" );

    // Force the frame name used in config. the lib viewer frame has a name
    // depending on aFrameType (needed to identify the frame by wxWidgets),
    // but only one configuration is preferable.
    m_configName = LIB_VIEW_NAME;

    // Give an icon
    wxIcon  icon;
    icon.CopyFromBitmap( KiBitmap( BITMAPS::library_browser ) );
    SetIcon( icon );

    m_libListWidth = 200;
    m_symbolListWidth = 300;
    m_listPowerCmpOnly = false;

    SetScreen( new SCH_SCREEN );
    GetScreen()->m_Center = true;      // Axis origin centered on screen.
    LoadSettings( config() );

    // Ensure axis are always drawn (initial default display was not drawn)
    KIGFX::GAL_DISPLAY_OPTIONS& gal_opts = GetGalDisplayOptions();
    gal_opts.m_axesEnabled = true;
    gal_opts.m_gridMinSpacing = 10.0;
    gal_opts.NotifyChanged();

    GetRenderSettings()->LoadColors( GetColorSettings() );
    GetCanvas()->GetGAL()->SetAxesColor( m_colorSettings->GetColor( LAYER_SCHEMATIC_GRID_AXES ) );

    GetRenderSettings()->m_ShowHiddenText = true;
    GetRenderSettings()->m_ShowHiddenPins = true;
    GetRenderSettings()->SetDefaultPenWidth( DEFAULT_LINE_THICKNESS * IU_PER_MILS );

    setupTools();
    setupUIConditions();

    ReCreateHToolbar();
    ReCreateVToolbar();
    ReCreateMenuBar();

    m_libList = new wxListBox( this, ID_LIBVIEW_LIB_LIST, wxDefaultPosition, wxDefaultSize,
                               0, NULL, wxLB_HSCROLL | wxNO_BORDER );

    m_symbolList = new wxListBox( this, ID_LIBVIEW_CMP_LIST, wxDefaultPosition, wxDefaultSize,
                                  0, NULL, wxLB_HSCROLL | wxNO_BORDER );

    if( aLibraryName.empty() )
    {
        ReCreateLibList();
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

    // Manage main toolbar
    m_auimgr.AddPane( m_mainToolBar, EDA_PANE().HToolbar().Name( "MainToolbar" ).Top().Layer(6) );
    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( "MsgPanel" ).Bottom().Layer(6) );

    m_auimgr.AddPane( m_libList, EDA_PANE().Palette().Name( "Libraries" ).Left().Layer(3)
                      .CaptionVisible( false ).MinSize( 80, -1 ).BestSize( m_libListWidth, -1 ) );
    m_auimgr.AddPane( m_symbolList, EDA_PANE().Palette().Name( "Symbols" ).Left().Layer(1)
                      .CaptionVisible( false ).MinSize( 80, -1 ).BestSize( m_symbolListWidth, -1 ) );

    m_auimgr.AddPane( GetCanvas(), EDA_PANE().Canvas().Name( "DrawFrame" ).Center() );

    m_auimgr.GetPane( m_libList ).Show( aLibraryName.empty() );

    m_auimgr.Update();

    if( !IsModal() )        // For modal mode, calling ShowModal() will show this frame
    {
        Raise();
        Show( true );
    }

    SyncView();
    GetCanvas()->SetCanFocus( false );

    setupUnits( config() );

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


SYMBOL_VIEWER_FRAME::~SYMBOL_VIEWER_FRAME()
{
    // Shutdown all running tools
    if( m_toolManager )
        m_toolManager->ShutdownAllTools();

    if( m_previewItem )
        GetCanvas()->GetView()->Remove( m_previewItem );
}


void SYMBOL_VIEWER_FRAME::setupTools()
{
    // Create the manager and dispatcher & route draw panel events to the dispatcher
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( GetScreen(), GetCanvas()->GetView(),
                                   GetCanvas()->GetViewControls(), config(), this );
    m_actions = new EE_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager );

    // Register tools
    m_toolManager->RegisterTool( new COMMON_TOOLS );
    m_toolManager->RegisterTool( new COMMON_CONTROL );
    m_toolManager->RegisterTool( new ZOOM_TOOL );
    m_toolManager->RegisterTool( new EE_INSPECTION_TOOL );  // manage show datasheet
    m_toolManager->RegisterTool( new EE_SELECTION_TOOL );   // manage context menu
    m_toolManager->RegisterTool( new SYMBOL_EDITOR_CONTROL );

    m_toolManager->InitTools();

    // Run the selection tool, it is supposed to be always active
    // It also manages the mouse right click to show the context menu
    m_toolManager->InvokeTool( "eeschema.InteractiveSelection" );

    GetCanvas()->SetEventDispatcher( m_toolDispatcher );
}


void SYMBOL_VIEWER_FRAME::setupUIConditions()
{
    SCH_BASE_FRAME::setupUIConditions();

    ACTION_MANAGER*   mgr = m_toolManager->GetActionManager();
    EDITOR_CONDITIONS cond( this );

    wxASSERT( mgr );

#define ENABLE( x ) ACTION_CONDITIONS().Enable( x )
#define CHECK( x )  ACTION_CONDITIONS().Check( x )

    mgr->SetConditions( ACTIONS::toggleGrid,          CHECK( cond.GridVisible() ) );

    auto electricalTypesShownCondition =
        [this] ( const SELECTION& aSel )
        {
            return GetRenderSettings()->m_ShowPinsElectricalType;
        };

    auto demorganCond =
        [this] ( const SELECTION& )
        {
            LIB_PART* symbol = GetSelectedSymbol();

            return symbol && symbol->HasConversion();
        };

    auto demorganStandardCond =
        [] ( const SELECTION& )
        {
            return m_convert == LIB_ITEM::LIB_CONVERT::BASE;
        };

    auto demorganAlternateCond =
        [] ( const SELECTION& )
        {
            return m_convert == LIB_ITEM::LIB_CONVERT::DEMORGAN;
        };

    auto haveDatasheetCond =
        [this] ( const SELECTION& )
        {
            LIB_PART* symbol = GetSelectedSymbol();

            return symbol && !symbol->GetDatasheetField().GetText().IsEmpty();
        };

    mgr->SetConditions( EE_ACTIONS::showDatasheet,       ENABLE( haveDatasheetCond ) );
    mgr->SetConditions( EE_ACTIONS::showElectricalTypes, CHECK( electricalTypesShownCondition ) );

    mgr->SetConditions( EE_ACTIONS::showDeMorganStandard,
                        ACTION_CONDITIONS().Enable( demorganCond ).Check( demorganStandardCond ) );
    mgr->SetConditions( EE_ACTIONS::showDeMorganAlternate,
                        ACTION_CONDITIONS().Enable( demorganCond ).Check( demorganAlternateCond ) );

#undef CHECK
#undef ENABLE
}


void SYMBOL_VIEWER_FRAME::SetUnitAndConvert( int aUnit, int aConvert )
{
    m_unit = aUnit > 0 ? aUnit : 1;
    m_convert = aConvert > 0 ? aConvert : LIB_ITEM::LIB_CONVERT::BASE;
    m_selection_changed = false;

    updatePreviewSymbol();
}


LIB_PART* SYMBOL_VIEWER_FRAME::GetSelectedSymbol() const
{
    LIB_PART* symbol = nullptr;

    if( !m_libraryName.IsEmpty() && !m_entryName.IsEmpty() )
        symbol = Prj().SchSymbolLibTable()->LoadSymbol( m_libraryName, m_entryName );

    return symbol;
}


void SYMBOL_VIEWER_FRAME::updatePreviewSymbol()
{
    LIB_PART* symbol = GetSelectedSymbol();
    KIGFX::SCH_VIEW* view = GetCanvas()->GetView();

    if( m_previewItem )
    {
        view->Remove( m_previewItem );
        m_previewItem = nullptr;
    }

    ClearMsgPanel();

    if( symbol )
    {
        GetRenderSettings()->m_ShowUnit = m_unit;
        GetRenderSettings()->m_ShowConvert = m_convert;

        m_previewItem = symbol;
        view->Add( m_previewItem );

        wxString parentName;
        std::shared_ptr<LIB_PART> parent  = m_previewItem->GetParent().lock();

        if( parent )
            parentName = parent->GetName();

        AppendMsgPanel( _( "Name" ), m_previewItem->GetName() );
        AppendMsgPanel( _( "Parent" ),  parentName );
        AppendMsgPanel( _( "Description" ), m_previewItem->GetDescription() );
        AppendMsgPanel( _( "Keywords" ), m_previewItem->GetKeyWords() );
    }

    GetCanvas()->ForceRefresh();
}


bool SYMBOL_VIEWER_FRAME::ShowModal( wxString* aSymbol, wxWindow* aParent )
{
    if( aSymbol && !aSymbol->IsEmpty() )
    {
        wxString msg;
        LIB_TABLE* libTable = Prj().SchSymbolLibTable();
        LIB_ID libid;

        libid.Parse( *aSymbol, true );

        if( libid.IsValid() )
        {
            wxString libName = libid.GetLibNickname();

            if( !libTable->HasLibrary( libid.GetLibNickname(), false ) )
            {
                msg.Printf( _( "The current configuration does not include the library '%s'.\n"
                               "Use Manage Symbol Libraries to edit the configuration." ),
                             libName );
                DisplayErrorMessage( this, _( "Library not found in symbol library table." ), msg );
            }
            else if ( !libTable->HasLibrary( libid.GetLibNickname(), true ) )
            {
                msg.Printf( _( "The library '%s' is not enabled in the current configuration.\n"
                               "Use Manage Symbol Libraries to edit the configuration." ),
                             libName );
                DisplayErrorMessage( aParent, _( "Symbol library not enabled." ), msg );
            }
            else
            {
                SetSelectedLibrary( libid.GetLibNickname() );
                SetSelectedSymbol( libid.GetLibItemName());
            }
        }
    }

    return KIWAY_PLAYER::ShowModal( aSymbol, aParent );
}


void SYMBOL_VIEWER_FRAME::doCloseWindow()
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


void SYMBOL_VIEWER_FRAME::OnSize( wxSizeEvent& SizeEv )
{
    if( m_auimgr.GetManagedWindow() )
        m_auimgr.Update();

    SizeEv.Skip();
}


void SYMBOL_VIEWER_FRAME::onUpdateUnitChoice( wxUpdateUIEvent& aEvent )
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


bool SYMBOL_VIEWER_FRAME::ReCreateLibList()
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

    bool cmp_changed = ReCreateSymbolList();
    DisplayLibInfos();
    GetCanvas()->Refresh();

    return cmp_changed;
}


bool SYMBOL_VIEWER_FRAME::ReCreateSymbolList()
{
    if( m_symbolList == NULL )
        return false;

    wxArrayString aliasNames;

    try
    {
        Prj().SchSymbolLibTable()->EnumerateSymbolLib( m_libraryName, aliasNames,
                                                       m_listPowerCmpOnly );
    }
    catch( const IO_ERROR& ) {}   // ignore, it is handled below

    m_symbolList->Clear();

    if( aliasNames.IsEmpty() )
    {
        m_libraryName = wxEmptyString;
        m_entryName = wxEmptyString;
        m_convert = LIB_ITEM::LIB_CONVERT::BASE;
        m_unit    = 1;
        return true;
    }

    m_symbolList->Append( aliasNames );

    int  index = m_symbolList->FindString( m_entryName );
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

    m_symbolList->SetSelection( index, true );

    wxCommandEvent evt( wxEVT_COMMAND_LISTBOX_SELECTED, ID_LIBVIEW_CMP_LIST );
    ProcessEvent( evt );

    return changed;
}


void SYMBOL_VIEWER_FRAME::ClickOnLibList( wxCommandEvent& event )
{
    int ii = m_libList->GetSelection();

    if( ii < 0 )
        return;

    m_selection_changed = true;

    SetSelectedLibrary( m_libList->GetString( ii ) );
}


void SYMBOL_VIEWER_FRAME::SetSelectedLibrary( const wxString& aLibraryName )
{
    if( m_libraryName == aLibraryName )
        return;

    m_libraryName = aLibraryName;
    ReCreateSymbolList();
    GetCanvas()->Refresh();
    DisplayLibInfos();

    // Ensure the corresponding line in m_libList is selected
    // (which is not necessary the case if SetSelectedLibrary is called
    // by another caller than ClickOnLibList.
    m_libList->SetStringSelection( m_libraryName, true );

    // The m_libList has now the focus, in order to be able to use arrow keys
    // to navigate inside the list.
    // the gal canvas must not steal the focus to allow navigation
    GetCanvas()->SetStealsFocus( false );
    m_libList->SetFocus();
}


void SYMBOL_VIEWER_FRAME::ClickOnCmpList( wxCommandEvent& event )
{
    int ii = m_symbolList->GetSelection();

    if( ii < 0 )
        return;

    m_selection_changed = true;

    SetSelectedSymbol( m_symbolList->GetString( ii ));

    // The m_symbolList has now the focus, in order to be able to use arrow keys
    // to navigate inside the list.
    // the gal canvas must not steal the focus to allow navigation
    GetCanvas()->SetStealsFocus( false );
    m_symbolList->SetFocus();
}


void SYMBOL_VIEWER_FRAME::SetSelectedSymbol( const wxString& aSymbolName )
{
    if( m_entryName != aSymbolName )
    {
        m_entryName = aSymbolName;

        // Ensure the corresponding line in m_symbolList is selected
        // (which is not necessarily the case if SetSelectedSymbol is called
        // by another caller than ClickOnCmpList.
        m_symbolList->SetStringSelection( aSymbolName, true );
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


void SYMBOL_VIEWER_FRAME::DClickOnCmpList( wxCommandEvent& event )
{
    m_toolManager->RunAction( EE_ACTIONS::addSymbolToSchematic, true );
}


void SYMBOL_VIEWER_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    auto cfg = Pgm().GetSettingsManager().GetAppSettings<EESCHEMA_SETTINGS>();

    SCH_BASE_FRAME::LoadSettings( cfg );

    // Grid shape, etc.
    GetGalDisplayOptions().ReadWindowSettings( cfg->m_LibViewPanel.window );

    m_libListWidth = cfg->m_LibViewPanel.lib_list_width;
    m_symbolListWidth = cfg->m_LibViewPanel.cmp_list_width;

    GetRenderSettings()->m_ShowPinsElectricalType = cfg->m_LibViewPanel.show_pin_electrical_type;

    // Set parameters to a reasonable value.
    if( m_libListWidth > m_frameSize.x / 2 )
        m_libListWidth = m_frameSize.x / 2;

    if( m_symbolListWidth > m_frameSize.x / 2 )
        m_symbolListWidth = m_frameSize.x / 2;
}


void SYMBOL_VIEWER_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg)
{
    EESCHEMA_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<EESCHEMA_SETTINGS>();

    SCH_BASE_FRAME::SaveSettings( cfg );

    if( m_libListWidth && m_libList )
        m_libListWidth = m_libList->GetSize().x;

    m_symbolListWidth = m_symbolList->GetSize().x;

    cfg->m_LibViewPanel.lib_list_width = m_libListWidth;
    cfg->m_LibViewPanel.cmp_list_width = m_symbolListWidth;
    cfg->m_LibViewPanel.show_pin_electrical_type = GetRenderSettings()->m_ShowPinsElectricalType;
}


WINDOW_SETTINGS* SYMBOL_VIEWER_FRAME::GetWindowSettings( APP_SETTINGS_BASE* aCfg )
{
    auto cfg = dynamic_cast<EESCHEMA_SETTINGS*>( aCfg );
    wxASSERT( cfg );
    return &cfg->m_LibViewPanel.window;
}


void SYMBOL_VIEWER_FRAME::CommonSettingsChanged( bool aEnvVarsChanged, bool aTextVarsChanged )
{
    SCH_BASE_FRAME::CommonSettingsChanged( aEnvVarsChanged, aTextVarsChanged );

    GetCanvas()->GetGAL()->SetAxesColor( m_colorSettings->GetColor( LAYER_SCHEMATIC_GRID_AXES ) );
    GetCanvas()->GetGAL()->DrawGrid();

    if( aEnvVarsChanged )
        ReCreateLibList();
}


void SYMBOL_VIEWER_FRAME::OnActivate( wxActivateEvent& event )
{
    if( event.GetActive() )
    {
        bool changed = m_libList ? ReCreateLibList() : false;

        if (changed)
            m_selection_changed = true;

        updatePreviewSymbol();

        DisplayLibInfos();
    }

    event.Skip();    // required under wxMAC
}


void SYMBOL_VIEWER_FRAME::CloseLibraryViewer( wxCommandEvent& event )
{
    Close();
}


void SYMBOL_VIEWER_FRAME::SetFilter( const SCHLIB_FILTER* aFilter )
{
    m_listPowerCmpOnly = false;
    m_allowedLibs.Clear();

    if( aFilter )
    {
        m_allowedLibs = aFilter->GetAllowedLibList();
        m_listPowerCmpOnly = aFilter->GetFilterPowerParts();
    }

    ReCreateLibList();
}


const BOX2I SYMBOL_VIEWER_FRAME::GetDocumentExtents( bool aIncludeAllVisible ) const
{
    LIB_PART* part = GetSelectedSymbol();

    if( !part )
    {
        return BOX2I( VECTOR2I(-200, -200), VECTOR2I( 400, 400 ) );
    }
    else
    {
        std::shared_ptr< LIB_PART > tmp;

        tmp = ( part->IsAlias() ) ? part->GetParent().lock() : part->SharedPtr();

        wxCHECK( tmp, BOX2I( VECTOR2I(-200, -200), VECTOR2I( 400, 400 ) ) );

        EDA_RECT bbox = tmp->GetUnitBoundingBox( m_unit, m_convert );
        return BOX2I( bbox.GetOrigin(), VECTOR2I( bbox.GetWidth(), bbox.GetHeight() ) );
    }
}


void SYMBOL_VIEWER_FRAME::FinishModal()
{
    if( m_symbolList->GetSelection() >= 0 )
        DismissModal( true, m_libraryName + ':' + m_symbolList->GetStringSelection() );
    else
        DismissModal( false );

    Close( true );
}


void SYMBOL_VIEWER_FRAME::OnSelectSymbol( wxCommandEvent& aEvent )
{
    std::unique_lock<std::mutex> dialogLock( DIALOG_CHOOSE_SYMBOL::g_Mutex, std::defer_lock );

    // One CHOOSE_COMPONENT dialog at a time.  User probaby can't handle more anyway.
    if( !dialogLock.try_lock() )
        return;

    // Container doing search-as-you-type.
    SYMBOL_LIB_TABLE* libs = Prj().SchSymbolLibTable();
    wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER> adapter = SYMBOL_TREE_MODEL_ADAPTER::Create( this, libs );

    const auto libNicknames = libs->GetLogicalLibs();
    static_cast<SYMBOL_TREE_MODEL_ADAPTER*>( adapter.get() )->AddLibraries( libNicknames, this );

    LIB_PART* current = GetSelectedSymbol();
    LIB_ID id;
    int unit = 0;

    if( current )
    {
        id = current->GetLibId();
        adapter->SetPreselectNode( id, unit );
    }

    wxString dialogTitle;
    dialogTitle.Printf( _( "Choose Symbol (%d items loaded)" ), adapter->GetItemCount() );

    DIALOG_CHOOSE_SYMBOL dlg( this, dialogTitle, adapter, m_convert, false, false, false );

    if( dlg.ShowQuasiModal() == wxID_CANCEL )
        return;

    id = dlg.GetSelectedLibId( &unit );

    if( !id.IsValid() )
        return;

    SetSelectedLibrary( id.GetLibNickname() );
    SetSelectedSymbol( id.GetLibItemName() );
    SetUnitAndConvert( unit, 1 );
}


void SYMBOL_VIEWER_FRAME::onSelectNextSymbol( wxCommandEvent& aEvent )
{
    wxCommandEvent evt( wxEVT_COMMAND_LISTBOX_SELECTED, ID_LIBVIEW_CMP_LIST );
    int            ii = m_symbolList->GetSelection();

    // Select the next symbol or stop at the end of the list.
    if( ii != wxNOT_FOUND || ii != (int) m_symbolList->GetCount() - 1 )
        ii += 1;

    m_symbolList->SetSelection( ii );
    ProcessEvent( evt );
}


void SYMBOL_VIEWER_FRAME::onSelectPreviousSymbol( wxCommandEvent& aEvent )
{
    wxCommandEvent evt( wxEVT_COMMAND_LISTBOX_SELECTED, ID_LIBVIEW_CMP_LIST );
    int            ii = m_symbolList->GetSelection();

    // Select the previous symbol or stop at the beginning of list.
    if( ii != wxNOT_FOUND && ii != 0 )
        ii -= 1;

    m_symbolList->SetSelection( ii );
    ProcessEvent( evt );
}


void SYMBOL_VIEWER_FRAME::onSelectSymbolUnit( wxCommandEvent& aEvent )
{
    int ii = m_unitChoice->GetSelection();

    if( ii < 0 )
        return;

    m_unit = ii + 1;

    updatePreviewSymbol();
}


void SYMBOL_VIEWER_FRAME::DisplayLibInfos()
{
    if( m_libList && !m_libList->IsEmpty() && !m_libraryName.IsEmpty() )
    {
        const SYMBOL_LIB_TABLE_ROW* row = Prj().SchSymbolLibTable()->FindRow( m_libraryName, true );

        wxString title = wxString::Format( wxT( "%s \u2014 " ) + _( "Symbol Library Browser" ),
                                           row ? row->GetFullURI() : _( "no library selected" ) );
        SetTitle( title );
    }
}


SELECTION& SYMBOL_VIEWER_FRAME::GetCurrentSelection()
{
    return m_toolManager->GetTool<EE_SELECTION_TOOL>()->GetSelection();
}
