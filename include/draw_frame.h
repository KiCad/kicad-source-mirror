#ifndef EDA_DRAW_FRAME_H_
#define EDA_DRAW_FRAME_H_
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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

#include <wxstruct.h>
#include <kiway_player.h>
#include <climits>

class wxSingleInstanceChecker;
class EDA_HOTKEY;

#define DEFAULT_MAX_UNDO_ITEMS 0
#define ABS_MAX_UNDO_ITEMS (INT_MAX / 2)

/**
 * Class EDA_DRAW_FRAME
 * is the base class for create windows for drawing purpose.  The Eeschema, Pcbnew and
 * GerbView main windows are just a few examples of classes derived from EDA_DRAW_FRAME.
 */
class EDA_DRAW_FRAME : public KIWAY_PLAYER
{
    /// Let the #EDA_DRAW_PANEL object have access to the protected data since
    /// it is closely tied to the #EDA_DRAW_FRAME.
    friend class EDA_DRAW_PANEL;

    ///< Id of active button on the vertical toolbar.
    int         m_toolId;

    BASE_SCREEN*    m_currentScreen;            ///< current used SCREEN

    bool        m_snapToGrid;                   ///< Indicates if cursor should be snapped to grid.
    bool        m_galCanvasActive;              ///< whether to use new GAL engine

    EDA_DRAW_PANEL_GAL* m_galCanvas;

protected:

    wxSingleInstanceChecker* m_file_checker;    ///< prevents opening same file multiple times.

    EDA_HOTKEY_CONFIG* m_hotkeysDescrList;
    int         m_LastGridSizeId;           // the command id offset (>= 0) of the last selected grid
                                            // 0 is for the grid corresponding to
                                            // a wxCommand ID = ID_POPUP_GRID_LEVEL_1000.
    bool        m_drawGrid;                 // hide/Show grid
    bool        m_showPageLimits;           ///< true to display the page limits
    EDA_COLOR_T m_gridColor;                // Grid color
    EDA_COLOR_T m_drawBgColor;              ///< the background color of the draw canvas
                                            ///< BLACK for Pcbnew, BLACK or WHITE for eeschema
    double      m_zoomLevelCoeff;           ///< a suitable value to convert the internal zoom scaling factor
                                            // to a zoom level value which rougly gives 1.0 when the board/schematic
                                            // is at scale = 1
    int         m_UndoRedoCountMax;         ///< default Undo/Redo command Max depth, to be handed
                                            // to screens

    /// The area to draw on.
    EDA_DRAW_PANEL* m_canvas;

    TOOL_MANAGER*       m_toolManager;
    TOOL_DISPATCHER*    m_toolDispatcher;

    /// Tool ID of previously active draw tool bar button.
    int     m_lastDrawToolId;

    /// The shape of the KiCad cursor.  The default value (0) is the normal cross
    /// hair cursor.  Set to non-zero value to draw the full screen cursor.
    /// @note This is not the system mouse cursor.
    int     m_cursorShape;

    /// True shows the X and Y axis indicators.
    bool    m_showAxis;

    /// True shows the grid axis indicators.
    bool    m_showGridAxis;

    /// True shows the origin axis used to indicate the coordinate offset for
    /// drill, gerber, and component position files.
    bool    m_showOriginAxis;

    /// True shows the drawing border and title block.
    bool    m_showBorderAndTitleBlock;

    /// Choice box to choose the grid size.
    wxChoice*       m_gridSelectBox;

    /// Choice box to choose the zoom value.
    wxChoice*       m_zoomSelectBox;

    /// The tool bar that contains the buttons for quick access to the application draw
    /// tools.  It typically is located on the right side of the main window.
    wxAuiToolBar*   m_drawToolBar;

    /// The options tool bar typcially located on the left edge of the main window.
    wxAuiToolBar*   m_optionsToolBar;

    /// Panel used to display information at the bottom of the main window.
    EDA_MSG_PANEL*  m_messagePanel;

