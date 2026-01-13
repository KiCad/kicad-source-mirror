/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <api/api_plugin.h>
#include <eda_base_frame.h>
#include <eda_search_data.h>
#include <kiway_player.h>
#include <gal/gal_display_options.h>
#include <gal_display_options_common.h>
#include <gal/color4d.h>
#include <class_draw_panel_gal.h>
#include <kiid.h>
#include <hotkeys_basic.h>
#include <widgets/lib_tree.h>

class EDA_ITEM;
class wxSingleInstanceChecker;
class ACTION_TOOLBAR;
class GRID_HELPER;
class COLOR_SETTINGS;
class LOCKFILE;
class TOOL_MENU;
class APP_SETTINGS_BASE;
class wxFindReplaceData;
class SEARCH_PANE;
class HOTKEY_CYCLE_POPUP;
class PROPERTIES_PANEL;
class NET_INSPECTOR_PANEL;
enum class BITMAP_TYPE;
class FILEDLG_HOOK_NEW_LIBRARY;

namespace KIGFX
{
    class GAL_DISPLAY_OPTIONS;
    class RENDER_SETTINGS;
}

using KIGFX::COLOR4D;
using KIGFX::RENDER_SETTINGS;

#define LIB_EDIT_FRAME_NAME           wxT( "LibeditFrame" )
#define LIB_VIEW_FRAME_NAME           wxT( "ViewlibFrame" )
#define SCH_EDIT_FRAME_NAME           wxT( "SchematicFrame" )
#define SYMBOL_CHOOSER_FRAME_NAME     wxT( "SymbolChooserFrame" )
#define PL_EDITOR_FRAME_NAME          wxT( "PlEditorFrame" )
#define FOOTPRINT_WIZARD_FRAME_NAME   wxT( "FootprintWizard" )
#define FOOTPRINT_CHOOSER_FRAME_NAME  wxT( "FootprintChooserFrame" )
#define FOOTPRINT_EDIT_FRAME_NAME     wxT( "ModEditFrame" )
#define FOOTPRINT_VIEWER_FRAME_NAME   wxT( "ModViewFrame" )
#define PCB_EDIT_FRAME_NAME           wxT( "PcbFrame" )


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
                    const wxString& aFrameName, const EDA_IU_SCALE& aIuScale );

    ~EDA_DRAW_FRAME();

    /**
     * Mark a schematic file as being in use.
     *
     * Use #ReleaseFile() to undo this.
     *
     * @param aFileName full path to the file.
     * @return true if the file is locked or read-only, false otherwise.
     */
    bool LockFile( const wxString& aFileName );

    /**
     * Release the current file marked in use.  See m_file_checker.
     */
    void ReleaseFile();

    /**
     * Toggle the scripting console visibility.
     */
    void ScriptingConsoleEnableDisable();

    /**
     * Get the current visibility of the scripting console window.
     */
    bool IsScriptingConsoleVisible();

    EDA_SEARCH_DATA& GetFindReplaceData() { return *m_findReplaceData; }
    wxArrayString& GetFindHistoryList() { return m_findStringHistoryList; }

    virtual void SetPageSettings( const PAGE_INFO& aPageSettings ) = 0;
    virtual const PAGE_INFO& GetPageSettings() const = 0;

    /**
     * Works off of GetPageSettings() to return the size of the paper page in
     * the internal units of this particular view.
     */
    virtual const VECTOR2I GetPageSizeIU() const = 0;

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
    void GetUnitPair( EDA_UNITS& aPrimaryUnit, EDA_UNITS& aSecondaryUnits ) override;

    /**
     * Return the absolute coordinates of the origin of the snap grid.
     *
     * This is treated as a relative offset and snapping will occur at multiples of the grid
     * size relative to this point.
     */
    virtual const VECTOR2I& GetGridOrigin() const = 0;
    virtual void            SetGridOrigin( const VECTOR2I& aPosition ) = 0;

    virtual std::unique_ptr<GRID_HELPER> MakeGridHelper();

    /**
     * Return the nearest \a aGridSize location to \a aPosition.
     *
     * @param aPosition The position to check.
     * @return The nearest grid position.
     */
    VECTOR2I GetNearestGridPosition( const VECTOR2I& aPosition ) const;

    /**
     * Return the nearest \a aGridSize / 2 location to \a aPosition.
     *
     * This is useful when attempting for keep outer points on grid but
     * not the middle point.
     *
     * @param aPosition The position to check.
     * @return The nearest half-grid position.
     */
    VECTOR2I GetNearestHalfGridPosition( const VECTOR2I& aPosition ) const;

    virtual const TITLE_BLOCK& GetTitleBlock() const = 0;
    virtual void SetTitleBlock( const TITLE_BLOCK& aTitleBlock ) = 0;

    // the background color of the draw canvas:
    // Virtual because some frames can have a specific way to get/set the bg color
    virtual COLOR4D GetDrawBgColor() const { return m_drawBgColor; }
    virtual void SetDrawBgColor( const COLOR4D& aColor) { m_drawBgColor= aColor ; }

    /// Returns a pointer to the active color theme settings
    virtual COLOR_SETTINGS* GetColorSettings( bool aForceRefresh = false ) const;

    /**
     * @param aTitle dialog title
     * @param doOpen if true runs an Open Library browser, otherwise New Library
     * @param aFilename for New may contain a default name; in both cases return the chosen
     *                  filename.
     * @param wildcard a wildcard to filter the displayed files
     * @param ext the library file extension
     * @param isDirectory indicates the library files are directories
     * @param aFileDlgHook optional; adds customized controls to dialog
     * @return true for OK; false for Cancel.
     */
    bool LibraryFileBrowser( const wxString& aTitle, bool doOpen, wxFileName& aFilename,
                             const wxString& wildcard, const wxString& ext, bool isDirectory,
                             FILEDLG_HOOK_NEW_LIBRARY* aFileDlgHook = nullptr );

    void CommonSettingsChanged( int aFlags ) override;

    virtual wxString GetScreenDesc() const;
    virtual wxString GetFullScreenDesc() const;

    /**
     * Return a pointer to a BASE_SCREEN or one of its derivatives.
     *
     * It is overloaded by derived classes to return #SCH_SCREEN or #PCB_SCREEN.
     */
    virtual BASE_SCREEN* GetScreen() const  { return m_currentScreen; }

    void EraseMsgBox();

    // Toolbar-related functions
    virtual void ReCreateHToolbar() { };
    virtual void ReCreateVToolbar() { };
    virtual void ReCreateLeftToolbar() { };
    virtual void ReCreateAuxiliaryToolbar() { }

    /*
     * These 4 functions provide a basic way to show/hide grid and /get/set grid color.
     *
     * These parameters are saved in KiCad config for each main frame.
     */
    bool IsGridVisible();
    virtual void SetGridVisibility( bool aVisible );

    bool         IsGridOverridden();
    virtual void SetGridOverrides( bool aOverride );

    virtual COLOR4D GetGridColor() { return m_gridColor; }
    virtual void SetGridColor( const COLOR4D& aColor ) { m_gridColor = aColor; }

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

    /**
     * Rebuild the grid combobox to respond to any changes in the GUI (units, user
     * grid changes, etc.).
     */
    void UpdateGridSelectBox();

    /**
     * Update the checked item in the grid wxchoice.
     */
    void OnUpdateSelectGrid( wxUpdateUIEvent& aEvent );

    /**
     * Update the checked item in the zoom wxchoice.
     */
    void OnUpdateSelectZoom( wxUpdateUIEvent& aEvent );

    /**
     * Rebuild the grid combobox to respond to any changes in the GUI (units, user
     * grid changes, etc.)
     */
    void UpdateZoomSelectBox();

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
    virtual void OnSize( wxSizeEvent& event ) override;

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
    void FocusOnLocation( const VECTOR2I& aPos, bool aAllowScroll = true );

    /**
     * Focus on a particular canvas item.
     *
     * @param aItem is the item to focus on. nullptr clears the focus.
     */
    virtual void FocusOnItem( EDA_ITEM* aItem, bool aAllowScroll = true ) {}

    virtual void ClearFocus() { FocusOnItem( nullptr ); }

    /**
     * Construct a "basic" menu for a tool, containing only items that apply to all tools
     * (e.g. zoom and grid).
     */
    void AddStandardSubMenus( TOOL_MENU& aMenu );

    /**
     * Print the drawing-sheet (frame and title block).
     *
     * @param aScreen screen to draw.
     * @param aProperties Optional properties for text variable resolution.
     * @param aMils2Iu The mils to Iu conversion factor.
     * @param aFilename The filename to display in basic inscriptions.
     * @param aSheetLayer The layer displayed from PcbNew.
     */
    void PrintDrawingSheet( const RENDER_SETTINGS* aSettings, BASE_SCREEN* aScreen,
                            const std::map<wxString, wxString>* aProperties, double aMils2Iu,
                            const wxString& aFilename,
                            const wxString& aSheetLayer = wxEmptyString );

    void DisplayToolMsg( const wxString& msg ) override;

    void DisplayConstraintsMsg( const wxString& msg );

    /**
     * Called when modifying the page settings.
     *
     * In derived classes it can be used to modify parameters like draw area size,
     * and any other local parameter related to the page settings.
     */
    virtual void OnPageSettingsChange() {}

    /** Create the status line (like a wxStatusBar). This is actually a KISTATUSBAR status bar.
     * the specified number of fields is the extra number of fields, not the full field count.
     * @return a KISTATUSBAR (derived from wxStatusBar)
     */
    wxStatusBar* OnCreateStatusBar( int number, long style, wxWindowID id,
                                    const wxString& name ) override;

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

    bool GetOverrideLocks() const;

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

    virtual LIB_TREE* GetLibTree() const { return nullptr; }
    virtual LIB_ID GetTargetLibId() const { return LIB_ID(); }

    virtual bool IsLibraryTreeShown() const { return false; }
    virtual void ToggleLibraryTree() {};
    virtual void FocusLibraryTreeInput() {};

    PROPERTIES_PANEL* GetPropertiesPanel() { return m_propertiesPanel; }

    void UpdateProperties();

    virtual void ToggleProperties() {}

    static const wxString PropertiesPaneName() { return wxS( "PropertiesManager" ); }

    static const wxString NetInspectorPanelName() { return wxS( "NetInspector" ); }

    static const wxString DesignBlocksPaneName() { return wxS( "DesignBlocks" ); }

    static const wxString RemoteSymbolPaneName() { return wxS( "RemoteSymbol" ); }

    static const wxString AppearancePanelName() { return wxS( "LayersManager" ); }

    /**
     * Fetch an item by KIID.  Frame-type-specific implementation.
     */
    virtual EDA_ITEM* ResolveItem( const KIID& aId, bool aAllowNullptrReturn = false ) const
    {
        return nullptr;
    }

    /**
     * Use to start up the GAL drawing canvas.
     */
    virtual void ActivateGalCanvas();

    /**
     * Change the current rendering backend.
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

    void ClearToolbarControl( int aId ) override;

    /**
     * Return a reference to the gal rendering options used by GAL for rendering.
     */
    GAL_DISPLAY_OPTIONS_IMPL& GetGalDisplayOptions() { return m_galDisplayOptions; }

    void RefreshCanvas() override
    {
        GetCanvas()->Refresh();
    }

    /**
     * Return bounding box of document with option to not include some items.
     *
     * Used most commonly by "Zoom to Fit" and "Zoom to Objects".  In Eeschema for "Zoom to Fit"
     * it's passed "true" to include drawing sheet border, and "false" by "Zoom To Objects" to
     * ignore drawing sheet border.  In Pcbnew, false makes it ignore any items outside the PCB
     * edge such as fabrication notes.
     *
     * @param aIncludeAllVisible True to include everything visible in bbox calculations,
     *                           false to ignore some visible items (program dependent).
     * @return Bounding box of the document (ignoring some items as requested).
     */
    virtual const BOX2I GetDocumentExtents( bool aIncludeAllVisible = true ) const;

    /**
     * Redraw the menus and what not in current language.
     */
    void ShowChangedLanguage() override;

    HOTKEY_CYCLE_POPUP* GetHotkeyPopup() { return m_hotkeyPopup; }

    virtual void CreateHotkeyPopup();


    /**
     * Save the current view as an image file.
     *
     * @param aFrame The current draw frame view to save.
     * @param aFileName The file name to save the image.  This will overwrite an existing file.
     * @param aBitmapType The type of bitmap create as defined by wxImage.
     * @return True if the file was successfully saved or false if the file failed to be saved.
     */
    bool SaveCanvasImageToFile( const wxString& aFileName, BITMAP_TYPE aBitmapType );

    /**
     * Handler for activating an API plugin (via toolbar or menu).
     */
    virtual void OnApiPluginInvoke( wxCommandEvent& aEvent );

    virtual PLUGIN_ACTION_SCOPE PluginActionScope() const { return PLUGIN_ACTION_SCOPE::INVALID; }

    static bool IsPluginActionButtonVisible( const PLUGIN_ACTION& aAction,
                                             APP_SETTINGS_BASE* aCfg );

    /**
     * Return ordered list of plugin actions for display in the toolbar.
     *
     * Must be static at the moment because this needs to be called from the preferences dialog,
     * which can exist without the frame in question actually being created.
     *
     * @param aCfg is the settings to read the plugin ordering from.
     */
    static std::vector<const PLUGIN_ACTION*> GetOrderedPluginActions( PLUGIN_ACTION_SCOPE aScope,
                                                                      APP_SETTINGS_BASE* aCfg );

    /**
     * Append actions from API plugins to the given toolbar.
     *
     * @param aToolbar is the toolbar to add the plugins to
     */
    virtual void AddApiPluginTools( ACTION_TOOLBAR* aToolbar );

    DECLARE_EVENT_TABLE()

