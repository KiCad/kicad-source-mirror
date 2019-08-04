/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef  SCH_EDIT_FRAME_H
#define  SCH_EDIT_FRAME_H

#include <sch_base_frame.h>
#include <config_params.h>
#include <undo_redo_container.h>
#include <template_fieldnames.h>
#include <ee_collectors.h>
#include <tool/selection.h>
#include <erc_settings.h>
#include <sch_draw_panel.h>
#include <sch_text.h>               // enum PINSHEETLABEL_SHAPE
#include <tool/selection.h>
#include <status_popup.h>

class SCH_ITEM;
class EDA_ITEM;
class SCH_TEXT;
class SCH_BITMAP;
class SCH_SHEET;
class SCH_SHEET_PATH;
class SCH_SHEET_PIN;
class SCH_COMPONENT;
class SCH_FIELD;
class SCH_JUNCTION;
class DIALOG_SCH_FIND;
class wxFindDialogEvent;
class wxFindReplaceData;
class RESCUER;


/// enum used in RotationMiroir()
enum COMPONENT_ORIENTATION_T {
    CMP_NORMAL,                     // Normal orientation, no rotation or mirror
    CMP_ROTATE_CLOCKWISE,           // Rotate -90
    CMP_ROTATE_COUNTERCLOCKWISE,    // Rotate +90
    CMP_ORIENT_0,                   // No rotation and no mirror id CMP_NORMAL
    CMP_ORIENT_90,                  // Rotate 90, no mirror
    CMP_ORIENT_180,                 // Rotate 180, no mirror
    CMP_ORIENT_270,                 // Rotate -90, no mirror
    CMP_MIRROR_X = 0x100,           // Mirror around X axis
    CMP_MIRROR_Y = 0x200            // Mirror around Y axis
};


/** Schematic annotation order options. */
enum ANNOTATE_ORDER_T {
    SORT_BY_X_POSITION,     ///< Annotate by X position from left to right.
    SORT_BY_Y_POSITION,     ///< Annotate by Y position from top to bottom.
    UNSORTED,               ///< Annotate by position of component in the schematic sheet
                            ///< object list.
};


/** Schematic annotation type options. */
enum ANNOTATE_OPTION_T {
    INCREMENTAL_BY_REF,     ///< Annotate incrementally using the first free reference number.
    SHEET_NUMBER_X_100,     ///< Annotate using the first free reference number starting at
                            ///< the sheet number * 100.
    SHEET_NUMBER_X_1000,    ///< Annotate using the first free reference number starting at
                            ///< the sheet number * 1000.
};


/// Schematic search type used by the socket link with Pcbnew
enum SCH_SEARCH_T {
    HIGHLIGHT_PIN,
    HIGHLIGHT_COMPONENT
};


/**
 * Schematic editor (Eeschema) main window.
 */
class SCH_EDIT_FRAME : public SCH_BASE_FRAME
{
private:
    wxString                m_DefaultSchematicFileName;
    wxString                m_SelectedNetName;

    PARAM_CFG_ARRAY         m_projectFileParams;
    PARAM_CFG_ARRAY         m_configSettings;
    ERC_SETTINGS            m_ercSettings;
    wxPageSetupDialogData   m_pageSetupData;
    bool                    m_printMonochrome;    ///< Print monochrome instead of grey scale.
    bool                    m_printSheetReference;
    SCH_ITEM*               m_item_to_repeat;     ///< Last item to insert by the repeat command.
    int                     m_repeatLabelDelta;   ///< Repeat label number increment step.
    SCH_ITEM*               m_undoItem;           ///< Copy of the current item being edited.
    wxString                m_netListerCommand;   ///< Command line to call a custom net list
                                                  ///< generator.
    int                     m_exec_flags;         ///< Flags of the wxExecute() function
                                                  ///< to call a custom net list generator.

    bool                    m_forceHVLines;       ///< force H or V directions for wires, bus, line

    bool                    m_autoplaceFields;    ///< automatically place component fields
    bool                    m_autoplaceJustify;   ///< allow autoplace to change justification
    bool                    m_autoplaceAlign;     ///< align autoplaced fields to the grid
    bool                    m_footprintPreview;   ///< whether to show footprint previews
    bool                    m_showIllegalSymbolLibDialog;

    DIALOG_SCH_FIND*        m_findReplaceDialog;
    STATUS_TEXT_POPUP*      m_findReplaceStatusPopup;

    /// Flag to indicate show hidden pins.
    bool        m_showAllPins;

    /// The name of the destination directory to use when generating plot files.
    wxString    m_plotDirectoryName;

    /// The name of the format to use when generating a net list.
    wxString    m_netListFormat;

    /// Use netcodes (net number) as net names when generating spice net lists.
    bool        m_spiceAjustPassiveValues;

private:

