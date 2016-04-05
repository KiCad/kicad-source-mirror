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

#include <wx/filename.h>

#include "3d_info.h"
#include "3d_cache.h"
#include "plugins/3dapi/ifsg_api.h"
#include "3d_cache_dialogs.h"
#include "dlg_3d_pathconfig.h"
#include "dlg_select_3dmodel.h"


bool S3D::Select3DModel( wxWindow* aParent, S3D_CACHE* aCache,
    wxString& prevModelSelectDir, int& prevModelWildcard, S3D_INFO* aModel )
{
    if( NULL == aModel )
        return false;

    DLG_SELECT_3DMODEL* dm = new DLG_SELECT_3DMODEL( aParent, aCache, aModel,
        prevModelSelectDir, prevModelWildcard );

    if( wxID_OK == dm->ShowModal() )
    {
        delete dm;
        return true;
    }

    delete dm;
    return false;
}


bool S3D::Configure3DPaths( wxWindow* aParent, S3D_FILENAME_RESOLVER* aResolver )
{
    DLG_3D_PATH_CONFIG* dp = new DLG_3D_PATH_CONFIG( aParent, aResolver );

    if( wxID_OK == dp->ShowModal() )
    {
        delete dp;
        return true;
    }

    delete dp;
    return false;
}
