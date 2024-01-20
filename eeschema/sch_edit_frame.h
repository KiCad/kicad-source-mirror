/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2023 CERN (www.cern.ch)
 * Copyright (C) 2004-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <stddef.h>
#include <vector>
#include <wx/cmndata.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/string.h>
#include <wx/treectrl.h>
#include <wx/utils.h>

#include <core/typeinfo.h>
#include <eda_base_frame.h>
#include <eeschema_settings.h>
#include <math/box2.h>
#include <sch_base_frame.h>
#include <template_fieldnames.h>

class SCH_ITEM;
class EDA_ITEM;
class SCH_LINE;
class SCH_TEXT;
class SCH_BITMAP;
class SCH_SHEET;
class SCH_SHEET_PATH;
class SCH_SHEET_PIN;
class SCH_SYMBOL;
class SCH_FIELD;
class SCH_JUNCTION;
class SCHEMATIC;
class SCH_COMMIT;
class DIALOG_BOOK_REPORTER;
class DIALOG_ERC;
class DIALOG_SYMBOL_FIELDS_TABLE;
class DIALOG_SCH_FIND;
class RESCUER;
class HIERARCHY_PANE;
class API_HANDLER_SCH;


/// Schematic search type used by the socket link with Pcbnew
enum SCH_SEARCH_T
{
    HIGHLIGHT_PIN,
    HIGHLIGHT_SYMBOL
};


enum SCH_CLEANUP_FLAGS
{
    NO_CLEANUP,
    LOCAL_CLEANUP,
    GLOBAL_CLEANUP
};


wxDECLARE_EVENT( EDA_EVT_SCHEMATIC_CHANGING, wxCommandEvent );
wxDECLARE_EVENT( EDA_EVT_SCHEMATIC_CHANGED, wxCommandEvent );


/**
 * Tree view item data for the net navigator.
 */
class NET_NAVIGATOR_ITEM_DATA : public wxTreeItemData
{
public:
    NET_NAVIGATOR_ITEM_DATA( const SCH_SHEET_PATH& aSheetPath, const SCH_ITEM* aItem ) :
        m_sheetPath( aSheetPath ),
        m_item( aItem )
    {
    }

    NET_NAVIGATOR_ITEM_DATA() :
        m_item( nullptr )
    {
    }

    SCH_SHEET_PATH& GetSheetPath() { return m_sheetPath; }
    const SCH_ITEM* GetItem() const { return m_item; }

    bool operator==( const NET_NAVIGATOR_ITEM_DATA& aRhs ) const
    {
        return ( m_sheetPath == aRhs.m_sheetPath ) && ( m_item == aRhs.m_item );
    }

    NET_NAVIGATOR_ITEM_DATA& operator=( const NET_NAVIGATOR_ITEM_DATA& aItemData )
    {
        if( this == &aItemData )
            return *this;

        m_sheetPath = aItemData.m_sheetPath;
        m_item = aItemData.m_item;

        return *this;
    }

private:
    SCH_SHEET_PATH m_sheetPath;
    const SCH_ITEM* m_item;
};


/**
 * Schematic editor (Eeschema) main window.
 */
class SCH_EDIT_FRAME : public SCH_BASE_FRAME
{
public:
    SCH_EDIT_FRAME( KIWAY* aKiway, wxWindow* aParent );
    ~SCH_EDIT_FRAME() override;

    SCH_SCREEN* GetScreen() const override;

    SELECTION& GetCurrentSelection() override;

    SCHEMATIC& Schematic() const;

    /**
     * Allow edit frame to show/hide hidden pins.
     */
    bool GetShowAllPins() const override;

    /**
     * Save changes to the project settings to the project (.pro) file.
     */
    void SaveProjectLocalSettings() override;

    /**
     * Load the KiCad project file (*.pro) settings specific to Eeschema.
     *
     * @return True if the project file was loaded correctly.
     */
    bool LoadProjectSettings();

    void ShowSchematicSetupDialog( const wxString& aInitialPage = wxEmptyString );

    void LoadSettings( APP_SETTINGS_BASE* aCfg ) override;
    void SaveSettings( APP_SETTINGS_BASE* aCfg ) override;

