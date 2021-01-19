/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FOOTPRINT_EDIT_FRAME_H
#define FOOTPRINT_EDIT_FRAME_H

#include <pcb_base_frame.h>
#include <pcb_base_edit_frame.h>
#include <io_mgr.h>
#include <config_params.h>
#include <fp_tree_synchronizing_adapter.h>

class PCB_LAYER_BOX_SELECTOR;
class FP_LIB_TABLE;
class FOOTPRINT_TREE_PANE;
class SYMBOL_LIBRARY_MANAGER;
class FOOTPRINT_EDITOR_SETTINGS;

namespace PCB { struct IFACE; }     // A KIFACE_I coded in pcbnew.c


class FOOTPRINT_EDIT_FRAME : public PCB_BASE_EDIT_FRAME
{
    friend struct PCB::IFACE;

    FOOTPRINT_TREE_PANE*        m_treePane;

    wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER> m_adapter;

    std::unique_ptr<FOOTPRINT>  m_revertModule;
    wxString                    m_footprintNameWhenLoaded;

    int                         m_defaultLibWidth;

public:

    ~FOOTPRINT_EDIT_FRAME();

    /**
     * Function GetFootprintEditorFrameName (static)
     * @return the frame name used when creating the frame
     * used to get a reference to this frame, if exists
     */
    static const wxChar* GetFootprintEditorFrameName();

    ///> @copydoc PCB_BASE_FRAME::GetModel()
    BOARD_ITEM_CONTAINER* GetModel() const override;
    SELECTION&            GetCurrentSelection() override;

    /**
     * Get if any footprints or libraries have been modified but not saved.
     *
     * @return true if the any changes have not been saved
     */
    bool IsContentModified() override;

    bool IsCurrentFPFromBoard() const;

    FOOTPRINT_EDITOR_SETTINGS* GetSettings();

    APP_SETTINGS_BASE* config() const override;

    BOARD_DESIGN_SETTINGS& GetDesignSettings() const override;

    const PCB_PLOT_PARAMS& GetPlotSettings() const override;
    void SetPlotSettings( const PCB_PLOT_PARAMS& aSettings ) override;

    MAGNETIC_SETTINGS* GetMagneticItemsSettings() override;

    void LoadSettings( APP_SETTINGS_BASE* aCfg ) override;
    void SaveSettings( APP_SETTINGS_BASE* aCfg ) override;

    COLOR_SETTINGS* GetColorSettings() const override;

    const BOX2I GetDocumentExtents( bool aIncludeAllVisible = true ) const override;

    bool canCloseWindow( wxCloseEvent& Event ) override;
    void doCloseWindow() override;
    void CloseFootprintEditor( wxCommandEvent& Event );
    void OnExitKiCad( wxCommandEvent& aEvent );

    /**
     * switches currently used canvas (Cairo / OpenGL).
     * It also reinit the layers manager that slightly changes with canvases
     */
    void SwitchCanvas( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType ) override;

    /**
     * Update the layer manager and other widgets from the board setup
     * (layer and items visibility, colors ...)
     */
    void UpdateUserInterface();

    /**
     * Refresh the library tree and redraw the window
     */
    void HardRedraw() override;

    /**
     * Create the main horizontal toolbar for the footprint editor.
     */
    void ReCreateHToolbar() override;

    void ReCreateVToolbar() override;
    void ReCreateOptToolbar() override;

    /**
     * @brief (Re)Create the menubar for the Footprint Editor frame
     */
    void ReCreateMenuBar() override;

    /**
     * Re create the layer Box by clearing the old list, and building
     * le new one, from the new layers names and cole layers
     * @param aForceResizeToolbar = true to resize the parent toolbar
     * false if not needed (mainly in parent toolbar creation,
     * or when the layers names are not modified)
     */
    void ReCreateLayerBox( bool aForceResizeToolbar = true );

    void OnUpdateLayerSelectBox( wxUpdateUIEvent& aEvent );

    void SelectLayer( wxCommandEvent& event );

    // The Tool Framework initalization, for GAL mode
    void setupTools();

    void OnSaveFootprintAsPng( wxCommandEvent& event );

    bool IsSearchTreeShown();
    void ToggleSearchTree();

    /**
     * Save a library to a new name and/or library type.
     *
     * @note Saving as a new library type requires the plug-in to support saving libraries
     * @see PLUGIN::FootprintSave and PLUGIN::FootprintLibCreate
     */
    bool SaveLibraryAs( const wxString& aLibraryPath );

    void OnUpdateModuleSelected( wxUpdateUIEvent& aEvent );
    void OnUpdateLoadFootprintFromBoard( wxUpdateUIEvent& aEvent );
    void OnUpdateSaveFootprintToBoard( wxUpdateUIEvent& aEvent );

    ///> @copydoc PCB_BASE_EDIT_FRAME::OnEditItemRequest()
    void OnEditItemRequest( BOARD_ITEM* aItem ) override;

    /**
     * Called from the main toolbar to load a footprint from board mainly to edit it.
     */
    void LoadFootprintFromBoard( wxCommandEvent& event );

