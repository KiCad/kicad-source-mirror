/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015-2022 KiCad Developers, see change_log.txt for contributors.
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

#include <advanced_config.h>
#include <base_units.h>
#include <kiway.h>
#include <lib_tree_model_adapter.h>
#include <pgm_base.h>
#include <eda_list_dialog.h>
#include <eeschema_settings.h>
#include <project/project_file.h>
#include <symbol_editor/symbol_editor_settings.h>
#include <sch_draw_panel.h>
#include <sch_view.h>
#include <sch_painter.h>
#include <settings/settings_manager.h>
#include <confirm.h>
#include <preview_items/selection_area.h>
#include <symbol_library.h>
#include <sch_base_frame.h>
#include <symbol_lib_table.h>
#include <tool/action_toolbar.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tools/ee_actions.h>
#include <tools/ee_selection_tool.h>
#include <wx/choicdlg.h>
#include <wx/log.h>

#include <navlib/nl_schematic_plugin.h>

LIB_SYMBOL* SchGetLibSymbol( const LIB_ID& aLibId, SYMBOL_LIB_TABLE* aLibTable,
                             SYMBOL_LIB* aCacheLib, wxWindow* aParent, bool aShowErrorMsg )
{
    wxCHECK_MSG( aLibTable, nullptr, wxS( "Invalid symbol library table." ) );

    LIB_SYMBOL* symbol = nullptr;

    try
    {
        symbol = aLibTable->LoadSymbol( aLibId );

        if( !symbol && aCacheLib )
        {
            wxCHECK_MSG( aCacheLib->IsCache(), nullptr, wxS( "Invalid cache library." ) );

            wxString cacheName = aLibId.GetLibNickname().wx_str();
            cacheName += "_" + aLibId.GetLibItemName();
            symbol = aCacheLib->FindSymbol( cacheName );
        }
    }
    catch( const IO_ERROR& ioe )
    {
        if( aShowErrorMsg )
        {
            wxString msg = wxString::Format( _( "Error loading symbol %s from library '%s'." ),
                                             aLibId.GetLibItemName().wx_str(),
                                             aLibId.GetLibNickname().wx_str() );
            DisplayErrorMessage( aParent, msg, ioe.What() );
        }
    }

    return symbol;
}


SCH_BASE_FRAME::SCH_BASE_FRAME( KIWAY* aKiway, wxWindow* aParent, FRAME_T aWindowType,
                                const wxString& aTitle, const wxPoint& aPosition,
                                const wxSize& aSize, long aStyle, const wxString& aFrameName ) :
        EDA_DRAW_FRAME( aKiway, aParent, aWindowType, aTitle, aPosition, aSize, aStyle,
                        aFrameName, schIUScale ),
        m_base_frame_defaults( nullptr, "base_Frame_defaults" ), m_spaceMouse( nullptr )
{
    createCanvas();

    Bind( wxEVT_IDLE,
          [this]( wxIdleEvent& aEvent )
          {
              // Handle cursor adjustments.  While we can get motion and key events through
              // wxWidgets, we can't get modifier-key-up events.
              if( m_toolManager )
              {
                  EE_SELECTION_TOOL* selTool = m_toolManager->GetTool<EE_SELECTION_TOOL>();

                  if( selTool )
                      selTool->OnIdle( aEvent );
              }
          } );

    m_watcherDebounceTimer.Bind( wxEVT_TIMER, &SCH_BASE_FRAME::OnSymChangeDebounceTimer, this );
}


SCH_BASE_FRAME::~SCH_BASE_FRAME()
{
    delete m_spaceMouse;
}


SCH_SCREEN* SCH_BASE_FRAME::GetScreen() const
{
    return (SCH_SCREEN*) EDA_DRAW_FRAME::GetScreen();
}


EESCHEMA_SETTINGS* SCH_BASE_FRAME::eeconfig() const
{
    return dynamic_cast<EESCHEMA_SETTINGS*>( config() );
}


SYMBOL_EDITOR_SETTINGS* SCH_BASE_FRAME::libeditconfig() const
{
    return dynamic_cast<SYMBOL_EDITOR_SETTINGS*>( config() );
}


