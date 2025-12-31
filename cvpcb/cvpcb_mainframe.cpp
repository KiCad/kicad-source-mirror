/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pgm_base.h>
#include <bitmaps.h>
#include <confirm.h>
#include <eda_dde.h>
#include <footprint_library_adapter.h>
#include <kiface_base.h>
#include <kiplatform/app.h>
#include <kiway_express.h>
#include <string_utils.h>
#include <project/project_file.h>
#include <netlist_reader/netlist_reader.h>
#include <lib_tree_model_adapter.h>
#include <numeric>
#include <richio.h>
#include <tool/action_manager.h>
#include <tool/action_toolbar.h>
#include <tool/common_control.h>
#include <tool/editor_conditions.h>
#include <tool/tool_dispatcher.h>
#include <tool/tool_manager.h>
#include <widgets/kistatusbar.h>
#include <widgets/wx_progress_reporters.h>

#include <cvpcb_association.h>
#include <cvpcb_id.h>
#include <cvpcb_mainframe.h>
#include <settings/settings_manager.h>
#include <settings/cvpcb_settings.h>
#include <display_footprints_frame.h>
#include <kiplatform/ui.h>
#include <listboxes.h>
#include <tools/cvpcb_actions.h>
#include <tools/cvpcb_association_tool.h>
#include <tools/cvpcb_control.h>
#include <project_pcb.h>
#include <toolbars_cvpcb.h>

#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/msgdlg.h>
#include <wx/ffile.h>


CVPCB_MAINFRAME::CVPCB_MAINFRAME( KIWAY* aKiway, wxWindow* aParent ) :
        KIWAY_PLAYER( aKiway, aParent, FRAME_CVPCB, _( "Assign Footprints" ), wxDefaultPosition,
                      wxDefaultSize, KICAD_DEFAULT_DRAWFRAME_STYLE, wxT( "CvpcbFrame" ),
                      unityScale ),
        m_footprintListBox( nullptr ),
        m_librariesListBox( nullptr ),
        m_symbolsListBox( nullptr ),
        m_tcFilterString( nullptr ),
        m_viewerPendingUpdate( false )
{
    m_modified            = false;
    m_cannotClose         = false;
    m_skipComponentSelect = false;
    m_filteringOptions    = FOOTPRINTS_LISTBOX::UNFILTERED_FP_LIST;
    m_FootprintsList      = FOOTPRINT_LIST::GetInstance( Kiway() );
    m_initialized         = false;
    m_aboutTitle          = _( "Assign Footprints" );

    // Give an icon
    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_cvpcb ) );
    SetIcon( icon );

    SetAutoLayout( true );

    LoadSettings( config() );

    setupTools();
    setupUIConditions();

    m_toolbarSettings = Pgm().GetSettingsManager().GetToolbarSettings<CVPCB_TOOLBAR_SETTINGS>( "cvpcb-toolbars" );
    configureToolbars();
    RecreateToolbars();
    ReCreateMenuBar();

    m_footprintListBox = new FOOTPRINTS_LISTBOX( this, ID_CVPCB_FOOTPRINT_LIST );
    m_footprintListBox->SetFont( KIUI::GetMonospacedUIFont() );

    m_symbolsListBox = new SYMBOLS_LISTBOX( this, ID_CVPCB_COMPONENT_LIST );
    m_symbolsListBox->SetFont( KIUI::GetMonospacedUIFont() );

    m_librariesListBox = new LIBRARY_LISTBOX( this, ID_CVPCB_LIBRARY_LIST );
    m_librariesListBox->SetFont( KIUI::GetMonospacedUIFont() );

    BuildFootprintsList();

    m_auimgr.SetManagedWindow( this );

    m_auimgr.AddPane( m_tbTopMain, EDA_PANE().HToolbar().Name( "TopMainToolbar" ).Top().Layer(6) );

    m_auimgr.AddPane( m_librariesListBox, EDA_PANE().Palette().Name( "Libraries" ).Left().Layer(1)
                      .Caption( _( "Footprint Libraries" ) )
                      .BestSize((int) ( m_frameSize.x * 0.20 ), m_frameSize.y ) );

    m_auimgr.AddPane( m_symbolsListBox, EDA_PANE().Palette().Name( "Symbols" ).Center().Layer(0)
                      .Caption( _( "Symbol : Footprint Assignments" ) ) );

    m_auimgr.AddPane( m_footprintListBox, EDA_PANE().Palette().Name( "Footprints" ).Right().Layer(1)
                      .Caption( _( "Filtered Footprints" ) )
                      .BestSize((int) ( m_frameSize.x * 0.30 ), m_frameSize.y ) );

    // Build the bottom panel, to display 2 status texts and the buttons:
    auto bottomPanel = new wxPanel( this );
    auto panelSizer = new wxBoxSizer( wxVERTICAL );

    wxFlexGridSizer* fgSizerStatus = new wxFlexGridSizer( 3, 1, 0, 0 );
    fgSizerStatus->SetFlexibleDirection( wxBOTH );
    fgSizerStatus->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    m_statusLine1 = new wxStaticText( bottomPanel, wxID_ANY, wxEmptyString );
    fgSizerStatus->Add( m_statusLine1, 0, 0, 5 );

    m_statusLine2 = new wxStaticText( bottomPanel, wxID_ANY, wxEmptyString );
    fgSizerStatus->Add( m_statusLine2, 0, 0, 5 );

    m_statusLine3 = new wxStaticText( bottomPanel, wxID_ANY, wxEmptyString );
    fgSizerStatus->Add( m_statusLine3, 0, 0, 5 );

    panelSizer->Add( fgSizerStatus, 1, wxEXPAND|wxLEFT, 2 );

    m_statusLine1->SetFont( KIUI::GetStatusFont( this ) );
    m_statusLine2->SetFont( KIUI::GetStatusFont( this ) );
    m_statusLine3->SetFont( KIUI::GetStatusFont( this ) );

    // Add buttons:
    auto buttonsSizer = new wxBoxSizer( wxHORIZONTAL );
    auto sdbSizer = new wxStdDialogButtonSizer();

    m_saveAndContinue = new wxButton( bottomPanel, wxID_ANY, _( "Apply, Save Schematic && Continue" ) );
    buttonsSizer->Add( m_saveAndContinue, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 20 );

    auto sdbSizerOK = new wxButton( bottomPanel, wxID_OK );
    sdbSizer->AddButton( sdbSizerOK );
    auto sdbSizerCancel = new wxButton( bottomPanel, wxID_CANCEL );
    sdbSizer->AddButton( sdbSizerCancel );
    sdbSizer->Realize();

    buttonsSizer->Add( sdbSizer, 0, 0, 5 );
    panelSizer->Add( buttonsSizer, 0, wxALIGN_RIGHT|wxALL, 5 );

    bottomPanel->SetSizer( panelSizer );
    bottomPanel->Fit();

    sdbSizerOK->SetDefault();
    KIPLATFORM::UI::FixupCancelButtonCmdKeyCollision( this );

    m_auimgr.AddPane( bottomPanel, EDA_PANE().HToolbar().Name( "Buttons" ).Bottom().Layer(6) );

    m_auimgr.Update();
    m_initialized = true;

    auto setPaneWidth =
            [this]( wxAuiPaneInfo& pane, int width )
            {
                // wxAUI hack: force width by setting MinSize() and then Fixed()
                // thanks to ZenJu http://trac.wxwidgets.org/ticket/13180
                pane.MinSize( width, -1 );
                pane.BestSize( width, -1 );
                pane.MaxSize( width, -1 );
                pane.Fixed();
                m_auimgr.Update();

                // now make it resizable again
                pane.MinSize( 20, -1 );
                pane.Resizable();
                m_auimgr.Update();
            };

    if( CVPCB_SETTINGS* cfg = dynamic_cast<CVPCB_SETTINGS*>( config() ) )
    {
        m_tcFilterString->ChangeValue( cfg->m_FilterString );

        if( cfg->m_LibrariesWidth > 0 )
            setPaneWidth( m_auimgr.GetPane( "Libraries" ), cfg->m_LibrariesWidth );

        if( cfg->m_FootprintsWidth > 0 )
            setPaneWidth( m_auimgr.GetPane( "Footprints" ), cfg->m_FootprintsWidth );
    }

    // Connect Events
    setupEventHandlers();

    // Toolbar events
    Bind( wxEVT_TEXT, &CVPCB_MAINFRAME::onTextFilterChanged, this );

    // Attach the events to the tool dispatcher
    Bind( wxEVT_CHAR, &TOOL_DISPATCHER::DispatchWxEvent, m_toolDispatcher );
    Bind( wxEVT_CHAR_HOOK, &TOOL_DISPATCHER::DispatchWxEvent, m_toolDispatcher );

    m_filterTimer = new wxTimer( this );
    Bind( wxEVT_TIMER, &CVPCB_MAINFRAME::onTextFilterChangedTimer, this, m_filterTimer->GetId() );

    // Start the main processing loop
    m_toolManager->InvokeTool( "cvpcb.Control" );

    m_filterTimer->StartOnce( 100 );

    KIPLATFORM::APP::SetShutdownBlockReason( this, _( "Symbol to footprint changes are unsaved" ) );
}


