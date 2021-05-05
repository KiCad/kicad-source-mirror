/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2009-2020 KiCad Developers, see change_log.txt for contributors.
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
 * @file erc.h
 */

#ifndef _ERC_H
#define _ERC_H

#include <erc_settings.h>


class NETLIST_OBJECT;
class NETLIST_OBJECT_LIST;
class SCH_SHEET_LIST;
class SCHEMATIC;
class DS_PROXY_VIEW_ITEM;


extern const wxString CommentERC_H[];
extern const wxString CommentERC_V[];


class ERC_TESTER
{
public:

    ERC_TESTER( SCHEMATIC* aSchematic ) :
            m_schematic( aSchematic )
    {
    }

    /**
     * Perform ERC testing for electrical conflicts between \a NetItemRef and other items
     * (mainly pin) on the same net.
     *
     * @param aList = a reference to the list of connected objects
     * @param aNetItemRef = index in list of the current object
     * @param aNetStart = index in list of net objects of the first item
     * @param aMinConnexion = a pointer to a variable to store the minimal connection
     * found( NOD, DRV, NPI, NET_NC)
     */
    void TestOthersItems( NETLIST_OBJECT_LIST* aList, unsigned aNetItemRef, unsigned aNetStart,
                          int* aMinConnexion );

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
     * Check that there are no conflicting bus alias definitions in the schematic.
     *
     * (for example, two hierarchical sub-sheets contain different definitions for
     * the same bus alias)
     *
     * @return the error count
     */
    int TestConflictingBusAliases();

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
     * Checks for labels that differ only in capitalization
     * @return the error count
     */
    int TestSimilarLabels();

    /**
     * Test symbols for changed library symbols and broken symbol library links.
     * @return the number of issues found
     */
    int TestLibSymbolIssues();

private:

    SCHEMATIC* m_schematic;
};


#endif  // _ERC_H