void SCH_BASE_FRAME::SetPageSettings( const PAGE_INFO& aPageSettings )
{
    GetScreen()->SetPageSettings( aPageSettings );
}


const PAGE_INFO& SCH_BASE_FRAME::GetPageSettings () const
{
    return GetScreen()->GetPageSettings();
}


const VECTOR2I SCH_BASE_FRAME::GetPageSizeIU() const
{
    // GetSizeIU is compile time dependent:
    return GetScreen()->GetPageSettings().GetSizeIU( schIUScale.IU_PER_MILS );
}


const TITLE_BLOCK& SCH_BASE_FRAME::GetTitleBlock() const
{
    wxASSERT( GetScreen() );
    return GetScreen()->GetTitleBlock();
}


void SCH_BASE_FRAME::SetTitleBlock( const TITLE_BLOCK& aTitleBlock )
{
    wxASSERT( GetScreen() );
    GetScreen()->SetTitleBlock( aTitleBlock );
}


void SCH_BASE_FRAME::UpdateStatusBar()
{
    wxString        line;
    BASE_SCREEN*    screen = GetScreen();

    if( !screen )
        return;

    EDA_DRAW_FRAME::UpdateStatusBar();

    // Display absolute and relative coordinates
    VECTOR2D cursorPos = GetCanvas()->GetViewControls()->GetCursorPosition();
    VECTOR2D d         = cursorPos - screen->m_LocalOrigin;

    line.Printf( wxS( "X %s  Y %s" ),
                 MessageTextFromValue( cursorPos.x, false ),
                 MessageTextFromValue( cursorPos.y, false ) );
    SetStatusText( line, 2 );

    line.Printf( wxS( "dx %s  dy %s  dist %s" ),
                 MessageTextFromValue( d.x, false ),
                 MessageTextFromValue( d.y, false ),
                 MessageTextFromValue( hypot( d.x, d.y ), false ) );
    SetStatusText( line, 3 );

    DisplayGridMsg();
    DisplayUnitsMsg();
}


LIB_SYMBOL* SCH_BASE_FRAME::GetLibSymbol( const LIB_ID& aLibId, bool aUseCacheLib,
                                          bool aShowErrorMsg )
{
    SYMBOL_LIB* cache = ( aUseCacheLib ) ? Prj().SchLibs()->GetCacheLibrary() : nullptr;

    return SchGetLibSymbol( aLibId, Prj().SchSymbolLibTable(), cache, this, aShowErrorMsg );
}


bool SCH_BASE_FRAME::saveSymbolLibTables( bool aGlobal, bool aProject )
{
    wxString msg;
    bool success = true;

    if( aGlobal )
    {
        try
        {
            SYMBOL_LIB_TABLE::GetGlobalLibTable().Save( SYMBOL_LIB_TABLE::GetGlobalTableFileName() );
        }
        catch( const IO_ERROR& ioe )
        {
            success = false;
            msg.Printf( _( "Error saving global symbol library table:\n%s" ), ioe.What() );
            DisplayErrorMessage( this, msg );
        }
    }

    if( aProject && !Prj().GetProjectName().IsEmpty() )
    {
        wxFileName fn( Prj().GetProjectPath(), SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );

        try
        {
            Prj().SchSymbolLibTable()->Save( fn.GetFullPath() );
        }
        catch( const IO_ERROR& ioe )
        {
            success = false;
            msg.Printf( _( "Error saving project-specific symbol library table:\n%s" ),
                        ioe.What() );
            DisplayErrorMessage( this, msg );
        }
    }

    return success;
}


