/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 NBEE Embedded Systems SL, Miguel Angel Ajo <miguelangel@ajo.es>
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

#ifndef __PCBNEW_SCRIPTING_HELPERS_H
#define __PCBNEW_SCRIPTING_HELPERS_H

#include <deque>
#include <pcb_io/pcb_io_mgr.h>
#include <layer_ids.h>

class PCB_EDIT_FRAME;
class BOARD;
class SETTINGS_MANAGER;
class BOARD_ITEM;
enum class EDA_UNITS;
enum PCB_LAYER_ID : int;

/* we could be including all these methods as static in a class, but
 * we want plain pcbnew.<method_name> access from python
 */

#ifndef SWIG
void ScriptingSetPcbEditFrame( PCB_EDIT_FRAME* aPCBEdaFrame );
void ScriptingOnDestructPcbEditFrame( PCB_EDIT_FRAME* aPCBEdaFrame );

#endif

// For Python scripts: return the current board.
BOARD*  GetBoard();

/**
 * Loads a board from file using the specified file io handler
 *
 * This function does not set the board project as the active one
 * @return a pointer to the board if it was created, or None if not
 */
BOARD* LoadBoard( const wxString& aFileName, PCB_IO_MGR::PCB_FILE_T aFormat );

#ifndef SWIG
/**
 * Loads a board from file using the specified file io handler
 *
 * Hidden from SWIG as aSetActive should not be used by python, but cli also leverages this function
 */
BOARD* LoadBoard( const wxString& aFileName, PCB_IO_MGR::PCB_FILE_T aFormat, bool aSetActive );
#endif

// Default LoadBoard() to load .kicad_pcb files:.
#ifndef SWIG

/**
 * Loads a board from file
 * This function identifies the file type by extension and determines the correct file io to use
 *
 * Hidden from SWIG as aSetActive should not be used by python, but cli also leverages this function
 */
BOARD* LoadBoard( const wxString& aFileName, bool aSetActive );
#endif

/**
 * Loads a board from file
 * This function identifies the file type by extension and determines the correct file io to use
 *
 * This function does not set the board project as the active one
 * @return a pointer to the board if it was created, or None if not
 */
BOARD* LoadBoard( const wxString& aFileName );

/**
 * Creates a new board and project with the given filename (will overwrite existing files!)
 *
 * @param aFileName is the filename (including full path if desired) of the kicad_pcb to create
 * @return a pointer to the board if it was created, or None if not
 */
BOARD*  NewBoard( wxString& aFileName );

SETTINGS_MANAGER* GetSettingsManager();

/**
 * Construct a default BOARD with a temporary (no filename) project.
 *
 * @return the created board.
 */
BOARD* CreateEmptyBoard();

/**
 * Saves a copy of the given board and its associated project to the given path.
 * Boards can only be saved in KiCad native format.
 * @param aFileName is the full path to save a copy to.
 * @param aBoard is a pointer to a loaded BOARD to save.
 * @param aSkipSettings if true, only save the board file.  This will lose settings changes
 * that are saved in the project file
 * @return true if the save was completed.
 */
bool SaveBoard( wxString& aFileName, BOARD* aBoard, bool aSkipSettings = false );

/**
 * Get the nicknames of all of the footprint libraries configured in
 * pcbnew in both the project and global library tables.
 *
 * @return the list of footprint library nicknames, empty on error.
 */
wxArrayString GetFootprintLibraries();

/**
 * Get the names of all of the footprints available in a footprint library.
 *
 * @param aNickName is the nickname specifying which footprint library to fetch from.
 * @return the list of footprint names, empty on error.
 */
wxArrayString GetFootprints( const wxString& aNickName );

/**
 * Will export the current BOARD to a specctra dsn file.
 *
 * See http://www.autotraxeda.com/docs/SPECCTRA/SPECCTRA.pdf for the specification.
 *
 * @return true if OK
 */
bool ExportSpecctraDSN( wxString& aFullFilename );

/**
 * Will export the BOARD to a specctra dsn file.
 * Unlike first overload doesn't need a valid PCB_EDIT_FRAME set and can be used
 * in a standalone python script.
 *
 * See http://www.autotraxeda.com/docs/SPECCTRA/SPECCTRA.pdf for the specification.
 *
 * @return true if OK
 */
