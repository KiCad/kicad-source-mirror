/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <confirm.h>
#include <eda_dde.h>
#include <fp_lib_table.h>
#include <kiface_i.h>
#include <kiplatform/app.h>
#include <kiway_express.h>
#include <macros.h>
#include <netlist_reader/netlist_reader.h>
#include <numeric>
#include <tool/action_manager.h>
#include <tool/action_toolbar.h>
#include <tool/common_control.h>
#include <tool/editor_conditions.h>
#include <tool/tool_dispatcher.h>
#include <tool/tool_manager.h>
#include <widgets/progress_reporter.h>
#include <wx/statline.h>

#include <cvpcb_association.h>
#include <cvpcb_id.h>
#include <cvpcb_mainframe.h>
#include <cvpcb_settings.h>
#include <display_footprints_frame.h>
#include <kiplatform/ui.h>
#include <listboxes.h>
#include <tools/cvpcb_actions.h>
#include <tools/cvpcb_association_tool.h>
#include <tools/cvpcb_control.h>

#define CVPCB_MAINFRAME_NAME wxT( "CvpcbFrame" )


CVPCB_MAINFRAME::CVPCB_MAINFRAME( KIWAY* aKiway, wxWindow* aParent ) :
    KIWAY_PLAYER( aKiway, aParent, FRAME_CVPCB, _( "Assign Footprints" ), wxDefaultPosition,
                  wxDefaultSize, KICAD_DEFAULT_DRAWFRAME_STYLE, CVPCB_MAINFRAME_NAME )
{
    m_compListBox         = NULL;
    m_footprintListBox    = NULL;
    m_libListBox          = NULL;
    m_mainToolBar         = NULL;
    m_modified            = false;
    m_skipComponentSelect = false;
    m_filteringOptions    = FOOTPRINTS_LISTBOX::UNFILTERED_FP_LIST;
    m_tcFilterString      = NULL;
    m_FootprintsList      = FOOTPRINT_LIST::GetInstance( Kiway() );
    m_initialized         = false;
    m_aboutTitle          = "CvPcb";

    // Give an icon
    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_cvpcb ) );
    SetIcon( icon );

    SetAutoLayout( true );

    LoadSettings( config() );

    setupTools();
    setupUIConditions();
    ReCreateMenuBar();
    ReCreateHToolbar();

    // Create list of available footprints and components of the schematic
    BuildCmpListBox();
    BuildFOOTPRINTS_LISTBOX();
    BuildLIBRARY_LISTBOX();

    m_auimgr.SetManagedWindow( this );

    m_auimgr.AddPane( m_mainToolBar, EDA_PANE().HToolbar().Name( "MainToolbar" ).Top().Layer(6) );

    m_auimgr.AddPane( m_libListBox, EDA_PANE().Palette().Name( "Libraries" ).Left().Layer(1)
                      .Caption( _( "Footprint Libraries" ) )
                      .BestSize((int) ( m_frameSize.x * 0.20 ), m_frameSize.y ) );

    m_auimgr.AddPane( m_compListBox, EDA_PANE().Palette().Name( "Components" ).Center().Layer(0)
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
    fgSizerStatus->Add( m_statusLine3, 0, wxBOTTOM, 3 );

    panelSizer->Add( fgSizerStatus, 1, wxEXPAND|wxLEFT, 2 );

    wxStaticLine* staticline1 = new wxStaticLine( bottomPanel );
    panelSizer->Add( staticline1, 0, wxEXPAND, 5 );

    wxFont statusFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    statusFont.SetSymbolicSize( wxFONTSIZE_SMALL );
    m_statusLine1->SetFont( statusFont );
    m_statusLine2->SetFont( statusFont );
    m_statusLine3->SetFont( statusFont );

    // Add buttons:
    auto buttonsSizer = new wxBoxSizer( wxHORIZONTAL );
    auto sdbSizer = new wxStdDialogButtonSizer();

    m_saveAndContinue = new wxButton( bottomPanel, ID_SAVE_PROJECT,
                                      _( "Apply, Save Schematic && Continue" ) );
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

    if( CVPCB_SETTINGS* cfg = dynamic_cast<CVPCB_SETTINGS*>( config() ) )
    {
        if( cfg->m_LibrariesWidth > 0 )
        {
            wxAuiPaneInfo& librariesPane = m_auimgr.GetPane( "Libraries" );

            // wxAUI hack: force width by setting MinSize() and then Fixed()
            // thanks to ZenJu http://trac.wxwidgets.org/ticket/13180
            librariesPane.MinSize( cfg->m_LibrariesWidth, -1 );
            librariesPane.BestSize( cfg->m_LibrariesWidth, -1 );
            librariesPane.MaxSize( cfg->m_LibrariesWidth, -1 );
            librariesPane.Fixed();
            m_auimgr.Update();

            // now make it resizable again
            librariesPane.MinSize( 20, -1 );
            librariesPane.Resizable();
            m_auimgr.Update();
        }

        if( cfg->m_FootprintsWidth > 0 )
        {
            wxAuiPaneInfo& footprintsPane = m_auimgr.GetPane( "Footprints" );

            // wxAUI hack: force width by setting MinSize() and then Fixed()
            // thanks to ZenJu http://trac.wxwidgets.org/ticket/13180
            footprintsPane.MinSize( cfg->m_FootprintsWidth, -1 );
            footprintsPane.BestSize( cfg->m_FootprintsWidth, -1 );
            footprintsPane.MaxSize( cfg->m_FootprintsWidth, -1 );
            footprintsPane.Fixed();
            m_auimgr.Update();

            // now make it resizable again
            footprintsPane.MinSize( 20, -1 );
            footprintsPane.Resizable();
            m_auimgr.Update();
        }
    }

    // Connect Events
    setupEventHandlers();

    // Start the main processing loop
    m_toolManager->InvokeTool( "cvpcb.Control" );

    KIPLATFORM::APP::SetShutdownBlockReason( this, _( "Symbol to footprint changes are unsaved" ) );
}


