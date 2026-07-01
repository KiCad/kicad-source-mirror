/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file
 * Tabbed-document machinery for the Symbol Editor.  Tab switching is a non-destructive ownership
 * handoff: the active tab lends its working symbol/screen and undo/redo lists to the frame and
 * takes them back on detach.
 */

#include <symbol_edit_frame.h>

#include <kidialog.h>
#include <lib_symbol.h>
#include <lib_symbol_library_manager.h>
#include <sch_screen.h>
#include <sch_view.h>
#include <symbol_editor/symbol_editor_settings.h>
#include <symbol_editor/symbol_editor_tab_context.h>
#include <tool/actions.h>
#include <tool/selection.h>
#include <tool/tool_manager.h>
#include <tools/sch_selection_tool.h>
#include <view/view.h>
#include <view/view_controls.h>
#include <widgets/editor_tabs_panel.h>
#include <widgets/lib_tree.h>
#include <widgets/sch_properties_panel.h>


EDITOR_TAB_CONTEXT::VIEW_SNAPSHOT SYMBOL_EDIT_FRAME::captureSymbolViewSnapshot() const
{
    EDITOR_TAB_CONTEXT::VIEW_SNAPSHOT snapshot;

    if( KIGFX::VIEW* view = GetCanvas() ? GetCanvas()->GetView() : nullptr )
    {
        snapshot.scale = view->GetScale();
        snapshot.center = view->GetCenter();
        snapshot.valid = true;
    }

    return snapshot;
}


void SYMBOL_EDIT_FRAME::restoreSymbolViewSnapshot( const EDITOR_TAB_CONTEXT::VIEW_SNAPSHOT& aSnapshot )
{
    if( !aSnapshot.valid )
        return;

    if( KIGFX::VIEW* view = GetCanvas() ? GetCanvas()->GetView() : nullptr )
    {
        view->SetScale( aSnapshot.scale );
        view->SetCenter( aSnapshot.center );
    }
}


std::vector<KIID> SYMBOL_EDIT_FRAME::captureSymbolSelectionKiids() const
{
    std::vector<KIID> kiids;

    SCH_SELECTION_TOOL* selTool = m_toolManager
                                          ? m_toolManager->GetTool<SCH_SELECTION_TOOL>()
                                          : nullptr;

    if( !selTool )
        return kiids;

    for( EDA_ITEM* item : selTool->GetSelection() )
    {
        if( item )
            kiids.push_back( item->m_Uuid );
    }

    return kiids;
}


void SYMBOL_EDIT_FRAME::restoreSymbolSelectionKiids( const std::vector<KIID>& aKiids )
{
    if( aKiids.empty() )
        return;

    SCH_SELECTION_TOOL* selTool = m_toolManager
                                          ? m_toolManager->GetTool<SCH_SELECTION_TOOL>()
                                          : nullptr;
    SCH_SCREEN*         screen = GetScreen();

    if( !selTool || !screen )
        return;

    for( SCH_ITEM* item : screen->Items() )
    {
        for( const KIID& kiid : aKiids )
        {
            if( item->m_Uuid == kiid )
            {
                selTool->AddItemToSel( item, true );
                break;
            }
        }
    }
}


void SYMBOL_EDIT_FRAME::detachActiveSymbolTab()
{
    if( !m_activeTab )
        return;

    m_activeTab->ViewSnapshot() = captureSymbolViewSnapshot();
    m_activeTab->SavedSelection() = captureSymbolSelectionKiids();
    m_activeTab->SetUnit( m_unit );
    m_activeTab->SetBodyStyle( m_bodyStyle );

    // m_symbol may have been replaced by undo/redo since activation.
    m_activeTab->AdoptWorkingObjects( m_symbol, static_cast<SCH_SCREEN*>( GetScreen() ) );

    m_activeTab->UndoList().m_CommandsList.swap( m_undoList.m_CommandsList );
    m_activeTab->RedoList().m_CommandsList.swap( m_redoList.m_CommandsList );
}


