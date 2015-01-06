/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2013 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <config_params.h>
#include <draw_frame.h>

#include <gerbview.h>
#include <class_gbr_layout.h>
#include <class_gbr_screen.h>
#include <class_page_info.h>

#define NO_AVAILABLE_LAYERS UNDEFINED_LAYER

class DCODE_SELECTION_BOX;
class GERBER_LAYER_WIDGET;
class GBR_LAYER_BOX_SELECTOR;
class GERBER_DRAW_ITEM;


/**
 * Class GBR_DISPLAY_OPTIONS
 * A helper class to handle display options.
 */
class GBR_DISPLAY_OPTIONS
{
public:
    bool    m_DisplayFlashedItemsFill;
    bool    m_DisplayLinesFill;
    bool    m_DisplayPolygonsFill;
    bool    m_DisplayPolarCood;
    bool    m_DisplayDCodes;
    bool    m_DisplayNegativeObjects;
    bool    m_IsPrinting;

public:
    GBR_DISPLAY_OPTIONS()
    {
        m_DisplayFlashedItemsFill = true;
        m_DisplayLinesFill      = true;
        m_DisplayPolygonsFill   = true;
        m_DisplayPolarCood      = false;
        m_DisplayDCodes = true;
        m_IsPrinting = false;
    }
};


/**
 * Class GERBVIEW_FRAME
 * is the main window used in GerbView.
 */

class GERBVIEW_FRAME : public EDA_DRAW_FRAME    // PCB_BASE_FRAME
{
    GBR_LAYOUT*     m_gerberLayout;
    wxPoint         m_grid_origin;
    PAGE_INFO       m_paper;            // used only to show paper limits to screen

public:
    GBR_DISPLAY_OPTIONS m_DisplayOptions;

    /**
     * Function SetLayout
     * sets the m_gerberLayout member in such as way as to ensure deleting any previous
     * GBR_LAYOUT.
     * @param aLayout The GBR_LAYOUT to put into the frame.
     */
    void SetLayout( GBR_LAYOUT* aLayout )
    {
        delete m_gerberLayout;
        m_gerberLayout = aLayout;
    }

    GBR_LAYOUT* GetGerberLayout() const
    {
        wxASSERT( m_gerberLayout );
        return m_gerberLayout;
    }

    /**
     * Function GetItemsList
     * @return the first GERBER_DRAW_ITEM * item of the items list
     */
    GERBER_DRAW_ITEM* GetItemsList()
    {
        GERBER_DRAW_ITEM* item = GetGerberLayout()->m_Drawings;

        return (GERBER_DRAW_ITEM*) item;
    }

    /**
     * Function GetGerberLayoutBoundingBox
     * calculates the bounding box containing all gerber items.
     * @return EDA_RECT - the items bounding box
     */
    EDA_RECT            GetGerberLayoutBoundingBox();

    void                SetPageSettings( const PAGE_INFO& aPageSettings );  // overload
    const PAGE_INFO&    GetPageSettings() const;                            // overload
    const wxSize        GetPageSizeIU() const;                              // overload

    const wxPoint&      GetAuxOrigin() const;                               // overload
    void                SetAuxOrigin( const wxPoint& aPoint );              // overload

    const wxPoint&      GetGridOrigin() const  { return m_grid_origin; }    // overload
    void                SetGridOrigin( const wxPoint& aPoint )              // overload
    {
        m_grid_origin = aPoint;
    }

    const TITLE_BLOCK&  GetTitleBlock() const;                              // overload
    void                SetTitleBlock( const TITLE_BLOCK& aTitleBlock );    // overload

    /**
     * Function SetCurItem
     * sets the currently selected item and displays it in the MsgPanel.
     * If the given item is NULL then the MsgPanel is erased and there is no
     * currently selected item. This function is intended to make the process
     * of "selecting" an item more formal, and to indivisibly tie the operation
     * of selecting an item to displaying it using GERBER_DRAW_ITEM::Display_Infos().
     * @param aItem The GERBER_DRAW_ITEM to make the selected item or NULL if none.
     * @param aDisplayInfo = true to display item info, false if not (default = true)
     */
    void                SetCurItem( GERBER_DRAW_ITEM* aItem, bool aDisplayInfo = true );

