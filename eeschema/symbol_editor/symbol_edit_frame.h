/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2017 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef SYMBOL_EDIT_FRAME_H
#define SYMBOL_EDIT_FRAME_H

#include <sch_base_frame.h>
#include <sch_screen.h>
#include <symbol_tree_pane.h>
#include <widgets/editor_tab_context.h>
#include <memory>
#include <optional>
#include <vector>

class SCH_EDIT_FRAME;
class LIB_SYMBOL;
class LIB_TREE_NODE;
class LIB_ID;
class LIB_SYMBOL_LIBRARY_MANAGER;
class SYMBOL_EDITOR_SETTINGS;
class SYMBOL_EDITOR_TAB_CONTEXT;
class EDITOR_TABS_PANEL;
class EDITOR_TABS_MODEL;
class EDA_LIST_DIALOG;
class UNDO_REDO_CONTAINER;


#define UNITS_ALL _HKI( "ALL" )
#define DEMORGAN_ALL _HKI( "ALL" )
#define DEMORGAN_STD _HKI( "Standard" )
#define DEMORGAN_ALT _HKI( "Alternate" )


/**
 * The symbol library editor main window.
 */
class SYMBOL_EDIT_FRAME : public SCH_BASE_FRAME
{
    // Lets the headless tab tests drive the static undo/redo helpers without a full GUI frame.
    friend struct SYMBOL_EDITOR_TABS_TEST_FIXTURE;

public:
    SYMBOL_EDIT_FRAME( KIWAY* aKiway, wxWindow* aParent );

    ~SYMBOL_EDIT_FRAME() override;

    std::unique_ptr<GRID_HELPER> MakeGridHelper() override;

    /**
     * Switch currently used canvas ( Cairo / OpenGL).
     */
    void SwitchCanvas( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType ) override;

    /**
     * Get if any symbols or libraries have been modified but not saved.
     *
     * @return true if the any changes have not been saved
     */
    bool IsContentModified() const override;

    /**
     * Check if any pending libraries have been modified.
     *
     * This only checks for modified libraries.  If a new symbol was created and modified
     * and no libraries have been modified, the return value will be false.
     *
     * @return True if there are any pending library modifications.
     */
    bool HasLibModifications() const;

    bool CanCloseSymbolFromSchematic( bool doClose );

    /**
     * The nickname of the current library being edited and empty string if none.
     */
    wxString GetCurLib() const;

    /**
     * Set the current library nickname and returns the old library nickname.
     */
    wxString SetCurLib( const wxString& aLibNickname );

    LIB_TREE* GetLibTree() const override { return m_treePane->GetLibTree(); }

    /**
     * The tab strip fronting the shared canvas, or nullptr before it is mounted.
     */
    EDITOR_TABS_PANEL* GetTabsPanel() const { return m_tabsPanel; }

    /**
     * Cycle symbol tabs from the char hook, since GTK cannot register WXK_TAB as an accelerator.
     */
    void OnTabCharHook( wxKeyEvent& aEvent );

    /**
     * Clear the unsaved-edits flag on every tab in a saved library so its dirty indicator clears.
     */
    void clearSymbolTabsModifiedForLibrary( const wxString& aLibrary );

    /**
     * Clear a tab's dirty state on both the context (read on repaint) and the strip model (read on
     * close), keeping the two stores that diverge after a save in agreement.
     */
    static void clearTabModifiedState( SYMBOL_EDITOR_TAB_CONTEXT& aContext, EDITOR_TABS_MODEL& aModel );

    /**
     * Free a detached context's undo/redo, which the frame's own teardown path never reaches.
     */
    static void clearSymbolTabUndoRedo( SYMBOL_EDITOR_TAB_CONTEXT& aContext );

    /**
     * Free every command in the list and the UR_TRANSIENT-flagged copies it owns, which the shared
     * deleters miss. aLiveSymbol is the working symbol, guarded against to avoid a double-free.
     */
    static void freeTransientUndoCommands( UNDO_REDO_CONTAINER& aList, const LIB_SYMBOL* aLiveSymbol );

    /**
     * Return the LIB_ID of the library or symbol selected in the symbol tree.
     */
    LIB_ID GetTreeLIBID( int* aUnit = nullptr ) const;

    int GetTreeSelectionCount() const;

    int GetTreeLIBIDs( std::vector<LIB_ID>& aSelection ) const;

