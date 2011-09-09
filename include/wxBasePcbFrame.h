/***********************************************************/
/*                      wxPcbStruct.h:                     */
/*      Classes used in pcbnew, cvpcb and gerbview         */
/***********************************************************/

#ifndef  WX_BASE_PCB_FRAME_H
#define  WX_BASE_PCB_FRAME_H


#include <vector>

#include "wxstruct.h"
#include "base_struct.h"
#include "richio.h"
#include "class_pcb_screen.h"

#ifndef PCB_INTERNAL_UNIT
#define PCB_INTERNAL_UNIT 10000
#endif


/*  Forward declarations of classes. */
class WinEDA_CvpcbFrame;
class PCB_EDIT_FRAME;
class FOOTPRINT_EDIT_FRAME;
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
class EDA_3D_FRAME;
class DRC;
class ZONE_CONTAINER;
class DRAWSEGMENT;
class GENERAL_COLLECTOR;
class GENERAL_COLLECTORS_GUIDE;


/******************************************************************/
/* class PCB_BASE_FRAME: Basic class for pcbnew and gerbview */
/******************************************************************/

class PCB_BASE_FRAME : public EDA_DRAW_FRAME
{
public:
    bool m_DisplayPadFill;          // How show pads
    bool m_DisplayViaFill;          // How show vias
    bool m_DisplayPadNum;           // show pads numbers

    int  m_DisplayModEdge;          // How to display module drawings (line/ filled / sketch)
    int  m_DisplayModText;          // How to display module texts (line/ filled / sketch)
    bool m_DisplayPcbTrackFill;     /* FALSE : tracks are show in sketch mode,
                                     * TRUE = filled */
    EDA_UNITS_T             m_UserGridUnit;
    wxRealPoint             m_UserGridSize;

    int                     m_FastGrid1;
    int                     m_FastGrid2;

    EDA_3D_FRAME*           m_Draw3DFrame;
    FOOTPRINT_EDIT_FRAME*   m_ModuleEditFrame;

protected:
    BOARD*                  m_Pcb;
    GENERAL_COLLECTOR*      m_Collector;

    void updateGridSelectBox();
    void updateZoomSelectBox();
    virtual void unitsChangeRefresh();

public:
    PCB_BASE_FRAME( wxWindow* father, int idtype, const wxString& title,
                    const wxPoint& pos, const wxSize& size,
                    long style = KICAD_DEFAULT_DRAWFRAME_STYLE );

    ~PCB_BASE_FRAME();

    /**
     * Function SetBoard
     * sets the m_Pcb member in such as way as to ensure deleting any previous
     * BOARD.
     * @param aBoard The BOARD to put into the frame.
     */
    void SetBoard( BOARD* aBoard );

    BOARD* GetBoard()
    {
        wxASSERT( m_Pcb );  // phasing out m_Pcb for gerbview
        return m_Pcb;
    }


    // General
    virtual void    OnCloseWindow( wxCloseEvent& Event ) = 0;

    virtual void    RedrawActiveWindow( wxDC* DC, bool EraseBg ) { }
    virtual void    ReCreateHToolbar() = 0;
    virtual void    ReCreateVToolbar() = 0;
    virtual void    OnLeftClick( wxDC*          DC,
                                 const wxPoint& MousePos ) = 0;
    virtual void    OnLeftDClick( wxDC*          DC,
                                  const wxPoint& MousePos ) = 0;
    virtual bool    OnRightClick( const wxPoint& MousePos,
                                  wxMenu*        PopMenu )  = 0;
    virtual void    ReCreateMenuBar();
    virtual void    SetToolID( int aId, int aCursor, const wxString& aToolMsg );
    virtual void    UpdateStatusBar();

    virtual PCB_SCREEN* GetScreen() const
    {
        return (PCB_SCREEN*) EDA_DRAW_FRAME::GetScreen();
    }

    virtual double  BestZoom();

    virtual void    Show3D_Frame( wxCommandEvent& event );

public:

    // Read/write functions:
    EDA_ITEM* ReadDrawSegmentDescr( LINE_READER* aReader );
    int             ReadListeSegmentDescr( LINE_READER* aReader,
                                           TRACK* PtSegm,
                                           int    StructType,
                                           int    NumSegm );

    int             ReadSetup( LINE_READER* aReader );
    int             ReadGeneralDescrPcb( LINE_READER* aReader );


    /**
     * Function PcbGeneralLocateAndDisplay
     * searches for an item under the mouse cursor.
     * Items are searched first on the current working layer.
     * If nothing found, an item will be searched without layer restriction.
     * If more than one item is found meeting the current working layer
     * criterion, then a popup menu is shown which allows the user to pick
     * which item he/she is interested in.  Once an item is chosen, then it
     * is make the "current item" and the status window is updated to reflect
     * this.
     *
     * @param aHotKeyCode The hotkey which relates to the caller and determines
     *                    the type of search to be performed.  If zero, then
     *                    the mouse tools will be tested instead.
     */
    BOARD_ITEM*              PcbGeneralLocateAndDisplay( int aHotKeyCode = 0 );

    void                     ProcessItemSelection( wxCommandEvent& event );

    /**
     * Function SetCurItem
     * sets the currently selected item and displays it in the MsgPanel.
     * If the given item is NULL then the MsgPanel is erased and there is no
     * currently selected item. This function is intended to make the process
     * of "selecting" an item more formal, and to indivisibly tie the operation
     * of selecting an item to displaying it using BOARD_ITEM::Display_Infos().
     * @param aItem The BOARD_ITEM to make the selected item or NULL if none.
     * @param aDisplayInfo = true to display item info, false if not (default =
     *true)
     */
    void                     SetCurItem( BOARD_ITEM* aItem,
                                         bool        aDisplayInfo = true );
    BOARD_ITEM*              GetCurItem();

    /**
     * Function GetCollectorsGuide
     * @return GENERAL_COLLECTORS_GUIDE - that considers the global
     *configuration options.
     */
    GENERAL_COLLECTORS_GUIDE GetCollectorsGuide();

    /**
     * Function CursorGoto
     * positions the cursor at a given coordinate and reframes the drawing if
     *the
     * requested point is out of view.
     * @param aPos The point to go to.
     */
    void                     CursorGoto(  const wxPoint& aPos );

    void                     place_marqueur( wxDC*          DC,
                                             const wxPoint& pos,
                                             char*          pt_bitmap,
                                             int            DrawMode,
                                             int            color,
                                             int            type );

    MODULE* Copie_Module( MODULE* module );

    /**
     * Function Save_Module_In_Library
     *  Save in an existing library a given footprint
     * @param aLibName = name of the library to use
     * @param aModule = the given footprint
     * @param aOverwrite = true to overwrite an existing footprint, false to
     *                     abort an existing footprint is found
     * @param aDisplayDialog = true to display a dialog to enter or confirm the
     *                         footprint name
     * @return : true if OK, false if abort
     */
    bool    Save_Module_In_Library( const wxString& aLibName,
                                    MODULE*         aModule,
                                    bool            aOverwrite,
                                    bool            aDisplayDialog );

    /**
     * Function Archive_Modules
     * Save in the library:
     * All new modules (ie modules not found in this lib) (if NewModulesOnly == true)
     * all modules (if NewModulesOnly == false)
     */
    void Archive_Modules( const wxString& LibName, bool NewModulesOnly );

    MODULE* GetModuleByName();

    /**
     * Function OnModify
     * Virtual
     * Must be called after a change
     * in order to set the "modify" flag of the current screen
     * and update the date in frame reference
     * do not forget to call this basic OnModify function to update info
     * in derived OnModify functions
     */
    virtual void OnModify( );

    // Modules (footprints)
    /**
     * Function Create_1_Module
     * Creates a new module or footprint : A new module contains 2 texts :
     *  First = REFERENCE
     *  Second = VALUE: "VAL**"
     * the new module is added to the board module list
     * @param aModuleName = name of the new footprint
     *                  (will be the component reference in board)
     * @return a pointer to the new module
     */
    MODULE* Create_1_Module( const wxString& aModuleName );