    /*  these are PROJECT specific, not schematic editor specific
    wxString        m_userLibraryPath;
    wxArrayString   m_componentLibFiles;
    */

    static PINSHEETLABEL_SHAPE m_lastSheetPinType;  ///< Last sheet pin type.
    static wxSize   m_lastSheetPinTextSize;         ///< Last sheet pin text size.
    static wxPoint  m_lastSheetPinPosition;         ///< Last sheet pin position.

protected:
    /**
     * Initializing accessor for the pin text size
     */
    const wxSize &GetLastSheetPinTextSize();

    /**
     * Save the schematic files that have been modified and not yet saved.
     *
     * @return true if the auto save was successful otherwise false.
     */
    bool doAutoSave() override;

    /**
     * Returns true if the schematic has been modified.
     */
    bool isAutoSaveRequired() const override;

    /**
     * Verify that annotation is complete so that a proper netlist is even
     * possible.  If not, asks the user if annotation should be done.
     *
     * @return bool - true if annotation is complete, else false.
     */
    bool prepareForNetlist();

    /**
     * Send the kicad netlist over to CVPCB.
     */
    void sendNetlistToCvpcb();

public:
    SCH_EDIT_FRAME( KIWAY* aKiway, wxWindow* aParent );
    ~SCH_EDIT_FRAME() override;

    SCH_SCREEN* GetScreen() const override;

    void OnCloseWindow( wxCloseEvent& Event );

    bool GetForceHVLines() const { return m_forceHVLines; }
    void SetForceHVLines( bool aForceHVdirection ) { m_forceHVLines = aForceHVdirection; }

    bool GetShowAllPins() const override { return m_showAllPins; }
    void SetShowAllPins( bool aEnable ) { m_showAllPins = aEnable; }

    bool GetShowFootprintPreviews() const { return m_footprintPreview; }
    void SetShowFootprintPreviews( bool aEnable ) { m_footprintPreview = aEnable; }

    bool GetAutoplaceFields() const { return m_autoplaceFields; }
    void SetAutoplaceFields( bool aEnable ) { m_autoplaceFields = aEnable; }

    bool GetAutoplaceAlign() const { return m_autoplaceAlign; }
    void SetAutoplaceAlign( bool aEnable ) { m_autoplaceAlign = aEnable; }

    bool GetAutoplaceJustify() const { return m_autoplaceJustify; }
    void SetAutoplaceJustify( bool aEnable ) { m_autoplaceJustify = aEnable; }

    const wxString& GetNetListFormatName() const { return m_netListFormat; }
    void SetNetListFormatName( const wxString& aFormat ) { m_netListFormat = aFormat; }

    bool GetSpiceAjustPassiveValues() const { return m_spiceAjustPassiveValues; }
    void SetSpiceAdjustPassiveValues( bool aEnable ) { m_spiceAjustPassiveValues = aEnable; }

    /// accessor to the destination directory to use when generating plot files.
    const wxString& GetPlotDirectoryName() const { return m_plotDirectoryName; }
    void SetPlotDirectoryName( const wxString& aDirName ) { m_plotDirectoryName = aDirName; }

    /**
     * Return the project file parameter list for Eeschema.
     *
     * Populate the project file parameter array specific to Eeschema if it hasn't
     * already been populated and return a reference to the array to the caller.
     */
    PARAM_CFG_ARRAY& GetProjectFileParameters();

    /**
     * Save changes to the project settings to the project (.pro) file.
     *
     * @param aAskForSave = true to open a dialog before saving the settings
     */
    void SaveProjectSettings( bool aAskForSave ) override;

    /**
     * Loads the KiCad project file (*.pro) settings specific to Eeschema.
     *
     * @return True if the project file was loaded correctly.
     */
    bool LoadProjectFile();

    const ERC_SETTINGS& GetErcSettings() { return m_ercSettings; }

    void UpdateErcSettings( const ERC_SETTINGS& aSettings ) { m_ercSettings = aSettings; }

    /**
     * Insert or append a wanted symbol field name into the field names template.
     *
     * Should be used for any symbol property editor.  If the name already exists, it
     * overwrites the same name.
     *
     * @param aFieldName is a full description of the wanted field, and it must not match
     *          any of the default field names.
     * @return int - the index within the config container at which aFieldName was
     *          added, or -1 if the name is illegal because it matches a default field name.
     */
    int AddTemplateFieldName( const TEMPLATE_FIELDNAME& aFieldName )
    {
        return m_templateFieldNames.AddTemplateFieldName( aFieldName );
    }

    /**
     * Remove all template field names.
     */
    void DeleteAllTemplateFieldNames()
    {
        m_templateFieldNames.DeleteAllTemplateFieldNames();
    }

