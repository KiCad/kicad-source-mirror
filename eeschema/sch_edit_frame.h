/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file sch_edit_frame.h
 */

#ifndef  SCH_EDIT_FRAME_H
#define  SCH_EDIT_FRAME_H

#include <sch_base_frame.h>
#include <config_params.h>
#include <undo_redo_container.h>
#include <template_fieldnames.h>
#include <block_commande.h>
#include <sch_collectors.h>
#include <sch_draw_panel.h>

// enum PINSHEETLABEL_SHAPE
#include <sch_text.h>

class LIB_EDIT_FRAME;
class LIB_VIEW_FRAME;
class DRAWSEGMENT;
class SCH_ITEM;
class SCH_NO_CONNECT;
class EDA_ITEM;
class SCH_BUS_ENTRY_BASE;
class SCH_BUS_WIRE_ENTRY;
class SCH_BUS_BUS_ENTRY;
class SCH_GLOBALLABEL;
class SCH_TEXT;
class SCH_BITMAP;
class SCH_SHEET;
class SCH_SHEET_PATH;
class SCH_SHEET_PIN;
class SCH_COMPONENT;
class SCH_FIELD;
class LIB_PIN;
class SCH_JUNCTION;
class DIALOG_SCH_FIND;
class DIALOG_ANNOTATE;
class wxFindDialogEvent;
class wxFindReplaceData;
class SCHLIB_FILTER;
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
    FIND_COMPONENT_ONLY,    ///< Find a component in the schematic.
    FIND_PIN,               ///< Find a component pin in the schematic.
    FIND_REFERENCE,         ///< Find an item by it's reference designator.
    FIND_VALUE,             ///< Find an item by it's value field.
    FIND_FIELD              ///< Find a component field.
};


/**
 * Schematic editor (Eeschema) main window.
 */
class SCH_EDIT_FRAME : public SCH_BASE_FRAME
{
private:
    SCH_SHEET_PATH*         m_CurrentSheet;    ///< which sheet we are presently working on.
    wxString                m_DefaultSchematicFileName;
    wxString                m_SelectedNetName;

    PARAM_CFG_ARRAY         m_projectFileParams;
    PARAM_CFG_ARRAY         m_configSettings;
    wxPageSetupDialogData   m_pageSetupData;
    wxFindReplaceData*      m_findReplaceData;
    wxString*               m_findReplaceStatus;
    bool                    m_printMonochrome;     ///< Print monochrome instead of grey scale.
    bool                    m_printSheetReference;
    DIALOG_SCH_FIND*        m_dlgFindReplace;
    wxArrayString           m_findStringHistoryList;
    wxArrayString           m_replaceStringHistoryList;
    BLOCK_SELECTOR          m_blockItems;         ///< List of selected items.
    SCH_ITEM*               m_item_to_repeat;     ///< Last item to insert by the repeat command.
    int                     m_repeatLabelDelta;   ///< Repeat label number increment step.
    SCH_COLLECTOR           m_collectedItems;     ///< List of collected items.
    SCH_FIND_COLLECTOR      m_foundItems;         ///< List of find/replace items.
    SCH_ITEM*               m_undoItem;           ///< Copy of the current item being edited.
    wxString                m_simulatorCommand;   ///< Command line used to call the circuit
                                                  ///< simulator (gnucap, spice, ...)
    wxString                m_netListerCommand;   ///< Command line to call a custom net list
                                                  ///< generator.
    int                     m_exec_flags;         ///< Flags of the wxExecute() function
                                                  ///< to call a custom net list generator.

    bool                    m_forceHVLines;       ///< force H or V directions for wires, bus, line

    bool                    m_autoplaceFields;    ///< automatically place component fields
    bool                    m_autoplaceJustify;   ///< allow autoplace to change justification
    bool                    m_autoplaceAlign;     ///< align autoplaced fields to the grid
    bool                    m_footprintPreview;   ///< whether to show footprint previews

    /// An index to the last find item in the found items list #m_foundItems.
    int         m_foundItemIndex;

    /// Flag to indicate show hidden pins.
    bool        m_showAllPins;

    /// The name of the destination directory to use when generating plot files.
    wxString    m_plotDirectoryName;

    /// The name of the format to use when generating a net list.
    wxString    m_netListFormat;

    /// Use netcodes (net number) as net names when generating spice net lists.
    bool        m_spiceAjustPassiveValues;

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
    virtual bool doAutoSave() override;

    /**
     * Returns true if the schematic has been modified.
     */
    virtual bool isAutoSaveRequired() const override;

    /**
     * Add the item currently being edited to the schematic and adds the changes to
     * the undo/redo container.
     */
    void addCurrentItemToScreen();

    void updateFindReplaceView( wxFindDialogEvent& aEvent );

    void backAnnotateFootprints( const std::string& aChangedSetOfReferences );

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
    void sendNetlist();

public:
    SCH_EDIT_FRAME( KIWAY* aKiway, wxWindow* aParent );
    ~SCH_EDIT_FRAME();

