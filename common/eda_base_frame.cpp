/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2023 CERN (www.cern.ch)
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

#include "dialogs/panel_maintenance.h"
#include "kicad_manager_frame.h"
#include <eda_base_frame.h>

#include <advanced_config.h>
#include <bitmaps.h>
#include <bitmap_store.h>
#include <dialog_shim.h>
#include <dialogs/git/panel_git_repos.h>
#include <dialogs/panel_common_settings.h>
#include <dialogs/panel_mouse_settings.h>
#include <dialogs/panel_spacemouse.h>
#include <dialogs/panel_data_collection.h>
#include <dialogs/panel_plugin_settings.h>
#include <eda_dde.h>
#include <file_history.h>
#include <id.h>
#include <kiface_base.h>
#include <hotkeys_basic.h>
#include <panel_hotkeys_editor.h>
#include <paths.h>
#include <local_history.h>
#include <confirm.h>
#include <panel_packages_and_updates.h>
#include <pgm_base.h>
#include <settings/app_settings.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <project/project_local_settings.h>
#include <tool/action_manager.h>
#include <tool/action_menu.h>
#include <tool/action_toolbar.h>
#include <tool/actions.h>
#include <tool/common_control.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tool/ui/toolbar_configuration.h>
#include <trace_helpers.h>
#include <widgets/paged_dialog.h>
#include <widgets/wx_busy_indicator.h>
#include <widgets/wx_infobar.h>
#include <widgets/aui_json_serializer.h>
#include <widgets/wx_aui_art_providers.h>
#include <widgets/wx_grid.h>
#include <widgets/wx_treebook.h>
#include <wx/app.h>
#include <wx/config.h>
#include <wx/display.h>
#include <wx/stdpaths.h>
#include <wx/string.h>
#include <wx/msgdlg.h>
#include <wx/wupdlock.h>
#include <kiplatform/app.h>
#include <kiplatform/io.h>
#include <kiplatform/ui.h>

#include <nlohmann/json.hpp>

#include <functional>
#include <kiface_ids.h>

#ifdef KICAD_IPC_API
#include <api/api_server.h>
#endif


// Minimum window size
static const wxSize minSizeLookup( FRAME_T aFrameType, wxWindow* aWindow )
{
    switch( aFrameType )
    {
    case KICAD_MAIN_FRAME_T:
        return wxWindow::FromDIP( wxSize( 406, 354 ), aWindow );

    default:
        return wxWindow::FromDIP( wxSize( 500, 400 ), aWindow );
    }
}


static const wxSize defaultSize( FRAME_T aFrameType, wxWindow* aWindow )
{
    switch( aFrameType )
    {
    case KICAD_MAIN_FRAME_T:
        return wxWindow::FromDIP( wxSize( 850, 540 ), aWindow );

    default:
        return wxWindow::FromDIP( wxSize( 1280, 720 ), aWindow );
    }
}


BEGIN_EVENT_TABLE( EDA_BASE_FRAME, wxFrame )
    // These event table entries are needed to handle events from the mac application menu
    EVT_MENU( wxID_ABOUT, EDA_BASE_FRAME::OnKicadAbout )
    EVT_MENU( wxID_PREFERENCES, EDA_BASE_FRAME::OnPreferences )

    EVT_CHAR_HOOK( EDA_BASE_FRAME::OnCharHook )
    EVT_MENU_OPEN( EDA_BASE_FRAME::OnMenuEvent )
    EVT_MENU_CLOSE( EDA_BASE_FRAME::OnMenuEvent )
    EVT_MENU_HIGHLIGHT_ALL( EDA_BASE_FRAME::OnMenuEvent )
    EVT_MOVE( EDA_BASE_FRAME::OnMove )
    EVT_SIZE( EDA_BASE_FRAME::OnSize )
    EVT_MAXIMIZE( EDA_BASE_FRAME::OnMaximize )

    EVT_SYS_COLOUR_CHANGED( EDA_BASE_FRAME::onSystemColorChange )
    EVT_ICONIZE( EDA_BASE_FRAME::onIconize )
END_EVENT_TABLE()


void EDA_BASE_FRAME::commonInit( FRAME_T aFrameType )
{
    m_ident             = aFrameType;
    m_maximizeByDefault = false;
    m_infoBar           = nullptr;
    m_settingsManager   = nullptr;
    m_fileHistory       = nullptr;
    m_supportsAutoSave  = false;
    m_autoSavePending   = false;
    m_undoRedoCountMax  = DEFAULT_MAX_UNDO_ITEMS;
    m_isClosing         = false;
    m_isNonUserClose    = false;
    m_autoSaveTimer     = new wxTimer( this, ID_AUTO_SAVE_TIMER );
    m_autoSaveRequired  = false;
    m_autoSavePermissionError = false;
    m_mruPath           = PATHS::GetDefaultUserProjectsPath();
    m_frameSize         = defaultSize( aFrameType, this );
    m_displayIndex      = -1;

    m_auimgr.SetArtProvider( new WX_AUI_DOCK_ART() );

    m_settingsManager = &Pgm().GetSettingsManager();

    // Set a reasonable minimal size for the frame
    wxSize minSize = minSizeLookup( aFrameType, this );
    SetSizeHints( minSize.x, minSize.y, -1, -1, -1, -1 );

    // Store dimensions of the user area of the main window.
    GetClientSize( &m_frameSize.x, &m_frameSize.y );

    Connect( ID_AUTO_SAVE_TIMER, wxEVT_TIMER,
             wxTimerEventHandler( EDA_BASE_FRAME::onAutoSaveTimer ) );

    // hook wxEVT_CLOSE_WINDOW so we can call SaveSettings().  This function seems
    // to be called before any other hook for wxCloseEvent, which is necessary.
    Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( EDA_BASE_FRAME::windowClosing ) );

    initExitKey();
}


EDA_BASE_FRAME::EDA_BASE_FRAME( wxWindow* aParent, FRAME_T aFrameType, const wxString& aTitle,
                                const wxPoint& aPos, const wxSize& aSize, long aStyle,
                                const wxString& aFrameName, KIWAY* aKiway,
                                const EDA_IU_SCALE& aIuScale ) :
        wxFrame( aParent, wxID_ANY, aTitle, aPos, aSize, aStyle, aFrameName ),
        TOOLS_HOLDER(),
        KIWAY_HOLDER( aKiway, KIWAY_HOLDER::FRAME ),
        UNITS_PROVIDER( aIuScale, EDA_UNITS::MM )
{
    m_tbTopMain      = nullptr;
    m_tbTopAux = nullptr;
    m_tbRight      = nullptr;
    m_tbLeft   = nullptr;

    commonInit( aFrameType );

    Bind( wxEVT_DPI_CHANGED,
          [&]( wxDPIChangedEvent& aEvent )
          {
#ifdef __WXMSW__
              // Workaround to update toolbar sizes on MSW
              if( m_auimgr.GetManagedWindow() )
              {
                  wxAuiPaneInfoArray& panes = m_auimgr.GetAllPanes();

                  for( size_t ii = 0; ii < panes.GetCount(); ii++ )
                  {
                      wxAuiPaneInfo& pinfo = panes.Item( ii );
                      pinfo.best_size = pinfo.window->GetSize();

                      // But we still shouldn't make it too small.
                      pinfo.best_size.IncTo( pinfo.window->GetBestSize() );
                      pinfo.best_size.IncTo( pinfo.min_size );
                  }

                  m_auimgr.Update();
              }
#endif

              aEvent.Skip();
          } );
}


wxWindow* findQuasiModalDialog( wxWindow* aParent )
{
    for( wxWindow* child : aParent->GetChildren() )
    {
        if( DIALOG_SHIM* dlg = dynamic_cast<DIALOG_SHIM*>( child ) )
        {
            if( dlg->IsQuasiModal() )
                return dlg;

            if( wxWindow* nestedDlg = findQuasiModalDialog( child ) )
                return nestedDlg;
        }
    }

    return nullptr;
}