    /**
     * Return the Eeschema applications settings.
     * <p>
     * This replaces the old statically define list that had the project file settings and
     * the application settings mixed together.  This was confusing and caused some settings
     * to get saved and loaded incorrectly.  Currently, only the settings that are needed at
     * start up by the main window are defined here.  There are other locally used settings
     * scattered throughout the Eeschema source code.  If you need to define a configuration
     * setting that need to be loaded at run time, this is the place to define it.
     * </p>
     */
    PARAM_CFG_ARRAY& GetConfigurationSettings();

    void LoadSettings( wxConfigBase* aCfg ) override;
    void SaveSettings( wxConfigBase* aCfg ) override;

    void CreateScreens();
    void ReCreateHToolbar() override;
    void ReCreateVToolbar() override;
    void ReCreateOptToolbar() override;
    void ReCreateMenuBar() override;

    /**
     * Must be called after a schematic change in order to set the "modify" flag of the
     * current screen and update the date in frame reference.
     */
    void OnModify() override;

    /**
     * Return a human-readable description of the current screen.
     */
    wxString GetScreenDesc() const override;

    /**
     * Execute a remote command send by Pcbnew via a socket,
     * port KICAD_SCH_PORT_SERVICE_NUMBER (currently 4243)
     * this is a virtual function called by EDA_DRAW_FRAME::OnSockRequest().
     * @param cmdline = received command from socket
     */
    void ExecuteRemoteCommand( const char* cmdline ) override;

    void KiwayMailIn( KIWAY_EXPRESS& aEvent ) override;

    double BestZoom() override;

    /**
     * Add an item to the schematic and adds the changes to the undo/redo container.
     * @param aUndoAppend True if the action should be appended to the current undo record.
     */
    void AddItemToScreenAndUndoList( SCH_ITEM* aItem, bool aUndoAppend = false );

    /**
     * Run the Find or Find & Replace dialog.
     */
    void ShowFindReplaceDialog( bool aReplace );

    void ShowFindReplaceStatus( const wxString& aMsg );
    void ClearFindReplaceStatus();

    /**
     * Get the find criteria (as set by the dialog).
     */
    wxFindReplaceData* GetFindReplaceData();

    /**
     * Notification that the Find dialog has closed.
     */
    void OnFindDialogClose();

    /**
     * Breaks a single segment into two at the specified point.
     *
     * NOTE: always appends to the existing undo state.
     *
     * @param aSegment Line segment to break
     * @param aPoint Point at which to break the segment
     * @param aNewSegment Pointer to the newly created segment (if given and created)
     * @param aScreen is the screen to examine, or nullptr to examine the current screen
     * @return True if any wires or buses were broken.
     */
    bool BreakSegment( SCH_LINE* aSegment, const wxPoint& aPoint,
                       SCH_LINE** aNewSegment = NULL, SCH_SCREEN* aScreen = nullptr );

    /**
     * Checks every wire and bus for a intersection at \a aPoint and break into two segments
     * at \a aPoint if an intersection is found.
     *
     * NOTE: always appends to the existing undo state.
     *
     * @param aPoint Test this point for an intersection.
     * @param aScreen is the screen to examine, or nullptr to examine the current screen
     * @return True if any wires or buses were broken.
     */
    bool BreakSegments( const wxPoint& aPoint, SCH_SCREEN* aScreen = nullptr );

    /**
     * Tests all junctions and bus entries in the schematic for intersections with wires and
     * buses and breaks any intersections into multiple segments.
     *
     * NOTE: always appends to the existing undo state.
     *
     * @param aScreen is the screen to examine, or nullptr to examine the current screen
     * @return True if any wires or buses were broken.
     */
    bool BreakSegmentsOnJunctions( SCH_SCREEN* aScreen = nullptr );

    /**
     * Test all of the connectable objects in the schematic for unused connection points.
     * @return True if any connection state changes were made.
     */
    bool TestDanglingEnds();

    /**
     * Send a message to Pcbnew via a socket connection.
     *
     * Commands are:
     * - $PART: reference   put cursor on footprint anchor
     * - $PIN: number $PART: reference put cursor on the footprint pad
     * - $SHEET: time_stamp  select all footprints of components is the schematic sheet path
     *
     * @param aObjectToSync = item to be located on board
     *      (footprint, pad, text or schematic sheet)
     * @param aPart = component if objectToSync is a sub item of a symbol (like a pin)
     */
    void SendMessageToPCBNEW( EDA_ITEM* aObjectToSync, SCH_COMPONENT* aPart );

    /**
     * Sends a net name to pcbnew for highlighting
     *
     * @param aNetName is the name of a net, or empty string to clear highlight
     */
    void SendCrossProbeNetName( const wxString& aNetName );

    /**
     * Tells PcbNew to clear the existing highlighted net, if one exists
     */
    void SendCrossProbeClearHighlight();

