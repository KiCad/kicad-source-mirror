/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Alexander Shuklin <Jasuramme@gmail.com>
 * Copyright (C) 2004-2020 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef BACKANNOTATE_H
#define BACKANNOTATE_H

#include <deque>
#include <map>
#include <memory>
#include <sch_reference_list.h>
#include <template_fieldnames.h>
#include <wx/string.h>

// Forward declarations
class REPORTER;
class SCH_SHEET_LIST;
class SCH_EDIT_FRAME;


/**
 * @brief Back annotation algorithm class used to recieve, check, and apply a \ref NETLIST from
 * PCBNEW.
 * The following checks are made:
 * - Schematic symbol exists, but linked PCBnew module missing
 * - PCBnew module exists but no schematic symbol connected to
 * - PCBnew module is standalone
 * - Schematic sheet is reused one or more times and user trying to change footprint or value
 * only for few of them.
 * - Schematic symbols share same path
 * - More than one PCBnew module linked to same path

 */
class BACK_ANNOTATE
{
public:
    /**
     * @brief Struct to hold PCBnew module data
     */
    struct PCB_MODULE_DATA
    {
        PCB_MODULE_DATA( const wxString& aRef, const wxString& aFootprint,
                         const wxString& aValue, const std::map<wxString, wxString> aPinMap ) :
                m_ref( aRef ),
                m_footprint( aFootprint ),
                m_value( aValue ),
                m_pinMap( aPinMap )
        {};

        wxString m_ref;
        wxString m_footprint;
        wxString m_value;
        std::map<wxString, wxString> m_pinMap;
    };

    ///> Map to hold NETLIST modules data
    using PCB_MODULES_MAP = std::map<wxString, std::shared_ptr<PCB_MODULE_DATA>>;

    using CHANGELIST_ITEM = std::pair<SCH_REFERENCE, std::shared_ptr<PCB_MODULE_DATA>>;

    BACK_ANNOTATE( SCH_EDIT_FRAME* aFrame, REPORTER& aReporter, bool aProcessFootprints,
                   bool aProcessValues, bool aProcessReferences, bool aProcessNetNames,
                   bool aIgnoreOtherProjects, bool aDryRun );
    ~BACK_ANNOTATE();

    /**
     * @brief Get netlist from the PCBnew.
     * @param aNetlist reference to where netlist will be stored
     * @return true if success
     */
    bool FetchNetlistFromPCB( std::string& aNetlist );

    /**
     * @brief Run back annotation algorithm. If any errors, back annotation doesn't run.
     * only report
     * @param aNetlist netlist to run back annotation from
     * @return true if success
     */
    bool BackAnnotateSymbols( const std::string& aNetlist );

private:
    REPORTER&                    m_reporter;

    bool                         m_processFootprints;
    bool                         m_processValues;
    bool                         m_processReferences;
    bool                         m_processNetNames;
    bool                         m_ignoreOtherProjects;
    bool                         m_dryRun;

    PCB_MODULES_MAP              m_pcbModules;
    SCH_REFERENCE_LIST           m_refs;
    SCH_MULTI_UNIT_REFERENCE_MAP m_multiUnitsRefs;
    std::deque<CHANGELIST_ITEM>  m_changelist;
    SCH_EDIT_FRAME*              m_frame;

    ///> To count number of changes applied to the schematic
    int m_changesCount;

    ///> Get text from symbol's field ( such as Footprint or Value )
    wxString getTextFromField( const SCH_REFERENCE& aRef, const NumFieldType aField );

    /**
     * @brief Check if modules has different data. Check only if corresponding \ref m_boardAdapter
     * flag is rised
     * @param aFirst first module to compare
     * @param aSecond second module to compare
     * @return true if no violation
     */
    bool checkReuseViolation( PCB_MODULE_DATA& aFirst, PCB_MODULE_DATA& aSecond );

    /**
     * @brief Parse netlist sent over KiWay epress mail interface and fill \ref m_pcbModules
     * @param aPayload - netlist from PCBnew
     * @return number of errors during parsing
     */
    void getPcbModulesFromString( const std::string& aPayload );

    ///> Create changelist
    void getChangeList();

    /**
     * @brief Check if some symbols are not represented in PCB modules and vice versa.
     * \ref m_refs must be sorted by path
     */
    void checkForUnusedSymbols();

    /**
     * @brief Check for errors connected to reusing schematic in project or between projects
     */
    void checkSharedSchematicErrors();

    /**
    * @brief Apply changelist to the schematic
    */
    void applyChangelist();

    void processNetNameChange( SCH_CONNECTION* aConn, const wxString& aOldName,
                               const wxString& aNewName );
};

#endif