CVPCB_MAINFRAME::~CVPCB_MAINFRAME()
{
    Unbind( wxEVT_TEXT, &CVPCB_MAINFRAME::onTextFilterChanged, this );
    Unbind( wxEVT_CHAR, &TOOL_DISPATCHER::DispatchWxEvent, m_toolDispatcher );
    Unbind( wxEVT_CHAR_HOOK, &TOOL_DISPATCHER::DispatchWxEvent, m_toolDispatcher );

    Unbind( wxEVT_TIMER, &CVPCB_MAINFRAME::onTextFilterChangedTimer, this, m_filterTimer->GetId() );
    Unbind( wxEVT_IDLE, &CVPCB_MAINFRAME::updateFootprintViewerOnIdle, this );

    // Clean up the tool infrastructure
    delete m_actions;
    delete m_toolManager;
    delete m_toolDispatcher;

    m_auimgr.UnInit();
}


void CVPCB_MAINFRAME::setupTools()
{
    // Create the manager
    m_actions = new CVPCB_ACTIONS();
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( nullptr, nullptr, nullptr, config(), this );
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager );

    // Register tools
    m_toolManager->RegisterTool( new COMMON_CONTROL );
    m_toolManager->RegisterTool( new CVPCB_CONTROL );
    m_toolManager->RegisterTool( new CVPCB_ASSOCIATION_TOOL );
    m_toolManager->InitTools();

    CVPCB_CONTROL* tool = m_toolManager->GetTool<CVPCB_CONTROL>();

    // Even though these menus will open with the right-click, we treat them as a normal
    // menu instead of a context menu because we don't care about their position and want
    // to be able to tell the difference between a menu click and a hotkey activation.

    // Create the context menu for the symbols list box
    m_symbolsContextMenu = new ACTION_MENU( false, tool );
    m_symbolsContextMenu->Add( CVPCB_ACTIONS::showFootprintViewer );
    m_symbolsContextMenu->AppendSeparator();
    m_symbolsContextMenu->Add( ACTIONS::cut );
    m_symbolsContextMenu->Add( ACTIONS::copy );
    m_symbolsContextMenu->Add( ACTIONS::paste );
    m_symbolsContextMenu->AppendSeparator();
    m_symbolsContextMenu->Add( CVPCB_ACTIONS::deleteAssoc );

    // Create the context menu for the footprint list box
    m_footprintContextMenu = new ACTION_MENU( false, tool );
    m_footprintContextMenu->Add( CVPCB_ACTIONS::showFootprintViewer );
}