void SYMBOL_EDIT_FRAME::activateSymbolTab( SYMBOL_EDITOR_TAB_CONTEXT* aContext )
{
    wxCHECK( m_toolManager, /* void */ );

    if( !aContext )
        return;

    if( aContext == m_activeTab )
    {
        // Already active, but the caller may have requested a different unit/body style; apply it.
        m_unit = aContext->GetUnit();
        m_bodyStyle = aContext->GetBodyStyle();

        GetRenderSettings()->m_ShowUnit = m_unit;
        GetRenderSettings()->m_ShowBodyStyle = m_bodyStyle;
        GetRenderSettings()->m_ShowDisabled = IsSymbolFromLegacyLibrary() && !IsSymbolFromSchematic();
        GetRenderSettings()->m_ShowGraphicsDisabled = IsSymbolAlias() && !IsSymbolFromSchematic();

        m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );
        RebuildSymbolUnitAndBodyStyleLists();
        GetCanvas()->Refresh();
        return;
    }

    detachActiveSymbolTab();
    m_activeTab = aContext;

    m_undoList.m_CommandsList.swap( aContext->UndoList().m_CommandsList );
    m_redoList.m_CommandsList.swap( aContext->RedoList().m_CommandsList );

    m_toolManager->RunAction( ACTIONS::selectionClear );

    aContext->ReleaseToFrame();

    // Restore the frame's schematic-source state from the incoming tab so the existing save path
    // (SaveSymbolToSchematic vs the library) keys off the active tab, not whichever tab loaded last.
    m_isSymbolFromSchematic = aContext->IsFromSchematic();
    m_schematicSymbolUUID = aContext->GetSchematicSymbolUUID();
    m_reference = aContext->GetReference();

    m_symbol = aContext->GetSymbol();
    m_unit = aContext->GetUnit();
    m_bodyStyle = aContext->GetBodyStyle();

    // Install the new screen before SetScreen, whose ResetTools would otherwise deref the freed one.
    m_toolManager->SetEnvironment( aContext->GetScreen(), GetCanvas()->GetView(),
                                   GetCanvas()->GetViewControls(), GetSettings(), this );
    SetScreen( aContext->GetScreen() );

    SetCurLib( aContext->GetLibrary() );

    if( m_symbol )
    {
        wxLogTrace( wxT( "KICAD_TABS_DBG" ),
                    wxT( "activateSymbolTab SelectLibId '%s'" ),
                    m_symbol->GetLibId().Format().wx_str() );
        GetLibTree()->SelectLibId( m_symbol->GetLibId() );
    }

    m_SyncPinEdit = m_symbol && m_symbol->IsRoot() && m_symbol->IsMultiUnit()
                    && !m_symbol->UnitsLocked();

    m_toolManager->SetEnvironment( GetScreen(), GetCanvas()->GetView(),
                                   GetCanvas()->GetViewControls(), GetSettings(), this );

    GetRenderSettings()->m_ShowUnit = m_unit;
    GetRenderSettings()->m_ShowBodyStyle = m_bodyStyle;
    GetRenderSettings()->m_ShowDisabled = IsSymbolFromLegacyLibrary() && !IsSymbolFromSchematic();
    GetRenderSettings()->m_ShowGraphicsDisabled = IsSymbolAlias() && !IsSymbolFromSchematic();

    GetCanvas()->DisplaySymbol( m_symbol );
    GetCanvas()->GetView()->HideDrawingSheet();
    GetCanvas()->GetView()->ClearHiddenFlags();

    // MODEL_RELOAD clears the selection, so restore the saved one afterwards.
    m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );

    if( aContext->ViewSnapshot().valid )
    {
        restoreSymbolViewSnapshot( aContext->ViewSnapshot() );
    }
    else
    {
        // First time this tab is shown there is no saved view
        m_toolManager->RunAction( ACTIONS::zoomFitScreen );
    }

    restoreSymbolSelectionKiids( aContext->SavedSelection() );

    UpdateTitle();
    RebuildSymbolUnitAndBodyStyleLists();

    if( m_propertiesPanel )
        m_propertiesPanel->UpdateData();

    UpdateSymbolMsgPanelInfo();
    GetCanvas()->Refresh();
}


