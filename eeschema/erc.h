/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2009-2015 KiCad Developers, see change_log.txt for contributors.
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


class NETLIST_OBJECT;
class NETLIST_OBJECT_LIST;
class SCH_SHEET_LIST;

/* For ERC markers: error types (used in diags, and to set the color):
*/
enum errortype
{
    OK  = 0,
    WAR,        // Error: severity = warning
    ERR,        // Error: severity = error
    UNC         // Error: unconnected pin
};

extern const wxString CommentERC_H[];
extern const wxString CommentERC_V[];

/// DRC error codes:
enum ERCE_T
{
    ERCE_UNSPECIFIED = 0,
    ERCE_DUPLICATE_SHEET_NAME,  // duplicate sheet names within a given sheet
    ERCE_PIN_NOT_CONNECTED,     // pin not connected and not no connect symbol
    ERCE_PIN_NOT_DRIVEN,        // pin connected to some others pins but no pin to drive it
    ERCE_PIN_TO_PIN_WARNING,    // pin connected to an other pin: warning level
    ERCE_PIN_TO_PIN_ERROR,      // pin connected to an other pin: error level
    ERCE_HIERACHICAL_LABEL,     // mismatch between hierarchical labels and pins sheets
    ERCE_NOCONNECT_CONNECTED,   // a no connect symbol is connected to more than 1 pin
    ERCE_NOCONNECT_NOT_CONNECTED, // a no connect symbol is not connected to anything
    ERCE_LABEL_NOT_CONNECTED,   // label not connected to anything
    ERCE_SIMILAR_LABELS,        // 2 labels are equal fir case insensitive comparisons
    ERCE_SIMILAR_GLBL_LABELS,   // 2 labels are equal fir case insensitive comparisons
    ERCE_DIFFERENT_UNIT_FP,     // different units of the same component have different footprints assigned
    ERCE_DIFFERENT_UNIT_NET,    // a shared pin in a multi-unit component is connected to more than one net
    ERCE_BUS_ALIAS_CONFLICT,    // conflicting bus alias definitions across sheets
    ERCE_DRIVER_CONFLICT,       // conflicting drivers (labels, etc) on a subgraph
    ERCE_BUS_ENTRY_CONFLICT,    // a wire connected to a bus doesn't match the bus
    ERCE_BUS_LABEL_ERROR,       // a label attached to a bus isn't in bus format
    ERCE_BUS_TO_BUS_CONFLICT,   // a connection between bus objects doesn't share at least one net
    ERCE_BUS_TO_NET_CONFLICT,   // a bus wire is graphically connected to a net port/pin (or vice versa)
    ERCE_GLOBLABEL,             // a global label is unique
};

/* Minimal connection table */
#define NPI    4  // Net with Pin isolated, this pin has type Not Connected and must be left N.C.
#define DRV    3  // Net driven by a signal (a pin output for instance)
#define NET_NC 2  // Net "connected" to a "NoConnect symbol"
#define NOD    1  // Net not driven ( Such as 2 or more connected inputs )
#define NOC    0  // initial state of a net: no connection


/**
 * Function WriteDiagnosticERC
 * save the ERC errors to \a aFullFileName.
 *
 * @param aFullFileName A wxString object containing the file name and path.
 */
bool WriteDiagnosticERC( EDA_UNITS_T aUnits, const wxString& aFullFileName );

/**
 * Performs ERC testing and creates an ERC marker to show the ERC problem for aNetItemRef
 * or between aNetItemRef and aNetItemTst.
 *  if MinConn < 0: this is an error on labels
 */
void Diagnose( NETLIST_OBJECT* NetItemRef, NETLIST_OBJECT* NetItemTst,
                      int MinConnexion, int Diag );

/**
 * Perform ERC testing for electrical conflicts between \a NetItemRef and other items
 * (mainly pin) on the same net.
 * @param aList = a reference to the list of connected objects
 * @param aNetItemRef = index in list of the current object
 * @param aNetStart = index in list of net objects of the first item
 * @param aMinConnexion = a pointer to a variable to store the minimal connection
 * found( NOD, DRV, NPI, NET_NC)
 */
void TestOthersItems( NETLIST_OBJECT_LIST* aList,
                             unsigned aNetItemRef, unsigned aNetStart,
                             int* aMinConnexion );

/**
 * Function TestDuplicateSheetNames( )
 * inside a given sheet, one cannot have sheets with duplicate names (file
 * names can be duplicated).
 * @return the error count
 * @param aCreateMarker: true = create error markers in schematic,
 *                       false = calculate error count only
 */
int TestDuplicateSheetNames( bool aCreateMarker );

/**
 * Checks that there are not conflicting bus alias definitions in the schematic
 *
 * (for example, two hierarchical sub-sheets contain different definitions for
 * the same bus alias)
 *
 * @param aCreateMarker: true = create error markers in schematic,
 *                       false = calculate error count only
 * @return the error count
 */
int TestConflictingBusAliases( bool aCreateMarker = true );

/**
 * Test if all units of each multiunit component have the same footprint assigned.
 * @param aSheetList contains components to be validated.
 * @return The error count.
 */
int TestMultiunitFootprints( SCH_SHEET_LIST& aSheetList );


#endif  // _ERC_H
