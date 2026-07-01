/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2020 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2007-2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * The common library
 * @file common.h
 */

#ifndef INCLUDE__COMMON_H_
#define INCLUDE__COMMON_H_

#include <kicommon.h>
#include <functional>
#include <memory>
#include <vector>

#include <wx/fileconf.h>
#include <wx/string.h>
#include <wx/process.h>

#include <text_var_dependency.h>

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
KICOMMON_API wxString SearchHelpFileFullPath( const wxString& aBaseName );

/**
 * Make \a aTargetFullFileName absolute and create the path of this file if it doesn't yet exist.
 *
 * @param aTargetFullFileName the wxFileName containing the full path and file name to modify.
 *                            The path may be absolute or relative to \a aBaseFilename .
 * @param aBaseFilename a full filename. Only its path is used to set the aTargetFullFileName path.
 * @param aReporter a point to a REPORTER object use to show messages (can be NULL)
 * @return true if \a aOutputDir already exists or was successfully created.
 */
KICOMMON_API bool EnsureFileDirectoryExists( wxFileName* aTargetFullFileName, const wxString& aBaseFilename,
                                             REPORTER* aReporter = nullptr );

/**
 * It's annoying to throw up nag dialogs when the extension isn't right.  Just fix it.
 */
KICOMMON_API wxString EnsureFileExtension( const wxString& aFilename, const wxString& aExtension );

/**
 * Join a list of file extensions for use in a file dialog.
 * @param aExts
 * @return
 */
KICOMMON_API wxString JoinExtensions( const std::vector<std::string>& aExts );

/**
 * Replace any environment variable & text variable references with their values.
 *
 * @param aString a string containing (perhaps) references to env var
 * @return the expanded environment variable.
 */
KICOMMON_API const wxString ExpandEnvVarSubstitutions( const wxString& aString, const PROJECT* aProject );

/**
 * Expand '${var-name}' templates in text.
 */
#define FOR_ERC_DRC 1

KICOMMON_API wxString ExpandTextVars( const wxString& aSource, const std::function<bool( wxString* )>* aResolver,
                                      int aFlags = 0, int aDepth = 0 );

KICOMMON_API wxString ExpandTextVars( const wxString& aSource, const PROJECT* aProject, int aFlags = 0 );

/**
 * Normalize a file path so its text variables survive ExpandTextVars.
 *
 * ExpandTextVars treats \${ and \@{ as escape sequences, so a Windows path like
 * "subdir\${REVISION}.csv" leaves the variable unexpanded. Callers that expand text
 * variables in a user-entered file path must run the path through this first so the
 * backslash reads as a separator, not an escape. Backslashes not immediately preceding a
 * text variable are left untouched.
 */
KICOMMON_API wxString NormalizeFilePathForTextVars( const wxString& aPath );

/**
 * Lex-scan @p aSource and return every `${...}` reference that appears, without
 * resolving. Nested references are captured at every nesting level.
 *
 * Used by the reactive dependency tracker to register edges on SetText without
 * needing a resolver context. Escape sequences (`\${...}`) are skipped. `@{...}`
 * math expressions are walked so that nested `${...}` inside them is captured.
 */
KICOMMON_API std::vector<TEXT_VAR_REF_KEY> ExtractTextVarReferences( const wxString& aSource );

/**
 * Multi-pass text variable expansion and math expression evaluation.
 *
 * Performs recursive resolution of both ${...} variable references and @{...} math expressions,
 * then cleans up escape sequences (\${...} and \@{...}) to display literals.
 *
 * This helper encapsulates the common pattern used across schematic text components:
 * - While text contains ${...} or @{...} and depth < max:
 *   - Expand variables via ExpandTextVars()
 *   - Evaluate math expressions via EXPRESSION_EVALUATOR
 * - Convert escape markers back to literals
 *
 * @param aSource The source text containing variables and/or expressions
 * @param aResolver Function to resolve variable references
 * @param aDepth Current recursion depth (passed by reference, will be incremented)
 * @return Fully expanded and evaluated text with escape sequences cleaned up
 */
KICOMMON_API wxString ResolveTextVars( const wxString& aSource, const std::function<bool( wxString* )>* aResolver,
                                       int& aDepth );

/**
 * Returns any variables unexpanded, e.g. ${VAR} -> VAR
 */
KICOMMON_API wxString GetGeneratedFieldDisplayName( const wxString& aSource );

/**
 * Returns true if the string is generated, e.g contains a single text var reference
 */
KICOMMON_API bool IsGeneratedField( const wxString& aSource );

/**
 * Returns a user-visible HTML string describing a footprint reference designator.
 */
KICOMMON_API wxString DescribeRef( const wxString& aRef );

/**
 * Replace any environment and/or text variables in URIs
 */
KICOMMON_API const wxString ResolveUriByEnvVars( const wxString& aUri, const PROJECT* aProject );


/**
 * Checks if the operating system is explicitly unsupported and displays a disclaimer message box
 *
 * @return true if the operating system is unsupported
 */
KICOMMON_API bool WarnUserIfOperatingSystemUnsupported();


#endif // INCLUDE__COMMON_H_
