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


#ifndef INVOKE_PL_EDITOR_DIALOG_H_
#define INVOKE_PL_EDITOR_DIALOG_H_

// Often this is not used in the prototypes, since wxFrame is good enough and would
// represent maximum information hiding.
class PL_EDITOR_FRAME;
class DS_DATA_ITEM;
class wxPrintData;
class wxPageSetupDialogData;


/// Create and show a print dialog
/// returns 1 if OK, 0 , there is a problem.
int InvokeDialogPrint( PL_EDITOR_FRAME* aCaller, wxPrintData* aPrintData,
                        wxPageSetupDialogData* aPageSetupData );

/// Create and show a print preview dialog
/// returns 1 if OK, 0 , there is a problem.
int InvokeDialogPrintPreview( PL_EDITOR_FRAME* aCaller, wxPrintData* aPrintData );

#endif  // INVOKE_PL_EDITOR_DIALOG_H_
