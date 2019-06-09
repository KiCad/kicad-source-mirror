/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
class ACTION_TOOLBAR;

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
#define UserUnitsEntryKeyword "Units"
/// Nonzero to show grid (suffix)
#define ShowGridEntryKeyword "ShowGrid"
/// Grid color ID (suffix)
#define GridColorEntryKeyword "GridColor"
/// Most recently used grid size (suffix)
#define LastGridSizeIdKeyword "_LastGridSize"

/// The key to store the canvas type in config. This is the base key.
/// can be a suffix if the canvas_type in config is specific to a frame
#define CanvasTypeKeyBase "canvas_type"
///@}


/**
 * The base class for create windows for drawing purpose.  The Eeschema, Pcbnew and
 * GerbView main windows are just a few examples of classes derived from EDA_DRAW_FRAME.
 */
class EDA_DRAW_FRAME : public KIWAY_PLAYER
{
    ///< Id of active button on the vertical toolbar.
    int                 m_toolId;
    wxString            m_toolMsg;

    BASE_SCREEN*        m_currentScreen;      ///< current used SCREEN

    EDA_DRAW_PANEL_GAL* m_galCanvas;

    ///< GAL display options - this is the frame's interface to setting GAL display options
    KIGFX::GAL_DISPLAY_OPTIONS  m_galDisplayOptions;

protected:
    wxSocketServer*                          m_socketServer;
    std::vector<wxSocketBase*>               m_sockets;         ///< interprocess communication

    std::unique_ptr<wxSingleInstanceChecker> m_file_checker;    ///< prevents opening same file multiple times.

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
    bool        m_PolarCoords;              //< for those frames that support polar coordinates

    TOOL_DISPATCHER*    m_toolDispatcher;

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

    wxChoice*       m_gridSelectBox;
    wxChoice*       m_zoomSelectBox;

    ACTION_TOOLBAR* m_mainToolBar;
    /// Auxiliary tool bar typically shown below the main tool bar at the top of the
    /// main window.
    ACTION_TOOLBAR* m_auxiliaryToolBar;

    /// The tool bar that contains the buttons for quick access to the application draw
    /// tools.  It typically is located on the right side of the main window.
    ACTION_TOOLBAR* m_drawToolBar;

    /// The options tool bar typcially located on the left edge of the main window.
    ACTION_TOOLBAR* m_optionsToolBar;

    /// Panel used to display information at the bottom of the main window.
    EDA_MSG_PANEL*  m_messagePanel;

    int             m_MsgFrameHeight;

    /// One-shot to avoid a recursive mouse event during hotkey movement
    bool            m_movingCursorWithKeyboard;

    /// The current canvas type
    EDA_DRAW_PANEL_GAL::GAL_TYPE    m_canvasType;

    virtual void SetScreen( BASE_SCREEN* aScreen )  { m_currentScreen = aScreen; }

    double bestZoom( double sizeX, double sizeY, double scaleFactor, wxPoint centre );

    void unitsChangeRefresh() override;

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
                             const wxString& wildcard, const wxString& ext,
                             bool isDirectory = false );

    /**
     * Stores the canvas type in the application settings.
     */
    bool saveCanvasTypeSetting( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType );

    bool saveCanvasImageToFile( const wxString& aFileName,
                                wxBitmapType aBitmapType = wxBITMAP_TYPE_PNG );

    /** @return the key in KifaceSettings to store the canvas type.
     * the base version returns only CanvasTypeKeyBase.
     * Can be overriden to return a key specific of a frame name
     */
    virtual wxString GetCanvasTypeKey()
    {
        return CanvasTypeKeyBase;
    }

