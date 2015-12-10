/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
 * @file 3d_plugin_tetra.h
 * is a sample plugin which demonstrates the basic use of the SG*
 * classes for the intermediate representation of objects. The plugin
 * accepts any *.wrl filename but does not actually load any files,
 * instead the Load() routine only creates a magenta tetrahedron.
 */


#ifndef PLUGIN_3D_TETRA_H
#define PLUGIN_3D_TETRA_H

#include <vector>
#include <plugins/3d/3d_plugin.h>

class SCENEGRAPH;

class S3D_PLUGIN_TETRA : public S3D_PLUGIN
{
private:
    std::vector< wxString > m_extensions;   // supported extensions
    std::vector< wxString > m_filters;       // file filters

public:
    S3D_PLUGIN_TETRA();
    virtual ~S3D_PLUGIN_TETRA();

    virtual int GetNExtensions( void ) const;
    virtual const wxString GetModelExtension( int aIndex ) const;
    virtual int GetNFilters( void ) const;
    virtual const wxString GetFileFilter( int aIndex ) const;

    bool CanRender( void ) const;
    SCENEGRAPH* Load( const wxString& aFileName );
};

#endif  // PLUGIN_3D_TETRA_H
