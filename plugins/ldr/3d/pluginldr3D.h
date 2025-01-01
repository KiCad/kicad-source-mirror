/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
 * @file pluginldr.h
 * defines the most basic functions which all kicad plugin loaders require.
 */


#ifndef PLUGINLDR3D_H
#define PLUGINLDR3D_H

#include "../pluginldr.h"

class SCENEGRAPH;

// typedefs of the functions exported by the 3D Plugin Class
typedef int (*PLUGIN_3D_GET_N_EXTENSIONS) ( void );

typedef char const* (*PLUGIN_3D_GET_MODEL_EXTENSION) ( int aIndex );

typedef int (*PLUGIN_3D_GET_N_FILTERS) ( void );

typedef char const* (*PLUGIN_3D_GET_FILE_FILTER) ( int aIndex );

typedef bool (*PLUGIN_3D_CAN_RENDER) ( void );

typedef SCENEGRAPH* (*PLUGIN_3D_LOAD) ( char const* aFileName );


class KICAD_PLUGIN_LDR_3D : public KICAD_PLUGIN_LDR
{
public:
    KICAD_PLUGIN_LDR_3D();
    virtual ~KICAD_PLUGIN_LDR_3D();

    bool Open( const wxString& aFullFileName ) override;

    void Close( void ) override;

    void GetLoaderVersion( unsigned char* Major, unsigned char* Minor,
                           unsigned char* Revision, unsigned char* Patch ) const override;

    int GetNExtensions( void );

    char const* GetModelExtension( int aIndex );

    int GetNFilters( void );

    char const* GetFileFilter( int aIndex );

    bool CanRender( void );

    SCENEGRAPH* Load( char const* aFileName );

private:
    bool ok;    // set TRUE if all functions are linked
    PLUGIN_3D_GET_N_EXTENSIONS      m_getNExtensions;
    PLUGIN_3D_GET_MODEL_EXTENSION   m_getModelExtension;
    PLUGIN_3D_GET_N_FILTERS         m_getNFilters;
    PLUGIN_3D_GET_FILE_FILTER       m_getFileFilter;
    PLUGIN_3D_CAN_RENDER            m_canRender;
    PLUGIN_3D_LOAD                  m_load;
};

#endif  // PLUGINMGR3D_H
