/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2015 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PANEL_FP_PROPERTIES_3D_MODEL_H_
#define PANEL_FP_PROPERTIES_3D_MODEL_H_

#include <footprint.h>
#include <panel_fp_properties_3d_model_base.h>
#include <vector>

class DIALOG_SHIM;
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
                                  DIALOG_SHIM* aDialogParent, wxWindow* aParent,
                                  wxWindowID aId = wxID_ANY,
                                  const wxPoint& aPos = wxDefaultPosition,
                                  const wxSize& aSize = wxDefaultSize,
                                  long aStyle = wxTAB_TRAVERSAL,
                                  const wxString& aName = wxEmptyString );

    ~PANEL_FP_PROPERTIES_3D_MODEL() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void ReloadModelsFromFootprint();

    void AdjustGridColumnWidths( int aWidth );

    std::vector<FP_3DMODEL>& GetModelList()
    {
        return m_shapes3D_list;
    }

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

private:
    DIALOG_SHIM*            m_parentDialog;
    PCB_BASE_EDIT_FRAME*    m_frame;
    FOOTPRINT*              m_footprint;

    std::vector<FP_3DMODEL> m_shapes3D_list;
    PANEL_PREVIEW_3D_MODEL* m_previewPane;

    bool                    m_inSelect;
};

#endif // PANEL_FP_PROPERTIES_3D_MODEL_H_
