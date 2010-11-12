
/*********************************************************/
/* class WinEDA_ModuleEditFrame: public WinEDA_DrawFrame */
/* Class for the footprint editor                        */
/*********************************************************/

#ifndef _MODULE_EDITOR_FRAME_H_
#define _MODULE_EDITOR_FRAME_H_


class WinEDA_ModuleEditFrame : public WinEDA_BasePcbFrame
{
public:
    MODULE*  CurrentModule;
    wxString m_CurrentLib;

public:
    WinEDA_ModuleEditFrame( wxWindow* father,
                            const wxString& title,
                            const wxPoint& pos, const wxSize& size,
                            long style = KICAD_DEFAULT_DRAWFRAME_STYLE );

    ~WinEDA_ModuleEditFrame();

    void         InstallOptionsFrame( const wxPoint& pos );

    void         OnCloseWindow( wxCloseEvent& Event );
    void         CloseModuleEditor( wxCommandEvent& Event );

    void         Process_Special_Functions( wxCommandEvent& event );
    void         RedrawActiveWindow( wxDC* DC, bool EraseBg );
    void         ReCreateHToolbar();
    void         ReCreateVToolbar();
    void         ReCreateOptToolbar();
    void         ReCreateAuxiliaryToolbar();
    void         OnLeftClick( wxDC* DC, const wxPoint& MousePos );
    void         OnLeftDClick( wxDC* DC, const wxPoint& MousePos );
    bool         OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu );
    void         SetToolbars();
    void         ReCreateMenuBar();
    void         ToolOnRightClick( wxCommandEvent& event );
    void         OnSelectOptionToolbar( wxCommandEvent& event );
    void         OnHotKey( wxDC* DC, int hotkey, EDA_BaseStruct* DrawStruct );
    bool         OnHotkeyEditItem( int aIdCommand );
    bool         OnHotkeyDeleteItem( int aIdCommand );
    bool         OnHotkeyMoveItem( int aIdCommand );
    bool         OnHotkeyRotateItem( int aIdCommand );
    void         Show3D_Frame( wxCommandEvent& event );
    void         GeneralControle( wxDC* DC, wxPoint Mouse );

    /**
     * Function LoadModuleFromBoard
     * called from the main toolbar
     * to load a footprint from board mainly to edit it
     */
    void         LoadModuleFromBoard( wxCommandEvent& event );

    /** Virtual Function OnModify()
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
    void         ToPrinter( wxCommandEvent& event );

    /** Virtual function PrintPage
     * used to print a page
     * Print the page pointed by ActiveScreen, set by the calling print function
     * @param aDC = wxDC given by the calling print function
     * @param aPrint_Sheet_Ref = true to print page references
     * @param aPrintMask = not used here
     * @param aPrintMirrorMode = not used here (Set when printing in mirror mode)
     * @param aData = a pointer on an auxiliary data (NULL if not used)
     */
    virtual void PrintPage( wxDC* aDC, bool aPrint_Sheet_Ref,
                    int aPrintMask, bool aPrintMirrorMode,
                    void * aData = NULL);

    // BOARD handling

    /**
     * Function Clear_Pcb
     * delete all and reinitialize the current board
     * @param aQuery = true to prompt user for confirmation, false to
     *                 initialize silently
     */
    bool         Clear_Pcb( bool aQuery );

    /* handlers for block commands */
    int          ReturnBlockCommand( int key );
    virtual void HandleBlockPlace( wxDC* DC );
    virtual int  HandleBlockEnd( wxDC* DC );

    BOARD_ITEM*  ModeditLocateAndDisplay( int aHotKeyCode = 0 );

    /* Undo and redo functions */