    SCH_SCREEN* GetScreen() const override;

    void OnCloseWindow( wxCloseEvent& Event );

    bool GetForceHVLines() const { return m_forceHVLines; }
    void SetForceHVLines( bool aForceHVdirection ) { m_forceHVLines = aForceHVdirection; }

    bool GetShowAllPins() const { return m_showAllPins; }
    void SetShowAllPins( bool aEnable ) { m_showAllPins = aEnable; }

    bool GetFootprintPreview() const { return m_footprintPreview; }
    void SetFootprintPreview( bool aEnable ) { m_footprintPreview = aEnable; }

    bool GetAutoplaceFields() const { return m_autoplaceFields; }
    void SetAutoplaceFields( bool aEnable ) { m_autoplaceFields = aEnable; }

    bool GetAutoplaceAlign() const { return m_autoplaceAlign; }
    void SetAutoplaceAlign( bool aEnable ) { m_autoplaceAlign = aEnable; }

    bool GetAutoplaceJustify() const { return m_autoplaceJustify; }
    void SetAutoplaceJustify( bool aEnable ) { m_autoplaceJustify = aEnable; }

    const wxString GetNetListFormatName() const { return m_netListFormat; }
    void SetNetListFormatName( const wxString& aFormat ) { m_netListFormat = aFormat; }

    bool GetSpiceAjustPassiveValues() const { return m_spiceAjustPassiveValues; }
    void SetSpiceAjustPassiveValues( bool aEnable ) { m_spiceAjustPassiveValues = aEnable; }

    /// accessor to the destination directory to use when generating plot files.
    const wxString& GetPlotDirectoryName() const { return m_plotDirectoryName; }
    void SetPlotDirectoryName( const wxString& aDirName ) { m_plotDirectoryName = aDirName; }

    void Process_Special_Functions( wxCommandEvent& event );
    void Process_Config( wxCommandEvent& event );
    void OnSelectTool( wxCommandEvent& aEvent );

    bool GeneralControl( wxDC* aDC, const wxPoint& aPosition, EDA_KEY aHotKey ) override;

    /**
     * Return the project file parameter list for Eeschema.
     *
     *<p>
     * Populate the project file parameter array specific to Eeschema if it hasn't
     * already been populated and return a reference to the array to the caller.
     * </p>
     */
    PARAM_CFG_ARRAY& GetProjectFileParametersList();

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

    /**
     * Return a default symbol field name for field \a aFieldNdx for all components.
     *
     * These field names are not modifiable, but template field names are.
     *
     * @param aFieldNdx The field number index
     */
    static wxString GetDefaultFieldName( int aFieldNdx );

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

    void RedrawActiveWindow( wxDC* DC, bool EraseBg ) override;

    void CreateScreens();
    void ReCreateHToolbar() override;
    void ReCreateVToolbar() override;
    void ReCreateOptToolbar();
    void ReCreateMenuBar() override;

    ///> @copydoc EDA_DRAW_FRAME::GetHotKeyDescription()
    EDA_HOTKEY* GetHotKeyDescription( int aCommand ) const override;

    bool OnHotKey( wxDC* aDC, int aHotKey, const wxPoint& aPosition, EDA_ITEM* aItem ) override;

    /**
     * Must be called after a schematic change in order to set the "modify" flag of the
     * current screen* and update the date in frame reference.
     */
    void OnModify();

    virtual wxString GetScreenDesc() const override;

    /**
     * Execute a remote command send by Pcbnew via a socket,
     * port KICAD_SCH_PORT_SERVICE_NUMBER (currently 4243)
     * this is a virtual function called by EDA_DRAW_FRAME::OnSockRequest().
     * @param cmdline = received command from socket
     */
    virtual void ExecuteRemoteCommand( const char* cmdline ) override;

    void KiwayMailIn( KIWAY_EXPRESS& aEvent ) override;

    void OnLeftClick( wxDC* aDC, const wxPoint& aPosition ) override;
    void OnLeftDClick( wxDC* aDC, const wxPoint& aPosition ) override;
    bool OnRightClick( const wxPoint& aPosition, wxMenu* PopMenu ) override;
    void OnSelectOptionToolbar( wxCommandEvent& event );
    double BestZoom() override;

    /**
     * Check the schematic at \a aPosition in logical (drawing) units for a item
     * matching the types in \a aFilterList.
     * <p>
     * The search is first performed at the nearest grid position to \a aPosition.  If no
     * item if found on grid, then \a aPosition is tested for any items.  If the item found
     * can be cross probed, a message is send to Pcbnew and the selected item is highlighted
     * in PCB editor.
     * </p>
     *
     * @param aPosition The wxPoint on the schematic to search.
     * @param aFilterList A list of #KICAD_T types to to filter.
     * @param aHotKeyCommandId A hot key command ID for performing additional tests when
     *                         multiple items are found at \a aPosition.
     * @param aClarifySelectionMenuCancelled is a pointer to a bool to handle a cancel command
     * from user when the user cancels the locate menu disambiguation (selection between located items)
     * @return A SCH_ITEM pointer of the item found or NULL if no item found
     */
    SCH_ITEM* LocateAndShowItem( const wxPoint& aPosition,
                                 const KICAD_T aFilterList[] = SCH_COLLECTOR::AllItems,
                                 int aHotKeyCommandId = 0,
                                 bool* aClarifySelectionMenuCancelled = nullptr );

