/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DRAW_FRAME_H_
#define DRAW_FRAME_H_

#include <eda_base_frame.h>
#include <kiway_player.h>
#include <climits>
#include <gal/gal_display_options.h>
#include <gal/color4d.h>
#include <class_draw_panel_gal.h>
#include "hotkeys_basic.h"

class wxSingleInstanceChecker;
class EDA_HOTKEY;

using KIGFX::COLOR4D;

namespace KIGFX
{
    class GAL_DISPLAY_OPTIONS;
}

#define DEFAULT_MAX_UNDO_ITEMS 0
#define ABS_MAX_UNDO_ITEMS (INT_MAX / 2)
#define LIB_EDIT_FRAME_NAME                 wxT( "LibeditFrame" )
#define SCH_EDIT_FRAME_NAME                 wxT( "SchematicFrame" )
#define PL_EDITOR_FRAME_NAME                wxT( "PlEditorFrame" )
#define FOOTPRINT_WIZARD_FRAME_NAME         wxT( "FootprintWizard" )
#define FOOTPRINT_EDIT_FRAME_NAME           wxT( "ModEditFrame" )
#define FOOTPRINT_VIEWER_FRAME_NAME         wxT( "ModViewFrame" )
#define FOOTPRINT_VIEWER_FRAME_NAME_MODAL   wxT( "ModViewFrameModal" )
#define PCB_EDIT_FRAME_NAME                 wxT( "PcbFrame" )


///@{
/// \ingroup config

/// User units
static const wxString UserUnitsEntryKeyword( wxT( "Units" ) );
/// Nonzero to show grid (suffix)
static const wxString ShowGridEntryKeyword( wxT( "ShowGrid" ) );
/// Grid color ID (suffix)
static const wxString GridColorEntryKeyword( wxT( "GridColor" ) );
/// Most recently used grid size (suffix)
static const wxString LastGridSizeIdKeyword( wxT( "_LastGridSize" ) );

///@}


/**
 * The base class for create windows for drawing purpose.  The Eeschema, Pcbnew and
 * GerbView main windows are just a few examples of classes derived from EDA_DRAW_FRAME.
 */
class EDA_DRAW_FRAME : public KIWAY_PLAYER
{
    /// Let the #EDA_DRAW_PANEL object have access to the protected data since
    /// it is closely tied to the #EDA_DRAW_FRAME.
    friend class EDA_DRAW_PANEL;

    ///< Id of active button on the vertical toolbar.
    int                 m_toolId;

    BASE_SCREEN*        m_currentScreen;      ///< current used SCREEN

    bool                m_snapToGrid;         ///< Indicates if cursor should be snapped to grid.

    EDA_DRAW_PANEL_GAL* m_galCanvas;

    ///< GAL display options - this is the frame's interface to setting GAL display options
    KIGFX::GAL_DISPLAY_OPTIONS  m_galDisplayOptions;

protected:
    bool m_galCanvasActive;    ///< whether to use new GAL engine
    bool m_useSingleCanvasPane;

    wxSocketServer*                          m_socketServer;
    std::vector<wxSocketBase*>               m_sockets;         ///< interprocess communication

    std::unique_ptr<wxSingleInstanceChecker> m_file_checker;    ///< prevents opening same file multiple times.

    EDA_HOTKEY_CONFIG*                       m_hotkeysDescrList;

    int         m_LastGridSizeId;           // the command id offset (>= 0) of the last selected grid
                                            // 0 is for the grid corresponding to
                                            // a wxCommand ID = ID_POPUP_GRID_LEVEL_1000.
    bool        m_drawGrid;                 // hide/Show grid
    bool        m_showPageLimits;           ///< true to display the page limits
    COLOR4D     m_gridColor;                ///< Grid color
    COLOR4D     m_drawBgColor;              ///< the background color of the draw canvas
                                            ///< BLACK for Pcbnew, BLACK or WHITE for eeschema
    double      m_zoomLevelCoeff;           ///< a suitable value to convert the internal zoom scaling factor
                                            // to a zoom level value which rougly gives 1.0 when the board/schematic
                                            // is at scale = 1
    int         m_UndoRedoCountMax;         ///< default Undo/Redo command Max depth, to be handed
                                            // to screens
    EDA_UNITS_T m_UserUnits;

