/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Jean-Pierre Charras, jp.charras@wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef  __PCB_EDIT_FRAME_H__
#define  __PCB_EDIT_FRAME_H__

#include "pcb_base_edit_frame.h"
#include "zones.h"
#include <mail_type.h>
#include <settings/app_settings.h>
#include <variant>

class ACTION_PLUGIN;
class PCB_SCREEN;
class BOARD;
class BOARD_COMMIT;
class BOARD_ITEM_CONTAINER;
class DESIGN_BLOCK;
class DIALOG_BOOK_REPORTER;
class FOOTPRINT;
class PCB_TRACK;
class PCB_VIA;
class PAD;
class PCB_SELECTION;
class PCB_TARGET;
class PCB_GROUP;
class PCB_DIMENSION_BASE;
class DRC;
class DIALOG_FIND;
class DIALOG_PLOT;
class ZONE;
class GENERAL_COLLECTOR;
class GENERAL_COLLECTORS_GUIDE;
class SELECTION;
class PCB_MARKER;
class BOARD_ITEM;
class NETLIST;
class REPORTER;
struct PARSE_ERROR;
class IO_ERROR;
class FP_LIB_TABLE;
class BOARD_NETLIST_UPDATER;
class ACTION_MENU;
class TOOL_ACTION;
class DIALOG_BOARD_SETUP;
class PCB_DESIGN_BLOCK_PANE;

#ifdef KICAD_IPC_API
class KICAD_API_SERVER;
class API_HANDLER_PCB;
class API_HANDLER_COMMON;
#endif

enum LAST_PATH_TYPE : unsigned int;

namespace PCB { struct IFACE; }     // KIFACE is in pcbnew.cpp

/**
 * The main frame for Pcbnew.
 *
 * See also class PCB_BASE_FRAME(): Basic class for Pcbnew and GerbView.
 */
class PCB_EDIT_FRAME : public PCB_BASE_EDIT_FRAME
{
public:
    virtual ~PCB_EDIT_FRAME();

    /**
     * Load the footprints for each #SCH_COMPONENT in \a aNetlist from the list of libraries.
     *
     * @param aNetlist is the netlist of components to load the footprints into.
     * @param aReporter is the #REPORTER object to report to.
     * @throw IO_ERROR if an I/O error occurs or a #PARSE_ERROR if a file parsing error
     *           occurs while reading footprint library files.
     */
    void LoadFootprints( NETLIST& aNetlist, REPORTER& aReporter );

    void OnQuit( wxCommandEvent& event );

    /**
     * Get if the current board has been modified but not saved.
     *
     * @return true if the any changes have not been saved
     */
    bool IsContentModified() const override;

    /**
     * Synchronize the environment variables from KiCad's environment into the Python interpreter.
     */
    void PythonSyncEnvironmentVariables();

    /**
     * Synchronize the project name from KiCad's environment into the Python interpreter.
     */
    void PythonSyncProjectName();

    /**
     * Update the layer manager and other widgets from the board setup
     * (layer and items visibility, colors ...)
     */
    void UpdateUserInterface();

    void HardRedraw() override;

    /**
     * Rebuilds board connectivity, refreshes canvas.
     */
    void RebuildAndRefresh();

    /**
     * Execute a remote command send by Eeschema via a socket, port KICAD_PCB_PORT_SERVICE_NUMBER
     * (currently 4242).
     *
     * This is a virtual function called by EDA_DRAW_FRAME::OnSockRequest().
     *
     * @param cmdline is the received command from socket.
     */
    void ExecuteRemoteCommand( const char* cmdline ) override;

    void KiwayMailIn( KIWAY_EXPRESS& aEvent ) override;

    /**
     * Used to find items by selection synchronization spec string.
     */
    std::vector<BOARD_ITEM*> FindItemsFromSyncSelection( std::string syncStr );

    /**
     * @return the name of the wxAuiPaneInfo managing the Search panel
     */
    static const wxString SearchPaneName() { return wxT( "Search" ); }