    void CreateScreens();
    void ReCreateHToolbar() override;
    void ReCreateVToolbar() override;
    void ReCreateOptToolbar() override;

    void setupUIConditions() override;

    /**
     * Get if the current schematic has been modified but not saved.
     *
     * @return true if the any changes have not been saved
     */
    bool IsContentModified() const override;

    /**
     * Must be called after a schematic change in order to set the "modify" flag and update other
     * data structures and GUI elements.
     */
    void OnModify() override;

    SEVERITY GetSeverity( int aErrorCode ) const override;

    /**
     * Return a human-readable description of the current screen.
     */
    wxString GetScreenDesc() const override;
    wxString GetFullScreenDesc() const override;

    /**
     * Execute a remote command sent via a socket on port KICAD_SCH_PORT_SERVICE_NUMBER (which
     * defaults to 4243).
     *
     * When user selects a footprint or pin in Pcbnew, Eeschema shows that same symbol or pin
     * and moves cursor on the item.
     *
     * Valid commands are:
     * \li \c \$PART: \c "reference" Put cursor on symbol.
     * \li \c \$PART: \c "reference" \c \$REF: \c "ref" Put cursor on symbol reference.
     * \li \c \$PART: \c "reference" \c \$VAL: \c "value" Put cursor on symbol value.
     * \li \c \$PART: \c "reference" \c \$PAD: \c "pin name" Put cursor on the symbol pin.
     * \li \c \$NET: \c "netname" Highlight a specified net
     * \li \c \$CLEAR: \c "HIGHLIGHTED" Clear symbols highlight
     * <p>
     * They are a keyword followed by a quoted string.
     * @param cmdline is the command received from Pcbnew.
     */
    void ExecuteRemoteCommand( const char* cmdline ) override;

    void KiwayMailIn( KIWAY_EXPRESS& aEvent ) override;

    /**
     * Refresh the display of any operaintg points.  Called after a .op simulation completes.
     */
    void RefreshOperatingPointDisplay();

    /**
     * Automatically set the rotation of an item (if the item supports it)
     */
    void AutoRotateItem( SCH_SCREEN* aScreen, SCH_ITEM* aItem );

    /**
     * Run the Find or Find & Replace dialog.
     */
    void ShowFindReplaceDialog( bool aReplace );

    /**
     * Update the hierarchy navigation tree and history
     */
    void UpdateHierarchyNavigator();

    /**
     * Update the hierarchy navigation tree labels.
     * No change for the tree, only the labels are updated, after editing a sheet
     * name or a sheet number.
     */
    void UpdateLabelsHierarchyNavigator();

    /**
     * Update the hierarchy navigation tree selection (cross-probe from schematic to hierarchy
     * pane).
     */
    void UpdateHierarchySelection();

    void ShowFindReplaceStatus( const wxString& aMsg, int aStatusTime );
    void ClearFindReplaceStatus();

    /**
     * Notification that the Find dialog has closed.
     */
    void OnFindDialogClose();

    /**
     * Break a single segment into two at the specified point.
     * @param aCommit Transaction container used to record changes for undo/redo
     * @param aSegment Line segment to break
     * @param aPoint Point at which to break the segment
     * @param aNewSegment Pointer to the newly created segment (if created)
     * @param aScreen is the screen to examine
     */
    void BreakSegment( SCH_COMMIT* aCommit, SCH_LINE* aSegment, const VECTOR2I& aPoint,
                       SCH_LINE** aNewSegment, SCH_SCREEN* aScreen );

    /**
     * Check every wire and bus for a intersection at \a aPoint and break into two segments
     * at \a aPoint if an intersection is found.
     * @param aCommit Transaction container used to record changes for undo/redo
     * @param aPoint Test this point for an intersection.
     * @param aScreen is the screen to examine.
     * @return True if any wires or buses were broken.
     */
    bool BreakSegments( SCH_COMMIT* aCommit, const VECTOR2I& aPoint, SCH_SCREEN* aScreen );

    /**
     * Test all junctions and bus entries in the schematic for intersections with wires and
     * buses and breaks any intersections into multiple segments.
     * @param aCommit Transaction container used to record changes for undo/redo
     * @param aScreen is the screen to examine.
     * @return True if any wires or buses were broken.
     */
    bool BreakSegmentsOnJunctions( SCH_COMMIT* aCommit, SCH_SCREEN* aScreen );