    void SaveFootprintToBoard( wxCommandEvent& event );

    void LoadFootprintFromLibrary( LIB_ID aFPID );

    /**
     * Returns the adapter object that provides the stored data.
     */
    wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER>& GetLibTreeAdapter() { return m_adapter; }

    /**
     * Save in an existing library a given footprint.
     *
     * @param aFootprint = the given footprint
     * @return : true if OK, false if abort
     */
    bool SaveFootprint( FOOTPRINT* aFootprint );
    bool SaveFootprintAs( FOOTPRINT* aFootprint );
    bool SaveFootprintToBoard( bool aAddNew );
    bool SaveFootprintInLibrary( FOOTPRINT* aFootprint, const wxString& aLibraryName );
    bool RevertFootprint();

    /**
     * Must be called after a footprint change
     * in order to set the "modify" flag of the current screen
     * and prepare, if needed the refresh of the 3D frame showing the footprint
     * do not forget to call the basic OnModify function to update auxiliary info
     */
    void OnModify() override;

    // BOARD handling

    /**
     * Delete all and reinitialize the current board.
     *
     * @param aQuery = true to prompt user for confirmation, false to initialize silently
     */
    bool Clear_Pcb( bool aQuery );

    /// Return the LIB_ID of the part or library selected in the footprint tree.
    LIB_ID GetTreeFPID() const;

    LIB_TREE_NODE* GetCurrentTreeNode() const;

    /// Return the LIB_ID of the part being edited.
    LIB_ID GetLoadedFPID() const;

    /// Return the LIB_ID of the part selected in the footprint tree, or the loaded part if
    /// there is no selection in the tree.
    LIB_ID GetTargetFPID() const;

    void ClearModify();

    // importing / exporting Footprint
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
    FOOTPRINT* ImportFootprint( const wxString& aName = wxT( "") );

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

    ///> @copydoc PCB_BASE_FRAME::SetActiveLayer()
    void SetActiveLayer( PCB_LAYER_ID aLayer ) override;

    ///> @copydoc PCB_BASE_FRAME::OnUpdateLayerAlpha()
    void OnUpdateLayerAlpha( wxUpdateUIEvent& aEvent ) override;

    void OnDisplayOptionsChanged() override;

    ///> @copydoc EDA_DRAW_FRAME::UseGalCanvas()
    void ActivateGalCanvas() override;

    /**
     * Load a KiCad board (.kicad_pcb) from \a aFileName.
     *
     * @param aFileSet - hold the BOARD file to load, a vector of one element.
     * @param aCtl      - KICTL_ bits, one to indicate that an append of the board file
     *                      aFileName to the currently loaded file is desired.
     *                    @see #KIWAY_PLAYER for bit defines.
     * @return bool - false if file load fails, otherwise true.
     */
    bool OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl = 0 ) override;

    /**
     * Override from PCB_BASE_EDIT_FRAME which adds a footprint to the editor's dummy board,
     * NOT to the user's PCB.
     */
    void AddFootprintToBoard( FOOTPRINT* aFootprint ) override;

    /**
     * Allows Footprint Editor to install its preferences panel into the preferences dialog.
     */
    void InstallPreferences( PAGED_DIALOG* aParent, PANEL_HOTKEYS_EDITOR* aHotkeysPanel ) override;

    /**
     * Update visible items after a language change.
     */
    void ShowChangedLanguage() override;

    /**
     * Called after the preferences dialog is run.
     */
    void CommonSettingsChanged( bool aEnvVarsChanged, bool aTextVarsChanged ) override;

    /**
     * Synchronize the footprint library tree to the current state of the footprint library
     * table.
     * @param aProgress
     */
    void SyncLibraryTree( bool aProgress );

    /**
     * Filter, sort, and redisplay the library tree.  Does NOT synchronize it with libraries
     * in disk.
     */
    void RegenerateLibraryTree();

    void RefreshLibraryTree();

    /// Reloads displayed items and sets view.
    void UpdateView();

    void FocusOnLibID( const LIB_ID& aLibID );

    void KiwayMailIn( KIWAY_EXPRESS& mail ) override;

    DECLARE_EVENT_TABLE()

protected:
    /// protected so only friend PCB::IFACE::CreateWindow() can act as sole factory.
    FOOTPRINT_EDIT_FRAME( KIWAY* aKiway, wxWindow* aParent, EDA_DRAW_PANEL_GAL::GAL_TYPE aBackend );

    PCB_LAYER_BOX_SELECTOR* m_selLayerBox;  // a combo box to display and select active layer

    FOOTPRINT_EDITOR_SETTINGS* m_settings;

    /**
     * Make sure the footprint info list is loaded (with a progress dialog) and then initialize
     * the footprint library tree.
     */
    void initLibraryTree();

    /**
     * Updates window title according to getLibNickName().
     */
    void updateTitle();

    void restoreLastFootprint();
    void retainLastFootprint();

    /**
     * Run the Footprint Properties dialog and handle changes made in it.
     */
    void editFootprintProperties( FOOTPRINT* aFootprint );

    void setupUIConditions() override;
};

#endif      // FOOTPRINT_EDIT_FRAME_H