    /**
     * Check for items at \a aPosition matching the types in \a aFilterList.
     * <p>
     * If multiple items are located at \a aPosition, a context menu is displayed to clarify
     * which item the user intended to select.  If the user aborts the context menu, NULL is
     * returned and the abort request flag will be set to true.  Make sure to clear this flag
     * before attempting to display any other context menus.
     * </p>
     *
     * @param aPosition The wxPoint location where to search.
     * @param aFilterList A list of #KICAD_T types to to filter.
     * @param aHotKeyCommandId A hot key command ID for performing additional tests when
     *                         multiple items are found at \a aPosition.
     * @return The SCH_ITEM pointer of the item found or NULL if no item found.
     */
    SCH_ITEM* LocateItem( const wxPoint& aPosition,
                          const KICAD_T aFilterList[] = SCH_COLLECTOR::AllItems,
                          int aHotKeyCommandId = 0 );

    /**
     * Delete the item found under the cross hair.  If multiple items are found at the
     * cross hair position, a context menu is displayed to clarify which item to delete.
     * See LocateItem() for more information on locating multiple items.
     *
     * @return True if an item was deleted.
     */
    bool DeleteItemAtCrossHair();


    /**
     * Highlight the connection found at aPosition.
     *
     * If no connection to highlight is found, clear the current highlighted connect (if any).
     *
     * @param aPosition is the location of the test point (usually cross hair position).
     * @return true if OK, false if there was an issue to build the netlist
     * needed to highlight a connection.
     */
    bool HighlightConnectionAtPosition( wxPoint aPosition );

    /**
     * Finds a component in the schematic and an item in this component.
     *
     * @param aReference The component reference designator to find.
     * @param aSearchHierarchy If false, search the current sheet only.  Otherwise,
     *                         the entire hierarchy
     * @param aSearchType A #SCH_SEARCH_T value used to determine what to search for.
     * @param aSearchText The text to search for, either in value, reference or elsewhere.
     */
    SCH_ITEM* FindComponentAndItem( const wxString& aReference,
                                    bool            aSearchHierarchy,
                                    SCH_SEARCH_T    aSearchType,
                                    const wxString& aSearchText );

    /**
     * Breaks a single segment into two at the specified point
     *
     * @param aSegment Line segment to break
     * @param aPoint Point at which to break the segment
     * @param aAppend Add the changes to the previous undo state
     * @param aNewSegment Pointer to the newly created segment (if given and created)
     * @return True if any wires or buses were broken.
     */
    bool BreakSegment( SCH_LINE* aSegment, const wxPoint& aPoint, bool aAppend = false,
            SCH_LINE** aNewSegment = NULL );

    /**
     * Checks every wire and bus for a intersection at \a aPoint and break into two segments
     * at \a aPoint if an intersection is found.
     *
     * @param aPoint Test this point for an intersection.
     * @param aAppend Add the changes to the previous undo state
     * @return True if any wires or buses were broken.
     */
    bool BreakSegments( const wxPoint& aPoint, bool aAppend = false );

    /**
     * Tests all junctions and bus entries in the schematic for intersections with wires and
     * buses and breaks any intersections into multiple segments.
     *
     * @param aAppend Add the changes to the previous undo state
     * @return True if any wires or buses were broken.
     */
    bool BreakSegmentsOnJunctions( bool aApped = false );

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
     * Sends a net name to eeschema for highlighting
     *
     * @param aNetName is the name of a net, or empty string to clear highlight
     */
    void SendCrossProbeNetName( const wxString& aNetName );

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
    bool CreateNetlist( int             aFormat,
                        const wxString& aFullFileName,
                        unsigned        aNetlistOptions,
                        REPORTER*       aReporter = NULL,
                        bool silent = false );

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
     * Called when modifying the page settings.
     * In derived classes it can be used to modify parameters like draw area size,
     * and any other local parameter related to the page settings.
     */
    void OnPageSettingsChange() override;

    /**
     * Set or reset the BRIGHTENED of connected objects inside the current sheet,
     * according to the highlighted net name.
     *
     * @param aItemsToRedrawList is the list of modified items (flag BRIGHTENED modified)
     * that must be redrawn.
     * Can be NULL
     * @return true if the flags are correctly set, and false if something goes wrong
     * (duplicate sheet names)
     */
    bool SetCurrentSheetHighlightFlags( std::vector<EDA_ITEM*>* aItemsToRedrawList );

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
    void OnPrint( wxCommandEvent& event );