void CVPCB_MAINFRAME::setupUIConditions()
{
    EDA_BASE_FRAME::setupUIConditions();

    ACTION_MANAGER*   mgr = m_toolManager->GetActionManager();
    EDITOR_CONDITIONS cond( this );

    wxASSERT( mgr );

#define ENABLE( x ) ACTION_CONDITIONS().Enable( x )
#define CHECK( x )  ACTION_CONDITIONS().Check( x )

    mgr->SetConditions( CVPCB_ACTIONS::saveAssociationsToSchematic, ENABLE( cond.ContentModified() ) );
    mgr->SetConditions( CVPCB_ACTIONS::saveAssociationsToFile,      ENABLE( cond.ContentModified() ) );
    mgr->SetConditions( ACTIONS::undo,                              ENABLE( cond.UndoAvailable() ) );
    mgr->SetConditions( ACTIONS::redo,                              ENABLE( cond.RedoAvailable() ) );

    auto compFilter =
            [this] ( const SELECTION& )
            {
                return m_filteringOptions & FOOTPRINTS_LISTBOX::FILTERING_BY_COMPONENT_FP_FILTERS;
            };

    auto libFilter =
            [this] ( const SELECTION& )
            {
                return m_filteringOptions & FOOTPRINTS_LISTBOX::FILTERING_BY_LIBRARY;
            };

    auto pinFilter =
            [this] ( const SELECTION& )
            {
                return m_filteringOptions & FOOTPRINTS_LISTBOX::FILTERING_BY_PIN_COUNT;
            };

    mgr->SetConditions( CVPCB_ACTIONS::FilterFPbyFPFilters, CHECK( compFilter ) );
    mgr->SetConditions( CVPCB_ACTIONS::FilterFPbyLibrary,   CHECK( libFilter ) );
    mgr->SetConditions( CVPCB_ACTIONS::filterFPbyPin,       CHECK( pinFilter ) );

#undef CHECK
#undef ENABLE
}


void CVPCB_MAINFRAME::setupEventHandlers()
{
    // Connect the handlers to launch the context menus in the listboxes
    m_footprintListBox->Bind( wxEVT_RIGHT_DOWN,
            [this]( wxMouseEvent& )
            {
                PopupMenu( m_footprintContextMenu );
            } );

    m_symbolsListBox->Bind( wxEVT_RIGHT_DOWN,
            [this]( wxMouseEvent& )
            {
                PopupMenu( m_symbolsContextMenu );
            } );

    // Connect the handler for the save button
    m_saveAndContinue->Bind( wxEVT_COMMAND_BUTTON_CLICKED,
            [this]( wxCommandEvent& )
            {
                // saveAssociations must be run immediately
                GetToolManager()->RunAction( CVPCB_ACTIONS::saveAssociationsToFile );
            } );

    // Connect the handlers for the ok/cancel buttons
    Bind( wxEVT_BUTTON,
            [this]( wxCommandEvent& )
            {
                // saveAssociations must be run immediately, before running Close( true )
                GetToolManager()->RunAction( CVPCB_ACTIONS::saveAssociationsToSchematic );
                Close( true );
            }, wxID_OK );

    Bind( wxEVT_BUTTON,
            [this]( wxCommandEvent& )
            {
                Close( false );
            }, wxID_CANCEL );

    // Connect the handlers for the close events
    Bind( wxEVT_MENU,
            [this]( wxCommandEvent& )
            {
                Close( false );
            }, wxID_CLOSE );
    Bind( wxEVT_MENU,
            [this]( wxCommandEvent& )
            {
                Close( false );
            }, wxID_EXIT );

    // Just skip the resize events
    Bind( wxEVT_SIZE,
          []( wxSizeEvent& aEvent )
          {
              aEvent.Skip();
          } );
}


bool CVPCB_MAINFRAME::canCloseWindow( wxCloseEvent& aEvent )
{
    if( m_modified )
    {
        // Shutdown blocks must be determined and vetoed as early as possible
        if( KIPLATFORM::APP::SupportsShutdownBlockReason() && aEvent.GetId() == wxEVT_QUERY_END_SESSION )
            return false;

        if( !HandleUnsavedChanges( this, _( "Symbol to Footprint links have been modified. Save changes?" ),
                                   [&]() -> bool
                                   {
                                       return SaveFootprintAssociation( false );
                                   } ) )
        {
            return false;
        }
    }

    if( m_cannotClose )
        return false;

    return true;
}


