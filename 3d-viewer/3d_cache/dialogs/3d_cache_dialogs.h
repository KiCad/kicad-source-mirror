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

#ifndef CACHE_DIALOGS_3D_H
#define CACHE_DIALOGS_3D_H

#include <wx/window.h>

class S3D_CACHE;
class S3D_FILENAME_RESOLVER;
struct S3D_INFO;

namespace S3D
{
    bool Select3DModel( wxWindow* aParent, S3D_CACHE* aCache,
        wxString& prevModelSelectDir, int& prevModelWildcard, S3D_INFO* aModel );

    bool Configure3DPaths( wxWindow* aParent, S3D_FILENAME_RESOLVER* aResolver );
};

#endif  // CACHE_DIALOGS_3D_H
