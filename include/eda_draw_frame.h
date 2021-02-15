/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <gal/gal_display_options.h>
#include <gal/color4d.h>
#include <class_draw_panel_gal.h>
#include <origin_transforms.h>
#include "hotkeys_basic.h"

class wxSingleInstanceChecker;
class ACTION_TOOLBAR;
class COLOR_SETTINGS;
class TOOL_MENU;
class APP_SETTINGS_BASE;
class wxFindReplaceData;

namespace KIGFX
{
    class GAL_DISPLAY_OPTIONS;
    class RENDER_SETTINGS;
}

using KIGFX::COLOR4D;
using KIGFX::RENDER_SETTINGS;

#define LIB_EDIT_FRAME_NAME                 wxT( "LibeditFrame" )
#define SCH_EDIT_FRAME_NAME                 wxT( "SchematicFrame" )
#define PL_EDITOR_FRAME_NAME                wxT( "PlEditorFrame" )
#define FOOTPRINT_WIZARD_FRAME_NAME         wxT( "FootprintWizard" )
#define FOOTPRINT_EDIT_FRAME_NAME           wxT( "ModEditFrame" )
#define FOOTPRINT_VIEWER_FRAME_NAME         wxT( "ModViewFrame" )
#define FOOTPRINT_VIEWER_FRAME_NAME_MODAL   wxT( "ModViewFrameModal" )
#define PCB_EDIT_FRAME_NAME                 wxT( "PcbFrame" )


/**
 * The base class for create windows for drawing purpose.
 *
 * The Eeschema, Pcbnew and GerbView main windows are just a few examples of classes
 * derived from EDA_DRAW_FRAME.
 */
class EDA_DRAW_FRAME : public KIWAY_PLAYER
{
public:
    EDA_DRAW_FRAME( KIWAY* aKiway, wxWindow* aParent, FRAME_T aFrameType, const wxString& aTitle,
                    const wxPoint& aPos, const wxSize& aSize, long aStyle,
                    const wxString& aFrameName );

    ~EDA_DRAW_FRAME();

    /**
     * Mark a schematic file as being in use.
     *
     * Use #ReleaseFile() to undo this.
     *
     * @param aFileName full path to the file.
     * @return false if the file was already locked, true otherwise.
     */
    bool LockFile( const wxString& aFileName );

    /**
     * Release the current file marked in use.  See m_file_checker.
     */
    void ReleaseFile();

    wxFindReplaceData& GetFindReplaceData() { return *m_findReplaceData; }
    wxArrayString& GetFindHistoryList() { return m_findStringHistoryList; }

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
    bool GetShowPolarCoords() const { return m_polarCoords; }
    void SetShowPolarCoords( bool aShow ) { m_polarCoords = aShow; }

    void ToggleUserUnits() override;

    /**
     * Get the pair or units in current use.
     *
     * The primary unit is the main unit of the frame, and the secondary unit is the unit
     * of the other system that was used most recently.
     */
    void GetUnitPair( EDA_UNITS& aPrimaryUnit, EDA_UNITS& aSecondaryUnits );

    /**
     * Return the absolute coordinates of the origin of the snap grid.
     *
     * This is treated as a relative offset and snapping will occur at multiples of the grid
     * size relative to this point.
     */
    virtual const wxPoint& GetGridOrigin() const = 0;
    virtual void SetGridOrigin( const wxPoint& aPosition ) = 0;

    /**
     * Return the nearest \a aGridSize location to \a aPosition.
     *
     * @param aPosition The position to check.
     * @return The nearest grid position.
     */
    wxPoint GetNearestGridPosition( const wxPoint& aPosition ) const;

    /**
     * Return the nearest \a aGridSize / 2 location to \a aPosition.
     *
     * This is useful when attempting for keep outer points on grid but
     * not the middle point.
     *
     * @param aPosition The position to check.
     * @return The nearest half-grid position.
     */
    wxPoint GetNearestHalfGridPosition( const wxPoint& aPosition ) const;