    int             m_MsgFrameHeight;

#ifdef USE_WX_OVERLAY
    // MAC Uses overlay to workaround the wxINVERT and wxXOR miss
    wxOverlay       m_overlay;
#endif

    /// One-shot to avoid a recursive mouse event during hotkey movement
    bool            m_movingCursorWithKeyboard;

    void SetScreen( BASE_SCREEN* aScreen )  { m_currentScreen = aScreen; }

    /**
     * Function unitsChangeRefresh
     * is called when when the units setting has changed to allow for any derived classes
     * to handle refreshing and controls that have units based measurements in them.  The
     * default version only updates the status bar.  Don't forget to call the default
     * in your derived class or the status bar will not get updated properly.
     */
    virtual void unitsChangeRefresh();

    /**
     * Function GeneralControlKeyMovement
     * Handle the common part of GeneralControl dedicated to global
     * cursor keys (i.e. cursor movement by keyboard) */
    void GeneralControlKeyMovement( int aHotKey, wxPoint *aPos, bool aSnapToGrid );

    /* Function RefreshCrosshair
     * Move and refresh the crosshair after movement; also call the
     * mouse capture function, if active.
     */
    void RefreshCrossHair( const wxPoint &aOldPos, const wxPoint &aEvtPos, wxDC* aDC );
public:
    EDA_DRAW_FRAME( KIWAY* aKiway, wxWindow* aParent,
                    FRAME_T aFrameType,
                    const wxString& aTitle,
                    const wxPoint& aPos, const wxSize& aSize,
                    long aStyle,
                    const wxString& aFrameName );

    ~EDA_DRAW_FRAME();

    /**
     * Function LockFile
     * marks a schematic file as being in use.  Use ReleaseFile() to undo this.
     * @param aFileName = full path to the file.
     * @return false if the file was already locked, true otherwise.
     */
    bool LockFile( const wxString& aFileName );

    /**
     * Function ReleaseFile
     * Release the current file marked in use.  See m_file_checker.
     */
    void ReleaseFile();

    virtual void SetPageSettings( const PAGE_INFO& aPageSettings ) = 0;
    virtual const PAGE_INFO& GetPageSettings() const = 0;

    /**
     * Function GetPageSizeIU
     * works off of GetPageSettings() to return the size of the paper page in
     * the internal units of this particular view.
     */
    virtual const wxSize GetPageSizeIU() const = 0;

    /**
     * Function GetAuxOrigin
     * returns the origin of the axis used for plotting and various exports.
     */
    virtual const wxPoint& GetAuxOrigin() const = 0;
    virtual void SetAuxOrigin( const wxPoint& aPosition ) = 0;

    /**
     * Function GetGridOrigin
     * returns the absolute coordinates of the origin of the snap grid.  This is
     * treated as a relative offset, and snapping will occur at multiples of the grid
     * size relative to this point.
     */
    virtual const wxPoint& GetGridOrigin() const = 0;
    virtual void SetGridOrigin( const wxPoint& aPosition ) = 0;

    //-----<BASE_SCREEN API moved here>------------------------------------------
    /**
     * Function GetCrossHairPosition
     * return the current cross hair position in logical (drawing) coordinates.
     * @param aInvertY Inverts the Y axis position.
     * @return The cross hair position in drawing coordinates.
     */
    wxPoint GetCrossHairPosition( bool aInvertY = false ) const;

    /**
     * Function SetCrossHairPosition
     * sets the screen cross hair position to \a aPosition in logical (drawing) units.
     * @param aPosition The new cross hair position.
     * @param aSnapToGrid Sets the cross hair position to the nearest grid position to
     *                    \a aPosition.
     *
     */
    void SetCrossHairPosition( const wxPoint& aPosition, bool aSnapToGrid = true );

    /**
     * Function GetCursorPosition
     * returns the current cursor position in logical (drawing) units.
     * @param aOnGrid Returns the nearest grid position at the current cursor position.
     * @param aGridSize Custom grid size instead of the current grid size.  Only valid
     *        if \a aOnGrid is true.
     * @return The current cursor position.
     */
    wxPoint GetCursorPosition( bool aOnGrid, wxRealPoint* aGridSize = NULL ) const;

