/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2014 Jerry Jacobs
 * Copyright (C) 1992-2021 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#ifndef GESTFICH_H
#define GESTFICH_H

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
 * Run the PDF viewer and display a PDF file.
 *
 * @param file the PDF file to open.
 * @return true is success or false if no PDF viewer found.
 */
bool OpenPDF( const wxString& file );
void OpenFile( const wxString& file );

void PrintFile( const wxString& file );
bool CanPrintFile( const wxString& file );

/**
 * @param aSrcPath is the full filename of the source.
 * @param aDestPath is the full filename of the target
 * @param aErrors a wxString to *append* any errors to
 */
void KiCopyFile( const wxString& aSrcPath, const wxString& aDestPath, wxString& aErrors );

/**
 * Call the executable file \a aEditorName with the parameter \a aFileName.
 */
int ExecuteFile( const wxString& aEditorName, const wxString& aFileName = wxEmptyString,
                 wxProcess* aCallback = nullptr );

/**
 * Add un " to the start and the end of string (if not already done).
 *
 * @param string string to modify.
 */
void QuoteString( wxString& string );

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
wxString FindKicadFile( const wxString& shortname );

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
extern wxString QuoteFullPath( wxFileName& fn, wxPathFormat format = wxPATH_NATIVE );

#endif /* GESTFICH_H */
