/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file gerbview_frame.h
 */

#ifndef  WX_GERBER_STRUCT_H
#define  WX_GERBER_STRUCT_H


#include "param_config.h"
#include "wxBasePcbFrame.h"

#include "../pcbnew/class_board.h"


class DCODE_SELECTION_BOX;
class GERBER_LAYER_WIDGET;
class LAYER_BOX_SELECTOR;
class GERBER_DRAW_ITEM;


#define NO_AVAILABLE_LAYERS -1


/**
 * Class GERBVIEW_FRAME
 * is the main window used in GerbView.
 */

class GERBVIEW_FRAME : public PCB_BASE_FRAME
{
    friend class PCB_LAYER_WIDGET;

protected:
    GERBER_LAYER_WIDGET*  m_LayersManager;

    // Auxiliary file history used to store drill files history.
    wxFileHistory         m_drillFileHistory;

public:
    LAYER_BOX_SELECTOR* m_SelLayerBox;
    DCODE_SELECTION_BOX*  m_DCodeSelector;  // a list box to select the dcode Id to highlight.
    wxTextCtrl*           m_TextInfo;       // a wxTextCtrl used to display some info about
                                            // gerber data (format..)
    wxArrayString         m_DCodesList;     // an array string containing all decodes Id (10 to 999)

private:
    // list of PARAM_CFG_xxx to read/write parameters saved in config
    PARAM_CFG_ARRAY       m_configSettings;

    int m_displayMode;                  // Gerber images ("layers" in Gerbview) can be drawn:
                                        //  - in fast mode (write mode) but if there are negative
                                        // items only the last image is correctly drawn (no
                                        // problem to see only one image or when no negative items)
                                        //  - in "exact" mode (but slower) in write mode:
                                        //                last image covers previous images
                                        //  - in "exact" mode (also slower) in OR mode
                                        //                (transparency mode)
                                        // m_displayMode = 0, 1 or 2

    bool          m_show_layer_manager_tools;

    // An array sting to store warning messages when reaging a gerber file.
    wxArrayString m_Messages;

public: GERBVIEW_FRAME( wxWindow* father, const wxString& title,
                        const wxPoint& pos, const wxSize& size,
                        long style = KICAD_DEFAULT_DRAWFRAME_STYLE );

    ~GERBVIEW_FRAME();

    void OnCloseWindow( wxCloseEvent& Event );

    // Virtual basic functions:
    void RedrawActiveWindow( wxDC* DC, bool EraseBg );
    void ReCreateHToolbar();

    /**
     * Function ReCreateVToolbar
     * creates or updates the right vertical toolbar.
     *
     * @note This is currently not used.
     */
    void ReCreateVToolbar();

    /**
     * Create or update the left vertical toolbar (option toolbar
     */
    void ReCreateOptToolbar();

