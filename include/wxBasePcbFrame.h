/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
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
 * @file wxBasePcbFrame.h
 * @brief Classes used in Pcbnew, CvPcb and GerbView.
 */

#ifndef  WX_BASE_PCB_FRAME_H_
#define  WX_BASE_PCB_FRAME_H_


#include <vector>

#include <wxstruct.h>
#include <base_struct.h>
#include <eda_text.h>                // EDA_DRAW_MODE_T
#include <richio.h>
#include <class_pcb_screen.h>


/* Forward declarations of classes. */
class FOOTPRINT_EDIT_FRAME;
class FOOTPRINT_VIEWER_FRAME;
class BOARD;
class BOARD_CONNECTED_ITEM;
class MODULE;
class TRACK;
class D_PAD;
class TEXTE_MODULE;
class EDA_3D_FRAME;
class GENERAL_COLLECTOR;
class GENERAL_COLLECTORS_GUIDE;
class BOARD_DESIGN_SETTINGS;
class ZONE_SETTINGS;
class PCB_PLOT_PARAMS;


/**
 * class PCB_BASE_FRAME
 * basic PCB main window class for Pcbnew, Gerbview, and CvPcb footprint viewer.
 */
class PCB_BASE_FRAME : public EDA_DRAW_FRAME
{
public:
    bool m_DisplayPadFill;          // How show pads
    bool m_DisplayViaFill;          // How show vias
    bool m_DisplayPadNum;           // show pads numbers

    int  m_DisplayModEdge;          // How to display module drawings (line/ filled / sketch)
    int  m_DisplayModText;          // How to display module texts (line/ filled / sketch)
    bool m_DisplayPcbTrackFill;     // false : tracks are show in sketch mode, true = filled.
    EDA_UNITS_T m_UserGridUnit;
    wxRealPoint m_UserGridSize;

    int m_FastGrid1;
    int m_FastGrid2;

    EDA_3D_FRAME* m_Draw3DFrame;
    FOOTPRINT_EDIT_FRAME* m_ModuleEditFrame;
    FOOTPRINT_VIEWER_FRAME * m_ModuleViewerFrame;


protected:
    BOARD*              m_Pcb;
    GENERAL_COLLECTOR*  m_Collector;

    /// Auxiliary tool bar typically shown below the main tool bar at the top of the
    /// main window.
    wxAuiToolBar* m_auxiliaryToolBar;

    /// True prints or plots the drawing border and title block.
    bool m_printBorderAndTitleBlock;

    void updateGridSelectBox();
    void updateZoomSelectBox();
    virtual void unitsChangeRefresh();

public:
    PCB_BASE_FRAME( wxWindow* father, int idtype, const wxString& title,
                    const wxPoint& pos, const wxSize& size,
                    long style = KICAD_DEFAULT_DRAWFRAME_STYLE );

    ~PCB_BASE_FRAME();

    bool GetPrintBorderAndTitleBlock() const { return m_printBorderAndTitleBlock; }

    /**
     * Function GetBoardBoundingBox
     * calculates the bounding box containing all board items (or board edge segments).
     * @param aBoardEdgesOnly is true if we are interested in board edge segments only.
     * @return EDA_RECT - the board's bounding box
     */
    EDA_RECT    GetBoardBoundingBox( bool aBoardEdgesOnly = false ) const;

    void SetPageSettings( const PAGE_INFO& aPageSettings );     // overload
    const PAGE_INFO& GetPageSettings() const;                   // overload
    const wxSize GetPageSizeIU() const;                         // overload

    const wxPoint& GetOriginAxisPosition() const;               // overload
    void SetOriginAxisPosition( const wxPoint& aPosition );     // overload

    const TITLE_BLOCK& GetTitleBlock() const;                   // overload
    void SetTitleBlock( const TITLE_BLOCK& aTitleBlock );       // overload

    /**
     * Function GetDesignSettings
     * returns the BOARD_DESIGN_SETTINGS for the BOARD owned by this frame.
     * Overloaded in FOOTPRINT_EDIT_FRAME.
     */
    virtual BOARD_DESIGN_SETTINGS& GetDesignSettings() const;
    virtual void SetDesignSettings( const BOARD_DESIGN_SETTINGS& aSettings );

