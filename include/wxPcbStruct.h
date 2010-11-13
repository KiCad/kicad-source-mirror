/***********************************************************/
/*                      wxPcbStruct.h                      */
/***********************************************************/

#ifndef  WXPCB_STRUCT_H
#define  WXPCB_STRUCT_H


#include "wxstruct.h"
#include "base_struct.h"
#include "param_config.h"
#include "class_layerchoicebox.h"

#ifndef PCB_INTERNAL_UNIT
#define PCB_INTERNAL_UNIT 10000
#endif


/*  Forward declarations of classes. */
class PCB_SCREEN;
class WinEDA_Toolbar;
class WinEDA_ModuleEditFrame;
class BOARD;
class TEXTE_PCB;
class MODULE;
class TRACK;
class SEGZONE;
class SEGVIA;
class D_PAD;
class TEXTE_MODULE;
class MIREPCB;
class DIMENSION;
class EDGE_MODULE;
class WinEDA3D_DrawFrame;
class DRC;
class ZONE_CONTAINER;
class DRAWSEGMENT;
class GENERAL_COLLECTOR;
class GENERAL_COLLECTORS_GUIDE;
class PCB_LAYER_WIDGET;


/**
 * @info see also class WinEDA_BasePcbFrame: Basic class for pcbnew and gerbview.
 */


/*****************************************************/
/* class WinEDA_PcbFrame: the main frame for Pcbnew  */
/*****************************************************/
class WinEDA_PcbFrame : public WinEDA_BasePcbFrame
{
    friend class PCB_LAYER_WIDGET;

protected:

    PCB_LAYER_WIDGET* m_Layers;

    DRC* m_drc;                     ///< the DRC controller, see drc.cpp

    PARAM_CFG_ARRAY   m_projectFileParams;   ///< List of PCBNew project file settings.
    PARAM_CFG_ARRAY   m_configSettings;      ///< List of PCBNew configuration settings.

    wxString          m_lastNetListRead;     ///< Last net list read with relative path.

    // we'll use lower case function names for private member functions.
    void createPopUpMenuForZones( ZONE_CONTAINER* edge_zone, wxMenu* aPopMenu );
    void createPopUpMenuForFootprints( MODULE* aModule, wxMenu* aPopMenu );
    void createPopUpMenuForFpTexts( TEXTE_MODULE* aText, wxMenu* aPopMenu );
    void createPopUpMenuForFpPads( D_PAD* aPad, wxMenu* aPopMenu );
    void createPopupMenuForTracks( TRACK* aTrack, wxMenu* aPopMenu );
    void createPopUpMenuForTexts( TEXTE_PCB* Text, wxMenu* menu );
    void createPopUpBlockMenu( wxMenu* menu );
    void createPopUpMenuForMarkers( MARKER_PCB* aMarker, wxMenu* aPopMenu );

    /**
     * Function setActiveLayer
     * will change the currently active layer to \a aLayer and also
     * update the PCB_LAYER_WIDGET.
     */
    void setActiveLayer( int aLayer, bool doLayerWidgetUpdate = true )
    {
        ( (PCB_SCREEN*) GetScreen() )->m_Active_Layer = aLayer;

        if( doLayerWidgetUpdate )
            syncLayerWidget();
    }

    /**
     * Function getActiveLayer
     * returns the active layer
     */
    int getActiveLayer()
    {
        return ( (PCB_SCREEN*) GetScreen() )->m_Active_Layer;
    }

    /**
     * Function syncLayerWidget
     * updates the currently "selected" layer within the PCB_LAYER_WIDGET.
     * The currently active layer is defined by the return value of getActiveLayer().
     * <p>
     * This function cannot be inline without including layer_widget.h in
     * here and we do not want to do that.
     */
    void syncLayerWidget( );

    /**
     * Function syncLayerBox
     * updates the currently "selected" layer within m_SelLayerBox
     * The currently active layer, as defined by the return value of
     * getActiveLayer().  And updates the colored icon in the toolbar.
     */
    void syncLayerBox();


public:
    WinEDALayerChoiceBox* m_SelLayerBox;    // a combo box to display and
                                            // select active layer
    WinEDAChoiceBox* m_SelTrackWidthBox;    // a combo box to display and
                                            // select current track width
    WinEDAChoiceBox* m_SelViaSizeBox;       // a combo box to display and
                                            // select current via diameter
    wxTextCtrl*      m_ClearanceBox;        // a text ctrl to display the
                                            // current tracks and vias
                                            // clearance
    wxTextCtrl*      m_NetClassSelectedBox; // a text ctrl to display the
                                            // current NetClass
    bool             m_TrackAndViasSizesList_Changed;

    bool             m_show_microwave_tools;
    bool             m_show_layer_manager_tools;


public:
    WinEDA_PcbFrame( wxWindow* father, const wxString& title,
                     const wxPoint& pos, const wxSize& size,
                     long style = KICAD_DEFAULT_DRAWFRAME_STYLE );

    ~WinEDA_PcbFrame();

    void             OnQuit( wxCommandEvent & WXUNUSED(event) );