void CVPCB_MAINFRAME::doCloseWindow()
{
    m_symbolsListBox->Shutdown();
    m_footprintListBox->Shutdown();

    if( GetFootprintViewerFrame() )
        GetFootprintViewerFrame()->Close( true );

    m_modified = false;

    // Stop the timer during destruction early to avoid potential race conditions (that do happen)
    m_filterTimer->Stop();

    // Shutdown all running tools
    if( m_toolManager )
        m_toolManager->ShutdownAllTools();

    // clear symbol selection in schematic:
    SendComponentSelectionToSch( true );
}


void CVPCB_MAINFRAME::onTextFilterChanged( wxCommandEvent& event )
{
    // Called when changing the filter string in main toolbar.
    // If the option FOOTPRINTS_LISTBOX::FILTERING_BY_TEXT_PATTERN is set, update the list
    // of available footprints which match the filter

    m_filterTimer->StartOnce( 200 );
}


void CVPCB_MAINFRAME::onTextFilterChangedTimer( wxTimerEvent& aEvent )
{
    // GTK loses the search-control's focus on a Refresh event, so we record the focus and
    // insertion point here and then restore them at the end.
    bool searchCtrlHasFocus = m_tcFilterString->HasFocus();
    long pos = m_tcFilterString->GetInsertionPoint();

    COMPONENT* symbol = GetSelectedComponent();
    wxString   libraryName = m_librariesListBox->GetSelectedLibrary();

    m_footprintListBox->SetFootprints( *m_FootprintsList, libraryName, symbol,
                                       m_tcFilterString->GetValue(), m_filteringOptions );

    if( symbol && symbol->GetFPID().IsValid() )
        m_footprintListBox->SetSelectedFootprint( symbol->GetFPID() );
    else if( m_footprintListBox->GetSelection() >= 0 )
        m_footprintListBox->SetSelection( m_footprintListBox->GetSelection(), false );

    RefreshFootprintViewer();

    DisplayStatus();

    if( searchCtrlHasFocus && !m_tcFilterString->HasFocus() )
    {
        m_tcFilterString->SetFocus();
        m_tcFilterString->SetInsertionPoint( pos );
    }
}


void CVPCB_MAINFRAME::OnSelectComponent( wxListEvent& event )
{
    if( m_skipComponentSelect )
        return;

    COMPONENT* symbol = GetSelectedComponent();
    wxString   libraryName = m_librariesListBox->GetSelectedLibrary();

    m_footprintListBox->SetFootprints( *m_FootprintsList, libraryName, symbol,
                                       m_tcFilterString->GetValue(), m_filteringOptions );

    if( symbol && symbol->GetFPID().IsValid() )
        m_footprintListBox->SetSelectedFootprint( symbol->GetFPID() );
    else if( m_footprintListBox->GetSelection() >= 0 )
        m_footprintListBox->SetSelection( m_footprintListBox->GetSelection(), false );

    RefreshFootprintViewer();

    refreshAfterSymbolSearch( symbol );
}


void CVPCB_MAINFRAME::RefreshFootprintViewer()
{
    if( GetFootprintViewerFrame() && !m_viewerPendingUpdate )
    {
        Bind( wxEVT_IDLE, &CVPCB_MAINFRAME::updateFootprintViewerOnIdle, this );
        m_viewerPendingUpdate = true;
    }
}


void CVPCB_MAINFRAME::updateFootprintViewerOnIdle( wxIdleEvent& aEvent )
{
    Unbind( wxEVT_IDLE, &CVPCB_MAINFRAME::updateFootprintViewerOnIdle, this );
    m_viewerPendingUpdate = false;

    // On some platforms (OSX) the focus is lost when the viewers (fp and 3D viewers)
    // are opened and refreshed when a new footprint is selected.
    // If the listbox has the focus before selecting a new footprint, it will be forced
    // after selection.
    bool footprintListHasFocus = m_footprintListBox->HasFocus();
    bool symbolListHasFocus = m_symbolsListBox->HasFocus();

    // If the footprint view window is displayed, update the footprint.
    if( GetFootprintViewerFrame() )
        GetToolManager()->RunAction( CVPCB_ACTIONS::showFootprintViewer );

    DisplayStatus();

    if( footprintListHasFocus )
        m_footprintListBox->SetFocus();
    else if( symbolListHasFocus )
        m_symbolsListBox->SetFocus();
}


void CVPCB_MAINFRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    EDA_BASE_FRAME::LoadSettings( aCfg );

    if( CVPCB_SETTINGS* cfg = dynamic_cast<CVPCB_SETTINGS*>( aCfg ) )
        m_filteringOptions = cfg->m_FilterFlags;
}


void CVPCB_MAINFRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    EDA_BASE_FRAME::SaveSettings( aCfg );

    if( CVPCB_SETTINGS* cfg = dynamic_cast<CVPCB_SETTINGS*>( aCfg ) )
    {
        cfg->m_FilterFlags = m_filteringOptions;
        cfg->m_FilterString = m_tcFilterString->GetValue();

        cfg->m_LibrariesWidth = m_librariesListBox->GetSize().x;
        cfg->m_FootprintsWidth = m_footprintListBox->GetSize().x;
    }
}


void CVPCB_MAINFRAME::UndoAssociation()
{
    if( m_undoList.size() == 0 )
        return;

    CVPCB_UNDO_REDO_ENTRIES redoEntries;
    CVPCB_UNDO_REDO_ENTRIES curEntry = m_undoList.back();
    m_undoList.pop_back();

    // Iterate over the entries to undo
    for( const auto& assoc : curEntry )
    {
        AssociateFootprint( assoc, true, false );
        redoEntries.emplace_back( assoc.Reverse() );
    }

    // Add the redo entries to the redo stack
    m_redoList.emplace_back( redoEntries );
}


