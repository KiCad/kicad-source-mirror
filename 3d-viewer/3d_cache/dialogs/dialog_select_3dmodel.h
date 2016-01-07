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

/**
 * @file dialog_select_3dmodel.h
 * creates a dialog to select 3D model files
 */

#ifndef DIALOG_SELECT_3DMODEL_H
#define DIALOG_SELECT_3DMODEL_H

#include <wx/wx.h>
#include <wx/filedlg.h>

class S3D_CACHE;
struct S3D_INFO;

class DLG_SEL_3DMODEL : public wxFileDialog
{
private:
    S3D_CACHE* m_manager;

public:
    DLG_SEL_3DMODEL( wxWindow* aParent, S3D_CACHE* aManager,
        const wxString& aDefaultDir, int aFilterIndex );

    // Retrieve model data
    void GetModelData( S3D_INFO* aModel );

private:
    void OnExit( wxCommandEvent& event );
    void OnOK( wxCommandEvent& event );

    wxDECLARE_EVENT_TABLE();
};

#endif  // DIALOG_SELECT_3DMODEL_H