    /**
     * Function GetNearestGridPosition
     * returns the nearest \a aGridSize location to \a aPosition.
     * @param aPosition The position to check.
     * @param aGridSize The grid size to locate to if provided.  If NULL then the current
     *                  grid size is used.
     * @return The nearst grid position.
     */
    wxPoint GetNearestGridPosition( const wxPoint& aPosition, wxRealPoint* aGridSize = NULL ) const;

    /**
     * Function GetCursorScreenPosition
     * returns the cross hair position in device (display) units.b
     * @return The current cross hair position.
     */
    wxPoint GetCrossHairScreenPosition() const;

    void SetMousePosition( const wxPoint& aPosition );

    /**
     * Function RefPos
     * Return the reference position, coming from either the mouse position
     * or the cursor position.
     *
     * @param useMouse If true, return mouse position, else cursor's.
     *
     * @return wxPoint - The reference point, either the mouse position or
     *                   the cursor position.
     */
    wxPoint RefPos( bool useMouse ) const;

    const wxPoint& GetScrollCenterPosition() const;
    void SetScrollCenterPosition( const wxPoint& aPoint );

    //-----</BASE_SCREEN API moved here>-----------------------------------------


    virtual const TITLE_BLOCK& GetTitleBlock() const = 0;
    virtual void SetTitleBlock( const TITLE_BLOCK& aTitleBlock ) = 0;

    // the background color of the draw canvas:
    // Virtual because some frames can have a specific way to get/set the bg color
    /**
     * @return the EDA_COLOR_T for the canvas background
     */
    virtual EDA_COLOR_T GetDrawBgColor() const { return m_drawBgColor; }

    /**
     * @param aColor: the EDA_COLOR_T for the canvas background
     */
    virtual void SetDrawBgColor( EDA_COLOR_T aColor) { m_drawBgColor= aColor ; }

    int GetCursorShape() const { return m_cursorShape; }

    virtual void SetCursorShape( int aCursorShape ) { m_cursorShape = aCursorShape; }

    bool GetShowBorderAndTitleBlock() const { return m_showBorderAndTitleBlock; }

    void SetShowBorderAndTitleBlock( bool aShow ) { m_showBorderAndTitleBlock = aShow; }
    bool ShowPageLimits() const { return m_showPageLimits; }
    void SetShowPageLimits( bool aShow ) { m_showPageLimits = aShow; }

    EDA_DRAW_PANEL* GetCanvas() { return m_canvas; }

    virtual wxString GetScreenDesc() const;

    /**
     * Function GetScreen
     * returns a pointer to a BASE_SCREEN or one of its
     * derivatives.  It is overloaded by derived classes to return
     * SCH_SCREEN or PCB_SCREEN.
     */
    virtual BASE_SCREEN* GetScreen() const  { return m_currentScreen; }

    /**
     * Execute a remote command send via a socket to the application,
     * port KICAD_PCB_PORT_SERVICE_NUMBER (currently 4242)
     * It called by EDA_DRAW_FRAME::OnSockRequest().
     * this is a virtual function becuse the actual commands depends on the
     * application.
     * the basic function do nothing
     * @param cmdline = received command from socket
     */
    virtual void ExecuteRemoteCommand( const char* cmdline ){}

    void OnMenuOpen( wxMenuEvent& event );
    void  OnMouseEvent( wxMouseEvent& event );

    /**
     * function SkipNextLeftButtonReleaseEvent
     * after calling this function, if the left mouse button
     * is down, the next left mouse button release event will be ignored.
     * It is is usefull for instance when closing a dialog on a mouse click,
     * to skip the next mouse left button release event
     * by the parent window, because the mouse button
     * clicked on the dialog is often released in the parent frame,
     * and therefore creates a left button released mouse event
     * which can be unwanted in some cases
     */
    void SkipNextLeftButtonReleaseEvent();

    ///> @copydoc EDA_BASE_FRAME::WriteHotkeyConfig
    int WriteHotkeyConfig( struct EDA_HOTKEY_CONFIG* aDescList, wxString* aFullFileName = NULL );