    wxPageSetupDialogData& GetPageSetupData() { return m_pageSetupData; }

    bool GetPrintMonochrome() { return m_printMonochrome; }
    void SetPrintMonochrome( bool aMonochrome ) { m_printMonochrome = aMonochrome; }
    bool GetPrintSheetReference() { return m_printSheetReference; }
    void SetPrintSheetReference( bool aShow ) { m_printSheetReference = aShow; }

    // Plot functions:
    void PlotSchematic( wxCommandEvent& event );

    // read and save files
    void Save_File( wxCommandEvent& event );

    /**
     * Command event handler to save the entire project and create a component library archive.
     *
     * The component library archive name is &ltroot_name&gt-cache.lib
     */
    void OnSaveProject( wxCommandEvent& aEvent );
    bool SaveProject();

    bool OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl = 0 ) override;

    /**
     * Import a KiCad schematic into the current page.
     *
     * In order to import a schematic a lot of things have to happen to before the contents
     * of the imported schematic can be appended to the current page.  The following list
     * describes this process:
     *
     * - Load the schematic into a temporary SCH_SHEET object.
     * - Make sure the imported schematic does not cause any hierarchy recursion issues.
     * - Verify the imported schematic uses fully qualified #LIB_ID objects (symbol library table).
     * - Check to see if any symbol libraries need to be added to the current project's symbol
     *   library table.  This includes:
     *   - Check if the symbol library already exists in the project or global symbol library
     *     table.
     *   - Convert symbol library URLS that use the ${KIPRJMOD} environment variable to absolute
     *     paths.  ${KIPRJMOD} will not be the same for this project.
     *   - Check for duplicate symbol library nicknames and change the new symbol library nickname
     *     to prevent library name clashes.
     *   - Update all schematic symbol LIB_ID object library nicknames when the library nickname
     *     was changed to prevent clashes.
     * - Check for duplicate sheet names which is illegal and automatically rename any duplicate
     *   sheets in the imported schematic.
     * - Clear all of the annotation in the imported schematic to prevent clashes.
     * - Append the objects from the temporary sheet to the current page.
     * - Replace any duplicate time stamps.
     * - Refresh the symbol library links.
     *
     * @return True if the project was imported properly.
     */
    bool AppendSchematic();

    /**
     * Loads a .cmp file from CvPcb and update the footprint field of components.
     *
     * Prepares parameters and calls ProcessCmpToFootprintLinkFileto actually read the file and
     * update the footprint fields
     */
    bool LoadCmpToFootprintLinkFile();

    /**
     * Read the footprint info from each line in the stuff file by reference designator.
     *
     * The footprint link file (.cmp) entries created by CvPcb:
     *
     *  BeginCmp
     *  TimeStamp = /32307DE2/AA450F67;
     *  Reference = C1;
     *  ValeurCmp = 47uF;
     *  IdModule  = CP6;
     *  EndCmp
     *
     * @param aFullFilename = the full filename to read
     * @param aForceVisibilityState = Set to true to change the footprint field visibility
     *                                state to \a aVisibilityState.  False retains the
     *                                current footprint field visibility state.
     * @param aVisibilityState True to show the footprint field or false to hide the footprint
     *                         field if \a aForceVisibilityState is true.
     * @return bool = true if success.
     */
    bool ProcessCmpToFootprintLinkFile( const wxString& aFullFilename,
                                        bool            aForceVisibilityState,
                                        bool            aVisibilityState );

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

    // General search:

    bool IsSearchCacheObsolete( const SCH_FIND_REPLACE_DATA& aSearchCriteria );

    /**
     * Checks if any of the screens has unsaved changes and asks the user whether to save or
     * drop them.
     *
     * @return True if user decided to save or drop changes, false if the
     * operation should be canceled.
     */
    bool AskToSaveChanges();