    /// The area to draw on.
    EDA_DRAW_PANEL* m_canvas;

    TOOL_MANAGER*       m_toolManager;
    TOOL_DISPATCHER*    m_toolDispatcher;
    ACTIONS*            m_actions;

    /// Tool ID of previously active draw tool bar button.
    int     m_lastDrawToolId;

    /// True shows the X and Y axis indicators.
    bool    m_showAxis;

    /// True shows the grid axis indicators.
    bool    m_showGridAxis;

    /// True shows the origin axis used to indicate the coordinate offset for
    /// drill, gerber, and component position files.
    bool    m_showOriginAxis;

    /// True shows the drawing border and title block.
    bool    m_showBorderAndTitleBlock;

    /// Key to control whether first run dialog is shown on startup
    long    m_firstRunDialogSetting;

    wxComboBox*       m_gridSelectBox;
    wxComboBox*       m_zoomSelectBox;

    /// Auxiliary tool bar typically shown below the main tool bar at the top of the
    /// main window.
    wxAuiToolBar*   m_auxiliaryToolBar;

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

    /// Flag indicating that drawing canvas type needs to be saved to config
    bool            m_canvasTypeDirty;

    /// The current canvas type
    EDA_DRAW_PANEL_GAL::GAL_TYPE    m_canvasType;

    virtual void SetScreen( BASE_SCREEN* aScreen )  { m_currentScreen = aScreen; }

    double bestZoom( double sizeX, double sizeY, double scaleFactor, wxPoint centre );

    /**
     * Called when when the units setting has changed to allow for any derived classes
     * to handle refreshing and controls that have units based measurements in them.  The
     * default version only updates the status bar.  Don't forget to call the default
     * in your derived class or the status bar will not get updated properly.
     */
    virtual void unitsChangeRefresh();

    void CommonSettingsChanged() override;

    /**
     * @param doOpen if true runs an Open Library browser, otherwise New Library
     * @param aFilename for New may contain a default name; in both cases return the chosen
     *                  filename.
     * @param wildcard a wildcard to filter the displayed files
     * @param ext the library file extension
     * @param isDirectory indicates the library files are directories
     * @return true for OK; false for Cancel.
     */
    bool LibraryFileBrowser( bool doOpen, wxFileName& aFilename,
                             const wxString& wildcard, const wxString& ext, bool isDirectory );

    /**
     * Handle the common part of GeneralControl dedicated to global
     * cursor keys (i.e. cursor movement by keyboard)
     *
     * @param aHotKey is the hotkey code
     * @param aPos is the position of the cursor (initial then new)
     * @param aSnapToGrid = true to force the cursor position on grid
     * @return true if the hotkey code is handled (captured).
     */
    bool GeneralControlKeyMovement( int aHotKey, wxPoint *aPos, bool aSnapToGrid );

    /**
     * Move and refresh the crosshair after movement and call the mouse capture function.
     */
    void RefreshCrossHair( const wxPoint &aOldPos, const wxPoint &aEvtPos, wxDC* aDC );

    /**
     * @return true if an item edit or a block operation is in progress.
     */
    bool isBusy() const;

    /**
     * Stores the canvas type in the application settings.
     */
    bool saveCanvasTypeSetting( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType );

    bool saveCanvasImageToFile( const wxString& aFileName,
                                wxBitmapType aBitmapType = wxBITMAP_TYPE_PNG );

    ///> Key in KifaceSettings to store the canvas type.
    static const wxChar CANVAS_TYPE_KEY[];

public:
    EDA_DRAW_FRAME( KIWAY* aKiway, wxWindow* aParent,
                    FRAME_T aFrameType,
                    const wxString& aTitle,
                    const wxPoint& aPos, const wxSize& aSize,
                    long aStyle,
                    const wxString& aFrameName );

    ~EDA_DRAW_FRAME();

    /**
     * Capture the key event before it is sent to the GUI.
     *
     * the basic frame does not capture this event.
     * editor frames should override this event function to capture and filter
     * these keys when they are used as hotkeys, and skip it if the key is not
     * used as hotkey (otherwise the key events will be not sent to menus)
     */
    virtual void OnCharHook( wxKeyEvent& event );

