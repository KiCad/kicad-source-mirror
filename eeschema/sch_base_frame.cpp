/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#include <advanced_config.h>
#include <base_units.h>
#include <kiway.h>
#include <lib_tree_model_adapter.h>
#include <pgm_base.h>
#include <eda_list_dialog.h>
#include <widgets/filedlg_hook_new_library.h>
#include <symbol_library_manager.h>
#include <eeschema_settings.h>
#include <gal/graphics_abstraction_layer.h>
#include <project/project_file.h>
#include <symbol_editor/symbol_editor_settings.h>
#include <sch_draw_panel.h>
#include <sch_group.h>
#include <sch_view.h>
#include <sch_painter.h>
#include <sch_shape.h>
#include <settings/settings_manager.h>
#include <confirm.h>
#include <preview_items/selection_area.h>
#include <project_sch.h>
#include <symbol_library.h>
#include <symbol_lib_table.h>
#include <sch_base_frame.h>
#include <dialogs/dialog_sch_find.h>
#include <design_block.h>
#include <design_block_lib_table.h>
#include <tool/actions.h>
#include <tool/action_toolbar.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tools/sch_selection_tool.h>
#include <view/view_controls.h>
#include <wx/choicdlg.h>
#include <wx/fswatcher.h>
#include <wx/log.h>
#include <wx/msgdlg.h>

#ifndef __linux__
#include <navlib/nl_schematic_plugin.h>
#include <wx/fdrepdlg.h>
#else
#include <spacenav/spnav_2d_plugin.h>
#endif


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
            cacheName << "_" << aLibId.GetLibItemName();
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
        m_selectionFilterPanel( nullptr ),
        m_findReplaceDialog( nullptr ),
        m_base_frame_defaults( nullptr, "base_Frame_defaults" ),
        m_inSymChangeTimerEvent( false )
{
    m_findReplaceData = std::make_unique<SCH_SEARCH_DATA>();

    if( ( aStyle & wxFRAME_NO_TASKBAR ) == 0 )
        createCanvas();

    Bind( wxEVT_IDLE,
          [this]( wxIdleEvent& aEvent )
          {
              // Handle cursor adjustments.  While we can get motion and key events through
              // wxWidgets, we can't get modifier-key-up events.
              if( m_toolManager )
              {
                  SCH_SELECTION_TOOL* selTool = m_toolManager->GetTool<SCH_SELECTION_TOOL>();

                  if( selTool )
                      selTool->OnIdle( aEvent );
              }
          } );

    m_watcherDebounceTimer.Bind( wxEVT_TIMER, &SCH_BASE_FRAME::OnSymChangeDebounceTimer, this );
}


/// Needs to be in the cpp file to encode the sizeof() for std::unique_ptr
SCH_BASE_FRAME::~SCH_BASE_FRAME()
{}


