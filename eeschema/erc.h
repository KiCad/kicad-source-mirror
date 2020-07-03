/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
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

namespace KIGFX
{
class WS_PROXY_VIEW_ITEM;
}


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
     * @param aList = a reference to the list of connected objects
     * @param aNetItemRef = index in list of the current object
     * @param aNetStart = index in list of net objects of the first item
     * @param aMinConnexion = a pointer to a variable to store the minimal connection
     * found( NOD, DRV, NPI, NET_NC)
     */
    void TestOthersItems( NETLIST_OBJECT_LIST* aList, unsigned aNetItemRef, unsigned aNetStart,
                          int* aMinConnexion );

    /**
     * inside a given sheet, one cannot have sheets with duplicate names (file
     * names can be duplicated).
     * @return the error count
     * @param aCreateMarker: true = create error markers in schematic,
     *                       false = calculate error count only
     */
    int TestDuplicateSheetNames( bool aCreateMarker );

    /**
     * Checks for any unresolved text variable references.
     */
    void TestTextVars( KIGFX::WS_PROXY_VIEW_ITEM* aWorksheet );

    /**
     * Checks that there are not conflicting bus alias definitions in the schematic
     *
     * (for example, two hierarchical sub-sheets contain different definitions for
     * the same bus alias)
     *
     * @return the error count
     */
    int TestConflictingBusAliases();

    /**
     * Test if all units of each multiunit component have the same footprint assigned.
     * @return The error count.
     */
    int TestMultiunitFootprints();

private:
    /**
     * Performs ERC testing and creates an ERC marker to show the ERC problem for aNetItemRef
     * or between aNetItemRef and aNetItemTst.
     *  if MinConn < 0: this is an error on labels
     */
    void diagnose( NETLIST_OBJECT* NetItemRef, NETLIST_OBJECT* NetItemTst, int MinConnexion,
                   PIN_ERROR Diag );

    SCHEMATIC* m_schematic;
};


#endif  // _ERC_H