SYMBOL_LIB_TABLE* SCH_BASE_FRAME::SelectSymLibTable( bool aOptional )
{
    // If no project is loaded, always work with the global table
    if( Prj().IsNullProject() )
    {
        SYMBOL_LIB_TABLE* ret = &SYMBOL_LIB_TABLE::GetGlobalLibTable();

        if( aOptional )
        {
            wxMessageDialog dlg( this, _( "Add the library to the global library table?" ),
                                 _( "Add To Global Library Table" ), wxYES_NO );

            if( dlg.ShowModal() != wxID_OK )
                ret = nullptr;
        }

        return ret;
    }

    wxArrayString libTableNames;
    libTableNames.Add( _( "Global" ) );
    libTableNames.Add( _( "Project" ) );

    wxSingleChoiceDialog dlg( this, _( "Choose the Library Table to add the library to:" ),
                              _( "Add To Library Table" ), libTableNames );

    if( aOptional )
    {
        dlg.FindWindow( wxID_CANCEL )->SetLabel( _( "Skip" ) );
        dlg.FindWindow( wxID_OK )->SetLabel( _( "Add" ) );
    }

    if( dlg.ShowModal() != wxID_OK )
        return nullptr;

    switch( dlg.GetSelection() )
    {
    case 0:  return &SYMBOL_LIB_TABLE::GetGlobalLibTable();
    case 1:  return Prj().SchSymbolLibTable();
    default: return nullptr;
    }
}


void SCH_BASE_FRAME::RedrawScreen( const VECTOR2I& aCenterPoint, bool aWarpPointer )
{
    GetCanvas()->GetView()->SetCenter( aCenterPoint );

    if( aWarpPointer )
        GetCanvas()->GetViewControls()->CenterOnCursor();

    GetCanvas()->Refresh();
}


void SCH_BASE_FRAME::HardRedraw()
{
    if( GetCanvas() && GetCanvas()->GetView() )
    {
        GetCanvas()->GetView()->UpdateAllItems( KIGFX::ALL );
        GetCanvas()->ForceRefresh();
    }
}


SCH_DRAW_PANEL* SCH_BASE_FRAME::GetCanvas() const
{
    return static_cast<SCH_DRAW_PANEL*>( EDA_DRAW_FRAME::GetCanvas() );
}


KIGFX::SCH_RENDER_SETTINGS* SCH_BASE_FRAME::GetRenderSettings()
{
    if( GetCanvas() && GetCanvas()->GetView() )
    {
        if( KIGFX::PAINTER* painter = GetCanvas()->GetView()->GetPainter() )
            return static_cast<KIGFX::SCH_RENDER_SETTINGS*>( painter->GetSettings() );
    }

    return nullptr;
}


void SCH_BASE_FRAME::createCanvas()
{
    m_canvasType = loadCanvasTypeSetting();

    SetCanvas( new SCH_DRAW_PANEL( this, wxID_ANY, wxPoint( 0, 0 ), m_frameSize,
                                   GetGalDisplayOptions(), m_canvasType ) );
    ActivateGalCanvas();
}


void SCH_BASE_FRAME::ActivateGalCanvas()
{
    EDA_DRAW_FRAME::ActivateGalCanvas();

    try
    {
        if( !m_spaceMouse )
            m_spaceMouse = new NL_SCHEMATIC_PLUGIN();

        m_spaceMouse->SetCanvas( GetCanvas() );
    }
    catch( const std::system_error& e )
    {
        wxLogTrace( wxT( "KI_TRACE_NAVLIB" ), e.what() );
    }
}


void SCH_BASE_FRAME::UpdateItem( EDA_ITEM* aItem, bool isAddOrDelete, bool aUpdateRtree )
{
    EDA_ITEM* parent = aItem->GetParent();

    if( aItem->Type() == SCH_SHEET_PIN_T )
    {
        // Sheet pins aren't in the view.  Refresh their parent.
        if( parent )
            GetCanvas()->GetView()->Update( parent );
    }
    else
    {
        if( !isAddOrDelete )
            GetCanvas()->GetView()->Update( aItem );

        // Some children are drawn from their parents.  Mark them for re-paint.
        if( parent && parent->IsType( { SCH_SYMBOL_T, SCH_SHEET_T, SCH_LABEL_LOCATE_ANY_T } ) )
            GetCanvas()->GetView()->Update( parent, KIGFX::REPAINT );
    }

    /**
     * Be careful when calling this.  Update will invalidate RTree iterators, so you cannot call this
     * while doing things like `for( SCH_ITEM* item : screen->Items() )`
     */
    if( aUpdateRtree && dynamic_cast<SCH_ITEM*>( aItem ) )
        GetScreen()->Update( static_cast<SCH_ITEM*>( aItem ) );

    // Calling Refresh() here introduces a bi-stable state: when doing operations on a
    // large number of items if at some point the refresh timer times out and does a
    // refresh it will take long enough that the next item will also time out, and the
    // next, and the next, etc.
    // GetCanvas()->Refresh();
}


