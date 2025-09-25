/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2015 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
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

#pragma once

#include <footprint.h>
#include <panel_fp_properties_3d_model_base.h>
#include <vector>

class DIALOG_SHIM;
class PANEL_EMBEDDED_FILES;
class PANEL_PREVIEW_3D_MODEL;
class PCB_BASE_EDIT_FRAME;

enum class MODEL_VALIDATE_ERRORS
{
    MODEL_NO_ERROR,
    RESOLVE_FAIL,
    OPEN_FAIL,
    NO_FILENAME,
    ILLEGAL_FILENAME
};

class PANEL_FP_PROPERTIES_3D_MODEL : public PANEL_FP_PROPERTIES_3D_MODEL_BASE
{

public:
    PANEL_FP_PROPERTIES_3D_MODEL( PCB_BASE_EDIT_FRAME* aFrame, FOOTPRINT* aFootprint,
                                  DIALOG_SHIM* aDialogParent, PANEL_EMBEDDED_FILES* aFilesPanel,
                                  wxWindow* aParent );

    ~PANEL_FP_PROPERTIES_3D_MODEL() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void ReloadModelsFromFootprint();

    std::vector<FP_3DMODEL>& GetModelList() { return m_shapes3D_list; }

private:
    // virtual event functions
    void On3DModelSelected( wxGridEvent&  ) override;
    void On3DModelCellChanged( wxGridEvent& aEvent ) override;
    void OnRemove3DModel( wxCommandEvent& event ) override;
    void OnAdd3DModel( wxCommandEvent& event ) override;
    void OnAdd3DRow( wxCommandEvent& event ) override;
    void Cfg3DPath( wxCommandEvent& event ) override;

    void OnUpdateUI( wxUpdateUIEvent& event ) override;

    void updateValidateStatus( int aRow );

    MODEL_VALIDATE_ERRORS validateModelExists( const wxString& aFilename );

    void select3DModel( int aModelIdx );

    void onModify();

    virtual void onDialogActivateEvent( wxActivateEvent& aEvent );
    virtual void onShowEvent( wxShowEvent& aEvent );

    // Wrapper on creating and posting custom event
    void postCustomPanelShownEventWithPredicate( bool predicate );

private:
    DIALOG_SHIM*            m_parentDialog;
    PCB_BASE_EDIT_FRAME*    m_frame;
    FOOTPRINT*              m_footprint;

    std::vector<FP_3DMODEL> m_shapes3D_list;
    PANEL_PREVIEW_3D_MODEL* m_previewPane;
    PANEL_EMBEDDED_FILES*   m_filesPanel;

    bool                    m_inSelect;
};
