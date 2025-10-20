/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Alexander Shuklin <Jasuramme@gmail.com>
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

#pragma once

#include <deque>
#include <map>
#include <set>
#include <memory>
#include <sch_reference_list.h>
#include <template_fieldnames.h>
#include <wx/string.h>

// Forward declarations
class REPORTER;
class SCH_SHEET_LIST;
class SCH_EDIT_FRAME;
class SCH_COMMIT;

/**
 * Back annotation algorithm class used to receive, check, and apply a \ref NETLIST from
 * Pcbnew.
 *
 * The following checks are made:
 * - Schematic symbol exists, but linked Pcbnew footprint missing.
 * - Pcbnew footprint exists but no schematic symbol connected to.
 * - Pcbnew footprint is standalone.
 * - Schematic sheet is reused one or more times and user trying to change footprint or value
 *   only for few of them.
 * - Schematic symbols share same path.
 * - More than one Pcbnew footprint linked to same path.
 */
class BACK_ANNOTATE
{
public:
    /**
     * Container for Pcbnew footprint data.
     */
    struct PCB_FP_DATA
    {
        PCB_FP_DATA( const wxString& aRef, const wxString& aFootprint, const wxString& aValue,
                     bool aDNP, bool aExcludeFromBOM,
                     const std::map<wxString, wxString>& aPinMap,
                     const std::map<wxString, wxString>& aFieldsMap ) :
                m_ref( aRef ),
                m_footprint( aFootprint ),
                m_value( aValue ),
                m_DNP( aDNP ),
                m_excludeFromBOM( aExcludeFromBOM ),
                m_pinMap( aPinMap ),
                m_fieldsMap( aFieldsMap )
        {}

        wxString                     m_ref;
        wxString                     m_footprint;
        wxString                     m_value;
        bool                         m_DNP;
        bool                         m_excludeFromBOM;
        std::map<wxString, wxString> m_pinMap;
        std::map<wxString, wxString> m_fieldsMap;
    };

    ///< Map to hold NETLIST footprints data
    using PCB_FOOTPRINTS_MAP = std::map<wxString, std::shared_ptr<PCB_FP_DATA>>;

    using CHANGELIST_ITEM = std::pair<SCH_REFERENCE, std::shared_ptr<PCB_FP_DATA>>;

    BACK_ANNOTATE( SCH_EDIT_FRAME* aFrame, REPORTER& aReporter, bool aRelinkFootprints,
                   bool aProcessFootprints, bool aProcessValues, bool aProcessReferences,
                   bool aProcessNetNames, bool aProcessAttributes, bool aProcessOtherFields,
                   bool aPreferUnitSwaps, bool aPreferPinSwaps, bool aDryRun );

    ~BACK_ANNOTATE() = default;

    /**
     * Get netlist from the Pcbnew.
     *
     * @param aNetlist is the netlist for the board editor.
     * @return true if success.
     */
    bool FetchNetlistFromPCB( std::string& aNetlist );

    void PushNewLinksToPCB();

    /**
     * Run back annotation algorithm. If any errors, back annotation doesn't run.
     *
     * @param aNetlist is the netlist to use for back annotation.
     * @return true if back annotation completed success.
     */
    bool BackAnnotateSymbols( const std::string& aNetlist );

private:
    /**
     * Parse netlist sent over KiWay express mail interface and fill \ref m_pcbModules.
     *
     * @param aPayload is the netlist from Pcbnew.
     * @return number of errors during parsing.
     */
    void getPcbModulesFromString( const std::string& aPayload );

    ///< Create changelist.
    void getChangeList();

    /**
     * Check if some symbols are not represented in PCB footprints and vice versa.
     *
     * \ref m_refs must be sorted by path.
     */
    void checkForUnusedSymbols();

    /**
     * Apply changelist to the schematic.
     */
    void applyChangelist();

    /**
     * Handle footprint pad net swaps with symbol pin swaps where possible.
     */
    std::set<wxString> applyPinSwaps( SCH_SYMBOL* aSymbol, const SCH_REFERENCE& aReference, const PCB_FP_DATA& aFpData,
                                      SCH_COMMIT* aCommit );

    void processNetNameChange( SCH_COMMIT* aCommit, const wxString& aRef, SCH_PIN* aPin,
                               const SCH_CONNECTION* aConnection, const wxString& aOldName, const wxString& aNewName );

private:
    REPORTER&                    m_reporter;

    bool                         m_matchByReference;
    bool                         m_processFootprints;
    bool                         m_processValues;
    bool                         m_processReferences;
    bool                         m_processNetNames;
    bool                         m_processAttributes;
    bool                         m_processOtherFields;
    bool                         m_preferUnitSwaps;
    bool                         m_preferPinSwaps;
    bool                         m_dryRun;

    PCB_FOOTPRINTS_MAP           m_pcbFootprints;
    SCH_REFERENCE_LIST           m_refs;
    SCH_MULTI_UNIT_REFERENCE_MAP m_multiUnitsRefs;
    std::deque<CHANGELIST_ITEM>  m_changelist;
    SCH_EDIT_FRAME*              m_frame;

    int                          m_changesCount;    // Number of user-level changes
};
