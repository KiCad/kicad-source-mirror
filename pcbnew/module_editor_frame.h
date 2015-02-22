/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file module_editor_frame.h
 * @brief Definition of class FOOTPRINT_EDIT_FRAME.
 */

#ifndef MODULE_EDITOR_FRAME_H_
#define MODULE_EDITOR_FRAME_H_

#include <wxBasePcbFrame.h>
#include <pcb_base_edit_frame.h>
#include <io_mgr.h>

class PCB_LAYER_WIDGET;
class FP_LIB_TABLE;

namespace PCB { struct IFACE; }     // A KIFACE_I coded in pcbnew.c


class FOOTPRINT_EDIT_FRAME : public PCB_BASE_EDIT_FRAME
{
    friend struct PCB::IFACE;

public:

    ~FOOTPRINT_EDIT_FRAME();

    /**
     * Function GetFootprintEditorFrameName (static)
     * @return the frame name used when creating the frame
     * used to get a reference to this frame, if exists
     */
    static const wxChar* GetFootprintEditorFrameName();

    BOARD_DESIGN_SETTINGS& GetDesignSettings() const;           // overload PCB_BASE_FRAME, get parent's
    void SetDesignSettings( const BOARD_DESIGN_SETTINGS& aSettings );  // overload

    const PCB_PLOT_PARAMS& GetPlotSettings() const;             // overload PCB_BASE_FRAME, get parent's
    void SetPlotSettings( const PCB_PLOT_PARAMS& aSettings );   // overload

    void LoadSettings( wxConfigBase* aCfg );         // Virtual
    void SaveSettings( wxConfigBase* aCfg );         // Virtual

    /**
     * Function GetConfigurationSettings
     * returns the footprçint editor settings list.
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

    void InstallOptionsFrame( const wxPoint& pos );

    void OnCloseWindow( wxCloseEvent& Event );
    void CloseModuleEditor( wxCommandEvent& Event );

    void Process_Special_Functions( wxCommandEvent& event );

    void ProcessPreferences( wxCommandEvent& event );

    /**
     * Function RedrawActiveWindoow
     * draws the footprint editor BOARD, and others elements such as axis and grid.
     */
    void RedrawActiveWindow( wxDC* DC, bool EraseBg );

    /**
     * Function ReCreateHToolbar
     * create the main horizontal toolbar for the footprint editor
     */
    void ReCreateHToolbar();

    void ReCreateVToolbar();
    void ReCreateOptToolbar();
    void ReCreateAuxiliaryToolbar();
    void OnLeftClick( wxDC* DC, const wxPoint& MousePos );

    /**
     * Function OnLeftDClick
     * handles the double click in the footprint editor:
     * If the double clicked item is editable: call the corresponding editor.
     */
    void OnLeftDClick( wxDC* DC, const wxPoint& MousePos );