wxWindow* EDA_BASE_FRAME::findQuasiModalDialog()
{
    if( wxWindow* dlg = ::findQuasiModalDialog( this ) )
        return dlg;

    // FIXME: CvPcb is currently implemented on top of KIWAY_PLAYER rather than DIALOG_SHIM,
    // so we have to look for it separately.
    if( m_ident == FRAME_SCH )
    {
        wxWindow* cvpcb = wxWindow::FindWindowByName( wxS( "CvpcbFrame" ) );

        if( cvpcb )
            return cvpcb;
    }

    return nullptr;
}


void EDA_BASE_FRAME::windowClosing( wxCloseEvent& event )
{
    // Don't allow closing when a quasi-modal is open.
    wxWindow* quasiModal = findQuasiModalDialog();

    if( quasiModal )
    {
        // Raise and notify; don't give the user a warning regarding "quasi-modal dialogs"
        // when they have no idea what those are.
        quasiModal->Raise();
        wxBell();

        if( event.CanVeto() )
            event.Veto();

        return;
    }


    if( event.GetId() == wxEVT_QUERY_END_SESSION
        || event.GetId() == wxEVT_END_SESSION )
    {
        // End session means the OS is going to terminate us
        m_isNonUserClose = true;
    }

    if( canCloseWindow( event ) )
    {
        m_isClosing = true;

        if( m_infoBar )
            m_infoBar->Dismiss();

        APP_SETTINGS_BASE* cfg = config();

        if( cfg )
            SaveSettings( cfg );    // virtual, wxFrame specific

        doCloseWindow();

        // Destroy (safe delete frame) this frame only in non modal mode.
        // In modal mode, the caller will call Destroy().
        if( !IsModal() )
            Destroy();
    }
    else
    {
        if( event.CanVeto() )
            event.Veto();
    }
}


EDA_BASE_FRAME::~EDA_BASE_FRAME()
{
    Disconnect( ID_AUTO_SAVE_TIMER, wxEVT_TIMER,
                wxTimerEventHandler( EDA_BASE_FRAME::onAutoSaveTimer ) );
    Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( EDA_BASE_FRAME::windowClosing ) );

    delete m_autoSaveTimer;
    delete m_fileHistory;

    ClearUndoRedoList();

    SocketCleanup();

    KIPLATFORM::APP::RemoveShutdownBlockReason( this );
}


bool EDA_BASE_FRAME::ProcessEvent( wxEvent& aEvent )
{
#ifdef  __WXMAC__
    // Apple in its infinite wisdom will raise a disabled window before even passing
    // us the event, so we have no way to stop it.  Instead, we have to catch an
    // improperly ordered disabled window and quasi-modal dialog here and reorder
    // them.
    if( !IsEnabled() && IsActive() )
    {
        wxWindow* dlg = findQuasiModalDialog();

        if( dlg )
            dlg->Raise();
    }
#endif

#ifdef __WXMSW__
    // When changing DPI to a lower value, somehow, called from wxNonOwnedWindow::HandleDPIChange,
    // our sizers compute a min size that is larger than the old frame size. wx then sets this wrong size.
    // This shouldn't be needed since the OS have already sent a size event.
    // Avoid this wx behaviour by pretending we've processed the event even if we use Skip in handlers.
    if( aEvent.GetEventType() == wxEVT_DPI_CHANGED )
    {
        wxFrame::ProcessEvent( aEvent );
        return true;
    }
#endif

    if( !wxFrame::ProcessEvent( aEvent ) )
        return false;

    if( Pgm().m_Quitting )
        return true;

    if( !m_isClosing && m_supportsAutoSave && IsShownOnScreen() && IsActive()
            && m_autoSavePending != isAutoSaveRequired()
            && GetAutoSaveInterval() > 0 )
    {
        if( !m_autoSavePending )
        {
            wxLogTrace( traceAutoSave, wxT( "Starting auto save timer." ) );
            m_autoSaveTimer->Start( GetAutoSaveInterval() * 1000, wxTIMER_ONE_SHOT );
            m_autoSavePending = true;
        }
        else if( m_autoSaveTimer->IsRunning() )
        {
            wxLogTrace( traceAutoSave, wxT( "Stopping auto save timer." ) );
            m_autoSaveTimer->Stop();
            m_autoSavePending = false;
        }
    }

    return true;
}


int EDA_BASE_FRAME::GetAutoSaveInterval() const
{
    return Pgm().GetCommonSettings()->m_System.local_history_debounce;
}


void EDA_BASE_FRAME::onAutoSaveTimer( wxTimerEvent& aEvent )
{
    // Don't stomp on someone else's timer event.
    if( aEvent.GetId() != ID_AUTO_SAVE_TIMER )
    {
        aEvent.Skip();
        return;
    }

    if( !doAutoSave() )
        m_autoSaveTimer->Start( GetAutoSaveInterval() * 1000, wxTIMER_ONE_SHOT );
}


bool EDA_BASE_FRAME::doAutoSave()
{
    m_autoSaveRequired = false;
    m_autoSavePending = false;

    // Use registered saver callbacks to snapshot editor state into .history and only commit
    // if there are material changes.
    if( !Prj().IsReadOnly() )
        Kiway().LocalHistory().RunRegisteredSaversAndCommit( Prj().GetProjectPath(), wxS( "Autosave" ) );

    return true;
}


void EDA_BASE_FRAME::OnCharHook( wxKeyEvent& aKeyEvent )
{
    wxLogTrace( kicadTraceKeyEvent, wxS( "EDA_BASE_FRAME::OnCharHook %s" ), dump( aKeyEvent ) );

    // Key events can be filtered here.
    // Currently no filtering is made.
    aKeyEvent.Skip();
}


void EDA_BASE_FRAME::OnMenuEvent( wxMenuEvent& aEvent )
{
    if( !m_toolDispatcher )
        aEvent.Skip();
    else
        m_toolDispatcher->DispatchWxEvent( aEvent );
}


void EDA_BASE_FRAME::RegisterUIUpdateHandler( int aID, const ACTION_CONDITIONS& aConditions )
{
    UIUpdateHandler evtFunc = std::bind( &EDA_BASE_FRAME::HandleUpdateUIEvent,
                                         std::placeholders::_1,
                                         this,
                                         aConditions );

    m_uiUpdateMap[aID] = evtFunc;

    Bind( wxEVT_UPDATE_UI, evtFunc, aID );
}


void EDA_BASE_FRAME::UnregisterUIUpdateHandler( int aID )
{
    const auto it = m_uiUpdateMap.find( aID );

    if( it == m_uiUpdateMap.end() )
        return;

    Unbind( wxEVT_UPDATE_UI, it->second, aID );
}