    /**
     * Return the current symbol being edited or NULL if none selected.
     *
     * This is a LIB_SYMBOL that I own, it is at best a copy of one in a library.
     */
    LIB_SYMBOL* GetCurSymbol() const { return m_symbol; }

    /**
     * Take ownership of aSymbol and notes that it is the one currently being edited.
     */
    void SetCurSymbol( LIB_SYMBOL* aSymbol, bool aUpdateZoom );

    LIB_SYMBOL_LIBRARY_MANAGER& GetLibManager();

    SELECTION& GetCurrentSelection() override;

    // See comments for m_SyncPinEdit.
    bool SynchronizePins();

    /**
     * Create or add an existing library to the symbol library table.
     */
    wxString AddLibraryFile( bool aCreateNew );

    /**
     * Add a library dropped file to the symbol library table.
     */
    void DdAddLibrary( wxString aLibFile );

    /**
     * Create a new symbol in the selected library.
     *
     * @param newName is the name of the symbol to derive the new symbol from or empty
     *                     to create a new root symbol.
     */
    void CreateNewSymbol( const wxString& newName = wxEmptyString );

    void ImportSymbol();
    void ExportSymbol();

    /**
     * Save the selected symbol or library.
     */
    void Save();

    /**
     * Save the currently selected symbol to a new name and/or location.
     */
    void SaveSymbolCopyAs( bool aOpenCopy );

    /**
     * Save the currently selected library to a new file.
     */
    void SaveLibraryAs();

    /**
     * Save all modified symbols and libraries.
     */
    void SaveAll();

    /**
     * Revert unsaved changes in a symbol, restoring to the last saved state.
     */
    void Revert( bool aConfirm = true );
    void RevertAll();

    void DeleteSymbolFromLibrary();

    /**
     * Update the open tab for aOldId, if any, to the renamed symbol aNewId so its label and key
     * track the rename.
     */
    void RenameSymbolTab( const LIB_ID& aOldId, const LIB_ID& aNewId );

    void CopySymbolToClipboard();

    void LoadSymbol( const wxString& aLibrary, const wxString& aSymbol, int Unit );

    /**
     * Insert a duplicate symbol.
     *
     * If \a aFromClipboard is true then action is a paste.
     */
    void DuplicateSymbol( bool aFromClipboard );

    void OnSelectUnit( wxCommandEvent& event );
    void OnSelectBodyStyle( wxCommandEvent& event );

    void ToggleProperties() override;

    void ToggleLibraryTree() override;
    bool IsLibraryTreeShown() const override;
    void FocusLibraryTreeInput() override;
    void FreezeLibraryTree();
    void ThawLibraryTree();

    bool IsSyncLibrariesInProgress() const { return m_syncLibrariesInProgress; }

    void OnUpdateUnitNumber( wxUpdateUIEvent& event );
    void OnUpdateBodyStyle( wxUpdateUIEvent& event );

    void UpdateAfterSymbolProperties( wxString* aOldName = nullptr );
    void RebuildSymbolUnitAndBodyStyleLists();

    bool canCloseWindow( wxCloseEvent& aCloseEvent ) override;
    void doCloseWindow() override;
    void OnExitKiCad( wxCommandEvent& event );

    void LoadSettings( APP_SETTINGS_BASE* aCfg ) override;
    void SaveSettings( APP_SETTINGS_BASE* aCfg ) override;

    SYMBOL_EDITOR_SETTINGS* GetSettings() const
    {
        return m_settings;
    }

    APP_SETTINGS_BASE* config() const override;

    COLOR_SETTINGS* GetColorSettings( bool aForceRefresh = false ) const override;

    /**
     * Trigger the wxCloseEvent, which is handled by the function given to EVT_CLOSE() macro:
     * <p>
     * EVT_CLOSE( SYMBOL_EDIT_FRAME::OnCloseWindow )
     * </p>
     */
    void CloseWindow( wxCommandEvent& event )
    {
        // Generate a wxCloseEvent
        Close( false );
    }

    /**
     * Must be called after a schematic change in order to set the "modify" flag of the
     * current symbol.
     */
    void OnModify() override;

    int GetUnit() const { return m_unit; }
    void SetUnit( int aUnit );

    int  GetBodyStyle() const { return m_bodyStyle; }
    void SetBodyStyle( int aBodyStyle );

    bool GetShowInvisibleFields();
    bool GetShowInvisiblePins();

