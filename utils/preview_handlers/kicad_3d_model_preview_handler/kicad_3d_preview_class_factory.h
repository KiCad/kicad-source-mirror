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

#ifndef KICAD_3D_PREVIEW_CLASS_FACTORY_H
#define KICAD_3D_PREVIEW_CLASS_FACTORY_H

#include <windows.h>
#include <objbase.h>

class KICAD_3D_PREVIEW_CLASS_FACTORY: public IClassFactory
{
public:
    IFACEMETHODIMP QueryInterface( REFIID aRiid, void** aPpv );
    IFACEMETHODIMP_(ULONG) AddRef();
    IFACEMETHODIMP_(ULONG) Release();

    IFACEMETHODIMP CreateInstance( IUnknown* aPunkOuter, REFIID aRiid, void** aPpv );
    IFACEMETHODIMP LockServer( BOOL aLock );

    KICAD_3D_PREVIEW_CLASS_FACTORY();

protected:
    ~KICAD_3D_PREVIEW_CLASS_FACTORY();

private:
    long m_cRef;
};

#endif // KICAD_3D_PREVIEW_CLASS_FACTORY_H