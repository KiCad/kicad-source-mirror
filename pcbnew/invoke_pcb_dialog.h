/**
 * @file invoke_pcb_dialog.h
 */

/* This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2013 KiCad Developers, see change_log.txt for contributors.
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


#ifndef INVOKE_A_DIALOG_H_
#define INVOKE_A_DIALOG_H_


class wxTopLevelWindow;
class wxPoint;
class wxSize;
class wxString;

class BOARD;
class MODULE;

// Often this is not used in the prototypes, since wxFrame is good enough and would
// represent maximum information hiding.
class PCB_BASE_FRAME;
class FOOTPRINT_EDIT_FRAME;
class FP_LIB_TABLE;
class BOARD;
class PCB_PLOT_PARAMS;


/**
 * Function InvokePcbLibTableEditor
 * shows the modal DIALOG_FP_LIB_TABLE for purposes of editing two lib tables.
 *
 * @param aCaller is the wxTopLevelWindow which is invoking the dialog.
 * @param aGlobal is the common footprint library table file being edited.
 * @param aProject is the project specific footprint library table file being edited.
 *
 * @return int - bits 0 and 1 tell whether a change was made to the @a aGlobal
 *  and/or the @a aProject table, respectively.  If set, table was modified.
 */
int InvokePcbLibTableEditor( wxTopLevelWindow* aCaller, FP_LIB_TABLE* aGlobal, FP_LIB_TABLE* aProject );

/**
 * Function InvokeFootprintWizard
 * Runs the footprint library wizard for easy library addition.
 *
 * @param aCaller is the wxTopLevelWindow which is invoking the dialog.
 * @param aGlobal is the common footprint library table file being edited. If aGlobal is NULL, then
 *                  it will not be updated.
 * @param aProject is the project specific footprint library table file being edited. if aProject
 *                  is NULL, then it will not be updated.
 *
 * @return int 0 - no changes
 *             1 - changes in the global table
 *             2 - changes in the project table
 *             3 - changes in both tables
 */
int InvokeFootprintWizard( wxTopLevelWindow* aParent, FP_LIB_TABLE* aGlobal, FP_LIB_TABLE* aProject );

/**
 * Function Invoke3DShapeLibsDownloaderWizard
 * Runs the downloader wizard for easy 3D shape libraries download from
 * the official Kicad Github repository of *.3Dshape libraries.
 *
 * @param aCaller is the wxTopLevelWindow which is invoking the dialog.
 */
void Invoke3DShapeLibsDownloaderWizard( wxTopLevelWindow* aParent );


/**
 * Function InvokePluginOptionsEditor
 * calls DIALOG_FP_PLUGIN_OPTIONS dialog so that plugin options set can be edited.
 *
 * @param aCaller is the wxTopLevelWindow which is invoking the dialog.
 * @param aNickname is the footprint library whose options are being edited.
 * @param aPluginType is something that will pass through IO_MGR::EnumFromStr().
 * @param aOptions is the options string on calling into this function.
 * @param aResult is where to put the result of the editing.
 */
void InvokePluginOptionsEditor( wxTopLevelWindow* aCaller, const wxString& aNickname,
    const wxString& aPluginType, const wxString& aOptions, wxString* aResult );

/**
 * Function InvokeDXFDialogBoardImport
 * shows the modal DIALOG_DXF_IMPORT for importing a DXF file to a board.

 * @param aCaller is the wxTopLevelWindow which is invoking the dialog.
 * @return true if the import was made.
 */
bool InvokeDXFDialogBoardImport( PCB_BASE_FRAME* aCaller );

/**
 * Function InvokeDXFDialogModuleImport
 * shows the modal DIALOG_DXF_IMPORT for importing a DXF file as footprint outlines.
 *
 * @param aCaller is the wxTopLevelWindow which is invoking the dialog.
 * @param aModule is the footprint currently edited.
 * @return true if the import was made.
 */
bool InvokeDXFDialogModuleImport( PCB_BASE_FRAME* aCaller, MODULE* aModule );

/**
 * Function InvokeLayerSetup
 * shows the layer setup dialog
 * @param aCaller is the wxTopLevelWindow which is invoking the dialog.
 * @param aBoard is the currently edited board.
 * @return bool - true if user pressed OK (did not abort), else false.
 */
bool InvokeLayerSetup( wxTopLevelWindow* aCaller, BOARD* aBoard );

/**
 * Function InvokeSVGPrint
 * shows the SVG print dialog
 * @param aCaller is the wxTopLevelWindow which is invoking the dialog.
 * @param aBoard is the currently edited board.
 * @param aSettings is the current pcb plot parameters.
 * @return bool - true if user pressed OK (did not abort), else false.
 */
bool InvokeSVGPrint( wxTopLevelWindow* aCaller, BOARD* aBoard, PCB_PLOT_PARAMS* aSettings );

/**
 * Function InvokeSVGPrint
 * shows the SVG print dialog
 * @param aCaller is the FOOTPRINT_EDIT_FRAME which is invoking the dialog.
 * @return bool - true if user pressed OK (did not abort), else false.
 */
bool InvokeFPEditorPrefsDlg( FOOTPRINT_EDIT_FRAME* aCaller );

#endif  // INVOKE_A_DIALOG_H_