    void    Edit_Module( MODULE* module, wxDC* DC );
    void    Rotate_Module( wxDC*   DC,
                           MODULE* module,
                           int     angle,
                           bool    incremental );
    void    Place_Module( MODULE* module,
                          wxDC*   DC,
                          bool    aDoNotRecreateRatsnest = false );

    // module texts
    void          RotateTextModule( TEXTE_MODULE* Text, wxDC* DC );
    void          DeleteTextModule( TEXTE_MODULE* Text );
    void          PlaceTexteModule( TEXTE_MODULE* Text, wxDC* DC );
    void          StartMoveTexteModule( TEXTE_MODULE* Text,
                                        wxDC*         DC );
    TEXTE_MODULE* CreateTextModule( MODULE* Module, wxDC* DC );

    /**
     * Function ResetTextSize
     * resets given field text size and width to current settings in
     * Preferences->Dimensions->Texts and Drawings.
     * @param aItem is the item to be reset, either TEXTE_PCB or TEXTE_MODULE.
     * @param aDC is the drawing context.
     */
    void          ResetTextSize( BOARD_ITEM* aItem, wxDC* aDC );

    /**
     * Function ResetModuleTextSizes
     * resets text size and width of all module text fields of given field
     * type to current settings in Preferences->Dimensions->Texts and Drawings.
     * @param aType is the field type (TEXT_is_REFERENCE, TEXT_is_VALUE, or TEXT_is_DIVERS).
     * @param aDC is the drawing context.
     */
    void          ResetModuleTextSizes( int aType, wxDC* aDC );

    void          InstallPadOptionsFrame( D_PAD*         pad );
    void          InstallTextModOptionsFrame( TEXTE_MODULE* TextMod,
                                              wxDC*         DC );

    void          AddPad( MODULE* Module, bool draw );

    /**
     * Function DeletePad
     * Delete the pad aPad.
     * Refresh the modified screen area
     * Refresh modified parameters of the parent module (bounding box, last date)
     * @param aPad = the pad to delete
     * @param aQuery = true to prompt for confirmation, false to delete silently
     */
    void          DeletePad( D_PAD* aPad, bool aQuery = true );

    void          StartMovePad( D_PAD* Pad, wxDC* DC );
    void          RotatePad( D_PAD* Pad, wxDC* DC );
    void          PlacePad( D_PAD* Pad, wxDC* DC );
    void          Export_Pad_Settings( D_PAD* aPad );
    void          Import_Pad_Settings( D_PAD* aPad, bool aDraw );
    void          Global_Import_Pad_Settings( D_PAD* aPad,
                                              bool   aDraw );


    // loading footprints

    /**
     * Function Get_Librairie_Module
     *
     *  Read active libraries or one library to find and load a given module
     *  If found the module is linked to the tail of linked list of modules
     *  @param aLibraryFullFilename - the full filename of the library to read. If empty,
     *                   all active libraries are read
     *  @param aModuleName = module name to load
     *  @param aDisplayMessageError = true to display an error message if any.
     *  @return a pointer to the new module, or NULL
     *
     */
    MODULE*  Get_Librairie_Module( const wxString& aLibraryFullFilename,
                                   const wxString& aModuleName,
                                   bool            aDisplayMessageError );

    /**
     * Function Select_1_Module_From_List
     *  Display a list of modules found in active libraries or a given library
     *  @param aWindow = the current window ( parent window )
     *  @param aLibraryFullFilename = library to list (if aLibraryFullFilename
     *                                == void, list all modules)
     *  @param aMask = Display filter (wildcart)( Mask = wxEmptyString if not
     *                  used )
     *  @param aKeyWord = keyword list, to display a filtered list of module
     *                    having one (or more) of these keywords in their
     *                    keyword list ( aKeyWord = wxEmptyString if not used )
     *
     *  @return wxEmptyString if abort or fails, or the selected module name if
     *          Ok
     */
    wxString Select_1_Module_From_List( EDA_DRAW_FRAME* aWindow,
                                        const wxString& aLibraryFullFilename,
                                        const wxString& aMask,
                                        const wxString& aKeyWord );

