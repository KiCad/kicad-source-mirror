/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

// This header is an insolation layer between top most frames and any number of
// DIALOG classes which can be called from a frame window.
// It is a place to put invocation functions for [modal] dialogs, with benefits:
//
// 1) The information about each dialog class is not exposed to the frame.
//    So therefore the DIALOG class can often be kept out of a header file entirely.
//
// 2) The information about the calling frame is not necessarily exposed to
//    to the called dialog class, at least not in here.

// The actual InvokeDialog<class>() function is usually coded at the bottom of the
// DIALOG_<class>.cpp file.


#ifndef INVOKE_SCH_DIALOG_H_
#define INVOKE_SCH_DIALOG_H_

#include <set>
#include <vector>
#include <list>
#include <class_draw_panel_gal.h>

class SCH_SHEET_PATH;
class RESCUER;
class SCH_EDIT_FRAME;

/**
 * This dialog asks the user which rescuable, cached parts he wants to rescue.
 * Any rejects will be pruned from \a aCandidates.
 *
 * @param aParent - the wxWindow object calling this dialog
 * @param aRescuer - the active RESCUER instance
 * @param aCurrentSheet the current sheet in the schematic editor frame
 * @param aGalBackEndType the current GAL type used to render symbols
 * @param aAskShowAgain - if true, a "Never Show Again" button will be included
 */
int InvokeDialogRescueEach( wxWindow* aParent, RESCUER& aRescuer, SCH_SHEET_PATH* aCurrentSheet,
                            EDA_DRAW_PANEL_GAL::GAL_TYPE aGalBackEndType, bool aAskShowAgain );

/// Create and show DIALOG_BOM and return whatever
/// DIALOG_BOM::ShowModal() returns.
int InvokeDialogCreateBOM( SCH_EDIT_FRAME* aCaller );


/**
 * Create and shows DIALOG_EXPORT_NETLIST and returns whatever
 * DIALOG_EXPORT_NETLIST::ShowModal() returns.
 *
 * @param int - NET_PLUGIN_CHANGE means user added or deleted a plugin,
 *              wxID_OK, or wxID_CANCEL.
 */
#define NET_PLUGIN_CHANGE   1
int InvokeDialogNetList( SCH_EDIT_FRAME* aCaller );

/**
 * Run a dialog to modify the LIB_ID of symbols for instance when a symbol has
 * moved from a symbol library to another symbol library
 * @return true if changes are made, false if no change
 */
bool InvokeDialogEditSymbolsLibId( SCH_EDIT_FRAME* aCaller );



#endif  // INVOKE_SCH_DIALOG_H_
