/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <symbol_library_common.h>
#include <confirm.h>
#include <dialog_choose_symbol.h>
#include <dialogs/html_message_box.h>
#include <eeschema_id.h>
#include <eeschema_settings.h>
#include <kiface_base.h>
#include <kiway.h>
#include <kiway_express.h>
#include <locale_io.h>
#include <symbol_viewer_frame.h>
#include <widgets/msgpanel.h>
#include <widgets/wx_listbox.h>
#include <widgets/wx_aui_utils.h>
#include <widgets/wx_progress_reporters.h>
#include <sch_view.h>
#include <sch_painter.h>
#include <symbol_lib_table.h>
#include <symbol_tree_model_adapter.h>
#include <pgm_base.h>
#include <project/project_file.h>
#include <settings/settings_manager.h>
#include <symbol_async_loader.h>
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
#include <wx/srchctrl.h>

#include <default_values.h>
#include <string_utils.h>
#include "eda_pattern_match.h"

// Save previous symbol library viewer state.
LIB_ID SYMBOL_VIEWER_FRAME::m_currentSymbol;

int SYMBOL_VIEWER_FRAME::m_unit = 1;
int SYMBOL_VIEWER_FRAME::m_convert = 1;
bool SYMBOL_VIEWER_FRAME::m_show_progress = true;