    void ReCreateMenuBar();
    void OnLeftClick( wxDC* DC, const wxPoint& MousePos );
    void OnLeftDClick( wxDC* DC, const wxPoint& MousePos );
    bool OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu );
    double BestZoom();

    /**
     * Function ReportMessage
     * Add a message (a string) in message list
     * for instance when reading a Gerber file
     * @param aMessage = the string to add in list
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
     * if you want to store/retrieve the grid visibility in configuration.
     * @param aVisible = true if the grid must be shown
     */
    virtual void SetGridVisibility( bool aVisible );

    /**
     * Function GetGridColor() , virtual
     * @return the color of the grid
     */
    virtual int GetGridColor();

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
     * Function getNextAvailableLayer
     * finds the next empty layer starting at \a aLayer and returns it to the caller.  If no
     * empty layers are found, NO_AVAILABLE_LAYERS is return.
     * @param aLayer The first layer to search.
     * @return The first empty layer found or NO_AVAILABLE_LAYERS.
     */
    int getNextAvailableLayer( int aLayer = 0 ) const;

    bool hasAvailableLayers() const { return getNextAvailableLayer() != NO_AVAILABLE_LAYERS; }

    /**
     * Function syncLayerWidget
     * updates the currently "selected" layer within the PCB_LAYER_WIDGET.
     * The currently active layer is defined by the return value of getActiveLayer().
     * <p>
     * This function cannot be inline without including layer_widget.h in
     * here and we do not want to do that.
     */
    void syncLayerWidget();

    /**
     * Function syncLayerBox
     * updates the currently "selected" layer within m_SelLayerBox
     * The currently active layer, as defined by the return value of
     * getActiveLayer().  And updates the colored icon in the toolbar.
     */
    void syncLayerBox();

    /**
     * Function UpdateTitleAndInfo
     * displays the short filename (if exists) of the selected layer
     * on the caption of the main GerbView window
     * and some other parameters
     *    Name of the layer (found in the gerber file: LN &ltname&gt command) in the status bar
     *    Name of the Image (found in the gerber file: IN &ltname&gt command) in the status bar
     *    and other data in toolbar
     */
    void UpdateTitleAndInfo();

    /**
     * Function GetConfigurationSettings
     * Populates the GerbView applications settings list.
     * (list of parameters that must be saved in GerbView parameters)
     * Currently, only the settings that are needed at start up by the main window are
     * defined here.  There are other locally used settings scattered throughout the
     * GerbView source code (mainly in dialogs).  If you need to define a configuration
     * setting that need to be loaded at run time, this is the place to define it.
     */
    PARAM_CFG_ARRAY& GetConfigurationSettings( void );

    /**
     * Load applications settings specific to the Pcbnew.
     *
     * This overrides the base class PCB_BASE_FRAME::LoadSettings() to
     * handle settings specific common to the PCB layout application.  It
     * calls down to the base class to load settings common to all PCB type
     * drawing frames.  Please put your application settings for Pcbnew here
     * to avoid having application settings loaded all over the place.
     */
    virtual void LoadSettings();

    /**
     * Save applications settings common to PCB draw frame objects.
     *
     * This overrides the base class PCB_BASE_FRAME::SaveSettings() to
     * save settings specific to the PCB layout application main window.  It
     * calls down to the base class to save settings common to all PCB type
     * drawing frames.  Please put your application settings for Pcbnew here
     * to avoid having application settings saved all over the place.
     */
    virtual void SaveSettings();

    /**
     * Function SetLanguage
     * called on a language menu selection
     */
    virtual void SetLanguage( wxCommandEvent& event );

    void Process_Special_Functions( wxCommandEvent& event );
    void OnSelectOptionToolbar( wxCommandEvent& event );

    /**
     * Function OnSelectActiveDCode
     * Selects the active DCode for the current active layer.
     * Items using this DCode are highlighted.
     */
    void OnSelectActiveDCode( wxCommandEvent& event );

    /**
     * Function OnSelectActiveLayer
     * Selects the active layer:
     *  - if a file is loaded, it is loaded in this layer
     *  _ this layer is displayed on top of other layers
     */
    void OnSelectActiveLayer( wxCommandEvent& event );

    /**
     * Function OnShowGerberSourceFile
     * Call the preferred editor to show (and edit) the gerber source file
     * loaded in the active layer
     */
    void OnShowGerberSourceFile( wxCommandEvent& event );

    /**
     * Function OnSelectDisplayMode
     * called on a display mode selection
     * Mode selection can be fast display,
     * or exact mode with stacked images or with transparency
     */
    void OnSelectDisplayMode( wxCommandEvent& event );

    /**
     * Function OnQuit
     * called on request of application quit
     */
    void OnQuit( wxCommandEvent& event );

    /**
     * Function OnHotKey
     * called when on hotkey trigger
     */
    void OnHotKey( wxDC* DC, int hotkey, EDA_ITEM* DrawStruct );

    GERBER_DRAW_ITEM* GerberGeneralLocateAndDisplay();
    GERBER_DRAW_ITEM* Locate( const wxPoint& aPosition, int typeloc );

    void Process_Settings( wxCommandEvent& event );
    void Process_Config( wxCommandEvent& event );
    void InstallGerberOptionsDialog( wxCommandEvent& event );

    void OnUpdateDrawMode( wxUpdateUIEvent& aEvent );
    void OnUpdateFlashedItemsDrawMode( wxUpdateUIEvent& aEvent );
    void OnUpdateLinesDrawMode( wxUpdateUIEvent& aEvent );
    void OnUpdatePolygonsDrawMode( wxUpdateUIEvent& aEvent );
    void OnUpdateShowDCodes( wxUpdateUIEvent& aEvent );
    void OnUpdateShowLayerManager( wxUpdateUIEvent& aEvent );
    void OnUpdateSelectDCode( wxUpdateUIEvent& aEvent );
    void OnUpdateLayerSelectBox( wxUpdateUIEvent& aEvent );

    /**
     * Function ReturnBlockCommand
     * returns the block command (BLOCK_MOVE, BLOCK_COPY...) corresponding to
     * the \a aKey (ALT, SHIFT ALT ..)
     */
    virtual int ReturnBlockCommand( int key );

    /**
     * Function HandleBlockPlace
     * handles the block place command.
     */
    virtual void HandleBlockPlace( wxDC* DC );

    /**
     * Function HandleBlockEnd( )
     * handles the end of a block command,
     * It is called at the end of the definition of the area of a block.
     * Depending on the current block command, this command is executed
     * or parameters are initialized to prepare a call to HandleBlockPlace
     * in GetScreen()->m_BlockLocate
     *
     * @return false if no item selected, or command finished,
     *         true if some items found and HandleBlockPlace must be called later.
     */
    virtual bool HandleBlockEnd( wxDC* DC );

    /**
     * Function Block_Delete
     * deletes all tracks and segments within the selected block.
     * Defined separately in Pcbnew and GerbView
     *
     * @param DC A device context to draw on.
     */
    void Block_Delete( wxDC* DC );

    /**
     * Function Block_Move
     * moves all tracks and segments within the selected block.
     * New location is determined by the current offset from the selected
     * block's original location.
     * Defined separately in Pcbnew and GerbView
     *
     * @param DC A device context to draw on.
     */
    void Block_Move( wxDC* DC );

    /**
     * Function Block_Duplicate
     * copies-and-moves all tracks and segments within the selected block.
     * New location is determined by the current offset from the selected
     * block's original location.
     * Defined separately in Pcbnew and GerbView
     *
     * @param DC A device context to draw on.
     */
    void Block_Duplicate( wxDC* DC );

    void ToPostProcess( wxCommandEvent& event );

    /**
     * Function ToPlotter
     * Open a dialog frame to create plot and drill files
     * relative to the current board
     */
    void ToPlotter( wxCommandEvent& event );

    /**
     * Function ToPrinter
     * Open a dialog frame to print layers
     */
    void ToPrinter( wxCommandEvent& event );

    void Genere_HPGL( const wxString& FullFileName, int Layers );
    void Genere_GERBER( const wxString& FullFileName, int Layers );
    void Genere_PS( const wxString& FullFileName, int Layers );
    void Plot_Layer_HPGL( FILE* File, int masque_layer,int garde, bool trace_via,
                          GRTraceMode trace_mode );
    void Plot_Layer_GERBER( FILE* File, int masque_layer, int garde, bool trace_via,
                            GRTraceMode trace_mode );
    int Gen_D_CODE_File( const wxString& Name_File );
    void Plot_Layer_PS( FILE* File, int masque_layer, int garde, bool trace_via,
                        GRTraceMode trace_mode );

    void Files_io( wxCommandEvent& event );

    /**
     * Function OnGbrFileHistory
     * deletes the current data and loads a Gerber file selected from history list on
     * current layer.
     */
    void OnGbrFileHistory( wxCommandEvent& event );

    /**
     * Function OnDrlFileHistory
     * deletes the current data and load a drill file in Excellon format selected from
     * history list on current layer.
     */
    void OnDrlFileHistory( wxCommandEvent& event );

    /**
     * function LoadGerberFiles
     * Load a photoplot (Gerber) file or many files.
     * @param aFileName - void string or file name with full path to open or empty string to
     *                    open a new file. In this case one one file is loaded
     *                    if void string: user will be prompted for filename(s)
     * @return true if file was opened successfully.
     */
    bool LoadGerberFiles( const wxString& aFileName );
    int ReadGerberFile( FILE* File, bool Append );
    bool Read_GERBER_File( const wxString& GERBER_FullFileName,
                           const wxString& D_Code_FullFileName );

    /**
     * function LoadDrllFiles
     * Load a drill (EXCELLON) file or many files.
     * @param aFileName - void string or file name with full path to open or empty string to
     *                    open a new file. In this case one one file is loaded
     *                    if void string: user will be prompted for filename(s)
     * @return true if file was opened successfully.
     */
    bool LoadExcellonFiles( const wxString& aFileName );
    bool Read_EXCELLON_File( const wxString& aFullFileName );

    void GeneralControl( wxDC* aDC, const wxPoint& aPosition, int aHotKey = 0 );

    /**
     * Read a DCode file (not used with RX274X files , just with RS274D old files).
     * Note: there is no standard for DCode file.
     * Just read a file format created by early versions of Pcbnew.
     * @return false if file not read (cancellation)
     *          true if OK
     * @ aparm aFullFileName = name of file to load.
     *  if empty, or if the file does not exist, a file dialog is opened
     */
    bool LoadDCodeFile( const wxString& aFullFileName );

    /**
     * Function ReadDCodeDefinitionFile
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
     * Load all found dcodes into a table of D_CODE instantiations.
     * @param D_Code_FullFileName The name of the file to read from.
     * @return int - <br>
     *                 -1 = file not found<br>
     *                 -2 = parsing problem<br>
     *                  0 = the \a D_Code_FullFileName is empty, no reading
     *                      is done but an empty GERBER is put into
     *                      g_GERBER_List[]<br>
     *                  1 = read OK<br>
     */
    int ReadDCodeDefinitionFile( const wxString& D_Code_FullFileName );

    /**
     * Set Size Items (Lines, Flashes) from DCodes List
     */
    void CopyDCodesSizeToItems();
    void Liste_D_Codes();

    // PCB handling
    bool Clear_Pcb( bool query );
    void Erase_Current_Layer( bool query );

    // Conversion function
    void ExportDataInPcbnewFormat( wxCommandEvent& event );

    /* SaveCopyInUndoList() virtual
     * currently: do nothing in GerbView.
     * but must be defined because it is a pure virtual in PCB_BASE_FRAME
     */
    virtual void SaveCopyInUndoList( BOARD_ITEM* aItemToCopy,
                                     UNDO_REDO_T aTypeCommand = UR_UNSPECIFIED,
                                     const wxPoint& aTransformPoint = wxPoint( 0, 0 ) ) { }

    /**
     * Function SaveCopyInUndoList (overloaded).
     * Creates a new entry in undo list of commands.
     * add a list of pickers to handle a list of items
     * @param aItemsList = the list of items modified by the command to undo
     * @param aTypeCommand = command type (see enum UNDO_REDO_T)
     * @param aTransformPoint = the reference point of the transformation,
     *                          for commands like move
     */
    virtual void SaveCopyInUndoList( PICKED_ITEMS_LIST& aItemsList,
                                     UNDO_REDO_T aTypeCommand,
                                     const wxPoint& aTransformPoint = wxPoint( 0, 0 ) )
    {
        // currently: do nothing in GerbView.
    }

    /** Virtual function PrintPage
     * used to print a page
     * @param aDC = wxDC given by the calling print function
     * @param aPrintMasklayer = a 32 bits mask: bit n = 1 -> layer n is printed
     * @param aPrintMirrorMode = not used here (Set when printing in mirror mode)
     * @param aData = a pointer on an auxiliary data (not always used, NULL if not used)
     */
    virtual void PrintPage( wxDC* aDC, int aPrintMasklayer, bool aPrintMirrorMode,
                            void* aData = NULL );

    /**
     * Function DrawItemsDCodeID
     * Draw the DCode value (if exists) corresponding to gerber item
     * (polygons do not have a DCode)
     * @param aDC = the current device context
     * @param aDrawMode = GR_COPY, GR_OR ...
     */
    void DrawItemsDCodeID( wxDC* aDC, int aDrawMode );

    DECLARE_EVENT_TABLE()
};

#endif  /* WX_GERBER_STRUCT_H */
