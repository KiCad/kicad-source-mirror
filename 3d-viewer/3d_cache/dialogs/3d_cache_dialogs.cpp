/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialogs/dialog_configure_paths.h>

#include "3d_info.h"
#include "3d_cache.h"
#include "3d_cache_dialogs.h"
#include "dialog_select_3d_model.h"


bool S3D::Select3DModel( wxWindow* aParent, S3D_CACHE* aCache, wxString& prevModelSelectDir,
                         int& prevModelWildcard, FP_3DMODEL* aModel )
{
    if( nullptr == aModel )
        return false;

    DIALOG_SELECT_3DMODEL dm( aParent, aCache, aModel, prevModelSelectDir, prevModelWildcard );

    // Use QuasiModal so that Configure3DPaths (and its help window) will work
    return dm.ShowQuasiModal() == wxID_OK;
}


bool S3D::Configure3DPaths( wxWindow* aParent, FILENAME_RESOLVER* aResolver )
{
    DIALOG_CONFIGURE_PATHS dlg( aParent, aResolver );

    // Use QuasiModal so that HTML help window will work
    return( dlg.ShowQuasiModal() == wxID_OK );
}