    const wxString& GetSelectedNetName() const { return m_SelectedNetName; }
    void SetSelectedNetName( const wxString& aNetName ) { m_SelectedNetName = aNetName; }

    /**
     * Create a flat list which stores all connected objects.
     *
     * @param updateStatusText decides if window StatusText should be modified.
     * @return NETLIST_OBJECT_LIST* - caller owns the object.
     */
    NETLIST_OBJECT_LIST* BuildNetListBase( bool updateStatusText = true );

    /**
     * Create a netlist for the current schematic.
     *
     * - Test for some issues (missing or duplicate references and sheet names)
     * - Build netlist info
     * - Create the netlist file (different formats)
     *
     * @param aSilent is true if annotation error dialog should be skipped
     * @param aSilentAnnotate is true if components should be reannotated silently
     * @returns a unique_ptr to the netlist
     */
    NETLIST_OBJECT_LIST* CreateNetlist( bool aSilent = false,
                                        bool aSilentAnnotate = false );

    /**
     * Create a netlist file.
     *
     * @param aConnectedItemsList = the initialized list of connected items, take ownership.
     * @param aFormat = netlist format (NET_TYPE_PCBNEW ...)
     * @param aFullFileName = full netlist file name
     * @param aNetlistOptions = netlist options using OR'ed bits.
     * <p>
     * For SPICE netlist only:
     *      if NET_USE_NETNAMES is set, use net names from labels in schematic
     *                             else use net numbers (net codes)
     *      if NET_USE_X_PREFIX is set : change "U" and "IC" reference prefix to "X"
     * </p>
     * @param aReporter = a REPORTER to report error messages,
     *          mainly if a command line must be run (can be NULL
     * @return true if success.
     */
    bool WriteNetListFile( NETLIST_OBJECT_LIST* aConnectedItemsList,
                           int             aFormat,
                           const wxString& aFullFileName,
                           unsigned        aNetlistOptions,
                           REPORTER*       aReporter = NULL );

    /**
     * Clear the current component annotation.
     *
     * @param aCurrentSheetOnly Clear only the annotation for the current sheet if true.
     *                          Otherwise clear the entire schematic annotation.
     */
    void DeleteAnnotation( bool aCurrentSheetOnly );

    /**
     * Annotate the components in the schematic that are not currently annotated.
     *
     * @param aAnnotateSchematic Annotate the entire schematic if true.  Otherwise annotate
     *                           the current sheet only.
     * @param aSortOption Define the annotation order.  See #ANNOTATE_ORDER_T.
     * @param aAlgoOption Define the annotation style.  See #ANNOTATE_OPTION_T.
     * @param aStartNumber The start number for non-sheet-based annotation styles.
     * @param aResetAnnotation Clear any previous annotation if true.  Otherwise, keep the
     *                         existing component annotation.
     * @param aRepairTimestamps Test for and repair any duplicate time stamps if true.
     *                          Otherwise, keep the existing time stamps.  This option
     *                          could change previous annotation because time stamps are
     *                          used to handle annotation in complex hierarchies.
     * @param aLockUnits    When both aLockUnits and aResetAnnotation are true, all unit
     *                          associations should be kept when reannotating. That is, if
     *                          two components were R8A and R8B, they may become R3A and R3B,
     *                          but not R3A and R3C or R3C and R4D.
     *                          When aResetAnnotation is true but aLockUnits is false, the
     *                          usual behavior of annotating each part individually is
     *                          performed.
     *                          When aResetAnnotation is false, this option has no effect.
     * @param aReporter A sink for error messages.  Use NULL_REPORTER if you don't need errors.
     *
     * When the sheet number is used in annotation, each sheet annotation starts from sheet
     * number * 100.  In other words the first sheet uses 100 to 199, the second sheet uses
     * 200 to 299, and so on.
     */
    void AnnotateComponents( bool aAnnotateSchematic, ANNOTATE_ORDER_T aSortOption,
                             ANNOTATE_OPTION_T aAlgoOption, int aStartNumber,
                             bool aResetAnnotation, bool aRepairTimestamps, bool aLockUnits,
                             REPORTER& aReporter );

    /**
     * Check for annotation errors.
     *
     * The following list of items are checked:
     * - Components that are not annotated.
     * - Duplicate component references.
     * - Multiple part per package components where the part number is greater number of parts
     *   in the package.
     * - Multiple part per package components where the reference designator is different
     *   between parts.
     *
     * @return Number of annotation errors found.
     * @param aReporter A sink for error messages.  Use NULL_REPORTER if you don't need errors.
     * @param aOneSheetOnly Check the current sheet only if true.  Otherwise check
     *                      the entire schematic.
     */
    int CheckAnnotate( REPORTER& aReporter, bool aOneSheetOnly );

    /**
     * Run a modal version of the Annotate dialog for a specific purpose.
     * @param aMessage A user message indicating the purpose.
     * @return the result of ShowModal()
     */
    int ModalAnnotate( const wxString& aMessage );