private:

    /**
     * Handle the #ID_AUTOPLACE_FIELDS event.
     */
    void OnAutoplaceFields( wxCommandEvent& aEvent );

    /**
     * Handle the #ID_SCH_MOVE_ITEM event used to move schematic items.
     */
    void OnMoveItem( wxCommandEvent& aEvent );

    /**
     * Handle the #ID_SCH_ROTATE_CLOCKWISE and #ID_SCH_ROTATE_COUNTERCLOCKWISE events
     * used to rotate schematic items and blocks.
     */
    void OnRotate( wxCommandEvent& aEvent );

    /**
     * Handle the #ID_SCH_EDIT_ITEM event used to edit schematic items.
     */
    void OnEditItem( wxCommandEvent& aEvent );

    /**
     * Handle the #ID_POPUP_SCH_SELECT_ON_PCB event used to select items in Pcbnew
     * based on the sheet they are placed on.
     */
    void SelectAllFromSheet( wxCommandEvent& aEvent );

    /**
     * Handle the #ID_SCH_DRAG_ITEM event used to drag schematic items.
     */
    void OnDragItem( wxCommandEvent& aEvent );

    /**
     * Handle the #ID_SCH_MIRROR_X, #ID_SCH_MIRROR_Y, and #ID_SCH_ORIENT_NORMAL events
     * used to orient schematic items and blocks.
     */
    void OnOrient( wxCommandEvent& aEvent );

    void OnExit( wxCommandEvent& event );
    void OnAnnotate( wxCommandEvent& event );
    void OnErc( wxCommandEvent& event );
    void OnCreateNetlist( wxCommandEvent& event );
    void OnUpdatePCB( wxCommandEvent& event );
    void OnSimulate( wxCommandEvent& event );
    void OnCreateBillOfMaterials( wxCommandEvent& event );
    void OnLaunchBomManager( wxCommandEvent& event );
    void OnFindItems( wxCommandEvent& event );
    void OnFindDialogClose( wxFindDialogEvent& event );
    void OnFindDrcMarker( wxFindDialogEvent& event );
    void OnFindCompnentInLib( wxFindDialogEvent& event );

    /**
     * Find an item in the schematic matching the search criteria in \a aEvent.
     *
     * @param aEvent - Find dialog event containing the find parameters.
     */
    void OnFindSchematicItem( wxFindDialogEvent& aEvent );

    /**
     * Perform a search and replace of text in an item in the schematic matching the
     * search and replace criteria in \a aEvent.
     *
     * @param aEvent - Find dialog event containing the search and replace parameters.
     */
    void OnFindReplace( wxFindDialogEvent& aEvent );

    void OnLoadFile( wxCommandEvent& event );
    void OnLoadCmpToFootprintLinkFile( wxCommandEvent& event );
    void OnUpdateFields( wxCommandEvent& event );
    void OnNewProject( wxCommandEvent& event );
    void OnLoadProject( wxCommandEvent& event );
    void OnAppendProject( wxCommandEvent& event );
    void OnImportProject( wxCommandEvent& event );
    void OnOpenPcbnew( wxCommandEvent& event );
    void OnOpenPcbModuleEditor( wxCommandEvent& event );
    void OnOpenCvpcb( wxCommandEvent& event );
    void OnOpenLibraryEditor( wxCommandEvent& event );
    void OnRescueProject( wxCommandEvent& event );
    void OnRemapSymbols( wxCommandEvent& aEvent );

    // a helper function to run the dialog that allows to rename the symbol library Id of
    // groups of components, for instance after a symbol has moved from a library to
    // another library
    void OnEditComponentSymbolsId( wxCommandEvent& aEvent );
    void OnPreferencesOptions( wxCommandEvent& event );
    void OnCancelCurrentCommand( wxCommandEvent& aEvent );

    void OnSelectItem( wxCommandEvent& aEvent );

    /**
     * Command event handler for duplicating the item at the current location.
     */
    void OnCopySchematicItemRequest( wxCommandEvent& event );

    /* User interface update event handlers. */
    void OnUpdatePaste( wxUpdateUIEvent& event );
    void OnUpdateHiddenPins( wxUpdateUIEvent& event );
    void OnUpdateBusOrientation( wxUpdateUIEvent& event );
    void OnUpdateSelectTool( wxUpdateUIEvent& aEvent );
    void OnUpdateSave( wxUpdateUIEvent& aEvent );
    void OnUpdateSaveSheet( wxUpdateUIEvent& aEvent );
    void OnUpdateHierarchySheet( wxUpdateUIEvent& aEvent );
    void OnUpdateRemapSymbols( wxUpdateUIEvent& aEvent );

    /**
     * Close the ERC dialog if it is open.
     */
    void CloseErc();

    /**
     * Set the main window title bar text.
     *
     * If file name defined by SCH_SCREEN::m_FileName is not set, the title is set to the
     * application name appended with no file.
     * Otherwise, the title is set to the hierarchical sheet path and the full file name,
     * and read only is appended to the title if the user does not have write
     * access to the file.
     */
    void UpdateTitle();

    // Bus Entry
    SCH_BUS_WIRE_ENTRY* CreateBusWireEntry();
    SCH_BUS_BUS_ENTRY* CreateBusBusEntry();
    void SetBusEntryShape( wxDC* DC, SCH_BUS_ENTRY_BASE* BusEntry, char entry_shape );

    /**
     * Add no connect item to the current schematic sheet at \a aPosition.
     *
     * @param aPosition The position in logical (drawing) units to add the no connect.
     * @return The no connect item added.
     */
    SCH_NO_CONNECT* AddNoConnect( const wxPoint& aPosition );

    /**
     * Add a new junction at \a aPosition.
     */
    SCH_JUNCTION* AddJunction( const wxPoint& aPosition, bool aPutInUndoList = false );

    /**
     * Save a copy of the current wire image in the undo list.
     */
    void SaveWireImage();

    /**
     * Collects a unique list of all possible connection points in the schematic.
     *
     * @param aConnections vector of connections
     */
    void GetSchematicConnections( std::vector< wxPoint >& aConnections );

    /**
     * Performs routine schematic cleaning including breaking wire and buses and
     * deleting identical objects superimposed on top of each other.
     *
     * @param aAppend The changes to the schematic should be appended to the previous undo
     * @return True if any schematic clean up was performed.
     */
    bool SchematicCleanUp( bool aAppend = false );

    /**
     * If any single wire passes through _both points_, remove the portion between the two points,
     * potentially splitting the wire into two.
     *
     * @param aStart The starting point for trimmming
     * @param aEnd The ending point for trimming
     * @param aAppend Should the line changes be appended to a previous undo state
     * @return True if any wires were changed by this operation
     */
    bool TrimWire( const wxPoint& aStart, const wxPoint& aEnd, bool aAppend = true );

    /**
     * Start moving \a aItem using the mouse.
     *
     * @param aItem A pointer to an SCH_ITEM to move.
     * @param aDC The device context to draw \a aItem.
     */
    void PrepareMoveItem( SCH_ITEM* aItem );

    // Text, label, glabel
    SCH_TEXT* CreateNewText( int aType );
    void EditSchematicText( SCH_TEXT* TextStruct );
    void ChangeTextOrient( SCH_TEXT* aTextItem );

    /**
     * Command event handler to change a text type to another one.
     *
     * The new text, label, hierarchical label, or global label is created from the old text
     * and the old text object is deleted.
     */
    void OnConvertTextType( wxCommandEvent& aEvent );

    /**
     * Creates a new segment ( WIRE, BUS ) or terminates the current segment in progress.
     *
     * If the end of the current segment is on a different segment, place a junction if needed
     * and terminates the command.  If the end of the current segment is on a pin, terminate
     * the command.  In all other cases starts a new segment.
     */
    void BeginSegment( int type );

    /**
     * Terminate a bus, wire, or line creation.
     */
    void EndSegment();

    /**
     * Erase the last segment at the current mouse position.
     */
    void DeleteCurrentSegment();
    void DeleteConnection( bool DeleteFullConnection );

    // Images:
    SCH_BITMAP* CreateNewImage( wxDC* aDC );
    void RotateImage( SCH_BITMAP* aItem );

    /**
     * Mirror a bitmap.
     *
     * @param aItem = the SCH_BITMAP item to mirror
     * @param Is_X_axis = true to mirror relative to Horizontal axis
     *                      false to mirror relative to vertical axis
     */
    void MirrorImage( SCH_BITMAP* aItem, bool Is_X_axis );

    /**
     * Launches the "Edit Image" dialog to modify an image
     * @param aItem Pointer to the image item to modify
     * @return true if the image was modified, false if the user canceled
     */
    bool EditImage( SCH_BITMAP* aItem );

    // Hierarchical Sheet & PinSheet
    void        InstallHierarchyFrame( wxPoint& pos );
    SCH_SHEET*  CreateSheet( wxDC* DC );
    void        ReSizeSheet( SCH_SHEET* Sheet, wxDC* DC );

    /**
     * Rotate a sheet object.
     *
     * Sheets do not have a anchor point.  Because rotating it from its origin or its end is
     * not friendly, the rotation is made around its center.
     *
     * @param aSheet the hierarchical sheet to rotate
     * @param aRotCCW = true to rotate CCW, false to rotate CW
     */
    void RotateHierarchicalSheet( SCH_SHEET* aSheet, bool aRotCCW );

    /**
     * Mirror a hierarchical sheet.
     *
     * Mirroring is performed around its center.
     *
     * @param aSheet = the SCH_SHEET to mirror
     * @param aFromXaxis = true to mirror relative to Horizontal axis
     *                     false to mirror relative to vertical axis
     */
    void MirrorSheet( SCH_SHEET* aSheet, bool aFromXaxis );

    /**
     * Function EditLine
     * displays the dialog for editing the parameters of \a aLine.
     * @param aLine The Line/Wire/Bus to edit.
     * @param aRedraw = true to refresh the screen
     * @return The user response from the edit dialog.
     */
    int EditLine( SCH_LINE* aLine, bool aRedraw );