    /**
     * Function GetHotkeyConfig()
     * Returns a structure containing currently used hotkey mapping.
     */
    EDA_HOTKEY_CONFIG* GetHotkeyConfig() const { return m_hotkeysDescrList; }

    /**
     * Function GetHotKeyDescription
     * Searches lists of hot key identifiers (HK_xxx) used in the frame to find a matching
     * hot key descriptor.
     * @param aCommand is the hot key identifier.
     * @return Hot key descriptor or NULL if none found.
     */
    virtual EDA_HOTKEY* GetHotKeyDescription( int aCommand ) const = 0;

    virtual bool OnHotKey( wxDC* aDC, int aHotKey, const wxPoint& aPosition,
                           EDA_ITEM* aItem = NULL );

    /**
     * Function AddMenuZoomAndGrid (virtual)
     * Add standard zoom commands and submenu zoom and grid selection to a popup menu
     * uses zoom hotkeys info base to add hotkeys info to menu commands
     * @param aMasterMenu = the menu to populate.
     */
    virtual void AddMenuZoomAndGrid( wxMenu* aMasterMenu );

    /**
     * Function GetZoomLevelIndicator
     * returns a human readable value which can be displayed as zoom
     * level indicator in dialogs.
     * this can be a percentage or other indicator.
     * it is virtual because it could be different for pcbnew, gerbview or eeschema
     * (different internal units and different purposes)
     * note also adjust m_zoomLevelCoeff is the way to adjust the displayed value
     */
    virtual const wxString GetZoomLevelIndicator() const;

    /**
     * Function GetZoomLevelCoeff
     * returns the coefficient to convert internal display scale factor to zoom level.
     */
    inline double GetZoomLevelCoeff() const { return m_zoomLevelCoeff; }

    void EraseMsgBox();
    void Process_PageSettings( wxCommandEvent& event );

    virtual void ReCreateHToolbar() = 0;
    virtual void ReCreateVToolbar() = 0;
    virtual void ReCreateMenuBar();
    virtual void ReCreateAuxiliaryToolbar();

    /**
     * Function SetToolID
     * sets the tool command ID to \a aId and sets the cursor to \a aCursor.  The
     * command ID must be greater or equal ::ID_NO_TOOL_SELECTED.  If the command
     * ID is less than ::ID_NO_TOOL_SELECTED, the tool command ID is set to
     * ::ID_NO_TOOL_SELECTED.  On debug builds, an assertion will be raised when
     * \a aId is invalid.
     * @param aId New tool command ID if greater than or equal to ::ID_NO_TOOL_SELECTED.
                  If less than zero, the current tool command ID is retained.
     * @param aCursor Sets the cursor shape if greater than or equal to zero.
     * @param aToolMsg The tool message to set in the status bar.
     */
    virtual void SetToolID( int aId, int aCursor, const wxString& aToolMsg );

    int GetToolId() const { return m_toolId; }

    /* These 4 functions provide a basic way to show/hide grid
     * and /get/set grid color.
     * These parameters are saved in KiCad config for each main frame
     */
    /**
     * Function IsGridVisible() , virtual
     * @return true if the grid must be shown
     */
    virtual bool IsGridVisible() const
    {
        return m_drawGrid;
    }

    /**
     * Function SetGridVisibility() , virtual
     * It may be overloaded by derived classes
     * @param aVisible = true if the grid must be shown
     */
    virtual void SetGridVisibility( bool aVisible )
    {
        m_drawGrid = aVisible;
    }

    /**
     * Function GetGridColor() , virtual
     * @return the color of the grid
     */
    virtual EDA_COLOR_T GetGridColor() const
    {
        return m_gridColor;
    }

    /**
     * Function SetGridColor() , virtual
     * @param aColor = the new color of the grid
     */
    virtual void SetGridColor( EDA_COLOR_T aColor )
    {
        m_gridColor = aColor;
    }