    /**
     * Mark a schematic file as being in use.  Use ReleaseFile() to undo this.
     *
     * @param aFileName = full path to the file.
     * @return false if the file was already locked, true otherwise.
     */
    bool LockFile( const wxString& aFileName );

    /**
     * Release the current file marked in use.  See m_file_checker.
     */
    void ReleaseFile();

    virtual void SetPageSettings( const PAGE_INFO& aPageSettings ) = 0;
    virtual const PAGE_INFO& GetPageSettings() const = 0;

    /**
     * Works off of GetPageSettings() to return the size of the paper page in
     * the internal units of this particular view.
     */
    virtual const wxSize GetPageSizeIU() const = 0;

    /**
     * Return the user units currently in use.
     */
    EDA_UNITS_T GetUserUnits() const override { return m_UserUnits; }
    void SetUserUnits( EDA_UNITS_T aUnits ) { m_UserUnits = aUnits; }

    /**
     * Return the origin of the axis used for plotting and various exports.
     */
    virtual const wxPoint& GetAuxOrigin() const = 0;
    virtual void SetAuxOrigin( const wxPoint& aPosition ) = 0;

    /**
     * Return the absolute coordinates of the origin of the snap grid.  This is
     * treated as a relative offset, and snapping will occur at multiples of the grid
     * size relative to this point.
     */
    virtual const wxPoint& GetGridOrigin() const = 0;
    virtual void SetGridOrigin( const wxPoint& aPosition ) = 0;

    int GetLastGridSizeId() const { return m_LastGridSizeId; }
    void SetLastGridSizeId( int aId ) { m_LastGridSizeId = aId; }

    //-----<BASE_SCREEN API moved here>------------------------------------------
    /**
     * Return the current cross hair position in logical (drawing) coordinates.
     *
     * @param aInvertY Inverts the Y axis position.
     * @return The cross hair position in drawing coordinates.
     */
    wxPoint GetCrossHairPosition( bool aInvertY = false ) const;

    /**
     * Set the screen cross hair position to \a aPosition in logical (drawing) units.
     *
     * @param aPosition The new cross hair position.
     * @param aSnapToGrid Sets the cross hair position to the nearest grid position to
     *                    \a aPosition.
     */
    void SetCrossHairPosition( const wxPoint& aPosition, bool aSnapToGrid = true );

    /**
     * Return the current cursor position in logical (drawing) units.
     *
     * @param aOnGrid Returns the nearest grid position at the current cursor position.
     * @param aGridSize Custom grid size instead of the current grid size.  Only valid
     *        if \a aOnGrid is true.
     * @return The current cursor position.
     */
    wxPoint GetCursorPosition( bool aOnGrid, wxRealPoint* aGridSize = NULL ) const;

    /**
     * Return the nearest \a aGridSize location to \a aPosition.
     *
     * @param aPosition The position to check.
     * @param aGridSize The grid size to locate to if provided.  If NULL then the current
     *                  grid size is used.
     * @return The nearst grid position.
     */
    wxPoint GetNearestGridPosition( const wxPoint& aPosition, wxRealPoint* aGridSize = NULL ) const;

    /**
     * Return the cross hair position in device (display) units.b
     *
     * @return The current cross hair position.
     */
    wxPoint GetCrossHairScreenPosition() const;

    void SetMousePosition( const wxPoint& aPosition );

    /**
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
     * @return the COLOR4D for the canvas background
     */
    virtual COLOR4D GetDrawBgColor() const { return m_drawBgColor; }

    /**
     * @param aColor: the COLOR4D for the canvas background
     */
    virtual void SetDrawBgColor( COLOR4D aColor) { m_drawBgColor= aColor ; }

    bool GetShowBorderAndTitleBlock() const { return m_showBorderAndTitleBlock; }

    void SetShowBorderAndTitleBlock( bool aShow ) { m_showBorderAndTitleBlock = aShow; }
    bool ShowPageLimits() const { return m_showPageLimits; }
    void SetShowPageLimits( bool aShow ) { m_showPageLimits = aShow; }