void SYMBOL_EDIT_FRAME::OnTabCharHook( wxKeyEvent& aEvent )
{
    // Only plain Ctrl+Tab and Ctrl+Shift+Tab are ours; anything else falls through.
    const bool isTab = aEvent.GetKeyCode() == WXK_TAB;
    const bool ctrlOnly = aEvent.ControlDown() && !aEvent.AltDown() && !aEvent.MetaDown();

    // Ctrl+W closes the active tab while tabs are open, else falls through to close-window.  The
    // keycode arrives as 'W' or the control char 0x17 depending on platform, so accept both.
    const int  keyCode = aEvent.GetKeyCode();
    const bool isCtrlW = ctrlOnly && !aEvent.ShiftDown()
                         && ( keyCode == 'W' || keyCode == ( 'W' - '@' ) );

    if( isCtrlW && m_tabsPanel && m_tabsPanel->Model().Entries().size() >= 1 )
    {
        // Route through CloseTab so the unsaved-changes prompt still runs.
        m_tabsPanel->CloseTab( m_tabsPanel->GetActiveTab() );
        return;
    }

    if( !isTab || !ctrlOnly || !m_tabsPanel )
    {
        aEvent.Skip();
        return;
    }

    m_tabsPanel->AdvanceTab( !aEvent.ShiftDown() );

    // Consume the event so GTK's default Tab focus-traversal does not also run.
}


void SYMBOL_EDIT_FRAME::freeTransientUndoCommands( UNDO_REDO_CONTAINER& aList,
                                                   const LIB_SYMBOL*    aLiveSymbol )
{
    for( PICKED_ITEMS_LIST* cmd : aList.m_CommandsList )
    {
        // The undo copy is the picked ITEM flagged UR_TRANSIENT, not the wrapper, so the shared
        // deleters skip it.  Free exactly the items that carry UR_TRANSIENT here.
        for( unsigned ii = 0; ii < cmd->GetCount(); ++ii )
        {
            EDA_ITEM* item = cmd->GetPickedItem( ii );

            if( !item || !item->HasFlag( UR_TRANSIENT ) )
                continue;

            // The live working symbol clears UR_TRANSIENT when it becomes m_symbol; guard anyway
            // against a double-free of the live symbol.
            wxCHECK2_MSG( item != aLiveSymbol, continue,
                          wxT( "Live working symbol flagged UR_TRANSIENT in undo/redo list" ) );

            delete item;
        }

        cmd->ClearItemsList();
        delete cmd;
    }

    aList.m_CommandsList.clear();
}


void SYMBOL_EDIT_FRAME::clearSymbolTabUndoRedo( SYMBOL_EDITOR_TAB_CONTEXT& aContext )
{
    // The context is detached here, so no undo copy is the live working object.
    freeTransientUndoCommands( aContext.UndoList(), nullptr );
    freeTransientUndoCommands( aContext.RedoList(), nullptr );
}


void SYMBOL_EDIT_FRAME::clearTabModifiedState( SYMBOL_EDITOR_TAB_CONTEXT& aContext,
                                               EDITOR_TABS_MODEL&         aModel )
{
    // An active tab's screen aliases the frame's current screen, so this clears the live flag too.
    if( SCH_SCREEN* screen = aContext.GetScreen() )
        screen->SetContentModified( false );

    aModel.MarkModified( aContext.GetTabKey(), false );
}


void SYMBOL_EDIT_FRAME::clearSymbolTabsModifiedForLibrary( const wxString& aLibrary )
{
    for( const std::unique_ptr<SYMBOL_EDITOR_TAB_CONTEXT>& ctx : m_tabContexts )
    {
        if( ctx->GetLibrary() != aLibrary )
            continue;

        if( m_tabsPanel )
            clearTabModifiedState( *ctx, m_tabsPanel->MutableModel() );
        else if( SCH_SCREEN* screen = ctx->GetScreen() )
            screen->SetContentModified( false );
    }

    // The strip queries the dirty flag only on repaint, so without this the cleared asterisk lingers
    // until the next stray paint event reaches the tab the user was on.
    if( m_tabsPanel )
        m_tabsPanel->RefreshTabLabels();
}


