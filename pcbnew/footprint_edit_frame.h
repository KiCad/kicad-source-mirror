/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file footprint_edit_frame.h
 * @brief Definition of class FOOTPRINT_EDIT_FRAME.
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

    std::unique_ptr<MODULE>     m_copiedModule;
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

    /**
     * switches currently used canvas (default / Cairo / OpenGL).
     * It also reinit the layers manager that slightly changes with canvases
     */
    virtual void OnSwitchCanvas( wxCommandEvent& aEvent ) override;

    /**
     * Update the layer manager and other widgets from the board setup
     * (layer and items visibility, colors ...)
     */
    void UpdateUserInterface();

    void Process_Special_Functions( wxCommandEvent& event );

    void ProcessPreferences( wxCommandEvent& event );

    /**
     * Draw the footprint editor BOARD, and others elements such as axis and grid.
     */
    void RedrawActiveWindow( wxDC* DC, bool EraseBg ) override;

    /**
     * Create the main horizontal toolbar for the footprint editor.
     */
    void ReCreateHToolbar() override;

    void ReCreateVToolbar() override;
    void ReCreateOptToolbar() override;
    void OnLeftClick( wxDC* DC, const wxPoint& MousePos ) override;

    /**
     * Handle the double click in the footprint editor.
     *
     * If the double clicked item is editable: call the corresponding editor.
     */
    void OnLeftDClick( wxDC* DC, const wxPoint& MousePos ) override;

    /**
     * Handle the right mouse click in the footprint editor.
     *
     * Create the pop up menu.  After this menu is built, the standard ZOOM menu is added
     */
    bool OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu ) override;

    /**
     * @brief (Re)Create the menubar for the module editor frame
     */
    void ReCreateMenuBar() override;

    // The Tool Framework initalization, for GAL mode
    void setupTools();

    void ToolOnRightClick( wxCommandEvent& event ) override;
    void OnSelectOptionToolbar( wxCommandEvent& event );
    void OnConfigurePaths( wxCommandEvent& aEvent );
    void OnToggleSearchTree( wxCommandEvent& event );

    void OnSaveFootprintAsPng( wxCommandEvent& event );

    bool IsSearchTreeShown();

    /**
     * Save a library to a new name and/or library type.
     *
     * @note Saving as a new library type requires the plug-in to support saving libraries
     * @see PLUGIN::FootprintSave and PLUGIN::FootprintLibCreate
     */
    bool SaveLibraryAs( const wxString& aLibraryPath );

    ///> @copydoc EDA_DRAW_FRAME::GetHotKeyDescription()
    EDA_HOTKEY* GetHotKeyDescription( int aCommand ) const override;

    /**
     * Handle hot key events.
     * <p>
     * Some commands are relative to the item under the mouse cursor.  Commands are
     * case insensitive
     * </p>
     */
    bool OnHotKey( wxDC* aDC, int aHotKey, const wxPoint& aPosition, EDA_ITEM* aItem = NULL ) override;

    BOARD_ITEM* PrepareItemForHotkey( bool failIfCurrentlyEdited );

    bool OnHotkeyEditItem( int aIdCommand );
    bool OnHotkeyDeleteItem( int aIdCommand );
    bool OnHotkeyMoveItem( int aIdCommand );
    bool OnHotkeyMoveItemExact();
    bool OnHotkeyRotateItem( int aIdCommand );
    bool OnHotkeyDuplicateItem( int aIdCommand );

    /**
     * Display 3D view of the footprint (module) being edited.
     */
    void Show3D_Frame( wxCommandEvent& event ) override;

    bool GeneralControl( wxDC* aDC, const wxPoint& aPosition, EDA_KEY aHotKey = 0 ) override;
    void OnVerticalToolbar( wxCommandEvent& aEvent );

    /**
     * Handle ID_ZOOM_SELECTION and ID_NO_TOOL_SELECTED tools
     */
    void OnUpdateSelectTool( wxUpdateUIEvent& aEvent );

    /**
     * Handle most of tools og the vertical right toolbar ("Tools" toolbar)
     */
    void OnUpdateVerticalToolbar( wxUpdateUIEvent& aEvent );

    void OnUpdateOptionsToolbar( wxUpdateUIEvent& aEvent );
    void OnUpdateModuleSelected( wxUpdateUIEvent& aEvent );
    void OnUpdateModuleTargeted( wxUpdateUIEvent& aEvent );
    void OnUpdateSave( wxUpdateUIEvent& aEvent );
    void OnUpdateSaveAs( wxUpdateUIEvent& aEvent );
    void OnUpdateLoadModuleFromBoard( wxUpdateUIEvent& aEvent );
    void OnUpdateInsertModuleInBoard( wxUpdateUIEvent& aEvent );

    ///> @copydoc PCB_BASE_EDIT_FRAME::OnEditItemRequest()
    void OnEditItemRequest( wxDC* aDC, BOARD_ITEM* aItem ) override;

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
    bool RevertFootprint();

    /**
     * Must be called after a footprint change
     * in order to set the "modify" flag of the current screen
     * and prepare, if needed the refresh of the 3D frame showing the footprint
     * do not forget to call the basic OnModify function to update auxiliary info
     */
    virtual void OnModify() override;

    /**
     * Install the print dialog
     */
    void ToPrinter( wxCommandEvent& event );

    // BOARD handling

    /**
     * Delete all and reinitialize the current board.
     *
     * @param aQuery = true to prompt user for confirmation, false to initialize silently
     */
    bool Clear_Pcb( bool aQuery );

    /* handlers for block commands */
    virtual int BlockCommand( EDA_KEY key ) override;

    /**
     * Handle the BLOCK PLACE command.
     *
     *  Last routine for block operation for:
     *  - block move & drag
     *  - block copy & paste
     */
    virtual void HandleBlockPlace( wxDC* DC ) override;

    /**
     * Handle the "end"  of a block command,
     * i.e. is called at the end of the definition of the area of a block.
     * depending on the current block command, this command is executed
     * or parameters are initialized to prepare a call to HandleBlockPlace
     * in GetScreen()->m_BlockLocate
     * @return false if no item selected, or command finished,
     * true if some items found and HandleBlockPlace must be called later
     */
    virtual bool HandleBlockEnd( wxDC* DC ) override;

    BOARD_ITEM* ModeditLocateAndDisplay( int aHotKeyCode = 0 );

    /// Return the LIB_ID of the part selected in the footprint or the part being edited.
    LIB_ID getTargetFPID() const;

    /// Return the LIB_ID of the part being edited.
    LIB_ID GetLoadedFPID() const;

    void RemoveStruct( EDA_ITEM* Item );

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
     * Change the width of module perimeter lines, EDGE_MODULEs.
     *
     * param ModuleSegmentWidth (global) = new width
     * @param aEdge = edge to edit, or NULL.  If aEdge == NULL change
     *               the width of all footprint's edges
     */
    void Edit_Edge_Width( EDGE_MODULE* aEdge );

    /**
     * Change the EDGE_MODULE Edge layer,  (The new layer will be asked)
     * if Edge == NULL change the layer of the entire footprint edges
     *
     * @param Edge = edge to edit, or NULL
     */
    void Edit_Edge_Layer( EDGE_MODULE* Edge );

    /**
     * Delete EDGE_MODULE ddge.
     *
     * @param Edge = edge to delete
     */
    void Delete_Edge_Module( EDGE_MODULE* Edge );

    /**
     * Creates a new edge item (line, arc ..).
     *
     * @param Edge = if NULL: create new edge else terminate edge and create a new edge
     * @param DC = current Device Context
     * @param type_edge = S_SEGMENT,S_ARC ..
     * @return the new created edge.
     */
    EDGE_MODULE* Begin_Edge_Module( EDGE_MODULE* Edge, wxDC* DC, STROKE_T type_edge );

    /**
     * Terminate a move or create edge function.
     */
    void End_Edge_Module( EDGE_MODULE* Edge );

    /// Function to initialize the move function params of a graphic item type DRAWSEGMENT
    void Start_Move_EdgeMod( EDGE_MODULE* drawitem, wxDC* DC );

    /// Function to place a graphic item type EDGE_MODULE currently moved
    void Place_EdgeMod( EDGE_MODULE* drawitem );

    /**
     * Change pad characteristics for the given footprint
     * or all footprints which look like the given footprint.
     * Options are set by the opened dialog.
     * @param aPad is the pattern. The given footprint is the parent of this pad
     */
    void PushPadProperties( D_PAD* aPad );

    /**
     * Delete the given module from its library.
     */
    bool DeleteModuleFromLibrary( const LIB_ID& aFPID, bool aConfirm );

    /**
     * Test whether a given element category is visible.
     *
     * Keep this as an inline function.
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
    virtual bool IsGridVisible() const override;

    /**
     * It may be overloaded by derived classes
     * if you want to store/retrieve the grid visibility in configuration.
     * @param aVisible = true if the grid must be shown
     */
    virtual void SetGridVisibility( bool aVisible ) override;

    /**
     * @return the color of the grid
     */
    virtual COLOR4D GetGridColor() override;

    ///> @copydoc PCB_BASE_FRAME::SetActiveLayer()
    void SetActiveLayer( PCB_LAYER_ID aLayer ) override;

    ///> @copydoc PCB_BASE_FRAME::OnUpdateLayerAlpha()
    void OnUpdateLayerAlpha( wxUpdateUIEvent& aEvent ) override;

    ///> @copydoc EDA_DRAW_FRAME::UseGalCanvas()
    virtual void UseGalCanvas( bool aEnable ) override;

    /**
     * Load a KiCad board (.kicad_pcb) from \a aFileName.
     *
     * @param aFileSet - hold the BOARD file to load, a vector of one element.
     *
     * @param aCtl      - KICTL_ bits, one to indicate that an append of the board file
     *                      aFileName to the currently loaded file is desired.
     *                    @see #KIWAY_PLAYER for bit defines.
     *
     * @return bool - false if file load fails, otherwise true.
    bool LoadOnePcbFile( const wxString& aFileName, bool aAppend = false,
                         bool aForceFileDialog = false );
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
    void InstallPreferences( PAGED_DIALOG* aParent ) override;

    void ReFillLayerWidget();

    /**
     * Update visible items after a language change.
     */
    void ShowChangedLanguage() override;

    /**
     * Called after the preferences dialog is run.
     */
    void CommonSettingsChanged() override;

    /**
     * Synchronize the footprint library tree to the current state of the footprint library
     * table.
     * @param aProgress
     */
    void SyncLibraryTree( bool aProgress );

    /**
     * Redraw the message panel.
     *
     * If a item is currently selected, displays the item info.
     * If nothing selected, display the current footprint info, or
     * clear the message panel if nothing is edited
     */
    void UpdateMsgPanel() override;

    void KiwayMailIn( KIWAY_EXPRESS& mail ) override;

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
     * Creates a new text for the footprint
     * @param aModule is the owner of the text
     * @param aDC is the current DC (can be NULL )
     * @return a pointer to the new text, or NULL if aborted
     */
    TEXTE_MODULE* CreateTextModule( MODULE* aModule, wxDC* aDC );

private:

    /**
     * Run the Footprint Properties dialog and handle changes made in it.
     */
    void editFootprintProperties( MODULE* aFootprint );

    bool saveFootprintInLibrary( MODULE* aModule, const wxString& aLibraryName );

    /**
     * Move the selected item exactly, popping up a dialog to allow the
     * user the enter the move delta
     */
    void moveExact();

    /**
     * Duplicate the item under the cursor
     *
     * @param aIncrement increment the number of pad (if that is what is selected)
     */
    void duplicateItems( bool aIncrement ) override;

};

#endif      // FOOTPRINT_EDIT_FRAME_H