void CVPCB_MAINFRAME::RedoAssociation()
{
    if( m_redoList.size() == 0 )
        return;

    CVPCB_UNDO_REDO_ENTRIES curEntry = m_redoList.back();
    m_redoList.pop_back();

    // Iterate over the entries to undo
    bool firstAssoc = true;

    for( const auto& assoc : curEntry )
    {
        AssociateFootprint( assoc, firstAssoc );
        firstAssoc = false;
    }
}


wxString CVPCB_MAINFRAME::formatSymbolDesc( int idx, const wxString& aReference,
                                            const wxString& aValue, const wxString& aFootprint )
{
    // Work around a bug in wxString::Format with double-byte chars (and double-quote, for some
    // reason).
    wxString desc = wxString::Format( wxT( "%3d " ), idx );

    for( int ii = aReference.Length(); ii < 8; ++ii )
        desc += wxS( " " );

    desc += aReference + wxT( " - " );

    for( int ii = aValue.Length(); ii < 16; ++ii )
        desc += wxS( " " );

    desc += aValue + wxT( " : " ) + aFootprint;

    return desc;
}


void CVPCB_MAINFRAME::AssociateFootprint( const CVPCB_ASSOCIATION& aAssociation,
                                          bool aNewEntry, bool aAddUndoItem )
{
    if( m_netlist.IsEmpty() )
        return;

    COMPONENT* symbol = m_netlist.GetComponent( aAssociation.GetComponentIndex() );

    if( symbol == nullptr )
        return;

    LIB_ID fpid    = aAssociation.GetNewFootprint();
    LIB_ID oldFpid = symbol->GetFPID();

    // Test for validity of the requested footprint
    if( !fpid.empty() && !fpid.IsValid() )
    {
        wxString msg = wxString::Format( _( "'%s' is not a valid footprint." ),
                                         fpid.Format().wx_str() );
        DisplayErrorMessage( this, msg );
        return;
    }

    const KIID& id = symbol->GetKIIDs().front();

    // Set new footprint to all instances of the selected symbol
    for( unsigned int idx : GetComponentIndices() )
    {
        COMPONENT* candidate = m_netlist.GetComponent( idx );
        const std::vector<KIID>& kiids = candidate->GetKIIDs();

        if( std::find( kiids.begin(), kiids.end(), id ) != kiids.end() )
        {
            // Set the new footprint
            candidate->SetFPID( fpid );

            // create the new symbol description and set it
            wxString description = formatSymbolDesc( idx + 1,
                                                     candidate->GetReference(),
                                                     candidate->GetValue(),
                                                     candidate->GetFPID().Format().wx_str() );
            m_symbolsListBox->SetString( idx, description );

            if( !m_FootprintsList->GetFootprintInfo( symbol->GetFPID().Format().wx_str() ) )
                m_symbolsListBox->AppendWarning( idx );
            else
                m_symbolsListBox->RemoveWarning( idx );
        }
    }

    // Mark the data as being modified
    m_modified = true;

    // Update the statusbar and refresh the list
    DisplayStatus();
    m_symbolsListBox->Refresh();

    if( !aAddUndoItem )
        return;

    // Update the undo list
    if ( aNewEntry )
    {
        // Create a new entry for this association
        CVPCB_UNDO_REDO_ENTRIES newEntry;
        newEntry.emplace_back( CVPCB_ASSOCIATION( aAssociation.GetComponentIndex(), oldFpid,
                                                  aAssociation.GetNewFootprint() ) );
        m_undoList.emplace_back( newEntry );

        // Clear the redo list
        m_redoList.clear();
    }
    else
    {
        m_undoList.back().emplace_back( CVPCB_ASSOCIATION( aAssociation.GetComponentIndex(),
                                                           oldFpid,
                                                           aAssociation.GetNewFootprint() ) );
    }

}


bool CVPCB_MAINFRAME::OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl )
{
    return true;
}


void CVPCB_MAINFRAME::refreshAfterSymbolSearch( COMPONENT* aSymbol )
{
    // Tell AuiMgr that objects are changed !
    if( m_auimgr.GetManagedWindow() )   // Be sure Aui Manager is initialized
        m_auimgr.Update();              // (could be not the case when starting CvPcb)

    if( aSymbol == nullptr )
    {
        DisplayStatus();
        return;
    }

    // Preview of the already assigned footprint.
    // Find the footprint that was already chosen for this aSymbol and select it,
    // but only if the selection is made from the aSymbol list or the library list.
    // If the selection is made from the footprint list, do not change the current
    // selected footprint.
    if( FindFocus() == m_symbolsListBox || FindFocus() == m_librariesListBox )
    {
        wxString footprintName = From_UTF8( aSymbol->GetFPID().Format().c_str() );

        m_footprintListBox->SetSelection( m_footprintListBox->GetSelection(), false );

        for( int ii = 0; ii < m_footprintListBox->GetCount(); ii++ )
        {
            wxString candidateName;
            wxString msg = m_footprintListBox->OnGetItemText( ii, 0 );
            msg.Trim( true );
            msg.Trim( false );
            candidateName = msg.AfterFirst( wxChar( ' ' ) );

            if( footprintName.Cmp( candidateName ) == 0 )
            {
                m_footprintListBox->SetSelection( ii, true );
                break;
            }
        }
    }

    SendComponentSelectionToSch();
    DisplayStatus();
}