    /**
     * Function GetGridPosition
     * returns the nearest grid position to \a aPosition if a screen is defined and snap to
     * grid is enabled.  Otherwise, the original positions is returned.
     * @see m_snapToGrid and m_BaseScreen members.
     * @param aPosition The position to test.
     * @return The wxPoint of the appropriate cursor position.
     */
    wxPoint GetGridPosition( const wxPoint& aPosition ) const;

    /**
     * Function SetNextGrid()
     * changes the grid size settings to the next one available.
     */
    virtual void SetNextGrid();

    /**
     * Function SetPrevGrid()
     * changes the grid size settings to the previous one available.
     */
    virtual void SetPrevGrid();

    /**
     * Function SetPresetGrid()
     * changes the grid size to one of the preset values.
     * @param aIndex is the index from the list.
     */
    void SetPresetGrid( int aIndex );

    /**
     * Command event handler for selecting grid sizes.
     *
     * All commands that set the grid size should eventually end up here.
     * This is where the application setting is saved.  If you override
     * this method, make sure you call down to the base class.
     *
     * @param event - Command event passed by selecting grid size from the
     *                grid size combobox on the toolbar.
     */
    virtual void OnSelectGrid( wxCommandEvent& event );

    /**
     * Functions OnSelectZoom
     * sets the zoom factor when selected by the zoom list box in the main tool bar.
     * @note List position 0 is fit to page
     *       List position >= 1 = zoom (1 to zoom max)
     *       Last list position is custom zoom not in zoom list.
     */
    virtual void OnSelectZoom( wxCommandEvent& event );

    // Command event handlers shared by all applications derived from EDA_DRAW_FRAME.
    void OnToggleGridState( wxCommandEvent& aEvent );
    void OnSelectUnits( wxCommandEvent& aEvent );
    void OnToggleCrossHairStyle( wxCommandEvent& aEvent );

    // Update user interface event handlers shared by all applications derived from
    // EDA_DRAW_FRAME.
    void OnUpdateUndo( wxUpdateUIEvent& aEvent );
    void OnUpdateRedo( wxUpdateUIEvent& aEvent );
    void OnUpdateGrid( wxUpdateUIEvent& aEvent );
    void OnUpdateUnits( wxUpdateUIEvent& aEvent );
    void OnUpdateCrossHairStyle( wxUpdateUIEvent& aEvent );

    /**
     * Function GeneralControl
     * performs application specific control using \a aDC at \a aPosition in logical units.
     * <p>
     * Override this function for application specific control.  This function gets
     * called on every mouse and key event.
     *</p>
     * @param aDC A device context.
     * @param aPosition The current cursor position in logical (drawing) units.
     * @param aHotKey A key event used for application specific control if not zero.
     */
    virtual bool GeneralControl( wxDC* aDC, const wxPoint& aPosition, int aHotKey = 0 )
    {
        return false;
    }

    /**
     * Function OnSize
     * recalculates the size of toolbars and display panel when the frame size changes.
     */
    virtual void OnSize( wxSizeEvent& event );

    void OnEraseBackground( wxEraseEvent& SizeEvent );

    virtual void OnZoom( wxCommandEvent& event );

    /**
     * Function SetNextZoom()
     * changes the zoom to the next one available.
     */
    void SetNextZoom();

    /**
     * Function SetPrevZoom()
     * changes the zoom to the previous one available.
     */
    void SetPrevZoom();

    /**
     * Function SetPresetZoom()
     * changes zoom to one of the preset values.
     * @param aIndex is the zoom index from the list.
     */
    void SetPresetZoom( int aIndex );

    /**
     * Function RedrawScreen
     * redraws the entire screen area by updating the scroll bars and mouse pointer in
     * order to have \a aCenterPoint at the center of the screen.
     * @param aCenterPoint The position in logical units to center the scroll bars.
     * @param aWarpPointer Moves the mouse cursor to \a aCenterPoint if true.
     */
    void RedrawScreen( const wxPoint& aCenterPoint, bool aWarpPointer );

    /**
     * Function RedrawScreen2
     * puts the crosshair back to the screen position it had before zooming
     * @param posBefore screen position of the crosshair before zooming
     */
    void RedrawScreen2( const wxPoint& posBefore );