    /**
     * Test all of the connectable objects in the schematic for unused connection points.
     * @return True if any connection state changes were made.
     */
    void TestDanglingEnds();

    /**
     * Send items to board editor for selection.
     *
     * This is used for when the eeschema user is using the cross-probe tool.
     *
     * @param aItems are the items to select
     * @param aForce select the element in pcbnew whether or not the user has the select option
     *        chosen
     */
    void SendSelectItemsToPcb( const std::vector<EDA_ITEM*>& aItems, bool aForce );

    /**
     * Send a net name to Pcbnew for highlighting.
     *
     * @param aNetName is the name of a net, or empty string to clear highlight
     */
    void SendCrossProbeNetName( const wxString& aNetName );

    /**
     * Send a connection (net or bus) to Pcbnew for highlighting.
     *
     * @param aConnection is the connection to highlight
     */
    void SetCrossProbeConnection( const SCH_CONNECTION* aConnection );

    /**
     * Tell Pcbnew to clear the existing highlighted net, if one exists
     */
    void SendCrossProbeClearHighlight();

    const wxString& GetHighlightedConnection() const
    {
        return m_highlightedConn;
    }

    void SetHighlightedConnection( const wxString& aConnection,
                                   const NET_NAVIGATOR_ITEM_DATA* aSelection = nullptr );

    /**
     * Check if we are ready to write a netlist file for the current schematic.
     *
     * Test for some issues (missing or duplicate references and sheet names).
     *
     * @param aAnnotateMessage a message to put up in case annotation needs to be performed.
     * @return true if all is well (i.e. you can call WriteNetListFile next).
     */
    bool ReadyToNetlist( const wxString& aAnnotateMessage );

    /**
     * Create a netlist file.
     *
     * @param aFormat is the netlist format (NET_TYPE_PCBNEW ...).
     * @param aFullFileName is the full netlist file name.
     * @param aNetlistOptions is the netlist options using OR'ed bits.
     * <p>
     * For SPICE netlist only:
     *      if NET_USE_NETNAMES is set, use net names from labels in schematic
     *                             else use net numbers (net codes)
     *      if NET_USE_X_PREFIX is set : change "U" and "IC" reference prefix to "X"
     * </p>
     * @param aReporter is a #REPORTER to report error messages, can be a nullptr.
     * @return true if success.
     */
    bool WriteNetListFile( int aFormat, const wxString& aFullFileName, unsigned aNetlistOptions,
                           REPORTER* aReporter = nullptr );

    /**
     * Clear the current symbol annotation.
     *
     * @param aCurrentSheetOnly Where to clear the annotation. See #ANNOTATE_SCOPE_T
     */
    void DeleteAnnotation( ANNOTATE_SCOPE_T aAnnotateScope, bool aRecursive );

    /**
     * Annotate the symbols in the schematic that are not currently annotated. Multi-unit symbols
     * are annotated together. E.g. if two symbols were R8A and R8B, they may become R3A and
     * R3B, but not R3A and R3C or R3C and R4D.
     * @param aCommit Transaction container used to record changes for undo/redo
     * @param aAnnotateScope See #ANNOTATE_SCOPE_T
     * @param aSortOption Define the annotation order.  See #ANNOTATE_ORDER_T.
     * @param aAlgoOption Define the annotation style.  See #ANNOTATE_ALGO_T.
     * @param aRecursive  Annotation should descend into and annotate subsheets
     * @param aStartNumber The start number for non-sheet-based annotation styles.
     * @param aResetAnnotation Clear any previous annotation if true.  Otherwise, keep the
     *                         existing symbol annotation.
     * @param aRepairTimestamps Test for and repair any duplicate time stamps if true.
     *                          Otherwise, keep the existing time stamps.  This option
     *                          could change previous annotation because time stamps are
     *                          used to handle annotation in complex hierarchies.
     * @param aReporter A sink for error messages.  Use NULL_REPORTER if you don't need errors.
     *
     * When the sheet number is used in annotation, each sheet annotation starts from sheet
     * number * 100.  In other words the first sheet uses 100 to 199, the second sheet uses
     * 200 to 299, and so on.
     */
    void AnnotateSymbols( SCH_COMMIT* aCommit, ANNOTATE_SCOPE_T aAnnotateScope,
                          ANNOTATE_ORDER_T aSortOption, ANNOTATE_ALGO_T aAlgoOption,
                          bool aRecursive, int aStartNumber, bool aResetAnnotation,
                          bool aRepairTimestamps, REPORTER& aReporter );