void EDA_BASE_FRAME::HandleUpdateUIEvent( wxUpdateUIEvent& aEvent, EDA_BASE_FRAME* aFrame,
                                          ACTION_CONDITIONS aCond )
{
    bool       checkRes  = false;
    bool       enableRes = true;
    bool       showRes   = true;
    bool       isCut     = aEvent.GetId() == ACTIONS::cut.GetUIId();
    bool       isCopy    = aEvent.GetId() == ACTIONS::copy.GetUIId();
    bool       isPaste   = aEvent.GetId() == ACTIONS::paste.GetUIId();
    SELECTION& selection = aFrame->GetCurrentSelection();

    try
    {
        checkRes  = aCond.checkCondition( selection );
        enableRes = aCond.enableCondition( selection );
        showRes   = aCond.showCondition( selection );
    }
    catch( std::exception& )
    {
        // Something broke with the conditions, just skip the event.
        aEvent.Skip();
        return;
    }

    if( showRes && aEvent.GetId() == ACTIONS::undo.GetUIId() )
    {
        wxString msg = _( "Undo" );

        if( enableRes )
            msg += wxS( " " ) + aFrame->GetUndoActionDescription();

        aEvent.SetText( msg );
    }
    else if( showRes && aEvent.GetId() == ACTIONS::redo.GetUIId() )
    {
        wxString msg = _( "Redo" );

        if( enableRes )
            msg += wxS( " " ) + aFrame->GetRedoActionDescription();

        aEvent.SetText( msg );
    }

    if( isCut || isCopy || isPaste )
    {
        wxWindow*    focus = wxWindow::FindFocus();
        wxTextEntry* textEntry = dynamic_cast<wxTextEntry*>( focus );

        if( textEntry && isCut && textEntry->CanCut() )
            enableRes = true;
        else if( textEntry && isCopy && textEntry->CanCopy() )
            enableRes = true;
        else if( textEntry && isPaste && textEntry->CanPaste() )
            enableRes = true;
        else if( dynamic_cast<WX_GRID*>( focus ) )
            enableRes = false;  // Must disable menu in order to get command as CharHook event
    }

    aEvent.Enable( enableRes );
    aEvent.Show( showRes );

    if( aEvent.IsCheckable() )
        aEvent.Check( checkRes );
}


void EDA_BASE_FRAME::setupUIConditions()
{
    // Setup the conditions to check a language menu item
    auto isCurrentLang =
            [] ( const SELECTION& aSel, int aLangIdentifier )
            {
                return Pgm().GetSelectedLanguageIdentifier() == aLangIdentifier;
            };

    for( unsigned ii = 0;  LanguagesList[ii].m_KI_Lang_Identifier != 0; ii++ )
    {
        ACTION_CONDITIONS cond;
        cond.Check( std::bind( isCurrentLang, std::placeholders::_1,
                               LanguagesList[ii].m_WX_Lang_Identifier ) );
        RegisterUIUpdateHandler( LanguagesList[ii].m_KI_Lang_Identifier, cond );
    }
}


void EDA_BASE_FRAME::RegisterCustomToolbarControlFactory( const ACTION_TOOLBAR_CONTROL& aControlDesc,
                                                          const ACTION_TOOLBAR_CONTROL_FACTORY& aControlFactory )
{
    m_toolbarControlFactories.emplace( aControlDesc.GetName(), aControlFactory );
}


ACTION_TOOLBAR_CONTROL_FACTORY* EDA_BASE_FRAME::GetCustomToolbarControlFactory( const std::string& aName )
{
    for( auto& control : m_toolbarControlFactories )
    {
        if( control.first == aName )
            return &control.second;
    }

    return nullptr;
}


void EDA_BASE_FRAME::configureToolbars()
{
}


void EDA_BASE_FRAME::RecreateToolbars()
{
    wxWindowUpdateLocker dummy( this );

    wxASSERT( m_toolbarSettings );

    std::optional<TOOLBAR_CONFIGURATION> tbConfig;

    // Drawing tools (typically on right edge of window)
    tbConfig = m_toolbarSettings->GetToolbarConfig( TOOLBAR_LOC::RIGHT, config()->m_CustomToolbars );

    if( tbConfig.has_value() )
    {
        if( !m_tbRight )
        {
            m_tbRight =
                    new ACTION_TOOLBAR( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                        KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL | wxAUI_TB_TEXT | wxAUI_TB_OVERFLOW );
            m_tbRight->SetAuiManager( &m_auimgr );
        }

        m_tbRight->ApplyConfiguration( tbConfig.value() );
    }

    // Options (typically on left edge of window)
    tbConfig = m_toolbarSettings->GetToolbarConfig( TOOLBAR_LOC::LEFT, config()->m_CustomToolbars );

    if( tbConfig.has_value() )
    {
        if( !m_tbLeft )
        {
            m_tbLeft = new ACTION_TOOLBAR( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                           KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL | wxAUI_TB_TEXT | wxAUI_TB_OVERFLOW );
            m_tbLeft->SetAuiManager( &m_auimgr );
        }

        m_tbLeft->ApplyConfiguration( tbConfig.value() );
    }

    // Top main toolbar (the top one)
    tbConfig = m_toolbarSettings->GetToolbarConfig( TOOLBAR_LOC::TOP_MAIN, config()->m_CustomToolbars );

    if( tbConfig.has_value() )
    {
        if( !m_tbTopMain )
        {
            m_tbTopMain = new ACTION_TOOLBAR( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                              KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT | wxAUI_TB_HORIZONTAL
                                                      | wxAUI_TB_TEXT | wxAUI_TB_OVERFLOW );
            m_tbTopMain->SetAuiManager( &m_auimgr );
        }

        m_tbTopMain->ApplyConfiguration( tbConfig.value() );
    }

    // Top aux toolbar (the bottom one)
    tbConfig = m_toolbarSettings->GetToolbarConfig( TOOLBAR_LOC::TOP_AUX, config()->m_CustomToolbars );

    if( tbConfig.has_value() )
    {
        if( !m_tbTopAux )
        {
            m_tbTopAux = new ACTION_TOOLBAR( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                             KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT | wxAUI_TB_HORIZONTAL
                                                     | wxAUI_TB_TEXT | wxAUI_TB_OVERFLOW );
            m_tbTopAux->SetAuiManager( &m_auimgr );
        }

        m_tbTopAux->ApplyConfiguration( tbConfig.value() );
    }
}


void EDA_BASE_FRAME::UpdateToolbarControlSizes()
{
    if( m_tbTopMain )
        m_tbTopMain->UpdateControlWidths();

    if( m_tbRight )
        m_tbRight->UpdateControlWidths();

    if( m_tbLeft )
        m_tbLeft->UpdateControlWidths();

    if( m_tbTopAux )
        m_tbTopAux->UpdateControlWidths();

}


void EDA_BASE_FRAME::OnToolbarSizeChanged()
{
    if( m_tbTopMain )
        m_auimgr.GetPane( m_tbTopMain ).MaxSize( m_tbTopMain->GetSize() );

    if( m_tbRight )
        m_auimgr.GetPane( m_tbRight ).MaxSize( m_tbRight->GetSize() );

    if( m_tbLeft )
        m_auimgr.GetPane( m_tbLeft ).MaxSize( m_tbLeft->GetSize() );

    if( m_tbTopAux )
        m_auimgr.GetPane( m_tbTopAux ).MaxSize( m_tbTopAux->GetSize() );

    m_auimgr.Update();
}


void EDA_BASE_FRAME::ReCreateMenuBar()
{
    /**
     * As of wxWidgets 3.2, recreating the menubar from within an event handler of that menubar
     * will result in memory corruption on macOS.  In order to minimize the chance of programmer
     * error causing regressions here, we always wrap calls to ReCreateMenuBar in a CallAfter to
     * ensure that they do not occur within the same event handling call stack.
     */

    CallAfter( [this]()
               {
                   if( !m_isClosing )
                       doReCreateMenuBar();
               } );
}


void EDA_BASE_FRAME::AddStandardHelpMenu( wxMenuBar* aMenuBar )
{
    COMMON_CONTROL* commonControl = m_toolManager->GetTool<COMMON_CONTROL>();
    ACTION_MENU*    helpMenu = new ACTION_MENU( false, commonControl );

    helpMenu->Add( ACTIONS::help );
    helpMenu->Add( ACTIONS::gettingStarted );
    helpMenu->Add( ACTIONS::listHotKeys );
    helpMenu->Add( ACTIONS::getInvolved );
    helpMenu->Add( ACTIONS::donate );
    helpMenu->Add( ACTIONS::reportBug );

    helpMenu->AppendSeparator();
    helpMenu->Add( ACTIONS::about );

    aMenuBar->Append( helpMenu, _( "&Help" ) );
}



