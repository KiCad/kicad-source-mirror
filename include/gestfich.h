/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2014 Jerry Jacobs
 * Copyright (C) 1992-2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * This file is part of the common library
 * TODO brief description
 * @file  gestfich.h
 * @see   common.h
 */


#ifndef __INCLUDE__GESTFICH_H__
#define __INCLUDE__GESTFICH_H__ 1

#include <wx/filename.h>
#include <wx/process.h>


/**
 * @note Do we really need these defined?
 */
#define UNIX_STRING_DIR_SEP wxT( "/" )
#define WIN_STRING_DIR_SEP  wxT( "\\" )

/* Forward class declarations. */
class EDA_LIST_DIALOG;


/**
 * Function OpenPDF
 * run the PDF viewer and display a PDF file
 * @param file = PDF file to open
 * @return true is success, false if no PDF viewer found
 */
bool OpenPDF( const wxString& file );

void OpenFile( const wxString& file );

/**
 * Function EDA_PATH_SELECTOR
 *
 * is a helper function that wraps wxDirDialog.
 *
 * @param aTitle is a string to display in the dialog title bar.
 * @param aPath is a string contain the default path for the path dialog.  This string also
 *              contains the result of the wxDirDialog when the OK button is used to dismiss
 *              the dialog.
 * @param aFlags is the style of the path dialog, wxDD_???.
 * @param aParaent is the parent window of the dialog.
 * @param aPosition is the position of the dialog.
 * @return true if a path was selected.
 */
bool EDA_PATH_SELECTOR( const wxString& aTitle,
                        wxString&       aPath,
                        int             aFlags,       /* reserve */
                        wxWindow*       aParent,
                        const wxPoint&  aPosition = wxDefaultPosition );

/**
 * Function EDA_FILE_SELECTOR
 *
 * is a helper function that wraps a call to wxFileSelector.
 *
 * @param aTitle is a string to display in the dialog title bar.
 * @param aPath is a string contain the default path for the path dialog.
 * @param aFileName is a string containing the default file name.
 * @param aExtension is a string containing the default file extension.
 * @param aWildcard is a string containing the default wildcard.
 * @param aParaent is the parent window of the dialog.
 * @param aFlags is the style of the path dialog, wxFD_???.
 * @param aKeepWorkingDirectory determines if current working directory should be set to the
 *                              user selected path.
 * @param aPosition is the position of the dialog.
 * @param aMruPath is a pointer to a string to copy the path selected by the user when
 *                 the OK button is pressed to dismiss the dialog.  This can be NULL.
 * @return the full path and file name of the selected file or wxEmptyString if the user
 *         pressed the cancel button to dismiss the dialog.
 */
wxString EDA_FILE_SELECTOR( const wxString& aTitle,
                            const wxString& aPath,
                            const wxString& aFileName,
                            const wxString& aExtension,
                            const wxString& aWildcard,
                            wxWindow*       aParent,
                            int             aStyle,
                            const bool      aKeepWorkingDirectory,
                            const wxPoint&  aPosition = wxDefaultPosition,
                            wxString*       aMruPath = NULL );

EDA_LIST_DIALOG* GetFileNames( char* Directory, char* Mask );


/**
 * Function ExecuteFile
 * calls the executable file \a ExecFile with the command line parameters \a param.
 */
int ExecuteFile( wxWindow* frame, const wxString& ExecFile,
                 const wxString& param = wxEmptyString, wxProcess *callback = NULL );

/**
 * Function AddDelimiterString
 * Add un " to the start and the end of string (if not already done).
 * @param string = string to modify
 */
void AddDelimiterString( wxString& string );

/**
 * Function KicadDatasPath
 * returns the data path common to KiCad.
 * If environment variable KICAD is defined (KICAD = path to kicad)
 * Returns \<KICAD\> /;
 * Otherwise returns \<path of binaries\> / (if "kicad" is in the path name)
 * Otherwise returns /usr /share/kicad/
 *
 * Note:
 * The \\ are replaced by / (a la Unix)
 */
wxString KicadDatasPath();

/**
 * Function FindKicadFile
 * searches the executable file shortname in KiCad binary path  and return full file
 * name if found or shortname if the kicad binary path is kicad/bin.
 *
 *  kicad binary path is found from:
 *  BinDir
 *  or environment variable KICAD
 *  or (default) c:\\kicad or /usr/local/kicad
 *  or default binary path
 */
wxString FindKicadFile( const wxString& shortname );

/**
 * Quote return value of wxFileName::GetFullPath().
 *
 * This allows file name paths with spaces to be used as parameters to
 * ProcessExecute function calls.
 * @param fn is the filename to wrap
 * @param format if provided, can be used to transform the nature of the
 *    wrapped filename to another platform.
 */
extern wxString QuoteFullPath( wxFileName& fn, wxPathFormat format = wxPATH_NATIVE );

#endif /* __INCLUDE__GESTFICH_H__ */