    void ClearMsgPanel() override
    {
        UpdateSymbolMsgPanelInfo();
    }

    void UpdateMsgPanel() override
    {
        UpdateSymbolMsgPanelInfo();
    }

    /**
     * Update the main window title bar with the current library name and read only status
     * of the library.
     */
    void UpdateTitle();

    bool IsSymbolFromSchematic() const
    {
        // If we've already vetted closing this window, then we have no symbol anymore
        if( m_isClosing )
            return false;

        return m_isSymbolFromSchematic;
    }

    bool IsSymbolFromLegacyLibrary() const;

    /**
     * Display the documentation of the selected symbol.
     */
    void UpdateSymbolMsgPanelInfo();

    // General editing
    /**
     * Create a copy of the current symbol, and save it in the undo list.
     *
     * Because a symbol in library editor does not have a lot of primitives, the full data is
     * duplicated. It is not worth to try to optimize this save function.
     */
    void SaveCopyInUndoList( const wxString& aDescription, LIB_SYMBOL* aSymbol,
                             UNDO_REDO aUndoType = UNDO_REDO::LIBEDIT );

    void PushSymbolToUndoList( const wxString& aDescription, LIB_SYMBOL* aSymbolCopy,
                               UNDO_REDO aUndoType = UNDO_REDO::LIBEDIT );

    void GetSymbolFromUndoList();
    void GetSymbolFromRedoList();

    /**
     * Free the undo or redo list from \a aList element.
     *
     * - Wrappers are deleted.
     * - data pointed by wrappers are deleted if not in use in schematic
     *   i.e. when they are copy of a schematic item or they are no more in use (DELETED)
     *
     * @param whichList = the UNDO_REDO_CONTAINER to clear
     * @param aItemCount = the count of items to remove. < 0 for all items
     * items are removed from the beginning of the list.
     * So this function can be called to remove old commands
     */
    void ClearUndoORRedoList( UNDO_REDO_LIST whichList, int aItemCount = -1 ) override;

    /**
     * Select the currently active library and loads the symbol from \a aLibId.
     *
     * @param aLibId is the #LIB_ID of the symbol to select.
     * @param aUnit the unit to show
     * @param aBodyStyle the DeMorgan variant to show
     * @return true if the symbol defined by \a aLibId was loaded.
     */
    bool LoadSymbol( const LIB_ID& aLibId, int aUnit, int aBodyStyle );

    /**
     * Create the SVG print file for the current edited symbol.
     * @param aFullFileName is the full filename
     * @param aOffset is a plot offset, in iu
     */
    void SVGPlotSymbol( const wxString& aFullFileName, const VECTOR2I& aOffset );

    /**
     * Synchronize the library manager to the symbol library table, and then the symbol tree
     * to the library manager.  Optionally displays a progress dialog.
     */
    void SyncLibraries( bool aShowProgress, bool aPreloadCancelled = false,
                        const wxString& aForceRefresh = wxEmptyString );

    /**
     * Redisplay the library tree.  Used after changing modified states, descriptions, etc.
     */
    void RefreshLibraryTree();

    /**
     * Update a symbol node in the library tree.
     */
    void UpdateLibraryTree( const wxDataViewItem& aTreeItem, LIB_SYMBOL* aSymbol );

    /**
     * Return either the symbol selected in the symbol tree (if context menu is active) or the
     * symbol on the editor canvas.
     */
    LIB_ID GetTargetLibId() const override;

    /**
     * @return a list of selected items in the symbol tree
     */
    std::vector<LIB_ID> GetSelectedLibIds() const;

    void FocusOnLibId( const LIB_ID& aLibID );

    /**
     * Called after the preferences dialog is run.
     */
    void CommonSettingsChanged( int aFlags ) override;

    void ShowChangedLanguage() override;

    void SetScreen( BASE_SCREEN* aScreen ) override;

    const BOX2I GetDocumentExtents( bool aIncludeAllVisible = true ) const override;

    void RebuildView();

    void UpdateItem( EDA_ITEM* aItem, bool isAddOrDelete = false,
                     bool aUpdateRtree = false ) override;

    /**
     * Rebuild the GAL and redraw the screen.  Call when something went wrong.
     */
    void HardRedraw() override;

    void KiwayMailIn( KIWAY_MAIL_EVENT& mail ) override;

    void FocusOnItem( EDA_ITEM* aItem, bool aAllowScroll = true ) override;