    MODULE*  Load_Module_From_Library( const wxString& library, wxDC* DC );

    //  ratsnest functions
    /**
     * Function Compile_Ratsnest
     *  Create the entire board ratsnest.
     *  Must be called after a board change (changes for
     *  pads, footprints or a read netlist ).
     * @param aDC = the current device context (can be NULL)
     * @param aDisplayStatus : if true, display the computation results
     */
    void     Compile_Ratsnest( wxDC* aDC, bool aDisplayStatus );

    /**
     * Function Test_1_Net_Ratsnest
     * Compute the ratsnest relative to the net "net_code"
     * @param aDC - Device context to draw on.
     * @param aNetcode = netcode used to compute the ratsnest.
     */
    int      Test_1_Net_Ratsnest( wxDC* aDC, int aNetcode );

    /**
     * Function build_ratsnest_module
     * Build a ratsnest relative to one footprint. This is a simplified computation
     * used only in move footprint. It is not optimal, but it is fast and sufficient
     * to help a footprint placement
     * It shows the connections from a pad to the nearest connected pad
     * @param aModule = module to consider.
     */
    void     build_ratsnest_module( MODULE* aModule );

    void     trace_ratsnest_module( wxDC* DC );
    void     Build_Board_Ratsnest( wxDC* DC );

    /**
     *  function Displays the general ratsnest
     *  Only ratsnest with the status bit CH_VISIBLE is set are displayed
     * @param aDC = the current device context (can be NULL)
     * @param aNetcode if > 0, Display only the ratsnest relative to the
     * corresponding net_code
     */
    void     DrawGeneralRatsnest( wxDC* aDC, int aNetcode = 0 );

    void     trace_ratsnest_pad( wxDC* DC );
    void     build_ratsnest_pad( BOARD_ITEM*    ref,
                                 const wxPoint& refpos,
                                 bool           init );

    void     Tst_Ratsnest( wxDC* DC, int ref_netcode );
    void     test_connexions( wxDC* DC );
    void     test_1_net_connexion( wxDC* DC, int net_code );
    void     RecalculateAllTracksNetcode();

    /* Plotting functions:
     * Return true if OK, false if the file is not created (or has a problem)
     */

    bool     Genere_GERBER( const wxString& FullFileName,
                            int             Layer,
                            bool            PlotOriginIsAuxAxis,
                            GRTraceMode     trace_mode );
    bool     Genere_HPGL( const wxString& FullFileName,
                          int             Layer,
                          GRTraceMode     trace_mode );
    bool     Genere_PS( const wxString& FullFileName,
                        int             Layer,
                        bool            useA4,
                        GRTraceMode     trace_mode );
    bool     Genere_DXF( const wxString& FullFileName,
                         int             Layer,
                         GRTraceMode     trace_mode );
    void     Plot_Layer( PLOTTER*    plotter,
                         int         Layer,
                         GRTraceMode trace_mode );
    /**
     * Function Plot_Standard_Layer
     * plot copper or technical layers.
     * not used for silk screen layers, because these layers have specific
     * requirements, mainly for pads
     * @param aPlotter = the plotter to use
     * @param aLayerMask = the mask to define the layers to plot
     * @param aPlotVia = true to plot vias, false to skip vias (has meaning
     *                  only for solder mask layers).
     * @param aPlotMode = the plot mode (files, sketch). Has meaning for some formats only
     * @param aSkipNPTH_Pads = true to skip NPTH Pads, when the pad size and the pad hole
     *                      have the same size. Used in GERBER format only.
     */
    void     Plot_Standard_Layer( PLOTTER* aPlotter, int aLayerMask,
                                  bool aPlotVia, GRTraceMode aPlotMode,
                                  bool aSkipNPTH_Pads = false );