    /**
     * Show the Find dialog.
     */
    void ShowFindDialog();

    /**
     * Find the next item using our existing search parameters.
     */
    void FindNext( bool reverse = false );

    bool LayerManagerShown();
    bool PropertiesShown();
    bool NetInspectorShown();

    void OnUpdateSelectViaSize( wxUpdateUIEvent& aEvent );
    void OnUpdateSelectTrackWidth( wxUpdateUIEvent& aEvent );

    void UpdateTrackWidthSelectBox( wxChoice* aTrackWidthSelectBox, bool aShowNetclass,
                                    bool aShowEdit );
    void UpdateViaSizeSelectBox( wxChoice* aViaSizeSelectBox, bool aShowNetclass, bool aShowEdit );

    /**
     * Update the variant selection dropdown with the current board's variant names.
     *
     * If the currently selected variant is no longer available, the default (no variant)
     * will be selected.
     */
    void UpdateVariantSelectionCtrl();

    /**
     * Event handler for variant selection changes in the toolbar.
     */
    void onVariantSelected( wxCommandEvent& aEvent );

    /**
     * Return the angle used for rotate operations.
     */
    EDA_ANGLE GetRotationAngle() const override;

    /**
     * @return the color of the grid
     */
    COLOR4D GetGridColor() override;

    /**
     * @param[in] aColor the new color of the grid.
     */
    void SetGridColor( const COLOR4D& aColor ) override;

    /**
     * Return true if button visibility action plugin setting was set to true
     * or it is unset and plugin defaults to true.
     */
    static bool GetActionPluginButtonVisible( const wxString& aPluginPath, bool aPluginDefault );

    /**
     * Return ordered list of plugins in sequence in which they should appear on toolbar or
     * in settings.  Handles both legacy (SWIG) and API plugins, so returns a heterogenous list.
     */
    static std::vector<std::variant<ACTION_PLUGIN*, const PLUGIN_ACTION*>> GetOrderedActionPlugins();

    void SaveProjectLocalSettings() override;

    /**
     * Load the current project's file configuration settings which are pertinent
     * to this PCB_EDIT_FRAME instance.
     *
     * @return always returns true.
     */
    bool LoadProjectSettings();

    void LoadSettings( APP_SETTINGS_BASE* aCfg ) override;

    void SaveSettings( APP_SETTINGS_BASE* aCfg ) override;

    /**
     * Load the drawing sheet file.
     */
    void LoadDrawingSheet();

    /**
     * Get the last path for a particular type.
     *
     * @return the absolute path and file name of the last file successfully read.
     */
    wxString GetLastPath( LAST_PATH_TYPE aType );

    /**
     * Set the path of the last file successfully read.
     *
     * @note the file path is converted to a path relative to the project file path.  If
     *       the path cannot be made relative, than m_lastNetListRead is set to and empty
     *       string.  This could happen when the net list file is on a different drive than
     *       the project file.  The advantage of relative paths is that is more likely to
     *       work when opening the same project from both Windows and Linux.
     *
     * @param aLastPath - The last file with full path successfully read.
     */
    void SetLastPath( LAST_PATH_TYPE aType, const wxString& aLastPath );

    /**
     * If aCreateMarkers then create DRC exclusion markers from the serialized data.  If false,
     * then use the serialized data to set exclusion flags on existing markers.
     */
    void ResolveDRCExclusions( bool aCreateMarkers );

    void Process_Special_Functions( wxCommandEvent& event );
    void Tracks_and_Vias_Size_Event( wxCommandEvent& event );



    /**
     * Recreate the layer box by clearing the old list and building a new one from the new
     * layer names and colors.
     *
     * @param aForceResizeToolbar true to resize the parent toolbar false if not needed (mainly
     *                            in parent toolbar creation, or when the layers names are not
     *                            modified)
     */
    void ReCreateLayerBox( bool aForceResizeToolbar = true );