    /**
     * Function OnRightClick
     * handles the right mouse click in the footprint editor:
     * Create the pop up menu
     * After this menu is built, the standard ZOOM menu is added
     */
    bool OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu );

    /**
     * @brief (Re)Create the menubar for the module editor frame
     */
    void ReCreateMenuBar();

    void ToolOnRightClick( wxCommandEvent& event );
    void OnSelectOptionToolbar( wxCommandEvent& event );

    /**
     * Function OnSaveLibraryAs
     * saves the current library to a new name and/or library type.
     *
     * @note Saving as a new library type requires the plug-in to support saving libraries
     * @see PLUGIN::FootprintSave and PLUGIN::FootprintLibCreate
     */
    void OnSaveLibraryAs( wxCommandEvent& aEvent );

    /**
     * Function OnHotKey
     * handle hot key events.
     * <p?
     * Some commands are relative to the item under the mouse cursor.  Commands are
     * case insensitive
     * </p>
     */
    bool OnHotKey( wxDC* aDC, int aHotKey, const wxPoint& aPosition, EDA_ITEM* aItem = NULL );

    BOARD_ITEM* PrepareItemForHotkey( bool failIfCurrentlyEdited );

    bool OnHotkeyEditItem( int aIdCommand );
    bool OnHotkeyDeleteItem( int aIdCommand );
    bool OnHotkeyMoveItem( int aIdCommand );
    bool OnHotkeyMoveItemExact();
    bool OnHotkeyRotateItem( int aIdCommand );
    bool OnHotkeyDuplicateItem( int aIdCommand );

    /**
     * Function Show3D_Frame
     * displays 3D view of the footprint (module) being edited.
     */
    void Show3D_Frame( wxCommandEvent& event );

    bool GeneralControl( wxDC* aDC, const wxPoint& aPosition, int aHotKey = 0 );
    void OnVerticalToolbar( wxCommandEvent& aEvent );

    void OnUpdateVerticalToolbar( wxUpdateUIEvent& aEvent );
    void OnUpdateOptionsToolbar( wxUpdateUIEvent& aEvent );
    void OnUpdateLibSelected( wxUpdateUIEvent& aEvent );
    void OnUpdateModuleSelected( wxUpdateUIEvent& aEvent );
    void OnUpdateLibAndModuleSelected( wxUpdateUIEvent& aEvent );
    void OnUpdateLoadModuleFromBoard( wxUpdateUIEvent& aEvent );
    void OnUpdateInsertModuleInBoard( wxUpdateUIEvent& aEvent );
    void OnUpdateReplaceModuleInBoard( wxUpdateUIEvent& aEvent );
    void OnUpdateSelectCurrentLib( wxUpdateUIEvent& aEvent );

    ///> @copydoc PCB_BASE_EDIT_FRAME::OnEditItemRequest()
    void OnEditItemRequest( wxDC* aDC, BOARD_ITEM* aItem );

    /**
     * Function LoadModuleFromBoard
     * called from the main toolbar to load a footprint from board mainly to edit it.
     */
    void LoadModuleFromBoard( wxCommandEvent& event );

    /**
     * Function SaveFootprintInLibrary
     * Save in an existing library a given footprint
     * @param aLibName = name of the library to use
     * @param aModule = the given footprint
     * @param aOverwrite = true to overwrite an existing footprint, false to
     *                     abort if an existing footprint with same name is found
     * @param aDisplayDialog = true to display a dialog to enter or confirm the
     *                         footprint name
     * @return : true if OK, false if abort
     */
    bool SaveFootprintInLibrary( const wxString& aLibName,
                                 MODULE*         aModule,
                                 bool            aOverwrite,
                                 bool            aDisplayDialog );

    /**
     * Virtual Function OnModify()
     * Must be called after a footprint change
     * in order to set the "modify" flag of the current screen
     * and prepare, if needed the refresh of the 3D frame showing the footprint
     * do not forget to call the basic OnModify function to update auxiliary info
     */
    virtual void OnModify();

    /**
     * Function ToPrinter
     * Install the print dialog
     */
    void ToPrinter( wxCommandEvent& event );

    /**
     * Function PrintPage
     * is used to print a page. Prints the page pointed by ActiveScreen,
     * set by the calling print function.
     * @param aDC = wxDC given by the calling print function
     * @param aPrintMaskLayer = not used here
     * @param aPrintMirrorMode = not used here (Set when printing in mirror mode)
     * @param aData = a pointer on an auxiliary data (NULL if not used)
     */
    virtual void PrintPage( wxDC* aDC, LSET aPrintMaskLayer, bool aPrintMirrorMode,
                            void * aData = NULL);

    // BOARD handling

    /**
     * Function Clear_Pcb
     * delete all and reinitialize the current board
     * @param aQuery = true to prompt user for confirmation, false to initialize silently
     */
    bool Clear_Pcb( bool aQuery );

    /* handlers for block commands */
    virtual int BlockCommand( int key );

    /**
     * Function HandleBlockPlace
     * handles the BLOCK PLACE command
     *  Last routine for block operation for:
     *  - block move & drag
     *  - block copy & paste
     */
    virtual void HandleBlockPlace( wxDC* DC );

    /**
     * Function HandleBlockEnd( )
     * Handle the "end"  of a block command,
     * i.e. is called at the end of the definition of the area of a block.
     * depending on the current block command, this command is executed
     * or parameters are initialized to prepare a call to HandleBlockPlace
     * in GetScreen()->m_BlockLocate
     * @return false if no item selected, or command finished,
     * true if some items found and HandleBlockPlace must be called later
     */
    virtual bool HandleBlockEnd( wxDC* DC );

    BOARD_ITEM* ModeditLocateAndDisplay( int aHotKeyCode = 0 );

    /* Undo and redo functions */

    /**
     * Function SaveCopyInUndoList.
     * Creates a new entry in undo list of commands.
     * add a picker to handle aItemToCopy
     * @param aItem = the board item modified by the command to undo
     * @param aTypeCommand = command type (see enum UNDO_REDO_T)
     * @param aTransformPoint = the reference point of the transformation, for
     *                          commands like move
     */
    virtual void SaveCopyInUndoList( BOARD_ITEM* aItem,
                                     UNDO_REDO_T aTypeCommand,
                                     const wxPoint& aTransformPoint = wxPoint( 0, 0 ) );

    /**
     * Function SaveCopyInUndoList (overloaded).
     * Creates a new entry in undo list of commands.
     * add a list of pickers to handle a list of items
     * @param aItemsList = the list of items modified by the command to undo
     * @param aTypeCommand = command type (see enum UNDO_REDO_T)
     * @param aTransformPoint = the reference point of the transformation, for
     *                          commands like move
     */
    virtual void SaveCopyInUndoList( const PICKED_ITEMS_LIST& aItemsList,
                                     UNDO_REDO_T aTypeCommand,
                                     const wxPoint& aTransformPoint = wxPoint( 0, 0 ) );

    /**
     * Function RestoreCopyFromUndoList
     * performs an undo operation on the last edition:
     *  - Place the current edited library component in Redo list
     *  - Get old version of the current edited library component
     */
    void RestoreCopyFromUndoList( wxCommandEvent& aEvent );

    /**
     * Function RestoreCopyFromRedoList
     * performs a redo operation on the the last edition:
     *  - Place the current edited library component in undo list
     *  - Get old version of the current edited library component
     */
    void RestoreCopyFromRedoList( wxCommandEvent& aEvent );

    /// Return the current library nickname.
    const wxString GetCurrentLib() const;

    // Footprint edition
    void RemoveStruct( EDA_ITEM* Item );

    /**
     * Function Transform
     * performs a geometric transform on the current footprint.
     */
    void Transform( MODULE* module, int transform );

    // importing / exporting Footprint
    /**
     * Function Export_Module
     * Create a file containing only one footprint.
     * Used to export a footprint
     * Exported files  have the standard ext .emp
     * This is the same format as .mod files but restricted to only one footprint
     * So Create a new lib (which will contains one module) and export a footprint
     * is basically the same thing
     * @param aModule = the module to export
     */
    void Export_Module( MODULE* aModule );

    /**
     * Function Import_Module
     * Read a file containing only one footprint.
     * Used to import (after exporting) a footprint
     * Exported files  have the standard ext .emp
     * This is the same format as .mod files but restricted to only one footprint
     * The import function can also read gpcb footprint file, in Newlib format
     * (One footprint per file, Newlib files have no special ext.)
     */
    MODULE* Import_Module();

    /**
     * Function CreateNewLibrary
     * prompts user for a library path, then creates a new footprint library at that
     * location.  If library exists, user is warned about that, and is given a chance
     * to abort the new creation, and in that case existing library is first deleted.
     *
     * @return wxString - the newly created library path if library was successfully
     *   created, else wxEmptyString because user aborted or error.
     */
    wxString CreateNewLibrary();

    /**
     * Function SaveCurrentModule
     * saves the module which is currently being edited into aLibPath or into the
     * currently selected library if aLibPath is NULL.
     * @return bool - true if successfully saved, else false because abort or error.
     */
    bool SaveCurrentModule( const wxString* aLibPath = NULL );

    /**
     * Function Load_Module_From_BOARD
     * load in Modedit a footprint from the main board
     * @param Module = the module to load. If NULL, a module reference will we asked to user
     * @return true if a module isloaded, false otherwise.
     */
    bool Load_Module_From_BOARD( MODULE* Module );

    /**
     * Function SelectFootprint
     * Display the list of modules currently existing on the BOARD
     * @return a pointer to a module if this module is selected or NULL otherwise
     * @param aPcb = the board from modules can be loaded
     */
    MODULE* SelectFootprint( BOARD* aPcb );

    // functions to edit footprint edges

    /**
     * Function Edit_Edge_Width
     * changes the width of module perimeter lines, EDGE_MODULEs.
     * param ModuleSegmentWidth (global) = new width
     * @param aEdge = edge to edit, or NULL.  If aEdge == NULL change
     *               the width of all footprint's edges
     */
    void Edit_Edge_Width( EDGE_MODULE* aEdge );

    /**
     * Function Edit_Edge_Layer
     * changes the EDGE_MODULE Edge layer,  (The new layer will be asked)
     * if Edge == NULL change the layer of the entire footprint edges
     * @param Edge = edge to edit, or NULL
     */
    void Edit_Edge_Layer( EDGE_MODULE* Edge );

    /**
     * Function Delete_Edge_Module
     * deletes EDGE_MODULE Edge
     * @param Edge = edge to delete
     */
    void Delete_Edge_Module( EDGE_MODULE* Edge );

    /**
     * Function Begin_Edge_Module
     * creates a new edge item (line, arc ..).
     * @param Edge = if NULL: create new edge else terminate edge and create a
     *                new edge
     * @param DC = current Device Context
     * @param type_edge = S_SEGMENT,S_ARC ..
     * @return the new created edge.
     */
    EDGE_MODULE* Begin_Edge_Module( EDGE_MODULE* Edge, wxDC* DC, STROKE_T type_edge );

    /**
     * Function End_Edge_Module
     * terminates a move or create edge function
     */
    void End_Edge_Module( EDGE_MODULE* Edge );

    /**
     * Function Enter_Edge_Width
     * Edition of width of module outlines
     * Ask for a new width.
     * Change the width of EDGE_MODULE Edge if aEdge != NULL
     * @param aEdge = edge to edit, or NULL
     * changes ModuleSegmentWidth (global) = new width
     */
    void Enter_Edge_Width( EDGE_MODULE* aEdge );

    /// Function to initialize the move function params of a graphic item type DRAWSEGMENT
    void Start_Move_EdgeMod( EDGE_MODULE* drawitem, wxDC* DC );

    /// Function to place a graphic item type EDGE_MODULE currently moved
    void Place_EdgeMod( EDGE_MODULE* drawitem );

    /**
     * Function InstallFootprintBodyItemPropertiesDlg
     * Install a dialog to edit a graphic item of a footprint body.
     * @param aItem = a pointer to the graphic item to edit
     */
    void InstallFootprintBodyItemPropertiesDlg( EDGE_MODULE* aItem );

    /**
     * Function DlgGlobalChange_PadSettings
     * changes pad characteristics for the given footprint
     * or all footprints which look like the given footprint.
     * Options are set by the opened dialog.
     * @param aPad is the pattern. The given footprint is the parent of this pad
     */
    void DlgGlobalChange_PadSettings( D_PAD* aPad );

    /**
     * Function DeleteModuleFromCurrentLibrary
     * prompts user for footprint name, then deletes it from current library.
     */
    bool DeleteModuleFromCurrentLibrary();

    /**
     * Function IsElementVisible
     * tests whether a given element category is visible. Keep this as an
     * inline function.
     * @param aElement is from the enum by the same name
     * @return bool - true if the element is visible.
     * @see enum PCB_VISIBLE
     */
    bool IsElementVisible( int aElement ) const;

    /**
     * Function SetElementVisibility
     * changes the visibility of an element category
     * @param aElement is from the enum by the same name
     * @param aNewState = The new visibility state of the element category
     * @see enum PCB_VISIBLE
     */
    void SetElementVisibility( int aElement, bool aNewState );

    /**
     * Function IsGridVisible() , virtual
     * @return true if the grid must be shown
     */
    virtual bool IsGridVisible() const;

    /**
     * Function SetGridVisibility() , virtual
     * It may be overloaded by derived classes
     * if you want to store/retrieve the grid visibility in configuration.
     * @param aVisible = true if the grid must be shown
     */
    virtual void SetGridVisibility( bool aVisible );

    /**
     * Function GetGridColor() , virtual
     * @return the color of the grid
     */
    virtual EDA_COLOR_T GetGridColor() const;

    ///> @copydoc PCB_BASE_FRAME::SetActiveLayer()
    void SetActiveLayer( LAYER_ID aLayer );

    ///> @copydoc EDA_DRAW_FRAME::UseGalCanvas()
    virtual void UseGalCanvas( bool aEnable );

    DECLARE_EVENT_TABLE()

protected:

    /// protected so only friend PCB::IFACE::CreateWindow() can act as sole factory.
    FOOTPRINT_EDIT_FRAME( KIWAY* aKiway, wxWindow* aParent );

    PCB_LAYER_WIDGET* m_Layers;

    PARAM_CFG_ARRAY   m_configSettings;         ///< List of footprint editor configuration settings.

    /**
     * Function UpdateTitle
     * updates window title according to getLibNickName().
     */
    void updateTitle();

    /// Reloads displayed items and sets view.
    void updateView();

    /// The libPath is not publicly visible, grab it from the FP_LIB_TABLE if we must.
    const wxString getLibPath();

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
     * Function moveExact
     * Move the selected item exactly, popping up a dialog to allow the
     * user the enter the move delta
     */
    void moveExact();

    /**
     * Function duplicateItems
     * Duplicate the item under the cursor
     * @param aIncrement increment the number of pad (if that is what is selected)
     */
    void duplicateItems( bool aIncrement );
};

#endif      // MODULE_EDITOR_FRAME_H_
