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

class PCB_LAYER_WIDGET;
class FP_LIB_TABLE;
class EDGE_MODULE;
class FOOTPRINT_TREE_PANE;
class LIB_MANAGER;

namespace PCB { struct IFACE; }     // A KIFACE_I coded in pcbnew.c


class FOOTPRINT_EDIT_FRAME : public PCB_BASE_EDIT_FRAME
{
    friend struct PCB::IFACE;

    FOOTPRINT_TREE_PANE*        m_treePane;
    LIB_TREE_MODEL_ADAPTER::PTR m_adapter;

    std::unique_ptr<MODULE>     m_revertModule;
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

    ///> @copydoc PCB_BASE_EDIT_FRAME::GetModel()
    BOARD_ITEM_CONTAINER* GetModel() const override;

    bool IsCurrentFPFromBoard() const;

    BOARD_DESIGN_SETTINGS& GetDesignSettings() const override;
    void SetDesignSettings( const BOARD_DESIGN_SETTINGS& aSettings ) override;

    const PCB_PLOT_PARAMS& GetPlotSettings() const override;
    void SetPlotSettings( const PCB_PLOT_PARAMS& aSettings ) override;

    void LoadSettings( wxConfigBase* aCfg ) override;
    void SaveSettings( wxConfigBase* aCfg ) override;

    double BestZoom() override;

    /**
     * Return the footprint editor settings list.
     *
     * Currently, only the settings that are needed at start
     * up by the main window are defined here.  There are other locally used
     * settings that are scattered throughout the Pcbnew source code.  If you need
     * to define a configuration setting that needs to be loaded at run time,
     * this is the place to define it.
     *
     * @return - Reference to the list of applications settings.
     */
    PARAM_CFG_ARRAY& GetConfigurationSettings();

    void OnCloseWindow( wxCloseEvent& Event ) override;
    void CloseModuleEditor( wxCommandEvent& Event );
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

    void Process_Special_Functions( wxCommandEvent& event );

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
     * @brief (Re)Create the menubar for the module editor frame
     */
    void ReCreateMenuBar() override;

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
    void OnUpdateLoadModuleFromBoard( wxUpdateUIEvent& aEvent );
    void OnUpdateInsertModuleInBoard( wxUpdateUIEvent& aEvent );

    ///> @copydoc PCB_BASE_EDIT_FRAME::OnEditItemRequest()
    void OnEditItemRequest( BOARD_ITEM* aItem ) override;

    /**
     * Called from the main toolbar to load a footprint from board mainly to edit it.
     */
    void LoadModuleFromBoard( wxCommandEvent& event );

    void LoadModuleFromLibrary( LIB_ID aFPID );

    /**
     * Returns the adapter object that provides the stored data.
     */
    LIB_TREE_MODEL_ADAPTER::PTR& GetLibTreeAdapter() { return m_adapter; }

    /**
     * Save in an existing library a given footprint.
     *
     * @param aModule = the given footprint
     * @return : true if OK, false if abort
     */
    bool SaveFootprint( MODULE* aModule );
    bool SaveFootprintAs( MODULE* aModule );
    bool SaveFootprintToBoard( bool aAddNew );
    bool SaveFootprintInLibrary( MODULE* aModule, const wxString& aLibraryName );
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

    /// Return the LIB_ID of the part being edited.
    LIB_ID GetLoadedFPID() const;

    /// Return the LIB_ID of the part selected in the footprint tree, or the loaded part if
    /// there is no selection in the tree.
    LIB_ID GetTargetFPID() const;

    /**
     * Perform a geometric transform on the current footprint.
     */
    void Transform( MODULE* module, int transform );

    // importing / exporting Footprint
    /**
     * Create a file containing only one footprint.
     *
     * Used to export a footprint
     * Exported files  have the standard ext .emp
     * This is the same format as .mod files but restricted to only one footprint
     * So Create a new lib (which will contains one module) and export a footprint
     * is basically the same thing
     * @param aModule = the module to export
     */
    void Export_Module( MODULE* aModule );

    /**
     * Read a file containing only one footprint.
     *
     * Used to import (after exporting) a footprint
     * Exported files  have the standard ext .emp
     * This is the same format as .mod files but restricted to only one footprint
     * The import function can also read gpcb footprint file, in Newlib format
     * (One footprint per file, Newlib files have no special ext.)
     */
    MODULE* Import_Module( const wxString& aName = wxT("") );