SYMBOL_EDITOR_TAB_CONTEXT* SYMBOL_EDIT_FRAME::symbolTabContextForKey( const wxString& aKey ) const
{
    for( const std::unique_ptr<SYMBOL_EDITOR_TAB_CONTEXT>& ctx : m_tabContexts )
    {
        if( ctx->GetTabKey() == aKey )
            return ctx.get();
    }

    return nullptr;
}


void SYMBOL_EDIT_FRAME::dropSymbolTabContext( const wxString& aKey )
{
    SYMBOL_EDITOR_TAB_CONTEXT* ctx = symbolTabContextForKey( aKey );

    // The active context is still on screen and owned by the frame, so never drop it here; only an
    // evicted, inactive preview reaches this path.
    if( !ctx || ctx == m_activeTab )
        return;

    clearSymbolTabUndoRedo( *ctx );

    std::erase_if( m_tabContexts,
                   [ctx]( const std::unique_ptr<SYMBOL_EDITOR_TAB_CONTEXT>& aOwned )
                   {
                       return aOwned.get() == ctx;
                   } );
}


SYMBOL_EDITOR_TAB_CONTEXT* SYMBOL_EDIT_FRAME::symbolTabContextForIndex( int aIdx ) const
{
    if( !m_tabsPanel )
        return nullptr;

    const std::vector<EDITOR_TABS_MODEL::ENTRY>& entries = m_tabsPanel->Model().Entries();

    if( aIdx < 0 || aIdx >= static_cast<int>( entries.size() ) )
        return nullptr;

    return symbolTabContextForKey( entries[aIdx].key );
}


SYMBOL_EDITOR_TAB_CONTEXT* SYMBOL_EDIT_FRAME::findOrCreateSymbolTab( const wxString& aLib,
                                                                    const wxString& aName,
                                                                    int aUnit, int aBodyStyle,
                                                                    bool aAsPreview,
                                                                    bool* aWasCreated )
{
    const wxString key = SYMBOL_EDITOR_TAB_CONTEXT::MakeTabKey( aLib, aName );
    const int      unit = aUnit > 0 ? aUnit : 1;
    const int      bodyStyle = aBodyStyle > 0 ? aBodyStyle : 1;

    SYMBOL_EDITOR_TAB_CONTEXT* ctx = symbolTabContextForKey( key );

    if( aWasCreated )
        *aWasCreated = false;

    if( !ctx )
    {
        SYMBOL_BUFFER* buffer = m_libMgr ? m_libMgr->GetBuffer( aName, aLib ) : nullptr;

        if( !buffer )
            return nullptr;

        m_tabContexts.push_back(
                std::make_unique<SYMBOL_EDITOR_TAB_CONTEXT>( aLib, aName, buffer ) );
        ctx = m_tabContexts.back().get();

        if( aWasCreated )
            *aWasCreated = true;
    }

    ctx->SetUnit( unit );
    ctx->SetBodyStyle( bodyStyle );

    if( m_tabsPanel )
    {
        // A previewed open reuses the existing preview slot in the panel instead of adding a tab.
        // The panel drops the evicted key from its model, but the matching context lingers in
        // m_tabContexts and would be persisted as a phantom tab, so capture the evicted key and drop
        // its context once the reuse completes.
        wxString replacedKey;

        if( aAsPreview )
        {
            const std::vector<EDITOR_TABS_MODEL::ENTRY>& entries = m_tabsPanel->Model().Entries();
            const int previewIdx = m_tabsPanel->Model().PreviewIndex();

            if( previewIdx >= 0 && previewIdx < static_cast<int>( entries.size() )
                    && entries[previewIdx].key != key )
            {
                replacedKey = entries[previewIdx].key;
            }
        }

        m_tabsPanel->AddTab( key, aName, aAsPreview );

        if( !replacedKey.empty() )
            dropSymbolTabContext( replacedKey );
    }
    else
    {
        activateSymbolTab( ctx );
    }

    return ctx;
}


