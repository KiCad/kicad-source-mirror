/***********************************************************/
/*                   wxGerberFrame.h:                     */
/***********************************************************/

#ifndef  WX_GERBER_STRUCT_H
#define  WX_GERBER_STRUCT_H


#include "id.h"
#include "class_gerbview_layer_widget.h"
#include "class_layerchoicebox.h"

/**
 * Command IDs for the gerber file viewer.
 *
 * Please add IDs that are unique to the gerber file viewer here and not in the
 * global id.h file.  This will prevent the entire project from being rebuilt
 * when adding new command to the gerber file viewer.
 */
enum id_gerbview_frm {
    // A MenuItem ID of Zero does not work under Mac,first id = 1
    ID_GERBVIEW_SHOW_LIST_DCODES = 1,
    ID_GERBVIEW_LOAD_DRILL_FILE,
    ID_GERBVIEW_LOAD_DCODE_FILE,
    ID_TOOLBARH_GERBER_SELECT_TOOL,
    ID_MENU_INC_LAYER_AND_APPEND_FILE,
    ID_INC_LAYER_AND_APPEND_FILE,
    ID_GERBVIEW_SHOW_SOURCE,
    ID_GERBVIEW_EXPORT_TO_PCBNEW,
    ID_GERBVIEW_POPUP_DELETE_DCODE_ITEMS
};


class DCODE_SELECTION_BOX;

/******************************************************************
*   class WinEDA_GerberFrame: this is the main window used in gerbview
******************************************************************/

class WinEDA_GerberFrame : public WinEDA_BasePcbFrame
{
    friend class PCB_LAYER_WIDGET;

protected:
    GERBER_LAYER_WIDGET*  m_LayersManager;

public:
    WinEDALayerChoiceBox* m_SelLayerBox;
    DCODE_SELECTION_BOX*  m_DCodeSelector;  // a list box to select the dcode Id to hightlight.
    wxTextCtrl*           m_TextInfo;       // a wxTextCtrl used to display some info about
                                            // gerber data (format..)
    wxArrayString         m_DCodesList;     // an array string containint all decodes Id (10 to 999)

private:
    int m_displayMode;                  // Gerber images ("layers" in Gerbview) can be drawn:
                                        //  - in fast mode (write mode) but if there are negative items
                                        // only the last image is correctly drawn
                                        // (no problem to see only one image or when no negative items)
                                        //  - in "exact" mode (but slower) in write mode:
                                        //                last image covers previous images
                                        //  - in "exact" mode (also slower) in OR mode
                                        //                (transparency mode)
                                        // m_displayMode = 0, 1 or 2
    bool          m_show_layer_manager_tools;
    wxArrayString m_Messages;           // An array sting to store warning messages when reaging a gerber file

public: WinEDA_GerberFrame( wxWindow* father, const wxString& title,
                            const wxPoint& pos, const wxSize& size,
                            long style = KICAD_DEFAULT_DRAWFRAME_STYLE );

    ~WinEDA_GerberFrame();

    void Update_config();
    void OnCloseWindow( wxCloseEvent& Event );

    /**
     * Function ReportMessage
     * Add a message (a string) in message list
     * for instance when reading a Gerber file
     * @param aMessage = the straing to add in list
     */
    void ReportMessage( const wxString aMessage )
    {
        m_Messages.Add( aMessage );
    }


    /**
     * Function ClearMessageList
     * Clear the message list
     * Call it before reading a Gerber file
     */
    void ClearMessageList()
    {
        m_Messages.Clear();
    }


    /**
     * Function GetDisplayMode
     *  @return 0 for fast mode (not fully compatible with negative objects)
     *          1 for exact mode, write mode
     *          2 for exact mode, OR mode (transparency mode)
     */
    int GetDisplayMode() { return m_displayMode; }

