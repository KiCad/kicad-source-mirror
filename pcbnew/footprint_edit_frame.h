/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef FOOTPRINT_EDIT_FRAME_H
#define FOOTPRINT_EDIT_FRAME_H

#include <functional>
#include <memory>
#include <vector>

#include <pcb_base_frame.h>
#include <pcb_base_edit_frame.h>
#include <pcb_io/pcb_io_mgr.h>
#include <widgets/lib_tree.h>
#include <footprint_tree_pane.h>
#include <fp_tree_synchronizing_adapter.h>

class PCB_LAYER_BOX_SELECTOR;
class FP_LIB_TABLE;
class FOOTPRINT_TREE_PANE;
class SYMBOL_LIBRARY_MANAGER;
class FOOTPRINT_EDITOR_SETTINGS;
class EDA_LIST_DIALOG;
class EDITOR_TABS_PANEL;
class FOOTPRINT_EDITOR_TAB_CONTEXT;
class UNDO_REDO_CONTAINER;

class API_HANDLER_FOOTPRINT;
class API_HANDLER_COMMON;

namespace PCB { struct IFACE; }     // A KIFACE coded in pcbnew.cpp


class FOOTPRINT_EDIT_FRAME : public PCB_BASE_EDIT_FRAME
{
public:
    ~FOOTPRINT_EDIT_FRAME();

    /**
     * @return the frame name used when creating the frame used to get a reference to this
     *         frame, if exists
     */
    static const wxChar* GetFootprintEditorFrameName();

    /**
     * Replace the tab context at @p aSlot with @p aNew, running @p aInstallSuccessor before the
     * displaced context is destroyed.
     *
     * @p aInstallSuccessor repoints the canvas VIEW at the incoming board; the displaced context is
     * passed in so callers can keep the old board protected during install.
     *
     * Exposed for unit testing of the reuse ordering.
     *
     * @return the raw pointer to the newly installed context.
     */
    static FOOTPRINT_EDITOR_TAB_CONTEXT* placeReusedTabContext(
            std::vector<std::unique_ptr<FOOTPRINT_EDITOR_TAB_CONTEXT>>& aContexts, int aSlot,
            std::unique_ptr<FOOTPRINT_EDITOR_TAB_CONTEXT> aNew,
            const std::function<void( const FOOTPRINT_EDITOR_TAB_CONTEXT& )>& aInstallSuccessor );

    ///< @copydoc PCB_BASE_FRAME::GetModel()
    BOARD_ITEM_CONTAINER* GetModel() const override;
    SELECTION&            GetCurrentSelection() override;

    /**
     * Get if any footprints or libraries have been modified but not saved.
     *
     * @return true if the any changes have not been saved
     */
    bool IsContentModified() const override;

    bool IsCurrentFPFromBoard() const;

    bool CanCloseFPFromBoard( bool doClose );

    FOOTPRINT_EDITOR_SETTINGS* GetSettings();

    /**
     * Return the angle used for rotate operations.
     */
    EDA_ANGLE GetRotationAngle() const override;

    APP_SETTINGS_BASE* config() const override;

    BOARD_DESIGN_SETTINGS& GetDesignSettings() const override;

    const PCB_PLOT_PARAMS& GetPlotSettings() const override;
    void SetPlotSettings( const PCB_PLOT_PARAMS& aSettings ) override;

    MAGNETIC_SETTINGS* GetMagneticItemsSettings() override;

    void LoadSettings( APP_SETTINGS_BASE* aCfg ) override;
    void SaveSettings( APP_SETTINGS_BASE* aCfg ) override;

    COLOR_SETTINGS* GetColorSettings( bool aForceRefresh = false ) const override;

    std::unique_ptr<GRID_HELPER> MakeGridHelper() override;

    const BOX2I GetDocumentExtents( bool aIncludeAllVisible = true ) const override;

    bool canCloseWindow( wxCloseEvent& Event ) override;
    void doCloseWindow() override;
    void CloseFootprintEditor( wxCommandEvent& Event );
    void OnExitKiCad( wxCommandEvent& aEvent );

    /**
     * Switch the currently used canvas (Cairo / OpenGL).
     *
     * It also reinitializes the layer manager that slightly changes with canvases.
     */
    void SwitchCanvas( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType ) override;

    /**
     * Update the layer manager and other widgets from the board setup (layer and items
     * visibility, colors ...).
     */
    void UpdateUserInterface();

    ///< @copydoc EDADRAW_FRAME::UpdateMsgPanel
    void UpdateMsgPanel() override;

    /**
     * Refresh the library tree and redraw the window
     */
    void HardRedraw() override;

