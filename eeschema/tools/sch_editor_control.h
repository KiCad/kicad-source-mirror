/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef SCH_EDITOR_CONTROL_H
#define SCH_EDITOR_CONTROL_H

#include <sch_base_frame.h>
#include <tools/ee_tool_base.h>
#include <status_popup.h>

class SCH_EDIT_FRAME;

/**
 * Handle actions specific to the schematic editor.
 */
class SCH_EDITOR_CONTROL : public wxEvtHandler, public EE_TOOL_BASE<SCH_EDIT_FRAME>
{
public:
    SCH_EDITOR_CONTROL()  :
            EE_TOOL_BASE<SCH_EDIT_FRAME>( "eeschema.EditorControl" ),
            m_probingPcbToSch( false ),
            m_pickerItem( nullptr )
    { }

    ~SCH_EDITOR_CONTROL() { }

    int New( const TOOL_EVENT& aEvent );
    int Open( const TOOL_EVENT& aEvent );
    int Save( const TOOL_EVENT& aEvent );
    int SaveAs( const TOOL_EVENT& aEvent );

    /// Saves the currently-open schematic sheet to an other name
    int SaveCurrSheetCopyAs( const TOOL_EVENT& aEvent );
    int ShowSchematicSetup( const TOOL_EVENT& aEvent );
    int PageSetup( const TOOL_EVENT& aEvent );
    int Print( const TOOL_EVENT& aEvent );
    int Plot( const TOOL_EVENT& aEvent );
    int Quit( const TOOL_EVENT& aEvent );

    /**
     * Perform rescue operations to recover old projects from before certain changes were made.
     *
     * - Exports cached symbols that conflict with new symbols to a separate library.
     * - Exports cached symbols not found in any symbol library.
     * - Renames symbols named before libraries were case sensitive.
     */
    int RescueSymbols( const TOOL_EVENT& aEvent );
    int RemapSymbols( const TOOL_EVENT& aEvent );

    bool RescueLegacyProject( bool aRunningOnDemand );
    bool RescueSymbolLibTableProject( bool aRunningOnDemand );

    int FindAndReplace( const TOOL_EVENT& aEvent );

    int FindNext( const TOOL_EVENT& aEvent );
    bool HasMatch();
    int ReplaceAndFindNext( const TOOL_EVENT& aEvent );
    int ReplaceAll( const TOOL_EVENT& aEvent );

    int UpdateFind( const TOOL_EVENT& aEvent );

    ///< Notifies pcbnew about the selected item.
    int CrossProbeToPcb( const TOOL_EVENT& aEvent );

    ///< Equivalent to the above, but initiated by the user.  We also do SCH_SHEETs on this
    ///< one (they're too slow on big projects for the auto version above).
    int ExplicitCrossProbeToPcb( const TOOL_EVENT& aEvent );

#ifdef KICAD_SPICE
    int SimProbe( const TOOL_EVENT& aEvent );
    int SimTune( const TOOL_EVENT& aEvent );
#endif /* KICAD_SPICE */

    ///< Highlight net under the cursor.
    int HighlightNet( const TOOL_EVENT& aEvent );

    ///< Remove any net highlighting
    int ClearHighlight( const TOOL_EVENT& aEvent );

    ///< Update net highlighting after an edit
    int UpdateNetHighlighting( const TOOL_EVENT& aEvent );

    ///< Launch a tool to highlight nets.
    int HighlightNetCursor( const TOOL_EVENT& aEvent );

    int AssignNetclass( const TOOL_EVENT& aEvent );

    int Undo( const TOOL_EVENT& aEvent );
    int Redo( const TOOL_EVENT& aEvent );

    ///< Clipboard support.
    int Cut( const TOOL_EVENT& aEvent );
    int Copy( const TOOL_EVENT& aEvent );
    int Paste( const TOOL_EVENT& aEvent );
    int Duplicate( const TOOL_EVENT& aEvent );

    int EditWithSymbolEditor( const TOOL_EVENT& aEvent );
    int ShowCvpcb( const TOOL_EVENT& aEvent );
    int Annotate( const TOOL_EVENT& aEvent );
    int EditSymbolFields( const TOOL_EVENT& aEvent );
    int EditSymbolLibraryLinks( const TOOL_EVENT& aEvent );
    int ShowPcbNew( const TOOL_EVENT& aEvent );
    int UpdatePCB( const TOOL_EVENT& aEvent );
    int UpdateFromPCB( const TOOL_EVENT& aEvent );
    int ImportFPAssignments( const TOOL_EVENT& aEvent );
    int ExportNetlist( const TOOL_EVENT& aEvent );
    int GenerateBOM( const TOOL_EVENT& aEvent );
    int DrawSheetOnClipboard( const TOOL_EVENT& aEvent );

