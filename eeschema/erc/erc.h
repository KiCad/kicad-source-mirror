/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#ifndef ERC_H
#define ERC_H

#include <erc/erc_settings.h>
#include <sch_sheet_path.h>
#include <sch_screen.h>
#include <sch_reference_list.h>
#include <connection_graph.h>
#include <vector>
#include <map>


class SCHEMATIC;
class DS_PROXY_VIEW_ITEM;
class SCH_EDIT_FRAME;
class PROGRESS_REPORTER;
struct KIFACE;
class PROJECT;
class SCH_RULE_AREA;


extern const wxString CommentERC_H[];
extern const wxString CommentERC_V[];


class ERC_TESTER
{
public:

    ERC_TESTER( SCHEMATIC* aSchematic, bool aShowAllErrors = false ) :
            m_schematic( aSchematic ),
            m_settings( aSchematic->ErcSettings() ),
            m_sheetList( aSchematic->BuildSheetListSortedByPageNumbers() ),
            m_screens( aSchematic->Root() ),
            m_nets( aSchematic->ConnectionGraph()->GetNetMap() ),
            m_showAllErrors( aShowAllErrors )
    {
        m_sheetList.GetMultiUnitSymbols( m_refMap, true );
    }

    /**
     * Inside a given sheet, one cannot have sheets with duplicate names (file
     * names can be duplicated).
     *
     * @return the error count
     * @param aCreateMarker: true = create error markers in schematic,
     *                       false = calculate error count only
     */
    int TestDuplicateSheetNames( bool aCreateMarker );

    /**
     * Check for any unresolved text variable references.
     */
    void TestTextVars( DS_PROXY_VIEW_ITEM* aDrawingSheet );

    /**
     * Test if all units of each multiunit symbol have the same footprint assigned.
     * @return The error count.
     */
    int TestMultiunitFootprints();

    /**
     * In KiCad 5 and earlier, you could connect stuff up to pins with NC electrical type.
     * In KiCad 6, this no longer results in those pins joining the net, so we need to warn about it
     * @return the error count
     */
    int TestNoConnectPins();

    /**
     * Checks the full netlist against the pin-to-pin connectivity requirements
     * @return the error count
     */
    int TestPinToPin();

    /**
     * Checks if shared pins on multi-unit symbols have been connected to different nets
     * @return the error count
     */
    int TestMultUnitPinConflicts();

    /**
     * Checks for ground-labeled pins not on a ground net while another pin is.
     * @return warning count
     */
    int TestGroundPins();

    /**
     * Checks for pin numbers that resemble stacked pin notation but are invalid.
     * @return warning count
     */
    int TestStackedPinNotation();

    /**
     * Checks for global and local labels with the same name
     * @return the error count
     */
    int TestSameLocalGlobalLabel();

    /**
     * Checks for labels that differ only in capitalization
     * @return the error count
     */
    int TestSimilarLabels();

    /**
     * Test to see if there are potentially confusing 4-way junctions in the schematic.
    */
    int TestFourWayJunction();

    /**
     * Test to see if there are labels that are connected to more than one wire.
     */
    int TestLabelMultipleWires();

    /**
     * Test symbols for changed library symbols and broken symbol library links.
     * @return the number of issues found
     */
    int TestLibSymbolIssues();

    /**
     * Test footprint links against the current footprint libraries.
     * @return the number of issues found
     */
    int TestFootprintLinkIssues( KIFACE* aCvPcb, PROJECT* aProject );

    /**
     * Test symbols to ensure that assigned footprint passes any given footprint filters.
     * @return the number of issues found
     */
    int TestFootprintFilters();

    /**
     * Test pins and wire ends for being off grid.
     * @return the error count
     */
    int TestOffGridEndpoints();

    /**
     * Test SPICE models for various issues.
     */
    int TestSimModelIssues();

    /**
     * Test for uninstantiated units of multi unit symbols
     */
    int TestMissingUnits();

    /**
     * Tests for netclasses that are referenced but not defined.
     * @return
     */
    int TestMissingNetclasses();

    /**
     * Tests for rule area ERC issues
     */
    int RunRuleAreaERC();

    void RunTests( DS_PROXY_VIEW_ITEM* aDrawingSheet, SCH_EDIT_FRAME* aEditFrame,
                   KIFACE* aCvPcb, PROJECT* aProject, PROGRESS_REPORTER* aProgressReporter );

private:
    SCHEMATIC*                   m_schematic;
    ERC_SETTINGS&                m_settings;
    SCH_SHEET_LIST               m_sheetList;
    SCH_SCREENS                  m_screens;
    SCH_MULTI_UNIT_REFERENCE_MAP m_refMap;
    const NET_MAP&               m_nets;
    bool                         m_showAllErrors;
};


#endif  // ERC_H
