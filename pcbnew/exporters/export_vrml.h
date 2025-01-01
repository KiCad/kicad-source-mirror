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

#pragma once

#include <wx/string.h>

class BOARD;
class PROJECT;
class EXPORTER_PCB_VRML;

/**
 * Wrapper to expose an API for writing VRML files, without exposing all the many
 * structures used in the actual exporter EXPORTER_PCB_VRML
 */
class EXPORTER_VRML
{
public:
    EXPORTER_VRML( BOARD* aBoard );
    ~EXPORTER_VRML();

    /**
     * Exports the board and its footprint shapes 3D (vrml files only) as a
     * vrml file
     * @param aFullFileName is the full filename of the board vrml file to create
     * @param aMMtoWRMLunit is the convert factor from mm to the desired vrml file
     * @param aExport3DFiles = true to copy 3D fp vrml models to a folder,
     * and use " { inline fp_3d_model_filename }" keyword in vrml board file
     * false to include them in the vrml board file
     * @param aUseRelativePaths = true to use fp 3D relative paths,
     * false to use absolute paths
     * @param a3D_Subdir is the folder to copy 3D fp models
     * @param aXRef = X position of board (in mm)
     * @param aYRef = Y position of board (in mm)
     */
    bool ExportVRML_File( PROJECT* aProject, wxString *aMessages,
                                  const wxString& aFullFileName, double aMMtoWRMLunit,
                                  bool aIncludeUnspecified, bool aIncludeDNP,
                                  bool aExport3DFiles, bool aUseRelativePaths,
                                  const wxString& a3D_Subdir,
                                  double aXRef, double aYRef );

private:
    EXPORTER_PCB_VRML* pcb_exporter;
};
