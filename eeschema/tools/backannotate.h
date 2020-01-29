/**
 * @file eeschema/tools/backannotate.h
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Alexander Shuklin <Jasuramme@gmail.com>
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
 * @brief Back annotation algorithm class. It used to recieve, check and apply \ref NETLIST
 * from PCBNEW. Class check for following collisions:
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
     * @brief settings struct to set up back annotation
     */
    struct SETTINGS
    {
        REPORTER& reporter;
        bool      processFootprints;
        bool      processValues;
        bool      processReferences;
        bool      ignoreOtherProjects;
        bool      dryRun;
    };

    /**
     * @brief Struct to hold PCBnew module data
     */
    struct PCB_MODULE_DATA
    {
        PCB_MODULE_DATA(wxString aRef, wxString aFootprint, wxString aValue) :
            ref(aRef),
            footprint(aFootprint),
            value(aValue){};
        wxString ref;
        wxString footprint;
        wxString value;
    };

    ///> Map to hold NETLIST modules data
    using PCB_MODULES_MAP = std::map<wxString, std::shared_ptr<PCB_MODULE_DATA>>;

    using CHANGELIST_ITEM = std::pair<SCH_REFERENCE, std::shared_ptr<PCB_MODULE_DATA>>;

    ///> To hold match between reference and PCBnew module
    using CHANGELIST = std::deque<CHANGELIST_ITEM>;


    BACK_ANNOTATE( SCH_EDIT_FRAME* aFrame, SETTINGS aSettings );
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
    SETTINGS                     m_settings;
    PCB_MODULES_MAP              m_pcbModules;
    SCH_REFERENCE_LIST           m_refs;
    SCH_MULTI_UNIT_REFERENCE_MAP m_multiUnitsRefs;
    CHANGELIST                   m_changelist;
    SCH_EDIT_FRAME*              m_frame;

    ///> To count number of changes applied to the schematic
    int m_changesCount;

    ///> Get text from symbol's field ( such as Footprint or Value )
    wxString getTextFromField( const SCH_REFERENCE& aRef, const NumFieldType aField );

    /**
     * @brief Check if modules has different data. Check only if corresponding \ref m_settings
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
    int getPcbModulesFromString( const std::string& aPayload );

    ///> Create changelist
    int getChangeList();

    /**
     * @brief Check if some symbols are not represented in PCB modules and vice versa.
     * \ref m_refs must be sorted by path
     * @return number of errors
     */
    int checkForUnusedSymbols();

    /**
     * @brief Check for errors connected to reusing schematic in project or between projects
     * @return number of errors
     */
    int checkSharedSchematicErrors();

    /**
    * @brief Apply changelist to the schematic
    */
    void applyChangelist();
};

#endif