    /** Install the dialog box for layer selection
     * @param aDefaultLayer = Preselection (NB_PCB_LAYERS for "(Deselect)" layer)
     * @param aCopperLayerCount = number of copper layers
     * @param aShowDeselectOption = display a "(Deselect)" radiobutton (when set to true)
     * @return new layer value (NB_PCB_LAYERS when "(Deselect)" radiobutton selected),
     *                         or -1 if canceled
     *
     * Providing the option to also display a "(Deselect)" radiobutton makes the
     *  GerbView's "Export to Pcbnew" command) more "user friendly",
     * by permitting any layer to be "deselected" immediately after its
     * corresponding radiobutton has been clicked on. (It would otherwise be
     * necessary to first cancel the "Select Layer:" dialog box (invoked after a
     * different radiobutton is clicked on) prior to then clicking on the "Deselect"
     * button provided within the "Layer selection:" dialog box).
     */
    int SelectPCBLayer( int aDefaultLayer, int aOpperLayerCount, bool aNullLayer = false );

protected:
    GERBER_LAYER_WIDGET*    m_LayersManager;

    // Auxiliary file history used to store drill files history.
    wxFileHistory           m_drillFileHistory;
    /// The last filename chosen to be proposed to the user
    wxString                m_lastFileName;
public:
    GBR_LAYER_BOX_SELECTOR* m_SelLayerBox;
    DCODE_SELECTION_BOX*    m_DCodeSelector;    // a list box to select the dcode Id to highlight.
    wxTextCtrl*             m_TextInfo;         // a wxTextCtrl used to display some info about
                                                // gerber data (format..)
    wxArrayString           m_DCodesList;       // an array string containing all decodes Id (10 to 999)
private:
    // list of PARAM_CFG_xxx to read/write parameters saved in config
    PARAM_CFG_ARRAY         m_configSettings;
    COLORS_DESIGN_SETTINGS* m_colorsSettings;

    int m_displayMode;                  // Gerber images ("layers" in Gerbview) can be drawn:
                                        // - in fast mode (write mode) but if there are negative
                                        // items only the last image is correctly drawn (no
                                        // problem to see only one image or when no negative items)
                                        // - in "exact" mode (but slower) in write mode:
                                        // last image covers previous images
                                        // - in "exact" mode (also slower) in OR mode
                                        // (transparency mode)
                                        // m_displayMode = 0, 1 or 2

    bool            m_show_layer_manager_tools;

    // An array sting to store warning messages when reaging a gerber file.
    wxArrayString   m_Messages;

public:
    GERBVIEW_FRAME( KIWAY* aKiway, wxWindow* aParent );
    ~GERBVIEW_FRAME();

    void    OnCloseWindow( wxCloseEvent& Event );

