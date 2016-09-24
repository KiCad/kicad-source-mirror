/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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


#ifndef DLG_3D_PATHCONFIG_H
#define DLG_3D_PATHCONFIG_H

#include <wx/valtext.h>
#include "dlg_3d_pathconfig_base.h"

class S3D_FILENAME_RESOLVER;

class DLG_3D_PATH_CONFIG : public DLG_3D_PATH_CONFIG_BASE
{
private:
    S3D_FILENAME_RESOLVER* m_resolver;
    wxString m_curdir;
    wxTextValidator m_aliasValidator;

    void initDialog();

    void OnAddAlias( wxCommandEvent& event ) override;
    void OnDelAlias( wxCommandEvent& event ) override;
    void OnAliasMoveUp( wxCommandEvent& event ) override;
    void OnAliasMoveDown( wxCommandEvent& event ) override;
    void OnConfigEnvVar( wxCommandEvent& event ) override;
    void OnHelp( wxCommandEvent& event ) override;

public:
    DLG_3D_PATH_CONFIG( wxWindow* aParent, S3D_FILENAME_RESOLVER* aResolver );
    bool TransferDataFromWindow() override;

private:
    void updateEnvVars( void );
};

#endif  // DLG_3D_PATHCONFIG_H