    const ZONE_SETTINGS& GetZoneSettings() const;
    void SetZoneSettings( const ZONE_SETTINGS& aSettings );

    /**
     * Function GetPlotSettings
     * returns the PCB_PLOT_PARAMS for the BOARD owned by this frame.
     * Overloaded in FOOTPRINT_EDIT_FRAME.
     */
    virtual const PCB_PLOT_PARAMS& GetPlotSettings() const;
    virtual void SetPlotSettings( const PCB_PLOT_PARAMS& aSettings );

    /**
     * Function SetBoard
     * sets the m_Pcb member in such as way as to ensure deleting any previous
     * BOARD.
     * @param aBoard The BOARD to put into the frame.
     */
    void SetBoard( BOARD* aBoard );

    BOARD* GetBoard() const
    {
        wxASSERT( m_Pcb );
        return m_Pcb;
    }

    // General
    virtual void OnCloseWindow( wxCloseEvent& Event ) = 0;
    virtual void RedrawActiveWindow( wxDC* DC, bool EraseBg ) { }
    virtual void ReCreateHToolbar() = 0;
    virtual void ReCreateVToolbar() = 0;
    virtual void OnLeftClick( wxDC* DC, const wxPoint& MousePos ) = 0;
    virtual void OnLeftDClick( wxDC* DC, const wxPoint& MousePos ) = 0;
    virtual bool OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu )  = 0;
    virtual void ReCreateMenuBar();
    virtual void SetToolID( int aId, int aCursor, const wxString& aToolMsg );
    virtual void UpdateStatusBar();

    PCB_SCREEN* GetScreen() const { return (PCB_SCREEN*) EDA_DRAW_FRAME::GetScreen(); }

    /**
     * Function BestZoom
     * @return the "best" zoom to show the entire board or footprint on the screen.
     */

    virtual double BestZoom();

    virtual void Show3D_Frame( wxCommandEvent& event );