void SCH_BASE_FRAME::RefreshZoomDependentItems()
{
    // We currently have two zoom-dependent renderings: text, which is rendered as bitmap text
    // when too small to see the difference, and selection shadows.
    //
    // Because non-selected text is cached by OpenGL, we only apply the bitmap performance hack
    // to selected text items.
    //
    // Thus, as it currently stands, all zoom-dependent items can be found in the list of selected
    // items.

    if( m_toolManager )
    {
        EE_SELECTION_TOOL* selectionTool = m_toolManager->GetTool<EE_SELECTION_TOOL>();
        SELECTION&         selection = selectionTool->GetSelection();
        KIGFX::SCH_VIEW*   view = GetCanvas()->GetView();

        for( EDA_ITEM* item : selection )
        {
            if( item->RenderAsBitmap( view->GetGAL()->GetWorldScale() ) != item->IsShownAsBitmap()
                    || item->IsType( KIGFX::SCH_PAINTER::g_ScaledSelectionTypes ) )
            {
                view->Update( item, KIGFX::REPAINT );

                EDA_ITEM* parent = item->GetParent();

                // Symbol children are drawn from their parents.  Mark them for re-paint.
                if( parent && parent->Type() == SCH_SYMBOL_T )
                    GetCanvas()->GetView()->Update( parent, KIGFX::REPAINT );
            }
        }
    }
}


void SCH_BASE_FRAME::AddToScreen( EDA_ITEM* aItem, SCH_SCREEN* aScreen )
{
    // Null pointers will cause boost::ptr_vector to raise a boost::bad_pointer exception which
    // will be unhandled.  There is no valid reason to pass an invalid EDA_ITEM pointer to the
    // screen append function.
    wxCHECK( aItem != nullptr, /* voide */ );

    auto screen = aScreen;

    if( aScreen == nullptr )
        screen = GetScreen();

    screen->Append( (SCH_ITEM*) aItem );

    if( screen == GetScreen() )
    {
        GetCanvas()->GetView()->Add( aItem );
        UpdateItem( aItem, true );           // handle any additional parent semantics
    }
}


void SCH_BASE_FRAME::RemoveFromScreen( EDA_ITEM* aItem, SCH_SCREEN* aScreen )
{
    auto screen = aScreen;

    if( aScreen == nullptr )
        screen = GetScreen();

    if( screen == GetScreen() )
        GetCanvas()->GetView()->Remove( aItem );

    screen->Remove( (SCH_ITEM*) aItem );

    if( screen == GetScreen() )
        UpdateItem( aItem, true );           // handle any additional parent semantics
}


void SCH_BASE_FRAME::SyncView()
{
    // Let tools add things to the view if necessary
    if( m_toolManager )
        m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );

    GetCanvas()->GetView()->UpdateAllItems( KIGFX::ALL );
}


COLOR4D SCH_BASE_FRAME::GetLayerColor( SCH_LAYER_ID aLayer )
{
    return GetColorSettings()->GetColor( aLayer );
}


void SCH_BASE_FRAME::CommonSettingsChanged( bool aEnvVarsChanged, bool aTextVarsChanged )
{
    EDA_DRAW_FRAME::CommonSettingsChanged( aEnvVarsChanged, aTextVarsChanged );

    COLOR_SETTINGS* colorSettings = GetColorSettings( true );

    GetCanvas()->GetView()->GetPainter()->GetSettings()->LoadColors( colorSettings );
    GetCanvas()->GetGAL()->SetAxesColor( colorSettings->GetColor( LAYER_SCHEMATIC_GRID_AXES ) );

    GetCanvas()->GetView()->UpdateAllItems( KIGFX::ALL );
    GetCanvas()->GetView()->RecacheAllItems();
    GetCanvas()->GetView()->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
}


