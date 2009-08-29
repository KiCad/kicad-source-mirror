/***********************************************************/
/*                      wxPcbStruct.h:                     */
/*      Classes used in pcbnew, cvpcb and gerbview         */
/***********************************************************/

#ifndef  WX_BASE_PCB_FRAME_H
#define  WX_BASE_PCB_FRAME_H


#include <vector>

#include "wxstruct.h"
#include "base_struct.h"

#ifndef PCB_INTERNAL_UNIT
#define PCB_INTERNAL_UNIT 10000
#endif


/*  Forward declarations of classes. */
class PCB_SCREEN;
class WinEDA_Toolbar;
class WinEDA_CvpcbFrame;
class WinEDA_PcbFrame;
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
class COTATION;
class EDGE_MODULE;
class WinEDA3D_DrawFrame;
class DRC;
class ZONE_CONTAINER;
class DRAWSEGMENT;
class GENERAL_COLLECTOR;
class GENERAL_COLLECTORS_GUIDE;


/******************************************************************/
/* class WinEDA_BasePcbFrame: Basic class for pcbnew and gerbview */
/******************************************************************/

class WinEDA_BasePcbFrame : public WinEDA_DrawFrame
{
public:

    bool m_DisplayPadFill;          // How show pads
    bool m_DisplayPadNum;           // show pads numbers

    int m_DisplayModEdge;           // How show module drawings
    int m_DisplayModText;           // How show module texts
    bool m_DisplayPcbTrackFill;     /* FALSE : tracks are show in sketch mode, TRUE = filled */
    int m_UserGridUnits;
    wxRealPoint             m_UserGridSize;

    WinEDA3D_DrawFrame*     m_Draw3DFrame;
    WinEDA_ModuleEditFrame* m_ModuleEditFrame;

protected:
    BOARD*                  m_Pcb;
    GENERAL_COLLECTOR*      m_Collector;

public:
    WinEDA_BasePcbFrame( wxWindow* father, int idtype,
                         const wxString& title,
                         const wxPoint& pos, const wxSize& size,
                         long style = KICAD_DEFAULT_DRAWFRAME_STYLE );

    ~WinEDA_BasePcbFrame();

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
    virtual void             OnCloseWindow( wxCloseEvent& Event ) = 0;

    virtual void RedrawActiveWindow( wxDC* DC, bool EraseBg ) { }
    virtual void             ReCreateHToolbar() = 0;
    virtual void             ReCreateVToolbar() = 0;
    virtual void             OnLeftClick( wxDC* DC, const wxPoint& MousePos )  = 0;
    virtual void             OnLeftDClick( wxDC* DC, const wxPoint& MousePos ) = 0;
    virtual bool             OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu ) = 0;
    virtual void             ReCreateMenuBar();
    virtual void             SetToolID( int id, int new_cursor_id,
                                        const wxString& title );
    virtual void             UpdateStatusBar();

    PCB_SCREEN* GetScreen() const { return (PCB_SCREEN*) WinEDA_DrawFrame::GetBaseScreen(); }

    BASE_SCREEN*             GetBaseScreen() const;

    int                      BestZoom();

    virtual void             Show3D_Frame( wxCommandEvent& event );