bool ExportSpecctraDSN( BOARD* aBoard, wxString& aFullFilename );

/**
 * Export the current BOARD to a VRML (wrl) file.
 *
 * See ExportVRML_File in pcb_edit_frame.h for detailed documentation.
 * @return true if OK.
 */
bool ExportVRML( const wxString& aFullFileName, double aMMtoWRMLunit, bool aIncludeUnspecified,
                 bool aIncludeDNP, bool aExport3DFiles,
                 bool aUseRelativePaths, const wxString& a3D_Subdir, double aXRef, double aYRef );

/**
 * Import a specctra *.ses file and use it to relocate MODULEs and to replace all vias and
 * tracks in an existing and loaded #BOARD.
 *
 * See http://www.autotraxeda.com/docs/SPECCTRA/SPECCTRA.pdf for the specification.
 *
 * @return true if OK
 */
bool ImportSpecctraSES( wxString& aFullFilename );

/**
 * Import a specctra *.ses file and use it to relocate MODULEs and to replace all vias and
 * Unlike first overload doesn't need a valid PCB_EDIT_FRAME set and can be used
 * in a standalone python script.
 *
 * See http://www.autotraxeda.com/docs/SPECCTRA/SPECCTRA.pdf for the specification.
 *
 * @return true if OK
 */
bool ImportSpecctraSES( BOARD* aBoard, wxString& aFullFilename );

/**
 * Save footprints in a library:
 *
 * @param aStoreInNewLib set to true to save footprints in a existing lib. Existing footprints
 *                       will be kept or updated.  This lib should be in fp lib table, and is
 *                       type is .pretty.  Set to false to save footprints in a new lib.  If it
 *                       is an existing lib, previous footprints will be removed.
 *
 * @param aLibName is the optional library name to create, stops dialog call.  Must be called
 *                 with \a aStoreInNewLib as true.
 */
bool ExportFootprintsToLibrary( bool aStoreInNewLib, const wxString& aLibName = wxEmptyString,
                                wxString* aLibPath = nullptr );

/**
 * Update the board display after modifying it by a python script
 * (note: it is automatically called by action plugins, after running the plugin,
 * so call this function is usually not needed inside action plugins
 *
 * Could be deprecated because modifying a board (especially deleting items) outside
 * a action plugin can crash Pcbnew.
 */
void Refresh();

/**
 * Update the layer manager and other widgets from the board setup
 * (layer and items visibility, colors ...)
 * (note: it is automatically called by action plugins, after running the plugin,
 * so call this function is usually not needed inside action plugins
 */
void UpdateUserInterface();

/**
 * Return the currently selected user unit value for the interface.
 *
 * @return 0 = Inches, 1=mm, -1 if the frame isn't set
 */
int GetUserUnits();

/**
 * Get the list of selected objects.
 */
std::deque<BOARD_ITEM*> GetCurrentSelection();

/**
 * Focus the view on the target item.
 *
 * @param aItem is the target board item.
 * @param aLayer is the layer ID of the target item.
 */
void FocusOnItem( BOARD_ITEM* aItem, PCB_LAYER_ID aLayer = UNDEFINED_LAYER );

/**
 * Are we currently in an action plugin?
 */
bool IsActionRunning();

/**
 * Run the DRC check on the given board and writes the results to a report file.
 * Requires that the project for the board be loaded, and note that unlike the DRC dialog
 * this does not attempt to fill zones, so zones must be valid before calling.
 *
 * @param aBoard is a valid loaded board.
 * @param aFileName is the full path and name of the report file to write.
 * @param aUnits is the units to use in the report.
 * @param aReportAllTrackErrors controls whether all errors or just the first error is reported
 *                              for each track.
 * @return true if successful, false if not.
 */
bool WriteDRCReport( BOARD* aBoard, const wxString& aFileName, EDA_UNITS aUnits,
                     bool aReportAllTrackErrors );

/**
 * Get the language string from COMMON_SETTINGS.
 *
 * @return the current language string.
 */
wxString GetLanguage();

#endif      // __PCBNEW_SCRIPTING_HELPERS_H
