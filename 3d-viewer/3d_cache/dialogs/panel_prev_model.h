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
 * @file panel_prev_model.h
 * defines a panel which is to be added to a wxFileDialog via SetExtraControl();
 * the panel shows a preview of the model being browsed (if preview is supported
 * by a plugin) and provides controls to set the offset/rotation/scale of the
 * model as per KiCad's current behavior. The panel may also be used in the 3D
 * configuration dialog to tune the positioning of the models without invoking
 * a file selector dialog.
 */

#ifndef PANEL_PREV_MODEL_H
#define PANEL_PREV_MODEL_H

#include <wx/panel.h>
#include <wx/textctrl.h>

#include "plugins/3dapi/c3dmodel.h"
#include "3d_cache/3d_info.h"

class S3D_CACHE;
class S3D_FILENAME_RESOLVER;
class C3D_MODEL_VIEWER;
class wxGenericDirCtrl;

class PANEL_PREV_3D : public wxPanel
{
public:
    PANEL_PREV_3D( wxWindow* aParent, S3D_CACHE* aCacheManager );
    ~PANEL_PREV_3D();

    // 3D views
    void View3DISO( wxCommandEvent& event );
    void View3DUpdate( wxCommandEvent& event );
    void View3DLeft( wxCommandEvent& event );
    void View3DRight( wxCommandEvent& event );
    void View3DFront( wxCommandEvent& event );
    void View3DBack( wxCommandEvent& event );
    void View3DTop( wxCommandEvent& event );
    void View3DBottom( wxCommandEvent& event );
    // Set / Retrieve model data
    void SetModelData( S3D_INFO const* aModel );
    void GetModelData( S3D_INFO* aModel );
    void UpdateModelName( wxString const& aModel );
    // Update on change of FileDlg selection
    virtual void UpdateWindowUI( long flags = wxUPDATE_UI_NONE );

private:
    wxString currentModelFile;
    S3D_CACHE* m_ModelManager;
    S3D_FILENAME_RESOLVER* m_resolver;
    wxGenericDirCtrl* m_FileTree;
    wxTextCtrl* xscale;
    wxTextCtrl* yscale;
    wxTextCtrl* zscale;
    wxTextCtrl* xrot;
    wxTextCtrl* yrot;
    wxTextCtrl* zrot;
    wxTextCtrl* xoff;
    wxTextCtrl* yoff;
    wxTextCtrl* zoff;
    wxPanel* preview;
    C3D_MODEL_VIEWER* canvas;
    S3DMODEL* model;
    S3D_INFO  modelInfo;


private:
    void updateOrientation( wxCommandEvent &event );

    void getOrientationVars( SGPOINT& scale, SGPOINT& rotation, SGPOINT& offset );

    wxDECLARE_EVENT_TABLE();
};

#endif  // PANEL_PREV_MODEL_H