    int ShowBusManager( const TOOL_EVENT& aEvent );

    int EnterSheet( const TOOL_EVENT& aEvent );
    int LeaveSheet( const TOOL_EVENT& aEvent );
    int NavigateHierarchy( const TOOL_EVENT& aEvent );

    int ToggleHiddenPins( const TOOL_EVENT& aEvent );
    int ToggleHiddenFields( const TOOL_EVENT& aEvent );
    int ToggleForceHV( const TOOL_EVENT& aEvent );
    int TogglePythonConsole( const TOOL_EVENT& aEvent );

    int RepairSchematic( const TOOL_EVENT& aEvent );

    void AssignFootprints( const std::string& aChangedSetOfReferences );

    /**
     * Find a symbol in the schematic and an item in this symbol.
     *
     * @param aReference The symbol reference designator to find.
     * @param aSearchHierarchy If false, search the current sheet only.  Otherwise,
     *                         the entire hierarchy
     * @param aSearchType A #SCH_SEARCH_T value used to determine what to search for.
     * @param aSearchText The text to search for, either in value, reference or elsewhere.
     */
    SCH_ITEM* FindSymbolAndItem( const wxString& aReference, bool aSearchHierarchy,
                                 SCH_SEARCH_T aSearchType, const wxString& aSearchText );

private:
    ///< copy selection to clipboard or to m_localClipboard if aUseLocalClipboard is true
    bool doCopy( bool aUseLocalClipboard = false );

    bool rescueProject( RESCUER& aRescuer, bool aRunningOnDemand );

    bool searchSupplementaryClipboard( const wxString& aSheetFilename, SCH_SCREEN** aScreen );

    void doCrossProbeSchToPcb( const TOOL_EVENT& aEvent, bool aForce );

    void updatePastedSymbol( SCH_SYMBOL* aSymbol, SCH_SCREEN* aPasteScreen,
                             const SCH_SHEET_PATH& aPastePath, const KIID_PATH& aClipPath,
                             bool aForceKeepAnnotations );

    SCH_SHEET_PATH updatePastedSheet( const SCH_SHEET_PATH& aPastePath, const KIID_PATH& aClipPath,
                                      SCH_SHEET* aSheet, bool aForceKeepAnnotations,
                                      SCH_SHEET_LIST* aPastedSheetsSoFar,
                                      SCH_REFERENCE_LIST* aPastedSymbolsSoFar );

    void setClipboardInstances( const SCH_SCREEN* aPastedScreen );

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
    bool processCmpToFootprintLinkFile( const wxString& aFullFilename, bool aForceVisibilityState,
                                        bool aVisibilityState );

    ///< Set up handlers for various events.
    void setTransitions() override;

    /**
     * Advance the search and returns the next matching item after \a aAfter.
     *
     * @param aScreen Pointer to the screen used for searching
     * @param aAfter Starting match to compare
     * @param aData Search data to compare against or NULL to match the first item found
     * @return pointer to the next search item found or NULL if nothing found
     */
    SCH_ITEM* nextMatch( SCH_SCREEN* aScreen, SCH_SHEET_PATH* aSheet, SCH_ITEM* aAfter,
                         wxFindReplaceData& aData );

private:
    bool      m_probingPcbToSch; // Recursion guard when cross-probing to PcbNew
    EDA_ITEM* m_pickerItem;      // Current item for picker highlighting.

    // Temporary storage location for Duplicate action
    std::string m_localClipboard;

    // A map of sheet filename --> screens for the clipboard contents.  We use these to hook up
    // cut/paste operations for unsaved sheet content.
    std::map<wxString, SCH_SCREEN*> m_supplementaryClipboard;

    // A map of KIID_PATH --> symbol instances for the clipboard contents.
    std::map<KIID_PATH, SYMBOL_INSTANCE_REFERENCE> m_clipboardSymbolInstances;

    // A map of KIID_PATH --> sheet instances for the clipboard contents.
    std::map<KIID_PATH, SCH_SHEET_INSTANCE> m_clipboardSheetInstances;
};


#endif // SCH_EDITOR_CONTROL_H