wxString EDA_BASE_FRAME::GetRunMenuCommandDescription( const TOOL_ACTION& aAction )
{
   wxString   menuItemLabel = aAction.GetMenuLabel();
   wxMenuBar* menuBar = GetMenuBar();

   for( size_t ii = 0; ii < menuBar->GetMenuCount(); ++ii )
   {
       for( wxMenuItem* menuItem : menuBar->GetMenu( ii )->GetMenuItems() )
       {
           if( menuItem->GetItemLabelText() == menuItemLabel )
           {
               wxString menuTitleLabel = menuBar->GetMenuLabelText( ii );

               menuTitleLabel.Replace( wxS( "&" ), wxS( "&&" ) );
               menuItemLabel.Replace( wxS( "&" ), wxS( "&&" ) );

               return wxString::Format( _( "Run: %s > %s" ),
                                        menuTitleLabel,
                                        menuItemLabel );
           }
       }
   }

   return wxString::Format( _( "Run: %s" ), aAction.GetFriendlyName() );
};


void EDA_BASE_FRAME::ShowChangedLanguage()
{
    TOOLS_HOLDER::ShowChangedLanguage();

    if( GetMenuBar() )
    {
        ReCreateMenuBar();
        GetMenuBar()->Refresh();
    }
}


void EDA_BASE_FRAME::CommonSettingsChanged( int aFlags )
{
    TOOLS_HOLDER::CommonSettingsChanged( aFlags );

    COMMON_SETTINGS* settings = Pgm().GetCommonSettings();

#ifdef KICAD_IPC_API
    bool running = Pgm().GetApiServer().Running();

    if( running && !settings->m_Api.enable_server )
        Pgm().GetApiServer().Stop();
    else if( !running && settings->m_Api.enable_server )
        Pgm().GetApiServer().Start();
#endif

    if( m_fileHistory )
    {
        int historySize = settings->m_System.file_history_size;
        m_fileHistory->SetMaxFiles( (unsigned) std::max( 0, historySize ) );
    }

    if( Pgm().GetCommonSettings()->m_Backup.enabled )
        Kiway().LocalHistory().Init( Prj().GetProjectPath() );

    GetBitmapStore()->ThemeChanged();
    ThemeChanged();

    if( GetMenuBar() )
    {
        // For icons in menus, icon scaling & hotkeys
        ReCreateMenuBar();
        GetMenuBar()->Refresh();
    }

    // Update the toolbars
    RecreateToolbars();
}


void EDA_BASE_FRAME::ThemeChanged()
{
    ClearScaledBitmapCache();

    // Update all the toolbars to have new icons
    wxAuiPaneInfoArray panes = m_auimgr.GetAllPanes();

    for( size_t i = 0; i < panes.GetCount(); ++i )
    {
        if( ACTION_TOOLBAR* toolbar = dynamic_cast<ACTION_TOOLBAR*>( panes[i].window ) )
            toolbar->RefreshBitmaps();
    }
}


void EDA_BASE_FRAME::OnSize( wxSizeEvent& aEvent )
{
#ifdef __WXMAC__
    int currentDisplay = wxDisplay::GetFromWindow( this );

    if( m_displayIndex >= 0 && currentDisplay >= 0 && currentDisplay != m_displayIndex )
    {
        wxLogTrace( traceDisplayLocation, wxS( "OnSize: current display changed %d to %d" ),
                    m_displayIndex, currentDisplay );
        m_displayIndex = currentDisplay;
        ensureWindowIsOnScreen();
    }
#endif

    aEvent.Skip();
}


void EDA_BASE_FRAME::LoadWindowState( const wxString& aFileName )
{
    if( !Pgm().GetCommonSettings()->m_Session.remember_open_files )
        return;

    const PROJECT_FILE_STATE* state = Prj().GetLocalSettings().GetFileState( aFileName );

    if( state != nullptr )
    {
        LoadWindowState( state->window );
    }
}


void EDA_BASE_FRAME::LoadWindowState( const WINDOW_STATE& aState )
{
    bool wasDefault = false;

    m_framePos.x  = aState.pos_x;
    m_framePos.y  = aState.pos_y;
    m_frameSize.x = aState.size_x;
    m_frameSize.y = aState.size_y;

    wxLogTrace( traceDisplayLocation, wxS( "Config position (%d, %d) with size (%d, %d)" ),
                m_framePos.x, m_framePos.y, m_frameSize.x, m_frameSize.y );

    // Ensure minimum size is set if the stored config was zero-initialized
    wxSize minSize = minSizeLookup( m_ident, this );

    if( m_frameSize.x < minSize.x || m_frameSize.y < minSize.y )
    {
        m_frameSize = defaultSize( m_ident, this );
        wasDefault  = true;

        wxLogTrace( traceDisplayLocation, wxS( "Using minimum size (%d, %d)" ),
                    m_frameSize.x, m_frameSize.y );
    }

    wxLogTrace( traceDisplayLocation, wxS( "Number of displays: %d" ), wxDisplay::GetCount() );

    if( aState.display >= wxDisplay::GetCount() )
    {
        wxLogTrace( traceDisplayLocation, wxS( "Previous display not found" ) );

        // If it isn't attached, use the first display
        // Warning wxDisplay has 2 ctor variants. the parameter needs a type:
        const unsigned int index = 0;
        wxDisplay display( index );
        wxRect    clientSize = display.GetGeometry();

        m_framePos = wxDefaultPosition;

        // Ensure the window fits on the display, since the other one could have been larger
        if( m_frameSize.x > clientSize.width )
            m_frameSize.x = clientSize.width;

        if( m_frameSize.y > clientSize.height )
            m_frameSize.y = clientSize.height;
    }
    else
    {
        wxPoint upperRight( m_framePos.x + m_frameSize.x, m_framePos.y );
        wxPoint upperLeft( m_framePos.x, m_framePos.y );

        wxDisplay display( aState.display );
        wxRect clientSize = display.GetClientArea();

        int yLimTop   = clientSize.y;
        int yLimBottom = clientSize.y + clientSize.height;
        int xLimLeft  = clientSize.x;
        int xLimRight = clientSize.x + clientSize.width;

        if( upperLeft.x  > xLimRight ||  // Upper left corner too close to right edge of screen
            upperRight.x < xLimLeft  ||  // Upper right corner too close to left edge of screen
            upperLeft.y < yLimTop ||   // Upper corner too close to the bottom of the screen
            upperLeft.y > yLimBottom )
        {
            m_framePos = wxDefaultPosition;
            wxLogTrace( traceDisplayLocation, wxS( "Resetting to default position" ) );
        }
    }

    wxLogTrace( traceDisplayLocation, wxS( "Final window position (%d, %d) with size (%d, %d)" ),
                m_framePos.x, m_framePos.y, m_frameSize.x, m_frameSize.y );

    SetSize( m_framePos.x, m_framePos.y, m_frameSize.x, m_frameSize.y );

    // Center the window if we reset to default
    if( m_framePos.x == -1 )
    {
        wxLogTrace( traceDisplayLocation, wxS( "Centering window" ) );
        Center();
        m_framePos = GetPosition();
    }

    // Record the frame sizes in an un-maximized state
    m_normalFrameSize = m_frameSize;
    m_normalFramePos  = m_framePos;

    // Maximize if we were maximized before
    if( aState.maximized || ( wasDefault && m_maximizeByDefault ) )
    {
        wxLogTrace( traceDisplayLocation, wxS( "Maximizing window" ) );
        Maximize();
    }

    m_displayIndex = wxDisplay::GetFromWindow( this );
}