    /**
     * Re create the layer Box by clearing the old list, and building a new one from the new
     * layers names and layer colors..
     *
     * @param aForceResizeToolbar true to resize the parent toolbar or false if not needed
     *                            (mainly in parent toolbar creation or when the layers names
     *                            are not modified).
     */
    void ReCreateLayerBox( bool aForceResizeToolbar = true );

    // The Tool Framework initialization, for GAL mode
    void setupTools();

    void OnSaveFootprintAsPng( wxCommandEvent& event );

    bool IsLibraryTreeShown() const override;
    void ToggleLibraryTree() override;
    void FocusLibraryTreeInput() override;

    void ToggleLayersManager();

    /**
     * Save a library to a new name and/or library type.
     *
     * @see #PCB_IO::FootprintSave and #IO_BASE::LibraryCreate
     *
     * @note Saving as a new library type requires the plug-in to support saving libraries
     */
    bool SaveLibraryAs( const wxString& aLibraryPath );

    ///< @copydoc PCB_BASE_EDIT_FRAME::OnEditItemRequest()
    void OnEditItemRequest( BOARD_ITEM* aItem ) override;

    void LoadFootprintFromLibrary( LIB_ID aFPID );

    /**
     * Prepare the editor for a new footprint, returning false if the user cancels.
     *
     * With tabs and a target library the new footprint opens in its own tab and the other tabs are
     * left intact. Otherwise it falls back to the legacy single-board clear.
     */
    bool BeginNewFootprint( const wxString& aLibrary );

    /**
     * Close the open tab for aFPID, if any, without prompting and leaving the other tabs open. The
     * panel selects a successor when the closed tab was active.
     */
    void CloseFootprintTab( const LIB_ID& aFPID );

    /**
     * Update the open tab for aOldId, if any, to the renamed footprint aNewId so its label and key
     * track the rename.
     */
    void RenameFootprintTab( const LIB_ID& aOldId, const LIB_ID& aNewId );

    /**
     * Return the adapter object that provides the stored data.
     */
    wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER>& GetLibTreeAdapter() { return m_adapter; }

    /**
     * Save in an existing library a given footprint.
     *
     * @param aFootprint = the given footprint
     * @return : true if OK, false if abort
     */
    bool SaveFootprint( FOOTPRINT* aFootprint );
    bool DuplicateFootprint( FOOTPRINT* aFootprint );
    bool SaveFootprintAs( FOOTPRINT* aFootprint );
    bool SaveFootprintToBoard( bool aAddNew );
    bool SaveFootprintInLibrary( FOOTPRINT* aFootprint, const wxString& aLibraryName );
    bool RevertFootprint();

    /**
     * Must be called after a footprint change in order to set the "modify" flag of the
     * current screen and prepare, if needed the refresh of the 3D frame showing the footprint.
     *
     * Do not forget to call the basic OnModify function to update auxiliary info.
     */
    void OnModify() override;

    /**
     * Delete all and reinitialize the current board.
     *
     * @param doAskAboutUnsavedChanges = true to prompt user for confirmation if existing board
     *                                   contains unsaved changes, false to re-initialize silently
     */
    bool Clear_Pcb( bool doAskAboutUnsavedChanges );

    /// Return the LIB_ID of the part being edited.
    LIB_ID GetLoadedFPID() const;

    /// Return the LIB_ID of the part selected in the footprint tree, or the loaded part if
    /// there is no selection in the tree.
    LIB_ID GetTargetFPID() const;

    void ClearModify();

    /**
     * Create a file containing only one footprint.
     */
    void ExportFootprint( FOOTPRINT* aFootprint );

    /**
     * Read a file containing only one footprint.
     *
     * The import function can also read gpcb footprint file, in Newlib format.
     * (One footprint per file, Newlib files have no special ext.)
     */
    FOOTPRINT* ImportFootprint( const wxString& aName = wxT( "" ) );

    /**
     * Load a footprint from the main board into the Footprint Editor.
     *
     * @param aFootprint = the footprint to load. If NULL, the user will be asked for a
     *                     footprint reference.
     * @return true if a footprint is loaded.
     */
    bool LoadFootprintFromBoard( FOOTPRINT* aFootprint );

    /**
     * Display the list of footprints currently existing on the BOARD.
     *
     * @return the selected footprint or nullptr
     */
    FOOTPRINT* SelectFootprintFromBoard( BOARD* aPcb );

    /**
     * Delete the given footprint from its library.
     */
    bool DeleteFootprintFromLibrary( const LIB_ID& aFPID, bool aConfirm );

    /**
     * @return the color of the grid
     */
    COLOR4D GetGridColor() override;