public:

    /**
     * Function SaveCopyInUndoList.
     * Creates a new entry in undo list of commands.
     * add a picker to handle aItemToCopy
     * @param aItem = the board item modified by the command to undo
     * @param aTypeCommand = command type (see enum UndoRedoOpType)
     * @param aTransformPoint = the reference point of the transformation, for
     *                          commands like move
     */
    virtual void SaveCopyInUndoList( BOARD_ITEM* aItem,
                                    UndoRedoOpType aTypeCommand,
                                    const wxPoint& aTransformPoint =
                                        wxPoint( 0, 0 ) );

    /**
     * Function SaveCopyInUndoList (overloaded).
     * Creates a new entry in undo list of commands.
     * add a list of pickers to handle a list of items
     * @param aItemsList = the list of items modified by the command to undo
     * @param aTypeCommand = command type (see enum UndoRedoOpType)
     * @param aTransformPoint = the reference point of the transformation, for
     *                          commands like move
     */
    virtual void SaveCopyInUndoList( PICKED_ITEMS_LIST& aItemsList,
                                    UndoRedoOpType aTypeCommand,
                                    const wxPoint& aTransformPoint =
                                        wxPoint( 0, 0 ) );

private:
    void         GetComponentFromUndoList( wxCommandEvent& event );
    void         GetComponentFromRedoList( wxCommandEvent& event );

public:

    // Footprint edition
    void         Place_Ancre( MODULE* module );
    void         RemoveStruct( EDA_BaseStruct* Item );
    void         Transform( MODULE* module, int transform );

    // importing / exporting Footprint
    void         Export_Module( MODULE* ptmod, bool createlib );
    /**
     * Function Import_Module
     * Read a file containing only one footprint.
     * Used to import (after exporting) a footprint
     * Exported files  have the standard ext .emp
     * This is the same format as .mod files but restricted to only one footprint
     * The import function can also read gpcb footprint file, in Newlib format
     * (One footprint per file, Newlib files have no special ext.)
     * @param DC = Current Device Context (can be NULL)
     */
    MODULE* Import_Module( );


    /**
     * Function Load_Module_From_BOARD
     * load in Modedit a footfrint from the main board
     * @param Module = the module to load. If NULL, a module reference will we asked to user
     * @return true if a module isloaded, false otherwise.
     */
    bool         Load_Module_From_BOARD( MODULE* Module );

    /**
     * Function Select_1_Module_From_BOARD
     * Display the list of modules currently existing on the BOARD
     * @return a pointer to a module if this module is selected or NULL otherwise
     * @param aPcb = the board from modules can be loaded
     */
    MODULE*      Select_1_Module_From_BOARD( BOARD* aPcb );

    // functions to edit footprint edges

    /**
     * Function Edit_Edge_Width
     * changes the width of module perimeter lines, EDGE_MODULEs.
     * param ModuleSegmentWidth (global) = new width
     * @param aEdge = edge to edit, or NULL.  If aEdge == NULL change
     *               the width of all footprint's edges
     */
    void         Edit_Edge_Width( EDGE_MODULE* aEdge );
    void         Edit_Edge_Layer( EDGE_MODULE* Edge );
    void         Delete_Edge_Module( EDGE_MODULE* Edge );
    EDGE_MODULE* Begin_Edge_Module( EDGE_MODULE* Edge, wxDC* DC, int type_edge );
    void         End_Edge_Module( EDGE_MODULE* Edge );
    /**
     * Function Enter_Edge_Width
     * Edition of the edge items width
     * Ask for a new width.
     * Change the width of EDGE_MODULE Edge if aEdge != NULL
     * @param aEdge = edge to edit, or NULL
     * @output ModuleSegmentWidth (global) = new width
     */
    void         Enter_Edge_Width( EDGE_MODULE* aEdge );
    void         Start_Move_EdgeMod( EDGE_MODULE* drawitem, wxDC* DC );
    void         Place_EdgeMod( EDGE_MODULE* drawitem );

    // handlers for libraries:
    void         Delete_Module_In_Library( const wxString& libname );
    int          Create_Librairie( const wxString& LibName );
    void         Select_Active_Library();

    DECLARE_EVENT_TABLE()
};

#endif      // #define _MODULE_EDITOR_FRAME_H_