public:

    // Read/write functions:
    EDA_ITEM* ReadDrawSegmentDescr( LINE_READER* aReader );
    int ReadListeSegmentDescr( LINE_READER* aReader,
                               TRACK*       PtSegm,
                               int          StructType,
                               int          NumSegm );

    int ReadSetup( LINE_READER* aReader );
    int ReadGeneralDescrPcb( LINE_READER* aReader );


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
    BOARD_ITEM* PcbGeneralLocateAndDisplay( int aHotKeyCode = 0 );

    void ProcessItemSelection( wxCommandEvent& event );

    /**
     * Function SetCurItem
     * sets the currently selected item and displays it in the MsgPanel.
     * If the given item is NULL then the MsgPanel is erased and there is no
     * currently selected item. This function is intended to make the process
     * of "selecting" an item more formal, and to indivisibly tie the operation
     * of selecting an item to displaying it using BOARD_ITEM::Display_Infos().
     * @param aItem The BOARD_ITEM to make the selected item or NULL if none.
     * @param aDisplayInfo = true to display item info, false if not (default = true)
     */
    void SetCurItem( BOARD_ITEM* aItem, bool aDisplayInfo = true );

    BOARD_ITEM* GetCurItem();

    /**
     * Function GetCollectorsGuide
     * @return GENERAL_COLLECTORS_GUIDE - that considers the global
     *configuration options.
     */
    GENERAL_COLLECTORS_GUIDE GetCollectorsGuide();

    /**
     * Function CursorGoto
     * positions the cursor at a given coordinate and reframes the drawing if the
     * requested point is out of view.
     * @param aPos is the point to go to.
     * @param aWarp is true if the pointer should be warped to the new position.
     */
    void CursorGoto( const wxPoint& aPos, bool aWarp = true );

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
    bool Save_Module_In_Library( const wxString& aLibName,
                                 MODULE*         aModule,
                                 bool            aOverwrite,
                                 bool            aDisplayDialog );

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
    virtual void OnModify();

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

    void Edit_Module( MODULE* module, wxDC* DC );
    void Rotate_Module( wxDC* DC, MODULE* module, int angle, bool incremental );

    /**
     * Function PlaceModule
     * places \a aModule at the current cursor position and updates module coordinates
     * with the new position.
     *
     * @param aModule A MODULE object point of the module to be placed.
     * @param aDC A wxDC object point of the device context to draw \a aModule on
     *            or  NULL if no display screen need updated.
     * @param aDoNotRecreateRatsnest A bool true redraws the module rats nest.
     */
    void PlaceModule( MODULE* aModule, wxDC* aDC, bool aDoNotRecreateRatsnest = false );

    // module texts
    void RotateTextModule( TEXTE_MODULE* Text, wxDC* DC );
    void DeleteTextModule( TEXTE_MODULE* Text );
    void PlaceTexteModule( TEXTE_MODULE* Text, wxDC* DC );
    void StartMoveTexteModule( TEXTE_MODULE* Text, wxDC* DC );
    TEXTE_MODULE* CreateTextModule( MODULE* Module, wxDC* DC );

    /**
     * Function ResetTextSize
     * resets given field text size and width to current settings in
     * Preferences->Dimensions->Texts and Drawings.
     * @param aItem is the item to be reset, either TEXTE_PCB or TEXTE_MODULE.
     * @param aDC is the drawing context.
     */
    void ResetTextSize( BOARD_ITEM* aItem, wxDC* aDC );

    /**
     * Function ResetModuleTextSizes
     * resets text size and width of all module text fields of given field
     * type to current settings in Preferences->Dimensions->Texts and Drawings.
     * @param aType is the field type (TEXT_is_REFERENCE, TEXT_is_VALUE, or TEXT_is_DIVERS).
     * @param aDC is the drawing context.
     */
    void ResetModuleTextSizes( int aType, wxDC* aDC );

    void InstallPadOptionsFrame( D_PAD*         pad );
    void InstallTextModOptionsFrame( TEXTE_MODULE* TextMod, wxDC* DC );

    void AddPad( MODULE* Module, bool draw );

    /**
     * Function DeletePad
     * Delete the pad aPad.
     * Refresh the modified screen area
     * Refresh modified parameters of the parent module (bounding box, last date)
     * @param aPad = the pad to delete
     * @param aQuery = true to prompt for confirmation, false to delete silently
     */
    void DeletePad( D_PAD* aPad, bool aQuery = true );

    void StartMovePad( D_PAD* Pad, wxDC* DC );
    void RotatePad( D_PAD* Pad, wxDC* DC );
    void PlacePad( D_PAD* Pad, wxDC* DC );
    void Export_Pad_Settings( D_PAD* aPad );
    void Import_Pad_Settings( D_PAD* aPad, bool aDraw );

    /**
     * Function GlobalChange_PadSettings
     * Function to change pad caracteristics for the given footprint
     * or all footprints which look like the given footprint
     * @param aPad is the pattern. The given footprint is the parent of this pad
     * @param aSameFootprints: if true, make changes on all identical footprints
     * @param aPadShapeFilter: if true, make changes only on pads having the same shape as aPad
     * @param aPadOrientFilter: if true, make changes only on pads having the same orientation as aPad
     * @param aPadLayerFilter: if true, make changes only on pads having the same layers as aPad
     * @param aRedraw: if true: redraws the footprint
     * @param aSaveForUndo: if true: create an entry in the Undo/Redo list
     *        (usually: true in Schematic editor, false in Module editor)
     */
    void GlobalChange_PadSettings( D_PAD* aPad,
                                    bool aSameFootprints,
                                    bool aPadShapeFilter,
                                    bool aPadOrientFilter,
                                    bool aPadLayerFilter,
                                    bool aRedraw, bool aSaveForUndo );

    // loading footprints

    /**
     * Function loadFootprintFromLibrary
     * loads @a aFootprintName from @a aLibraryPath.
     * If found add the module is also added to the BOARD, just for good measure.
     *
     *  @param aLibraryFullFilename - the full filename of the library to read. If empty,
     *                   all active libraries are read
     *
     *  @param aFootprintName is the footprint to load
     *
     *  @param aDisplayError = true to display an error message if any.
     *
     *  @return MODULE* - new module, or NULL
     */
    MODULE* loadFootprintFromLibrary( const wxString& aLibraryPath,
              const wxString& aFootprintName, bool aDisplayError );

    MODULE* loadFootprintFromLibraries( const wxString& aFootprintName,
                              bool            aDisplayError );

    /**
     * Function GetModuleLibrary
     * scans active libraries to find and load @a aFootprintName.
     * If found add the module is also added to the BOARD, just for good measure.
     *
     *  @param aFootprintName is the footprint to load
     *
     *  @param aDisplayError = true to display an error message if any.
     *
     *  @return a pointer to the new module, or NULL
     *
     */
    MODULE* GetModuleLibrary( const wxString& aLibraryPath, const wxString& aFootprintName,
                              bool aDisplayError )
    {
        if( !aLibraryPath )
            return loadFootprintFromLibraries( aFootprintName, aDisplayError );
        else
            return loadFootprintFromLibrary( aLibraryPath, aFootprintName, aDisplayError );
    }

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
     *  @return wxEmptyString if abort or fails, or the selected module name if Ok
     */
    wxString Select_1_Module_From_List( EDA_DRAW_FRAME* aWindow,
                                        const wxString& aLibraryFullFilename,
                                        const wxString& aMask,
                                        const wxString& aKeyWord );

    /**
     * Function Load_Module_From_Library
     * opens a dialog to select a footprint, and loads it into current board.
     *
     * @param aLibrary = the library name to use, or empty string to search
     * in all loaded libraries
     * @param aUseFootprintViewer = true to show the option
     * allowing the footprint selection by the footprint viewer
     * @param aDC (can be NULL ) = the current Device Context, to draw the new footprint
     */
    MODULE* Load_Module_From_Library( const wxString& aLibrary,
                                      bool aUseFootprintViewer = true,
                                      wxDC* aDC = NULL );

    /**
     * SelectFootprintFromLibBrowser
     * Launch the footprint viewer to select the name of a footprint to load.
     * @return the selected footprint name
     */
    wxString SelectFootprintFromLibBrowser( void );

    /**
     * Function GetActiveViewerFrame
     * @return a reference to the current Module Viewer Frame if exists
     * if called from the PCB editor, this is the m_ModuleViewerFrame
     * or m_ModuleEditFrame->m_ModuleViewerFrame
     * if called from the module editor, this is the m_ModuleViewerFrame
     * or parent->m_ModuleViewerFrame
     */
    FOOTPRINT_VIEWER_FRAME * GetActiveViewerFrame();

    //  ratsnest functions
    /**
     * Function Compile_Ratsnest
     *  Create the entire board ratsnest.
     *  Must be called after a board change (changes for
     *  pads, footprints or a read netlist ).
     * @param aDC = the current device context (can be NULL)
     * @param aDisplayStatus : if true, display the computation results
     */
    void Compile_Ratsnest( wxDC* aDC, bool aDisplayStatus );

    /**
     * Function build_ratsnest_module
     * Build a ratsnest relative to one footprint. This is a simplified computation
     * used only in move footprint. It is not optimal, but it is fast and sufficient
     * to help a footprint placement
     * It shows the connections from a pad to the nearest connected pad
     * @param aModule = module to consider.
     */
    void build_ratsnest_module( MODULE* aModule );

    /**
     * Function TraceModuleRatsNest
     * display the rats nest of a moving footprint, computed by
     * build_ratsnest_module()
     */
    void TraceModuleRatsNest( wxDC* aDC );

    /**
     * Function Build_Board_Ratsnest.
     * Calculates the full ratsnest depending only on pads.
     */
    void Build_Board_Ratsnest();

    /**
     *  function Displays the general ratsnest
     *  Only ratsnest with the status bit CH_VISIBLE is set are displayed
     * @param aDC = the current device context (can be NULL)
     * @param aNetcode if > 0, Display only the ratsnest relative to the
     *                 corresponding net_code
     */
    void DrawGeneralRatsnest( wxDC* aDC, int aNetcode = 0 );

    /**
     * Function TraceAirWiresToTargets
     * This functions shows airwires to nearest connecting points (pads)
     * from the current new track end during track creation
     * Uses data prepared by BuildAirWiresTargetsList()
     * @param aDC = the current device context
     */
    void TraceAirWiresToTargets( wxDC* aDC );

    /**
     * Function BuildAirWiresTargetsList
     * Build a list of candidates that can be a coonection point
     * when a track is started.
     * This functions prepares data to show airwires to nearest connecting points (pads)
     * from the current new track to candidates during track creation
     * @param aItemRef = the item connected to the starting point of the new track (track or pad)
     * @param aPosition = the position of the new track end (usually the mouse cursor on grid)
     * @param aInit = true to build full candidate list or false to update data
     * When aInit = false, aItemRef is not used (can be NULL)
     */
    void BuildAirWiresTargetsList( BOARD_CONNECTED_ITEM* aItemRef,
                                   const wxPoint& aPosition, bool aInit );

    /**
     * Function TestForActiveLinksInRatsnest
     * Explores the full rats nest list (which must exist) to determine
     * the ACTIVE links in the full rats nest list
     * When tracks exist between pads, a link can connect 2 pads already connected by a track
     * and the link is said inactive.
     * When a link connects 2 pads not already connected by a track, the link is said active.
     * @param aNetCode = net code to test. If 0, test all nets
     */
    void TestForActiveLinksInRatsnest( int aNetCode );

    /**
     * Function TestConnections
     * tests the connections relative to all nets.
     * <p>
     * This function update the status of the ratsnest ( flag CH_ACTIF = 0 if a connection
     * is found, = 1 else) track segments are assumed to be sorted by net codes.
     * This is the case because when a new track is added, it is inserted in the linked list
     * according to its net code. and when nets are changed (when a new netlist is read)
     * tracks are sorted before using this function.
     * </p>
     */
    void TestConnections();

    /**
     * Function TestNetConnection
     * tests the connections relative to \a aNetCode.  Track segments are assumed to be
     * sorted by net codes.
     * @param aDC Current Device Context
     * @param aNetCode The net code to test
     */
    void TestNetConnection( wxDC* aDC, int aNetCode );

    /**
     * Function RecalculateAllTracksNetcode
     * search connections between tracks and pads and propagate pad net codes to the track
     * segments.
     */
    void RecalculateAllTracksNetcode();

    /* Plotting functions:
     * Return true if OK, false if the file is not created (or has a problem)
     */

    /**
     * Function ExportToGerberFile
     * create one output files one per board layer in the RS274X format.
     * <p>
     * The units are in inches and in the format 3.4 with the leading zeros omitted.
     * Coordinates are absolute value.  The 3.4 format is used because the native Pcbnew
     * units are 1/10000 inch.
     * </p>
     */
    bool ExportToGerberFile( const wxString& aFullFileName,
                             int             aLayer,
                             bool            aPlotOriginIsAuxAxis,
                             EDA_DRAW_MODE_T aTraceMode );

    bool ExportToHpglFile( const wxString& aFullFileName,
                           int             aLayer,
                           EDA_DRAW_MODE_T aTraceMode );

    bool ExportToPostScriptFile( const wxString& aFullFileName,
                                 int             aLayer,
                                 bool            aUseA4,
                                 EDA_DRAW_MODE_T aTraceMode );

    bool ExportToDxfFile( const wxString& aFullFileName,
                          int             aLayer,
                          EDA_DRAW_MODE_T aTraceMode );

    void Plot_Layer( PLOTTER*        plotter,
                     int             Layer,
                     EDA_DRAW_MODE_T trace_mode );

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
    void Plot_Standard_Layer( PLOTTER* aPlotter, int aLayerMask,
                              bool aPlotVia, EDA_DRAW_MODE_T aPlotMode,
                              bool aSkipNPTH_Pads = false );

    void PlotSilkScreen( PLOTTER* plotter, int masque_layer, EDA_DRAW_MODE_T trace_mode );

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
    void PlotDrillMark( PLOTTER* aPlotter, EDA_DRAW_MODE_T aTraceMode, bool aSmallDrillShape );

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
    int SelectLayer( int default_layer, int min_layer, int max_layer, bool null_layer = false );
    void SelectLayerPair();
    virtual void SwitchLayer( wxDC* DC, int layer );

    void InstallGridFrame( const wxPoint& pos );

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

#endif  // WX_BASE_PCB_FRAME_H_