COLOR_SETTINGS* SCH_BASE_FRAME::GetColorSettings( bool aForceRefresh ) const
{
    if( !m_colorSettings || aForceRefresh )
    {
        SETTINGS_MANAGER&  mgr = Pgm().GetSettingsManager();
        EESCHEMA_SETTINGS* cfg = mgr.GetAppSettings<EESCHEMA_SETTINGS>();
        wxString           colorTheme = cfg->m_ColorTheme;

        if( IsType( FRAME_SCH_SYMBOL_EDITOR ) )
        {
            SYMBOL_EDITOR_SETTINGS* symCfg = mgr.GetAppSettings<SYMBOL_EDITOR_SETTINGS>();

            if( !symCfg->m_UseEeschemaColorSettings )
                colorTheme = symCfg->m_ColorTheme;
        }

        COLOR_SETTINGS* colorSettings = mgr.GetColorSettings( colorTheme );

        const_cast<SCH_BASE_FRAME*>( this )->m_colorSettings = colorSettings;
    }

    return m_colorSettings;
}


COLOR4D SCH_BASE_FRAME::GetDrawBgColor() const
{
    return GetColorSettings()->GetColor( LAYER_SCHEMATIC_BACKGROUND );
}


void SCH_BASE_FRAME::handleActivateEvent( wxActivateEvent& aEvent )
{
    EDA_DRAW_FRAME::handleActivateEvent( aEvent );

    if( m_spaceMouse )
        m_spaceMouse->SetFocus( aEvent.GetActive() );
}


void SCH_BASE_FRAME::handleIconizeEvent( wxIconizeEvent& aEvent )
{
    EDA_DRAW_FRAME::handleIconizeEvent( aEvent );

    if( m_spaceMouse )
        m_spaceMouse->SetFocus( false );
}


wxString SCH_BASE_FRAME::SelectLibraryFromList()
{
    COMMON_SETTINGS* cfg = Pgm().GetCommonSettings();
    PROJECT&         prj = Prj();

    if( prj.SchSymbolLibTable()->IsEmpty() )
    {
        ShowInfoBarError( _( "No symbol libraries are loaded." ) );
        return wxEmptyString;
    }

    wxArrayString headers;

    headers.Add( _( "Library" ) );

    std::vector< wxArrayString > itemsToDisplay;
    std::vector< wxString > libNicknames = prj.SchSymbolLibTable()->GetLogicalLibs();

    for( const wxString& name : libNicknames )
    {
        // Exclude read only libraries.
        if( !prj.SchSymbolLibTable()->IsSymbolLibWritable( name ) )
            continue;

        if( alg::contains( prj.GetProjectFile().m_PinnedSymbolLibs, name )
            || alg::contains( cfg->m_Session.pinned_symbol_libs, name ) )
        {
            wxArrayString item;

            item.Add( LIB_TREE_MODEL_ADAPTER::GetPinningSymbol() + name );
            itemsToDisplay.push_back( item );
        }
    }

    for( const wxString& name : libNicknames )
    {
        // Exclude read only libraries.
        if( !prj.SchSymbolLibTable()->IsSymbolLibWritable( name ) )
            continue;

        if( !alg::contains( prj.GetProjectFile().m_PinnedSymbolLibs, name )
            && !alg::contains( cfg->m_Session.pinned_symbol_libs, name ) )
        {
            wxArrayString item;

            item.Add( name );
            itemsToDisplay.push_back( item );
        }
    }

    wxString oldLibName = prj.GetRString( PROJECT::SCH_LIB_SELECT );

    EDA_LIST_DIALOG dlg( this, _( "Select Symbol Library" ), headers, itemsToDisplay, oldLibName,
                         false );

    if( dlg.ShowModal() != wxID_OK )
        return wxEmptyString;

    wxString libName = dlg.GetTextSelection();

    if( !libName.empty() )
    {
        if( prj.SchSymbolLibTable()->HasLibrary( libName ) )
            prj.SetRString( PROJECT::SCH_LIB_SELECT, libName );
        else
            libName = wxEmptyString;
    }

    return libName;
}