    /**
     * Function Zoom_Automatique
     * redraws the screen with best zoom level and the best centering
     * that shows all the page or the board
     */
    void Zoom_Automatique( bool aWarpPointer );

    /* Set the zoom level to show the area Rect */
    void Window_Zoom( EDA_RECT& Rect );

    /** Return the zoom level which displays the full page on screen */
    virtual double BestZoom() = 0;

    /**
     * Function GetZoom
     * @return The current zoom level.
     */
    double GetZoom();

    /**
     * Function DrawWorkSheet
     * Draws on screen the page layout with the frame and the basic inscriptions.
     * @param aDC The device context.
     * @param aScreen screen to draw
     * @param aLineWidth The pen width to use to draw the layout.
     * @param aScale The mils to Iu conversion factor.
     * @param aFilename The filename to display in basic inscriptions.
     */
    void DrawWorkSheet( wxDC* aDC, BASE_SCREEN* aScreen, int aLineWidth,
                         double aScale, const wxString &aFilename );

    void            DisplayToolMsg( const wxString& msg );
    virtual void    RedrawActiveWindow( wxDC* DC, bool EraseBg ) = 0;
    virtual void    OnLeftClick( wxDC* DC, const wxPoint& MousePos ) = 0;
    virtual void    OnLeftDClick( wxDC* DC, const wxPoint& MousePos );
    virtual bool    OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu ) = 0;
    virtual void    ToolOnRightClick( wxCommandEvent& event );
    void            AdjustScrollBars( const wxPoint& aCenterPosition );

    /**
     * Function OnActivate (virtual)
     * is called when activating the frame.
     * In derived classes with a overriding OnActivate function,
     * do not forget to call this EDA_DRAW_FRAME::OnActivate( event ) basic function.
     */
    virtual void OnActivate( wxActivateEvent& event );

    /**
     * Function UpdateStatusBar
     * updates the status bar information.
     *
     * The base method updates the absolute and relative coordinates and the
     * zoom information.  If you override this virtual method, make sure to call
     * this subclassed method.  The status bar can draw itself.  This is not
     * a drawing function per se, but rather updates lines of text held by
     * the components within the status bar which is owned by the wxFrame.
     * <p>
     * On a MAC, be careful about calling this function when there is an
     * existing wxDC in existence on a sibling window.
     */
    virtual void UpdateStatusBar();

    /**
     * Function DisplayUnitsMsg
     * displays current unit pane on the status bar.
     */
    void DisplayUnitsMsg();

    /* Handlers for block commands */
    virtual void InitBlockPasteInfos();

    /**
     * Function HandleBlockBegin
     * initializes the block command including the command type, initial position,
     * and other variables.
     */
    virtual bool HandleBlockBegin( wxDC* aDC, int aKey, const wxPoint& aPosition );

    /**
     * Function BlockCommand
     * Returns the block command code (BLOCK_MOVE, BLOCK_COPY...) corresponding to the
     * keys pressed (ALT, SHIFT, SHIFT ALT ..) when block command is started by dragging
     * the mouse.
     *
     * @param aKey = the key modifiers (Alt, Shift ...)
     * @return the block command id (BLOCK_MOVE, BLOCK_COPY...)
     */
    virtual int BlockCommand( int aKey );

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
     * Function CopyToClipboard
     * copies the current page or the current block to the clipboard.
     */
    void CopyToClipboard( wxCommandEvent& event );

    /* interprocess communication */
    void OnSockRequest( wxSocketEvent& evt );
    void OnSockRequestServer( wxSocketEvent& evt );

    void LoadSettings( wxConfigBase* aCfg );    // override virtual

    void SaveSettings( wxConfigBase* aCfg );    // override virtual

    /**
     * Append a message to the message panel.
     *
     * This helper method checks to make sure the message panel exists in
     * the frame and appends a message to it using the message panel
     * AppendMessage() method.
     *
     * @param textUpper - The message upper text.
     * @param textLower - The message lower text.
     * @param color - A color ID from the KiCad color list (see colors.h).
     * @param pad - Number of spaces to pad between messages (default = 4).
     */
    void AppendMsgPanel( const wxString& textUpper, const wxString& textLower,
                         EDA_COLOR_T color, int pad = 6 );

    /**
     * Clear all messages from the message panel.
     */
    void ClearMsgPanel( void );

    /**
     * Function SetMsgPanel
     * clears the message panel and populates it with the contents of \a aList.
     *
     * @param aList is the list of #MSG_PANEL_ITEM objects to fill the message panel.
     */
    void SetMsgPanel( const std::vector< MSG_PANEL_ITEM >& aList );

    void SetMsgPanel( EDA_ITEM* aItem );

    /**
     * Function UpdateMsgPanel
     * redraws the message panel.
     */
    virtual void UpdateMsgPanel();

    /**
     * Function PushPreferences
     * Pushes a few preferences from a parent window to a child window.
     * (i.e. from eeschema to schematic symbol editor)
     *
     * @param aParentCanvas is the parent canvas to push preferences from.
     */
    void PushPreferences( const EDA_DRAW_PANEL* aParentCanvas );

    /**
     * Function PrintPage
     * used to print a page
     * Print the page pointed by current screen, set by the calling print function
     * @param aDC = wxDC given by the calling print function
     * @param aPrintMask = not used here
     * @param aPrintMirrorMode = not used here (Set when printing in mirror mode)
     * @param aData = a pointer on an auxiliary data (not always used, NULL if not used)
     */
    virtual void PrintPage( wxDC* aDC, LSET aPrintMask, bool aPrintMirrorMode, void* aData = NULL );

    /**
     * Function CoordinateToString
     * is a helper to convert the \a integer coordinate \a aValue to a string in inches or mm
     * according to the current user units setting.
     * @param aValue The coordinate to convert.
     * @param aConvertToMils Convert inch values to mils if true.  This setting has no effect if
     *                       the current user unit is millimeters.
     * @return The converted string for display in user interface elements.
     */
    wxString CoordinateToString( int aValue, bool aConvertToMils = false ) const;

    /**
     * Function LengthDoubleToString
     * is a helper to convert the \a double value \a aValue to a string in inches or mm
     * according to the current user units setting.
     * @param aValue The coordinate to convert.
     * @param aConvertToMils Convert inch values to mils if true.  This setting has no effect if
     *                       the current user unit is millimeters.
     * @return The converted string for display in user interface elements.
     */
    wxString LengthDoubleToString( double aValue, bool aConvertToMils = false ) const;

    /**
     * Function UseGalCanvas
     * used to switch between standard and GAL-based canvas.
     *
     * @param aEnable True for GAL-based canvas, false for standard canvas.
     */
    virtual void UseGalCanvas( bool aEnable );

    /**
     * Function IsGalCanvasActive
     * is used to check which canvas (GAL-based or standard) is currently in use.
     *
     * @return True for GAL-based canvas, false for standard canvas.
     */
    bool IsGalCanvasActive() const          { return m_galCanvasActive; }

    /**
     * Function GetGalCanvas
     * returns a pointer to GAL-based canvas of given EDA draw frame.
     *
     * @return Pointer to GAL-based canvas.
     */
    EDA_DRAW_PANEL_GAL* GetGalCanvas() const        { return m_galCanvas; }
    void SetGalCanvas( EDA_DRAW_PANEL_GAL* aPanel ) { m_galCanvas = aPanel; }

    /**
     * Function GetToolManager
     * returns the tool manager instance, if any.
     */
    TOOL_MANAGER* GetToolManager() const            { return m_toolManager; }

    /**
     * Function GetDisplayOptions
     * A way to pass info to draw functions. the base class has no knowledge about
     * these options. It is virtual because this function must be overloaded to
     * pass usefull info.
     */
    virtual void* GetDisplayOptions() { return NULL; }

    DECLARE_EVENT_TABLE()
};

#endif  // EDA_DRAW_FRAME_H_