public:
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

    wxPoint GetLastSheetPinPosition() const { return m_lastSheetPinPosition; }

private:
    /**
     *  Load the given filename but sets the path to the current project path.
     *
     *  @param full filepath of file to be imported.
     *  @param aFileType SCH_FILE_T value for file type
     */
    bool importFile( const wxString& aFileName, int aFileType );

    bool validateSheet( SCH_SHEET* aSheet, SCH_SHEET_PATH* aHierarchy );

    /**
     * Create a new SCH_SHEET_PIN object and add it to \a aSheet at the current cursor position.
     *
     * @param aSheet The sheet to add the new sheet pin to.
     * @return The new sheet pin object created or NULL if the task was aborted by the user.
     */
    SCH_SHEET_PIN* CreateSheetPin( SCH_SHEET* aSheet );

    /**
     * Display the dialog for editing the parameters of \a aSheetPin.
     *
     * @param aSheetPin The sheet pin item to edit.
     * @param aRedraw = true to refresh the screen
     * @return The user response from the edit dialog.
     */
    int EditSheetPin( SCH_SHEET_PIN* aSheetPin, bool aRedraw );

    /**
     * Automatically create a sheet pin from the hierarchical labels in the schematic
     * referenced by \a aSheet.
     *
     * @param aSheet The sheet to import the new sheet pin to.
     * @return The new sheet pin object imported or NULL if the task was aborted by the user.
     */
    SCH_SHEET_PIN* ImportSheetPin( SCH_SHEET* aSheet );