    /**
     * Check for annotation errors.
     *
     * The following list of items are checked:
     * - Symbols that are not annotated.
     * - Duplicate symbol references.
     * - Multiple part per package symbols where the part number is greater number of parts in
     *   the package.
     * - Multiple part per package symbols where the reference designator is different between
     *   parts.
     *
     * @return Number of annotation errors found.
     * @param aReporter A handler for error reporting.
     * @param aAnnotateScope See #ANNOTATE_SCOPE_T Check the current sheet only if true.
     *                       Otherwise check the entire schematic.
     */
    int CheckAnnotate( ANNOTATION_ERROR_HANDLER aErrorHandler,
                       ANNOTATE_SCOPE_T         aAnnotateScope = ANNOTATE_ALL,
                       bool                     aRecursive = true );

    /**
     * Run a modal version of the annotate dialog for a specific purpose.
     *
     * @param aMessage A user message indicating the purpose.
     * @return the result of ShowModal()
     */
    int ModalAnnotate( const wxString& aMessage );

    // Functions used for hierarchy handling
    SCH_SHEET_PATH& GetCurrentSheet() const;

    void SetCurrentSheet( const SCH_SHEET_PATH& aSheet );

    void UpdateItem( EDA_ITEM* aItem, bool isAddOrDelete = false,
                     bool aUpdateRtree = false ) override;

    /**
     * Rebuild the GAL and redraw the screen.  Call when something went wrong.
     */
    void HardRedraw() override;

    /**
     * Draw the current sheet on the display.
     */
    void DisplayCurrentSheet();

    /**
     * Use the wxWidgets print code to draw an image of the current sheet onto the clipboard.
     */
    void DrawCurrentSheetToClipboard();

    /**
     * Called when modifying the page settings.
     *
     * In derived classes it can be used to modify parameters like draw area size, and any other
     * local parameter related to the page settings.
     */
    void OnPageSettingsChange() override;

    /**
     * Set the m_ScreenNumber and m_NumberOfScreens members for screens.
     *
     * @note This must be called after deleting or adding a sheet and when entering a sheet.
     */
    void SetSheetNumberAndCount();

    wxPageSetupDialogData& GetPageSetupData() { return m_pageSetupData; }

    void NewProject();
    void LoadProject();

    /**
     * Save the currently-open schematic (including its hierarchy) and associated project.
     *
     * @param aSaveAs is true to perform a Save As operation (rename the schematic and project).
     *                This may only be done in standalone mode.
     * @return true if the schematic was saved
     */
    bool SaveProject( bool aSaveAs = false );