void EDA_BASE_FRAME::ensureWindowIsOnScreen()
{
    wxDisplay display( wxDisplay::GetFromWindow( this ) );
    wxRect    clientSize = display.GetClientArea();
    wxPoint   pos        = GetPosition();
    wxSize    size       = GetWindowSize();

    wxLogTrace( traceDisplayLocation,
                wxS( "ensureWindowIsOnScreen: clientArea (%d, %d) w %d h %d" ),
                clientSize.x, clientSize.y,
                clientSize.width, clientSize.height );

    if( pos.y < clientSize.y )
    {
        wxLogTrace( traceDisplayLocation,
                    wxS( "ensureWindowIsOnScreen: y pos %d below minimum, setting to %d" ), pos.y,
                    clientSize.y );
        pos.y = clientSize.y;
    }

    if( pos.x < clientSize.x )
    {
        wxLogTrace( traceDisplayLocation,
                    wxS( "ensureWindowIsOnScreen: x pos %d is off the client rect, setting to %d" ),
                    pos.x, clientSize.x );
        pos.x = clientSize.x;
    }

    if( pos.x + size.x - clientSize.x > clientSize.width )
    {
        int newWidth = clientSize.width - ( pos.x - clientSize.x );
        wxLogTrace( traceDisplayLocation,
                    wxS( "ensureWindowIsOnScreen: effective width %d above available %d, setting "
                         "to %d" ), pos.x + size.x, clientSize.width, newWidth );
        size.x = newWidth;
    }

    if( pos.y + size.y - clientSize.y > clientSize.height )
    {
        int newHeight = clientSize.height - ( pos.y - clientSize.y );
        wxLogTrace( traceDisplayLocation,
                    wxS( "ensureWindowIsOnScreen: effective height %d above available %d, setting "
                         "to %d" ), pos.y + size.y, clientSize.height, newHeight );
        size.y = newHeight;
    }

    wxLogTrace( traceDisplayLocation, wxS( "Updating window position (%d, %d) with size (%d, %d)" ),
                pos.x, pos.y, size.x, size.y );

    SetSize( pos.x, pos.y, size.x, size.y );
}


void EDA_BASE_FRAME::LoadWindowSettings( const WINDOW_SETTINGS* aCfg )
{
    LoadWindowState( aCfg->state );

    m_perspective = aCfg->perspective;
    m_auiLayoutState = aCfg->aui_state;
    m_mruPath = aCfg->mru_path;

    TOOLS_HOLDER::CommonSettingsChanged();
}


void EDA_BASE_FRAME::SaveWindowSettings( WINDOW_SETTINGS* aCfg )
{
    if( IsIconized() )
        return;

    // If the window is maximized, we use the saved window size from before it was maximized
    if( IsMaximized() )
    {
        m_framePos  = m_normalFramePos;
        m_frameSize = m_normalFrameSize;
    }
    else
    {
        m_frameSize = GetWindowSize();
        m_framePos  = GetPosition();
    }

    aCfg->state.pos_x     = m_framePos.x;
    aCfg->state.pos_y     = m_framePos.y;
    aCfg->state.size_x    = m_frameSize.x;
    aCfg->state.size_y    = m_frameSize.y;
    aCfg->state.maximized = IsMaximized();
    aCfg->state.display   = wxDisplay::GetFromWindow( this );

    wxLogTrace( traceDisplayLocation, wxS( "Saving window maximized: %s" ),
                IsMaximized() ? wxS( "true" ) : wxS( "false" ) );
    wxLogTrace( traceDisplayLocation, wxS( "Saving config position (%d, %d) with size (%d, %d)" ),
                m_framePos.x, m_framePos.y, m_frameSize.x, m_frameSize.y );

    // Once this is fully implemented, wxAuiManager will be used to maintain
    // the persistence of the main frame and all it's managed windows and
    // all of the legacy frame persistence position code can be removed.
#if wxCHECK_VERSION( 3, 3, 0 )
    {
        WX_AUI_JSON_SERIALIZER serializer( m_auimgr );
        nlohmann::json state = serializer.Serialize();

        if( state.is_null() || state.empty() )
            aCfg->aui_state = nlohmann::json();
        else
            aCfg->aui_state = state;

        aCfg->perspective.clear();
    }
#else
    aCfg->perspective = m_auimgr.SavePerspective().ToStdString();
    aCfg->aui_state = nlohmann::json();
#endif

    aCfg->mru_path = m_mruPath;
}


void EDA_BASE_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    LoadWindowSettings( GetWindowSettings( aCfg ) );

    // Get file history size from common settings
    int fileHistorySize = Pgm().GetCommonSettings()->m_System.file_history_size;

    // Load the recently used files into the history menu
    m_fileHistory = new FILE_HISTORY( (unsigned) std::max( 1, fileHistorySize ),
                                      ID_FILE1, ID_FILE_LIST_CLEAR );
    m_fileHistory->Load( *aCfg );
}


void EDA_BASE_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    wxCHECK( config(), /* void */ );

    SaveWindowSettings( GetWindowSettings( aCfg ) );

    bool fileOpen = m_isClosing && m_isNonUserClose;

    wxString currentlyOpenedFile = GetCurrentFileName();

    if( Pgm().GetCommonSettings()->m_Session.remember_open_files && !currentlyOpenedFile.IsEmpty() )
    {
        wxFileName rfn( currentlyOpenedFile );
        rfn.MakeRelativeTo( Prj().GetProjectPath() );
        Prj().GetLocalSettings().SaveFileState( rfn.GetFullPath(), &aCfg->m_Window, fileOpen );
    }

    // Save the recently used files list
    if( m_fileHistory )
    {
        // Save the currently opened file in the file history
        if( !currentlyOpenedFile.IsEmpty() )
            UpdateFileHistory( currentlyOpenedFile );

        m_fileHistory->Save( *aCfg );
    }
}


WINDOW_SETTINGS* EDA_BASE_FRAME::GetWindowSettings( APP_SETTINGS_BASE* aCfg )
{
    return &aCfg->m_Window;
}


APP_SETTINGS_BASE* EDA_BASE_FRAME::config() const
{
    // KICAD_MANAGER_FRAME overrides this
    return Kiface().KifaceSettings();
}


const SEARCH_STACK& EDA_BASE_FRAME::sys_search()
{
    return Kiface().KifaceSearch();
}


wxString EDA_BASE_FRAME::help_name()
{
    return Kiface().GetHelpFileName();
}


void EDA_BASE_FRAME::PrintMsg( const wxString& text )
{
    SetStatusText( text );
}


void EDA_BASE_FRAME::CreateInfoBar()
{
#if defined( __WXOSX_MAC__ )
    m_infoBar = new WX_INFOBAR( GetToolCanvas() );
#else
    m_infoBar = new WX_INFOBAR( this, &m_auimgr );

    m_auimgr.AddPane( m_infoBar, EDA_PANE().InfoBar().Name( wxS( "InfoBar" ) ).Top().Layer(1) );
#endif
}


void EDA_BASE_FRAME::FinishAUIInitialization()
{
#if defined( __WXOSX_MAC__ )
    m_auimgr.Update();
#else
    // Call Update() to fix all pane default sizes, especially the "InfoBar" pane before
    // hiding it.
    m_auimgr.Update();

    // We don't want the infobar displayed right away
    m_auimgr.GetPane( wxS( "InfoBar" ) ).Hide();
    m_auimgr.Update();
#endif
}