    virtual EDA_DRAW_PANEL* GetCanvas() const { return m_canvas; }

    virtual wxString GetScreenDesc() const;

    /**
     * Return a pointer to a BASE_SCREEN or one of its
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
     * After calling this function, if the left mouse button
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
    int WriteHotkeyConfig( struct EDA_HOTKEY_CONFIG* aDescList, wxString* aFullFileName = NULL ) override;

    /**
     * Return a structure containing currently used hotkey mapping.
     */
    EDA_HOTKEY_CONFIG* GetHotkeyConfig() const { return m_hotkeysDescrList; }

    /**
     * Search lists of hot key identifiers (HK_xxx) used in the frame to find a matching
     * hot key descriptor.
     * @param aCommand is the hot key identifier.
     * @return Hot key descriptor or NULL if none found.
     */
    virtual EDA_HOTKEY* GetHotKeyDescription( int aCommand ) const = 0;

    virtual bool OnHotKey( wxDC* aDC, int aHotKey, const wxPoint& aPosition,
                           EDA_ITEM* aItem = NULL );

    /**
     * Add standard zoom commands and submenu zoom and grid selection to a popup menu
     * uses zoom hotkeys info base to add hotkeys info to menu commands
     * @param aMasterMenu = the menu to populate.
     */
    virtual void AddMenuZoomAndGrid( wxMenu* aMasterMenu );

    /**
     * Return a human readable value which can be displayed as zoom
     * level indicator in dialogs.
     * this can be a percentage or other indicator.
     * it is virtual because it could be different for pcbnew, gerbview or eeschema
     * (different internal units and different purposes)
     * note also adjust m_zoomLevelCoeff is the way to adjust the displayed value
     */
    virtual const wxString GetZoomLevelIndicator() const;

    /**
     * Return the coefficient to convert internal display scale factor to zoom level.
     */
    inline double GetZoomLevelCoeff() const { return m_zoomLevelCoeff; }

    void EraseMsgBox();
    void Process_PageSettings( wxCommandEvent& event );

    virtual void ReCreateHToolbar() = 0;
    virtual void ReCreateVToolbar() = 0;
    virtual void ReCreateMenuBar() override;
    virtual void ReCreateAuxiliaryToolbar();

    // Toolbar accessors
    wxAuiToolBar* GetMainToolBar() const { return m_mainToolBar; }
    wxAuiToolBar* GetOptionsToolBar() const { return m_optionsToolBar; }
    wxAuiToolBar* GetDrawToolBar() const { return m_drawToolBar; }
    wxAuiToolBar* GetAuxiliaryToolBar() const { return m_auxiliaryToolBar; }

    /**
     * Checks all the toolbars and returns true if the given tool id is toggled.
     *
     * This is needed because GerbView and Pcbnew can put some of the same tools in
     * different toolbars.
     */
    bool GetToolToggled( int aToolId );

    /**
     * Checks all the toolbars and returns a reference to the given tool id
     * or nullptr if not found
     */
    wxAuiToolBarItem* GetToolbarTool( int aToolId );

    /**
     * Set the tool command ID to \a aId and sets the cursor to \a aCursor.
     *
     * The command ID must be greater or equal ::ID_NO_TOOL_SELECTED.  If the command
     * ID is less than ::ID_NO_TOOL_SELECTED, the tool command ID is set to
     * ::ID_NO_TOOL_SELECTED.  On debug builds, an assertion will be raised when
     * \a aId is invalid.
     *
     * @param aId New tool command ID if greater than or equal to ::ID_NO_TOOL_SELECTED.
                  If less than zero, the current tool command ID is retained.
     * @param aCursor Sets the cursor shape if greater than or equal to zero.
     * @param aToolMsg The tool message to set in the status bar.
     */
    virtual void SetToolID( int aId, int aCursor, const wxString& aToolMsg );

    /**
     * Select the ID_NO_TOOL_SELECTED id tool (Idle tool)
     */
    virtual void SetNoToolSelected();

    /**
     * @return the current tool ID
     * when there is no active tool, the ID_NO_TOOL_SELECTED is returned
     * (the id of the default Tool (idle tool) of the right vertical toolbar)
     */
    int GetToolId() const { return m_toolId; }

