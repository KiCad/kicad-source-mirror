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

#include <kicad_3d_preview_class_factory.h>
#include <kicad_3d_preview_handler.h>
#include <new>
#include <Shlwapi.h>

#pragma comment( lib, "shlwapi.lib" )

extern long g_cRefModule;

KICAD_3D_PREVIEW_CLASS_FACTORY::KICAD_3D_PREVIEW_CLASS_FACTORY() : m_cRef( 1 )
{
    InterlockedIncrement( &g_cRefModule );
}

KICAD_3D_PREVIEW_CLASS_FACTORY::~KICAD_3D_PREVIEW_CLASS_FACTORY()
{
    InterlockedDecrement( &g_cRefModule );
}

IFACEMETHODIMP KICAD_3D_PREVIEW_CLASS_FACTORY::QueryInterface( REFIID aRiid, void** aPpv )
{
    static const QITAB qit[] = {
        QITABENT(KICAD_3D_PREVIEW_CLASS_FACTORY, IClassFactory),
        { 0 },
    };
    return QISearch( this, qit, aRiid, aPpv );
}

IFACEMETHODIMP_(ULONG) KICAD_3D_PREVIEW_CLASS_FACTORY::AddRef()
{
    return InterlockedIncrement( &m_cRef );
}

IFACEMETHODIMP_(ULONG) KICAD_3D_PREVIEW_CLASS_FACTORY::Release()
{
    ULONG cRef = InterlockedDecrement( &m_cRef );

    if ( 0 == cRef )
    {
        delete this;
    }

    return cRef;
}

IFACEMETHODIMP KICAD_3D_PREVIEW_CLASS_FACTORY::CreateInstance( IUnknown* aPunkOuter, REFIID aRiid, void** aPpv )
{
    HRESULT hr = CLASS_E_NOAGGREGATION;

    if( aPunkOuter == NULL )
    {
        hr = E_OUTOFMEMORY;

        KICAD_3D_PREVIEW_HANDLER* pExt = new(std::nothrow) KICAD_3D_PREVIEW_HANDLER();

        if( pExt )
        {
            hr = pExt->QueryInterface( aRiid, aPpv );
            pExt->Release();
        }
    }

    return hr;
}

IFACEMETHODIMP KICAD_3D_PREVIEW_CLASS_FACTORY::LockServer(BOOL aLock)
{
    if( aLock )
    {
        InterlockedIncrement( &g_cRefModule );
    }
    else
    {
        InterlockedDecrement( &g_cRefModule );
    }

    return S_OK;
}