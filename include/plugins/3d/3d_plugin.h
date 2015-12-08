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
 * @file 3d_plugin.h
 * describes the runtime-loadable interface to support loading
 * and parsing of 3D models.
 */

#ifndef PLUGIN_3D_H
#define PLUGIN_3D_H

#include <wx/string.h>

class SCENEGRAPH;

class S3D_PLUGIN
{
public:
    virtual ~S3D_PLUGIN()
    {
        return;
    }

    /**
     * Function GetNExtensions
     *
     * @return the number of extensions supported by the plugin
     */
    virtual int GetNExtensions( void ) const = 0;

    /**
     * Function GetModelExtension
     *
     * @param aIndex is the extension to return; valid values are
     * 0 to GetNExtensions() - 1.
     * @return the requested extension or a null string if aIndex
     * was invalid.
     */
    virtual const wxString GetModelExtension( int aIndex ) const = 0;

    /**
     * Function GetNFilters
     * @returns the number of file filters
     */
    virtual int GetNFilters( void ) const = 0;

    /**
     * Function GetFileFilter
     *
     * @return the file filter string for the given index
     */
    virtual const wxString GetFileFilter( int aIndex ) const = 0;

    /**
     * Function CanRender
     *
     * @return true if the plugin can render a model, that is
     * the Load() function is implemented
     */
    virtual bool CanRender( void ) const = 0;

    /**
     * Function Load
     * reads the model file and creates a generic display structure
     *
     * @param aFileName is the full path of the model file
     * @param aModel is a handle to a null pointer; on successful
     * reading of the model data aModel will point to a representation
     * for rendering
     * @param returns true if the model was successfully loaded and false
     * if there is no rendering support for the model or there were
     * problems reading the model
     */
    virtual SCENEGRAPH* Load( const wxString& aFileName ) = 0;
};

#endif  // PLUGIN_3D_H