    /**
     * Must be called after a board change to set the modified flag.
     *
     * Reload the 3D view if required and calls the base PCB_BASE_FRAME::OnModify function
     * to update auxiliary information.
     */
    void OnModify() override;

    /**
     * Change the currently active layer to \a aLayer and also update the #APPEARANCE_CONTROLS.
     */
    void SetActiveLayer( PCB_LAYER_ID aLayer ) override
    {
        SetActiveLayer( aLayer, false );
    }

    /**
     * @param aLayer is the layer to set active
     * @param aForceRedraw will repaint things that depend on layer switch even if the new active
     *                     layer is the same as the previous one
     */
    void SetActiveLayer( PCB_LAYER_ID aLayer, bool aForceRedraw );

    void OnDisplayOptionsChanged() override;

    /**
     * Test whether a given element category is visible. Keep this as an inline function.
     *
     * @param aElement is from the enum by the same name
     * @return bool - true if the element is visible.
     * @see enum GAL_LAYER_ID
     */
    bool IsElementVisible( GAL_LAYER_ID aElement ) const;

    /**
     * Change the visibility of an element category.
     *
     * @param aElement is from the enum by the same name.
     * @param aNewState The new visibility state of the element category.
     * @see enum PCB_LAYER_ID.
     */
    void SetElementVisibility( GAL_LAYER_ID aElement, bool aNewState );

    ///< @copydoc EDA_DRAW_FRAME::UseGalCanvas()
    void ActivateGalCanvas() override;

    void ShowBoardSetupDialog( const wxString& aInitialPage = wxEmptyString, wxWindow* aParent = nullptr );

    void PrepareLayerIndicator( bool aForceRebuild = false );

    void ToggleLayersManager();

    void ToggleNetInspector();

    void ToggleSearch();

    bool IsSearchPaneShown() { return m_auimgr.GetPane( SearchPaneName() ).IsShown(); }
    void FocusSearch();

    void ToggleLibraryTree() override;

    /**
     * Create an ASCII footprint position file.
     *
     * @param aFullFileName the full file name of the file to create.
     * @param aUnitsMM false to use inches, true to use mm in coordinates.
     * @param aOnlySMD true to force only footprints flagged smd to be in the list
     * @param aNoTHItems true to include only footprints with no TH pads no matter
     *                   the footprint flag
     * @param aExcludeDNP true to exclude footprints flagged DNP
     * @param aExcludeBOM true to exclude footprints flagged exclude from BOM
     * @param aTopSide true to list footprints on front (top) side.
     * @param aBottomSide true to list footprints on back (bottom) side, if \a aTopSide and
     *                    \a aTopSide are true, list footprints on both sides.
     * @param aFormatCSV true to use a comma separated file (CSV) format; default = false
     * @param aUseAuxOrigin true to use auxiliary axis as an origin for the position data
     * @param aNegateBottomX true to negate X coordinates for bottom side of the placement file
     * @return the number of footprints found on aSide side or -1 if the file could not be created.
     */
    int DoGenFootprintsPositionFile( const wxString& aFullFileName, bool aUnitsMM, bool aOnlySMD,
                                     bool aNoTHItems, bool aExcludeDNP, bool aExcludeBOM, bool aTopSide,
                                     bool aBottomSide, bool aFormatCSV, bool aUseAuxOrigin,
                                     bool aNegateBottomX );

    void OnFileHistory( wxCommandEvent& event );
    void OnClearFileHistory( wxCommandEvent& aEvent );

    bool SaveBoard( bool aSaveAs = false, bool aSaveCopy = false );