    /* These 4 functions provide a basic way to show/hide grid
     * and /get/set grid color.
     * These parameters are saved in KiCad config for each main frame
     */
    /**
     * @return true if the grid must be shown
     */
    virtual bool IsGridVisible() const
    {
        return m_drawGrid;
    }

    /**
     * It may be overloaded by derived classes
     * @param aVisible = true if the grid must be shown
     */
    virtual void SetGridVisibility( bool aVisible )
    {
        m_drawGrid = aVisible;
    }

    /**
     * @return the color of the grid
     */
    virtual COLOR4D GetGridColor()
    {
        return m_gridColor;
    }

    /**
     * @param aColor = the new color of the grid
     */
    virtual void SetGridColor( COLOR4D aColor )
    {
        m_gridColor = aColor;
    }

    /**
     * Return the nearest grid position to \a aPosition if a screen is defined and snap to
     * grid is enabled.  Otherwise, the original positions is returned.
     *
     * @see m_snapToGrid and m_BaseScreen members.
     * @param aPosition The position to test.
     * @return The wxPoint of the appropriate cursor position.
     */
    wxPoint GetGridPosition( const wxPoint& aPosition ) const;

    /**
     * Change the grid size settings to the next one available.
     */
    virtual void SetNextGrid();

    /**
     * Change the grid size settings to the previous one available.
     */
    virtual void SetPrevGrid();

    /**
     * Change the grid size to one of the preset values.
     *
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

    virtual void OnGridSettings( wxCommandEvent& event ) { };

    /**
     * Set the zoom factor when selected by the zoom list box in the main tool bar.
     *
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
    void OnUpdateSelectGrid( wxUpdateUIEvent& aEvent );
    void OnUpdateUnits( wxUpdateUIEvent& aEvent );
    void OnUpdateCrossHairStyle( wxUpdateUIEvent& aEvent );

    /**
     * Perform application specific control using \a aDC at \a aPosition in logical units.
     * <p>
     * Override this function for application specific control.  This function gets
     * called on every mouse and key event.
     *</p>
     * @param aDC A device context.
     * @param aPosition The current cursor position in logical (drawing) units.
     * @param aHotKey A key event used for application specific control if not zero.
     * @return true if the hotkey code is handled (captured).
     */
    virtual bool GeneralControl( wxDC* aDC, const wxPoint& aPosition, EDA_KEY aHotKey = 0 )
    {
        return false;
    }

    /**
     * Recalculate the size of toolbars and display panel when the frame size changes.
     */
    virtual void OnSize( wxSizeEvent& event );

    void OnEraseBackground( wxEraseEvent& SizeEvent );

    virtual void OnZoom( wxCommandEvent& event );

    /**
     * Change the zoom to the next one available.
     */
    void SetNextZoom();

    /**
     * Change the zoom to the next one available redraws the screen
     * and warp the mouse pointer on request.
     *
     * @param aCenterPoint is the reference point for zooming
     * @param aWarpPointer = true to move the pointer to the aCenterPoint
     */
    void SetNextZoomAndRedraw( const wxPoint& aCenterPoint, bool aWarpPointer );

    /**
     * Change the zoom to the previous one available.
     */
    void SetPrevZoom();

    /**
     * Change the zoom to the previous one available redraws the screen
     * and warp the mouse pointer on request.
     *
     * @param aCenterPoint is the reference point for zooming
     * @param aWarpPointer = true to move the pointer to the aCenterPoint
     */
    void SetPreviousZoomAndRedraw( const wxPoint& aCenterPoint, bool aWarpPointer );

    /**
     * Change zoom to one of the preset values.
     *
     * @param aIndex is the zoom index from the list.
     */
    void SetPresetZoom( int aIndex );

    /**
     * Redraw the entire screen area by updating the scroll bars and mouse pointer in
     * order to have \a aCenterPoint at the center of the screen.
     *
     * @param aCenterPoint The position in logical units to center the scroll bars.
     * @param aWarpPointer Moves the mouse cursor to \a aCenterPoint if true.
     */
    virtual void RedrawScreen( const wxPoint& aCenterPoint, bool aWarpPointer );