void SCH_BASE_FRAME::doCloseWindow()
{
    GetCanvas()->SetEvtHandlerEnabled( false );
    GetCanvas()->StopDrawing();

    // Shutdown all running tools
    if( m_toolManager )
        m_toolManager->ShutdownAllTools();

    // Close the find dialog and preserve its setting if it is displayed.
    if( m_findReplaceDialog )
    {
        m_findStringHistoryList = m_findReplaceDialog->GetFindEntries();
        m_replaceStringHistoryList = m_findReplaceDialog->GetReplaceEntries();

        m_findReplaceDialog->Destroy();
        m_findReplaceDialog = nullptr;
    }

    // This class is pure virtual.  Derived class will finish shutdown and Destroy().
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


APP_SETTINGS_BASE* SCH_BASE_FRAME::GetViewerSettingsBase() const
{
    switch( GetFrameType() )
    {
    case FRAME_SCH:
    default:
        return GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" );

    case FRAME_SCH_SYMBOL_EDITOR:
    case FRAME_SCH_VIEWER:
    case FRAME_SYMBOL_CHOOSER:
        return GetAppSettings<SYMBOL_EDITOR_SETTINGS>( "symbol_editor" );
    }
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
    SYMBOL_LIB* cache =
            ( aUseCacheLib ) ? PROJECT_SCH::SchLibs( &Prj() )->GetCacheLibrary() : nullptr;

    return SchGetLibSymbol( aLibId, PROJECT_SCH::SchSymbolLibTable( &Prj() ), cache, this,
                            aShowErrorMsg );
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
            PROJECT_SCH::SchSymbolLibTable( &Prj() )->Save( fn.GetFullPath() );
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


SCH_RENDER_SETTINGS* SCH_BASE_FRAME::GetRenderSettings()
{
    if( GetCanvas() && GetCanvas()->GetView() )
    {
        if( KIGFX::PAINTER* painter = GetCanvas()->GetView()->GetPainter() )
            return static_cast<SCH_RENDER_SETTINGS*>( painter->GetSettings() );
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
        {
#ifndef __linux__
            m_spaceMouse = std::make_unique<NL_SCHEMATIC_PLUGIN>();
#else
            m_spaceMouse = std::make_unique<SPNAV_2D_PLUGIN>( GetCanvas() );
            m_spaceMouse->SetScale( schIUScale.IU_PER_MILS / pcbIUScale.IU_PER_MILS );
#endif
        }

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
        if( aItem->Type() == SCH_SHAPE_T )
            static_cast<SCH_SHAPE*>( aItem )->UpdateHatching();

        if( !isAddOrDelete )
            GetCanvas()->GetView()->Update( aItem );

        // Some children are drawn from their parents.  Mark them for re-paint.
        if( parent && ( parent->Type() == SCH_SYMBOL_T
                        || parent->Type() == SCH_SHEET_T
                        || parent->Type() == SCH_LABEL_LOCATE_ANY_T
                        || parent->Type() == SCH_TABLE_T ) )
        {
            GetCanvas()->GetView()->Update( parent, KIGFX::REPAINT );
        }
    }

    /*
     * Be careful when calling this.  Update will invalidate RTree iterators, so you cannot
     * call this while doing things like `for( SCH_ITEM* item : screen->Items() )`
     */
    if( aUpdateRtree && dynamic_cast<SCH_ITEM*>( aItem ) )
    {
        GetScreen()->Update( static_cast<SCH_ITEM*>( aItem ) );

        /*
         * If we are updating the group, we also need to update all the children otherwise
         * their positions will remain stale in the RTree
        */
        if( SCH_GROUP* group = dynamic_cast<SCH_GROUP*>( aItem ) )
        {
            group->RunOnChildren(
                    [&]( SCH_ITEM* item )
                    {
                        GetScreen()->Update( item );
                    },
                    RECURSE_MODE::RECURSE );
        }
    }

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
        SCH_SELECTION_TOOL* selectionTool = m_toolManager->GetTool<SCH_SELECTION_TOOL>();
        SELECTION&          selection = selectionTool->GetSelection();
        KIGFX::SCH_VIEW*    view = GetCanvas()->GetView();

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
    wxCHECK( aItem, /* void */ );

    SCH_SCREEN* screen = aScreen;

    if( aScreen == nullptr )
        screen = GetScreen();

    if( aItem->Type() != SCH_TABLECELL_T )
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

    if( aItem->Type() != SCH_TABLECELL_T )
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


void SCH_BASE_FRAME::ShowFindReplaceDialog( bool aReplace )
{
    wxString findString;

    SCH_SELECTION& selection = m_toolManager->GetTool<SCH_SELECTION_TOOL>()->GetSelection();

    if( selection.Size() == 1 )
    {
        EDA_ITEM* front = selection.Front();

        switch( front->Type() )
        {
        case SCH_SYMBOL_T:
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( front );
            findString = UnescapeString( symbol->GetField( FIELD_T::VALUE )->GetText() );
            break;
        }

        case SCH_FIELD_T:
            findString = UnescapeString( static_cast<SCH_FIELD*>( front )->GetText() );
            break;

        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
        case SCH_SHEET_PIN_T:
            findString = UnescapeString( static_cast<SCH_LABEL_BASE*>( front )->GetText() );
            break;

        case SCH_TEXT_T:
            findString = UnescapeString( static_cast<SCH_TEXT*>( front )->GetText() );

            if( findString.Contains( wxT( "\n" ) ) )
                findString = findString.Before( '\n' );

            break;

        default:
            break;
        }
    }

    if( m_findReplaceDialog )
        m_findReplaceDialog->Destroy();

    m_findReplaceDialog = new DIALOG_SCH_FIND( this, static_cast<SCH_SEARCH_DATA*>( m_findReplaceData.get() ),
                                               wxDefaultPosition, wxDefaultSize, aReplace ? wxFR_REPLACEDIALOG : 0 );

    m_findReplaceDialog->SetFindEntries( m_findStringHistoryList, findString );
    m_findReplaceDialog->SetReplaceEntries( m_replaceStringHistoryList );
    m_findReplaceDialog->Show( true );
}


void SCH_BASE_FRAME::ShowFindReplaceStatus( const wxString& aMsg, int aStatusTime )
{
    // Prepare the infobar, since we don't know its state
    m_infoBar->RemoveAllButtons();
    m_infoBar->AddCloseButton();

    m_infoBar->ShowMessageFor( aMsg, aStatusTime, wxICON_INFORMATION );
}


void SCH_BASE_FRAME::ClearFindReplaceStatus()
{
    m_infoBar->Dismiss();
}


void SCH_BASE_FRAME::OnFindDialogClose()
{
    m_findStringHistoryList = m_findReplaceDialog->GetFindEntries();
    m_replaceStringHistoryList = m_findReplaceDialog->GetReplaceEntries();

    m_findReplaceDialog->Destroy();
    m_findReplaceDialog = nullptr;

    m_toolManager->RunAction( ACTIONS::updateFind );
}


void SCH_BASE_FRAME::CommonSettingsChanged( int aFlags )
{
    EDA_DRAW_FRAME::CommonSettingsChanged( aFlags );

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
        EESCHEMA_SETTINGS* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" );
        wxString           colorTheme = cfg ? cfg->m_ColorTheme : wxString( "" );

        if( IsType( FRAME_SCH_SYMBOL_EDITOR ) )
        {
            if( SYMBOL_EDITOR_SETTINGS* sym_edit_cfg = GetAppSettings<SYMBOL_EDITOR_SETTINGS>( "symbol_editor" ) )
            {
                if( !sym_edit_cfg->m_UseEeschemaColorSettings )
                    colorTheme = sym_edit_cfg->m_ColorTheme;
            }
        }

        const_cast<SCH_BASE_FRAME*>( this )->m_colorSettings = ::GetColorSettings( colorTheme );
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


void SCH_BASE_FRAME::GetLibraryItemsForListDialog( wxArrayString& aHeaders,
                                                   std::vector<wxArrayString>& aItemsToDisplay )
{
    COMMON_SETTINGS*      cfg = Pgm().GetCommonSettings();
    PROJECT_FILE&         project = Prj().GetProjectFile();
    SYMBOL_LIB_TABLE*     tbl = PROJECT_SCH::SchSymbolLibTable( &Prj() );
    std::vector<wxString> libNicknames = tbl->GetLogicalLibs();

    aHeaders.Add( _( "Library" ) );
    aHeaders.Add( _( "Description" ) );

    for( const wxString& nickname : libNicknames )
    {
        if( alg::contains( project.m_PinnedSymbolLibs, nickname )
            || alg::contains( cfg->m_Session.pinned_symbol_libs, nickname ) )
        {
            wxArrayString item;

            item.Add( LIB_TREE_MODEL_ADAPTER::GetPinningSymbol() + nickname );
            item.Add( tbl->GetDescription( nickname ) );
            aItemsToDisplay.push_back( item );
        }
    }

    for( const wxString& nickname : libNicknames )
    {
        if( !alg::contains( project.m_PinnedSymbolLibs, nickname )
                && !alg::contains( cfg->m_Session.pinned_symbol_libs, nickname ) )
        {
            wxArrayString item;

            item.Add( nickname );
            item.Add( tbl->GetDescription( nickname ) );
            aItemsToDisplay.push_back( item );
        }
    }
}


wxString SCH_BASE_FRAME::SelectLibrary( const wxString& aDialogTitle, const wxString& aListLabel,
                                        const std::vector<std::pair<wxString, bool*>>& aExtraCheckboxes )
{
    static const int ID_MAKE_NEW_LIBRARY = wxID_HIGHEST;

    // Keep asking the user for a new name until they give a valid one or cancel the operation
    while( true )
    {
        wxArrayString              headers;
        std::vector<wxArrayString> itemsToDisplay;

        GetLibraryItemsForListDialog( headers, itemsToDisplay );

        wxString libraryName = Prj().GetRString( PROJECT::SCH_LIB_SELECT );

        EDA_LIST_DIALOG dlg( this, aDialogTitle, headers, itemsToDisplay, libraryName, false, aExtraCheckboxes );
        dlg.SetListLabel( aListLabel );

        wxButton* newLibraryButton = new wxButton( &dlg, ID_MAKE_NEW_LIBRARY, _( "New Library..." ) );
        dlg.m_ButtonsSizer->Prepend( 80, 20 );
        dlg.m_ButtonsSizer->Prepend( newLibraryButton, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 10 );

        newLibraryButton->Bind( wxEVT_BUTTON,
                [&dlg]( wxCommandEvent& )
                {
                    dlg.EndModal( ID_MAKE_NEW_LIBRARY );
                }, ID_MAKE_NEW_LIBRARY );

        dlg.Layout();
        dlg.GetSizer()->Fit( &dlg );

        int ret = dlg.ShowModal();

        switch( ret )
        {
        case wxID_CANCEL:
            return wxEmptyString;

        case wxID_OK:
            libraryName = dlg.GetTextSelection();
            Prj().SetRString( PROJECT::SCH_LIB_SELECT, libraryName );
            dlg.GetExtraCheckboxValues();
            return libraryName;

        case ID_MAKE_NEW_LIBRARY:
        {
            SYMBOL_LIBRARY_MANAGER   mgr( *this );
            wxFileName               fn( Prj().GetRString( PROJECT::SCH_LIB_PATH ) );
            bool                     useGlobalTable = false;
            FILEDLG_HOOK_NEW_LIBRARY tableChooser( useGlobalTable );

            if( !LibraryFileBrowser( _( "Create New Library" ), false, fn, FILEEXT::KiCadSymbolLibFileWildcard(),
                                     FILEEXT::KiCadSymbolLibFileExtension, false, &tableChooser ) )
            {
                break;
            }

            libraryName = fn.GetName();
            Prj().SetRString( PROJECT::SCH_LIB_PATH, fn.GetPath() );

            useGlobalTable = tableChooser.GetUseGlobalTable();

            SYMBOL_LIB_TABLE* libTable = useGlobalTable ? &SYMBOL_LIB_TABLE::GetGlobalLibTable()
                                                        : PROJECT_SCH::SchSymbolLibTable( &Prj() );

            if( libTable->HasLibrary( libraryName, false ) )
            {
                DisplayError( this, wxString::Format( _( "Library '%s' already exists." ), libraryName ) );
                break;
            }

            if( !mgr.CreateLibrary( fn.GetFullPath(), *libTable ) )
                DisplayError( this, wxString::Format( _( "Could not add library '%s'." ), libraryName ) );

            break;
        }

        default:
            break;
        }
    }
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
    SYMBOL_LIB_TABLE* tbl = PROJECT_SCH::SchSymbolLibTable( &Prj() );

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

    {
        // Silence OS errors that come from the watcher
        wxLogNull silence;
        m_watcher->Add( fn );
    }
}


void SCH_BASE_FRAME::OnSymChange( wxFileSystemWatcherEvent& aEvent )
{
    SYMBOL_LIBS* libs = PROJECT_SCH::SchLibs( &Prj() );

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
    if( aEvent.GetId() != m_watcherDebounceTimer.GetId() )
    {
        aEvent.Skip();
        return;
    }

    if( m_inSymChangeTimerEvent )
    {
        wxLogTrace( "KICAD_LIB_WATCH", "Restarting debounce timer" );
        m_watcherDebounceTimer.StartOnce( 3000 );
    }

    wxLogTrace( "KICAD_LIB_WATCH", "OnSymChangeDebounceTimer" );

    // Disable logging to avoid spurious messages and check if the file has changed
    wxLog::EnableLogging( false );
    wxDateTime lastModified = m_watcherFileName.GetModificationTime();
    wxLog::EnableLogging( true );

    if( lastModified == m_watcherLastModified || !lastModified.IsValid() )
        return;

    m_watcherLastModified = lastModified;

    m_inSymChangeTimerEvent = true;

    if( !GetScreen()->IsContentModified()
      || IsOK( this, _( "The library containing the current symbol has changed.\n"
                        "Do you want to reload the library?" ) ) )
    {
        wxLogTrace( "KICAD_LIB_WATCH", "Sending refresh symbol mail" );
        std::string libName = m_watcherFileName.GetFullPath().ToStdString();
        Kiway().ExpressMail( FRAME_SCH_VIEWER, MAIL_REFRESH_SYMBOL, libName );
        Kiway().ExpressMail( FRAME_SCH_SYMBOL_EDITOR, MAIL_REFRESH_SYMBOL, libName );
    }

    m_inSymChangeTimerEvent = false;
}


SCH_SELECTION_TOOL* SCH_BASE_FRAME::GetSelectionTool()
{
    if( m_toolManager )
        return m_toolManager->GetTool<SCH_SELECTION_TOOL>();

    return nullptr;
}
