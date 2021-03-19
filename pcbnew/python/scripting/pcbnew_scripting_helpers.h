/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 NBEE Embedded Systems SL, Miguel Angel Ajo <miguelangel@ajo.es>
 * Copyright (C) 2013-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef __PCBNEW_SCRIPTING_HELPERS_H
#define __PCBNEW_SCRIPTING_HELPERS_H

#include <pcb_edit_frame.h>
#include <io_mgr.h>

/* we could be including all these methods as static in a class, but
 * we want plain pcbnew.<method_name> access from python
 */

#ifndef SWIG
void    ScriptingSetPcbEditFrame( PCB_EDIT_FRAME* aPCBEdaFrame );

#endif

// For Python scripts: return the current board.
BOARD*  GetBoard();

BOARD*  LoadBoard( wxString& aFileName, IO_MGR::PCB_FILE_T aFormat );

// Default LoadBoard() to load .kicad_pcb files:.
BOARD*  LoadBoard( wxString& aFileName );

SETTINGS_MANAGER* GetSettingsManager();

/**
 * Constructs a default BOARD with a tempoary (no filename) project
 * @return the created board
 */
BOARD* CreateEmptyBoard();

// Boards can be saved only as .kicad_pcb file format,
// so no option to choose the file format.
bool    SaveBoard( wxString& aFileName, BOARD* aBoard );

/**
 * will get the nicknames of all of the footprint libraries configured in
 * pcbnew in both the project and global library tables
 * @return the list of footprint library nicknames, empty on error
 */
wxArrayString GetFootprintLibraries();

/**
 * will get the names of all of the footprints available in a footprint library
 * @param aNickName is the nickname specifying which footprint library to fetch
 * from
 * @return the list of footprint names, empty on error
 */
wxArrayString GetFootprints( const wxString& aNickName );

/**
 * will export the current BOARD to a specctra dsn file.
 * See http://www.autotraxeda.com/docs/SPECCTRA/SPECCTRA.pdf for the
 * specification.
 * @return true if OK
 */
bool ExportSpecctraDSN( wxString& aFullFilename );

/**
 * will export the current BOARD to a VRML (wrl) file.
 * See ExportVRML_File in pcb_edit_frame.h for detailed documentation.
 * @return true if OK
 */
bool ExportVRML( const wxString& aFullFileName, double aMMtoWRMLunit,
                 bool aExport3DFiles, bool aUseRelativePaths,
                 const wxString& a3D_Subdir,
                 double aXRef, double aYRef );

/**
 * will import a specctra *.ses file and use it to relocate MODULEs and
 * to replace all vias and tracks in an existing and loaded BOARD.
 * See http://www.autotraxeda.com/docs/SPECCTRA/SPECCTRA.pdf for the
 * specification.
 * @return true if OK
 */
bool ImportSpecctraSES( wxString& aFullFilename );

/**
 * Function ExportFootprintsToLibrary
 * Save footprints in a library:
 * @param aStoreInNewLib:
 *              true : save footprints in a existing lib. Existing footprints will be kept
 *              or updated.
 *              This lib should be in fp lib table, and is type is .pretty
 *              false: save footprints in a new lib. It it is an existing lib,
 *              previous footprints will be removed
 *
 * @param aLibName:
 *              optional library name to create, stops dialog call.
 *              must be called with aStoreInNewLib as true
 */
bool ExportFootprintsToLibrary( bool aStoreInNewLib, const wxString& aLibName = wxEmptyString,
                                wxString* aLibPath = NULL );

/**
 * Update the board display after modifying it by a python script
 * (note: it is automatically called by action plugins, after running the plugin,
 * so call this function is usually not needed inside action plugins
 *
 * Could be deprecated because modifying a board (especially deleting items) outside
 * a action plugin can crash Pcbnew.
 */
void    Refresh();

/**
 * Update the layer manager and other widgets from the board setup
 * (layer and items visibility, colors ...)
 * (note: it is automatically called by action plugins, after running the plugin,
 * so call this function is usually not needed inside action plugins
 */
void UpdateUserInterface();

/**
 * Returns the currently selected user unit value for the interface
 * @return 0 = Inches, 1=mm, -1 if the frame isn't set
 */
int GetUserUnits();

/**
 * Are we currently in an action plugin?
 */
bool IsActionRunning();

/**
 * Runs the DRC check on the given board and writes the results to a report file.
 * Requires that the project for the board be loaded, and note that unlike the DRC dialog
 * this does not attempt to fill zones, so zones must be valid before calling.
 *
 * @param aBoard is a valid loaded board
 * @param aFileName is the full path and name of the report file to write
 * @param aUnits is the units to use in the report
 * @param aReportAllTrackErrors controls whether all errors or just the first error is reported
 *                              for each track
 * @return true if successful, false if not
 */
bool WriteDRCReport( BOARD* aBoard, const wxString& aFileName, EDA_UNITS aUnits,
                     bool aReportAllTrackErrors );

#endif      // __PCBNEW_SCRIPTING_HELPERS_H