BEGIN_EVENT_TABLE( SYMBOL_VIEWER_FRAME, SCH_BASE_FRAME )
    // Window events
    EVT_SIZE( SYMBOL_VIEWER_FRAME::OnSize )
    EVT_ACTIVATE( SYMBOL_VIEWER_FRAME::OnActivate )

    // Toolbar events
    EVT_TOOL( ID_LIBVIEW_SELECT_PART, SYMBOL_VIEWER_FRAME::OnSelectSymbol )
    EVT_TOOL( ID_LIBVIEW_NEXT, SYMBOL_VIEWER_FRAME::onSelectNextSymbol )
    EVT_TOOL( ID_LIBVIEW_PREVIOUS, SYMBOL_VIEWER_FRAME::onSelectPreviousSymbol )
    EVT_CHOICE( ID_LIBVIEW_SELECT_UNIT_NUMBER, SYMBOL_VIEWER_FRAME::onSelectSymbolUnit )

    // listbox events
    EVT_TEXT( ID_LIBVIEW_LIB_FILTER, SYMBOL_VIEWER_FRAME::OnLibFilter )
    EVT_LISTBOX( ID_LIBVIEW_LIB_LIST, SYMBOL_VIEWER_FRAME::ClickOnLibList )
    EVT_TEXT( ID_LIBVIEW_SYM_FILTER, SYMBOL_VIEWER_FRAME::OnSymFilter )
    EVT_LISTBOX( ID_LIBVIEW_SYM_LIST, SYMBOL_VIEWER_FRAME::ClickOnSymbolList )
    EVT_LISTBOX_DCLICK( ID_LIBVIEW_SYM_LIST, SYMBOL_VIEWER_FRAME::DClickOnSymbolList )

    // Menu (and/or hotkey) events
    EVT_MENU( wxID_CLOSE, SYMBOL_VIEWER_FRAME::CloseLibraryViewer )
    EVT_MENU( ID_GRID_SETTINGS, SCH_BASE_FRAME::OnGridSettings )

    EVT_UPDATE_UI( ID_LIBVIEW_SELECT_UNIT_NUMBER, SYMBOL_VIEWER_FRAME::onUpdateUnitChoice )

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
        m_symbolList( nullptr )
{
    wxASSERT( aFrameType == FRAME_SCH_VIEWER || aFrameType == FRAME_SCH_VIEWER_MODAL );

    if( aFrameType == FRAME_SCH_VIEWER_MODAL )
        SetModal( true );

    m_aboutTitle = _HKI( "KiCad Symbol Library Viewer" );

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
    m_listPowerOnly = false;

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

    GetRenderSettings()->SetDefaultPenWidth( DEFAULT_LINE_WIDTH_MILS * schIUScale.IU_PER_MILS );

    setupTools();
    setupUIConditions();

    ReCreateHToolbar();
    ReCreateVToolbar();
    ReCreateMenuBar();

    wxPanel* libPanel = new wxPanel( this );
    wxSizer* libSizer = new wxBoxSizer( wxVERTICAL );

    m_libFilter = new wxSearchCtrl( libPanel, ID_LIBVIEW_LIB_FILTER, wxEmptyString,
                                    wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    m_libFilter->SetDescriptiveText( _( "Filter" ) );
    libSizer->Add( m_libFilter, 0, wxEXPAND, 5 );

    m_libList = new WX_LISTBOX( libPanel, ID_LIBVIEW_LIB_LIST, wxDefaultPosition, wxDefaultSize,
                                0, nullptr, wxLB_HSCROLL | wxNO_BORDER );
    libSizer->Add( m_libList, 1, wxEXPAND, 5 );

    libPanel->SetSizer( libSizer );
    libPanel->Fit();

    wxPanel* symbolPanel = new wxPanel( this );
    wxSizer* symbolSizer = new wxBoxSizer( wxVERTICAL );

    m_symbolFilter = new wxSearchCtrl( symbolPanel, ID_LIBVIEW_SYM_FILTER, wxEmptyString,
                                       wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    m_symbolFilter->SetDescriptiveText( _( "Filter" ) );
    m_symbolFilter->SetToolTip(
            _( "Filter on symbol name, keywords, description and pin count.\n"
               "Search terms are separated by spaces.  All search terms must match.\n"
               "A term which is a number will also match against the pin count." ) );
    symbolSizer->Add( m_symbolFilter, 0, wxEXPAND, 5 );

#ifdef __WXGTK__
    // wxSearchCtrl vertical height is not calculated correctly on some GTK setups
    // See https://gitlab.com/kicad/code/kicad/-/issues/9019
    m_libFilter->SetMinSize( wxSize( -1, GetTextExtent( wxT( "qb" ) ).y + 10 ) );
    m_symbolFilter->SetMinSize( wxSize( -1, GetTextExtent( wxT( "qb" ) ).y + 10 ) );
#endif

    m_symbolList = new WX_LISTBOX( symbolPanel, ID_LIBVIEW_SYM_LIST, wxDefaultPosition, wxDefaultSize,
                                   0, nullptr, wxLB_HSCROLL | wxNO_BORDER );
    symbolSizer->Add( m_symbolList, 1, wxEXPAND, 5 );

    symbolPanel->SetSizer( symbolSizer );
    symbolPanel->Fit();

    // Preload libraries
    loadAllLibraries();

    if( aLibraryName.empty() )
    {
        ReCreateLibList();
    }
    else
    {
        m_currentSymbol.SetLibNickname( aLibraryName );
        m_currentSymbol.SetLibItemName( "" );
        m_unit = 1;
        m_convert = 1;
    }

    m_selection_changed = false;

    DisplayLibInfos();

    m_auimgr.SetManagedWindow( this );

    CreateInfoBar();

    // Manage main toolbar
    m_auimgr.AddPane( m_mainToolBar, EDA_PANE().HToolbar().Name( "MainToolbar" ).Top().Layer(6) );
    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( "MsgPanel" ) .Bottom().Layer(6) );

    m_auimgr.AddPane( libPanel, EDA_PANE().Palette().Name( "Libraries" ).Left().Layer(2)
                      .CaptionVisible( false ).MinSize( 100, -1 ).BestSize( 200, -1 ) );
    m_auimgr.AddPane( symbolPanel, EDA_PANE().Palette().Name( "Symbols" ).Left().Layer(1)
                      .CaptionVisible( false ).MinSize( 100, -1 ).BestSize( 300, -1 ) );

    m_auimgr.AddPane( GetCanvas(), EDA_PANE().Canvas().Name( "DrawFrame" ).Center() );

    m_auimgr.GetPane( libPanel ).Show( aLibraryName.empty() );

    m_auimgr.Update();

    if( m_libListWidth > 0 )
        SetAuiPaneSize( m_auimgr, m_auimgr.GetPane( "Libraries" ), m_libListWidth, -1 );

    if( m_symbolListWidth > 0 )
        SetAuiPaneSize( m_auimgr, m_auimgr.GetPane( "Symbols" ), m_symbolListWidth, -1 );

    FinishAUIInitialization();

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
    double max_size_x = schIUScale.mmToIU( 450 );
    double max_size_y = schIUScale.mmToIU( 450 );
    BOX2D bbox;
    bbox.SetOrigin( -max_size_x / 2, -max_size_y / 2 );
    bbox.SetSize( max_size_x, max_size_y );
    GetCanvas()->GetView()->SetBoundary( bbox );
    GetToolManager()->RunAction( ACTIONS::zoomFitScreen );

    // If a symbol was previously selected in m_symbolList from a previous run, show it
    wxString symbName = m_symbolList->GetStringSelection();

    if( !symbName.IsEmpty() )
    {
        SetSelectedSymbol( symbName );
        updatePreviewSymbol();
    }
}


SYMBOL_VIEWER_FRAME::~SYMBOL_VIEWER_FRAME()
{
    // Shutdown all running tools
    if( m_toolManager )
        m_toolManager->ShutdownAllTools();

    if( m_previewItem )
    {
        GetCanvas()->GetView()->Remove( m_previewItem.get() );
        m_previewItem = nullptr;
    }
}


void SYMBOL_VIEWER_FRAME::loadAllLibraries()
{
    // TODO: deduplicate with SYMBOL_TREE_MODEL_ADAPTER::AddLibraries
    std::vector<wxString> libraryNames = Prj().SchSymbolLibTable()->GetLogicalLibs();
    std::unique_ptr<WX_PROGRESS_REPORTER> progressReporter = nullptr;

    if( m_show_progress )
    {
        progressReporter = std::make_unique<WX_PROGRESS_REPORTER>( this,
                                                                   _( "Loading Symbol Libraries" ),
                                                                   libraryNames.size(), true );
    }

    // Disable KIID generation: not needed for library parts; sometimes very slow
    KIID::CreateNilUuids( true );

    std::unordered_map<wxString, std::vector<LIB_SYMBOL*>> loadedSymbols;

    SYMBOL_ASYNC_LOADER loader( libraryNames, Prj().SchSymbolLibTable(), false, nullptr,
                                progressReporter.get() );

    LOCALE_IO toggle;

    loader.Start();

    while( !loader.Done() )
    {
        if( progressReporter && !progressReporter->KeepRefreshing() )
            break;

        wxMilliSleep( 33 );
    }

    loader.Join();

    KIID::CreateNilUuids( false );

    if( !loader.GetErrors().IsEmpty() )
    {
        HTML_MESSAGE_BOX dlg( this, _( "Load Error" ) );

        dlg.MessageSet( _( "Errors loading symbols:" ) );

        wxString msg = loader.GetErrors();
        msg.Replace( "\n", "<BR>" );

        dlg.AddHTML_Text( msg );
        dlg.ShowModal();
    }
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
    m_toolManager->RegisterTool( new EE_INSPECTION_TOOL );     // manage show datasheet
    m_toolManager->RegisterTool( new EE_SELECTION_TOOL );      // manage context menu
    m_toolManager->RegisterTool( new SYMBOL_EDITOR_CONTROL );  // manage render settings

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
            [this]( const SELECTION& aSel )
            {
                return GetRenderSettings() && GetRenderSettings()->m_ShowPinsElectricalType;
            };

    auto pinNumbersShownCondition =
            [this]( const SELECTION& )
            {
                return GetRenderSettings() && GetRenderSettings()->m_ShowPinNumbers;
            };

    auto demorganCond =
            [this]( const SELECTION& )
            {
                LIB_SYMBOL* symbol = GetSelectedSymbol();
                return symbol && symbol->HasConversion();
            };

    auto demorganStandardCond =
            []( const SELECTION& )
            {
                return m_convert == LIB_ITEM::LIB_CONVERT::BASE;
            };

    auto demorganAlternateCond =
            []( const SELECTION& )
            {
                return m_convert == LIB_ITEM::LIB_CONVERT::DEMORGAN;
            };

    auto haveDatasheetCond =
            [this]( const SELECTION& )
            {
                LIB_SYMBOL* symbol = GetSelectedSymbol();
                return symbol && !symbol->GetDatasheetField().GetText().IsEmpty();
            };

    mgr->SetConditions( EE_ACTIONS::showDatasheet,       ENABLE( haveDatasheetCond ) );
    mgr->SetConditions( EE_ACTIONS::showElectricalTypes, CHECK( electricalTypesShownCondition ) );
    mgr->SetConditions( EE_ACTIONS::showPinNumbers,      CHECK( pinNumbersShownCondition ) );

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


LIB_SYMBOL* SYMBOL_VIEWER_FRAME::GetSelectedSymbol() const
{
    LIB_SYMBOL* symbol = nullptr;

    if( m_currentSymbol.IsValid() )
        symbol = Prj().SchSymbolLibTable()->LoadSymbol( m_currentSymbol );

    return symbol;
}


void SYMBOL_VIEWER_FRAME::updatePreviewSymbol()
{
    LIB_SYMBOL* symbol = GetSelectedSymbol();
    KIGFX::SCH_VIEW* view = GetCanvas()->GetView();

    if( m_previewItem )
    {
        view->Remove( m_previewItem.get() );
        m_previewItem = nullptr;
    }

    ClearMsgPanel();

    if( symbol )
    {
        GetRenderSettings()->m_ShowUnit = m_unit;
        GetRenderSettings()->m_ShowConvert = m_convert;

        m_previewItem = symbol->Flatten();
        view->Add( m_previewItem.get() );

        wxString parentName;
        std::shared_ptr<LIB_SYMBOL> parent  = symbol->GetParent().lock();

        if( parent )
            parentName = parent->GetName();

        AppendMsgPanel( _( "Name" ), UnescapeString( m_previewItem->GetName() ) );
        AppendMsgPanel( _( "Parent" ),  UnescapeString( parentName ) );
        AppendMsgPanel( _( "Description" ), m_previewItem->GetDescription() );
        AppendMsgPanel( _( "Keywords" ), m_previewItem->GetKeyWords() );
    }

    m_toolManager->RunAction( ACTIONS::zoomFitScreen );
    GetCanvas()->Refresh();
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
                             UnescapeString( libName ) );
                DisplayErrorMessage( this, _( "Library not found in symbol library table." ), msg );
            }
            else if ( !libTable->HasLibrary( libid.GetLibNickname(), true ) )
            {
                msg.Printf( _( "The library '%s' is not enabled in the current configuration.\n"
                               "Use Manage Symbol Libraries to edit the configuration." ),
                             UnescapeString( libName ) );
                DisplayErrorMessage( aParent, _( "Symbol library not enabled." ), msg );
            }
            else
            {
                SetSelectedLibrary( libid.GetLibNickname(), libid.GetSubLibraryName() );
                SetSelectedSymbol( libid.GetLibItemName() );
            }
        }
    }

    m_libFilter->SetFocus();
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
    LIB_SYMBOL* symbol = GetSelectedSymbol();

    int unit_count = 1;

    if( symbol )
        unit_count = std::max( symbol->GetUnitCount(), 1 );

    m_unitChoice->Enable( unit_count > 1 );

    if( unit_count > 1 )
    {
        // rebuild the unit list if it is not suitable (after a new selection for instance)
        if( unit_count != (int)m_unitChoice->GetCount() )
        {
            m_unitChoice->Clear();

            for( int ii = 0; ii < unit_count; ii++ )
            {
                wxString unit = symbol->GetUnitDisplayName( ii + 1 );
                m_unitChoice->Append( unit );
            }

        }

        if( m_unitChoice->GetSelection() != std::max( 0, m_unit - 1 ) )
            m_unitChoice->SetSelection( std::max( 0, m_unit - 1 ) );
    }
    else if( m_unitChoice->GetCount() )
    {
        m_unitChoice->Clear();
    }
}