void CVPCB_MAINFRAME::SetFootprintFilter( FOOTPRINTS_LISTBOX::FP_FILTER_T aFilter,
                                          CVPCB_MAINFRAME::CVPCB_FILTER_ACTION aAction )
{
    int option = aFilter;

    // Apply the filter accordingly
    switch( aAction )
    {
    case CVPCB_MAINFRAME::FILTER_DISABLE: m_filteringOptions &= ~option; break;
    case CVPCB_MAINFRAME::FILTER_ENABLE:  m_filteringOptions |= option;  break;
    case CVPCB_MAINFRAME::FILTER_TOGGLE:  m_filteringOptions ^= option;  break;
    }

    wxListEvent l_event;
    OnSelectComponent( l_event );
}


void CVPCB_MAINFRAME::DisplayStatus()
{
    if( !m_initialized )
        return;

    wxString   filters, msg;
    COMPONENT* symbol = GetSelectedComponent();

    if( ( m_filteringOptions & FOOTPRINTS_LISTBOX::FILTERING_BY_COMPONENT_FP_FILTERS ) )
    {
        msg.Empty();

        if( symbol )
        {
            for( unsigned ii = 0; ii < symbol->GetFootprintFilters().GetCount(); ii++ )
            {
                if( msg.IsEmpty() )
                    msg += symbol->GetFootprintFilters()[ii];
                else
                    msg += wxT( ", " ) + symbol->GetFootprintFilters()[ii];
            }
        }

        filters += _( "Keywords" );

        if( !msg.IsEmpty() )
            filters += wxString::Format( wxT( " (%s)" ), msg );
    }

    if( ( m_filteringOptions & FOOTPRINTS_LISTBOX::FILTERING_BY_PIN_COUNT ) )
    {
        msg.Empty();

        if( symbol )
        {
            int pc = symbol->GetPinCount();
            wxLogTrace( "CVPCB_PINCOUNT", wxT( "DisplayStatus: selected '%s' pinCount=%d" ),
                        symbol->GetReference(), pc );
            msg = wxString::Format( wxT( "%i" ), pc );
        }

        if( !filters.IsEmpty() )
            filters += wxT( ", " );

        filters += _( "Pin Count" );

        if( !msg.IsEmpty() )
            filters += wxString::Format( wxT( " (%s)" ), msg );
    }

    if( ( m_filteringOptions & FOOTPRINTS_LISTBOX::FILTERING_BY_LIBRARY ) )
    {
        msg = m_librariesListBox->GetSelectedLibrary();

        if( !filters.IsEmpty() )
            filters += wxT( ", " );

        filters += _( "Library" );

        if( !msg.IsEmpty() )
            filters += wxString::Format( wxT( " (%s)" ), msg );
    }

    wxString textFilter = m_tcFilterString->GetValue();

    if( !textFilter.IsEmpty() )
    {
        if( !filters.IsEmpty() )
            filters += wxT( ", " );

        filters += _( "Search Text" ) + wxString::Format( wxT( " (%s)" ), textFilter );
    }

    if( filters.IsEmpty() )
        msg = _( "No Filtering" );
    else
        msg.Printf( _( "Filtered by %s" ), filters );

    msg += wxString::Format( _( ": %i matching footprints" ), m_footprintListBox->GetCount() );

    SetStatusText( msg );


    msg.Empty();
    wxString footprintName = GetSelectedFootprint();

    FOOTPRINT_INFO* fp = m_FootprintsList->GetFootprintInfo( footprintName );

    if( fp )    // can be NULL if no netlist loaded
    {
        msg = wxString::Format( _( "Description: %s;  Keywords: %s" ),
                                fp->GetDesc(),
                                fp->GetKeywords() );
    }

    SetStatusText( msg, 1 );

    msg.Empty();
    wxString lib;

    // Choose the footprint to get the information on
    if( fp )
    {
        // Use the footprint in the footprint viewer
        lib = fp->GetLibNickname();
    }
    else if( GetFocusedControl() == CVPCB_MAINFRAME::CONTROL_COMPONENT )
    {
        // Use the footprint of the selected symbol
        if( symbol )
            lib = symbol->GetFPID().GetUniStringLibNickname();
    }
    else if( GetFocusedControl() == CVPCB_MAINFRAME::CONTROL_LIBRARY )
    {
        // Use the library that is selected
        lib = m_librariesListBox->GetSelectedLibrary();
    }

    // Extract the library information
    FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( &Prj() );

    if( std::optional<LIBRARY_TABLE_ROW*> optRow = adapter->GetRow( lib ); optRow )
        msg = wxString::Format( _( "Library location: %s" ), LIBRARY_MANAGER::GetFullURI( *optRow ) );

    SetStatusText( msg, 2 );
}


bool CVPCB_MAINFRAME::LoadFootprintFiles()
{
    FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( &Prj() );

    // Check if there are footprint libraries in the footprint library table.
    if( !adapter || !adapter->Rows().size() )
    {
        wxMessageBox( _( "No PCB footprint libraries are listed in the current footprint "
                         "library table." ), _( "Configuration Error" ), wxOK | wxICON_ERROR );
        return false;
    }

    WX_PROGRESS_REPORTER progressReporter( this, _( "Load Footprint Libraries" ), 1, PR_CAN_ABORT );

    m_FootprintsList->ReadFootprintFiles( adapter, nullptr, &progressReporter );

    if( m_FootprintsList->GetErrorCount() )
    {
        if( KISTATUSBAR* statusBar = dynamic_cast<KISTATUSBAR*>( GetStatusBar() ) )
            statusBar->SetLoadWarningMessages( m_FootprintsList->GetErrorMessages() );
    }

    return true;
}