public:
    /**
     * Remove \a aItem from the current screen and saves it in the undo list.
     *
     * @param aItem The item to remove from the current screen.
     * @param aAppend True if we are updating a previous Undo state
     */
    void DeleteItem( SCH_ITEM* aItem, bool aAppend = false );

    /**
     * Removes all items (and unused junctions that connect to them) and saves
     * each in the undo list
     *
     * @param aItemsList The list of items to delete
     * @param aAppend True if we are updating a previous commit
     */
    void DeleteItemsInList( PICKED_ITEMS_LIST& aItemsList, bool aAppend = false );

    /**
     * Removes a given junction and heals any wire segments under the junction
     *
     * @param aItem The junction to delete
     * @param aAppend True if we are updating an ongoing commit
     */
    void DeleteJunction( SCH_ITEM* aItem, bool aAppend = false );

    /**
     * Adds junctions if needed to each item in the list after they have been
     * moved.
     *
     * @param aItemsList The list of items to check
     * @param aAppend True if we are updating a previous commit
     */
    void CheckListConnections( PICKED_ITEMS_LIST& aItemsList, bool aAppend = false );

    int GetLabelIncrement() const { return m_repeatLabelDelta; }

private:

    /**
     * Load a symbol library and places it on the current schematic.
     *.
     * if libname != "", search in lib "libname"
     * else search in all loaded libs
     *
     * @param aFilter is a filter to pass the allowed lib names list, or library name
     * to load the component from and/or some other filters
     *          if NULL, no filtering.
     * @param aHistoryList     list remembering recently used component names.
     * @param aUseLibBrowser is the flag to determine if the library browser should be launched.
     * @return a pointer the SCH_COMPONENT object selected or NULL if no component was selected.
     * (TODO(hzeller): This really should be a class doing history, but didn't
     *  want to change too much while other refactoring is going on)
     */
    SCH_COMPONENT* Load_Component( const SCHLIB_FILTER*             aFilter,
                                   SCH_BASE_FRAME::HISTORY_LIST&    aHistoryList,
                                   bool                             aUseLibBrowser );

    /**
     * Display the edit component dialog to edit the parameters of \a aComponent.
     *
     * @param aComponent is a pointer to the SCH_COMPONENT object to be edited.
     */
    void EditComponent( SCH_COMPONENT* aComponent );

public:

    /**
     * Rotate and mirror a component.
     */
    void OrientComponent( COMPONENT_ORIENTATION_T aOrientation = CMP_NORMAL );

private:
    void OnSelectUnit( wxCommandEvent& aEvent );
    void ConvertPart( SCH_COMPONENT* DrawComponent );

    /**
     * Display the edit field dialog to edit the parameters of \a aField.
     *
     * @param aField is a pointer to the SCH_FIELD object to be edited.
     */
    void EditComponentFieldText( SCH_FIELD* aField );

    void RotateField( SCH_FIELD* aField );

    /**
     * Paste a list of items from the block stack.
     */
    void PasteListOfItems( wxDC* DC );

    /* Undo - redo */
public:

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

private:

    /**
     * Restore an undo or redo command to put data pointed by \a aList in the previous state.
     *
     * @param aList a PICKED_ITEMS_LIST pointer to the list of items to undo/redo
     * @param aRedoCommand  a bool: true for redo, false for undo
     */
    void PutDataInPreviousState( PICKED_ITEMS_LIST* aList, bool aRedoCommand );

    /**
     *  Redo the last edit.
     *
     *  - Save the current schematic in Undo list
     *  - Get an old version of the schematic from Redo list
     *
     *  @return none
     */
    void GetSchematicFromRedoList( wxCommandEvent& event );

    /**
     * Perform an undo the last edit.
     *
     *  - Save the current schematic in Redo list
     *  - Get an old version of the schematic from Undo list
     */
    void GetSchematicFromUndoList( wxCommandEvent& event );

    /**
     * Copy the list of block item.
     *
     * @sa m_blockItems
     * @param aItemsList List to copy the block select items into.
     */
    void copyBlockItems( PICKED_ITEMS_LIST& aItemsList, const wxPoint& aMoveVector );

    /**
     * Add the context menu items to \a aMenu for \a aJunction.
     *
     * @param aMenu The menu to add the items to.
     * @param aJunction The SCH_JUNCTION object selected.
     */
    void addJunctionMenuEntries( wxMenu* aMenu, SCH_JUNCTION* aJunction );

