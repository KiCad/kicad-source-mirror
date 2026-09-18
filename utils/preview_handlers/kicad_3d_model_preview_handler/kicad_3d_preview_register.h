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

#ifndef KICAD_3D_PREVIEW_REGISTER_H
#define KICAD_3D_PREVIEW_REGISTER_H

#include <windows.h>

HRESULT RegisterInprocServer( PCWSTR aPszModule, const CLSID& aClsid, PCWSTR aPszFriendlyName,
                              PCWSTR aPszThreadModel, const GUID& aAppId );


HRESULT UnregisterInprocServer( PCWSTR aPszModule, const CLSID& aClsid, const GUID& aAppId );


HRESULT RegisterShellExtPreviewHandler( PCWSTR aPszFileTypes[], size_t aPszFileTypesCount, const CLSID& aClsid,
                                        PCWSTR aPszDescription );


HRESULT UnregisterShellExtPreviewHandler( PCWSTR aPszFileType, const CLSID& aClsid );

#endif // KICAD_3D_PREVIEW_REGISTER_H