    // Functions used for hierarchy handling
    SCH_SHEET_PATH& GetCurrentSheet();

    void SetCurrentSheet( const SCH_SHEET_PATH& aSheet );

    /**
     * Rebuild the GAL and redraw the screen.  Call when something went wrong.
     */
    void HardRedraw() override;

    /**
     * Draw the current sheet on the display.
     */
    void DisplayCurrentSheet();

    /**
     * Use the wxWidgets print code to draw an image of the current sheet onto
     * the clipboard.
     */
    void DrawCurrentSheetToClipboard();

    /**
     * Called when modifying the page settings.
     * In derived classes it can be used to modify parameters like draw area size,
     * and any other local parameter related to the page settings.
     */
    void OnPageSettingsChange() override;

    /**
     * @return a filename that can be used in plot and print functions
     * for the current screen and sheet path.
     * This filename is unique and must be used instead of the screen filename
     * (or screen filename) when one must creates file for each sheet in the
     * hierarchy.  because in complex hierarchies a sheet and a SCH_SCREEN is
     * used more than once
     * Name is &ltroot sheet filename&gt-&ltsheet path&gt and has no extension.
     * However if filename is too long name is &ltsheet filename&gt-&ltsheet number&gt
     */
    wxString GetUniqueFilenameForCurrentSheet();

    /**
     * Set the m_ScreenNumber and m_NumberOfScreens members for screens.
     *
     * @note This must be called after deleting or adding a sheet and when entering a sheet.
     */
    void SetSheetNumberAndCount();

    /**
     * Show the print dialog.
     */
    void Print();

    wxPageSetupDialogData& GetPageSetupData() { return m_pageSetupData; }

    bool GetPrintMonochrome() { return m_printMonochrome; }
    void SetPrintMonochrome( bool aMonochrome ) { m_printMonochrome = aMonochrome; }
    bool GetPrintSheetReference() { return m_printSheetReference; }
    void SetPrintSheetReference( bool aShow ) { m_printSheetReference = aShow; }

    // Plot functions:
    void PlotSchematic();

    void NewProject();
    void LoadProject();

    void Save_File( bool doSaveAs = false );

    bool SaveProject();

    bool OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl = 0 ) override;

    /**
     * Import a KiCad schematic into the current sheet.
     *
     * @return True if the schematic was imported properly.
     */
    bool AppendSchematic();

    /**
     * Save \a aScreen to a schematic file.
     *
     * @param aScreen A pointer to the SCH_SCREEN object to save.  A NULL pointer saves
     *                the current screen.
     * @param aSaveUnderNewName Controls how the file is to be saved;: using  previous name
     *                          or under a new name .
     * @param aCreateBackupFile Creates a back of the file associated with \a aScreen
     *                          if true.
     *                           Helper definitions #CREATE_BACKUP_FILE and
     *                          #NO_BACKUP_FILE are defined for improved code readability.
     * @return True if the file has been saved.
     */
    bool SaveEEFile( SCH_SCREEN* aScreen,
                     bool        aSaveUnderNewName = false,
                     bool        aCreateBackupFile = CREATE_BACKUP_FILE );


    /**
     * Checks if any of the screens has unsaved changes and asks the user whether to save or
     * drop them.
     *
     * @return True if user decided to save or drop changes, false if the operation should be
     *         canceled.
     */
    bool AskToSaveChanges();

    SCH_JUNCTION* AddJunction( const wxPoint& aPos, bool aAppendToUndo = false,
                               bool aFinal = true );

    /**
     * Gets the next queued text item
     * @return next SCH_TEXT* or nullptr if empty
     */
    SCH_TEXT* GetNextNewText();

    SCH_TEXT* CreateNewText( int aType );

    /**
     * Performs routine schematic cleaning including breaking wire and buses and deleting
     * identical objects superimposed on top of each other.
     *
     * NOTE: always appends to the existing undo state.
     *
     * @param aScreen is the screen to examine, or nullptr to examine the current screen
     * @return True if any schematic clean up was performed.
     */
    bool SchematicCleanUp( SCH_SCREEN* aScreen = nullptr );

    /**
     * If any single wire passes through _both points_, remove the portion between the two points,
     * potentially splitting the wire into two.
     *
     * NOTE: always appends to the existing undo state.
     *
     * @param aStart The starting point for trimmming
     * @param aEnd The ending point for trimming
     * @return True if any wires were changed by this operation
     */
    bool TrimWire( const wxPoint& aStart, const wxPoint& aEnd );

    /**
     * Collects a unique list of all possible connection points in the schematic.
     *
     * @param aConnections vector of connections
     */
    void GetSchematicConnections( std::vector< wxPoint >& aConnections );

    void OnOpenPcbnew( wxCommandEvent& event );
    void OnOpenCvpcb( wxCommandEvent& event );
    void OnRescueProject( wxCommandEvent& event );
    void OnRemapSymbols( wxCommandEvent& aEvent );
    void OnUpdatePCB( wxCommandEvent& event );
    void OnAnnotate( wxCommandEvent& event );