    /**
     * Load a symbol from the schematic to edit in place.
     *
     * @param aSymbol the symbol to edit.
     */
    void LoadSymbolFromSchematic( SCH_SYMBOL* aSymbol );

    /**
     * Test if a symbol is loaded and can be edited.
     *
     * The following conditions are required for a symbol to be editable:
     *  - The symbol must selected from either a library or the schematic.
     *  - The symbol must not be from a legacy library.
     *
     * Note that many things are not editable in a non-root symbol (ie: an alias), but others
     * are so this routine no longer returns false for an alias.
     */
    bool IsSymbolEditable() const;

    bool IsSymbolAlias() const;

    ///< Return true if \a aLibId is an alias for the editor screen symbol.
    bool IsCurrentSymbol( const LIB_ID& aLibId ) const;

    ///< Restore the empty editor screen, without any symbol or library selected.
    void emptyScreen();

    void ClearToolbarControl( int aId ) override;

protected:
    void configureToolbars() override;
    void setupUIConditions() override;

    void doReCreateMenuBar() override;

    void updateSelectionFilterVisbility() override;

private:
    // Set up the tool framework
    void setupTools();

    void saveSymbolCopyAs( bool aOpenCopy );

    /**
     * Save the changes to the current library.
     *
     * A backup file of the current library is saved with the .bak extension before the
     * changes made to the library are saved.
     *
     * @param aLibrary is the library name.
     * @param aNewFile Ask for a new file name to save the library.
     * @return True if the library was successfully saved.
     */
    bool saveLibrary( const wxString& aLibrary, bool aNewFile );

    /**
     * Set the current active library to \a aLibrary.
     *
     * @param aLibrary the nickname of the library in the symbol library table.  If empty,
     *                 display list of available libraries to select from.
     */
    void SelectActiveLibrary( const wxString& aLibrary = wxEmptyString );

    /**
     * Load a symbol from the current active library, optionally setting the selected unit
     * and convert.
     *
     * @param aSymbolName The symbol alias name to load from the current library.
     * @param aUnit Unit to be selected
     * @param aBodyStyle Convert to be selected
     * @return true if the symbol loaded correctly.
     */
    bool LoadSymbolFromCurrentLib( const wxString& aSymbolName, int aUnit = 0, int aBodyStyle = 0 );

    /**
     * Create a copy of \a aLibEntry into memory.
     *
     * @param aLibEntry A pointer to the LIB_SYMBOL object to an already loaded symbol.
     * @param aLibrary the path to the library file that \a aLibEntry was loaded from.  This is
     *                 for error messaging purposes only.
     * @param aUnit the initial unit to show.
     * @param aBodyStyle the initial DeMorgan variant to show.
     * @return True if a copy of \a aLibEntry was successfully copied.
     */
    bool LoadOneLibrarySymbolAux( LIB_SYMBOL* aLibEntry, const wxString& aLibrary, int aUnit,
                                  int aBodyStyle );

    ///< Create a backup copy of a file with requested extension.
    bool backupFile( const wxFileName& aOriginalFile, const wxString& aBackupExt );

    ///< Return currently edited symbol.
    LIB_SYMBOL* getTargetSymbol() const;

    ///< Return either the library selected in the symbol tree, if context menu is active or
    ///< the library that is currently modified.
    wxString getTargetLib() const;

    void centerItemIdleHandler( wxIdleEvent& aEvent );

    /*
     * Return true when the operation has succeeded (all requested libraries have been saved
     * or none was selected and confirmed by OK).
     *
     * @param aRequireConfirmation when true, the user must be asked to confirm.
     */
    bool saveAllLibraries( bool aRequireConfirmation );

    ///< Save the current symbol.
    bool saveCurrentSymbol();

    ///< Store the currently modified symbol in the library manager buffer.
    void storeCurrentSymbol();

    ///< Rename LIB_SYMBOL aliases to avoid conflicts before adding a symbol to a library.
    void ensureUniqueName( LIB_SYMBOL* aSymbol, const wxString& aLibrary );

    /**
     * Add \a aLibFile to the symbol library table defined by \a aScope.
     *
     * @note The library defined by \a aLibFile must be a KiCad (s-expression) library.
     *
     * @param aLibFile is the full path and file name of the symbol library to add to the table.
     * @param aScope defines if \a aLibFile is added to the global or project library table.
     * @return true if successful or false if a failure occurs.
     */
    bool addLibTableEntry( const wxString& aLibFile,
                           LIBRARY_TABLE_SCOPE aScope = LIBRARY_TABLE_SCOPE::GLOBAL );