protected:
    void configureToolbars() override;

    virtual void SetScreen( BASE_SCREEN* aScreen )  { m_currentScreen = aScreen; }

    void unitsChangeRefresh() override;

    void setupUIConditions() override;

    void setupUnits( APP_SETTINGS_BASE* aCfg );

    void updateStatusBarWidths();

    std::vector<wxWindow*> findDialogs();

    /**
     * Determine the canvas type to load (with prompt if required) and initializes #m_canvasType.
     */
    virtual void resolveCanvasType();

    /**
     * Return the canvas type stored in the application settings.
     *
     * @param aCfg is the APP_SETTINGS_BASE config storing the canvas type.
     * If nullptr (default) the KifaceSettings() will be used
     */
    EDA_DRAW_PANEL_GAL::GAL_TYPE loadCanvasTypeSetting();

    /**
     * Store the canvas type in the application settings.
     */
    bool saveCanvasTypeSetting( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType );

    /**
     * Handle a window activation event.
     */
    virtual void handleActivateEvent( wxActivateEvent& aEvent );
    void onActivate( wxActivateEvent& aEvent );

    wxSocketServer*             m_socketServer;

    ///< Prevents opening same file multiple times.
    std::unique_ptr<LOCKFILE> m_file_checker;

    COLOR4D              m_gridColor;         // Grid color
    COLOR4D              m_drawBgColor;       // The background color of the draw canvas; BLACK for
                                              // Pcbnew, BLACK or WHITE for Eeschema
    int                  m_undoRedoCountMax;  // Default Undo/Redo command Max depth, to be handed
                                              // to screens
    bool                 m_polarCoords;       // For those frames that support polar coordinates

    // Show the drawing sheet (border & title block).
    bool                 m_showBorderAndTitleBlock;

    wxChoice*            m_gridSelectBox;
    wxChoice*            m_zoomSelectBox;
    wxCheckBox*          m_overrideLocksCb;

    std::unique_ptr<EDA_SEARCH_DATA> m_findReplaceData;
    wxArrayString        m_findStringHistoryList;
    wxArrayString        m_replaceStringHistoryList;

    EDA_MSG_PANEL*       m_messagePanel;
    int                  m_msgFrameHeight;

    COLOR_SETTINGS*      m_colorSettings;
    SEARCH_PANE*         m_searchPane;
    PROPERTIES_PANEL*    m_propertiesPanel;
    NET_INSPECTOR_PANEL* m_netInspectorPanel;

    HOTKEY_CYCLE_POPUP* m_hotkeyPopup;

    /// The current canvas type.
    EDA_DRAW_PANEL_GAL::GAL_TYPE    m_canvasType;

    static bool m_openGLFailureOccured; ///< Has any failure occurred when switching to OpenGL in
                                        ///< any EDA_DRAW_FRAME?

private:
    BASE_SCREEN*                m_currentScreen;      ///< current used SCREEN
    EDA_DRAW_PANEL_GAL*         m_canvas;

    /// This the frame's interface to setting GAL display options.
    GAL_DISPLAY_OPTIONS_IMPL  m_galDisplayOptions;

    int m_lastToolbarIconSize;
};

#endif  // DRAW_FRAME_H_