    bool    OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl );   // overload KIWAY_PLAYER

    // Virtual basic functions:
    void    RedrawActiveWindow( wxDC* DC, bool EraseBg );
    void    ReCreateHToolbar();

    /**
     * Function ReCreateVToolbar
     * creates or updates the right vertical toolbar.
     *
     * @note This is currently not used.
     */
    void    ReCreateVToolbar();

    /**
     * Create or update the left vertical toolbar (option toolbar
     */
    void    ReCreateOptToolbar();

    void    ReCreateMenuBar();
    void    OnLeftClick( wxDC* DC, const wxPoint& MousePos );
    void    OnLeftDClick( wxDC* DC, const wxPoint& MousePos );
    bool    OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu );
    double  BestZoom();
    void    UpdateStatusBar();

    /**
     * Function GetZoomLevelIndicator
     * returns a human readable value which can be displayed as zoom
     * level indicator in dialogs.
     * Virtual from the base class
     */
    const wxString GetZoomLevelIndicator() const;

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
     * Function IsElementVisible
     * tests whether a given element category is visible. Keep this as an
     * inline function.
     * @param aItemIdVisible is an item id from the enum GERBER_VISIBLE_ID
     * @return bool - true if the element is visible.
     */
    bool    IsElementVisible( GERBER_VISIBLE_ID aItemIdVisible ) const;

    /**
     * Function SetElementVisibility
     * changes the visibility of an element category
     * @param aItemIdVisible is an item id from the enum GERBER_VISIBLE_ID
     * @param aNewState = The new visibility state of the element category
     *  (see enum PCB_VISIBLE)
     */
    void    SetElementVisibility( GERBER_VISIBLE_ID aItemIdVisible, bool aNewState );

    /**
     * Function SetVisibleAlls
     * Set the status of all visible element categories and layers to VISIBLE
     */
    void    SetVisibleAlls();

    /**
     * Function SetGridVisibility(), virtual from EDA_DRAW_FRAME
     * It may be overloaded by derived classes
     * @param aVisible = true if the grid must be shown
     */
    void    SetGridVisibility( bool aVisible );

    /**
     * Function GetVisibleLayers
     * is a proxy function that calls the correspondent function in m_BoardSettings
     * Returns a bit-mask of all the layers that are visible
     * @return long - the visible layers in bit-mapped form.
     */
    long GetVisibleLayers() const;

    /**
     * Function SetVisibleLayers
     * is a proxy function that calls the correspondent function in m_BoardSettings
     * changes the bit-mask of visible layers
     * @param aLayerMask = The new bit-mask of visible layers
     */
    void    SetVisibleLayers( long aLayerMask );

    /**
     * Function IsLayerVisible
     * tests whether a given layer is visible
     * @param aLayer = The layer to be tested
     * @return bool - true if the layer is visible.
     */
    bool    IsLayerVisible( int aLayer ) const;

    /**
     * Function GetVisibleElementColor
     * returns the color of a gerber visible element.
     */
    EDA_COLOR_T GetVisibleElementColor( GERBER_VISIBLE_ID aItemIdVisible ) const;

    void    SetVisibleElementColor( GERBER_VISIBLE_ID aItemIdVisible, EDA_COLOR_T aColor );

    /**
     * Function GetLayerColor
     * gets a layer color for any valid layer.
     */
    EDA_COLOR_T GetLayerColor( int aLayer ) const;

    /**
     * Function SetLayerColor
     * changes a layer color for any valid layer.
     */
    void    SetLayerColor( int aLayer, EDA_COLOR_T aColor );

    /**
     * Function GetNegativeItemsColor
     * @return the color of negative items.
     * This is usually the background color, but can be an other color
     * in order to see negative objects
     */
    EDA_COLOR_T GetNegativeItemsColor() const;

    /**
     * Function DisplayLinesSolidMode
     * @return true to draw gerber lines in solid (filled) mode,
     * false to draw gerber lines in sketch mode
     */
    bool DisplayLinesSolidMode()
    {
        return  m_DisplayOptions.m_DisplayLinesFill;
    }

    /**
     * Function DisplayPolygonsSolidMode
     * @return true to draw polygon in solid (filled) mode,
     * false to draw polygon outlines only
     */
    bool DisplayPolygonsSolidMode()
    {
        return  m_DisplayOptions.m_DisplayPolygonsFill;
    }

    /**
     * Function DisplayFlashedItemsSolidMode
     * @return true to draw flashed items in solid (filled) mode,
     * false to draw draw flashed in sketch mode
     */
    bool DisplayFlashedItemsSolidMode()
    {
        return  m_DisplayOptions.m_DisplayFlashedItemsFill;
    }

    /**
     * Function ReFillLayerWidget
     * changes out all the layers in m_Layers and may be called upon
     * loading new gerber files.
     */
    void    ReFillLayerWidget();

    /**
     * Function setActiveLayer
     * will change the currently active layer to \a aLayer and also
     * update the GERBER_LAYER_WIDGET.
     */
    void    setActiveLayer( int aLayer, bool doLayerWidgetUpdate = true );

    /**
     * Function getActiveLayer
     * returns the active layer
     */
    int getActiveLayer();

    /**
     * Function getNextAvailableLayer
     * finds the next empty layer starting at \a aLayer and returns it to the caller.  If no
     * empty layers are found, NO_AVAILABLE_LAYERS is return.
     * @param aLayer The first layer to search.
     * @return The first empty layer found or NO_AVAILABLE_LAYERS.
     */
    int getNextAvailableLayer( int aLayer = 0 ) const;

    bool hasAvailableLayers() const
    {
        return getNextAvailableLayer() != NO_AVAILABLE_LAYERS;
    }

    /**
     * Function syncLayerWidget
     * updates the currently "selected" layer within the GERBER_LAYER_WIDGET.
     * The currently active layer is defined by the return value of getActiveLayer().
     * <p>
     * This function cannot be inline without including layer_widget.h in
     * here and we do not want to do that.
     */
    void                syncLayerWidget();

    /**
     * Function syncLayerBox
     * updates the currently "selected" layer within m_SelLayerBox
     * The currently active layer, as defined by the return value of
     * getActiveLayer().  And updates the colored icon in the toolbar.
     */
    void                syncLayerBox();

    /**
     * Function UpdateTitleAndInfo
     * displays the short filename (if exists) of the selected layer
     * on the caption of the main GerbView window
     * and some other parameters
     *    Name of the layer (found in the gerber file: LN &ltname&gt command) in the status bar
     *    Name of the Image (found in the gerber file: IN &ltname&gt command) in the status bar
     *    and other data in toolbar
     */
    void                UpdateTitleAndInfo();

    /**
     * Function GetConfigurationSettings
     * Populates the GerbView applications settings list.
     * (list of parameters that must be saved in GerbView parameters)
     * Currently, only the settings that are needed at start up by the main window are
     * defined here.  There are other locally used settings scattered throughout the
     * GerbView source code (mainly in dialogs).  If you need to define a configuration
     * setting that need to be loaded at run time, this is the place to define it.
     */
    PARAM_CFG_ARRAY&    GetConfigurationSettings( void );

    void LoadSettings( wxConfigBase* aCfg );    // override virtual

    void SaveSettings( wxConfigBase* aCfg );    // override virtual

    void                ShowChangedLanguage();  // override EDA_BASE_FRAME virtual

    void                Process_Special_Functions( wxCommandEvent& event );
    void                OnSelectOptionToolbar( wxCommandEvent& event );

    /**
     * Function OnSelectActiveDCode
     * Selects the active DCode for the current active layer.
     * Items using this DCode are highlighted.
     */
    void                OnSelectActiveDCode( wxCommandEvent& event );

    /**
     * Function OnSelectActiveLayer
     * Selects the active layer:
     *  - if a file is loaded, it is loaded in this layer
     *  _ this layer is displayed on top of other layers
     */
    void                OnSelectActiveLayer( wxCommandEvent& event );

    /**
     * Function OnShowGerberSourceFile
     * Call the preferred editor to show (and edit) the gerber source file
     * loaded in the active layer
     */
    void                OnShowGerberSourceFile( wxCommandEvent& event );

    /**
     * Function OnSelectDisplayMode
     * called on a display mode selection
     * Mode selection can be fast display,
     * or exact mode with stacked images or with transparency
     */
    void                OnSelectDisplayMode( wxCommandEvent& event );

    /**
     * Function OnQuit
     * called on request of application quit
     */
    void                OnQuit( wxCommandEvent& event );

    /**
     * Function OnHotKey.
     *  ** Commands are case insensitive **
     *  Some commands are relatives to the item under the mouse cursor
     * @param aDC = current device context
     * @param aHotkeyCode = hotkey code (ascii or wxWidget code for special keys)
     * @param aPosition The cursor position in logical (drawing) units.
     * @param aItem = NULL or pointer on a EDA_ITEM under the mouse cursor
     */
    bool OnHotKey( wxDC* aDC, int aHotkeyCode, const wxPoint& aPosition, EDA_ITEM* aItem = NULL );

    GERBER_DRAW_ITEM*   GerberGeneralLocateAndDisplay();
    GERBER_DRAW_ITEM*   Locate( const wxPoint& aPosition, int typeloc );

    void                Process_Settings( wxCommandEvent& event );
    void                Process_Config( wxCommandEvent& event );
    void                InstallGerberOptionsDialog( wxCommandEvent& event );

    void                OnUpdateDrawMode( wxUpdateUIEvent& aEvent );
    void                OnUpdateCoordType( wxUpdateUIEvent& aEvent );
    void                OnUpdateFlashedItemsDrawMode( wxUpdateUIEvent& aEvent );
    void                OnUpdateLinesDrawMode( wxUpdateUIEvent& aEvent );
    void                OnUpdatePolygonsDrawMode( wxUpdateUIEvent& aEvent );
    void                OnUpdateShowDCodes( wxUpdateUIEvent& aEvent );
    void                OnUpdateShowNegativeItems( wxUpdateUIEvent& aEvent );
    void                OnUpdateShowLayerManager( wxUpdateUIEvent& aEvent );
    void                OnUpdateSelectDCode( wxUpdateUIEvent& aEvent );
    void                OnUpdateLayerSelectBox( wxUpdateUIEvent& aEvent );

    /**
     * Function BlockCommand
     * returns the block command (BLOCK_MOVE, BLOCK_COPY...) corresponding to
     * the \a aKey (ALT, SHIFT ALT ..)
     */
    virtual int         BlockCommand( int key );

    /**
     * Function HandleBlockPlace
     * handles the block place command.
     */
    virtual void        HandleBlockPlace( wxDC* DC );

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
    virtual bool        HandleBlockEnd( wxDC* DC );

    /**
     * Function Block_Move
     * moves all tracks and segments within the selected block.
     * New location is determined by the current offset from the selected
     * block's original location.
     * Defined separately in Pcbnew and GerbView
     *
     * @param DC A device context to draw on.
     */
    void                Block_Move( wxDC* DC );

    /**
     * Function ToPlotter
     * Open a dialog frame to create plot and drill files
     * relative to the current board
     */
    void                ToPlotter( wxCommandEvent& event );

    /**
     * Function ToPrinter
     * Open a dialog frame to print layers
     */
    void                ToPrinter( wxCommandEvent& event );

    void                Files_io( wxCommandEvent& event );

    /**
     * Function OnGbrFileHistory
     * deletes the current data and loads a Gerber file selected from history list on
     * current layer.
     */
    void                OnGbrFileHistory( wxCommandEvent& event );

    /**
     * Function OnDrlFileHistory
     * deletes the current data and load a drill file in Excellon format selected from
     * history list on current layer.
     */
    void                OnDrlFileHistory( wxCommandEvent& event );

    /**
     * function LoadGerberFiles
     * Load a photoplot (Gerber) file or many files.
     * @param aFileName - void string or file name with full path to open or empty string to
     *                    open a new file. In this case one one file is loaded
     *                    if void string: user will be prompted for filename(s)
     * @return true if file was opened successfully.
     */
    bool                LoadGerberFiles( const wxString& aFileName );
    int                 ReadGerberFile( FILE* File, bool Append );
    bool                Read_GERBER_File( const wxString&   GERBER_FullFileName,
                                          const wxString&   D_Code_FullFileName );

    /**
     * function LoadDrllFiles
     * Load a drill (EXCELLON) file or many files.
     * @param aFileName - void string or file name with full path to open or empty string to
     *                    open a new file. In this case one one file is loaded
     *                    if void string: user will be prompted for filename(s)
     * @return true if file was opened successfully.
     */
    bool                LoadExcellonFiles( const wxString& aFileName );
    bool                Read_EXCELLON_File( const wxString& aFullFileName );

    bool                GeneralControl( wxDC* aDC, const wxPoint& aPosition, int aHotKey = 0 );

    /**
     * Set Size Items (Lines, Flashes) from DCodes List
     */
    void                CopyDCodesSizeToItems();
    void                Liste_D_Codes();

    // PCB handling
    bool                Clear_DrawLayers( bool query );
    void                Erase_Current_DrawLayer( bool query );

    // Conversion function
    void                ExportDataInPcbnewFormat( wxCommandEvent& event );

    /* SaveCopyInUndoList() virtual
     * currently: do nothing in GerbView.
     */
    void SaveCopyInUndoList( GERBER_DRAW_ITEM* aItemToCopy,
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
    void SaveCopyInUndoList( const PICKED_ITEMS_LIST& aItemsList,
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
    virtual void    PrintPage( wxDC* aDC, LSET aPrintMasklayer, bool aPrintMirrorMode,
                               void* aData = NULL );

    /**
     * Function DrawItemsDCodeID
     * Draw the DCode value (if exists) corresponding to gerber item
     * (polygons do not have a DCode)
     * @param aDC = the current device context
     * @param aDrawMode = GR_COPY, GR_OR ...
     */
    void            DrawItemsDCodeID( wxDC* aDC, GR_DRAWMODE aDrawMode );

    DECLARE_EVENT_TABLE()
};

#endif /* WX_GERBER_STRUCT_H */