    /**
     * Return a reference to the default ORIGIN_TRANSFORMS object
     */
    virtual ORIGIN_TRANSFORMS& GetOriginTransforms()
    { return m_originTransforms; }


    virtual const TITLE_BLOCK& GetTitleBlock() const = 0;
    virtual void SetTitleBlock( const TITLE_BLOCK& aTitleBlock ) = 0;

    // the background color of the draw canvas:
    // Virtual because some frames can have a specific way to get/set the bg color
    virtual COLOR4D GetDrawBgColor() const { return m_drawBgColor; }
    virtual void SetDrawBgColor( COLOR4D aColor) { m_drawBgColor= aColor ; }

    /// Returns a pointer to the active color theme settings
    virtual COLOR_SETTINGS* GetColorSettings() const;

    bool ShowPageLimits() const { return m_showPageLimits; }
    void SetShowPageLimits( bool aShow ) { m_showPageLimits = aShow; }

    /**
     * @param doOpen if true runs an Open Library browser, otherwise New Library
     * @param aFilename for New may contain a default name; in both cases return the chosen
     *                  filename.
     * @param wildcard a wildcard to filter the displayed files
     * @param ext the library file extension
     * @param isDirectory indicates the library files are directories
     * @return true for OK; false for Cancel.
     */
    bool LibraryFileBrowser( bool doOpen, wxFileName& aFilename, const wxString& wildcard,
                             const wxString& ext, bool isDirectory = false, bool aIsGlobal = false,
                             const wxString& aGlobalPath = wxEmptyString );

    void CommonSettingsChanged( bool aEnvVarsChanged, bool aTextVarsChanged ) override;

    virtual wxString GetScreenDesc() const;

    /**
     * Return a pointer to a BASE_SCREEN or one of its derivatives.
     *
     * It is overloaded by derived classes to return #SCH_SCREEN or #PCB_SCREEN.
     */
    virtual BASE_SCREEN* GetScreen() const  { return m_currentScreen; }

    /**
     * Execute a remote command sent via socket (to port KICAD_PCB_PORT_SERVICE_NUMBER,
     * currently 4242).
     *
     * Subclasses should override to implement actual command handlers.
     */
    virtual void ExecuteRemoteCommand( const char* cmdline ){}

    void EraseMsgBox();

    void ReCreateMenuBar() override { }
    virtual void ReCreateHToolbar() = 0;
    virtual void ReCreateVToolbar() = 0;
    virtual void ReCreateOptToolbar() = 0;
    virtual void ReCreateAuxiliaryToolbar() { }

    /*
     * These 4 functions provide a basic way to show/hide grid and /get/set grid color.
     *
     * These parameters are saved in KiCad config for each main frame.
     */
    bool IsGridVisible() const;
    virtual void SetGridVisibility( bool aVisible );

    virtual COLOR4D GetGridColor() { return m_gridColor; }
    virtual void SetGridColor( COLOR4D aColor ) { m_gridColor = aColor; }

    /**
     * Command event handler for selecting grid sizes.
     *
     * All commands that set the grid size should eventually end up here. This is where the
     * application setting is saved.  If you override this method, make sure you call down
     * to the base class.
     *
     * @param event Command event from the grid size combobox on the toolbar.
     */
    void OnSelectGrid( wxCommandEvent& event );

    void OnGridSettings( wxCommandEvent& event );

    /**
     * Rebuild the grid combobox to respond to any changes in the GUI (units, user
     * grid changes, etc.).
     */
    void UpdateGridSelectBox();

    /**
     * Update the checked item in the grid combobox.
     */
    void OnUpdateSelectGrid( wxUpdateUIEvent& aEvent );

    /**
     * Rebuild the grid combobox to respond to any changes in the GUI (units, user
     * grid changes, etc.)
     */
    void UpdateZoomSelectBox();

    /**
     * Update the checked item in the zoom combobox.
     */
    void OnUpdateSelectZoom( wxUpdateUIEvent& aEvent );

    /**
     * Return a human readable value for display in dialogs.
     */
    const wxString GetZoomLevelIndicator() const;