    bool OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl = 0 ) override;

    wxString GetCurrentFileName() const override;

    /**
     * Import a KiCad schematic into the current sheet.
     *
     * @return True if the schematic was imported properly.
     */
    bool AppendSchematic();

    /**
     * Add a sheet file into the current sheet and updates display
     *
     * @note Used in AppendSchematic() and SCH_EDIT_TOOL::ddAppendFile() (so it is public)
     *
     * @param aFullFileName Path and name of sheet
     * @return True if the sheet was properly added
     */
    bool AddSheetAndUpdateDisplay( const wxString aFullFileName );

    /**
     * Check if any of the screens has unsaved changes and asks the user whether to save or
     * drop them.
     *
     * @return True if user decided to save or drop changes, false if the operation should be
     *         canceled.
     */
    bool AskToSaveChanges();

    SCH_JUNCTION* AddJunction( SCH_COMMIT* aCommit, SCH_SCREEN* aScreen, const VECTOR2I& aPos );

    /**
     * Perform routine schematic cleaning including breaking wire and buses and deleting
     * identical objects superimposed on top of each other.
     * @param aCommit Transaction container used to record changes for undo/redo
     * @param aScreen is the screen to examine, or nullptr to examine the current screen
     */
    void SchematicCleanUp( SCH_COMMIT* aCommit, SCH_SCREEN* aScreen = nullptr );

    /**
     * If any single wire passes through _both points_, remove the portion between the two points,
     * potentially splitting the wire into two.
     * @param aCommit Transaction container used to record changes for undo/redo
     * @param aStart The starting point for trimmming
     * @param aEnd The ending point for trimming
     * @return True if any wires were changed by this operation
     */
    bool TrimWire( SCH_COMMIT* aCommit, const VECTOR2I& aStart, const VECTOR2I& aEnd );

    void OnOpenPcbnew( wxCommandEvent& event );
    void OnOpenCvpcb( wxCommandEvent& event );
    void OnUpdatePCB( wxCommandEvent& event );
    void OnAnnotate( wxCommandEvent& event );

    /**
     * Verify that \a aSheet will not cause a recursion error in \a aCurrentSheet.
     *
     * @param aSheet is the #SCH_SHEET object to test.
     * @param aCurrentSheet is the #SCH_SHEET_PATH where \a aSheet is going to reside.
     * @return true if \a aSheet will cause a recursion error in \a aCurrentSheet.
     */
    bool CheckSheetForRecursion( SCH_SHEET* aSheet, SCH_SHEET_PATH* aCurrentSheet );

    /**
     * Check \a aSchematicFileName for a potential file name case sensitivity clashes.
     *
     * On platforms where file names are case sensitive, it is possible to schematic sheet
     * file names that would cause issues on platforms where file name are case insensitive.
     * File names foo.sch and Foo.sch are unique files on Linux and MacOS but on Windows
     * this would result in a broken schematic.
     *
     * @param aSchematicFileName is the absolute path and file name of the file to test.
     * @return true if the user accepts the potential file name clash risk.
     */
    bool AllowCaseSensitiveFileNameClashes( const wxString& aSchematicFileName );

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
     * it can happens when the edited sheet used an existing file, or becomes a new instance
     * of a already existing sheet.
     * @param aUpdateHierarchyNavigator is an optional flag to indicate the sheet changes require
     *                                  the hierarchy navigator panel to be updated.
     */
    bool EditSheetProperties( SCH_SHEET* aSheet, SCH_SHEET_PATH* aHierarchy,
                              bool* aClearAnnotationNewItems,
                              bool* aUpdateHierarchyNavigator = nullptr );

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
     * @param aCurrentSheet is the current position in the schematic hierarchy used to test for
     *                      possible file recursion issues.
     * @param aFileName is the file name to load.  The file name is expected to have an absolute
     *                  path.
     *
     * @return True if the schematic was imported properly.
     */
    bool LoadSheetFromFile( SCH_SHEET* aSheet, SCH_SHEET_PATH* aCurrentSheet,
                            const wxString& aFileName );

    /**
     * Removes a given junction and heals any wire segments under the junction
     *
     * @param aItem The junction to delete
     */
    void DeleteJunction( SCH_COMMIT* aCommit, SCH_ITEM* aItem );

    void FlipBodyStyle( SCH_SYMBOL* aSymbol );

    void SelectUnit( SCH_SYMBOL* aSymbol, int aUnit );

    void SetAltPinFunction( SCH_PIN* aPin, const wxString& aFunction );

    /* Undo - redo */

    /**
     * Create a copy of the current schematic item, and put it in the undo list.
     *
     *  aTypeCommand =
     *      CHANGED
     *      NEWITEM
     *      DELETED
     *
     * If it is a delete command, items are put on list with the .Flags member
     * set to DELETED.
     *
     * @param aItemToCopy is the schematic item modified by the command to undo.
     * @param aTypeCommand is the command type (see enum UNDO_REDO).
     * @param aAppend set to true to add the item to the previous undo list.
     * @param aDirtyConnectivity set to true if the change can affect connectivity.
     */
    void SaveCopyInUndoList( SCH_SCREEN* aScreen, SCH_ITEM* aItemToCopy, UNDO_REDO aTypeCommand,
                             bool aAppend, bool aDirtyConnectivity = true );

    /**
     * Create a new entry in undo list of commands.
     *
     * @param aItemsList is the list of items modified by the command to undo/
     * @param aTypeCommand is the command type (see enum UNDO_REDO).
     * @param aAppend set to true to add the item to the previous undo list.
     * @param aDirtyConnectivity set to true if the change can affect connectivity.
     */
    void SaveCopyInUndoList( const PICKED_ITEMS_LIST& aItemsList, UNDO_REDO aTypeCommand,
                             bool aAppend, bool aDirtyConnectivity = true );

    /**
     * Restore an undo or redo command to put data pointed by \a aList in the previous state.
     *
     * @param aList a PICKED_ITEMS_LIST pointer to the list of items to undo/redo
     */
    void PutDataInPreviousState( PICKED_ITEMS_LIST* aList );

    /**
     * Free the undo or redo list from \a aList element.
     *
     * - Wrappers are deleted.
     * - data pointed by wrappers are deleted if not in use in schematic
     *   i.e. when they are copy of a schematic item or they are no more in use (DELETED)
     *
     * @param whichList is the UNDO_REDO_CONTAINER to clear
     * @param aItemCount is the count of items to remove. Use < 0 to remove all items from
     *                   the beginning of the list.
     */
    void ClearUndoORRedoList( UNDO_REDO_LIST whichList, int aItemCount = -1 ) override;

    /**
     * Clone \a aItem and owns that clone in this container.
     */
    void SaveCopyForRepeatItem( const SCH_ITEM* aItem );
    void AddCopyForRepeatItem( const SCH_ITEM* aItem );

    /**
     * Return the items which are to be repeated with the insert key.  Such objects are owned by
     * this container, and must be cloned.
     */
    const std::vector<std::unique_ptr<SCH_ITEM>>& GetRepeatItems() const
    {
        return m_items_to_repeat;
    }

    EDA_ITEM* GetItem( const KIID& aId ) const override;

    /**
     * Perform an undo of the last edit WITHOUT logging a corresponding redo.  Used to cancel
     * an in-progress operation.
     */
    void RollbackSchematicFromUndo();

    /**
     * Create a library \a aFileName that contains all symbols used in the current schematic.
     *
     * @param aFileName The full path and file name of the archive library.
     * @return True if \a aFileName was written successfully.
     */
    bool CreateArchiveLibrary( const wxString& aFileName );

    /**
     * Plot or print the current sheet to the clipboard.
     */
    virtual void PrintPage( const RENDER_SETTINGS* aSettings ) override;

    void SetNetListerCommand( const wxString& aCommand ) { m_netListerCommand = aCommand; }

    /**
     * Reset the execution flags to defaults for external netlist and bom generators.
     */
    void DefaultExecFlags() { m_exec_flags = wxEXEC_SYNC; }

    /**
     * Set (adds) specified flags for next execution of external generator of the netlist or bom.
     *
     * @param aFlags is the wxEXEC_* flags, see wxExecute documentation.
     */
    void SetExecFlags( const int aFlags ) { m_exec_flags |= aFlags; }

    /**
     * Clear (removes) specified flags that not needed for next execution of external generator
     * of the netlist or bom.
     *
     * @param aFlags is the wxEXEC_* flags, see wxExecute documentation.
     */
    void ClearExecFlags( const int aFlags ) { m_exec_flags &= ~( aFlags ); }

    wxString GetNetListerCommand() const { return m_netListerCommand; }

    /**
     * Generate the connection data for the entire schematic hierarchy.
     */
    void RecalculateConnections( SCH_COMMIT* aCommit, SCH_CLEANUP_FLAGS aCleanupFlags );

    /**
     * Called after the preferences dialog is run.
     */
    void CommonSettingsChanged( bool aEnvVarsChanged, bool aTextVarsChanged ) override;

    void UpdateNetHighlightStatus();

    void ShowChangedLanguage() override;

    void SetScreen( BASE_SCREEN* aScreen ) override;

    const BOX2I GetDocumentExtents( bool aIncludeAllVisible = true ) const override;

    int GetSchematicJunctionSize();

    void FocusOnItem( SCH_ITEM* aItem );

    bool IsSyncingSelection() { return m_syncingPcbToSchSelection; }

    /**
     * Update a schematic symbol from a LIB_SYMBOL.
     *
     * This is typically called from the symbol editor when editing symbols in place.
     *
     * @param aSymbol is the new symbol data.
     * @param aSchematicSymbolUUID refers to the schematic symbol to update.
     */
    void SaveSymbolToSchematic( const LIB_SYMBOL& aSymbol, const KIID& aSchematicSymbolUUID );

    /**
     * Update the schematic's page reference map for all global labels, and refresh the labels
     * so that they are redrawn with up-to-date references.
     */
    void RecomputeIntersheetRefs();

    void ShowAllIntersheetRefs( bool aShow );

    /**
     * This overloaded version checks if the auto save master file "#auto_saved_files#" exists
     * and recovers all of the schematic files listed in it.
     *
     * @param aFileName is the project auto save master file name.
     */
    virtual void CheckForAutoSaveFile( const wxFileName& aFileName ) override;

    virtual void DeleteAutoSaveFile( const wxFileName& aFileName ) override;

    /**
     * Toggle the show/hide state of the left side schematic navigation panel
     */
    void ToggleSchematicHierarchy();

    /**
     * Toggle the show/hide state of Search pane
     */
    void ToggleSearch();

    void ToggleProperties() override;

    DIALOG_BOOK_REPORTER* GetSymbolDiffDialog();

    DIALOG_ERC* GetErcDialog();

    DIALOG_SYMBOL_FIELDS_TABLE* GetSymbolFieldsTableDialog();

    wxTreeCtrl* GetNetNavigator() { return m_netNavigator; }

    const SCH_ITEM* GetSelectedNetNavigatorItem() const;

    /**
     * @return the name of the wxAuiPaneInfo managing the Hierarchy Navigator panel
     */
    static const wxString SchematicHierarchyPaneName()
    {
        return wxT( "SchematicHierarchy" );
    }

    /**
     * @return the name of the wxAuiPaneInfo managing the Search panel
     */
    static const wxString SearchPaneName() { return wxT( "Search" ); }

    /**
     * Add \a aListener to post #EDA_EVT_SCHEMATIC_CHANGED command events to.
     *
     * @warning The caller is reponsible for removing any listeners that are no long valid.
     *
     * @note This only gets called when the schematic editor is in stand alone mode.  Changing
     *       projects in the project manager closes the schematic editor when a new project is
     *       loaded.
     */
    void AddSchematicChangeListener( wxEvtHandler* aListener );

    /**
     * Remove \a aListener to from the schematic changed listener list.
     */
    void RemoveSchematicChangeListener( wxEvtHandler* aListener );

    static const wxString NetNavigatorPaneName()
    {
        return wxS( "NetNavigator" );
    }

    void RefreshNetNavigator( const NET_NAVIGATOR_ITEM_DATA* aSelection = nullptr );

    void MakeNetNavigatorNode( const wxString& aNetName, wxTreeItemId aParentId,
                               const NET_NAVIGATOR_ITEM_DATA* aSelection = nullptr );

    void SelectNetNavigatorItem( const NET_NAVIGATOR_ITEM_DATA* aSelection = nullptr );

    void ToggleNetNavigator();

    PLUGIN_ACTION_SCOPE PluginActionScope() const override
    {
        return PLUGIN_ACTION_SCOPE::SCHEMATIC;
    }

    DECLARE_EVENT_TABLE()

