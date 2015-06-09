/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2009-2011 KiCad Developers, see change_log.txt for contributors.
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


class EDA_DRAW_PANEL;
class NETLIST_OBJECT;
class NETLIST_OBJECT_LIST;

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
#define ERCE_UNSPECIFIED          0
#define ERCE_DUPLICATE_SHEET_NAME 1    // duplicate sheet names within a given sheet
#define ERCE_PIN_NOT_CONNECTED    2    // pin not connected and not no connect symbol
#define ERCE_PIN_NOT_DRIVEN       3    // pin connected to some others pins but no pin to drive it
#define ERCE_PIN_TO_PIN_WARNING   4    // pin connected to an other pin: warning level
#define ERCE_PIN_TO_PIN_ERROR     5    // pin connected to an other pin: error level
#define ERCE_HIERACHICAL_LABEL    6    // mismatch between hierarchical labels and pins sheets
#define ERCE_NOCONNECT_CONNECTED  7    // a no connect symbol is connected to more than 1 pin

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
extern bool WriteDiagnosticERC( const wxString& aFullFileName );

/**
 * Performs ERC testing and creates an ERC marker to show the ERC problem for aNetItemRef
 * or between aNetItemRef and aNetItemTst.
 *  if MinConn < 0: this is an error on labels
 */
extern void Diagnose( NETLIST_OBJECT* NetItemRef, NETLIST_OBJECT* NetItemTst,
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
extern void TestOthersItems( NETLIST_OBJECT_LIST* aList,
                             unsigned aNetItemRef, unsigned aNetStart,
                             int* aMinConnexion );

/**
 * Counts number of pins connected on the same net.
 * Used to find all pins conected to a no connect symbol
 * @return the pin count of the net starting at aNetStart
 * @param aNetStart = index in list of net objects of the first item
 * @param aList = a reference to the list of connected objects
 */
int CountPinsInNet( NETLIST_OBJECT_LIST* aList, unsigned aNetStart );


/**
 * Function TestLabel
 * performs an ERC on a sheet labels to verify that it is connected to a corresponding
 * sub sheet global label.
 */
extern void TestLabel( NETLIST_OBJECT_LIST* aList, unsigned aNetItemRef, unsigned aStartNet );

/**
 * Function TestDuplicateSheetNames( )
 * inside a given sheet, one cannot have sheets with duplicate names (file
 * names can be duplicated).
 * @return the error count
 * @param aCreateMarker: true = create error markers in schematic,
 *                       false = calculate error count only
 */
extern int TestDuplicateSheetNames( bool aCreateMarker );


#endif  // _ERC_H