    /**
     * Load in Modedit a footprint from the main board.
     *
     * @param Module = the module to load. If NULL, a module reference will we asked to user
     * @return true if a module isloaded, false otherwise.
     */
    bool Load_Module_From_BOARD( MODULE* Module );

    /**
     * Display the list of modules currently existing on the BOARD.
     *
     * @return a pointer to a module if this module is selected or NULL otherwise
     * @param aPcb = the board from modules can be loaded
     */
    MODULE* SelectFootprintFromBoard( BOARD* aPcb );

    // functions to edit footprint edges

    /**
     * Delete the given module from its library.
     */
    bool DeleteModuleFromLibrary( const LIB_ID& aFPID, bool aConfirm );

    /**
     * Test whether a given element category is visible.
     *
     * @param aElement is from the enum by the same name
     * @return bool - true if the element is visible.
     * @see enum PCB_LAYER_ID
     */
    bool IsElementVisible( GAL_LAYER_ID aElement ) const;

    /**
     * Function SetElementVisibility
     * changes the visibility of an element category
     * @param aElement is from the enum by the same name
     * @param aNewState = The new visibility state of the element category
     * @see enum PCB_LAYER_ID
     */
    void SetElementVisibility( GAL_LAYER_ID aElement, bool aNewState );

    /**
     * @return true if the grid must be shown
     */
    bool IsGridVisible() const override;

    /**
     * It may be overloaded by derived classes
     * if you want to store/retrieve the grid visibility in configuration.
     * @param aVisible = true if the grid must be shown
     */
    void SetGridVisibility( bool aVisible ) override;

    /**
     * @return the color of the grid
     */
    COLOR4D GetGridColor() override;

    ///> @copydoc PCB_BASE_FRAME::SetActiveLayer()
    void SetActiveLayer( PCB_LAYER_ID aLayer ) override;

    ///> @copydoc PCB_BASE_FRAME::OnUpdateLayerAlpha()
    void OnUpdateLayerAlpha( wxUpdateUIEvent& aEvent ) override;

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
     * Override from PCB_BASE_EDIT_FRAME which adds a module to the editor's dummy board,
     * NOT to the user's PCB.
     */
    void AddModuleToBoard( MODULE* module ) override;

    /**
     * Allows Modedit to install its preferences panel into the preferences dialog.
     */
    void InstallPreferences( PAGED_DIALOG* aParent, PANEL_HOTKEYS_EDITOR* aHotkeysPanel ) override;

    void ReFillLayerWidget();

    /**
     * Update visible items after a language change.
     */
    void ShowChangedLanguage() override;

    /**
     * Called after the preferences dialog is run.
     */
    void CommonSettingsChanged( bool aEnvVarsChanged ) override;

    /**
     * Synchronize the footprint library tree to the current state of the footprint library
     * table.
     * @param aProgress
     */
    void SyncLibraryTree( bool aProgress );
    void FocusOnLibID( const LIB_ID& aLibID );

    void KiwayMailIn( KIWAY_EXPRESS& mail ) override;

    void SyncToolbars() override;

    DECLARE_EVENT_TABLE()

protected:

    /// protected so only friend PCB::IFACE::CreateWindow() can act as sole factory.
    FOOTPRINT_EDIT_FRAME( KIWAY* aKiway, wxWindow* aParent, EDA_DRAW_PANEL_GAL::GAL_TYPE aBackend );

    PCB_LAYER_WIDGET* m_Layers;     ///< the layer manager

    /// List of footprint editor configuration parameters.
    PARAM_CFG_ARRAY   m_configParams;

    /**
     * Make sure the footprint info list is loaded (with a progress dialog) and then initialize
     * the footprint library tree.
     */
    void initLibraryTree();

    /**
     * Updates window title according to getLibNickName().
     */
    void updateTitle();

    /// Reloads displayed items and sets view.
    void updateView();

    void restoreLastFootprint();
    void retainLastFootprint();

    /**
     * Run the Footprint Properties dialog and handle changes made in it.
     */
    void editFootprintProperties( MODULE* aFootprint );
};

#endif      // FOOTPRINT_EDIT_FRAME_H