protected:
    /**
     * Save the schematic files that have been modified and not yet saved.
     *
     * @return true if the auto save was successful otherwise false.
     */
    bool doAutoSave() override;

    void doReCreateMenuBar() override;

    /**
     * Send the KiCad netlist over to CVPCB.
     */
    void sendNetlistToCvpcb();

    void onSize( wxSizeEvent& aEvent );

    void saveProjectSettings() override;

    void onCloseSymbolDiffDialog( wxCommandEvent& aEvent );

    void onCloseErcDialog( wxCommandEvent& aEvent );

    void onCloseSymbolFieldsTableDialog( wxCommandEvent& aEvent );

    void unitsChangeRefresh() override;

    void updateSelectionFilterVisbility() override;

private:
    // Called when resizing the Hierarchy Navigator panel
    void OnResizeHierarchyNavigator( wxSizeEvent& aEvent );

    void onResizeNetNavigator( wxSizeEvent& aEvent );

    // Sets up the tool framework
    void setupTools();

    void OnExit( wxCommandEvent& event );

    void OnLoadFile( wxCommandEvent& event );
    void OnAppendProject( wxCommandEvent& event );
    void OnImportProject( wxCommandEvent& event );

    void OnClearFileHistory( wxCommandEvent& aEvent );

    bool canCloseWindow( wxCloseEvent& aCloseEvent ) override;
    void doCloseWindow() override;

    /**
     * Set the main window title bar text.
     *
     * If file name defined by SCH_SCREEN::m_FileName is not set, the title is set to the
     * application name appended with no file.
     * Otherwise, the title is set to the hierarchical sheet path and the full file name, and
     * read only is appended to the title if the user does not have write access to the file.
     */
    void updateTitle();

    /**
     * Initialize the zoom value of the current screen and mark the screen as zoom-initialized.
     */
    void initScreenZoom();

    /**
     * Verify that the symbol library links \a aSheet and all of its child sheets have
     * been remapped to the symbol library table.
     *
     * @param aSheet is the #SCH_SHEET object to test.
     *
     * @return true if \a aSheet and its child sheets have not been remapped.
     */
    bool checkForNoFullyDefinedLibIds( SCH_SHEET* aSheet );

    /**
     *  Load the given filename but sets the path to the current project path.
     *
     *  @param full filepath of file to be imported.
     *  @param aFileType SCH_FILE_T value for file type
     */
    bool importFile( const wxString& aFileName, int aFileType,
                     const STRING_UTF8_MAP* aProperties = nullptr );

    /**
     * Save \a aSheet to a schematic file.
     *
     * @param aSheet is the #SCH_SHEET object to save.
     * @param aSavePath is the full path of the destination file
     * @return True if the file has been saved.
     */
    bool saveSchematicFile( SCH_SHEET* aSheet, const wxString& aSavePath );

    /**
     * Fill a map of uuid -> reference from the currently loaded schematic.
     *
     * @param aMap is a map to fill
     */
    void mapExistingAnnotation( std::map<wxString, wxString>& aMap );

    bool updateAutoSaveFile();

    const wxString& getAutoSaveFileName() const;

    wxTreeCtrl* createHighlightedNetNavigator();

    void onNetNavigatorSelection( wxTreeEvent& aEvent );

    void onNetNavigatorSelChanging( wxTreeEvent& aEvent );