    /**
     * Put the crosshair back to the screen position it had before zooming.
     *
     * @param posBefore screen position of the crosshair before zooming
     */
    virtual void RedrawScreen2( const wxPoint& posBefore );

    /**
     * Rebuild the GAL and redraws the screen.  Call when something went wrong.
     */
    virtual void HardRedraw();

    /**
     * Redraw the screen with best zoom level and the best centering
     * that shows all the page or the board
     */
    virtual void Zoom_Automatique( bool aWarpPointer );

    /* Set the zoom level to show the area Rect */
    virtual void Window_Zoom( EDA_RECT& Rect );

    /** Return the zoom level which displays the full page on screen */
    virtual double BestZoom() = 0;

    /**
     * Useful to focus on a particular location, in find functions
     * Move the graphic cursor (crosshair cursor) at a given coordinate and reframes
     * the drawing if the requested point is out of view or if center on location is requested.
     * @param aPos is the point to go to.
     * @param aWarpCursor is true if the pointer should be warped to the new position.
     * @param aCenterView is true if the new cursor position should be centered on canvas.
     */
    void FocusOnLocation( const wxPoint& aPos, bool aWarpCursor = true, bool aCenterView = false );

    /**
     * @return The current zoom level.
     */
    double GetZoom();

    /**
     * Draws on screen the page layout with the frame and the basic inscriptions.
     *
     * @param aDC The device context.
     * @param aScreen screen to draw
     * @param aLineWidth The pen width to use to draw the layout.
     * @param aScale The mils to Iu conversion factor.
     * @param aFilename The filename to display in basic inscriptions.
     * @param aSheetLayer The layer displayed from pcbnew.
     */
    void DrawWorkSheet( wxDC* aDC, BASE_SCREEN* aScreen, int aLineWidth,
                         double aScale, const wxString &aFilename,
                         const wxString &aSheetLayer = wxEmptyString );