void EDA_BASE_FRAME::RestoreAuiLayout()
{
    if( !ADVANCED_CFG::GetCfg().m_EnableUseAuiPerspective )
        return;

#if wxCHECK_VERSION( 3, 3, 0 )
    bool restored = false;

    if( !m_auiLayoutState.is_null() && !m_auiLayoutState.empty() )
    {
        WX_AUI_JSON_SERIALIZER serializer( m_auimgr );

        if( serializer.Deserialize( m_auiLayoutState ) )
            restored = true;
    }

    if( !restored && !m_perspective.IsEmpty() )
        m_auimgr.LoadPerspective( m_perspective );
#else
    if( !m_perspective.IsEmpty() )
    {
        m_auimgr.LoadPerspective( m_perspective );

        // Workaround for wx 3.2: LoadPerspective() hides all panes first, then shows only
        // those in the saved string. If toolbar names changed or new toolbars were added,
        // they'd stay hidden. Ensure all toolbars are visible after restore.
        wxAuiPaneInfoArray& panes = m_auimgr.GetAllPanes();

        for( size_t i = 0; i < panes.GetCount(); ++i )
        {
            if( panes.Item( i ).IsToolbar() )
                panes.Item( i ).Show( true );
        }
    }
#endif
}


void EDA_BASE_FRAME::ShowInfoBarError( const wxString& aErrorMsg, bool aShowCloseButton,
                                       WX_INFOBAR::MESSAGE_TYPE aType )
{
    m_infoBar->RemoveAllButtons();

    if( aShowCloseButton )
        m_infoBar->AddCloseButton();

    GetInfoBar()->ShowMessageFor( aErrorMsg, 8000, wxICON_ERROR, aType );
}


void EDA_BASE_FRAME::ShowInfoBarError( const wxString& aErrorMsg, bool aShowCloseButton,
                                       std::function<void(void)> aCallback )
{
    m_infoBar->RemoveAllButtons();

    if( aShowCloseButton )
        m_infoBar->AddCloseButton();

    if( aCallback )
        m_infoBar->SetCallback( aCallback );

    GetInfoBar()->ShowMessageFor( aErrorMsg, 6000, wxICON_ERROR );
}


void EDA_BASE_FRAME::ShowInfoBarWarning( const wxString& aWarningMsg, bool aShowCloseButton )
{
    m_infoBar->RemoveAllButtons();

    if( aShowCloseButton )
        m_infoBar->AddCloseButton();

    GetInfoBar()->ShowMessageFor( aWarningMsg, 6000, wxICON_WARNING );
}


void EDA_BASE_FRAME::ShowInfoBarMsg( const wxString& aMsg, bool aShowCloseButton )
{
    m_infoBar->RemoveAllButtons();

    if( aShowCloseButton )
        m_infoBar->AddCloseButton();

    GetInfoBar()->ShowMessageFor( aMsg, 8000, wxICON_INFORMATION );
}


void EDA_BASE_FRAME::UpdateFileHistory( const wxString& FullFileName, FILE_HISTORY* aFileHistory )
{
    if( !aFileHistory )
        aFileHistory = m_fileHistory;

    wxASSERT( aFileHistory );

    aFileHistory->AddFileToHistory( FullFileName );

    // Update the menubar to update the file history menu
    if( !m_isClosing && GetMenuBar() )
    {
        ReCreateMenuBar();
        GetMenuBar()->Refresh();
    }
}


wxString EDA_BASE_FRAME::GetFileFromHistory( int cmdId, const wxString& type, FILE_HISTORY* aFileHistory )
{
    if( !aFileHistory )
        aFileHistory = m_fileHistory;

    wxASSERT( aFileHistory );

    int baseId = aFileHistory->GetBaseId();

    wxASSERT( cmdId >= baseId && cmdId < baseId + (int) aFileHistory->GetCount() );
    int i = cmdId - baseId;

    wxString fn = aFileHistory->GetHistoryFile( i );

    if( !wxFileName::FileExists( fn ) )
    {
        wxMessageDialog dlg( this, wxString::Format( _( "File '%s' was not found.\n" ), fn ), _( "Error" ),
                             wxYES_NO | wxYES_DEFAULT | wxICON_ERROR | wxCENTER );

        dlg.SetExtendedMessage( _( "Do you want to remove it from list of recently opened files?" ) );
        dlg.SetYesNoLabels( wxMessageDialog::ButtonLabel( _( "Remove" ) ),
                            wxMessageDialog::ButtonLabel( _( "Keep" ) ) );

        if( dlg.ShowModal() == wxID_YES )
            aFileHistory->RemoveFileFromHistory( i );

        fn.Clear();
    }

    // Update the menubar to update the file history menu
    if( GetMenuBar() )
    {
        ReCreateMenuBar();
        GetMenuBar()->Refresh();
    }

    return fn;
}


void EDA_BASE_FRAME::ClearFileHistory()
{
    wxASSERT( m_fileHistory );

    m_fileHistory->ClearFileHistory();

    // Update the menubar to update the file history menu
    if( GetMenuBar() )
    {
        ReCreateMenuBar();
        GetMenuBar()->Refresh();
    }
}


void EDA_BASE_FRAME::OnKicadAbout( wxCommandEvent& event )
{
    void ShowAboutDialog( EDA_BASE_FRAME * aParent ); // See AboutDialog_main.cpp
    ShowAboutDialog( this );
}


void EDA_BASE_FRAME::OnPreferences( wxCommandEvent& event )
{
    ShowPreferences( wxEmptyString, wxEmptyString );
}


