/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-3.0.html
 */

#ifndef VRML_LOADMODEL_H
#define VRML_LOADMODEL_H

#include <plugins/3dapi/model_import.h>

class SCENEGRAPH;
class wxString;
struct MODEL_IMPORT_STATUS;

KICAD_3D_IMPORT_API SCENEGRAPH* LoadVRMLModel( const char*                      aFileName,
                                               const S3D::MODEL_IMPORT_OPTIONS* aOptions = nullptr );
SCENEGRAPH*                     LoadVRMLModel( const char* aFileName, const S3D::MODEL_IMPORT_OPTIONS* aOptions,
                                               MODEL_IMPORT_STATUS* aStatus );
SCENEGRAPH* LoadVRML( const wxString& aFileName, const S3D::MODEL_IMPORT_OPTIONS& aOptions,
                      MODEL_IMPORT_STATUS* aStatus );

#endif
