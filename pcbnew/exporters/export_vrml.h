/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Julian Fellinger
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

#include <wx/string.h>
#include <pcbnew_scripting_helpers.h>

/**
 * Wrapper to expose an API for writing VRML files
 */

class VRML_WRITER
{
public:

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
    bool ExportVRML_File( const wxString& aFullFileName, double aMMtoWRMLunit,
                                      bool aExport3DFiles, bool aUseRelativePaths,
                                      const wxString& a3D_Subdir,
                                      double aXRef, double aYRef )
    {
        return ExportVRML( aFullFileName, aMMtoWRMLunit,
                           aExport3DFiles, aUseRelativePaths,
                           a3D_Subdir, aXRef, aYRef);
    }
};