public:

    // Read/write fonctions:
    EDA_BaseStruct*          ReadDrawSegmentDescr( FILE* File, int* LineNum );
    int                      ReadListeSegmentDescr( FILE* File,
                                                    TRACK* PtSegm, int StructType,
                                                    int* LineNum, int NumSegm );

    int                      ReadSetup( FILE* File, int* LineNum );
    int                      ReadGeneralDescrPcb( FILE* File, int* LineNum );


    /**
     * Function PcbGeneralLocateAndDisplay
     * searches for an item under the mouse cursor.
     * Items are searched first on the current working layer.
     * If nothing found, an item will be searched without layer restriction.  If
     * more than one item is found meeting the current working layer criterion, then
     * a popup menu is shown which allows the user to pick which item he/she is
     * interested in.  Once an item is chosen, then it is make the "current item"
     * and the status window is updated to reflect this.
     *
     * @param aHotKeyCode The hotkey which relates to the caller and determines the
     *  type of search to be performed.  If zero, then the mouse tools will be
     *  tested instead.
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
     */
    void                     SetCurItem( BOARD_ITEM* aItem );
    BOARD_ITEM*              GetCurItem();

    /**
     * Function GetCollectorsGuide
     * @return GENERAL_COLLECTORS_GUIDE - that considers the global configuration options.
     */
    GENERAL_COLLECTORS_GUIDE GetCollectorsGuide();


    /**
     * Function CursorGoto
     * positions the cursor at a given coordinate and reframes the drawing if the
     * requested point is out of view.
     * @param aPos The point to go to.
     */
    void                     CursorGoto(  const wxPoint& aPos );


    /* Place un repere sur l'ecran au point de coordonnees PCB pos */
    void                     place_marqueur( wxDC* DC, const wxPoint& pos, char* pt_bitmap,
                                             int DrawMode, int color, int type );

    // Gestion des modules
    MODULE*                  Copie_Module( MODULE* module );

    /** Function Save_Module_In_Library
     *  Save in an existing library a given footprint
     * @param aLibName = name of the library to use
     * @param aModule = the given footprint
     * @param aOverwrite = true to overwrite an existing footprint, false to abort an existing footprint is found
     * @param aDisplayDialog = true to display a dialog to enter or confirm the footprint name
     * @param aCreateDocFile = true to creates the associated doc file
     * @return : 1 if OK,0 if abort
     */
    int                      Save_Module_In_Library( const wxString& aLibName,
                                                     MODULE* aModule, bool aOverwrite,
                                                     bool aDisplayDialog, bool aCreateDocFile );

    void                     Archive_Modules( const wxString& LibName, bool NewModulesOnly );
    MODULE*                  Select_1_Module_From_BOARD( BOARD* Pcb );
    MODULE*                  GetModuleByName();

    // Modules (footprints)
    MODULE*                  Create_1_Module( wxDC* DC, const wxString& module_name );
    void                     Edit_Module( MODULE* module, wxDC* DC );
    void                     Rotate_Module( wxDC*   DC,
                                            MODULE* module,
                                            int     angle,
                                            bool    incremental );
    void                     Place_Module( MODULE* module,
                                           wxDC*   DC,
                                           bool    aDoNotRecreateRatsnest = false );

    // module texts
    void                     RotateTextModule( TEXTE_MODULE* Text, wxDC* DC );
    void                     DeleteTextModule( TEXTE_MODULE* Text );
    void                     PlaceTexteModule( TEXTE_MODULE* Text, wxDC* DC );
    void                     StartMoveTexteModule( TEXTE_MODULE* Text, wxDC* DC );
    TEXTE_MODULE*            CreateTextModule( MODULE* Module, wxDC* DC );

    void                     InstallPadOptionsFrame( D_PAD* pad, wxDC* DC, const wxPoint& pos );
    void                     InstallTextModOptionsFrame( TEXTE_MODULE* TextMod, wxDC* DC );

    // Pads sur modules
    void                     AddPad( MODULE* Module, bool draw );
    void                     DeletePad( D_PAD* Pad );
    void                     StartMovePad( D_PAD* Pad, wxDC* DC );
    void                     RotatePad( D_PAD* Pad, wxDC* DC );
    void                     PlacePad( D_PAD* Pad, wxDC* DC );
    void                     Export_Pad_Settings( D_PAD* aPad );
    void                     Import_Pad_Settings( D_PAD* aPad, bool aDraw );
    void                     Global_Import_Pad_Settings( D_PAD* aPad, bool aDraw );


    // loading footprints

    /** function Get_Librairie_Module
     *
     *  Read active libraries or one library to find and load a given module
     *  If found the lodule is linked to the tail of linked list of modules
     *  @param aLibrary: the full filename of the library to read. If empty, all active libraries are read
     *  @param aModuleName = module name to load
     *  @param aDisplayMessageError = true to display an error message if any.
     *  @return a MODULE * pointer to the new module, or NULL
     *
     */
    MODULE*      Get_Librairie_Module( const wxString& aLibraryFullFilename,
                                       const wxString& aModuleName,
                                       bool            aDisplayMessageError );

    /** Function Select_1_Module_From_List
     *  Display a list of modules found in active libraries or a given library
     *  @param aLibraryFullFilename = library to list (if aLibraryFullFilename == void, list all modules)
     *  @param aMask = Display filter (wildcart)( Mask = wxEmptyString if not used )
     *  @param aKeyWord = keyword list, to display a filtered list of module having one (or more) of these keyworks in their keywork list
     *    ( aKeyWord = wxEmptyString if not used )
     *
     *  @return wxEmptyString if abort or fails, or the selected module name if Ok
     */
    wxString     Select_1_Module_From_List(
        WinEDA_DrawFrame* active_window, const wxString& aLibraryFullFilename,
        const wxString& aMask, const wxString& aKeyWord );

    MODULE*      Load_Module_From_Library( const wxString& library, wxDC* DC );

    //  ratsnest functions
    void         Compile_Ratsnest( wxDC* DC, bool affiche );                /* Recalcul complet du chevelu */
    int          Test_1_Net_Ratsnest( wxDC* DC, int net_code );
    void         build_ratsnest_module( wxDC* DC, MODULE* Module );
    void         trace_ratsnest_module( wxDC* DC );
    void         Build_Board_Ratsnest( wxDC* DC );
    void         DrawGeneralRatsnest( wxDC* DC, int net_code = 0 );
    void         trace_ratsnest_pad( wxDC* DC );
    void         build_ratsnest_pad( BOARD_ITEM*    ref,
                                     const wxPoint& refpos,
                                     bool           init );

    void         Tst_Ratsnest( wxDC* DC, int ref_netcode );
    void         test_connexions( wxDC* DC );
    void         test_1_net_connexion( wxDC* DC, int net_code );
    void         RecalculateAllTracksNetcode();

    /* Plotting functions:
     */
    void         ToPlotter( wxCommandEvent& event );
    void         Genere_GERBER( const wxString& FullFileName, int Layer,
                                bool PlotOriginIsAuxAxis, GRTraceMode trace_mode );
    void         Genere_HPGL( const wxString& FullFileName, int Layer, GRTraceMode trace_mode );
    void         Genere_PS( const wxString& FullFileName,
                            int             Layer,
                            bool            useA4,
                            GRTraceMode     trace_mode );
    void         Genere_DXF( const wxString& FullFileName, int Layer, GRTraceMode trace_mode );
    void         Plot_Layer( PLOTTER* plotter, int Layer, GRTraceMode trace_mode );
    void         Plot_Standard_Layer( PLOTTER* plotter, int masque_layer,
                                      int garde, bool trace_via,
                                      GRTraceMode trace_mode );
    void         Plot_Serigraphie( PLOTTER* plotter, int masque_layer, GRTraceMode trace_mode );

    /** function PlotDrillMark
     * Draw a drill mark for pads and vias.
     * Must be called after all drawings, because it
     * redraw the drill mark on a pad or via, as a negative (i.e. white) shape in FILLED plot mode
     * @param aPlotter = the PLOTTER
     * @param aTraceMode = the mode of plot (FILLED, SKETCH)
     * @param aSmallDrillShape = true to plot a smalle drill shape, false to plot the actual drill shape
     */
    void         PlotDrillMark( PLOTTER* aPlotter, GRTraceMode aTraceMode, bool aSmallDrillShape );

    /* Functions relative to Undo/redo commands:
     */

    /** Function SaveCopyInUndoList (virtual pure)
     * Creates a new entry in undo list of commands.
     * add a picker to handle aItemToCopy
     * @param aItemToCopy = the board item modified by the command to undo
     * @param aTypeCommand = command type (see enum UndoRedoOpType)
     * @param aTransformPoint = the reference point of the transformation, for commands like move
     */
    virtual void SaveCopyInUndoList( BOARD_ITEM* aItemToCopy, UndoRedoOpType aTypeCommand,
                                    const wxPoint& aTransformPoint = wxPoint( 0, 0 ) ) = 0;

    /** Function SaveCopyInUndoList (virtual pure, overloaded).
     * Creates a new entry in undo list of commands.
     * add a list of pickers to handle a list of items
     * @param aItemsList = the list of items modified by the command to undo
     * @param aTypeCommand = command type (see enum UndoRedoOpType)
     * @param aTransformPoint = the reference point of the transformation, for commands like move
     */
    virtual void SaveCopyInUndoList( PICKED_ITEMS_LIST& aItemsList, UndoRedoOpType aTypeCommand,
                                    const wxPoint& aTransformPoint = wxPoint( 0, 0 ) ) = 0;


    // layerhandling:
    // (See pcbnew/sel_layer.cpp for description of why null_layer parameter is provided)
    int          SelectLayer( int default_layer, int min_layer, int max_layer,
                              bool null_layer = false );
    void         SelectLayerPair();
    virtual void SwitchLayer( wxDC* DC, int layer );

    // divers
    void         AddHistory( int value, KICAD_T type );                // Add value in data list history
    void         InstallGridFrame( const wxPoint& pos );

    virtual void LoadSettings();
    virtual void SaveSettings();

    DECLARE_EVENT_TABLE()
};

#endif  /* WX_BASE_PCB_FRAME_H */
