/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef __PYTHON_SCRIPTING_H
#define __PYTHON_SCRIPTING_H

#include <wx/string.h>


/**
 * Collect the list of python scripts which could not be loaded.
 *
 * @param aNames is a wxString which will contain the filenames (separated by '\n')
 */
void        pcbnewGetUnloadableScriptNames( wxString& aNames );

/**
 * Collect the list of paths where python scripts are searched.
 *
 * @param aNames is a wxString which will contain the paths (separated by '\n')
 */
void        pcbnewGetScriptsSearchPaths( wxString& aNames );

/**
 * Return the backtrace of errors (if any) when wizard python scripts are loaded.
 *
 * @param aNames is a wxString which will contain the trace
 */
void        pcbnewGetWizardsBackTrace( wxString& aNames );


#endif    // __PYTHON_SCRIPTING_H