CVPCB_MAINFRAME::~CVPCB_MAINFRAME()
{
    // Shutdown all running tools
    if( m_toolManager )
        m_toolManager->ShutdownAllTools();

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
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager, m_actions );

    // Register tools
    m_toolManager->RegisterTool( new COMMON_CONTROL );
    m_toolManager->RegisterTool( new CVPCB_CONTROL );
    m_toolManager->RegisterTool( new CVPCB_ASSOCIATION_TOOL );
    m_toolManager->InitTools();

    CVPCB_CONTROL* tool = m_toolManager->GetTool<CVPCB_CONTROL>();

    // Even though these menus will open with the right-click, we treat them as a normal
    // menu instead of a context menu because we don't care about their position and want
    // to be able to tell the difference between a menu click and a hotkey activation.

    // Create the context menu for the component list box
    m_componentContextMenu = new ACTION_MENU( false );
    m_componentContextMenu->SetTool( tool );
    m_componentContextMenu->Add( CVPCB_ACTIONS::showFootprintViewer );
    m_componentContextMenu->AppendSeparator();
    m_componentContextMenu->Add( ACTIONS::cut );
    m_componentContextMenu->Add( ACTIONS::copy );
    m_componentContextMenu->Add( ACTIONS::paste );
    m_componentContextMenu->AppendSeparator();
    m_componentContextMenu->Add( CVPCB_ACTIONS::deleteAssoc );

    // Create the context menu for the footprint list box
    m_footprintContextMenu = new ACTION_MENU( false );
    m_footprintContextMenu->SetTool( tool );
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

    mgr->SetConditions( CVPCB_ACTIONS::saveAssociations, ENABLE( cond.ContentModified() ) );
    mgr->SetConditions( ACTIONS::undo,                   ENABLE( cond.UndoAvailable() ) );
    mgr->SetConditions( ACTIONS::redo,                   ENABLE( cond.RedoAvailable() ) );

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

    m_compListBox->Bind( wxEVT_RIGHT_DOWN,
            [this]( wxMouseEvent& )
            {
                PopupMenu( m_componentContextMenu );
            } );

    // Connect the handler for the save button
    m_saveAndContinue->Bind( wxEVT_COMMAND_BUTTON_CLICKED,
            [this]( wxCommandEvent& )
            {
                // saveAssociations must be run immediately
                GetToolManager()->RunAction( CVPCB_ACTIONS::saveAssociations, true );
            } );

    // Connect the handlers for the ok/cancel buttons
    Bind( wxEVT_BUTTON,
            [this]( wxCommandEvent& )
            {
                // saveAssociations must be run immediately, before running Close( true )
                GetToolManager()->RunAction( CVPCB_ACTIONS::saveAssociations, true );
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

    // Toolbar events
    Bind( wxEVT_TEXT, &CVPCB_MAINFRAME::OnEnterFilteringText, this, ID_CVPCB_FILTER_TEXT_EDIT );

    // Just skip the resize events
    Bind( wxEVT_SIZE,
          []( wxSizeEvent& aEvent )
          {
              aEvent.Skip();
          } );

    // Attach the events to the tool dispatcher
    Bind( wxEVT_TOOL, &TOOL_DISPATCHER::DispatchWxCommand, m_toolDispatcher );
    Bind( wxEVT_CHAR, &TOOL_DISPATCHER::DispatchWxEvent, m_toolDispatcher );
    Bind( wxEVT_CHAR_HOOK, &TOOL_DISPATCHER::DispatchWxEvent, m_toolDispatcher );
}


bool CVPCB_MAINFRAME::canCloseWindow( wxCloseEvent& aEvent )
{
    if( m_modified )
    {
        // Shutdown blocks must be determined and vetoed as early as possible
        if( KIPLATFORM::APP::SupportsShutdownBlockReason()
                && aEvent.GetId() == wxEVT_QUERY_END_SESSION )
        {
            return false;
        }

        if( !HandleUnsavedChanges( this, _( "Symbol to Footprint links have been modified. "
                                            "Save changes?" ),
                                   [&]() -> bool
                                   {
                                       return SaveFootprintAssociation( false );
                                   } ) )
        {
            return false;
        }
    }

    return true;
}


void CVPCB_MAINFRAME::doCloseWindow()
{
    if( GetFootprintViewerFrame() )
        GetFootprintViewerFrame()->Close( true );

    m_modified = false;

    // clear highlight symbol in schematic:
    SendMessageToEESCHEMA( true );
}


void CVPCB_MAINFRAME::OnEnterFilteringText( wxCommandEvent& aEvent )
{
    // Called when changing the filter string in main toolbar.
    // If the option FOOTPRINTS_LISTBOX::FILTERING_BY_TEXT_PATTERN is set, update the list
    // of available footprints which match the filter

    wxListEvent l_event;
    OnSelectComponent( l_event );
}


void CVPCB_MAINFRAME::OnSelectComponent( wxListEvent& event )
{
    if( m_skipComponentSelect )
        return;

    wxString   libraryName;
    COMPONENT* component = GetSelectedComponent();
    libraryName = m_libListBox->GetSelectedLibrary();

    m_footprintListBox->SetFootprints( *m_FootprintsList, libraryName, component,
                                       m_tcFilterString->GetValue(), m_filteringOptions );

    if( component && component->GetFPID().IsValid() )
        m_footprintListBox->SetSelectedFootprint( component->GetFPID() );
    else
        m_footprintListBox->SetSelection( m_footprintListBox->GetSelection(), false );

    refreshAfterComponentSearch( component );
}


void CVPCB_MAINFRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    EDA_BASE_FRAME::LoadSettings( aCfg );

    CVPCB_SETTINGS* cfg = static_cast<CVPCB_SETTINGS*>( aCfg );

    m_filteringOptions = cfg->m_FilterFootprint;
}