private:
    // Sets up the tool framework
    void setupTools();

    void OnExit( wxCommandEvent& event );

    void OnLoadFile( wxCommandEvent& event );
    void OnAppendProject( wxCommandEvent& event );
    void OnImportProject( wxCommandEvent& event );

    /**
     * Set the main window title bar text.
     *
     * If file name defined by SCH_SCREEN::m_FileName is not set, the title is set to the
     * application name appended with no file.
     * Otherwise, the title is set to the hierarchical sheet path and the full file name, and
     * read only is appended to the title if the user does not have write access to the file.
     */
    void UpdateTitle();

    /**
     * Perform all cleanup and normalization steps so that the whole schematic
     * is in a good state.  This should only be called when loading a file.
     */
    void NormalizeSchematicOnFirstLoad( bool recalculateConnections );

    /**
     * Verify that \a aSheet will not cause a recursion error in \a aHierarchy.
     *
     * @param aSheet is the #SCH_SHEET object to test.
     * @param aHierarchy is the #SCH_SHEET_PATH where \a aSheet is going to reside.
     *
     * @return true if \a aSheet will cause a resursion error in \a aHierarchy.
     */
    bool checkSheetForRecursion( SCH_SHEET* aSheet, SCH_SHEET_PATH* aHierarchy );

    /**
     * Verify that the symbol library links \a aSheet and all of it's child sheets have
     * been remapped to the symbol library table.
     *
     * @param aSheet is the #SCH_SHEET object to test.
     *
     * @return true if \a aSheet and it's child sheets have not been remapped.
     */
    bool checkForNoFullyDefinedLibIds( SCH_SHEET* aSheet );

    /**
     *  Load the given filename but sets the path to the current project path.
     *
     *  @param full filepath of file to be imported.
     *  @param aFileType SCH_FILE_T value for file type
     */
    bool importFile( const wxString& aFileName, int aFileType );