bool SYMBOL_VIEWER_FRAME::ReCreateLibList()
{
    if( !m_libList )
        return false;

    m_libList->Clear();

    COMMON_SETTINGS*      cfg = Pgm().GetCommonSettings();
    PROJECT_FILE&         project = Kiway().Prj().GetProjectFile();
    SYMBOL_LIB_TABLE*     libTable = Prj().SchSymbolLibTable();
    std::vector<wxString> libs = libTable->GetLogicalLibs();
    std::vector<wxString> pinnedMatches;
    std::vector<wxString> otherMatches;

    auto doAddLib =
            [&]( const wxString& aLib )
            {
                if( alg::contains( project.m_PinnedSymbolLibs, aLib )
                    || alg::contains( cfg->m_Session.pinned_symbol_libs, aLib ) )
                {
                    pinnedMatches.push_back( aLib );
                }
                else
                {
                    otherMatches.push_back( aLib );
                }
            };

    auto process =
            [&]( const wxString& aLib )
            {
                // Remove not allowed libs, if the allowed lib list is not empty
                if( m_allowedLibs.GetCount() )
                {
                    if( m_allowedLibs.Index( aLib ) == wxNOT_FOUND )
                        return;
                }

                // Remove libs which have no power symbols, if this filter is activated
                if( m_listPowerOnly )
                {
                    wxArrayString aliasNames;

                    Prj().SchSymbolLibTable()->EnumerateSymbolLib( aLib, aliasNames, true );

                    if( aliasNames.IsEmpty() )
                        return;
                }

                SYMBOL_LIB_TABLE_ROW* row = libTable->FindRow( aLib );

                wxCHECK( row, /* void */ );

                if( !row->GetIsVisible() )
                    return;

                if( row->SupportsSubLibraries() )
                {
                    std::vector<wxString> subLibraries;
                    row->GetSubLibraryNames( subLibraries );

                    for( const wxString& lib : subLibraries )
                    {
                        wxString suffix = lib.IsEmpty() ? wxString( wxT( "" ) )
                                                        : wxString::Format( wxT( " - %s" ), lib );
                        wxString name = wxString::Format( wxT( "%s%s" ), aLib, suffix );

                        doAddLib( name );
                    }
                }
                else
                {
                    doAddLib( aLib );
                }
            };

    if( m_libFilter->GetValue().IsEmpty() )
    {
        for( const wxString& lib : libs )
            process( lib );
    }
    else
    {
        wxStringTokenizer tokenizer( m_libFilter->GetValue() );

        while( tokenizer.HasMoreTokens() )
        {
            const wxString       term = tokenizer.GetNextToken().Lower();
            EDA_COMBINED_MATCHER matcher( term, CTX_LIBITEM );

            for( const wxString& lib : libs )
            {
                if( matcher.Find( lib.Lower() ) )
                    process( lib );
            }
        }
    }

    if( libs.empty() )
        return true;

    for( const wxString& name : pinnedMatches )
        m_libList->Append( LIB_TREE_MODEL_ADAPTER::GetPinningSymbol() + UnescapeString( name ) );

    for( const wxString& name : otherMatches )
        m_libList->Append( UnescapeString( name ) );

    // Search for a previous selection:
    int index = m_libList->FindString( UnescapeString( m_currentSymbol.GetUniStringLibNickname() ) );

    if( index != wxNOT_FOUND )
    {
        m_libList->SetSelection( index, true );
    }
    else
    {
        // If not found, clear current library selection because it can be deleted after a
        // config change.
        m_currentSymbol.SetLibNickname( m_libList->GetCount() > 0
                                        ? m_libList->GetBaseString( 0 ) : wxString( wxT( "" ) ) );
        m_currentSymbol.SetLibItemName( wxEmptyString );
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
    if( m_symbolList == nullptr )
        return false;

    m_symbolList->Clear();

    wxString libName = m_currentSymbol.GetUniStringLibNickname();

    if( libName.IsEmpty() )
        return false;

    std::vector<LIB_SYMBOL*> symbols;
    SYMBOL_LIB_TABLE_ROW* row = Prj().SchSymbolLibTable()->FindRow( libName );

    try
    {
        if( row )
            Prj().SchSymbolLibTable()->LoadSymbolLib( symbols, libName, m_listPowerOnly );
    }
    catch( const IO_ERROR& ) {}   // ignore, it is handled below

    std::set<wxString> excludes;

    if( !m_symbolFilter->GetValue().IsEmpty() )
    {
        wxStringTokenizer tokenizer( m_symbolFilter->GetValue() );

        while( tokenizer.HasMoreTokens() )
        {
            const wxString       filterTerm = tokenizer.GetNextToken().Lower();
            EDA_COMBINED_MATCHER matcher( filterTerm, CTX_LIBITEM );

            for( LIB_SYMBOL* symbol : symbols )
            {
                std::vector<SEARCH_TERM> searchTerms = symbol->GetSearchTerms();
                int                      matched = matcher.ScoreTerms( searchTerms );

                if( filterTerm.IsNumber() && wxAtoi( filterTerm ) == (int)symbol->GetPinCount() )
                    matched++;

                if( !matched )
                    excludes.insert( symbol->GetName() );
            }
        }
    }

    wxString subLib = m_currentSymbol.GetSubLibraryName();

    for( const LIB_SYMBOL* symbol : symbols )
    {
        if( row && row->SupportsSubLibraries()
            && !subLib.IsSameAs( symbol->GetLibId().GetSubLibraryName() ) )
        {
            continue;
        }

        if( !excludes.count( symbol->GetName() ) )
            m_symbolList->Append( UnescapeString( symbol->GetName() ) );
    }

    if( m_symbolList->IsEmpty() )
    {
        SetSelectedSymbol( wxEmptyString );
        m_convert = LIB_ITEM::LIB_CONVERT::BASE;
        m_unit    = 1;
        return true;
    }

    int  index = m_symbolList->FindString( UnescapeString( m_currentSymbol.GetUniStringLibItemName() ) );
    bool changed = false;

    if( index == wxNOT_FOUND )
    {
        // Select the first library entry when the previous entry name does not exist in
        // the current library.
        m_convert   = LIB_ITEM::LIB_CONVERT::BASE;
        m_unit      = 1;
        index       = -1;
        changed     = true;
        SetSelectedSymbol( wxEmptyString );
    }

    m_symbolList->SetSelection( index, true );

    return changed;
}


void SYMBOL_VIEWER_FRAME::ClickOnLibList( wxCommandEvent& event )
{
    int ii = m_libList->GetSelection();

    if( ii < 0 )
        return;

    m_selection_changed = true;

    wxString selection = EscapeString( m_libList->GetBaseString( ii ), CTX_LIBID );

    if( !Prj().SchSymbolLibTable()->FindRow( selection )
        && selection.Find( '-' ) != wxNOT_FOUND )
    {
        // Probably a sub-library
        wxString sublib;
        selection = selection.BeforeLast( '-', &sublib ).Trim();
        sublib.Trim( false );
        SetSelectedLibrary( selection, sublib );
    }
    else
    {
        SetSelectedLibrary( selection );
    }
}


void SYMBOL_VIEWER_FRAME::SetSelectedLibrary( const wxString& aLibraryName,
                                              const wxString& aSubLibName )
{
    if( m_currentSymbol.GetUniStringLibNickname() == aLibraryName
        && wxString( m_currentSymbol.GetSubLibraryName().wx_str() ) == aSubLibName )
        return;

    m_currentSymbol.SetLibNickname( aLibraryName );
    m_currentSymbol.SetSubLibraryName( aSubLibName );
    ReCreateSymbolList();
    GetCanvas()->Refresh();
    DisplayLibInfos();

    // Ensure the corresponding line in m_libList is selected
    // (which is not necessary the case if SetSelectedLibrary is called
    // by another caller than ClickOnLibList.
    m_libList->SetStringSelection( UnescapeString( m_currentSymbol.GetFullLibraryName() ), true );

    // The m_libList has now the focus, in order to be able to use arrow keys
    // to navigate inside the list.
    // the gal canvas must not steal the focus to allow navigation
    GetCanvas()->SetStealsFocus( false );
    m_libList->SetFocus();
}


void SYMBOL_VIEWER_FRAME::ClickOnSymbolList( wxCommandEvent& event )
{
    int ii = m_symbolList->GetSelection();

    if( ii < 0 )
        return;

    m_selection_changed = true;

    SetSelectedSymbol( EscapeString( m_symbolList->GetBaseString( ii ), CTX_LIBID ) );

    // The m_symbolList has now the focus, in order to be able to use arrow keys
    // to navigate inside the list.
    // the gal canvas must not steal the focus to allow navigation
    GetCanvas()->SetStealsFocus( false );
    m_symbolList->SetFocus();
}


void SYMBOL_VIEWER_FRAME::SetSelectedSymbol( const wxString& aSymbolName )
{
    if( m_currentSymbol.GetUniStringLibItemName() != aSymbolName )
    {
        m_currentSymbol.SetLibItemName( aSymbolName );

        // Ensure the corresponding line in m_symbolList is selected
        // (which is not necessarily the case if SetSelectedSymbol is called
        // by another caller than ClickOnSymbolList.
        m_symbolList->SetStringSelection( UnescapeString( aSymbolName ), true );
        DisplayLibInfos();

        if( m_selection_changed )
        {
            m_unit = 1;
            m_convert = LIB_ITEM::LIB_CONVERT::BASE;
            m_selection_changed = false;
        }

        updatePreviewSymbol();
    }
}


void SYMBOL_VIEWER_FRAME::DClickOnSymbolList( wxCommandEvent& event )
{
    m_toolManager->RunAction( EE_ACTIONS::addSymbolToSchematic );
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
    GetRenderSettings()->m_ShowPinNumbers = cfg->m_LibViewPanel.show_pin_numbers;

    // Set parameters to a reasonable value.
    int maxWidth = cfg->m_LibViewPanel.window.state.size_x - 80;

    if( m_libListWidth + m_symbolListWidth > maxWidth )
    {
        m_libListWidth = maxWidth * ( m_libListWidth / ( m_libListWidth + m_symbolListWidth ) );
        m_symbolListWidth = maxWidth - m_libListWidth;
    }
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

    if( KIGFX::SCH_RENDER_SETTINGS* renderSettings = GetRenderSettings() )
    {
        cfg->m_LibViewPanel.show_pin_electrical_type = renderSettings->m_ShowPinsElectricalType;
        cfg->m_LibViewPanel.show_pin_numbers = renderSettings->m_ShowPinNumbers;
    }
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

    auto cfg = Pgm().GetSettingsManager().GetAppSettings<EESCHEMA_SETTINGS>();
    GetGalDisplayOptions().ReadWindowSettings( cfg->m_LibViewPanel.window );

    GetCanvas()->GetGAL()->SetAxesColor( m_colorSettings->GetColor( LAYER_SCHEMATIC_GRID_AXES ) );
    GetCanvas()->GetGAL()->DrawGrid();
    GetCanvas()->ForceRefresh();

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


void SYMBOL_VIEWER_FRAME::SetFilter( const SYMBOL_LIBRARY_FILTER* aFilter )
{
    m_listPowerOnly = false;
    m_allowedLibs.Clear();

    if( aFilter )
    {
        m_allowedLibs = aFilter->GetAllowedLibList();
        m_listPowerOnly = aFilter->GetFilterPowerSymbols();
    }

    ReCreateLibList();
}


const BOX2I SYMBOL_VIEWER_FRAME::GetDocumentExtents( bool aIncludeAllVisible ) const
{
    LIB_SYMBOL* symbol = GetSelectedSymbol();

    if( !symbol )
    {
        return BOX2I( VECTOR2I( -200, -200 ), VECTOR2I( 400, 400 ) );
    }
    else
    {
        std::shared_ptr<LIB_SYMBOL> tmp = symbol->IsAlias() ? symbol->GetParent().lock()
                                                            : symbol->SharedPtr();

        wxCHECK( tmp, BOX2I( VECTOR2I( -200, -200 ), VECTOR2I( 400, 400 ) ) );

        return tmp->GetUnitBoundingBox( m_unit, m_convert );
    }
}


void SYMBOL_VIEWER_FRAME::FinishModal()
{
    if( m_symbolList->GetSelection() >= 0 )
    {
        DismissModal( true, m_currentSymbol.Format() );
    }
    else
    {
        DismissModal( false );
    }

    Close( true );
}


void SYMBOL_VIEWER_FRAME::OnSelectSymbol( wxCommandEvent& aEvent )
{
    std::unique_lock<std::mutex> dialogLock( DIALOG_CHOOSE_SYMBOL::g_Mutex, std::defer_lock );

    // One CHOOSE_SYMBOL dialog at a time.  User probably can't handle more anyway.
    if( !dialogLock.try_lock() )
        return;

    // Container doing search-as-you-type.
    SYMBOL_LIB_TABLE* libs = Prj().SchSymbolLibTable();
    wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER> dataPtr
                                    = SYMBOL_TREE_MODEL_ADAPTER::Create( this, libs );
    SYMBOL_TREE_MODEL_ADAPTER* modelAdapter
                                    = static_cast<SYMBOL_TREE_MODEL_ADAPTER*>( dataPtr.get() );

    if( !modelAdapter->AddLibraries( libs->GetLogicalLibs(), this ) )
    {
        // loading cancelled by user
        return;
    }

    LIB_SYMBOL* current = GetSelectedSymbol();
    LIB_ID id;
    int unit = 0;

    if( current )
    {
        id = current->GetLibId();
        modelAdapter->SetPreselectNode( id, unit );
    }

    wxString dialogTitle;
    dialogTitle.Printf( _( "Choose Symbol (%d items loaded)" ), modelAdapter->GetItemCount() );

    DIALOG_CHOOSE_SYMBOL dlg( this, dialogTitle, dataPtr, m_convert, false, false, false );

    if( dlg.ShowQuasiModal() == wxID_CANCEL )
        return;

    // Save any changes to column widths, etc.
    modelAdapter->SaveSettings();

    id = dlg.GetSelectedLibId( &unit );

    if( !id.IsValid() )
        return;

    SetSelectedLibrary( id.GetLibNickname(), id.GetSubLibraryName() );
    SetSelectedSymbol( id.GetLibItemName() );
    SetUnitAndConvert( unit, 1 );
}


void SYMBOL_VIEWER_FRAME::OnLibFilter( wxCommandEvent& aEvent )
{
    ReCreateLibList();

    // Required to avoid interaction with SetHint()
    // See documentation for wxTextEntry::SetHint
    aEvent.Skip();
}


void SYMBOL_VIEWER_FRAME::OnSymFilter( wxCommandEvent& aEvent )
{
    ReCreateSymbolList();

    // Required to avoid interaction with SetHint()
    // See documentation for wxTextEntry::SetHint
    aEvent.Skip();
}


void SYMBOL_VIEWER_FRAME::OnCharHook( wxKeyEvent& aEvent )
{
    if( aEvent.GetKeyCode() == WXK_UP )
    {
        if( m_libFilter->HasFocus() || m_libList->HasFocus() )
        {
            int prev = m_libList->GetSelection() - 1;

            if( prev >= 0 )
            {
                m_libList->SetSelection( prev );
                m_libList->EnsureVisible( prev );

                wxCommandEvent dummy;
                ClickOnLibList( dummy );
            }
        }
        else
        {
            wxCommandEvent dummy;
            onSelectPreviousSymbol( dummy );
        }
    }
    else if( aEvent.GetKeyCode() == WXK_DOWN )
    {
        if( m_libFilter->HasFocus() || m_libList->HasFocus() )
        {
            int next = m_libList->GetSelection() + 1;

            if( next < (int)m_libList->GetCount() )
            {
                m_libList->SetSelection( next );
                m_libList->EnsureVisible( next );

                wxCommandEvent dummy;
                ClickOnLibList( dummy );
            }
        }
        else
        {
            wxCommandEvent dummy;
            onSelectNextSymbol( dummy );
        }
    }
    else if( aEvent.GetKeyCode() == WXK_TAB && m_libFilter->HasFocus() )
    {
        if( !aEvent.ShiftDown() )
            m_symbolFilter->SetFocus();
        else
            aEvent.Skip();
    }
    else if( aEvent.GetKeyCode() == WXK_TAB && m_symbolFilter->HasFocus() )
    {
        if( aEvent.ShiftDown() )
            m_libFilter->SetFocus();
        else
            aEvent.Skip();
    }
    else if( ( aEvent.GetKeyCode() == WXK_RETURN || aEvent.GetKeyCode() == WXK_NUMPAD_ENTER )
             && m_symbolList->GetSelection() >= 0 )
    {
        wxCommandEvent dummy;
        DClickOnSymbolList( dummy );
    }
    else
    {
        aEvent.Skip();
    }
}


void SYMBOL_VIEWER_FRAME::onSelectNextSymbol( wxCommandEvent& aEvent )
{
    wxCommandEvent evt( wxEVT_COMMAND_LISTBOX_SELECTED, ID_LIBVIEW_SYM_LIST );
    int            ii = m_symbolList->GetSelection();

    // Select the next symbol or stop at the end of the list.
    if( ii != wxNOT_FOUND || ii != (int) m_symbolList->GetCount() - 1 )
        ii += 1;

    m_symbolList->SetSelection( ii );
    ProcessEvent( evt );
}


void SYMBOL_VIEWER_FRAME::onSelectPreviousSymbol( wxCommandEvent& aEvent )
{
    wxCommandEvent evt( wxEVT_COMMAND_LISTBOX_SELECTED, ID_LIBVIEW_SYM_LIST );
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
    wxString libName = m_currentSymbol.GetUniStringLibNickname();

    if( m_libList && !m_libList->IsEmpty() && !libName.IsEmpty() )
    {
        const SYMBOL_LIB_TABLE_ROW* row =
                Prj().SchSymbolLibTable()->FindRow( libName, true );

        wxString title = row ? row->GetFullURI( true ) : _( "[no library selected]" );

        title += wxT( " \u2014 " ) + _( "Symbol Library Browser" );
        SetTitle( title );
    }
}


SELECTION& SYMBOL_VIEWER_FRAME::GetCurrentSelection()
{
    return m_toolManager->GetTool<EE_SELECTION_TOOL>()->GetSelection();
}


void SYMBOL_VIEWER_FRAME::KiwayMailIn( KIWAY_EXPRESS& mail )
{

    switch( mail.Command() )
    {
    case MAIL_RELOAD_LIB:
    {
        ReCreateLibList();
        break;
    }
    case MAIL_REFRESH_SYMBOL:
    {
        SYMBOL_LIB_TABLE* tbl = Prj().SchSymbolLibTable();
        LIB_SYMBOL* symbol = GetSelectedSymbol();

        wxCHECK2( tbl && symbol, break );

        const SYMBOL_LIB_TABLE_ROW* row = tbl->FindRow( symbol->GetLibId().GetLibNickname() );

        if( !row )
            return;

        wxString libfullname = row->GetFullURI( true );

        wxString lib( mail.GetPayload() );
        wxLogTrace( "KICAD_LIB_WATCH", "Received refresh symbol request for %s, current symbols "
                    "is %s", lib, libfullname );

        if( lib == libfullname )
        {
            wxLogTrace( "KICAD_LIB_WATCH", "Refreshing symbol %s", symbol->GetName() );
            updatePreviewSymbol();
            GetCanvas()->GetView()->UpdateAllItems( KIGFX::ALL );
        }

        break;
    }
    default:;
    }
}
