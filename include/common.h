/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2020 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2007-2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * The common library
 * @file common.h
 */

#ifndef INCLUDE__COMMON_H_
#define INCLUDE__COMMON_H_

#include <functional>
#include <memory>

#include <wx/fileconf.h>
#include <wx/string.h>
#include <wx/process.h>

class PROJECT;
class SEARCH_STACK;
class REPORTER;

/**
 * Return the help file's full path.
 *
 * Return the full path and name (including extension) of the given KiCad help file. It is expected
 * to be found in a subfolder help/\<_LANG_\>/ in one of the system paths. Supported file types are
 * *.html and *.pdf. If no such file is available for the current locale, an attempt to find the
 * English version is made. The search order for \<_LANG_\> is: 1) canonical form (e.g., "fr_FR"),
 * 2) short form (e.g., "fr"), and 3) "en".
 *
 * @param aBaseName is the name of the help file to search for (without extension).
 * @return the full path and filename if \a aBaseName is found, else wxEmptyString.
 */
wxString SearchHelpFileFullPath( const wxString& aBaseName );

/**
 * Make \a aTargetFullFileName absolute and create the path of this file if it doesn't yet exist.
 *
 * @param aTargetFullFileName the wxFileName containing the full path and file name to modify.
 *                            The path may be absolute or relative to \a aBaseFilename .
 * @param aBaseFilename a full filename. Only its path is used to set the aTargetFullFileName path.
 * @param aReporter a point to a REPORTER object use to show messages (can be NULL)
 * @return true if \a aOutputDir already exists or was successfully created.
 */
bool EnsureFileDirectoryExists( wxFileName*     aTargetFullFileName,
                                const wxString& aBaseFilename,
                                REPORTER*       aReporter = nullptr );

/**
 * Replace any environment variable & text variable references with their values.
 *
 * @param aString a string containing (perhaps) references to env var
 * @return the expanded environment variable.
 */
const wxString ExpandEnvVarSubstitutions( const wxString& aString, PROJECT* aProject );

/**
 * Expand '${var-name}' templates in text.  The LocalResolver is given first crack at it,
 * after which the PROJECT's resolver is called.
 */
wxString ExpandTextVars( const wxString& aSource,
                         const std::function<bool( wxString* )>* aLocalResolver,
                         const std::function<bool( wxString* )>* aFallbackResolver,
                         const PROJECT* aProject );

wxString ExpandTextVars( const wxString& aSource, const PROJECT* aProject );

/**
 * Replace any environment and/or text variables in file-path uris (leaving network-path URIs
 * alone).
 */
const wxString ResolveUriByEnvVars( const wxString& aUri, PROJECT* aProject );


long long TimestampDir( const wxString& aDirPath, const wxString& aFilespec );


/**
 * Checks if the operating system is explicitly unsupported and displays a disclaimer message box
 *
 * @return true if the operating system is unsupported
 */
bool WarnUserIfOperatingSystemUnsupported();


#endif  // INCLUDE__COMMON_H_