public:
    /**
     * Change a text type to another one.
     *
     * The new text, label, hierarchical label, or global label is created from the old text
     * and the old text object is deleted.
     *
     * A tricky case is when the 'old" text is being edited (i.e. moving) because we must
     * create a new text, and prepare the undo/redo command data for this change and the
     * current move/edit command
     */
    void ConvertTextType( SCH_TEXT* aText, KICAD_T aNewType );

    /**
     * Edit an existing sheet or add a new sheet to the schematic.
     *
     * When \a aSheet is a new sheet:
     * - and the file name already exists in the schematic hierarchy, the screen associated
     *   with the sheet found in the hierarchy is associated with \a aSheet.
     * - and the file name already exists on the system, then \a aSheet is loaded with the
     *   existing file.
     * - and the file name does not exist in the schematic hierarchy or on the file system,
     *   then a new screen is created and associated with \a aSheet.
     *
     * When \a aSheet is an existing sheet:
     * - and the file name already exists in the schematic hierarchy, the current associated
     *   screen is replace by the one found in the hierarchy.
     * - and the file name already exists on the system, the current associated screen file
     *   name is changed and the file is loaded.
     * - and the file name does not exist in the schematic hierarchy or on the file system,
     *   the current associated screen file name is changed and saved to disk.
     *
     * Note: the screen is not refresh. The caller is responsible to do that
     *
     * @param aSheet is the sheet to edit
     * @param aHierarchy is the current hierarchy containing aSheet
     * @param aClearAnnotationNewItems is a reference to a bool to know if the items managed by
     * this sheet need to have their annotation cleared i.e. when an existing item list is used.
     * it can happens when the edited sheet used an existying file, or becomes a new instance
     * of a already existing sheet.
     */
    bool EditSheet( SCH_SHEET* aSheet, SCH_SHEET_PATH* aHierarchy, bool* aClearAnnotationNewItems );

    void InitSheet( SCH_SHEET* aSheet, const wxString& aNewFilename );

    /**
     * Load a the KiCad schematic file \a aFileName into the sheet \a aSheet.
     *
     * If \a aSheet does not have a valid #SCH_SCREEN object, the schematic is loaded into
     * \a Sheet.  Otherwise, it is appended to the current #SCH_SCREEN object.
     *
     * In order to import a schematic a lot of things have to happen to before the contents
     * of the imported schematic can be appended to the current page.  The following list
     * describes this process:
     *
     * - Load the schematic into a temporary #SCH_SHEET object.
     * - Make sure the imported schematic does not cause any hierarchy recursion issues.
     * - Verify the imported schematic uses fully qualified #LIB_ID objects (symbol library table).
     * - Check all of the possible combinations that could cause broken symbol library links
     *   and give the user the option to cancel the append process.  The following conditions
     *   are check but they still do not guarantee that there will not be any broken symbol
     *   library links:
     *   - The source schematic is in the current project path and contains symbol library
     *     nicknames not found in the project symbol library table.  This can happen if the
     *     schematic is copied to the current project path from another project.
     *   - The source schematic is in a different path and there are symbol library link nicknames
     *     that do not exist in either the current symbol library table or the source project
     *     symbol library table if it exists in the source path.
     *   - The source schematic is in a different path and contains duplicate symbol library
     *     nicknames that point to different libraries.
     * - Check to see if any symbol libraries need to be added to the current project's symbol
     *   library table.  This includes:
     *   - Check if the symbol library already exists in the project or global symbol library
     *     table.
     *   - Convert symbol library URLS that use the ${KIPRJMOD} environment variable to absolute
     *     paths.  ${KIPRJMOD} will not be the same for this project.
     * - Clear all of the annotation in the imported schematic to prevent clashes.
     * - Append the objects from the temporary sheet to the current page.
     * - Replace any duplicate time stamps.
     * - Refresh the symbol library links.
     *
     * @param aSheet is the sheet to either append or load the schematic.
     * @param aHierarchy is the current position in the schematic hierarchy used to test for
     *                   possible file recursion issues.
     * @param aFileName is the file name to load.  The file name is expected to have an absolute
     *                  path.
     *
     * @return True if the schematic was imported properly.
     */
    bool LoadSheetFromFile( SCH_SHEET* aSheet, SCH_SHEET_PATH* aHierarchy,
                            const wxString& aFileName );

    /**
     * Create a new SCH_SHEET_PIN object and add it to \a aSheet at the current cursor position.
     *
     * @param aSheet The sheet to add the new sheet pin to.
     * @return The new sheet pin object created or NULL if the task was aborted by the user.
     */
    SCH_SHEET_PIN* CreateSheetPin( SCH_SHEET* aSheet, SCH_HIERLABEL* aLabel );

    /**
     * Import a hierarchical label with no attached sheet pin.
     *
     * @param aSheet The sheet to import the new sheet pin to.
     */
    SCH_HIERLABEL* ImportHierLabel( SCH_SHEET* aSheet );

    /**
     * Removes a given junction and heals any wire segments under the junction
     *
     * @param aItem The junction to delete
     * @param aAppend True if we are updating an ongoing commit
     */
    void DeleteJunction( SCH_ITEM* aItem, bool aAppend = false );

    int GetLabelIncrement() const { return m_repeatLabelDelta; }

    void ConvertPart( SCH_COMPONENT* aComponent );

    void SelectUnit( SCH_COMPONENT* aComponent, int aUnit );

    /* Undo - redo */

    /**
     * Create a copy of the current schematic item, and put it in the undo list.
     *
     *  flag_type_command =
     *      UR_CHANGED
     *      UR_NEW
     *      UR_DELETED
     *      UR_WIRE_IMAGE
     *      UR_MOVED
     *
     * If it is a delete command, items are put on list with the .Flags member
     * set to UR_DELETED.  When it will be really deleted, the GetDrawItems() and the
     * sub-hierarchy will be deleted.  If it is only a copy, the GetDrawItems() and the
     * sub-hierarchy must NOT be deleted.
     *
     * @note
     * Edit wires and buses is a bit complex.
     * because when a new wire is added, a lot of modifications in wire list is made
     * (wire concatenation): modified items, deleted items and new items
     * so flag_type_command is UR_WIRE_IMAGE: the struct ItemToCopy is a list of
     * wires saved in Undo List (for Undo or Redo commands, saved wires will be
     * exchanged with current wire list
     *
     * @param aItemToCopy = the schematic item modified by the command to undo
     * @param aTypeCommand = command type (see enum UNDO_REDO_T)
     * @param aAppend = add the item to the previous undo list
     * @param aTransformPoint = the reference point of the transformation,
     *                          for commands like move
     */
    void SaveCopyInUndoList( SCH_ITEM* aItemToCopy,
                             UNDO_REDO_T aTypeCommand,
                             bool aAppend = false,
                             const wxPoint& aTransformPoint = wxPoint( 0, 0 ) );

    /**
     * Create a new entry in undo list of commands.
     *
     * @param aItemsList = the list of items modified by the command to undo
     * @param aTypeCommand = command type (see enum UNDO_REDO_T)
     * @param aAppend = add the item to the previous undo list
     * @param aTransformPoint = the reference point of the transformation,
     *                          for commands like move
     */
    void SaveCopyInUndoList( const PICKED_ITEMS_LIST& aItemsList,
                             UNDO_REDO_T aTypeCommand,
                             bool aAppend = false,
                             const wxPoint& aTransformPoint = wxPoint( 0, 0 ) );

    /**
     * Restore an undo or redo command to put data pointed by \a aList in the previous state.
     *
     * @param aList a PICKED_ITEMS_LIST pointer to the list of items to undo/redo
     * @param aRedoCommand  a bool: true for redo, false for undo
     */
    void PutDataInPreviousState( PICKED_ITEMS_LIST* aList, bool aRedoCommand );

    /**
     * Clone \a aItem and owns that clone in this container.
     */
    void SaveCopyForRepeatItem( SCH_ITEM* aItem );

    /**
     * Return the item which is to be repeated with the insert key.
     *
     * Such object is owned by this container, and must be cloned.
     */
    SCH_ITEM* GetRepeatItem() const             { return m_item_to_repeat; }

    /**
     * Swap the cloned item in member variable m_undoItem with \a aItem and saves it to
     * the undo list then swap the data back.
     *
     * This swaps the internal structure of the item with the cloned item.  It does not
     * swap the actual item pointers themselves.
     *
     * @param aItem The item to swap with the current undo item.
     * @param aAppend True if the action should be appended to the current undo record.
     */
    void SaveUndoItemInUndoList( SCH_ITEM* aItem, bool aAppend = false );

    /**
     * Performs an undo of the last edit WITHOUT logging a corresponding redo.  Used to cancel
     * an in-progress operation.
     */
    void RollbackSchematicFromUndo();

    /**
     * Create a symbol library file with the name of the root document plus the '-cache' suffix,
     *
     * This file will contain all components used in the current schematic.
     *
     * @param aUseCurrentSheetFilename = false to use the root sheet filename
     * (default) or true to use the currently opened sheet.
     * @return true if the file was written successfully.
     */
    bool CreateArchiveLibraryCacheFile( bool aUseCurrentSheetFilename = false );

    /**
     * Create a library \a aFileName that contains all components used in the current schematic.
     *
     * @param aFileName The full path and file name of the archive library.
     * @return True if \a aFileName was written successfully.
     */
    bool CreateArchiveLibrary( const wxString& aFileName );

    /**
     * Perform rescue operations to recover old projects from before certain changes were made.
     *
     * - Exports cached symbols that conflict with new symbols to a separate library.
     * - Exports cached symbols not found in any symbol library.
     * - Renames symbols named before libraries were case sensitive.
     *
     * @param aRunningOnDemand - indicates whether the tool has been called up by the user
     *      (as opposed to being run automatically). If true, an information dialog is
     *      displayed if there are no components to rescue. If false, the tool is silent
     *      if there are no components to rescue, and a "Never Show Again" button is
     *      displayed.
     */
    bool rescueProject( RESCUER& aRescuer, bool aRunningOnDemand );
    bool RescueLegacyProject( bool aRunningOnDemand );
    bool RescueSymbolLibTableProject( bool aRunningOnDemand );

    /**
     * Plot or print the current sheet to the clipboard.
     *
     * @param aDC = wxDC given by the calling print function
     */
    virtual void PrintPage( wxDC* aDC ) override;

    void SetNetListerCommand( const wxString& aCommand ) { m_netListerCommand = aCommand; }

    /**
     * Reset the execution flags to defaults for external netlist and bom generators.
     */
    void DefaultExecFlags() { m_exec_flags = wxEXEC_SYNC; }

    /**
     * Set (adds) specified flags for next execution of external generator of the netlist or bom.
     *
     * @param aFlags = wxEXEC_* flags, see wxExecute docs.
     */
    void SetExecFlags( const int aFlags ) { m_exec_flags |= aFlags; }

    /**
     * Clear (removes) specified flags that not needed for next execution of external generator
     * of the netlist or bom.
     *
     * @param aFlags = wxEXEC_* flags, see wxExecute docs.
     */
    void ClearExecFlags( const int aFlags ) { m_exec_flags &= ~( aFlags ); }

    wxString GetNetListerCommand() const { return m_netListerCommand; }

    /**
     * Generates the connection data for the entire schematic hierarchy.
     */
    void RecalculateConnections( bool aDoCleanup = true );

    /**
     * Allows Eeschema to install its preferences panels into the preferences dialog.
     */
    void InstallPreferences( PAGED_DIALOG* aParent, PANEL_HOTKEYS_EDITOR* aHotkeysPanel ) override;

    /**
     * Called after the preferences dialog is run.
     */
    void CommonSettingsChanged( bool aEnvVarsChanged ) override;

    void ShowChangedLanguage() override;

    void SyncToolbars() override;

    void SetScreen( BASE_SCREEN* aScreen ) override;

    const BOX2I GetDocumentExtents() const override;

    void FixupJunctions();

    DECLARE_EVENT_TABLE()
};


#endif  // SCH_EDIT_FRAME_H