void CVPCB_MAINFRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    EDA_BASE_FRAME::SaveSettings( aCfg );

    CVPCB_SETTINGS* cfg = static_cast<CVPCB_SETTINGS*>( aCfg );
    cfg->m_FilterFootprint = m_filteringOptions;

    cfg->m_LibrariesWidth = m_libListBox->GetSize().x;
    cfg->m_FootprintsWidth = m_footprintListBox->GetSize().x;
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


void CVPCB_MAINFRAME::AssociateFootprint( const CVPCB_ASSOCIATION& aAssociation,
                                          bool aNewEntry, bool aAddUndoItem )
{
    if( m_netlist.IsEmpty() )
        return;

    COMPONENT* component = m_netlist.GetComponent( aAssociation.GetComponentIndex() );

    if( component == NULL )
        return;

    LIB_ID fpid    = aAssociation.GetNewFootprint();
    LIB_ID oldFpid = component->GetFPID();

    // Test for validity of the requested footprint
    if( !fpid.empty() && !fpid.IsValid() )
    {
        wxString msg = wxString::Format( _( "\"%s\" is not a valid footprint." ),
                                         fpid.Format().wx_str() );
        DisplayErrorMessage( this, msg );
        return;
    }

    const KIID& id = component->GetKIIDs().front();

    // Set new footprint to all instances of the selected component
    for( unsigned int idx : GetComponentIndices() )
    {
        COMPONENT* candidate = m_netlist.GetComponent( idx );
        const std::vector<KIID>& kiids = candidate->GetKIIDs();

        if( std::find( kiids.begin(), kiids.end(), id ) != kiids.end() )
        {
            // Set the new footprint
            candidate->SetFPID( fpid );

            // create the new component description and set it
            wxString description = wxString::Format( CMP_FORMAT,
                                                     idx + 1,
                                                     candidate->GetReference(),
                                                     candidate->GetValue(),
                                                     candidate->GetFPID().Format().wx_str() );
            m_compListBox->SetString( idx, description );
        }
    }

    // Mark the data as being modified
    m_modified = true;

    // Update the statusbar and refresh the list
    DisplayStatus();
    m_compListBox->Refresh();

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


void CVPCB_MAINFRAME::refreshAfterComponentSearch( COMPONENT* component )
{
    // Tell AuiMgr that objects are changed !
    if( m_auimgr.GetManagedWindow() )   // Be sure Aui Manager is initialized
        m_auimgr.Update();              // (could be not the case when starting CvPcb)

    if( component == NULL )
    {
        DisplayStatus();
        return;
    }

    // Preview of the already assigned footprint.
    // Find the footprint that was already chosen for this component and select it,
    // but only if the selection is made from the component list or the library list.
    // If the selection is made from the footprint list, do not change the current
    // selected footprint.
    if( FindFocus() == m_compListBox || FindFocus() == m_libListBox )
    {
        wxString footprintName = FROM_UTF8( component->GetFPID().Format().c_str() );

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

        if( GetFootprintViewerFrame() )
            m_toolManager->RunAction( CVPCB_ACTIONS::showFootprintViewer, true );
    }

    SendMessageToEESCHEMA();
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
    COMPONENT* component = GetSelectedComponent();

    if( ( m_filteringOptions & FOOTPRINTS_LISTBOX::FILTERING_BY_COMPONENT_FP_FILTERS ) )
    {
        msg.Empty();

        if( component )
        {
            for( unsigned ii = 0;  ii < component->GetFootprintFilters().GetCount();  ii++ )
            {
                if( msg.IsEmpty() )
                    msg += component->GetFootprintFilters()[ii];
                else
                    msg += wxT( ", " ) + component->GetFootprintFilters()[ii];
            }
        }

        filters += _( "Keywords" );

        if( !msg.IsEmpty() )
            filters += wxString::Format( wxT( " (%s)" ), msg );
    }

    if( ( m_filteringOptions & FOOTPRINTS_LISTBOX::FILTERING_BY_PIN_COUNT ) )
    {
        msg.Empty();

        if( component )
            msg = wxString::Format( wxT( "%i" ), component->GetPinCount() );

        if( !filters.IsEmpty() )
            filters += wxT( ", " );

        filters += _( "Pin Count" );

        if( !msg.IsEmpty() )
            filters += wxString::Format( wxT( " (%s)" ), msg );
    }

    if( ( m_filteringOptions & FOOTPRINTS_LISTBOX::FILTERING_BY_LIBRARY ) )
    {
        msg = m_libListBox->GetSelectedLibrary();

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

    msg << wxT( ": " ) << m_footprintListBox->GetCount();

    SetStatusText( msg );


    msg.Empty();
    wxString footprintName = GetSelectedFootprint();

    FOOTPRINT_INFO* fp = m_FootprintsList->GetFootprintInfo( footprintName );

    if( fp )    // can be NULL if no netlist loaded
    {
        msg = wxString::Format( _( "Description: %s;  Keywords: %s" ),
                                fp->GetDescription(),
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
        // Use the footprint of the selected component
        if( component )
            lib = component->GetFPID().GetLibNickname();
    }
    else if( GetFocusedControl() == CVPCB_MAINFRAME::CONTROL_LIBRARY )
    {
        // Use the library that is selected
        lib = m_libListBox->GetSelectedLibrary();
    }

    // Extract the library information
    FP_LIB_TABLE* fptbl = Prj().PcbFootprintLibs( Kiway() );

    if( fptbl->HasLibrary( lib ) )
        msg = wxString::Format( _( "Library location: %s" ), fptbl->GetFullURI( lib ) );
    else
        msg = wxString::Format( _( "Library location: unknown" ) );

    SetStatusText( msg, 2 );
}


bool CVPCB_MAINFRAME::LoadFootprintFiles()
{
    FP_LIB_TABLE* fptbl = Prj().PcbFootprintLibs( Kiway() );

    // Check if there are footprint libraries in the footprint library table.
    if( !fptbl || !fptbl->GetLogicalLibs().size() )
    {
        wxMessageBox( _( "No PCB footprint libraries are listed in the current footprint "
                         "library table." ), _( "Configuration Error" ), wxOK | wxICON_ERROR );
        return false;
    }

    WX_PROGRESS_REPORTER progressReporter( this, _( "Loading Footprint Libraries" ), 2 );

    m_FootprintsList->ReadFootprintFiles( fptbl, nullptr, &progressReporter );

    if( m_FootprintsList->GetErrorCount() )
    {
        m_FootprintsList->DisplayErrors( this );
    }

    return true;
}


void CVPCB_MAINFRAME::SendMessageToEESCHEMA( bool aClearHighligntOnly )
{
    if( m_netlist.IsEmpty() )
        return;

    // clear highlight of previously selected components (if any):
    // Selecting a non existing symbol clears any previously highlighted symbols
    std::string packet = "$CLEAR: \"HIGHLIGHTED\"";

    if( Kiface().IsSingle() )
        SendCommand( MSG_TO_SCH, packet );
    else
        Kiway().ExpressMail( FRAME_SCH, MAIL_CROSS_PROBE, packet, this );

    if( aClearHighligntOnly )
        return;

    int selection = m_compListBox->GetSelection();

    if ( selection < 0 )    // Nothing selected
        return;

    if( m_netlist.GetComponent( selection ) == NULL )
        return;

    // Now highlight the selected component:
    COMPONENT* component = m_netlist.GetComponent( selection );

    packet = StrPrintf( "$PART: \"%s\"", TO_UTF8( component->GetReference() ) );

    if( Kiface().IsSingle() )
        SendCommand( MSG_TO_SCH, packet );
    else
        Kiway().ExpressMail( FRAME_SCH, MAIL_CROSS_PROBE, packet, this );
}


int CVPCB_MAINFRAME::ReadSchematicNetlist( const std::string& aNetlist )
{
    STRING_LINE_READER*     strrdr = new STRING_LINE_READER( aNetlist, "Eeschema via Kiway" );
    KICAD_NETLIST_READER    netrdr( strrdr, &m_netlist );

    m_netlist.Clear();

    try
    {
        netrdr.LoadNetlist();
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

    // Sort components by reference:
    m_netlist.SortByReference();

    return 0;
}


void CVPCB_MAINFRAME::BuildFOOTPRINTS_LISTBOX()
{
    wxFont   guiFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );

    if( m_footprintListBox == NULL )
    {
        m_footprintListBox = new FOOTPRINTS_LISTBOX( this, ID_CVPCB_FOOTPRINT_LIST );
        m_footprintListBox->SetFont( wxFont( guiFont.GetPointSize(), wxFONTFAMILY_MODERN,
                                             wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL ) );
    }

    m_footprintListBox->SetFootprints( *m_FootprintsList, wxEmptyString, NULL, wxEmptyString,
                                       FOOTPRINTS_LISTBOX::UNFILTERED_FP_LIST );
    DisplayStatus();
}


void CVPCB_MAINFRAME::BuildCmpListBox()
{
    wxString    msg;
    COMPONENT*  component;
    wxFont      guiFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );

    if( m_compListBox == NULL )
    {
        m_compListBox = new COMPONENTS_LISTBOX( this, ID_CVPCB_COMPONENT_LIST );
        m_compListBox->SetFont( wxFont( guiFont.GetPointSize(), wxFONTFAMILY_MODERN,
                                        wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL ) );
    }

    m_compListBox->m_ComponentList.Clear();

    for( unsigned i = 0;  i < m_netlist.GetCount();  i++ )
    {
        component = m_netlist.GetComponent( i );

        msg.Printf( CMP_FORMAT,
                    m_compListBox->GetCount() + 1,
                    component->GetReference(),
                    component->GetValue(),
                    component->GetFPID().Format().wx_str() );
        m_compListBox->m_ComponentList.Add( msg );
    }

    if( m_compListBox->m_ComponentList.Count() )
    {
        m_compListBox->SetItemCount( m_compListBox->m_ComponentList.Count() );
        m_compListBox->SetSelection( 0, true );
        m_compListBox->RefreshItems( 0L, m_compListBox->m_ComponentList.Count()-1 );
        m_compListBox->UpdateWidth();
    }
}


void CVPCB_MAINFRAME::BuildLIBRARY_LISTBOX()
{
    wxFont   guiFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );

    if( m_libListBox == NULL )
    {
        m_libListBox = new LIBRARY_LISTBOX( this, ID_CVPCB_LIBRARY_LIST );
        m_libListBox->SetFont( wxFont( guiFont.GetPointSize(), wxFONTFAMILY_MODERN,
                                       wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL ) );
    }

    FP_LIB_TABLE* tbl = Prj().PcbFootprintLibs( Kiway() );

    if( tbl )
    {
        wxArrayString libNames;

        std::vector< wxString > libNickNames = tbl->GetLogicalLibs();

        for( const wxString& libNickName : libNickNames )
            libNames.Add( libNickName );

        m_libListBox->SetLibraryList( libNames );
    }
}


COMPONENT* CVPCB_MAINFRAME::GetSelectedComponent()
{
    int selection = m_compListBox->GetSelection();

    if( selection >= 0 && selection < (int) m_netlist.GetCount() )
        return m_netlist.GetComponent( selection );

    return NULL;
}


void CVPCB_MAINFRAME::SetSelectedComponent( int aIndex, bool aSkipUpdate )
{
    m_skipComponentSelect = aSkipUpdate;

    if( aIndex < 0 )
    {
        m_compListBox->DeselectAll();
    }
    else if( aIndex < m_compListBox->GetCount() )
    {
        m_compListBox->DeselectAll();
        m_compListBox->SetSelection( aIndex );
        SendMessageToEESCHEMA();
    }

    m_skipComponentSelect = false;
}


std::vector<unsigned int> CVPCB_MAINFRAME::GetComponentIndices(
        CVPCB_MAINFRAME::CRITERIA aCriteria )
{
    std::vector<unsigned int> idx;
    int                       lastIdx;

    // Make sure a netlist has been loaded and the box has contents
    if( m_netlist.IsEmpty() || m_compListBox->GetCount() == 0 )
        return idx;

    switch( aCriteria )
    {
    case CVPCB_MAINFRAME::ALL_COMPONENTS:
        idx.resize( m_netlist.GetCount() );
        std::iota( idx.begin(), idx.end(), 0 );
        break;

    case CVPCB_MAINFRAME::SEL_COMPONENTS:
        // Check to see if anything is selected
        if( m_compListBox->GetSelectedItemCount() < 1 )
            break;

        // Get the components
        lastIdx = m_compListBox->GetFirstSelected();
        idx.emplace_back( lastIdx );

        lastIdx = m_compListBox->GetNextSelected( lastIdx );
        while( lastIdx > 0 )
        {
            idx.emplace_back( lastIdx );
            lastIdx = m_compListBox->GetNextSelected( lastIdx );
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
        wxASSERT_MSG( false, "Invalid component selection criteria" );
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
    if( m_libListBox->HasFocus() )
        return CVPCB_MAINFRAME::CONTROL_LIBRARY;
    else if( m_compListBox->HasFocus() )
        return CVPCB_MAINFRAME::CONTROL_COMPONENT;
    else if( m_footprintListBox->HasFocus() )
        return CVPCB_MAINFRAME::CONTROL_FOOTPRINT;

    return CVPCB_MAINFRAME::CONTROL_NONE;
}


wxControl* CVPCB_MAINFRAME::GetFocusedControlObject() const
{
    if( m_libListBox->HasFocus() )
        return m_libListBox;
    else if( m_compListBox->HasFocus() )
        return m_compListBox;
    else if( m_footprintListBox->HasFocus() )
        return m_footprintListBox;

    return nullptr;
}


void CVPCB_MAINFRAME::SetFocusedControl( CVPCB_MAINFRAME::CONTROL_TYPE aLB )
{
    switch( aLB )
    {
    case CVPCB_MAINFRAME::CONTROL_LIBRARY:   m_libListBox->SetFocus();       break;
    case CVPCB_MAINFRAME::CONTROL_COMPONENT: m_compListBox->SetFocus();      break;
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
    case 0:  m_statusLine1->SetLabel( aText );          break;
    case 1:  m_statusLine2->SetLabel( aText );          break;
    case 2:  m_statusLine3->SetLabel( aText );          break;
    default: wxFAIL_MSG( "Invalid status row number" ); break;
    }
}


void CVPCB_MAINFRAME::ShowChangedLanguage()
{
    EDA_BASE_FRAME::ShowChangedLanguage();
    ReCreateHToolbar();
    DisplayStatus();
}


void CVPCB_MAINFRAME::KiwayMailIn( KIWAY_EXPRESS& mail )
{
    const std::string& payload = mail.GetPayload();

    switch( mail.Command() )
    {
    case MAIL_EESCHEMA_NETLIST:
        ReadNetListAndFpFiles( payload );
        /* @todo
        Go into SCH_EDIT_FRAME::OnOpenCvpcb( wxCommandEvent& event ) and trim GNL_ALL down.
        */
        break;

    default:
        ;       // ignore most
    }
}
