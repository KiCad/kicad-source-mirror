/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DIALOG_SELECT_3DMODEL_H
#define DIALOG_SELECT_3DMODEL_H

#include "dialog_select_3d_model_base.h"

class FP_3DMODEL;
class S3D_CACHE;
class FILENAME_RESOLVER;
class EDA_3D_MODEL_VIEWER;

class DIALOG_SELECT_3DMODEL : public DIALOG_SELECT_3D_MODEL_BASE
{
public:
    DIALOG_SELECT_3DMODEL( wxWindow* aParent, S3D_CACHE* aCacheManager, FP_3DMODEL* aModelItem,
                           wxString& prevModelSelectDir, int& prevModelWildcard );

    bool TransferDataFromWindow() override;
    void OnSelectionChanged( wxCommandEvent& event )override;
    void OnFileActivated( wxCommandEvent& event )override;
    void SetRootDir( wxCommandEvent& event ) override;
    void Cfg3DPaths( wxCommandEvent& event ) override;

    bool IsEmbedded3DModel() const
    {
        return m_EmbedModelCb->IsChecked();
    }

private:
    void updateDirChoiceList( void );

    FP_3DMODEL*          m_model;       // data for the selected model
    S3D_CACHE*           m_cache;       // cache manager
    FILENAME_RESOLVER*   m_resolver;    // 3D filename resolver

    wxString&            m_previousDir;

    EDA_3D_MODEL_VIEWER* m_modelViewer;
};

#endif  // DIALOG_SELECT_3DMODEL_H