    /**
     * Load a KiCad board (.kicad_pcb) from \a aFileName.
     *
     * @param aFileSet hold the BOARD file to load, a vector of one element.
     * @param aCtl KICTL_ bits, one to indicate that an append of the board file
     *             \a aFileName to the currently loaded file is desired.
     *             @see #KIWAY_PLAYER for bit defines.
     *
     * @return false if file load fails, otherwise true.
    bool LoadOnePcbFile( const wxString& aFileName, bool aAppend = false,
                         bool aForceFileDialog = false );
     */
    bool OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl = 0 ) override;

    /**
     * Write the board data structures to \a a aFileName.
     *
     * Create a backup when requested and update flags (modified and saved flags).
     *
     * @param aFileName The file name to write or wxEmptyString to prompt user for
     *                  file name.
     * @param addToHistory controls whether or not to add the saved file to the recent file list
     * @param aChangeProject is true if the project should be changed to the new board filename
     * @return True if file was saved successfully.
     */
    bool SavePcbFile( const wxString& aFileName, bool addToHistory = true,
                      bool aChangeProject = true );

    /**
     * Write the board data structures to \a aFileName.
     *
     * Unlike SavePcbFile, does not make anything else (no backup, broad filename change, no
     * flag changes ...).  Used under a project mgr to save under a new name the current board.
     * When not under a project mgr, the full SavePcbFile is used.
     *
     * @param aFileName The file name to write.
     * @param aCreateProject will create an empty project alongside the board file
     * @param aHeadless will suppress informational output (e.g. to be used from the API)
     * @return True if file was saved successfully.
     */
    bool SavePcbCopy( const wxString& aFileName, bool aCreateProject = false,
                      bool aHeadless = false );

    /**
     * Delete all and reinitialize the current board.
     *
     * @param doAskAboutUnsavedChanges true to prompt user if existing board contains unsaved
     *                                 changes, false to re-initialize silently.
     * @param aFinal if true, we are clearing the board to exit, so don't run more events.
     */
    bool Clear_Pcb( bool doAskAboutUnsavedChanges, bool aFinal = false );

    ///< @copydoc PCB_BASE_FRAME::SetBoard()
    void SetBoard( BOARD* aBoard, PROGRESS_REPORTER* aReporter = nullptr ) override
    {
        SetBoard( aBoard, true, aReporter );
    }

    void SetBoard( BOARD* aBoard, bool aBuildConnectivity, PROGRESS_REPORTER* aReporter = nullptr );

    ///< @copydoc PCB_BASE_FRAME::GetModel()
    BOARD_ITEM_CONTAINER* GetModel() const override;

    std::unique_ptr<GRID_HELPER> MakeGridHelper() override;

    ///< @copydoc PCB_BASE_FRAME::SetPageSettings()
    void SetPageSettings( const PAGE_INFO& aPageSettings ) override;

    bool SaveBoardAsDesignBlock( const wxString& aLibraryName );

    bool SaveSelectionAsDesignBlock( const wxString& aLibraryName );

    bool UpdateDesignBlockFromBoard( const LIB_ID& aLibId );

    bool UpdateDesignBlockFromSelection( const LIB_ID& aLibId );

    PCB_DESIGN_BLOCK_PANE* GetDesignBlockPane() const { return m_designBlocksPane; }

    /**
     * Save footprints in a library:
     *
     * @param aStoreInNewLib true to save footprints in a existing library. Existing footprints
     *                       will be kept or updated.  This library should be in fp lib table,
     *                       and is type is .pretty. False to save footprints in a new library.
     *                       If it is an existing lib, previous footprints will be removed.
     *
     * @param aLibName optional library name to create, stops dialog call. Must be called with
     *                 \a aStoreInNewLib as true.
     */
    void ExportFootprintsToLibrary( bool aStoreInNewLib, const wxString& aLibName = wxEmptyString,
                                    wxString* aLibPath = nullptr );

    /**
     * Create the file(s) exporting current BOARD to a VRML file.
     *
     * @note When copying 3D shapes files, the new filename is build from the full path
     *       name, changing the separators by underscore.  This is needed because files
     *       with the same shortname can exist in different directories
     * @note ExportVRML_File generates coordinates in board units (BIU) inside the file.
     * @todo Use mm inside the file.  A general scale transform is applied to the whole
     *       file (1.0 to have the actual WRML unit im mm, 0.001 to have the actual WRML
     *       unit in meters.
     * @note For 3D models built by a 3D modeler, the unit is 0,1 inches.  A specific scale
     *       is applied to 3D models to convert them to internal units.
     *
     * @param aFullFileName the full filename of the file to create
     * @param aMMtoWRMLunit the VRML scaling factor: 1.0 to export in mm. 0.001 for meters
     * @param aExport3DFiles true to copy 3D shapes in the subir a3D_Subdir
     * @param aUseRelativePaths set to true to use relative paths instead of absolute paths
     *                          in the board VRML file URLs.
     * @param a3D_Subdir sub directory where 3D shapes files are copied.  This is only used
     *                   when aExport3DFiles == true.
     * @param aXRef X value of PCB (0,0) reference point.
     * @param aYRef Y value of PCB (0,0) reference point.
     * @return true if Ok.
     */
    bool ExportVRML_File( const wxString& aFullFileName, double aMMtoWRMLunit,
                          bool aIncludeUnspecified, bool aIncludeDNP,
                          bool aExport3DFiles, bool aUseRelativePaths,
                          const wxString& a3D_Subdir, double aXRef, double aYRef );

    /**
     * Export the current BOARD to a Hyperlynx HYP file.
     */
    void OnExportHyperlynx();

    /**
     * Create an IDF3 compliant BOARD (*.emn) and LIBRARY (*.emp) file.
     *
     * @param aPcb a pointer to the board to be exported to IDF.
     * @param aFullFileName the full filename of the export file.
     * @param aUseThou set to true if the desired IDF unit is thou (mil).
     * @param aXRef the board Reference Point in mm, X value.
     * @param aYRef the board Reference Point in mm, Y value.
     * @param aIncludeUnspecified true to include unspecified-type footprint models
     * @param aIncludeDNP true to include DNP footprint models
     * @return true if OK.
     */
    bool Export_IDF3( BOARD* aPcb, const wxString& aFullFileName,
                      bool aUseThou, double aXRef, double aYRef,
                      bool aIncludeUnspecified, bool aIncludeDNP );

    /**
     * Export the current BOARD to a STEP assembly.
     */
    void OnExportSTEP();

    /**
     * Export the current BOARD to a specctra dsn file.
     *
     * See http://www.autotraxeda.com/docs/SPECCTRA/SPECCTRA.pdf for the specification.
     *
     * @return true if OK
     */
    bool ExportSpecctraFile( const wxString& aFullFilename );

    /**
     * Import a specctra *.ses file and use it to relocate MODULEs and to replace all vias and
     * tracks in an existing and loaded #BOARD.
     *
     * See http://www.autotraxeda.com/docs/SPECCTRA/SPECCTRA.pdf for the specification.
     */
    bool ImportSpecctraSession( const wxString& aFullFilename );

    // Footprint editing (see also PCB_BASE_FRAME)
    void ShowFootprintPropertiesDialog( FOOTPRINT* aFootprint );

    int ShowExchangeFootprintsDialog( FOOTPRINT* aFootprint, bool aUpdateMode, bool aSelectedMode );

    /**
     * Replace \a aExisting footprint by \a aNew footprint using the \a Existing footprint
     * settings (position, orientation, pad netnames ...).
     *
     * The \a aExisting footprint is deleted or put in undo list.
     *
     * @param aExisting footprint to replace.
     * @param aNew footprint to put.
     * @param aCommit commit that should store the changes.
     */
    void ExchangeFootprint( FOOTPRINT* aExisting, FOOTPRINT* aNew, BOARD_COMMIT& aCommit,
                            bool deleteExtraTexts = true,
                            bool resetTextLayers = true,
                            bool resetTextEffects = true,
                            bool resetTextPositions = true,
                            bool resetTextContent = true,
                            bool resetFabricationAttrs = true,
                            bool resetClearanceOverrides = true,
                            bool reset3DModels = true,
                            bool* aUpdated = nullptr );

    /**
     * Install the corresponding dialog editor for the given item.
     *
     * @param aDC the current device context.
     * @param aItem a pointer to the BOARD_ITEM to edit.
     */
    void OnEditItemRequest( BOARD_ITEM* aItem ) override;

    /**
     * Change the active layer in the editor
     *
     * @param layer New layer to make active
     */
    void SwitchLayer( PCB_LAYER_ID layer ) override;

    /**
     * Modify one track segment width or one via diameter (using DRC control).
     *
     * Basic routine used by other routines when editing tracks or vias.
     * Note that casting this to boolean will allow you to determine whether any action
     * happened.
     *
     * @param aItem the track segment or via to modify.
     * @param aItemsListPicker the list picker to use for an undo command (can be NULL).
     * @param aUseDesignRules true to use design rules value, false to use current designSettings
     *                        value.
     */
    void SetTrackSegmentWidth( PCB_TRACK* aItem, PICKED_ITEMS_LIST* aItemsListPicker,
                               bool aUseDesignRules );


    /**
     * Edit params (layer, clearance, ...) for a zone outline.
     */
    void Edit_Zone_Params( ZONE* zone_container );

    // Properties dialogs
    void ShowTargetOptionsDialog( PCB_TARGET* aTarget );
    void InstallNetlistFrame();

    /**
     * @param aNetlist a #NETLIST owned by the caller.  This function fills it in.
     * @param aAnnotateMessage a message to be shown if annotation must be performed.  If empty,
     *                         annotation will be skipped.
     * @return true if a netlist was fetched.
     */
    bool FetchNetlistFromSchematic( NETLIST& aNetlist, const wxString& aAnnotateMessage );

    /**
     * Test if standalone mode.
     *
     * @return 0 if in standalone, -1 if Eeschema cannot be opened,
     * -2 if the schematic cannot be opened and 1 if OK.
     * If OK, opens Eeschema, and opens the schematic for this project
     */
    int TestStandalone();

    /**
     * Read a netlist from a file into a #NETLIST object.
     *
     * @param aFilename is the netlist to load.
     * @param aNetlist is the object to populate with data.
     * @param aReporter is a #REPORTER object to display messages.
     * @return true if the netlist was read successfully.
     */
    bool ReadNetlistFromFile( const wxString& aFilename, NETLIST& aNetlist, REPORTER& aReporter );

    /**
     * Called after netlist is updated.
     *
     * @param aUpdater is the updater object that was run.
     * @param aRunDragCommand is set to true if the drag command was invoked by this call.
     */
    void OnNetlistChanged( BOARD_NETLIST_UPDATER& aUpdater, bool* aRunDragCommand );

    /**
     * Send a message to the schematic editor to try to find schematic counterparts
     * of specified PCB items and select them.
     *
     * @param aItems are the items to try to select on schematic.
     * @param aFocusItem set to item to select and focus on even if selection can't be
     *                   represented in Schematic editor fully.
     * @param aForce select elements in Schematic editor whether or not the user has
     *               the select option chosen.
     */
    void SendSelectItemsToSch( const std::deque<EDA_ITEM*>& aItems, EDA_ITEM* aFocusItem,
                               bool aForce );

    /**
     * Send a message to the schematic editor so that it may move its cursor
     * to an item with the same reference as the \a aSyncItem and highlight it.
     *
     * @param aSyncItem The object whose reference is used to highlight in Eeschema.
     */
    void SendCrossProbeItem( BOARD_ITEM* aSyncItem );

    /**
     * Send a net name to Eeschema for highlighting.
     *
     * @param aNetName is the name of a net, or empty string to clear highlight.
     */
    void SendCrossProbeNetName( const wxString& aNetName );

    void ShowChangedLanguage() override;

    /**
     * Update the state of the GUI after a new board is loaded or created.
     */
    void OnBoardLoaded();

    /**
     * Set the main window title bar text.
     *
     * If file name defined by PCB_SCREEN::m_FileName is not set, the title is set to the
     * application name appended with no file.  Otherwise, the title is set to the full path
     * and file name and read only is appended to the title if the user does not have write
     * access to the file.
     */
    void UpdateTitle();

    /**
     * Called after the preferences dialog is run.
     */
    void CommonSettingsChanged( int aFlags ) override;

    /**
     * Called when light/dark theme is changed.
     */
    void ThemeChanged() override;

    void ProjectChanged() override;

    bool CanAcceptApiCommands() override;

    wxString GetCurrentFileName() const override;

    SELECTION& GetCurrentSelection() override;

    TOOL_ACTION* GetExportNetlistAction() { return m_exportNetlistAction; }

    DIALOG_BOOK_REPORTER* GetInspectDrcErrorDialog();

    DIALOG_BOOK_REPORTER* GetInspectConstraintsDialog();

    DIALOG_BOOK_REPORTER* GetInspectClearanceDialog();

    DIALOG_BOOK_REPORTER* GetFootprintDiffDialog();

    /**
     * Perform auto save when the board has been modified and not saved within the
     * auto save interval.
     *
     * @return true if the auto save was successful.
     */
    bool DoAutoSave();

    void ClearToolbarControl( int aId ) override;

    DECLARE_EVENT_TABLE()