SYMBOL_EDITOR_TAB_CONTEXT*
SYMBOL_EDIT_FRAME::findOrCreateSymbolInstanceTab( LIB_SYMBOL* aSymbol, SCH_SCREEN* aScreen,
                                                  const KIID&     aSchematicSymbolUUID,
                                                  const wxString& aReference, int aUnit,
                                                  int aBodyStyle )
{
    const wxString key = SYMBOL_EDITOR_TAB_CONTEXT::MakeInstanceTabKey( aSchematicSymbolUUID );
    const int      unit = aUnit > 0 ? aUnit : 1;
    const int      bodyStyle = aBodyStyle > 0 ? aBodyStyle : 1;

    if( SYMBOL_EDITOR_TAB_CONTEXT* existing = symbolTabContextForKey( key ) )
    {
        // Re-editing the same placed symbol focuses the live tab, so free the redundant new objects.
        delete aSymbol;
        delete aScreen;

        existing->SetUnit( unit );
        existing->SetBodyStyle( bodyStyle );

        if( m_tabsPanel )
            m_tabsPanel->AddTab( key, existing->GetReference(), false );
        else
            activateSymbolTab( existing );

        return existing;
    }

    m_tabContexts.push_back( std::make_unique<SYMBOL_EDITOR_TAB_CONTEXT>(
            aSymbol, aScreen, aSchematicSymbolUUID, aReference ) );
    SYMBOL_EDITOR_TAB_CONTEXT* ctx = m_tabContexts.back().get();

    ctx->SetUnit( unit );
    ctx->SetBodyStyle( bodyStyle );

    if( m_tabsPanel )
        m_tabsPanel->AddTab( key, aReference, false );
    else
        activateSymbolTab( ctx );

    return ctx;
}


bool SYMBOL_EDIT_FRAME::promptAndCloseSymbolTab( int aIdx )
{
    SYMBOL_EDITOR_TAB_CONTEXT* ctx = symbolTabContextForIndex( aIdx );

    if( !ctx )
        return true;

    if( ctx->IsModified() && !m_silentSymbolTabClose )
    {
        wxString msg = wxString::Format( _( "Save changes to '%s' before closing?" ),
                                         ctx->GetDisplayName() );

        KIDIALOG dlg( this, msg, _( "Confirmation" ), wxYES_NO | wxCANCEL | wxICON_WARNING );
        dlg.SetYesNoCancelLabels( _( "Save" ), _( "Discard Changes" ), _( "Cancel" ) );

        switch( dlg.ShowModal() )
        {
        case wxID_YES:
            // saveCurrentSymbol operates on the active tab, so activate this one first to save it.
            if( ctx != m_activeTab )
                activateSymbolTab( ctx );

            if( !saveCurrentSymbol() )
                return false;

            if( IsLibraryTreeShown() )
                m_treePane->GetLibTree()->RefreshLibTree();

            UpdateTitle();
            break;

        case wxID_NO:
            break;

        default:
            return false;
        }
    }

    // If this is the active tab the frame holds its objects, so repoint at the dummy screen and drop
    // m_symbol before destroying the context, then let the context free them once.
    if( ctx == m_activeTab )
    {
        // Clear the frame's undo/redo so nothing references the about-to-be-deleted working symbol.
        ClearUndoRedoList();
        ctx->AdoptWorkingObjects( m_symbol, static_cast<SCH_SCREEN*>( GetScreen() ) );

        m_activeTab = nullptr;
        m_symbol = nullptr;
        SetScreen( m_dummyScreen );

        // Clear the mirrored schematic-source state so the frame no longer looks like it holds an
        // instance symbol; a successor tab's activateSymbolTab restores these from its own context.
        m_isSymbolFromSchematic = false;
        m_schematicSymbolUUID = niluuid;
        m_reference = wxEmptyString;
    }
    else
    {
        // An inactive tab's undo/redo still holds transient copies; free them before destroying it.
        clearSymbolTabUndoRedo( *ctx );
    }

    std::erase_if( m_tabContexts,
                   [ctx]( const std::unique_ptr<SYMBOL_EDITOR_TAB_CONTEXT>& aOwned )
                   {
                       return aOwned.get() == ctx;
                   } );

    return true;
}