void EDA_BASE_FRAME::ShowPreferences( wxString aStartPage, wxString aStartParentPage )
{
    PAGED_DIALOG dlg( this, _( "Preferences" ), true, true, wxEmptyString,
                      wxWindow::FromDIP( wxSize( 980, 560 ), nullptr ) );

    dlg.SetEvtHandlerEnabled( false );

    {
        WX_BUSY_INDICATOR busy_cursor;

        WX_TREEBOOK*            book = dlg.GetTreebook();
        PANEL_HOTKEYS_EDITOR*   hotkeysPanel = new PANEL_HOTKEYS_EDITOR( this, book );
        std::vector<int>        expand;

        wxWindow* kicadMgr_window = wxWindow::FindWindowByName( KICAD_MANAGER_FRAME_NAME );

        if( KICAD_MANAGER_FRAME* kicadMgr = static_cast<KICAD_MANAGER_FRAME*>( kicadMgr_window ) )
        {
            ACTION_MANAGER* actionMgr = kicadMgr->GetToolManager()->GetActionManager();

            for( const auto& [name, action] : actionMgr->GetActions() )
                hotkeysPanel->ActionsList().push_back( action );
        }

        book->AddLazyPage(
                []( wxWindow* aParent ) -> wxWindow*
                {
                    return new PANEL_COMMON_SETTINGS( aParent );
                },
                _( "Common" ) );

        book->AddLazyPage(
                []( wxWindow* aParent ) -> wxWindow*
                {
                    return new PANEL_MOUSE_SETTINGS( aParent );
                }, _( "Mouse and Touchpad" ) );

        book->AddLazyPage(
                [] ( wxWindow* aParent ) -> wxWindow*
                {
                    return new PANEL_SPACEMOUSE( aParent );
                }, _( "SpaceMouse" ) );

        book->AddPage( hotkeysPanel, _( "Hotkeys" ) );

        book->AddLazyPage(
                []( wxWindow* aParent ) -> wxWindow*
                {
                    return new PANEL_GIT_REPOS( aParent );
                }, _( "Version Control" ) );

#ifdef KICAD_USE_SENTRY
        book->AddLazyPage(
                []( wxWindow* aParent ) -> wxWindow*
                {
                    return new PANEL_DATA_COLLECTION( aParent );
                }, _( "Data Collection" ) );
#endif

#define LAZY_CTOR( key )                                                \
        [this, kiface]( wxWindow* aParent )                             \
        {                                                               \
            return kiface->CreateKiWindow( aParent, key, &Kiway() );    \
        }

        // If a dll is not loaded, the loader will show an error message.

        try
        {
            if( KIFACE* kiface = Kiway().KiFACE( KIWAY::FACE_SCH ) )
            {
                kiface->GetActions( hotkeysPanel->ActionsList() );

                if( GetFrameType() == FRAME_SCH_SYMBOL_EDITOR )
                    expand.push_back( (int) book->GetPageCount() );

                book->AddPage( new wxPanel( book ), _( "Symbol Editor" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_SYM_DISP_OPTIONS ), _( "Display Options" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_SYM_EDIT_GRIDS ), _( "Grids" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_SYM_EDIT_OPTIONS ), _( "Editing Options" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_SYM_COLORS ), _( "Colors" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_SYM_TOOLBARS ), _( "Toolbars" ) );

                if( GetFrameType() == FRAME_SCH )
                    expand.push_back( (int) book->GetPageCount() );

                book->AddPage( new wxPanel( book ), _( "Schematic Editor" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_SCH_DISP_OPTIONS ), _( "Display Options" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_SCH_GRIDS ), _( "Grids" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_SCH_EDIT_OPTIONS ), _( "Editing Options" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_SCH_COLORS ), _( "Colors" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_SCH_TOOLBARS ), _( "Toolbars" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_SCH_FIELD_NAME_TEMPLATES ), _( "Field Name Templates" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_SCH_DATA_SOURCES ), _( "Data Sources" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_SCH_SIMULATOR ), _( "Simulator" ) );
            }
        }
        catch( ... )
        {
        }

        try
        {
            if( KIFACE* kiface = Kiway().KiFACE( KIWAY::FACE_PCB ) )
            {
                kiface->GetActions( hotkeysPanel->ActionsList() );

                if( GetFrameType() == FRAME_FOOTPRINT_EDITOR )
                    expand.push_back( (int) book->GetPageCount() );

                book->AddPage( new wxPanel( book ), _( "Footprint Editor" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_FP_DISPLAY_OPTIONS ), _( "Display Options" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_FP_GRIDS ), _( "Grids" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_FP_ORIGINS_AXES ), _( "Origins & Axes" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_FP_EDIT_OPTIONS ), _( "Editing Options" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_FP_COLORS ), _( "Colors" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_FP_TOOLBARS ), _( "Toolbars" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_FP_DEFAULT_FIELDS ), _( "Footprint Defaults" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_FP_DEFAULT_GRAPHICS_VALUES ), _( "Graphics Defaults" ) );

                if( GetFrameType() ==  FRAME_PCB_EDITOR )
                    expand.push_back( (int) book->GetPageCount() );

                book->AddPage( new wxPanel( book ), _( "PCB Editor" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_PCB_DISPLAY_OPTS ), _( "Display Options" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_PCB_GRIDS ), _( "Grids" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_PCB_ORIGINS_AXES ), _( "Origins & Axes" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_PCB_EDIT_OPTIONS ), _( "Editing Options" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_PCB_COLORS ), _( "Colors" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_PCB_TOOLBARS ), _( "Toolbars" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_PCB_ACTION_PLUGINS ), _( "Plugins" ) );

                if( GetFrameType() == FRAME_PCB_DISPLAY3D )
                    expand.push_back( (int) book->GetPageCount() );

                book->AddPage( new wxPanel( book ), _( "3D Viewer" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_3DV_DISPLAY_OPTIONS ), _( "General" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_3DV_TOOLBARS ), _( "Toolbars" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_3DV_OPENGL ), _( "Realtime Renderer" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_3DV_RAYTRACING ), _( "Raytracing Renderer" ) );
            }
        }
        catch( ... )
        {
        }

        try
        {
            if( KIFACE* kiface = Kiway().KiFACE( KIWAY::FACE_GERBVIEW ) )
            {
                kiface->GetActions( hotkeysPanel->ActionsList() );

                if( GetFrameType() == FRAME_GERBER )
                    expand.push_back( (int) book->GetPageCount() );

                book->AddPage( new wxPanel( book ), _( "Gerber Viewer" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_GBR_DISPLAY_OPTIONS ), _( "Display Options" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_GBR_COLORS ), _( "Colors" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_GBR_TOOLBARS ), _( "Toolbars" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_GBR_GRIDS ), _( "Grids" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_GBR_EXCELLON_OPTIONS ), _( "Excellon Options" ) );
            }
        }
        catch( ... )
        {
        }

        try
        {
            if( KIFACE* kiface = Kiway().KiFACE( KIWAY::FACE_PL_EDITOR ) )
            {
                kiface->GetActions( hotkeysPanel->ActionsList() );

                if( GetFrameType() == FRAME_PL_EDITOR )
                    expand.push_back( (int) book->GetPageCount() );

                book->AddPage( new wxPanel( book ), _( "Drawing Sheet Editor" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_DS_DISPLAY_OPTIONS ), _( "Display Options" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_DS_GRIDS ), _( "Grids" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_DS_COLORS ), _( "Colors" ) );
                book->AddLazySubPage( LAZY_CTOR( PANEL_DS_TOOLBARS ), _( "Toolbars" ) );

                book->AddLazyPage(
                        []( wxWindow* aParent ) -> wxWindow*
                        {
                            return new PANEL_PACKAGES_AND_UPDATES( aParent );
                        }, _( "Packages and Updates" ) );
            }
        }
        catch( ... )
        {
        }

#ifdef KICAD_IPC_API
        book->AddPage( new PANEL_PLUGIN_SETTINGS( book ), _( "Plugins" ) );
#endif

        book->AddPage( new PANEL_MAINTENANCE( book, this ), _( "Maintenance" ) );

        // Update all of the action hotkeys. The process of loading the actions through
        // the KiFACE will only get us the default hotkeys
        ReadHotKeyConfigIntoActions( wxEmptyString, hotkeysPanel->ActionsList() );

        for( size_t i = 0; i < book->GetPageCount(); ++i )
            book->GetPage( i )->Layout();

        for( int page : expand )
            book->ExpandNode( page );

        if( !aStartPage.IsEmpty() )
            dlg.SetInitialPage( aStartPage, aStartParentPage );

        dlg.SetEvtHandlerEnabled( true );
#undef LAZY_CTOR
    }

    if( dlg.ShowModal() == wxID_OK )
    {
        // Update our grids that are cached in the tool
        m_toolManager->ResetTools( TOOL_BASE::REDRAW );
        Pgm().GetSettingsManager().Save();
        dlg.Kiway().CommonSettingsChanged( HOTKEYS_CHANGED );
    }

}


void EDA_BASE_FRAME::OnDropFiles( wxDropFilesEvent& aEvent )
{
    Raise();

    wxString* files = aEvent.GetFiles();

    for( int nb = 0; nb < aEvent.GetNumberOfFiles(); nb++ )
    {
        const wxFileName fn = wxFileName( files[nb] );
        wxString         ext = fn.GetExt();

        // Alias all gerber files as GerberFileExtension
        if( FILEEXT::IsGerberFileExtension( ext ) )
            ext = FILEEXT::GerberFileExtension;

        if( m_acceptedExts.find( ext.ToStdString() ) != m_acceptedExts.end() )
            m_AcceptedFiles.emplace_back( fn );
    }

    DoWithAcceptedFiles();
    m_AcceptedFiles.clear();
}


void EDA_BASE_FRAME::DoWithAcceptedFiles()
{
    for( const wxFileName& file : m_AcceptedFiles )
    {
        wxString fn = file.GetFullPath();
        m_toolManager->RunAction<wxString*>( *m_acceptedExts.at( file.GetExt() ), &fn );
    }
}