protected:
    /**
     * Store the previous layer toolbar icon state information
     */
    struct LAYER_TOOLBAR_ICON_VALUES
    {
        int     previous_icon_size;
        COLOR4D previous_Route_Layer_TOP_color;
        COLOR4D previous_Route_Layer_BOTTOM_color;
        COLOR4D previous_background_color;

        LAYER_TOOLBAR_ICON_VALUES()
                : previous_icon_size( 0 ),
                  previous_Route_Layer_TOP_color( COLOR4D::UNSPECIFIED ),
                  previous_Route_Layer_BOTTOM_color( COLOR4D::UNSPECIFIED ),
                  previous_background_color( COLOR4D::UNSPECIFIED )
        {
        }
    };

    LAYER_TOOLBAR_ICON_VALUES m_prevIconVal;

    void doReCreateMenuBar() override;

    void configureToolbars() override;

    // The Tool Framework initialization
    void setupTools();
    void setupUIConditions() override;

    /**
     * Switch currently used canvas (Cairo / OpenGL).
     *
     * It also reinit the layers manager that slightly changes with canvases
     */
    void SwitchCanvas( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType ) override;

    /**
     * Fill action menu with all registered action plugins
     */
    void buildActionPluginMenus( ACTION_MENU* aActionMenu );

    /**
     * Append action plugin buttons to given toolbar
     */
    void addActionPluginTools( ACTION_TOOLBAR* aToolbar );

    /**
     * Execute action plugin's Run() method and updates undo buffer.
     *
     * @param aActionPlugin action plugin
     */
    void RunActionPlugin( ACTION_PLUGIN* aActionPlugin );

    /**
     * Launched by the menu when an action is called.
     *
     * @param aEvent sent by wx
     */
    void OnActionPluginMenu( wxCommandEvent& aEvent);

    /**
     * Launched by the button when an action is called.
     *
     * @param aEvent sent by wx
     */
    void OnActionPluginButton( wxCommandEvent& aEvent );

    PLUGIN_ACTION_SCOPE PluginActionScope() const override { return PLUGIN_ACTION_SCOPE::PCB; }

    /**
     * Perform auto save when the board has been modified and not saved within the
     * auto save interval.
     *
     * @return true if the auto save was successful.
     */
    bool doAutoSave() override { return DoAutoSave(); }

    /**
     * Load the given filename but sets the path to the current project path.
     *
     * @param full file path of file to be imported.
     * @param aFileType PCB_FILE_T value for file type
     */
    bool importFile( const wxString& aFileName, int aFileType,
                     const std::map<std::string, UTF8>* aProperties = nullptr );

    /**
     * @brief Save a board object to a file
     *
     * @param aBoard The board object to save
     * @param aFileName The file name to save the board to
     * @param aHeadless If true, suppresses informational output (e.g. to be used from the API)
     *
     * @return
     */
    bool saveBoardAsFile( BOARD* aBoard, const wxString& aFileName, bool aHeadless = false );

    bool saveSelectionToDesignBlock( const wxString& aNickname, PCB_SELECTION& aSelection, DESIGN_BLOCK& aBlock );


    bool canCloseWindow( wxCloseEvent& aCloseEvent ) override;
    void doCloseWindow() override;

    // protected so that PCB::IFACE::CreateWindow() is the only factory.
    PCB_EDIT_FRAME( KIWAY* aKiway, wxWindow* aParent );

    void onSize( wxSizeEvent& aEvent );

    int inferLegacyEdgeClearance( BOARD* aBoard, bool aShowUserMsg = true );

    void redrawNetnames();

    void saveProjectSettings() override;

    void onCloseModelessBookReporterDialogs( wxCommandEvent& aEvent );

