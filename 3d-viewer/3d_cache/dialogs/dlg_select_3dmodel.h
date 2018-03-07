/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file dlg_select_3dmodel.h
 * creates a dialog to select 3D model files
 */


#ifndef DLG_SELECT_3DMODEL_H
#define DLG_SELECT_3DMODEL_H

#include <wx/event.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/dirctrl.h>
#include <wx/sizer.h>
#include <wx/frame.h>

#include <dialog_shim.h>

class MODULE_3D_SETTINGS;
class S3D_CACHE;
class FILENAME_RESOLVER;
class C3D_MODEL_VIEWER;

class DLG_SELECT_3DMODEL : public DIALOG_SHIM
{
private:
    MODULE_3D_SETTINGS* m_model;        // data for the selected model
    S3D_CACHE* m_cache;                 // cache manager
    FILENAME_RESOLVER*  m_resolver; // 3D filename resolver

    wxString& m_previousDir;
    int&      m_previousFilterIndex;

    wxGenericDirCtrl* m_FileTree;
    C3D_MODEL_VIEWER* m_modelViewer;
    wxChoice*         dirChoices;

    void updateDirChoiceList( void );

public:
    DLG_SELECT_3DMODEL( wxWindow* aParent, S3D_CACHE* aCacheManager, MODULE_3D_SETTINGS* aModelItem,
        wxString& prevModelSelectDir, int& prevModelWildcard );

    bool TransferDataFromWindow() override;
    void OnSelectionChanged( wxTreeEvent& event );
    void OnFileActivated( wxTreeEvent& event );
    void SetRootDir( wxCommandEvent& event );
    void Cfg3DPaths( wxCommandEvent& event );

    wxDECLARE_EVENT_TABLE();
};

#endif  // DLG_SELECT_3DMODEL_H