    /**
     * Set the zoom factor when selected by the zoom list box in the main tool bar.
     *
     * @note List position 0 is fit to page.
     *       List position >= 1 = zoom (1 to zoom max).
     *       Last list position is custom zoom not in zoom list.
     */
    virtual void OnSelectZoom( wxCommandEvent& event );

    /**
     * Recalculate the size of toolbars and display panel when the frame size changes.
     */
    virtual void OnSize( wxSizeEvent& event );

    void OnMove( wxMoveEvent& aEvent ) override;

    /**
     * Rebuild the GAL and redraws the screen.  Call when something went wrong.
     */
    virtual void HardRedraw();

    /**
     * Redraw the screen with best zoom level and the best centering that shows all the
     * page or the board.
     */
    virtual void Zoom_Automatique( bool aWarpPointer );

    /**
     * Useful to focus on a particular location, in find functions.
     *
     * Move the graphic cursor (crosshair cursor) at a given coordinate and reframes the
     * drawing if the requested point is out of view or if center on location is requested.
     *
     * @param aPos is the point to go to.
     */
    void FocusOnLocation( const wxPoint& aPos );

    /**
     * Construct a "basic" menu for a tool, containing only items that apply to all tools
     * (e.g. zoom and grid).
     */
    void AddStandardSubMenus( TOOL_MENU& aMenu );

    /**
     * Prints the page layout with the frame and the basic inscriptions.
     *
     * @param aScreen screen to draw.
     * @param aMils2Iu The mils to Iu conversion factor.
     * @param aFilename The filename to display in basic inscriptions.
     * @param aSheetLayer The layer displayed from PcbNew.
     */
    void PrintWorkSheet( const RENDER_SETTINGS* aSettings, BASE_SCREEN* aScreen, double aMils2Iu,
                         const wxString& aFilename, const wxString& aSheetLayer = wxEmptyString );

    void DisplayToolMsg( const wxString& msg ) override;

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
     * Display current unit pane in the status bar.
     */
    void DisplayUnitsMsg();

    /**
     * Display current grid size in the status bar.
     */
    virtual void DisplayGridMsg();

    /* interprocess communication */
    void CreateServer( int service, bool local = true );
    void OnSockRequest( wxSocketEvent& evt );
    void OnSockRequestServer( wxSocketEvent& evt );

    void LoadSettings( APP_SETTINGS_BASE* aCfg ) override;
    void SaveSettings( APP_SETTINGS_BASE* aCfg ) override;

    /**
     * Append a message to the message panel.
     *
     * This helper method checks to make sure the message panel exists in the frame and
     * appends a message to it using the message panel AppendMessage() method.
     *
     * @param aTextUpper The message upper text.
     * @param aTextLower The message lower text.
     * @param aPadding Number of spaces to pad between messages.
     */
    void AppendMsgPanel( const wxString& aTextUpper, const wxString& aTextLower, int aPadding = 6 );

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
     * Helper function that erases the msg panel and then appends a single message
     *
     * @param aTextUpper The message upper text.
     * @param aTextLower The message lower text.
     * @param aPadding Number of spaces to pad between messages.
     */
    void SetMsgPanel( const wxString& aTextUpper, const wxString& aTextLower, int aPadding = 6 );

    /**
     * Redraw the message panel.
     */
    virtual void UpdateMsgPanel();

    /**
     * Fetch an item by KIID.  Frame-type-specific implementation.
     */
    virtual EDA_ITEM* GetItem( const KIID& aId ) { return nullptr; }

    /**
     * Print the page pointed by current screen, set by the calling print function.
     *
     * @param aDC wxDC given by the calling print function
     */
    virtual void PrintPage( const RENDER_SETTINGS* aSettings );

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
    virtual EDA_DRAW_PANEL_GAL* GetCanvas() const { return m_canvas; }
    void SetCanvas( EDA_DRAW_PANEL_GAL* aPanel ) { m_canvas = aPanel; }

    wxWindow* GetToolCanvas() const override { return GetCanvas(); }