    /**
     * Function ToPlotter
     * Open a dialog frame to create plot and drill files
     * relative to the current board
     */
    void             ToPlotter( wxCommandEvent& event );

    /**
     * Function ToPrinter
     * Install the print dialog
     */
    void             ToPrinter( wxCommandEvent& event );

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
                            void * aData = NULL );

    void             GetKicadAbout( wxCommandEvent& event );

    /**
     * Function IsGridVisible() , virtual
     * @return true if the grid must be shown
     */
    virtual bool     IsGridVisible();

    /**
     * Function SetGridVisibility() , virtual
     * It may be overloaded by derived classes
     * if you want to store/retrieve the grid visibility in configuration.
     * @param aVisible = true if the grid must be shown
     */
    virtual void     SetGridVisibility(bool aVisible);

    /**
     * Function GetGridColor() , virtual
     * @return the color of the grid
     */
    virtual int     GetGridColor();

    /**
     * Function SetGridColor() , virtual
     * @param aColor = the new color of the grid
     */
    virtual void     SetGridColor(int aColor);

    // Configurations:
    void             InstallConfigFrame( const wxPoint& pos );
    void             Process_Config( wxCommandEvent& event );

    PARAM_CFG_ARRAY& GetProjectFileParameters();
    void             SaveProjectSettings();

    /**
     * Load the project file configuration settings.
     *
     * @param aProjectFileName = The project filename.
     *  if not found use kicad.pro and initialize default values
     * @return always returns true.
     */
    bool             LoadProjectSettings( const wxString& aProjectFileName );

    /**
     * Get the list of application specific settings.
     *
     * @return - Reference to the list of applications settings.
     */
    PARAM_CFG_ARRAY& GetConfigurationSettings();

    /**
     * Load applications settings specific to PCBNew.
     *
     * This overrides the base class WinEDA_BasePcbFrame::LoadSettings() to
     * handle settings specific common to the PCB layout application.  It
     * calls down to the base class to load settings common to all PCB type
     * drawing frames.  Please put your application settings for PCBNew here
     * to avoid having application settings loaded all over the place.
     */
    virtual void LoadSettings();

    /**
     * Save applications settings common to PCBNew.
     *
     * This overrides the base class WinEDA_BasePcbFrame::SaveSettings() to
     * save settings specific to the PCB layout application main window.  It
     * calls down to the base class to save settings common to all PCB type
     * drawing frames.  Please put your application settings for PCBNew here
     * to avoid having application settings saved all over the place.
     */
    virtual void SaveSettings();

    /**
     * Get the last net list read with the net list dialog box.
     *
     * @return - Absolute path and file name of the last net list file successfully read.
     */
    wxString         GetLastNetListRead();

    /**
     * Set the last net list successfully read by the net list dialog box.
     *
     * Note: the file path is converted to a path relative to the project file path.  If
     *       the path cannot be made relative, than m_lastNetListRead is set to and empty
     *       string.  This could happen when the net list file is on a different drive than
     *       the project file.  The advantage of relative paths is that is more likely to
     *       work when opening the same project from both Windows and Linux.
     *
     * @param aNetListFile - The last net list file with full path successfully read.
     */
    void             SetLastNetListRead( const wxString& aNetListFile );

    /**
     * Function OnHotKey.
     *  ** Commands are case insensitive **
     *  Some commands are relatives to the item under the mouse cursor
     *  @param aDC = current device context
     *  @param aHotkeyCode = hotkey code (ascii or wxWidget code for special keys)
     *  @param aItem = NULL or pointer on a EDA_BaseStruct under the mouse cursor
     */
    void             OnHotKey( wxDC*           aDC,
                               int             aHotkeyCode,
                               EDA_BaseStruct* aItem );

    /**
     * Function OnHotkeyDeleteItem
     * Delete the item found under the mouse cursor
     *  Depending on the current active tool::
     *      Tool track
     *          if a track is in progress: Delete the last segment
     *			else delete the entire track
     *      Tool module (footprint):
     *          Delete the module.
     * @param aDC = current device context
     * @return true if an item was deleted
     */
    bool             OnHotkeyDeleteItem( wxDC* aDC );

    bool             OnHotkeyEditItem( int aIdCommand );

    /**
     * Function OnHotkeyMoveItem
     * Moves or drag the item (footprint, track, text .. ) found under the mouse cursor
     * Only a footprint or a track can be dragged
     * @param aIdCommand = the hotkey command id
     * @return true if an item was moved
     */
    bool             OnHotkeyMoveItem( int aIdCommand );

    /**
     * Function OnHotkeyRotateItem
     * Rotate the item (text or footprint) found under the mouse cursor
     * @param aIdCommand = the hotkey command id
     * @return true if an item was moved
     */
    bool             OnHotkeyRotateItem( int aIdCommand );

    void             OnCloseWindow( wxCloseEvent& Event );
    void             Process_Special_Functions( wxCommandEvent& event );
    void             Tracks_and_Vias_Size_Event( wxCommandEvent& event );

    void             ProcessMuWaveFunctions( wxCommandEvent& event );
    void             MuWaveCommand( wxDC* DC, const wxPoint& MousePos );

    void             RedrawActiveWindow( wxDC* DC, bool EraseBg );
    void             ReCreateHToolbar();
    void             ReCreateAuxiliaryToolbar();
    void             ReCreateVToolbar();
    void             ReCreateMicrowaveVToolbar();
    void             ReCreateOptToolbar();
    void             ReCreateMenuBar();
    WinEDALayerChoiceBox* ReCreateLayerBox( WinEDA_Toolbar* parent );

    /** Virtual Function OnModify()
     * Must be called after a board change
     * in order to set the "modify" flag of the current screen
     * and prepare, if needed the refresh of the 3D frame showing the footprint
     * do not forget to call the basic OnModify function to update auxiliary info
     */
    virtual void OnModify( );

    /**
     * Function IsElementVisible
     * tests whether a given element category is visible. Keep this as an
     * inline function.
     * @param aPCB_VISIBLE is from the enum by the same name
     * @return bool - true if the element is visible.
     * @see enum PCB_VISIBLE
     */
    bool IsElementVisible( int aPCB_VISIBLE )
    {
        return GetBoard()->IsElementVisible( aPCB_VISIBLE );
    }

    /**
     * Function SetElementVisibility
     * changes the visibility of an element category
     * @param aPCB_VISIBLE is from the enum by the same name
     * @param aNewState = The new visibility state of the element category
     * @see enum PCB_VISIBLE
     */
    void SetElementVisibility( int aPCB_VISIBLE, bool aNewState );

    /**
     * Function SetVisibleAlls
     * Set the status of all visible element categories and layers to VISIBLE
     */
    void SetVisibleAlls( );

    /**
     * Function ReFillLayerWidget
     * changes out all the layers in m_Layers and may be called upon
     * loading a new BOARD.
     */
    void             ReFillLayerWidget();

    void             Show3D_Frame( wxCommandEvent& event );
    void             GeneralControle( wxDC* DC, wxPoint Mouse );

    /**
     * Function ShowDesignRulesEditor
     * Display the Design Rules Editor.
     */
    void             ShowDesignRulesEditor( wxCommandEvent& event );

    /* toolbars update UI functions: */

    void             PrepareLayerIndicator();

    /**
     * Function AuxiliaryToolBar_Update_UI
     * update the displayed values on auxiliary horizontal toolbar
     * (track width, via sizes, clearance ...
     */
    void             AuxiliaryToolBar_Update_UI();

    /**
     * Function AuxiliaryToolBar_DesignRules_Update_UI
     * update the displayed values: track width, via sizes, clearance
     * used when a new netclass is selected
     */
    void             AuxiliaryToolBar_DesignRules_Update_UI();

    /* mouse functions events: */
    void             OnLeftClick( wxDC* DC, const wxPoint& MousePos );
    void             OnLeftDClick( wxDC* DC, const wxPoint& MousePos );

    /**
     * Function OnRightClick
     * populates a popup menu with the choices appropriate for the current
     *context.
     * The caller will add the ZOOM menu choices afterward.
     * @param aMousePos The current mouse position
     * @param aPopMenu The menu to add to.
     */
    bool             OnRightClick( const wxPoint& aMousePos, wxMenu* aPopMenu );

    void             OnSelectOptionToolbar( wxCommandEvent& event );
    void             ToolOnRightClick( wxCommandEvent& event );

    /**
     * Function SaveCopyInUndoList.
     * Creates a new entry in undo list of commands.
     * add a picker to handle aItemToCopy
     * @param aItemToCopy = the board item modified by the command to undo
     * @param aTypeCommand = command type (see enum UndoRedoOpType)
     * @param aTransformPoint = the reference point of the transformation, for
     *commands like move
     */
    virtual void     SaveCopyInUndoList( BOARD_ITEM* aItemToCopy,
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
     *commands like move
     */
    virtual void SaveCopyInUndoList( PICKED_ITEMS_LIST& aItemsList,
                                    UndoRedoOpType aTypeCommand,
                                    const wxPoint& aTransformPoint =
                                        wxPoint( 0, 0 ) );

    /**
     * Function PutDataInPreviousState
     * Used in undo or redo command.
     * Put data pointed by List in the previous state, i.e. the state memorized
     * by List
     * @param aList = a PICKED_ITEMS_LIST pointer to the list of items to
     *                undo/redo
     * @param aRedoCommand = a bool: true for redo, false for undo
     * @param aRebuildRatsnet = a bool: true to rebuild ratsnet (normal use),
     *                          false
     * to just retrieve last state (used in abort commands that do not need to
     * rebuild ratsnest)
     */
    void PutDataInPreviousState( PICKED_ITEMS_LIST* aList,
                                 bool               aRedoCommand,
                                 bool               aRebuildRatsnet = true );

    /**
     * Function GetBoardFromRedoList
     *  Redo the last edition:
     *  - Save the current board in Undo list
     *  - Get an old version of the board from Redo list
     *  @return none
     */
    void GetBoardFromRedoList( wxCommandEvent& event );

    /**
     * Function GetBoardFromUndoList
     *  Undo the last edition:
     *  - Save the current board in Redo list
     *  - Get an old version of the board from Undo list
     *  @return none
     */
    void GetBoardFromUndoList( wxCommandEvent& event );

    /* Block operations: */

    /**
     * Function ReturnBlockCommand
     * Returns the block command internat code (BLOCK_MOVE, BLOCK_COPY...)
     * corresponding to the keys pressed (ALT, SHIFT, SHIFT ALT ..) when
     * block command is started by dragging the mouse.
     * @param aKey = the key modifiers (Alt, Shift ...)
     * @return the block command id (BLOCK_MOVE, BLOCK_COPY...)
     */
    virtual int  ReturnBlockCommand( int key );

    /**
     * Function HandleBlockPlace( )
     * Called after HandleBlockEnd, when a block command needs to be
     * executed after the block is moved to its new place
     * (bloc move, drag, copy .. )
     * Parameters must be initialized in GetScreen()->m_BlockLocate
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

    /**
     * Function Block_SelectItems
     * Uses  GetScreen()->m_BlockLocate
     * select items within the selected block.
     * selected items are put in the pick list
     * @param none
     */
    void Block_SelectItems();

    /**
     * Function Block_Delete
     * deletes all items within the selected block.
     * @param none
     */
    void Block_Delete();

    /**
     * Function Block_Rotate
     * Rotate all items within the selected block.
     * The rotation center is the center of the block
     * @param none
     */
    void Block_Rotate();

    /**
     * Function Block_Flip
     * Flip items within the selected block.
     * The flip center is the center of the block
     * @param none
     */
    void Block_Flip();

    /**
     * Function Block_Move
     * move all items within the selected block.
     * New location is determined by the current offset from the selected
     *block's original location.
     * @param none
     */
    void Block_Move();

    /**
     * Function Block_Mirror_X
     * mirrors all items within the currently selected block in the X axis.
     * @param none
     */
    void Block_Mirror_X();

    /**
     * Function Block_Duplicate
     * Duplicate all items within the selected block.
     * New location is determined by the current offset from the selected
     * block's original location.
     * @param none
     */
    void Block_Duplicate();


    void SetToolbars();
    void Process_Settings( wxCommandEvent& event );
    void OnConfigurePcbOptions( wxCommandEvent& aEvent );
    void InstallDisplayOptionsDialog( wxCommandEvent& aEvent );
    void InstallPcbGlobalDeleteFrame( const wxPoint& pos );

    void GenModulesPosition( wxCommandEvent& event );
    void GenModuleReport( wxCommandEvent& event );
    void InstallDrillFrame( wxCommandEvent& event );
    void ToPostProcess( wxCommandEvent& event );

    void OnFileHistory( wxCommandEvent& event );
    void Files_io( wxCommandEvent& event );

    /**
     * Function LoadOnePcbFile
     *  Load a Kicad board (.brd) file.
     *
     *  @param aFileName - File name including path. If empty, a file dialog will
     *                     be displayed.
     *  @param aAppend - Append board file aFileName to the currently loaded file if true.
     *                   Default = false.
     *  @param aForceFileDialog - Display the file open dialog even if aFullFileName is
     *                            valid if true; Default = false.
     *
     *  @return False if file load fails or is cancelled by the user, otherwise true.
     */
    bool LoadOnePcbFile( const wxString& aFileName, bool aAppend = false,
                         bool aForceFileDialog = false );


    /**
     * Function ReadPcbFile
     * reads a board file  <file>.brd
     * @param Append if 0: a previously loaded board is deleted before loading
     *               the file else all items of the board file are added to the
     *               existing board
     */
    int  ReadPcbFile( FILE* File, bool Append );

    bool SavePcbFile( const wxString& FileName );
    int  SavePcbFormatAscii( FILE* File );
    bool WriteGeneralDescrPcb( FILE* File );

    // BOARD handling

    /**
     * Function Clear_Pcb
     * delete all and reinitialize the current board
     * @param aQuery = true to prompt user for confirmation, false to
     *                 initialize silently
     */
    bool Clear_Pcb( bool aQuery );

    // Drc control

    /* function GetDrcController
     * @return the DRC controller
     */
    DRC* GetDrcController() { return m_drc; }

    /**
     * Function RecreateBOMFileFromBoard
     * Recreates a .cmp file from the current loaded board
     * this is the same as created by cvpcb.
     * can be used if this file is lost
     */
    void       RecreateCmpFileFromBoard( wxCommandEvent& aEvent );

    /**
     * Function RecreateBOMFileFromBoard
     * Creates a BOM file from the current loaded board
     */
    void       RecreateBOMFileFromBoard( wxCommandEvent& aEvent );

    void       ExportToGenCAD( wxCommandEvent& event );

    /**
     * Function OnExportVRML
     * will export the current BOARD to a VRML file.
     */
    void OnExportVRML( wxCommandEvent& event );

    /**
     * Function ExportVRML_File
     * Creates the file(s) exporting current BOARD to a VRML file.
     * @param aFullFileName = the full filename of the file to create
     * @param aScale = the general scaling factor. 1.0 to export in inches
     * @param aExport3DFiles = true to copy 3D shapes in the subir a3D_Subdir
     * @param a3D_Subdir = sub directory where 3D sahpes files are copied
     * used only when aExport3DFiles == true
     * @return true if Ok.
     */
    bool ExportVRML_File( const wxString & aFullFileName, double aScale,
                    bool aExport3DFiles, const wxString & a3D_Subdir );

    /**
     * Function ExporttoSPECCTRA
     * will export the current BOARD to a specctra dsn file.  See
     * See http://www.autotraxeda.com/docs/SPECCTRA/SPECCTRA.pdf for the
     * specification.
     */
    void       ExportToSpecctra( wxCommandEvent& event );

    /**
     * Function ImportSpecctraSession
     * will import a specctra *.ses file and use it to relocate MODULEs and
     * to replace all vias and tracks in an existing and loaded BOARD.
     * See http://www.autotraxeda.com/docs/SPECCTRA/SPECCTRA.pdf for the
     * specification.
     */
    void       ImportSpecctraSession( wxCommandEvent& event );

    /**
     * Function ImportSpecctraDesign
     * will import a specctra *.dsn file and use it to replace an entire BOARD.
     * The new board will not have any graphics, only components, tracks and
     * vias.
     * See http://www.autotraxeda.com/docs/SPECCTRA/SPECCTRA.pdf for the
     * specification.
     */
    void       ImportSpecctraDesign( wxCommandEvent& event );

    /**
     * Function Access_to_External_Tool
     * Run an external tool (like freeroute )
     */
    void       Access_to_External_Tool( wxCommandEvent& event );

    MODULE*    ListAndSelectModuleName();

    /**
     * Function ListNetsAndSelect
     * called by a command event
     * displays the sorted list of nets in a dialog frame
     * If a net is selected, it is highlighted
     */
    void       ListNetsAndSelect( wxCommandEvent& event );

    void       Swap_Layers( wxCommandEvent& event );

    // Handling texts on the board
    void       Rotate_Texte_Pcb( TEXTE_PCB* TextePcb, wxDC* DC );
    TEXTE_PCB* Create_Texte_Pcb( wxDC* DC );
    void       Delete_Texte_Pcb( TEXTE_PCB* TextePcb, wxDC* DC );
    void       StartMoveTextePcb( TEXTE_PCB* TextePcb, wxDC* DC );
    void       Place_Texte_Pcb( TEXTE_PCB* TextePcb, wxDC* DC );
    void       InstallTextPCBOptionsFrame( TEXTE_PCB* TextPCB, wxDC* DC );

    // Graphic Segments type DRAWSEGMENT
    void       Start_Move_DrawItem( DRAWSEGMENT* drawitem, wxDC* DC );
    void       Place_DrawItem( DRAWSEGMENT* drawitem, wxDC* DC );
    void       InstallGraphicItemPropertiesDialog( DRAWSEGMENT* aItem,
                                                   wxDC*        aDC );

    // Footprint edition (see also WinEDA_BasePcbFrame)
    void       InstallModuleOptionsFrame( MODULE* Module, wxDC* DC );
    void       StartMove_Module( MODULE* module, wxDC* DC );
    bool       Delete_Module( MODULE* module,
                              wxDC*   DC,
                              bool    aAskBeforeDeleting );
    void       Change_Side_Module( MODULE* Module, wxDC* DC );

    void       InstallExchangeModuleFrame( MODULE* ExchangeModuleModule );

    /**
     * Function Exchange_Module
     * Replaces OldModule by NewModule, using OldModule settings:
     * position, orientation, pad netnames ...)
     * OldModule is deleted or put in undo list.
     * @param aOldModule = footprint to replace
     * @param aNewModule = footprint to put
     * @param aUndoPickList = the undo list used to save  OldModule. If null,
     *                        OldModule is deleted
     */
    void       Exchange_Module( MODULE*            aOldModule,
                                MODULE*            aNewModule,
                                PICKED_ITEMS_LIST* aUndoPickList );

    // loading modules: see WinEDA_BasePcbFrame

    // Board handling
    void   RemoveStruct( BOARD_ITEM* Item, wxDC* DC );

    /**
     * Function OnEditItemRequest
     * Install the corresponding dialog editor for the given item
     * @param DC = the current device context
     * @param aItem = a pointer to the BOARD_ITEM to edit
     */
    void OnEditItemRequest( wxDC* DC, BOARD_ITEM* aItem );


    // Highlight functions:
    int    Select_High_Light( wxDC* DC );
    void   High_Light( wxDC* DC );

    // Track and via edition:
    void   Via_Edit_Control( wxCommandEvent& event );

    /* Return true if a microvia can be put on board
     * A microvia is a small via restricted to 2 near neighbor layers
     * because its is hole is made by laser which can penetrate only one layer
     * It is mainly used to connect BGA to the first inner layer
     * And it is allowed from an external layer to the first inner layer
     */
    bool IsMicroViaAcceptable( void );

    /**
     * Function Other_Layer_Route
     * operates in one of two ways.  If argument track is NULL, then swap the
     * active layer between m_Route_Layer_TOP and m_Route_Layer_BOTTOM.  If a
     * track is in progress (track is not NULL), and if DRC allows it, place
     * a via on the end of the current track, and then swap the current active
     * layer and start a new segment on the new layer.
     * @param track A TRACK* to append the via to or NULL.
     * @param DC A device context to draw on.
     * @return bool - true if the operation was successful, else false such as
     *                the case where DRC would not allow a via.
     */
    bool   Other_Layer_Route( TRACK* track, wxDC* DC );
    void   Affiche_PadsNoConnect( wxDC* DC );
    void   Affiche_Status_Net( wxDC* DC );
    TRACK* Delete_Segment( wxDC* DC, TRACK* Track );
    void   Delete_Track( wxDC* DC, TRACK* Track );
    void   Delete_net( wxDC* DC, TRACK* Track );
    void   Remove_One_Track( wxDC* DC, TRACK* pt_segm );

    /**
     * Function Reset_All_Tracks_And_Vias_To_Netclass_Values
     * Reset all tracks width and/or vias diameters and drill
     * to their default Netclass value
     * @param aTrack : bool true to modify tracks
     * @param aVia : bool true to modify vias
     */
    bool   Reset_All_Tracks_And_Vias_To_Netclass_Values( bool aTrack,
                                                         bool aVia );

    /**
     * Function Change_Net_Tracks_And_Vias_Sizes
     * Reset all tracks width and vias diameters and drill
     * to their default Netclass value or current values
     * @param aNetcode : the netcode of the net to edit
     * @param aUseNetclassValue : bool. True to use netclass values, false to
     *                            use current values
     */
    bool   Change_Net_Tracks_And_Vias_Sizes( int  aNetcode,
                                             bool aUseNetclassValue );

    /**
     * Function Edit_Track_Width
     * Modify a full track width (using DRC control).
     * a full track is the set of track segments between 2 ends: pads or a
     * point that has more than 2 segments ends connected
     * @param  DC = the curred device context (can be NULL)
     * @param aTrackSegment = a segment or via on the track to change
     */
    void   Edit_Track_Width( wxDC* DC, TRACK* Track );

    /**
     * Function Edit_TrackSegm_Width
     *  Modify one track segment width or one via diameter (using DRC control).
     * @param  DC = the current device context (can be NULL)
     * @param aTrackItem = the track segment or via to modify
     */
    void   Edit_TrackSegm_Width( wxDC* DC, TRACK* segm );
    TRACK* Begin_Route( TRACK* track, wxDC* DC );
    void   End_Route( TRACK* track, wxDC* DC );
    void   ExChange_Track_Layer( TRACK* pt_segm, wxDC* DC );
    void   Attribut_Segment( TRACK* track, wxDC* DC, bool Flag_On );
    void   Attribut_Track( TRACK* track, wxDC* DC, bool Flag_On );
    void   Attribut_net( wxDC* DC, int net_code, bool Flag_On );
    void   Start_MoveOneNodeOrSegment( TRACK* track, wxDC* DC, int command );
    bool   PlaceDraggedOrMovedTrackSegment( TRACK* Track, wxDC* DC );
    bool   MergeCollinearTracks( TRACK* track, wxDC* DC, int end );
    void   Start_DragTrackSegmentAndKeepSlope( TRACK* track, wxDC* DC );
    void   SwitchLayer( wxDC* DC, int layer );
    bool   Add_45_degrees_Segment( wxDC* DC );
    bool   Genere_Pad_Connexion( wxDC* DC, int layer );

    /**
     * Function EraseRedundantTrack
     * Called after creating a track
     * Remove (if exists) the old track that have the same starting and the
     * same ending point as the new created track
     * (this is the redunding track)
     * @param aDC = the current device context (can be NULL)
     * @param aNewTrack = the new created track (a pointer to a segment of the
     *                    track list)
     * @param aNewTrackSegmentsCount = number of segments in this new track
     * @param aItemsListPicker = the list picker to use for an undo command
     *                           (can be NULL)
     */
    int    EraseRedundantTrack( wxDC*              aDC,
                                TRACK*             aNewTrack,
                                int                aNewTrackSegmentsCount,
                                PICKED_ITEMS_LIST* aItemsListPicker );

    /**
     * Function SetTrackSegmentWidth
     *  Modify one track segment width or one via diameter (using DRC control).
     *  Basic routine used by other routines when editing tracks or vias
     * @param aTrackItem = the track segment or via to modify
     * @param aItemsListPicker = the list picker to use for an undo command
     *                           (can be NULL)
     * @param aUseNetclassValue = true to use NetClass value, false to use
     *                            current designSettings value
     * @return  true if done, false if no not change (because DRC error)
     */
    bool SetTrackSegmentWidth( TRACK*             aTrackItem,
                               PICKED_ITEMS_LIST* aItemsListPicker,
                               bool               aUseNetclassValue );


    // zone handling

    /**
     * Function Delete_Zone_Fill
     * Remove the zone filling which include the segment aZone, or the zone
     * which have the given time stamp.  A zone is a group of segments which
     * have the same TimeStamp
     * @param aZone = zone segment within the zone to delete. Can be NULL
     * @param aTimestamp = Timestamp for the zone to delete, used if aZone ==
     *                     NULL
     */
    void Delete_Zone_Fill( SEGZONE* Track, long aTimestamp = 0 );


    /**
     * Function Delete_LastCreatedCorner
     * Used only while creating a new zone outline
     * Remove and delete the current outline segment in progress
     * @return 0 if no corner in list, or corner number
     */
    int  Delete_LastCreatedCorner( wxDC* DC );

    /**
     * Function Begin_Zone
     * initiates a zone edge creation process,
     * or terminates the current zone edge and creates a new zone edge stub
     */
    int  Begin_Zone( wxDC* DC );

    /**
     * Function End_Zone
     * terminates (if no DRC error ) the zone edge creation process
     * @param DC = current Device Context
     * @return true if Ok, false if DRC error
     */
    bool End_Zone( wxDC* DC );

    /**
     * Function Fill_Zone
     *  Calculate the zone filling for the outline zone_container
     *  The zone outline is a frontier, and can be complex (with holes)
     *  The filling starts from starting points like pads, tracks.
     * If exists the old filling is removed
     * @param zone_container = zone to fill
     * @param verbose = true to show error messages
     * @return error level (0 = no error)
     */
    int  Fill_Zone( ZONE_CONTAINER* zone_container, bool verbose = TRUE );

    /**
     * Function Fill_All_Zones
     *  Fill all zones on the board
     * The old fillings are removed
     * @param verbose = true to show error messages
     * @return error level (0 = no error)
     */
    int  Fill_All_Zones( bool verbose = TRUE );


    /**
     * Function Add_Zone_Cutout
     * Add a cutout zone to a given zone outline
     * @param DC = current Device Context
     * @param zone_container = parent zone outline
     */
    void Add_Zone_Cutout( wxDC* DC, ZONE_CONTAINER* zone_container );

    /**
     * Function Add_Similar_Zone
     * Add a zone to a given zone outline.
     * if the zones are overlapping they will be merged
     * @param DC = current Device Context
     * @param zone_container = parent zone outline
     */
    void Add_Similar_Zone( wxDC* DC, ZONE_CONTAINER* zone_container );

    /**
     * Function Edit_Zone_Params
     * Edit params (layer, clearance, ...) for a zone outline
     */
    void Edit_Zone_Params( wxDC* DC, ZONE_CONTAINER* zone_container );

    /**
     * Function Start_Move_Zone_Corner
     * Prepares a move corner in a zone outline,
     * called from a move corner command (IsNewCorner = false),
     * or a create new cornet command (IsNewCorner = true )
     */
    void Start_Move_Zone_Corner( wxDC*           DC,
                                 ZONE_CONTAINER* zone_container,
                                 int             corner_id,
                                 bool            IsNewCorner );

    /**
     * Function Start_Move_Zone_Corner
     * Prepares a drag edge in an existing zone outline,
     */
    void Start_Move_Zone_Drag_Outline_Edge(
        wxDC* DC,
        ZONE_CONTAINER*
              zone_container,
        int   corner_id );

    /**
     * Function End_Move_Zone_Corner_Or_Outlines
     * Terminates a move corner in a zone outline, or a move zone outlines
     * @param DC = current Device Context (can be NULL)
     * @param zone_container: the given zone
     */
    void End_Move_Zone_Corner_Or_Outlines(
        wxDC* DC,
        ZONE_CONTAINER*
              zone_container );

    /**
     * Function End_Move_Zone_Corner_Or_Outlines
     * Remove the currently selected corner in a zone outline
     * the .m_CornerSelection is used as corner selection
     */
    void         Remove_Zone_Corner( wxDC* DC, ZONE_CONTAINER* zone_container );

    /**
     * Function Delete_Zone
     * Remove the zone which include the segment aZone, or the zone which have
     * the given time stamp.  A zone is a group of segments which have the
     * same TimeStamp
     * @param DC = current Device Context (can be NULL)
     * @param zone_container = zone to modify
     *  the member .m_CornerSelection is used to find the outline to remove.
     * if the outline is the main outline, all the zone is removed
     * otherwise, the hole is deleted
     */
    void         Delete_Zone_Contour( wxDC* DC, ZONE_CONTAINER* zone_container );

    /**
     * Function Start_Move_Zone_Outlines
     * Initialize parameters to move an existing zone outlines.
     * @param DC = current Device Context (can be NULL)
     * @param zone_container: the given zone to move
     */
    void         Start_Move_Zone_Outlines( wxDC*           DC,
                                           ZONE_CONTAINER* zone_container );

    // Target handling
    MIREPCB*     Create_Mire( wxDC* DC );
    void         Delete_Mire( MIREPCB* MirePcb, wxDC* DC );
    void         StartMove_Mire( MIREPCB* MirePcb, wxDC* DC );
    void         Place_Mire( MIREPCB* MirePcb, wxDC* DC );
    void         InstallMireOptionsFrame( MIREPCB*       MirePcb,
                                          wxDC*          DC );

    // Graphic segments type DRAWSEGMENT handling:
    DRAWSEGMENT* Begin_DrawSegment( DRAWSEGMENT* Segment, int shape, wxDC* DC );
    void         End_Edge( DRAWSEGMENT* Segment, wxDC* DC );
    void         Delete_Segment_Edge( DRAWSEGMENT* Segment, wxDC* DC );
    void         Delete_Drawings_All_Layer( int aLayer );

    // Dimension handling:
    void         Install_Edit_Dimension( DIMENSION*      Dimension,
                                        wxDC*          DC );
    DIMENSION*    Begin_Dimension( DIMENSION* Dimension, wxDC* DC );
    void         Delete_Dimension( DIMENSION* Dimension, wxDC* DC );


    // netlist  handling:
    void         InstallNetlistFrame( wxDC* DC, const wxPoint& pos );

    /**
     * Function ReadPcbNetlist
     * Update footprints (load missing footprints and delete on request extra
     * footprints)
     * Update connectivity info ( Net Name list )
     * Update Reference, value and "TIME STAMP"
     * @param aNetlistFullFilename = netlist file name (*.net)
     * @param aCmpFullFileName = cmp/footprint list file name (*.cmp) if not found,
     * only the netlist will be used
     * @return true if Ok
     *
     *  the format of the netlist is something like:
     # EESchema Netlist Version 1.0 generee le  18/5/2005-12:30:22
     *  (
     *  ( 40C08647 $noname R20 4,7K {Lib=R}
     *  (    1 VCC )
     *  (    2 MODB_1 )
     *  )
     *  ( 40C0863F $noname R18 4,7_k {Lib=R}
     *  (    1 VCC )
     *  (    2 MODA_1 )
     *  )
     *  }
     * #End
     */
    bool ReadPcbNetlist(
                         const wxString&  aNetlistFullFilename,
                         const wxString&  aCmpFullFileName,
                         wxTextCtrl*      aMessageWindow,
                         bool             aChangeFootprint,
                         bool             aDeleteBadTracks,
                         bool             aDeleteExtraFootprints,
                         bool             aSelect_By_Timestamp );

    /**
     * Function RemoveMisConnectedTracks
     * finds all track segments which are mis-connected (to more than one net).
     * When such a bad segment is found, mark it as needing to be removed.
     * and remove all tracks having at least one flagged segment.
     * @param aDC = the current device context (can be NULL)
     * @param aDisplayActivity = true to display activity on the frame status bar and message panel
     * @return true if any change is made
     */
    bool RemoveMisConnectedTracks( wxDC* aDC, bool aDisplayActivity );


    // Autoplacement:
    void         AutoPlace( wxCommandEvent& event );

    /**
     * Function OnOrientFootprints
     * install the dialog box for the common Orient Footprints
     */
    void         OnOrientFootprints( void );

    /**
     * Function ReOrientModules
     * Set the orientation of footprints
     * @param ModuleMask = mask (wildcard allowed) selection
     * @param Orient = new orientation
     * @param include_fixe = true to orient locked footprints
     */
    void         ReOrientModules( const wxString& ModuleMask, int Orient,
                                  bool include_fixe );
    void         FixeModule( MODULE* Module, bool Fixe );
    void         AutoMoveModulesOnPcb( bool PlaceModulesHorsPcb );
    bool         SetBoardBoundaryBoxFromEdgesOnly();
    void         AutoPlaceModule( MODULE* Module, int place_mode, wxDC* DC );
    int          RecherchePlacementModule( MODULE* Module, wxDC* DC );
    void         GenModuleOnBoard( MODULE* Module );
    float        Compute_Ratsnest_PlaceModule( wxDC* DC );
    int          GenPlaceBoard();
    void         DrawInfoPlace( wxDC* DC );

    // Autorouting:
    int          Solve( wxDC* DC, int two_sides );
    void         Reset_Noroutable( wxDC* DC );
    void         Autoroute( wxDC* DC, int mode );
    void         ReadAutoroutedTracks( wxDC* DC );
    void         GlobalRoute( wxDC* DC );

    void         Show_1_Ratsnest( EDA_BaseStruct* item, wxDC* DC );
    void         Clean_Pcb( wxDC* DC );

    void         InstallFindFrame( const wxPoint& pos, wxDC* DC );

    /**
     * Function SendMessageToEESCHEMA
     * sends a message to the schematic editor so that it may move its cursor
     * to a part with the same reference as the objectToSync
     * @param objectToSync The object whose reference is used to synchronize
     *                     eeschema.
     */
    void         SendMessageToEESCHEMA( BOARD_ITEM* objectToSync );

    /* Micro waves functions */
    void         Edit_Gap( wxDC* DC, MODULE* Module );
    MODULE*      Create_MuWaveBasicShape( const wxString& name, int pad_count );
    MODULE*      Create_MuWaveComponent( int shape_type );
    MODULE*      Create_MuWavePolygonShape();
    void         Begin_Self( wxDC* DC );
    MODULE*      Genere_Self( wxDC* DC );

    /**
     * Function SetLanguage
     * called on a language menu selection
     */
    virtual void SetLanguage( wxCommandEvent& event );


    DECLARE_EVENT_TABLE()
};


#endif  /* WXPCB_STRUCT_H */