    /**
     * Replace the file path of the symbol library table entry \a aLibNickname with \a aLibFile.
     *
     * @note The library defined by \a aLibFile must be a KiCad (s-expression) library.
     *
     * @param aLibNickmane is the nickname of an existing library table entry.
     * @param aLibFile is the full path and file name of the symbol library to replace in the
     *                 table.
     * @return true if successful or false if a failure occurs.
     */
    bool replaceLibTableEntry( const wxString& aLibNickname, const wxString& aLibFile );

    /**
     * Snapshot the active tab's view and selection into its context without deleting the document.
     */
    void detachActiveSymbolTab();

    /**
     * Make aContext the active tab, borrowing its working symbol, undo/redo, view and selection,
     * without deleting the previously active document.
     */
    void activateSymbolTab( SYMBOL_EDITOR_TAB_CONTEXT* aContext );

    /**
     * Open aName from aLib in a tab, creating it when absent, and return the activated context.
     */
    SYMBOL_EDITOR_TAB_CONTEXT* findOrCreateSymbolTab( const wxString& aLib, const wxString& aName,
                                                      int aUnit, int aBodyStyle, bool aAsPreview,
                                                      bool* aWasCreated = nullptr );

    /**
     * Find or create the instance tab for a placed schematic symbol and make it active.
     *
     * Re-editing the same placed symbol focuses the existing tab; otherwise a session-only instance
     * tab is created over the supplied transient working symbol/screen, which the context adopts. The
     * caller's objects are consumed only when a new tab is created; on a focus of an existing tab they
     * are deleted here.
     */
    SYMBOL_EDITOR_TAB_CONTEXT* findOrCreateSymbolInstanceTab( LIB_SYMBOL* aSymbol, SCH_SCREEN* aScreen,
                                                              const KIID&     aSchematicSymbolUUID,
                                                              const wxString& aReference, int aUnit,
                                                              int aBodyStyle );

    /**
     * Resolve the tab context for a panel tab index, or nullptr. The panel owns tab order, so the
     * index maps through the panel's key, not into m_tabContexts directly.
     */
    SYMBOL_EDITOR_TAB_CONTEXT* symbolTabContextForIndex( int aIdx ) const;

    /**
     * Resolve the tab context for a tab key, or nullptr.
     */
    SYMBOL_EDITOR_TAB_CONTEXT* symbolTabContextForKey( const wxString& aKey ) const;

    /**
     * Drop the inactive context for aKey from m_tabContexts, freeing its undo/redo. Used when the
     * panel evicts a preview tab so its now-orphaned context is not left behind to be persisted.
     */
    void dropSymbolTabContext( const wxString& aKey );

    /**
     * Prompt for unsaved changes on the tab and drop its context. Returns false if the user cancels.
     */
    bool promptAndCloseSymbolTab( int aIdx );

    /**
     * Prompt to save each dirty instance (schematic) tab that is not the active one, since the active
     * tab's unsaved state is handled by CanCloseSymbolFromSchematic. Returns false if the user cancels
     * so the window close can be vetoed.
     */
    bool promptToSaveInactiveInstanceTabs();

    /**
     * True if any non-active instance (schematic) tab has unsaved edits. Used to veto a session-end
     * query early, since those tabs are invisible to IsContentModified (which sees only the active tab).
     */
    bool hasDirtyInactiveInstanceTabs() const;

    /**
     * Close every tab without prompting and return the frame to the empty state. Used by the paths
     * that leave the tab model. The caller handles any unsaved-change prompting.
     */
    void closeAllSymbolTabsSilently();

    /**
     * Close the open tab for aLibId, if any, without prompting and leaving the other tabs open. The
     * panel selects a successor when the closed tab was active.
     */
    void closeSymbolTab( const LIB_ID& aLibId );

    /**
     * Capture the current view zoom/center for the active tab's snapshot.
     */
    EDITOR_TAB_CONTEXT::VIEW_SNAPSHOT captureSymbolViewSnapshot() const;

    /**
     * Restore a previously captured view zoom/center.
     */
    void restoreSymbolViewSnapshot( const EDITOR_TAB_CONTEXT::VIEW_SNAPSHOT& aSnapshot );

    /**
     * Capture the KIIDs of the current selection for the active tab's snapshot.
     */
    std::vector<KIID> captureSymbolSelectionKiids() const;