private:
    // The schematic editor control class should be able to access some internal
    // functions of the editor frame.
    friend class SCH_EDITOR_CONTROL;
    friend class SCH_FIND_REPLACE_TOOL;

    SCHEMATIC*              m_schematic;          ///< The currently loaded schematic
    wxString                m_highlightedConn;    ///< The highlighted net or bus or empty string.

    wxPageSetupDialogData   m_pageSetupData;
    std::vector<std::unique_ptr<SCH_ITEM>> m_items_to_repeat;  ///< For the repeat-last-item cmd

    wxString                m_netListerCommand;   ///< Command line to call a custom net list
                                                  ///< generator.
    int                     m_exec_flags;         ///< Flags of the wxExecute() function
                                                  ///< to call a custom net list generator.

    DIALOG_SCH_FIND*        m_findReplaceDialog;
    DIALOG_ERC*             m_ercDialog;
    DIALOG_BOOK_REPORTER*   m_diffSymbolDialog;
    HIERARCHY_PANE*         m_hierarchy;
    DIALOG_SYMBOL_FIELDS_TABLE* m_symbolFieldsTableDialog;


    wxTreeCtrl*             m_netNavigator;

	bool m_syncingPcbToSchSelection; // Recursion guard when synchronizing selection from PCB

    bool m_show_search;

    bool m_highlightedConnChanged;

    std::vector<wxEvtHandler*> m_schematicChangeListeners;

#ifdef KICAD_IPC_API
    std::unique_ptr<API_HANDLER_SCH> m_apiHandler;
#endif
};


#endif  // SCH_EDIT_FRAME_H