bool SYMBOL_EDIT_FRAME::hasDirtyInactiveInstanceTabs() const
{
    for( const std::unique_ptr<SYMBOL_EDITOR_TAB_CONTEXT>& ctx : m_tabContexts )
    {
        if( ctx.get() != m_activeTab && ctx->IsTransient() && ctx->IsModified() )
            return true;
    }

    return false;
}


bool SYMBOL_EDIT_FRAME::promptToSaveInactiveInstanceTabs()
{
    // Collect first; saving activates a tab, which mutates m_activeTab and m_tabContexts ordering.
    std::vector<SYMBOL_EDITOR_TAB_CONTEXT*> dirty;

    for( const std::unique_ptr<SYMBOL_EDITOR_TAB_CONTEXT>& ctx : m_tabContexts )
    {
        // The active tab and library tabs are handled by the existing close checks.
        if( ctx.get() != m_activeTab && ctx->IsTransient() && ctx->IsModified() )
            dirty.push_back( ctx.get() );
    }

    // Saving activates each dirty tab in turn; restore the tab the user was on so a vetoed close leaves
    // the editor where it was and a successful close persists the real active tab, not a discarded one.
    SYMBOL_EDITOR_TAB_CONTEXT* originalActive = m_activeTab;

    for( SYMBOL_EDITOR_TAB_CONTEXT* ctx : dirty )
    {
        wxString msg = wxString::Format( _( "Save changes to '%s' before closing?" ),
                                         ctx->GetDisplayName() );

        KIDIALOG dlg( this, msg, _( "Confirmation" ), wxYES_NO | wxCANCEL | wxICON_WARNING );
        dlg.SetYesNoCancelLabels( _( "Save" ), _( "Discard Changes" ), _( "Cancel" ) );

        const int answer = dlg.ShowModal();

        if( answer == wxID_YES )
        {
            // saveCurrentSymbol saves the active tab via the mirrored frame state, so activate this
            // one first; it clears the screen's modified flag on success.
            activateSymbolTab( ctx );

            if( !saveCurrentSymbol() )
            {
                activateSymbolTab( originalActive );
                return false;
            }
        }
        else if( answer != wxID_NO )
        {
            activateSymbolTab( originalActive );
            return false;
        }
    }

    activateSymbolTab( originalActive );

    return true;
}


void SYMBOL_EDIT_FRAME::closeAllSymbolTabsSilently()
{
    if( m_tabContexts.empty() )
        return;

    m_silentSymbolTabClose = true;

    if( m_tabsPanel )
    {
        // Remove through the panel so its widget pages and the contexts stay in sync.
        m_tabsPanel->CloseAll();
    }

    // Repoint the frame before the active tab's objects return to its soon-deleted context.
    if( m_activeTab )
    {
        ClearUndoRedoList();
        m_activeTab->AdoptWorkingObjects( m_symbol, static_cast<SCH_SCREEN*>( GetScreen() ) );
        m_activeTab = nullptr;
        m_symbol = nullptr;
        SetScreen( m_dummyScreen );
    }

    // Free undo/redo for any contexts the panel path did not already close.
    for( const std::unique_ptr<SYMBOL_EDITOR_TAB_CONTEXT>& ctx : m_tabContexts )
        clearSymbolTabUndoRedo( *ctx );

    m_tabContexts.clear();

    m_silentSymbolTabClose = false;
}


void SYMBOL_EDIT_FRAME::closeSymbolTab( const LIB_ID& aLibId )
{
    if( !m_tabsPanel )
        return;

    const wxString key = SYMBOL_EDITOR_TAB_CONTEXT::MakeTabKey( aLibId.GetLibNickname(), aLibId.GetLibItemName() );

    const int idx = m_tabsPanel->FindTab( key );

    if( idx < 0 )
        return;

    // The caller already confirmed the deletion, so close without re-prompting. The panel selects a
    // successor tab when this was the active one.
    m_silentSymbolTabClose = true;
    m_tabsPanel->CloseTab( idx );
    m_silentSymbolTabClose = false;
}


