/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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


#include <common.h>
#include <pgm_base.h>
#include "3d_cache_wrapper.h"

static wxCriticalSection lock3D_wrapper;

CACHE_WRAPPER::CACHE_WRAPPER()
{
    return;
}


CACHE_WRAPPER::~CACHE_WRAPPER()
{
    return;
}


S3D_CACHE* PROJECT::Get3DCacheManager( bool updateProjDir )
{
    wxCriticalSectionLocker lock( lock3D_wrapper );
    CACHE_WRAPPER* cw = (CACHE_WRAPPER*) GetElem( ELEM_3DCACHE );
    S3D_CACHE* cache = dynamic_cast<S3D_CACHE*>( cw );

    // check that we get the expected type of object or NULL
    wxASSERT( !cw || cache );

    if( !cw )
    {
        cw = new CACHE_WRAPPER;
        cache = dynamic_cast<S3D_CACHE*>( cw );

        wxFileName cfgpath;
        cfgpath.AssignDir( GetKicadConfigPath() );
        cfgpath.AppendDir( wxT( "3d" ) );
        cache->SetProgramBase( &Pgm() );
        cache->Set3DConfigDir( cfgpath.GetFullPath() );
        SetElem( ELEM_3DCACHE, cw );
        updateProjDir = true;
    }

    if( updateProjDir )
        cache->SetProjectDir( GetProjectPath() );

    return cache;
}
