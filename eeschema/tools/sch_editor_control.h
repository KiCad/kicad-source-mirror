/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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
 * Class SCH_EDITOR_CONTROL
 *
 * Handles actions specific to the schematic editor in eeschema.
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
    int SaveAll( const TOOL_EVENT& aEvent );
    int PageSetup( const TOOL_EVENT& aEvent );
    int Print( const TOOL_EVENT& aEvent );
    int Plot( const TOOL_EVENT& aEvent );
    int Quit( const TOOL_EVENT& aEvent );

    int FindAndReplace( const TOOL_EVENT& aEvent );

    int FindNext( const TOOL_EVENT& aEvent );
    bool HasMatch();
    int ReplaceAndFindNext( const TOOL_EVENT& aEvent );
    int ReplaceAll( const TOOL_EVENT& aEvent );

    int UpdateFind( const TOOL_EVENT& aEvent );

    ///> Notifies pcbnew about the selected item.
    int CrossProbeToPcb( const TOOL_EVENT& aEvent );

    ///> Equivalent to the above, but initiated by the user.  We also do SCH_SHEETs on this
    ///> one (they're too slow on big projects for the auto version above).
    int ExplicitCrossProbeToPcb( const TOOL_EVENT& aEvent );

#ifdef KICAD_SPICE
    int SimProbe( const TOOL_EVENT& aEvent );
    int SimTune( const TOOL_EVENT& aEvent );
#endif /* KICAD_SPICE */

    ///> Highlights net under the cursor.
    int HighlightNet( const TOOL_EVENT& aEvent );

    ///> Removes any net highlighting
    int ClearHighlight( const TOOL_EVENT& aEvent );

    ///> Updates net highlighting after an edit
    int UpdateNetHighlighting( const TOOL_EVENT& aEvent );

    ///> Launches a tool to highlight nets.
    int HighlightNetCursor( const TOOL_EVENT& aEvent );

    int Undo( const TOOL_EVENT& aEvent );
    int Redo( const TOOL_EVENT& aEvent );

    ///> Clipboard support.
    int Cut( const TOOL_EVENT& aEvent );
    int Copy( const TOOL_EVENT& aEvent );
    int Paste( const TOOL_EVENT& aEvent );

    int EditWithLibEdit( const TOOL_EVENT& aEvent );
    int ShowCvpcb( const TOOL_EVENT& aEvent );
    int Annotate( const TOOL_EVENT& aEvent );
    int EditSymbolFields( const TOOL_EVENT& aEvent );
    int EditSymbolLibraryLinks( const TOOL_EVENT& aEvent );
    int ShowPcbNew( const TOOL_EVENT& aEvent );
    int UpdatePCB( const TOOL_EVENT& aEvent );
    int ImportFPAssignments( const TOOL_EVENT& aEvent );
    int ExportNetlist( const TOOL_EVENT& aEvent );
    int GenerateBOM( const TOOL_EVENT& aEvent );
    int DrawSheetOnClipboard( const TOOL_EVENT& aEvent );

    int ShowBusManager( const TOOL_EVENT& aEvent );

    int EnterSheet( const TOOL_EVENT& aEvent );
    int LeaveSheet( const TOOL_EVENT& aEvent );
    int NavigateHierarchy( const TOOL_EVENT& aEvent );

    int ToggleHiddenPins( const TOOL_EVENT& aEvent );
    int ToggleForceHV( const TOOL_EVENT& aEvent );

    void BackAnnotateFootprints( const std::string& aChangedSetOfReferences );

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

private:
    ///> copy selection to clipboard
    bool doCopy();

    void doCrossProbeSchToPcb( const TOOL_EVENT& aEvent, bool aForce );

    /**
     * Loads a .cmp file from CvPcb and update the footprint field of components.
     *
     * Prepares parameters and calls ProcessCmpToFootprintLinkFileto actually read the file and
     * update the footprint fields
     */
    bool loadCmpToFootprintLinkFile();

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
    bool processCmpToFootprintLinkFile( const wxString& aFullFilename,
                                        bool            aForceVisibilityState,
                                        bool            aVisibilityState );

    ///> Sets up handlers for various events.
    void setTransitions() override;

private:
    bool      m_probingPcbToSch;    // Recursion guard when cross-probing to PCBNew
    EDA_ITEM* m_pickerItem;         // Current item for picker highlighting.
    wxString  m_pickerNet;
};


#endif // SCH_EDITOR_CONTROL_H