    void     Plot_Serigraphie( PLOTTER*    plotter,
                               int         masque_layer,
                               GRTraceMode trace_mode );

    /**
     * Function PlotDrillMark
     * Draw a drill mark for pads and vias.
     * Must be called after all drawings, because it
     * redraw the drill mark on a pad or via, as a negative (i.e. white) shape
     * in FILLED plot mode
     * @param aPlotter = the PLOTTER
     * @param aTraceMode = the mode of plot (FILLED, SKETCH)
     * @param aSmallDrillShape = true to plot a small drill shape, false to
     *                           plot the actual drill shape
     */
    void     PlotDrillMark( PLOTTER*    aPlotter,
                            GRTraceMode aTraceMode,
                            bool        aSmallDrillShape );

    /* Functions relative to Undo/redo commands:
     */

    /**
     * Function SaveCopyInUndoList (virtual pure)
     * Creates a new entry in undo list of commands.
     * add a picker to handle aItemToCopy
     * @param aItemToCopy = the board item modified by the command to undo
     * @param aTypeCommand = command type (see enum UNDO_REDO_T)
     * @param aTransformPoint = the reference point of the transformation, for
     *                          commands like move
     */
    virtual void SaveCopyInUndoList( BOARD_ITEM* aItemToCopy,
                                     UNDO_REDO_T aTypeCommand,
                                     const wxPoint& aTransformPoint = wxPoint( 0, 0 ) ) = 0;

    /**
     * Function SaveCopyInUndoList (virtual pure, overloaded).
     * Creates a new entry in undo list of commands.
     * add a list of pickers to handle a list of items
     * @param aItemsList = the list of items modified by the command to undo
     * @param aTypeCommand = command type (see enum UNDO_REDO_T)
     * @param aTransformPoint = the reference point of the transformation,
     *                          for commands like move
     */
    virtual void SaveCopyInUndoList( PICKED_ITEMS_LIST& aItemsList,
                                     UNDO_REDO_T aTypeCommand,
                                     const wxPoint& aTransformPoint = wxPoint( 0, 0 ) ) = 0;


    // layerhandling:
    // (See pcbnew/sel_layer.cpp for description of why null_layer parameter
    // is provided)
    int          SelectLayer( int default_layer, int min_layer, int max_layer,
                              bool null_layer = false );
    void         SelectLayerPair();
    virtual void SwitchLayer( wxDC* DC, int layer );

    void         InstallGridFrame( const wxPoint& pos );

    /**
     * Load applications settings common to PCB draw frame objects.
     *
     * This overrides the base class EDA_DRAW_FRAME::LoadSettings() to
     * handle settings common to the PCB layout application and footprint
     * editor main windows.  It calls down to the base class to load
     * settings common to all drawing frames.  Please put your application
     * settings common to all pcb drawing frames here to avoid having
     * application settings loaded all over the place.
     */
    virtual void LoadSettings();

    /**
     * Save applications settings common to PCB draw frame objects.
     *
     * This overrides the base class EDA_DRAW_FRAME::SaveSettings() to
     * save settings common to the PCB layout application and footprint
     * editor main windows.  It calls down to the base class to save
     * settings common to all drawing frames.  Please put your application
     * settings common to all pcb drawing frames here to avoid having
     * application settings saved all over the place.
     */
    virtual void SaveSettings();

    void OnTogglePolarCoords( wxCommandEvent& aEvent );
    void OnTogglePadDrawMode( wxCommandEvent& aEvent );

    /* User interface update event handlers. */
    void OnUpdateCoordType( wxUpdateUIEvent& aEvent );
    void OnUpdatePadDrawMode( wxUpdateUIEvent& aEvent );
    void OnUpdateSelectGrid( wxUpdateUIEvent& aEvent );
    void OnUpdateSelectZoom( wxUpdateUIEvent& aEvent );

    DECLARE_EVENT_TABLE()
};

#endif  /* WX_BASE_PCB_FRAME_H */