public:
    EDA_DRAW_FRAME( KIWAY* aKiway, wxWindow* aParent,
                    FRAME_T aFrameType,
                    const wxString& aTitle,
                    const wxPoint& aPos, const wxSize& aSize,
                    long aStyle,
                    const wxString& aFrameName );

    ~EDA_DRAW_FRAME();

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
     * For those frames that support polar coordinates.
     */
    bool GetShowPolarCoords() const { return m_PolarCoords; }
    void SetShowPolarCoords( bool aShow ) { m_PolarCoords = aShow; }

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

    void ReCreateMenuBar() override { }
    virtual void ReCreateHToolbar() = 0;
    virtual void ReCreateVToolbar() = 0;
    virtual void ReCreateAuxiliaryToolbar() { }

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

    // Update user interface event handlers shared by all applications derived from
    // EDA_DRAW_FRAME.
    void OnUpdateSelectGrid( wxUpdateUIEvent& aEvent );

    /**
     * Recalculate the size of toolbars and display panel when the frame size changes.
     */
    virtual void OnSize( wxSizeEvent& event );

    void OnEraseBackground( wxEraseEvent& SizeEvent );

    /**
     * Change the zoom to the next one available redraws the screen
     * and warp the mouse pointer on request.
     *
     * @param aCenterPoint is the reference point for zooming
     * @param aWarpPointer = true to move the pointer to the aCenterPoint
     */
    void SetNextZoomAndRedraw( const wxPoint& aCenterPoint, bool aWarpPointer );

    /**
     * Change the zoom to the previous one available redraws the screen
     * and warp the mouse pointer on request.
     *
     * @param aCenterPoint is the reference point for zooming
     * @param aWarpPointer = true to move the pointer to the aCenterPoint
     */
    void SetPreviousZoomAndRedraw( const wxPoint& aCenterPoint, bool aWarpPointer );

    /**
     * Rebuild the GAL and redraws the screen.  Call when something went wrong.
     */
    virtual void HardRedraw();

    /**
     * Redraw the screen with best zoom level and the best centering
     * that shows all the page or the board
     */
    virtual void Zoom_Automatique( bool aWarpPointer );

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
     * Prints the page layout with the frame and the basic inscriptions.
     *
     * @param aDC The device context.
     * @param aScreen screen to draw
     * @param aLineWidth The pen width to use to draw the layout.
     * @param aScale The mils to Iu conversion factor.
     * @param aFilename The filename to display in basic inscriptions.
     * @param aSheetLayer The layer displayed from pcbnew.
     */
    void PrintWorkSheet( wxDC* aDC, BASE_SCREEN* aScreen, int aLineWidth, double aScale,
                         const wxString &aFilename, const wxString &aSheetLayer = wxEmptyString,
                         COLOR4D aColor = COLOR4D::UNSPECIFIED );

    void            DisplayToolMsg( const wxString& msg );
    wxString        GetToolMsg() { return m_toolMsg; }

    /**
     * Called when modifying the page settings.
     * In derived classes it can be used to modify parameters like draw area size,
     * and any other local parameter related to the page settings.
     */
    virtual void OnPageSettingsChange() {}

    /**
     * Update the status bar information.
     *
     * The EDA_DRAW_FRAME level updates the absolute and relative coordinates and the
     * zoom information.  If you override this virtual method, make sure to call this 
     * subclassed method.
     */
    void UpdateStatusBar() override;

    /**
     * Display current unit pane on the status bar.
     */
    void DisplayUnitsMsg();

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
    virtual void ClearMsgPanel();

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
     * Print the page pointed by current screen, set by the calling print function.
     *
     * @param aDC = wxDC given by the calling print function
     */
    virtual void PrintPage( wxDC* aDC );

    /**
     * Returns the canvas type stored in the application settings.
     */
    EDA_DRAW_PANEL_GAL::GAL_TYPE LoadCanvasTypeSetting();

    /**
     * Use to start up the GAL drawing canvas.
     */
    virtual void ActivateGalCanvas();

    /**
     * Changes the current rendering backend.
     */
    virtual void SwitchCanvas( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType );

    /**
     * Return a pointer to GAL-based canvas of given EDA draw frame.
     *
     * @return Pointer to GAL-based canvas.
     */
    EDA_DRAW_PANEL_GAL* GetGalCanvas() const        { return m_galCanvas; }
    void SetGalCanvas( EDA_DRAW_PANEL_GAL* aPanel ) { m_galCanvas = aPanel; }

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

    void RefreshCanvas() override
    {
        GetGalCanvas()->Refresh();
    }
    
    virtual const BOX2I GetDocumentExtents() const;
};

#endif  // DRAW_FRAME_H_
