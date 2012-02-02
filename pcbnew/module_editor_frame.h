/**
 * @file module_editor_frame.h
 * @brief Definition of class FOOTPRINT_EDIT_FRAME.
 */

#ifndef MODULE_EDITOR_FRAME_H_
#define MODULE_EDITOR_FRAME_H_


class FOOTPRINT_EDIT_FRAME : public PCB_BASE_FRAME
{
public:
    MODULE*  CurrentModule;

public:
    FOOTPRINT_EDIT_FRAME( PCB_EDIT_FRAME* aParent,
                          const wxString& title,
                          const wxPoint& pos, const wxSize& size,
                          long style = KICAD_DEFAULT_DRAWFRAME_STYLE );

    ~FOOTPRINT_EDIT_FRAME();

    BOARD_DESIGN_SETTINGS& GetDesignSettings() const;       // overload PCB_BASE_FRAME, get parent's
    void SetDesignSettings( const BOARD_DESIGN_SETTINGS& aSettings );  // overload

    void InstallOptionsFrame( const wxPoint& pos );

    void OnCloseWindow( wxCloseEvent& Event );
    void CloseModuleEditor( wxCommandEvent& Event );

    void Process_Special_Functions( wxCommandEvent& event );

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
     * Function OnHotKey
     * handle hot key events.
     * <p?
     * Some commands are relative to the item under the mouse cursor.  Commands are
     * case insensitive
     * </p>
     */
    void OnHotKey( wxDC* aDC, int aHotKey, const wxPoint& aPosition, EDA_ITEM* aItem = NULL );

    bool OnHotkeyEditItem( int aIdCommand );
    bool OnHotkeyDeleteItem( int aIdCommand );
    bool OnHotkeyMoveItem( int aIdCommand );
    bool OnHotkeyRotateItem( int aIdCommand );

    /**
     * Function Show3D_Frame
     * displays 3D view of the footprint (module) being edited.
     */
    void Show3D_Frame( wxCommandEvent& event );

    void GeneralControl( wxDC* aDC, const wxPoint& aPosition, int aHotKey = 0 );
    void OnVerticalToolbar( wxCommandEvent& aEvent );

    void OnUpdateVerticalToolbar( wxUpdateUIEvent& aEvent );
    void OnUpdateLibSelected( wxUpdateUIEvent& aEvent );
    void OnUpdateModuleSelected( wxUpdateUIEvent& aEvent );
    void OnUpdateLibAndModuleSelected( wxUpdateUIEvent& aEvent );
    void OnUpdateLoadModuleFromBoard( wxUpdateUIEvent& aEvent );
    void OnUpdateInsertModuleInBoard( wxUpdateUIEvent& aEvent );
    void OnUpdateReplaceModuleInBoard( wxUpdateUIEvent& aEvent );

    /**
     * Function LoadModuleFromBoard
     * called from the main toolbar to load a footprint from board mainly to edit it.
     */
    void LoadModuleFromBoard( wxCommandEvent& event );

    /**
     * Virtual Function OnModify()
     * Must be called after a footprint change
     * in order to set the "modify" flag of the current screen
     * and prepare, if needed the refresh of the 3D frame showing the footprint
     * do not forget to call the basic OnModify function to update auxiliary info
     */
    virtual void OnModify( );

    /**
     * Function ToPrinter
     * Install the print dialog
     */
    void ToPrinter( wxCommandEvent& event );

    /**
     * Virtual function PrintPage
     * used to print a page
     * Print the page pointed by ActiveScreen, set by the calling print function
     * @param aDC = wxDC given by the calling print function
     * @param aPrintMaskLayer = not used here
     * @param aPrintMirrorMode = not used here (Set when printing in mirror mode)
     * @param aData = a pointer on an auxiliary data (NULL if not used)
     */
    virtual void PrintPage( wxDC* aDC, int aPrintMaskLayer, bool aPrintMirrorMode,
                            void * aData = NULL);

    // BOARD handling

    /**
     * Function Clear_Pcb
     * delete all and reinitialize the current board
     * @param aQuery = true to prompt user for confirmation, false to initialize silently
     */
    bool Clear_Pcb( bool aQuery );

    /* handlers for block commands */
    virtual int ReturnBlockCommand( int key );

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
public:

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
    virtual void SaveCopyInUndoList( PICKED_ITEMS_LIST& aItemsList,
                                     UNDO_REDO_T aTypeCommand,
                                     const wxPoint& aTransformPoint = wxPoint( 0, 0 ) );

private:
    static wxString m_CurrentLib;

    static BOARD*   s_Pcb;      ///< retain board accross invocations of module editor

    /**
     * Function GetComponentFromUndoList
     * performs an undo operation on the last edition:
     *  - Place the current edited library component in Redo list
     *  - Get old version of the current edited library component
     */
    void GetComponentFromUndoList( wxCommandEvent& event );

    /**
     * Fucntion GetComponentFromRedoList
     * performs a redo operation on the the last edition:
     *  - Place the current edited library component in undo list
     *  - Get old version of the current edited library component
     */
    void GetComponentFromRedoList( wxCommandEvent& event );

    /**
     * Function UpdateTitle
     * updates window title according to m_CurrentLib.
     */
    void UpdateTitle();

public:

    // Footprint edition
    void Place_Ancre( MODULE* module );
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
     * @param aCreateSysLib : true = use default lib path to create lib
     *                    false = use current path or last used path to export the footprint
     */
    void Export_Module( MODULE* aModule, bool aCreateSysLib );

    /**
     * Function Import_Module
     * Read a file containing only one footprint.
     * Used to import (after exporting) a footprint
     * Exported files  have the standard ext .emp
     * This is the same format as .mod files but restricted to only one footprint
     * The import function can also read gpcb footprint file, in Newlib format
     * (One footprint per file, Newlib files have no special ext.)
     */
    MODULE* Import_Module( );

    /**
     * Function Load_Module_From_BOARD
     * load in Modedit a footprint from the main board
     * @param Module = the module to load. If NULL, a module reference will we asked to user
     * @return true if a module isloaded, false otherwise.
     */
    bool Load_Module_From_BOARD( MODULE* Module );

    /**
     * Function Select_1_Module_From_BOARD
     * Display the list of modules currently existing on the BOARD
     * @return a pointer to a module if this module is selected or NULL otherwise
     * @param aPcb = the board from modules can be loaded
     */
    MODULE* Select_1_Module_From_BOARD( BOARD* aPcb );

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
    EDGE_MODULE* Begin_Edge_Module( EDGE_MODULE* Edge, wxDC* DC, int type_edge );

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
     * Function DlgGlobalChange_PadSettings
     * Function to change pad caracteristics for the given footprint
     * or all footprints which look like the given footprint
     * Options are set by the opened dialog.
     * @param aPad is the pattern. The given footprint is the parent of this pad
     */
    void DlgGlobalChange_PadSettings( D_PAD* aPad );

    // handlers for libraries:
    void Delete_Module_In_Library( const wxString& libname );

    int CreateLibrary( const wxString& LibName );

    void Select_Active_Library();

    wxString GetCurrentLib() const { return m_CurrentLib; };

    DECLARE_EVENT_TABLE()
};

#endif      // MODULE_EDITOR_FRAME_H_