void SCH_BASE_FRAME::setSymWatcher( const LIB_ID* aID )
{
    Unbind( wxEVT_FSWATCHER, &SCH_BASE_FRAME::OnSymChange, this );

    if( m_watcher )
    {
        wxLogTrace( "KICAD_LIB_WATCH", "Remove watch" );
        m_watcher->RemoveAll();
        m_watcher->SetOwner( nullptr );
        m_watcher.reset();
    }

    wxString libfullname;
    SYMBOL_LIB_TABLE* tbl = Prj().SchSymbolLibTable();

    if( !aID || !tbl )
        return;

    try
    {
        const SYMBOL_LIB_TABLE_ROW* row = tbl->FindRow( aID->GetLibNickname() );

        if( !row )
            return;

        libfullname = row->GetFullURI( true );
    }
    catch( const std::exception& e )
    {
        DisplayInfoMessage( this, e.what() );
        return;
    }
    catch( const IO_ERROR& error )
    {
        wxLogTrace( "KICAD_LIB_WATCH", "Error: %s", error.What() );
        return;
    }

    wxLogTrace( "KICAD_LIB_WATCH", "Setting up watcher for %s", libfullname );
    m_watcherFileName.Assign( libfullname );

    if( !m_watcherFileName.FileExists() )
        return;

    wxLog::EnableLogging( false );
    m_watcherLastModified = m_watcherFileName.GetModificationTime();
    wxLog::EnableLogging( true );

    Bind( wxEVT_FSWATCHER, &SCH_BASE_FRAME::OnSymChange, this );
    m_watcher = std::make_unique<wxFileSystemWatcher>();
    m_watcher->SetOwner( this );

    wxFileName fn;
    fn.AssignDir( m_watcherFileName.GetPath() );
    fn.DontFollowLink();

    m_watcher->AddTree( fn );
}


void SCH_BASE_FRAME::OnSymChange( wxFileSystemWatcherEvent& aEvent )
{
    SYMBOL_LIBS* libs = Prj().SchLibs();

    wxLogTrace( "KICAD_LIB_WATCH", "OnSymChange: %s, watcher file: %s",
                aEvent.GetPath().GetFullPath(), m_watcherFileName.GetFullPath() );

    if( !libs || !m_watcher || !m_watcher.get() || m_watcherFileName.GetPath().IsEmpty() )
        return;

    if( aEvent.GetPath() != m_watcherFileName )
        return;

    // Start the debounce timer (set to 1 second)
    if( !m_watcherDebounceTimer.StartOnce( 1000 ) )
    {
        wxLogTrace( "KICAD_LIB_WATCH", "Failed to start the debounce timer" );
        return;
    }
}


void SCH_BASE_FRAME::OnSymChangeDebounceTimer( wxTimerEvent& aEvent )
{
    wxLogTrace( "KICAD_LIB_WATCH", "OnSymChangeDebounceTimer" );
    // Disable logging to avoid spurious messages and check if the file has changed
    wxLog::EnableLogging( false );
    wxDateTime lastModified = m_watcherFileName.GetModificationTime();
    wxLog::EnableLogging( true );

    if( lastModified == m_watcherLastModified || !lastModified.IsValid() )
        return;

    m_watcherLastModified = lastModified;

    if( !GetScreen()->IsContentModified() || IsOK( this, _( "The library containing the current symbol has changed.\n"
                                                            "Do you want to reload the library?" ) ) )
    {
        wxLogTrace( "KICAD_LIB_WATCH", "Sending refresh symbol mail" );
        std::string libName = m_watcherFileName.GetFullPath().ToStdString();
        Kiway().ExpressMail( FRAME_SCH_VIEWER, MAIL_REFRESH_SYMBOL, libName );
        Kiway().ExpressMail( FRAME_SCH_SYMBOL_EDITOR, MAIL_REFRESH_SYMBOL, libName );
    }
}