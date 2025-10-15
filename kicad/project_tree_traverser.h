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

#ifndef PROJECT_TREE_TRAVERSER_H
#define PROJECT_TREE_TRAVERSER_H

#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/string.h>

class KICAD_MANAGER_FRAME;

/**
 * Traverser class to duplicate/copy project or template files with proper renaming.
 *
 * This class can operate in two modes:
 * 1. With KICAD_MANAGER_FRAME: Uses KIFACE interfaces for proper file type handling
 * 2. Without KICAD_MANAGER_FRAME (nullptr): Simple file copying for templates
 */
class PROJECT_TREE_TRAVERSER : public wxDirTraverser
{
public:
    PROJECT_TREE_TRAVERSER( KICAD_MANAGER_FRAME* aFrame,
                           const wxString& aSrcProjectDirPath,
                           const wxString& aSrcProjectName,
                           const wxString& aNewProjectDirPath,
                           const wxString& aNewProjectName );

    virtual wxDirTraverseResult OnFile( const wxString& aSrcFilePath ) override;
    virtual wxDirTraverseResult OnDir( const wxString& aSrcDirPath ) override;

    wxString GetErrors() const { return m_errors; }
    wxFileName GetNewProjectFile() const { return m_newProjectFile; }

private:
    KICAD_MANAGER_FRAME* m_frame;          // nullptr for simple copy mode (templates)

    wxString             m_projectDirPath;
    wxString             m_projectName;
    wxString             m_newProjectDirPath;
    wxString             m_newProjectName;

    wxFileName           m_newProjectFile;
    wxString             m_errors;
};

#endif // PROJECT_TREE_TRAVERSER_H