    /**
     * Function SetDisplayMode
     *  @param aMode =  0 for fast mode
     *                  1 for exact mode, write mode
     *                  2 for exact mode, OR mode (transparency mode)
     */
    void SetDisplayMode( int aMode ) { m_displayMode = aMode; }

    /**
     * Function IsGridVisible() , virtual
     * @return true if the grid must be shown
     */
    virtual bool IsGridVisible();

    /**
     * Function SetGridVisibility() , virtual
     * It may be overloaded by derived classes
     * if you want to store/retrieve the grid visiblity in configuration.
     * @param aVisible = true if the grid must be shown
     */
    virtual void SetGridVisibility( bool aVisible );

    /**
     * Function GetGridColor() , virtual
     * @return the color of the grid
     */
    virtual int  GetGridColor();

    /**
     * Function SetGridColor() , virtual
     * @param aColor = the new color of the grid
     */
    virtual void SetGridColor( int aColor );

    /**
     * Function IsElementVisible
     * tests whether a given element category is visible. Keep this as an
     * inline function.
     * @param aGERBER_VISIBLE is from the enum by the same name
     * @return bool - true if the element is visible.
     * @see enum PCB_VISIBLE
     */
    bool IsElementVisible( int aGERBER_VISIBLE )
    {
        return GetBoard()->IsElementVisible( aGERBER_VISIBLE );
    }


    /**
     * Function SetElementVisibility
     * changes the visibility of an element category
     * @param aGERBER_VISIBLE is from the enum by the same name
     * @param aNewState = The new visibility state of the element category
     * @see enum PCB_VISIBLE
     */
    void SetElementVisibility( int aGERBER_VISIBLE, bool aNewState );

    /**
     * Function SetVisibleAlls
     * Set the status of all visible element categories and layers to VISIBLE
     */
    void SetVisibleAlls();

    /**
     * Function ReFillLayerWidget
     * changes out all the layers in m_Layers and may be called upon
     * loading a new BOARD.
     */
    void ReFillLayerWidget();

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
    void              syncLayerWidget();

    /**
     * Function syncLayerBox
     * updates the currently "selected" layer within m_SelLayerBox
     * The currently active layer, as defined by the return value of
     * getActiveLayer().  And updates the colored icon in the toolbar.
     */
    void              syncLayerBox();

    /**
     * Function UpdateTitleAndInfo
     * displays the short filename (if exists) of the selected layer
     * on the caption of the main gerbview window
     * and some other parameters
     *    Name of the layer (found in the gerber file: LN &ltname&gt command) in the status bar
     *    Name of the Image (found in the gerber file: IN &ltname&gt command) in the status bar
     *    and other data in toolbar
     */
    void              UpdateTitleAndInfo();

    /**
     * Load applications settings specific to the PCBNew.
     *
     * This overrides the base class WinEDA_BasePcbFrame::LoadSettings() to
     * handle settings specific common to the PCB layout application.  It
     * calls down to the base class to load settings common to all PCB type
     * drawing frames.  Please put your application settings for PCBNew here
     * to avoid having application settings loaded all over the place.
     */
    virtual void      LoadSettings();

    /**
     * Save applications settings common to PCB draw frame objects.
     *
     * This overrides the base class WinEDA_BasePcbFrame::SaveSettings() to
     * save settings specific to the PCB layout application main window.  It
     * calls down to the base class to save settings common to all PCB type
     * drawing frames.  Please put your application settings for PCBNew here
     * to avoid having application settings saved all over the place.
     */
    virtual void      SaveSettings();

    /**
     * Function SetLanguage
     * called on a language menu selection
     */
    virtual void      SetLanguage( wxCommandEvent& event );