    /**
     * Reselect the items named by aKiids after a reload rebuilt the view.
     */
    void restoreSymbolSelectionKiids( const std::vector<KIID>& aKiids );

    /**
     * Recreate tabs from the persisted open-tab list once the libraries have loaded.
     */
    void restoreSymbolTabsFromSettings();

    /**
     * Write the current tab set into the editor settings for the next session.
     */
    void storeSymbolTabsToSettings();

    DECLARE_EVENT_TABLE()

public:
    /**
     * Set to true to synchronize pins at the same position when editing symbols with multiple
     * units or multiple body styles.  Deleting or moving pins will affect all pins at the same
     * location.
     * When units are interchangeable, synchronizing editing of pins is usually the best way,
     * because if units are interchangeable, it implies that all similar pins are at the same
     * location.
     * When units are not interchangeable, do not synchronize editing of pins, because each symbol
     * is specific, and there are no (or few) similar pins between units.
     *
     * Setting this to false allows editing each pin per symbol or body style regardless other
     * pins at the same location. This requires the user to open each symbol or body style to make
     * changes to the other pins at the same location.
     *
     * To know if others pins must be coupled when editing a pin, use SynchronizePins() instead
     * of m_syncPinEdit, because SynchronizePins() is more reliable (takes in account the fact
     * units are interchangeable, there are more than one unit).
     *
     * @todo Determine why this member variable is public when all the rest are private and
     *       either make it private or document why it needs to be public.
     */
    bool          m_SyncPinEdit;

private:
    ///< Helper screen used when no symbol is loaded
    SCH_SCREEN*         m_dummyScreen;

    LIB_SYMBOL*         m_symbol;                // a symbol I own, it is not in any library, but a copy could be.
    wxComboBox*         m_unitSelectBox;         // a ComboBox to select a unit to edit (if the
                                                 //   symbol has multiple units)
    wxComboBox*         m_bodyStyleSelectBox;    // a ComboBox to select a body style to edit (if the symbol has
                                                 //   multiple body styles)
    SYMBOL_TREE_PANE*           m_treePane;      // symbol search tree widget
    LIB_SYMBOL_LIBRARY_MANAGER* m_libMgr;        // manager taking care of temporary modifications
    SYMBOL_EDITOR_SETTINGS*     m_settings;      // Handle to the settings

    // Tabbed editing. One context per open symbol, owning a working symbol/screen copy plus its
    // per-tab undo/redo and view/selection snapshot. The active tab lends its objects to the frame
    // and reclaims them on detach. m_activeTab is the context currently bound to the frame.
    std::vector<std::unique_ptr<SYMBOL_EDITOR_TAB_CONTEXT>> m_tabContexts;
    SYMBOL_EDITOR_TAB_CONTEXT*                              m_activeTab = nullptr;
    EDITOR_TABS_PANEL*                                      m_tabsPanel = nullptr;

    // Guards against re-entering tab activation while a tab is being created or restored, since
    // AddTab() activates synchronously.
    bool m_loadingSymbolTab = false;

    // While true, promptAndCloseSymbolTab() skips the unsaved-changes dialog.
    bool m_silentSymbolTabClose = false;

    LIB_ID                      m_centerItemOnIdle;

    // The unit number to edit and show
    int         m_unit;

    // Show the normal shape (m_bodyStyle <= 1) or the DeMorgan converted shape (m_bodyStyle > 1)
    int         m_bodyStyle;

    ///< Flag if the symbol being edited was loaded directly from a schematic.
    bool        m_isSymbolFromSchematic;
    KIID        m_schematicSymbolUUID;

     ///< RefDes of the symbol (only valid if symbol was loaded from schematic)
    wxString    m_reference;

    // True to force DeMorgan/normal tools selection enabled.
    // They are enabled when the loaded symbol has graphic items for converted shape
    // But under some circumstances (New symbol created) these tools must left enabled
    static bool m_showDeMorgan;

    // Guard against re-entrant SyncLibraries calls.  The progress dialog used during sync
    // yields the event loop, which can dispatch queued UI events (e.g. menu clicks that
    // accumulated while the app was busy).  Without this guard, opening the symbol library
    // table dialog from within an active SyncLibraries call corrupts the library tree.
    bool        m_syncLibrariesInProgress;
};

#endif  // SYMBOL_EDIT_FRAME_H