void CVPCB_MAINFRAME::SendComponentSelectionToSch( bool aClearSelectionOnly )
{
    if( m_netlist.IsEmpty() )
        return;

    std::string command = "$SELECT: ";

    if( aClearSelectionOnly )
    {
        // Sending an empty list means clearing the selection.
        if( Kiface().IsSingle() )
            SendCommand( MSG_TO_SCH, command );
        else
            Kiway().ExpressMail( FRAME_SCH, MAIL_SELECTION, command, this );

        return;
    }

    int selection = m_symbolsListBox->GetSelection();

    if( selection < 0 ) // Nothing selected
        return;

    if( m_netlist.GetComponent( selection ) == nullptr )
        return;

    // Now select the corresponding symbol on the schematic:
    wxString ref = m_netlist.GetComponent( selection )->GetReference();

    // The prefix 0,F before the reference is for selecting the symbol
    // (one can select a pin with a different prefix)
    command += wxT( "0,F" ) + EscapeString( ref, CTX_IPC );

    if( Kiface().IsSingle() )
        SendCommand( MSG_TO_SCH, command );
    else
        Kiway().ExpressMail( FRAME_SCH, MAIL_SELECTION, command, this );
}


int CVPCB_MAINFRAME::readSchematicNetlist( const std::string& aNetlist )
{
    STRING_LINE_READER*  stringReader = new STRING_LINE_READER( aNetlist, "Eeschema via Kiway" );
    KICAD_NETLIST_READER netlistReader( stringReader, &m_netlist );

    m_netlist.Clear();

    // Trace basic payload characteristics to verify libparts are present and visible here
    wxLogTrace( "CVPCB_PINCOUNT",
                wxT( "readSchematicNetlist: payload size=%zu has_libparts=%d has_libpart=%d" ),
                aNetlist.size(), aNetlist.find( "(libparts" ) != std::string::npos,
                aNetlist.find( "(libpart" ) != std::string::npos );

    try
    {
        netlistReader.LoadNetlist();
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg = wxString::Format( _( "Error loading schematic.\n%s" ),
                                         ioe.What().GetData() );
        wxMessageBox( msg, _( "Load Error" ), wxOK | wxICON_ERROR );
        return 1;
    }

    // We also remove footprint name if it is "$noname" because this is a dummy name,
    // not the actual name of the footprint.
    for( unsigned ii = 0; ii < m_netlist.GetCount(); ii++ )
    {
        if( m_netlist.GetComponent( ii )->GetFPID().GetLibItemName() == std::string( "$noname" ) )
            m_netlist.GetComponent( ii )->SetFPID( LIB_ID() );
    }

    // Sort symbols by reference:
    m_netlist.SortByReference();

    return 0;
}


void CVPCB_MAINFRAME::BuildFootprintsList()
{
    m_footprintListBox->SetFootprints( *m_FootprintsList, wxEmptyString, nullptr, wxEmptyString,
                                       FOOTPRINTS_LISTBOX::UNFILTERED_FP_LIST );
    DisplayStatus();
}


void CVPCB_MAINFRAME::BuildLibrariesList()
{
    COMMON_SETTINGS*   cfg = Pgm().GetCommonSettings();
    PROJECT_FILE&      project = Kiway().Prj().GetProjectFile();
    FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( &Prj() );

    // Use same sorting algorithm as LIB_TREE_NODE::AssignIntrinsicRanks
    struct library_sort
    {
        bool operator()( const wxString& lhs, const wxString& rhs ) const
        {
            return StrNumCmp( lhs, rhs, true ) < 0;
        }
    };

    std::set<wxString, library_sort> pinnedMatches;
    std::set<wxString, library_sort> otherMatches;

    m_librariesListBox->ClearList();

    auto process =
            [&]( const wxString& aNickname )
            {
                if( alg::contains( project.m_PinnedFootprintLibs, aNickname )
                        || alg::contains( cfg->m_Session.pinned_fp_libs, aNickname ) )
                {
                    pinnedMatches.insert( aNickname );
                }
                else
                {
                    otherMatches.insert( aNickname );
                }
            };


    if( adapter )
    {
        std::vector<wxString> libNickNames = adapter->GetLibraryNames();

        for( const wxString& libNickName : libNickNames )
            process( libNickName );
    }

    for( const wxString& nickname : pinnedMatches )
        m_librariesListBox->AppendLine( LIB_TREE_MODEL_ADAPTER::GetPinningSymbol() + nickname );

    for( const wxString& nickname : otherMatches )
        m_librariesListBox->AppendLine( nickname );

    m_librariesListBox->Finish();
}


COMPONENT* CVPCB_MAINFRAME::GetSelectedComponent()
{
    int selection = m_symbolsListBox->GetSelection();

    if( selection >= 0 && selection < (int) m_netlist.GetCount() )
        return m_netlist.GetComponent( selection );

    return nullptr;
}


void CVPCB_MAINFRAME::SetSelectedComponent( int aIndex, bool aSkipUpdate )
{
    m_skipComponentSelect = aSkipUpdate;

    if( aIndex < 0 )
    {
        m_symbolsListBox->DeselectAll();
    }
    else if( aIndex < m_symbolsListBox->GetCount() )
    {
        m_symbolsListBox->DeselectAll();
        m_symbolsListBox->SetSelection( aIndex );
        SendComponentSelectionToSch();
    }

    m_skipComponentSelect = false;
}