public:
    void Key( wxDC* DC, int hotkey, EDA_ITEM* DrawStruct );

    /**
     * Initialize the parameters used by the block paste command.
     */
    void InitBlockPasteInfos() override;

    /**
     * Return the block command internal code (BLOCK_MOVE, BLOCK_DUPLICATE...)
     * corresponding to the keys pressed (ALT, SHIFT, SHIFT ALT ..) when
     * block command is started by dragging the mouse.
     *
     * @param aKey = the key modifiers (Alt, Shift ...)
     * @return the block command id (BLOCK_MOVE, BLOCK_DUPLICATE...)
     */
    virtual int BlockCommand( EDA_KEY aKey ) override;

    /**
     * Call after HandleBlockEnd, when a block command needs to be executed after the block
     * is moved to its new place.
     *
     * Parameters must be initialized in GetScreen()->m_BlockLocate
     */
    virtual void HandleBlockPlace( wxDC* DC ) override;

    /**
     * Handle the "end"  of a block command,
     * i.e. is called at the end of the definition of the area of a block.
     * depending on the current block command, this command is executed
     * or parameters are initialized to prepare a call to HandleBlockPlace
     * in GetScreen()->m_BlockLocate
     *
     * @param aDC is a device context to draw on.
     * @return false if no item selected, or command finished,
     * true if some items found and HandleBlockPlace must be called later
     */
    virtual bool HandleBlockEnd( wxDC* aDC ) override;

    /**
     * Repeat the last item placement if the last item was a bus, bus entry,
     * label, or component.
     *
     * Labels that end with a number will be incremented.
     */
    void RepeatDrawItem();

    /**
     * Clone \a aItem and owns that clone in this container.
     */
    void SetRepeatItem( SCH_ITEM* aItem );

    /**
     * Return the item which is to be repeated with the insert key.
     *
     * Such object is owned by this container, and must be cloned.
     */
    SCH_ITEM* GetRepeatItem() const             { return m_item_to_repeat; }

    /**
     * Clone \a aItem which can be used to restore the state of the item being edited
     * when the user cancels the editing in progress.
     *
     * @param aItem The item to make a clone of for undoing the last change.  Set to
     *              NULL to free the current undo item.
     */
    void SetUndoItem( const SCH_ITEM* aItem );

    SCH_ITEM* GetUndoItem() const { return m_undoItem; }

    /**
     * Swap the cloned item in member variable m_undoItem with \a aItem and saves it to
     * the undo list then swap the data back.
     *
     * This swaps the internal structure of the item with the cloned item.  It does not
     * swap the actual item pointers themselves.
     *
     * @param aItem The item to swap with the current undo item.
     */
    void SaveUndoItemInUndoList( SCH_ITEM* aItem );

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
     * @param aPrintMask = not used here
     * @param aPrintMirrorMode = not used here (Set when printing in mirror mode)
     * @param aData = a pointer on an auxiliary data (not always used, NULL if not used)
     */
    virtual void PrintPage( wxDC* aDC, LSET aPrintMask,
                            bool aPrintMirrorMode, void* aData = NULL ) override;

    void SetSimulatorCommand( const wxString& aCommand ) { m_simulatorCommand = aCommand; }

    wxString GetSimulatorCommand() const { return m_simulatorCommand; }

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
     * Updates netlist and sends it to pcbnew.
     * @param aUpdateOptions is a string defining update options:
     * - "no-annotate" does not perform schematic annotation
     * - "quiet-annotate" performs schematic annotation without showing annotation dialog
     * aUpdateOptions may also contain other options accepted for netlist reader.
     * @see PCB_EDIT_FRAME::KiwayMailIn()
     */
    void doUpdatePcb( const wxString& aUpdateOptions = "" );

    void SetCurrentSheet( SCH_SHEET_PATH *aSheet );

    /**
     * Allows Eeschema to install its preferences panels into the preferences dialog.
     */
    void InstallPreferences( PAGED_DIALOG* aParent ) override;

    /**
     * Called after the preferences dialog is run.
     */
    void CommonSettingsChanged() override;

    void ShowChangedLanguage() override;

    void DuplicateItemsInList( SCH_SCREEN* screen, PICKED_ITEMS_LIST& aItemsList,
                               const wxPoint& aMoveVector );

    virtual void SetScreen( BASE_SCREEN* aScreen ) override;

    virtual const BOX2I GetDocumentExtents() const override;

    ///> Probe cursor, used by circuit simulator
    const static wxCursor CURSOR_PROBE;

    ///> Tuner cursor, used by circuit simulator
    const static wxCursor CURSOR_TUNE;

    DECLARE_EVENT_TABLE()
};


#endif  // SCH_EDIT_FRAME_H
