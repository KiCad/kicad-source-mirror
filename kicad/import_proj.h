/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef IMPORT_PROJ_H
#define IMPORT_PROJ_H

#include <wx/filename.h>
#include <core/typeinfo.h>
#include <core/utf8.h>
#include <frame_type.h>

#include <map>
#include <set>

class KICAD_MANAGER_FRAME;

/**
 * A helper class to import non Kicad project.
 * */
class IMPORT_PROJ_HELPER
{
public:
    IMPORT_PROJ_HELPER( KICAD_MANAGER_FRAME*         aframe,
                        const std::vector<wxString>& aSchFileExtensions,
                        const std::vector<wxString>& aPcbFileExtensions );

    /**
     * @brief Appends a new directory with the name of the project file
     *        Keep iterating until an empty directory is found
     */
    void FindEmptyTargetDir();

    /**
     * @brief Converts imported files to kicad type files.
     *        Types of imported files are needed for conversion
     * @param aImportedSchFileType type of the imported schematic
     * @param aImportedPcbFileType type of the imported PCB
     */
    void ImportFiles( int aImportedSchFileType, int aImportedPcbFileType );

    wxFileName m_InputFile;
    wxFileName m_TargetProj;

private:
    KICAD_MANAGER_FRAME* m_frame;

    std::map<std::string, UTF8> m_properties;

    std::vector<wxString> m_copiedSchPaths;
    std::vector<wxString> m_copiedPcbPaths;

    std::vector<wxString> m_schExtenstions;
    std::vector<wxString> m_pcbExtenstions;

    void ImportIndividualFile( KICAD_T aKicad_T, int aImportedFileType );

    void doImport( const wxString& aFile, FRAME_T aFrameType, int aImportedFileType );

    void addLocalLibraries( const std::set<wxString>& aLibName, FRAME_T aFrameType );

    void EasyEDAProProjectHandler();

    void AltiumProjectHandler();
};

#endif