    void              Process_Special_Functions( wxCommandEvent& event );
    void              RedrawActiveWindow( wxDC* DC, bool EraseBg );
    void              ReCreateHToolbar();
    void              ReCreateVToolbar();
    void              ReCreateOptToolbar();
    void              ReCreateMenuBar();
    void              OnLeftClick( wxDC* DC, const wxPoint& MousePos );
    void              OnLeftDClick( wxDC* DC, const wxPoint& MousePos );
    bool              OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu );
    int               BestZoom();
    void              OnSelectOptionToolbar( wxCommandEvent& event );

    /**
     * Function OnSelectDisplayMode
     * called on a display mode selection
     * Mode selection can be fast display,
     * or exact mode with stacked images or with transparency
     */
    void              OnSelectDisplayMode( wxCommandEvent& event );
    void              OnHotKey( wxDC* DC, int hotkey, EDA_ITEM* DrawStruct );

    GERBER_DRAW_ITEM* GerberGeneralLocateAndDisplay();
    GERBER_DRAW_ITEM* Locate( int typeloc );


    void              SetToolbars();
    void              Process_Settings( wxCommandEvent& event );
    void              Process_Config( wxCommandEvent& event );
    void              InstallConfigFrame( const wxPoint& pos );
    void              InstallGerberOptionsDialog( wxCommandEvent& event );
    void              InstallPcbGlobalDeleteFrame( const wxPoint& pos );

    /* handlers for block commands */
    virtual int       ReturnBlockCommand( int key );
    virtual void      HandleBlockPlace( wxDC* DC );
    virtual bool      HandleBlockEnd( wxDC* DC );

    /* Block operations: */

    /**
     * Function Block_Delete
     * deletes all tracks and segments within the selected block.
     * Defined separately in pcbnew and gerbview
     *
     * @param DC A device context to draw on.
     */
    void              Block_Delete( wxDC* DC );

    /**
     * Function Block_Move
     * moves all tracks and segments within the selected block.
     * New location is determined by the current offset from the selected
     * block's original location.
     * Defined separately in pcbnew and gerbview
     *
     * @param DC A device context to draw on.
     */
    void              Block_Move( wxDC* DC );

    /**
     * Function Block_Duplicate
     * copies-and-moves all tracks and segments within the selected block.
     * New location is determined by the current offset from the selected
     * block's original location.
     * Defined separately in pcbnew and gerbview
     *
     * @param DC A device context to draw on.
     */
    void              Block_Duplicate( wxDC* DC );

    void              ToPostProcess( wxCommandEvent& event );

    /**
     * Function ToPlotter
     * Open a dialog frame to create plot and drill files
     * relative to the current board
     */
    void              ToPlotter( wxCommandEvent& event );

    /**
     * Function ToPrinter
     * Open a dialog frame to print layers
     */
    void              ToPrinter( wxCommandEvent& event );

    void              Genere_HPGL( const wxString& FullFileName, int Layers );
    void              Genere_GERBER( const wxString& FullFileName, int Layers );
    void              Genere_PS( const wxString& FullFileName, int Layers );
    void              Plot_Layer_HPGL( FILE* File, int masque_layer,
                                       int garde, bool trace_via,
                                       GRTraceMode trace_mode );
    void              Plot_Layer_GERBER( FILE* File, int masque_layer,
                                         int garde, bool trace_via,
                                         GRTraceMode trace_mode );
    int               Gen_D_CODE_File( const wxString& Name_File );
    void              Plot_Layer_PS( FILE* File, int masque_layer,
                                     int garde, bool trace_via,
                                     GRTraceMode trace_mode );

    void              Files_io( wxCommandEvent& event );
    void              OnFileHistory( wxCommandEvent& event );

    /**
     * function LoadGerberFiles
     * Load a photoplot (Gerber) file or many files.
     * @param aFileName - void string or file name with full path to open or empty string to open a new
     *                    file. In this case one one file is loaded
     *                    if void string: user will be prompted for filename(s)
     * @return true if file was opened successfully.
     */
    bool              LoadGerberFiles( const wxString& aFileName );
    int               ReadGerberFile( FILE* File, bool Append );
    bool              Read_GERBER_File( const wxString& GERBER_FullFileName,
                                        const wxString& D_Code_FullFileName );

    void              GeneralControle( wxDC* aDC, const wxPoint& aPosition );


    /**
     * Function Read_D_Code_File
     * reads in a dcode file assuming ALSPCB file format with ';' indicating
     * comments.
     * <p>
     * Format is like CSV but with optional ';' delineated comments:<br>
     * tool,     Horiz,       Vert,   drill, vitesse, acc. ,Type ; [DCODE (commentaire)]<br>
     * ex:     1,         12,       12,     0,        0,     0,   3 ; D10
     * <p>
     * Format:<br>
     * Ver,  Hor, Type, Tool [,Drill]<br>
     * example: 0.012, 0.012,  L   , D10<br>
     *
     * Categorize all found dcodes into a table of D_CODE instantiations.
     * @param D_Code_FullFileName The name of the file to read from.
     * @return int - <br>
     *                 -1 = file not found<br>
     *                 -2 = parsing problem<br>
     *                  0 = the \a D_Code_FullFileName is empty, no reading
     *                      is done but an empty GERBER is put into
     *                      g_GERBER_List[]<br>
     *                  1 = read OK<br>
     */
    int               Read_D_Code_File( const wxString& D_Code_FullFileName );
    void              CopyDCodesSizeToItems();
    void              Liste_D_Codes();

    // PCB handling
    bool              Clear_Pcb( bool query );
    void              Erase_Current_Layer( bool query );
    void              Delete_DCode_Items( wxDC* DC, int dcode_value, int layer_number );

    // Conversion function
    void              ExportDataInPcbnewFormat( wxCommandEvent& event );

    /* SaveCopyInUndoList() virtual
     * currently: do nothing in gerbview.
     * but must be defined because it is a pure virtual in WinEDA_BasePcbFrame
     */
    virtual void SaveCopyInUndoList(
        BOARD_ITEM* aItemToCopy,
        UndoRedoOpType aTypeCommand = UR_UNSPECIFIED,
        const wxPoint& aTransformPoint = wxPoint( 0, 0 ) ) { }

    /**
     * Function SaveCopyInUndoList (overloaded).
     * Creates a new entry in undo list of commands.
     * add a list of pickers to handle a list of items
     * @param aItemsList = the list of items modified by the command to undo
     * @param aTypeCommand = command type (see enum UndoRedoOpType)
     * @param aTransformPoint = the reference point of the transformation,
     *                          for commands like move
     */
    virtual void SaveCopyInUndoList(
        PICKED_ITEMS_LIST& aItemsList,
        UndoRedoOpType aTypeCommand,
        const wxPoint& aTransformPoint = wxPoint( 0, 0 ) )
    {
        // currently: do nothing in gerbview.
    }


    /** Virtual function PrintPage
     * used to print a page
     * @param aDC = wxDC given by the calling print function
     * @param aPrintMasklayer = a 32 bits mask: bit n = 1 -> layer n is printed
     * @param aPrintMirrorMode = not used here (Set when printing in mirror mode)
     * @param aData = a pointer on an auxiliary data (not always used, NULL if not used)
     */
    virtual void PrintPage( wxDC* aDC,
                            int aPrintMasklayer, bool aPrintMirrorMode,
                            void* aData = NULL );

    /**
     * Function InstallDialogLayerPairChoice
     * Install a dialog frame to choose the equivalence
     * between gerber layers and pcbnew layers
     * @return the "lookup table" if ok, or NULL
     */
    int*         InstallDialogLayerPairChoice();

    /**
     * Function DrawItemsDCodeID
     * Draw the DCode value (if exists) corresponding to gerber item
     * (polygons do not have a DCode)
     * @param aDC = the current device contect
     * @param aDrawMode = GR_COPY, GR_OR ...
     */
    void         DrawItemsDCodeID( wxDC* aDC, int aDrawMode );

    DECLARE_EVENT_TABLE()
};

#endif  /* WX_GERBER_STRUCT_H */