    /**
     * Return a reference to the gal rendering options used by GAL for rendering.
     */
    KIGFX::GAL_DISPLAY_OPTIONS& GetGalDisplayOptions() { return m_galDisplayOptions; }

    void RefreshCanvas() override
    {
        GetCanvas()->Refresh();
    }

    /**
     * Returns bbox of document with option to not include some items.
     *
     * Used most commonly by "Zoom to Fit" and "Zoom to Objects".  In Eeschema
     * for "Zoom to Fit", it's passed "true" to include worksheet border.  It's
     * passed false by "Zoom To Objects" to ignore worksheet border.  In Pcbnew,
     * false makes it ignore any items outside the PCB edge such as fabrication
     * notes.
     *
     * @param aIncludeAllVisible True to include everything visible in bbox calculations,
     *                           false to ignore some visible items (program dependent).
     * @return Bounding box of the document (ignoring some items as requested).
     */
    virtual const BOX2I GetDocumentExtents( bool aIncludeAllVisible = true ) const;

    /**
     * Rebuild all toolbars, and update the checked state of check tools
     */
    void RecreateToolbars();

    DECLARE_EVENT_TABLE()

protected:
    virtual void SetScreen( BASE_SCREEN* aScreen )  { m_currentScreen = aScreen; }

    void unitsChangeRefresh() override;

    void setupUnits( APP_SETTINGS_BASE* aCfg );

    /**
     * Determines the Canvas type to load (with prompt if required) and initializes m_canvasType
     */
    void resolveCanvasType();

    /**
     * Returns the canvas type stored in the application settings.
     */
    EDA_DRAW_PANEL_GAL::GAL_TYPE loadCanvasTypeSetting();

    /**
     * Stores the canvas type in the application settings.
     */
    bool saveCanvasTypeSetting( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType );

    wxSocketServer*             m_socketServer;
    std::vector<wxSocketBase*>  m_sockets;         ///< interprocess communication

    ///< Prevents opening same file multiple times.
    std::unique_ptr<wxSingleInstanceChecker> m_file_checker;

    bool               m_showPageLimits;    // True to display the page limits
    COLOR4D            m_gridColor;         // Grid color
    COLOR4D            m_drawBgColor;       // The background color of the draw canvas; BLACK for
                                            // Pcbnew, BLACK or WHITE for Eeschema
    int                m_undoRedoCountMax;  // Default Undo/Redo command Max depth, to be handed
                                            // to screens
    bool               m_polarCoords;       // For those frames that support polar coordinates

    bool               m_showBorderAndTitleBlock;  // Show the worksheet (border and title block).
    long               m_firstRunDialogSetting;    // Show first run dialog on startup

    wxChoice*          m_gridSelectBox;
    wxChoice*          m_zoomSelectBox;

    ACTION_TOOLBAR*    m_mainToolBar;
    ACTION_TOOLBAR*    m_auxiliaryToolBar;  // Additional tools under main toolbar
    ACTION_TOOLBAR*    m_drawToolBar;       // Drawing tools (typically on right edge of window)
    ACTION_TOOLBAR*    m_optionsToolBar;    // Options (typically on left edge of window)

    wxFindReplaceData* m_findReplaceData;
    wxArrayString      m_findStringHistoryList;
    wxArrayString      m_replaceStringHistoryList;

    EDA_MSG_PANEL*     m_messagePanel;
    int                m_msgFrameHeight;

    COLOR_SETTINGS*    m_colorSettings;

    ///< The current canvas type.
    EDA_DRAW_PANEL_GAL::GAL_TYPE    m_canvasType;

private:
    BASE_SCREEN*                m_currentScreen;      ///< current used SCREEN
    EDA_DRAW_PANEL_GAL*         m_canvas;

    ///< This the frame's interface to setting GAL display options.
    KIGFX::GAL_DISPLAY_OPTIONS  m_galDisplayOptions;

    ///< Default display origin transforms object.
    ORIGIN_TRANSFORMS           m_originTransforms;
};

#endif  // DRAW_FRAME_H_