    void            DisplayToolMsg( const wxString& msg );
    virtual void    RedrawActiveWindow( wxDC* DC, bool EraseBg ) = 0;
    virtual void    OnLeftClick( wxDC* DC, const wxPoint& MousePos ) = 0;
    virtual void    OnLeftDClick( wxDC* DC, const wxPoint& MousePos );
    virtual bool    OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu ) = 0;
    virtual void    ToolOnRightClick( wxCommandEvent& event );
    void            AdjustScrollBars( const wxPoint& aCenterPosition );

    /**
     * Called when modifying the page settings.
     * In derived classes it can be used to modify parameters like draw area size,
     * and any other local parameter related to the page settings.
     */
    virtual void OnPageSettingsChange() {};

    /**
     * Called when activating the frame.
     *
     * In derived classes with a overriding OnActivate function,
     * do not forget to call this EDA_DRAW_FRAME::OnActivate( event ) basic function.
     */
    virtual void OnActivate( wxActivateEvent& event );

    /**
     * Update the status bar information.
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
     * Display current unit pane on the status bar.
     */
    void DisplayUnitsMsg();

    /* Handlers for block commands */
    virtual void InitBlockPasteInfos();

    /**
     * Initialize a block command.
     *
     * @param aDC is the device context to perform the block command.
     * @param aKey is the block command key press.
     * @param aPosition is the logical position of the start of the block command.
     * @param aExplicitCommand - if this is given, begin with this command, rather
     *  than looking up the command from \a aKey.
     */
    virtual bool HandleBlockBegin( wxDC* aDC, EDA_KEY aKey, const wxPoint& aPosition,
                                   int aExplicitCommand = 0 );

    /**
     * Return the block command code (BLOCK_MOVE, BLOCK_COPY...) corresponding to the
     * keys pressed (ALT, SHIFT, SHIFT ALT ..) when block command is started by dragging
     * the mouse.
     *
     * @param aKey = the key modifiers (Alt, Shift ...)
     * @return the block command id (BLOCK_MOVE, BLOCK_COPY...)
     */
    virtual int BlockCommand( EDA_KEY aKey );

    /**
     * Called after HandleBlockEnd, when a block command needs to be
     * executed after the block is moved to its new place
     * (bloc move, drag, copy .. )
     * Parameters must be initialized in GetScreen()->m_BlockLocate
     */
    virtual void HandleBlockPlace( wxDC* DC );

    /**
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
     * Copy the current page or the current block to the clipboard.
     */
    void CopyToClipboard( wxCommandEvent& event );

    /* interprocess communication */
    void CreateServer( int service, bool local = true );
    void OnSockRequest( wxSocketEvent& evt );
    void OnSockRequestServer( wxSocketEvent& evt );

    void LoadSettings( wxConfigBase* aCfg ) override;

    void SaveSettings( wxConfigBase* aCfg ) override;

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
                         COLOR4D color, int pad = 6 );

    /**
     * Clear all messages from the message panel.
     */
    void ClearMsgPanel( void );

    /**
     * Clear the message panel and populates it with the contents of \a aList.
     *
     * @param aList is the list of #MSG_PANEL_ITEM objects to fill the message panel.
     */
    void SetMsgPanel( const std::vector< MSG_PANEL_ITEM >& aList );

    void SetMsgPanel( EDA_ITEM* aItem );

    /**
     * Redraw the message panel.
     */
    virtual void UpdateMsgPanel();

    /**
     * Push preferences from a parent window to a child window.
     * (i.e. from eeschema to schematic symbol editor)
     *
     * @param aParentCanvas is the parent canvas to push preferences from.
     */
    void PushPreferences( const EDA_DRAW_PANEL* aParentCanvas );

    /**
     * Print the page pointed by current screen, set by the calling print function.
     *
     * @param aDC = wxDC given by the calling print function
     * @param aPrintMask = not used here
     * @param aPrintMirrorMode = not used here (Set when printing in mirror mode)
     * @param aData = a pointer on an auxiliary data (not always used, NULL if not used)
     */
    virtual void PrintPage( wxDC* aDC, LSET aPrintMask, bool aPrintMirrorMode, void* aData = NULL );

    /**
     * Returns the canvas type stored in the application settings.
     */
    static EDA_DRAW_PANEL_GAL::GAL_TYPE LoadCanvasTypeSetting();

    /**
     * Use to switch between standard and GAL-based canvas.
     *
     * @param aEnable True for GAL-based canvas, false for standard canvas.
     */
    virtual void UseGalCanvas( bool aEnable );

    /**
     * Changes the current rendering backend.
     * aCanvasType is the new rendering backend type.
     * @return true if any kind of GAL canvas is used.
     */
    virtual bool SwitchCanvas( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType );

    /**
     * Function IsGalCanvasActive
     * is used to check which canvas (GAL-based or standard) is currently in use.
     *
     * @return True for GAL-based canvas, false for standard canvas.
     */
    bool IsGalCanvasActive() const          { return m_galCanvasActive; }

    /**
     * Return a pointer to GAL-based canvas of given EDA draw frame.
     *
     * @return Pointer to GAL-based canvas.
     */
    EDA_DRAW_PANEL_GAL* GetGalCanvas() const        { return m_galCanvas; }
    void SetGalCanvas( EDA_DRAW_PANEL_GAL* aPanel ) { m_galCanvas = aPanel; }

    /**
     * Return the tool manager instance, if any.
     */
    TOOL_MANAGER* GetToolManager() const            { return m_toolManager; }

    /**
     * A way to pass info to draw functions. the base class has no knowledge about
     * these options. It is virtual because this function must be overloaded to
     * pass usefull info.
     */
    virtual void* GetDisplayOptions() { return NULL; }

    /**
     * Return a reference to the gal rendering options used by GAL for rendering.
     */
    KIGFX::GAL_DISPLAY_OPTIONS& GetGalDisplayOptions() { return m_galDisplayOptions; }

    /**
     * Update the toolbars and menus (mostly settings/check buttons/checkboxes)
     * with the current controller state
     */
    virtual void SyncMenusAndToolbars( wxEvent& aEvent ) {};

    bool GetShowAxis() const { return m_showAxis; }
    bool GetShowGridAxis() const { return m_showGridAxis; }
    bool GetShowOriginAxis() const { return m_showOriginAxis; }

    virtual const BOX2I GetDocumentExtents() const;

    DECLARE_EVENT_TABLE()
};

#endif  // DRAW_FRAME_H_