    ///< @copydoc PCB_BASE_FRAME::SetActiveLayer()
    void SetActiveLayer( PCB_LAYER_ID aLayer ) override;

    void OnDisplayOptionsChanged() override;

    ///< @copydoc EDA_DRAW_FRAME::UseGalCanvas()
    void ActivateGalCanvas() override;

    /**
     * Load a KiCad board (.kicad_pcb) from \a aFileName.
     *
     * @param aFileSet is the BOARD file to load, a vector of one element.
     * @param aCtl is the KICTL_ bits, one to indicate that an append of the board file
     *             \a  aFileName to the currently loaded file is desired.  @see #KIWAY_PLAYER
     *             for bit defines.
     * @return false if file load fails, otherwise true.
     */
    bool OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl = 0 ) override;

    /**
     * Override from PCB_BASE_EDIT_FRAME which adds a footprint to the editor's dummy board,
     * NOT to the user's PCB.
     */
    void AddFootprintToBoard( FOOTPRINT* aFootprint ) override;

    /**
     * Override from PCB_BASE_FRAME which reloads the footprint from the library without
     * setting the footprint watcher
     */
    void ReloadFootprint( FOOTPRINT* aFootprint ) override;

    /**
     * Update visible items after a language change.
     */
    void ShowChangedLanguage() override;

    /**
     * Called after the preferences dialog is run.
     */
    void CommonSettingsChanged( int aFlags ) override;

    LIB_TREE* GetLibTree() const override
    {
        return m_treePane->GetLibTree();
    }

    /**
     * Synchronize the footprint library tree to the current state of the footprint library
     * table.
     */
    void SyncLibraryTree( [[maybe_unused]] bool aProgress );

    /**
     * Redisplay the library tree.  Used after changing modified states, descriptions, etc.
     */
    void RefreshLibraryTree();

    /**
     * Update a single node in the library tree.
     */
    void UpdateLibraryTree( const wxDataViewItem& treeItem, FOOTPRINT* aFootprint );

    ///< Reload displayed items and sets view.
    void UpdateView();

    void UpdateTitle();

    void FocusOnLibID( const LIB_ID& aLibID );

    void KiwayMailIn( KIWAY_MAIL_EVENT& mail ) override;

    /**
     * Return the tabbed-document strip, or nullptr if the tabs UI is not mounted.
     */
    EDITOR_TABS_PANEL* GetTabsPanel() const { return m_tabsPanel; }

    /**
     * Advance the active tab forward or backward in MRU order.
     */
    void AdvanceFootprintTab( bool aForward );

    /**
     * Request closing the active tab, prompting to save if it is modified.
     */
    void CloseActiveFootprintTab();

    DECLARE_EVENT_TABLE()

protected:
    /// protected so only friend PCB::IFACE::CreateWindow() can act as sole factory.
    FOOTPRINT_EDIT_FRAME( KIWAY* aKiway, wxWindow* aParent );

    /**
     * Determines the Canvas type to load (with prompt if required) and initializes m_canvasType
     */
    void resolveCanvasType() override;

    /**
     * Make sure the footprint info list is loaded (with a progress dialog) and then initialize
     * the footprint library tree.
     */
    void initLibraryTree();

    void restoreLastFootprint();
    void retainLastFootprint();

    void updateEnabledLayers();

    /**
     * @brief (Re)Create the menubar for the Footprint Editor frame
     */
    void doReCreateMenuBar() override;

    /**
     * Run the Footprint Properties dialog and handle changes made in it.
     */
    void editFootprintProperties( FOOTPRINT* aFootprint );

    void setupUIConditions() override;

    void centerItemIdleHandler( wxIdleEvent& aEvent );

    /**
     * Cycle footprint tabs from the CHAR_HOOK stream, since GTK cannot register WXK_TAB as a menu
     * accelerator.
     *
     * Yields to the appearance panel's preset cycle when that panel holds focus.
     */
    void OnTabCharHook( wxKeyEvent& aEvent );

protected:
    FOOTPRINT_EDITOR_SETTINGS*  m_editorSettings;