std::vector<unsigned int>
CVPCB_MAINFRAME::GetComponentIndices( CVPCB_MAINFRAME::CRITERIA aCriteria )
{
    std::vector<unsigned int> idx;
    int                       lastIdx;

    // Make sure a netlist has been loaded and the box has contents
    if( m_netlist.IsEmpty() || m_symbolsListBox->GetCount() == 0 )
        return idx;

    switch( aCriteria )
    {
    case CVPCB_MAINFRAME::ALL_COMPONENTS:
        idx.resize( m_netlist.GetCount() );
        std::iota( idx.begin(), idx.end(), 0 );
        break;

    case CVPCB_MAINFRAME::SEL_COMPONENTS:
        // Check to see if anything is selected
        if( m_symbolsListBox->GetSelectedItemCount() < 1 )
            break;

        // Get the symbols
        lastIdx = m_symbolsListBox->GetFirstSelected();
        idx.emplace_back( lastIdx );

        lastIdx = m_symbolsListBox->GetNextSelected( lastIdx );
        while( lastIdx > 0 )
        {
            idx.emplace_back( lastIdx );
            lastIdx = m_symbolsListBox->GetNextSelected( lastIdx );
        }
        break;

    case CVPCB_MAINFRAME::NA_COMPONENTS:
        for( unsigned int i = 0; i < m_netlist.GetCount(); i++ )
        {
            if( m_netlist.GetComponent( i )->GetFPID().empty() )
                idx.emplace_back( i );
        }
        break;

    case CVPCB_MAINFRAME::ASSOC_COMPONENTS:
        for( unsigned int i = 0; i < m_netlist.GetCount(); i++ )
        {
            if( !m_netlist.GetComponent( i )->GetFPID().empty() )
                idx.emplace_back( i );
        }
        break;

    default:
        wxASSERT_MSG( false, "Invalid symbol selection criteria" );
    }

    return idx;
}


DISPLAY_FOOTPRINTS_FRAME* CVPCB_MAINFRAME::GetFootprintViewerFrame() const
{
    // returns the Footprint Viewer frame, if exists, or NULL
    wxWindow* window = wxWindow::FindWindowByName( FOOTPRINTVIEWER_FRAME_NAME );
    return dynamic_cast<DISPLAY_FOOTPRINTS_FRAME*>( window );
}


wxWindow* CVPCB_MAINFRAME::GetToolCanvas() const
{
    return GetFootprintViewerFrame();
}


CVPCB_MAINFRAME::CONTROL_TYPE CVPCB_MAINFRAME::GetFocusedControl() const
{
    if( m_librariesListBox->HasFocus() )      return CVPCB_MAINFRAME::CONTROL_LIBRARY;
    else if( m_symbolsListBox->HasFocus() )   return CVPCB_MAINFRAME::CONTROL_COMPONENT;
    else if( m_footprintListBox->HasFocus() ) return CVPCB_MAINFRAME::CONTROL_FOOTPRINT;
    else                                      return CVPCB_MAINFRAME::CONTROL_NONE;
}


void CVPCB_MAINFRAME::SetFocusedControl( CVPCB_MAINFRAME::CONTROL_TYPE aLB )
{
    switch( aLB )
    {
    case CVPCB_MAINFRAME::CONTROL_LIBRARY:   m_librariesListBox->SetFocus(); break;
    case CVPCB_MAINFRAME::CONTROL_COMPONENT: m_symbolsListBox->SetFocus();   break;
    case CVPCB_MAINFRAME::CONTROL_FOOTPRINT: m_footprintListBox->SetFocus(); break;
    default:                                                                 break;
    }
}


wxString CVPCB_MAINFRAME::GetSelectedFootprint()
{
    // returns the LIB_ID of the selected footprint in footprint listview
    // or a empty string
    return m_footprintListBox->GetSelectedFootprint();
}


void CVPCB_MAINFRAME::SetStatusText( const wxString& aText, int aNumber )
{
    switch( aNumber )
    {
    case 0:  m_statusLine1->SetLabel( aText );                 break;
    case 1:  m_statusLine2->SetLabel( aText );                 break;
    case 2:  m_statusLine3->SetLabel( aText );                 break;
    default: wxFAIL_MSG( wxT( "Invalid status row number" ) ); break;
    }
}


void CVPCB_MAINFRAME::ShowChangedLanguage()
{
    EDA_BASE_FRAME::ShowChangedLanguage();
    RecreateToolbars();
    DisplayStatus();
}


void CVPCB_MAINFRAME::KiwayMailIn( KIWAY_EXPRESS& mail )
{
    const std::string& payload = mail.GetPayload();

    switch( mail.Command() )
    {
    case MAIL_EESCHEMA_NETLIST:
        // Disable Close events during readNetListAndFpFiles() to avoid crash when updating
        // widgets:
        m_cannotClose = true;
        readNetListAndFpFiles( payload );
        m_cannotClose = false;

        /* @todo
        Go into SCH_EDIT_FRAME::OnOpenCvpcb( wxCommandEvent& event ) and trim GNL_ALL down.
        */
        break;

    case MAIL_RELOAD_LIB:
        m_cannotClose = true;
        LoadFootprintFiles();
        BuildFootprintsList();
        BuildLibrariesList();
        m_cannotClose = false;
        break;

    default:
        ;       // ignore most
    }
}