void SYMBOL_EDIT_FRAME::RenameSymbolTab( const LIB_ID& aOldId, const LIB_ID& aNewId )
{
    if( !m_tabsPanel )
        return;

    const wxString oldKey = SYMBOL_EDITOR_TAB_CONTEXT::MakeTabKey( aOldId.GetLibNickname(), aOldId.GetLibItemName() );

    if( SYMBOL_EDITOR_TAB_CONTEXT* ctx = symbolTabContextForKey( oldKey ) )
    {
        const wxString newName = aNewId.GetLibItemName();
        const wxString newKey = SYMBOL_EDITOR_TAB_CONTEXT::MakeTabKey( aNewId.GetLibNickname(), newName );

        ctx->SetName( newName );
        m_tabsPanel->RenameTab( oldKey, newKey, newName );
        UpdateTitle();
    }
}


void SYMBOL_EDIT_FRAME::restoreSymbolTabsFromSettings()
{
    wxLogTrace( wxT( "KICAD_TABS_DBG" ), wxT( "restoreSymbolTabsFromSettings enter" ) );

    if( !m_tabsPanel || !m_settings )
        return;

    m_loadingSymbolTab = true;

    wxString activeKey = m_settings->m_ActiveTabKey;
    int      activeIdx = -1;

    for( const SYMBOL_EDITOR_SETTINGS::OPEN_TAB& tab : m_settings->m_OpenTabs )
    {
        if( !m_libMgr->LibraryExists( tab.lib )
                || !m_libMgr->SymbolExists( tab.name, tab.lib ) )
        {
            wxLogTrace( "KICAD_TABS", "Dropping unresolved persisted symbol tab '%s:%s'.",
                        tab.lib, tab.name );
            continue;
        }

        SYMBOL_EDITOR_TAB_CONTEXT* ctx =
                findOrCreateSymbolTab( tab.lib, tab.name, tab.unit, tab.bodyStyle, tab.preview );

        if( !ctx )
        {
            wxLogTrace( "KICAD_TABS", "Dropping persisted symbol tab '%s:%s'; buffer unavailable.",
                        tab.lib, tab.name );
            continue;
        }

        const int idx = m_tabsPanel->FindTab( ctx->GetTabKey() );

        if( ctx->GetTabKey() == activeKey )
            activeIdx = idx;
    }

    m_loadingSymbolTab = false;

    if( activeIdx >= 0 )
        m_tabsPanel->SelectTab( activeIdx );

    wxLogTrace( wxT( "KICAD_TABS_DBG" ), wxT( "restoreSymbolTabsFromSettings exit (activeIdx=%d)" ),
                activeIdx );
}


void SYMBOL_EDIT_FRAME::storeSymbolTabsToSettings()
{
    if( !m_settings )
        return;

    m_settings->m_OpenTabs.clear();
    m_settings->m_ActiveTabKey.clear();

    for( const std::unique_ptr<SYMBOL_EDITOR_TAB_CONTEXT>& ctx : m_tabContexts )
    {
        // Instance tabs are session-only and never persisted.
        if( ctx->IsTransient() )
            continue;

        SYMBOL_EDITOR_SETTINGS::OPEN_TAB tab;
        tab.lib = ctx->GetLibrary();
        tab.name = ctx->GetName();
        tab.unit = ctx->GetUnit();
        tab.bodyStyle = ctx->GetBodyStyle();

        if( m_tabsPanel )
        {
            const int                                    idx = m_tabsPanel->FindTab( ctx->GetTabKey() );
            const std::vector<EDITOR_TABS_MODEL::ENTRY>& entries = m_tabsPanel->Model().Entries();

            if( idx >= 0 && idx < static_cast<int>( entries.size() ) )
                tab.preview = entries[idx].preview;
        }

        m_settings->m_OpenTabs.push_back( tab );
    }

    // Never restore an instance tab as the active tab, since it is not persisted.
    if( m_activeTab && !m_activeTab->IsTransient() )
        m_settings->m_ActiveTabKey = m_activeTab->GetTabKey();
}
