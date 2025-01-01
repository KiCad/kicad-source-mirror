/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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

/**
 * @file x3d_ops.h
 */


#ifndef X3D_OPS_H
#define X3D_OPS_H

#include "x3d_base.h"
#include "wrltypes.h"

namespace X3D
{
    /* Functions to create and read X3D Nodes */
    bool ReadTransform( wxXmlNode* aNode, X3DNODE* aParent, X3D_DICT& aDict );
    bool ReadSwitch( wxXmlNode* aNode, X3DNODE* aParent, X3D_DICT& aDict );
    bool ReadShape( wxXmlNode* aNode, X3DNODE* aParent, X3D_DICT& aDict );
    bool ReadAppearance( wxXmlNode* aNode, X3DNODE* aParent, X3D_DICT& aDict );
    bool ReadIndexedFaceSet( wxXmlNode* aNode, X3DNODE* aParent, X3D_DICT& aDict );
    bool ReadCoordinates( wxXmlNode* aNode, X3DNODE* aParent, X3D_DICT& aDict );

    bool ParseSFBool( const wxString& aSource, bool& aResult );
    bool ParseSFFloat( const wxString& aSource, float& aResult );
    bool ParseSFVec3( const wxString& aSource, WRLVEC3F& aResult );
    bool ParseSFRotation( const wxString& aSource, WRLROTATION& aResult );
}

#endif  // X3D_OPS_H