bool EDA_BASE_FRAME::IsWritable( const wxFileName& aFileName, bool aVerbose )
{
    wxString msg;
    wxFileName fn = aFileName;

    // Check for absence of a file path with a file name.  Unfortunately KiCad
    // uses paths relative to the current project path without the ./ part which
    // confuses wxFileName. Making the file name path absolute may be less than
    // elegant but it solves the problem.
    if( fn.GetPath().IsEmpty() && fn.HasName() )
        fn.MakeAbsolute();

    wxCHECK_MSG( fn.IsOk(), false,
                 wxT( "File name object is invalid.  Bad programmer!" ) );
    wxCHECK_MSG( !fn.GetPath().IsEmpty(), false,
                 wxT( "File name object path <" ) + fn.GetFullPath() +
                 wxT( "> is not set.  Bad programmer!" ) );

    if( fn.IsDir() && !fn.IsDirWritable() )
    {
        msg.Printf( _( "Insufficient permissions to folder '%s'." ), fn.GetPath() );
    }
    else if( !fn.FileExists() && !fn.IsDirWritable() )
    {
        msg.Printf( _( "Insufficient permissions to save file '%s'." ), fn.GetFullPath() );
    }
    else if( fn.FileExists() && !fn.IsFileWritable() )
    {
        msg.Printf( _( "Insufficient permissions to save file '%s'." ), fn.GetFullPath() );
    }

    if( !msg.IsEmpty() )
    {
        if( aVerbose )
            DisplayErrorMessage( this, msg );

        return false;
    }

    return true;
}


bool EDA_BASE_FRAME::IsContentModified() const
{
    // This function should be overridden in child classes
    return false;
}


void EDA_BASE_FRAME::initExitKey()
{
    wxAcceleratorEntry entries[1];
    entries[0].Set( wxACCEL_CTRL, int( 'Q' ), wxID_EXIT );
    wxAcceleratorTable accel( 1, entries );
    SetAcceleratorTable( accel );
}


void EDA_BASE_FRAME::ClearUndoRedoList()
{
    ClearUndoORRedoList( UNDO_LIST );
    ClearUndoORRedoList( REDO_LIST );
}


void EDA_BASE_FRAME::PushCommandToUndoList( PICKED_ITEMS_LIST* aNewitem )
{
    m_undoList.PushCommand( aNewitem );

    // Delete the extra items, if count max reached
    if( m_undoRedoCountMax > 0 )
    {
        int extraitems = GetUndoCommandCount() - m_undoRedoCountMax;

        if( extraitems > 0 )
            ClearUndoORRedoList( UNDO_LIST, extraitems );
    }
}


void EDA_BASE_FRAME::PushCommandToRedoList( PICKED_ITEMS_LIST* aNewitem )
{
    m_redoList.PushCommand( aNewitem );

    // Delete the extra items, if count max reached
    if( m_undoRedoCountMax > 0 )
    {
        int extraitems = GetRedoCommandCount() - m_undoRedoCountMax;

        if( extraitems > 0 )
            ClearUndoORRedoList( REDO_LIST, extraitems );
    }
}


PICKED_ITEMS_LIST* EDA_BASE_FRAME::PopCommandFromUndoList( )
{
    return m_undoList.PopCommand();
}


PICKED_ITEMS_LIST* EDA_BASE_FRAME::PopCommandFromRedoList( )
{
    return m_redoList.PopCommand();
}


wxString EDA_BASE_FRAME::GetUndoActionDescription() const
{
    if( GetUndoCommandCount() > 0 )
        return m_undoList.m_CommandsList.back()->GetDescription();

    return wxEmptyString;
}


wxString EDA_BASE_FRAME::GetRedoActionDescription() const
{
    if( GetRedoCommandCount() > 0 )
        return m_redoList.m_CommandsList.back()->GetDescription();

    return wxEmptyString;
}


void EDA_BASE_FRAME::OnModify()
{
    m_autoSaveRequired = true;
}


void EDA_BASE_FRAME::ChangeUserUnits( EDA_UNITS aUnits )
{
    SetUserUnits( aUnits );
    unitsChangeRefresh();

    wxCommandEvent e( EDA_EVT_UNITS_CHANGED );
    e.SetInt( static_cast<int>( aUnits ) );
    e.SetClientData( this );
    ProcessEventLocally( e );
}


void EDA_BASE_FRAME::OnMaximize( wxMaximizeEvent& aEvent )
{
    // When we maximize the window, we want to save the old information
    // so that we can add it to the settings on next window load.
    // Contrary to the documentation, this event seems to be generated
    // when the window is also being unmaximized on OSX, so we only
    // capture the size information when we maximize the window when on OSX.
#ifdef __WXOSX__
    if( !IsMaximized() )
#endif
    {
        m_normalFrameSize = GetWindowSize();
        m_normalFramePos  = GetPosition();
        wxLogTrace( traceDisplayLocation,
                    "Maximizing window - Saving position (%d, %d) with size (%d, %d)",
                    m_normalFramePos.x, m_normalFramePos.y,
                    m_normalFrameSize.x, m_normalFrameSize.y );
    }

    // Skip event to actually maximize the window
    aEvent.Skip();
}


wxSize EDA_BASE_FRAME::GetWindowSize()
{
#ifdef __WXGTK__
    wxSize winSize = GetSize();

    // GTK includes the window decorations in the normal GetSize call,
    // so we have to use a GTK-specific sizing call that returns the
    // non-decorated window size.
    if( m_ident == KICAD_MAIN_FRAME_T )
    {
        int width  = 0;
        int height = 0;
        GTKDoGetSize( &width, &height );

        winSize.Set( width, height );
    }
#else
    wxSize winSize = GetSize();
#endif

    return winSize;
}


void EDA_BASE_FRAME::HandleSystemColorChange()
{
    // Update the icon theme when the system theme changes and update the toolbars
    GetBitmapStore()->ThemeChanged();
    ThemeChanged();

    // This isn't handled by ThemeChanged()
    if( GetMenuBar() )
    {
        // For icons in menus, icon scaling & hotkeys
        ReCreateMenuBar();
        GetMenuBar()->Refresh();
    }
}


void EDA_BASE_FRAME::onSystemColorChange( wxSysColourChangedEvent& aEvent )
{
    // Call the handler to update the colors used in the frame
    HandleSystemColorChange();

    // Skip the change event to ensure the rest of the window controls get it
    aEvent.Skip();
}


void EDA_BASE_FRAME::onIconize( wxIconizeEvent& aEvent )
{
    // Call the handler
    handleIconizeEvent( aEvent );

    // Skip the event.
    aEvent.Skip();
}


#ifdef __WXMSW__
WXLRESULT EDA_BASE_FRAME::MSWWindowProc( WXUINT message, WXWPARAM wParam, WXLPARAM lParam )
{
    // This will help avoid the menu keeping focus when the alt key is released
    // You can still trigger accelerators as long as you hold down alt
    if( message == WM_SYSCOMMAND )
    {
        if( wParam == SC_KEYMENU && ( lParam >> 16 ) <= 0 )
            return 0;
    }

    return wxFrame::MSWWindowProc( message, wParam, lParam );
}
#endif


void EDA_BASE_FRAME::AddMenuLanguageList( ACTION_MENU* aMasterMenu, TOOL_INTERACTIVE* aControlTool )
{
    ACTION_MENU* langsMenu = new ACTION_MENU( false, aControlTool );
    langsMenu->SetTitle( _( "Set Language" ) );
    langsMenu->SetIcon( BITMAPS::language );

    wxString tooltip;

    for( unsigned ii = 0; LanguagesList[ii].m_KI_Lang_Identifier != 0; ii++ )
    {
        wxString label;

        if( LanguagesList[ii].m_DoNotTranslate )
            label = LanguagesList[ii].m_Lang_Label;
        else
            label = wxGetTranslation( LanguagesList[ii].m_Lang_Label );

        wxMenuItem* item =
                new wxMenuItem( langsMenu,
                                LanguagesList[ii].m_KI_Lang_Identifier, // wxMenuItem wxID
                                label, tooltip, wxITEM_CHECK );

        langsMenu->Append( item );
    }

    // This must be done after the items are added
    aMasterMenu->Add( langsMenu );
}
