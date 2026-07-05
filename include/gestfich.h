/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2014 Jerry Jacobs
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#pragma once

#include <map>
#include <kicommon.h>
#include <wx/arrstr.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/process.h>
#include <wx/zipstrm.h>


/**
 * @note Do we really need these defined?
 */
#define UNIX_STRING_DIR_SEP wxT( "/" )
#define WIN_STRING_DIR_SEP  wxT( "\\" )

/* Forward class declarations. */
class EDA_LIST_DIALOG;


/**
 * Run the PDF viewer and display a PDF file.
 *
 * @param file the PDF file to open.
 * @retval true if PDF viewer found.
 * @retval false if no PDF viewer found.
 */
KICOMMON_API bool OpenPDF( const wxString& file );

/**
 * @param aSrcPath is the full filename of the source.
 * @param[in] aDestPath is the full filename of the target.
 * @param[out] aErrors a wxString to *append* any errors to.
 */
KICOMMON_API void KiCopyFile( const wxString& aSrcPath, const wxString& aDestPath,
                              wxString& aErrors );

/**
 * @param aSrcPath is the full filename of the source.
 * @param[in] aDestPath is the full filename of the target.
 * @param[in] aCallback a facility to allow modification of the values of particular tokens.  Normally used
 *                      to update paths in the copied document.
 * @param[out] aErrors a wxString to *append* any errors to.
 */
KICOMMON_API void CopySexprFile( const wxString& aSrcPath, const wxString& aDestPath,
                                 std::function<bool( const std::string& token, wxString& value )> aCallback,
                                 wxString& aErrors );

/**
 * Call the executable file \a aEditorName with the parameter \a aFileName.
 *
 * @param[in] aEditorName is the full filename for the binary.
 * @param[in] aFileName is the full filename of the file to open.
 * @param[in] aCallback a wxProcess* for the call.
 * @param aFileForKicad a boolean to flag if aFileName runs with a KiCad binary.
 * In this case aFileName is a shortname and FindKicadFile() is called to return the path.
 * In the other case, aFileName is a full file name (passed prefixed with the path).
 */
KICOMMON_API int ExecuteFile( const wxString& aEditorName,
                              const wxString& aFileName = wxEmptyString,
                              wxProcess* aCallback = nullptr, bool aFileForKicad = true );

/**
 * Run a user-supplied command line through the platform shell so glob expansion, pipes, and
 * other shell features work consistently.
 *
 * On Windows the command is wrapped as cmd.exe /d /s /c "<cmd>" so that absolute paths containing
 * spaces or quotes survive cmd.exe's quote handling intact. On POSIX the command is handed to
 * /bin/sh -c as a single argument.
 *
 * @param aCommand the command line exactly as entered by the user.
 * @param aProcess an optional wxProcess for the call; pass a Redirect()ed process to capture output.
 * @return the process exit code, or -1 if the command could not be executed.
 */
KICOMMON_API int ExecuteCommandThroughShell( const wxString& aCommand, wxProcess* aProcess = nullptr );

/**
 * Add un " to the start and the end of string (if not already done).
 *
 * @param string string to modify.
 */
KICOMMON_API void QuoteString( wxString& string );

/**
 * Search the executable file shortname in KiCad binary path and return full file
 * name if found or shortname if the kicad binary path is kicad/bin.
 *
 * The binary path is found from:
 *   - binary path.
 *   - KICAD environment variable.
 *   - c:\\kicad or /usr/local/kicad (the default).
 *   - default binary path.
 */
KICOMMON_API wxString FindKicadFile( const wxString& shortname );

/**
 * Quote return value of wxFileName::GetFullPath().
 *
 * This allows file name paths with spaces to be used as parameters to ProcessExecute
 * function calls.
 *
 * @param fn is the filename to wrap.
 * @param format if provided, can be used to transform the nature of the wrapped filename
 *               to another platform.
 */
KICOMMON_API extern wxString QuoteFullPath( wxFileName& fn, wxPathFormat format = wxPATH_NATIVE );


/**
 * Remove the directory \a aDirName and all its contents including
 * subdirectories and their files.
 */
KICOMMON_API bool RmDirRecursive( const wxString& aDirName, wxString* aErrors = nullptr );

/**
 * Copy a directory and its contents to another directory.
 *
 * @param aSourceDir is the directory to copy.
 * @param aDestDir is the directory to copy to.
 * @param aErrors is a string to append any errors to.
 */
KICOMMON_API bool CopyDirectory( const wxString& aSourceDir, const wxString& aDestDir,
                                 const std::vector<wxString>& aOverwriteExclusions, wxString& aErrors );

KICOMMON_API bool CopyFilesOrDirectory( const wxString& aSourceDir, const wxString& aDestDir, bool aAllowOverwrite,
                                        wxString& aErrors, std::vector<wxString>& aPathsWritten );

/**
 * Add a directory and its contents to a zip file.
 *
 * @param aZip is the zip file to add to.
 * @param aSourceDir is the directory to add.
 * @param aErrors is a string to append any errors to.
 * @param aParentDir is the parent directory to add to the zip file.
 */
KICOMMON_API bool AddDirectoryToZip( wxZipOutputStream& aZip, const wxString& aSourceDir, wxString& aErrors,
                                     const wxString& aParentDir = "" );

/**
 * Recursively collect every file under \a aRoot, deduplicating subdirectories by
 * their resolved path.  Legitimate one-level symlinks resolve normally, but
 * recursive symlinks (a Wine `dosdevices/z: -> /` loop, a self-referencing `.`
 * link, etc.) are visited at most once instead of recursing until OOM.  This is
 * a loop-safe replacement for wxDir::GetAllFiles() when scanning user-controlled
 * directory trees.
 *
 * @param aRoot is the directory to walk.
 * @param aFiles receives the full path of every matching file found.
 * @param aFileSpec is an optional wildcard filter (e.g. `*.step`); empty matches all.
 * @param aFlags is the wxDir traversal flag set; defaults to wxDIR_DEFAULT to match
 *               wxDir::GetAllFiles().  Pass `wxDIR_FILES | wxDIR_DIRS` to exclude
 *               hidden entries.  wxDIR_FILES and wxDIR_DIRS are always implied so
 *               recursion and file collection happen regardless.
 */
KICOMMON_API void CollectFilesLoopSafe( const wxString& aRoot, wxArrayString& aFiles,
                                        const wxString& aFileSpec = wxEmptyString,
                                        int aFlags = wxDIR_DEFAULT );

/**
 * Recursively collect every subdirectory under \a aRoot using the same loop
 * detection as CollectFilesLoopSafe().  Loop-safe replacement for
 * wxDir::GetAllFiles() called with the wxDIR_DIRS flag.
 *
 * @param aRoot is the directory to walk.
 * @param aDirs receives the full path of every subdirectory found.
 * @param aFlags is the wxDir traversal flag set; defaults to wxDIR_DIRS (hidden
 *               directories excluded).  Pass `wxDIR_DIRS | wxDIR_HIDDEN` to include
 *               them.  wxDIR_DIRS is always implied.
 */
KICOMMON_API void CollectSubdirsLoopSafe( const wxString& aRoot, wxArrayString& aDirs,
                                          int aFlags = wxDIR_DIRS );
