/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "project_pcb.h"
#include <fp_lib_table.h>
#include <project.h>
#include <confirm.h>
#include <pgm_base.h>
#include <3d_cache/3d_cache.h>
#include <paths.h>
#include <settings/common_settings.h>

#include <mutex>

static std::mutex mutex3D_cacheManager;

FP_LIB_TABLE* PROJECT_PCB::PcbFootprintLibs( PROJECT* aProject )
{
    // This is a lazy loading function, it loads the project specific table when
    // that table is asked for, not before.

    FP_LIB_TABLE* tbl = (FP_LIB_TABLE*) aProject->GetElem( PROJECT::ELEM::FPTBL );

    // its gotta be NULL or a FP_LIB_TABLE, or a bug.
    wxASSERT( !tbl || tbl->ProjectElementType() == PROJECT::ELEM::FPTBL );

    if( !tbl )
    {
        // Stack the project specific FP_LIB_TABLE overlay on top of the global table.
        // ~FP_LIB_TABLE() will not touch the fallback table, so multiple projects may
        // stack this way, all using the same global fallback table.
        tbl = new FP_LIB_TABLE( &GFootprintTable );

        aProject->SetElem( PROJECT::ELEM::FPTBL, tbl );

        wxString projectFpLibTableFileName = aProject->FootprintLibTblName();

        try
        {
            tbl->Load( projectFpLibTableFileName );
        }
        catch( const IO_ERROR& ioe )
        {
            DisplayErrorMessage( nullptr, _( "Error loading project footprint libraries." ),
                                 ioe.What() );
        }
        catch( ... )
        {
            DisplayErrorMessage( nullptr, _( "Error loading project footprint library table." ) );
        }
    }

    return tbl;
}


S3D_CACHE* PROJECT_PCB::Get3DCacheManager( PROJECT* aProject, bool aUpdateProjDir )
{
    std::lock_guard<std::mutex> lock( mutex3D_cacheManager );

    // Get the existing cache from the project
    S3D_CACHE* cache = static_cast<S3D_CACHE*>( aProject->GetElem( PROJECT::ELEM::S3DCACHE ) );

    if( !cache )
    {
        // Create a cache if there is not one already
        cache = new S3D_CACHE();

        wxFileName cfgpath;
        cfgpath.AssignDir( PATHS::GetUserSettingsPath() );
        cfgpath.AppendDir( wxT( "3d" ) );

        cache->SetProgramBase( &Pgm() );
        cache->Set3DConfigDir( cfgpath.GetFullPath() );

        aProject->SetElem( PROJECT::ELEM::S3DCACHE, cache );
        aUpdateProjDir = true;
    }

    if( aUpdateProjDir )
        cache->SetProject( aProject );

    return cache;
}


FILENAME_RESOLVER* PROJECT_PCB::Get3DFilenameResolver( PROJECT* aProject )
{
    return Get3DCacheManager( aProject )->GetResolver();
}


void PROJECT_PCB::Cleanup3DCache( PROJECT* aProject )
{
    std::lock_guard<std::mutex> lock( mutex3D_cacheManager );

    // Get the existing cache from the project
    S3D_CACHE* cache = static_cast<S3D_CACHE*>( aProject->GetElem( PROJECT::ELEM::S3DCACHE ) );

    if( cache )
    {
        // We'll delete ".3dc" cache files older than this many days
        int clearCacheInterval = 0;

        if( Pgm().GetCommonSettings() )
            clearCacheInterval = Pgm().GetCommonSettings()->m_System.clear_3d_cache_interval;

        // An interval of zero means the user doesn't want to ever clear the cache

        if( clearCacheInterval > 0 )
            cache->CleanCacheDir( clearCacheInterval );
    }
}