private:
    friend struct PCB::IFACE;

    /**
     * Find or create the tab for aLibId over its own fp-holder board and make it the active tab.
     */
    FOOTPRINT_EDITOR_TAB_CONTEXT* findOrCreateFootprintTab( const LIB_ID& aLibId, bool aAsPreview );

    /**
     * Find or create the instance tab for a placed board footprint and make it the active tab.
     *
     * Re-editing the same board footprint focuses the existing tab; otherwise a session-only instance
     * tab is created over a fresh fp-holder board carrying @p aBoardFootprint cloned, and the
     * editor-to-board UUID remap is recorded on the new tab for save-back. The clone is created here;
     * the caller retains ownership of @p aBoardFootprint.
     */
    FOOTPRINT_EDITOR_TAB_CONTEXT* findOrCreateFootprintInstanceTab( FOOTPRINT* aBoardFootprint );

    /**
     * Make aCtx the active tab, borrowing its board without deleting the outgoing one.
     */
    void activateFootprintTab( FOOTPRINT_EDITOR_TAB_CONTEXT* aCtx );

    /**
     * Install aBoard as the active document non-destructively, wiring the tool environment and
     * resetting the tools before the caller frees any outgoing context.
     *
     * aCtx is null for a frame-owned empty board. Never calls the base SetBoard or frees a board
     * m_pcb references.
     */
    void installFootprintTabBoard( FOOTPRINT_EDITOR_TAB_CONTEXT* aCtx, BOARD* aBoard );

    /**
     * Snapshot the active tab's view/selection, fold the dirty state back into it, and clear
     * m_activeTab.
     *
     * The board borrow stays until the next borrowBoardNonDestructive swaps it.
     */
    void detachActiveFootprintTab();

    /**
     * Swap the frame's borrowed board pointer to aBoard without deleting the previous board, which
     * a tab context owns on a tab switch.
     */
    void borrowBoardNonDestructive( BOARD* aBoard );

    /**
     * Prompt to save if needed, then drop the tab context at aIdx. Returns false to veto.
     */
    bool promptAndCloseFootprintTab( int aIdx );

    /**
     * Prompt to save each dirty instance (board) tab that is not the active one, since the active
     * tab's unsaved state is handled by the main canCloseWindow check. Returns false if the user
     * cancels so the window close can be vetoed.
     */
    bool promptToSaveInactiveInstanceTabs();

    /**
     * True if any non-active instance (board) tab has unsaved edits. Used to veto a session-end query
     * early, since those tabs are invisible to IsContentModified (which sees only the active tab).
     */
    bool hasDirtyInactiveInstanceTabs() const;

    /**
     * Free the transient board items a detached context's lists own before it is destroyed, which
     * the bare container destructor leaks.
     */
    void freeFootprintTabUndoRedo( FOOTPRINT_EDITOR_TAB_CONTEXT& aCtx );

    /**
     * Free both the transient board items and the command wrappers in the given lists.
     *
     * The frame's ClearUndoRedoList() frees only the wrappers, so use this to discard a tab's
     * history.
     */
    void freeUndoRedoCommandsWithItems( UNDO_REDO_CONTAINER& aUndo, UNDO_REDO_CONTAINER& aRedo );

    /**
     * Re-query and re-render the tab labels from the active context's modified state.
     */
    void refreshFootprintTabState();

    /**
     * Install aReplacement as the active document non-destructively, then drop every tab context
     * and free the boards they own.
     *
     * Installing the successor before freeing keeps tools off a freed board during a full clear.
     * The frame owns aReplacement afterwards like the bootstrap board.
     */
    void detachTabsForFullClear( BOARD* aReplacement );

    /**
     * True when a tab context owns the frame's borrowed board, so the frame must not delete it.
     */
    bool activeBoardOwnedByTab() const;

    FOOTPRINT_TREE_PANE*        m_treePane;
    LIB_ID                      m_centerItemOnIdle;

    wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER> m_adapter;

    bool                        m_show_layer_manager_tools;

    std::unique_ptr<FOOTPRINT>  m_originalFootprintCopy;
    wxString                    m_footprintNameWhenLoaded;
    std::map<KIID, KIID>        m_boardFootprintUuids;

    EDITOR_TABS_PANEL*          m_tabsPanel;
    FOOTPRINT_EDITOR_TAB_CONTEXT* m_activeTab;
    std::vector<std::unique_ptr<FOOTPRINT_EDITOR_TAB_CONTEXT>> m_tabContexts;

    /// Set while a full clear tears the tab strip down so the panel's close-driven re-activation does
    /// not re-borrow a tab-owned board about to be freed.
    bool                        m_suppressTabActivation;

    /// Board still owned by a displaced preview context while AddTab() synchronously installs its successor.
    BOARD*                      m_protectedBorrowedBoard;

    /// While true, promptAndCloseFootprintTab() skips the unsaved-changes dialog.
    bool m_silentFootprintTabClose;

    std::unique_ptr<API_HANDLER_FOOTPRINT> m_apiHandler;
    std::unique_ptr<API_HANDLER_COMMON>    m_apiHandlerCommon;
};

#endif      // FOOTPRINT_EDIT_FRAME_H