#ifdef KICAD_IPC_API
    void onPluginAvailabilityChanged( wxCommandEvent& aEvt );
#endif

public:
    wxChoice* m_SelTrackWidthBox;        // a choice box to display and select current track width
    wxChoice* m_SelViaSizeBox;           // a choice box to display and select current via diameter
    wxChoice* m_currentVariantCtrl;      // a choice box to display and select current variant

    bool m_show_layer_manager_tools;
    bool m_show_search;
    bool m_show_net_inspector;

    bool m_ZoneFillsDirty;          // Board has been modified since last zone fill.

    bool m_probingSchToPcb;         // Recursion guard when synchronizing selection from schematic

    // Cross-probe flashing support
    wxTimer        m_crossProbeFlashTimer;   ///< Timer to toggle selection visibility for flash
    int            m_crossProbeFlashPhase = 0; ///< Phase counter
    std::vector<KIID> m_crossProbeFlashItems;  ///< Items to flash (by UUID)
    bool           m_crossProbeFlashing = false; ///< Currently flashing guard

    void StartCrossProbeFlash( const std::vector<BOARD_ITEM*>& aItems );
    void OnCrossProbeFlashTimer( wxTimerEvent& aEvent );

private:
    friend struct PCB::IFACE;
    friend class APPEARANCE_CONTROLS;

    /**
     * The export board netlist tool action object.
     *
     * This is created at runtime rather than declared statically so it doesn't show up in
     * the list of assignable hot keys since it's only available as an advanced configuration
     * option.
     */
    TOOL_ACTION* m_exportNetlistAction;

    DIALOG_FIND* m_findDialog;
    DIALOG_BOOK_REPORTER* m_inspectDrcErrorDlg;
    DIALOG_BOOK_REPORTER* m_inspectClearanceDlg;
    DIALOG_BOOK_REPORTER* m_inspectConstraintsDlg;
    DIALOG_BOOK_REPORTER* m_footprintDiffDlg;
    DIALOG_BOARD_SETUP*   m_boardSetupDlg;

    std::vector<LIB_ID>    m_designBlockHistoryList;
    PCB_DESIGN_BLOCK_PANE* m_designBlocksPane;

    const std::map<std::string, UTF8>* m_importProperties; // Properties used for non-KiCad import.

    /**
     * Keep track of viewport so that track net labels can be adjusted when it changes.
     */
    BOX2D        m_lastNetnamesViewport;

    wxTimer*     m_eventCounterTimer;

#ifdef KICAD_IPC_API
    std::unique_ptr<API_HANDLER_PCB> m_apiHandler;
    std::unique_ptr<API_HANDLER_COMMON> m_apiHandlerCommon;
#endif
};

#endif  // __PCB_EDIT_FRAME_H__
